/**
 * @file  KeyEncoder.cpp
 * @brief HID report compression and layout-aware text decoder.
 *
 * Compares consecutive 8-byte HID reports and emits PackedEvents.
 * Decodes events to human-readable text using an O(1) indexed
 * scancode lookup table rebuilt on each layout change.
 */

#include "KeyEncoder.h"
#include "KeylogConfig.h"
#include "LayoutRegistry.h"

// =============================================================================
// ACTIVE LAYOUT STATE (must precede rebuildScancodeIndex)
// =============================================================================

/** @brief Pointer to the currently active layout key_map. */
static const KeyMapping* active_key_map = layout_registry[LAYOUT_ID_DEFAULT].key_map;

/** @brief Size of the active key_map. */
static uint8_t active_key_map_size = layout_registry[LAYOUT_ID_DEFAULT].map_size;

/** @brief Active layout ID (for queries and persistence). */
static uint8_t active_layout_id = LAYOUT_ID_DEFAULT;

// =============================================================================
// =============================================================================

static const KeyMapping* scancode_index[256] = {nullptr};

/**
 * @brief Rebuild the indexed lookup table from the active layout.
 *
 * @details Called on setActiveLayout() and at initialization. Clears
 *          the entire table, then inserts pointers for each scancode
 *          in the active layout's key_map.
 */
static void rebuildScancodeIndex() {
    memset(scancode_index, 0, sizeof(scancode_index));

    for (uint8_t i = 0; i < active_key_map_size; i++) {
        uint8_t sc = active_key_map[i].scancode;
        scancode_index[sc] = &active_key_map[i];
    }
}

void setActiveLayout(uint8_t layout_id) {
    if (layout_id >= LAYOUT_COUNT) {
        layout_id = LAYOUT_ID_DEFAULT;
    }
    active_layout_id    = layout_id;
    active_key_map      = layout_registry[layout_id].key_map;
    active_key_map_size = layout_registry[layout_id].map_size;

    // Rebuild the O(1) index for the new layout
    rebuildScancodeIndex();
}

uint8_t getActiveLayoutId() {
    return active_layout_id;
}

// ============================================================================
// 1. COMPRESSION (HID Report → PackedEvent)
// ============================================================================

uint8_t processHIDReport(
    const uint8_t* prev_report,
    const uint8_t* curr_report,
    uint8_t        led_status,
    PackedEvent*   events_out
) {
    uint8_t count = 0;

    // --- Build packed modifier + LED byte ---
    uint8_t packed_mods = 0;

    if (curr_report[0] & 0x11) packed_mods |= (1 << 0);  // Ctrl (L or R)
    if (curr_report[0] & 0x22) packed_mods |= (1 << 1);  // Shift (L or R)
    if (curr_report[0] & 0x04) packed_mods |= (1 << 2);  // LAlt
    if (led_status & 4)        packed_mods |= (1 << 3);   // Scroll Lock
    if (curr_report[0] & 0x40) packed_mods |= (1 << 4);   // RAlt (AltGr)
    if (led_status & 1)        packed_mods |= (1 << 5);   // Num Lock
    if (led_status & 2)        packed_mods |= (1 << 6);   // Caps Lock

    // --- Detect modifier bit changes ---
    uint8_t changed_mods = prev_report[0] ^ curr_report[0];
    if (changed_mods != 0) {
        for (int i = 0; i < 8; i++) {
            if (changed_mods & (1 << i)) {
                bool pressed = (curr_report[0] & (1 << i)) != 0;
                events_out[count].scancode = 0xE0 + i;
                events_out[count].meta     = (pressed ? 0x80 : 0x00) | packed_mods;
                count++;
            }
        }
    }

    // --- Newly pressed keys (present in curr but absent in prev) ---
    for (int i = 2; i < 8; i++) {
        uint8_t key = curr_report[i];
        if (key <= 0x03) continue;

        bool already_held = false;
        for (int j = 2; j < 8; j++) {
            if (prev_report[j] == key) {
                already_held = true;
                break;
            }
        }

        if (!already_held) {
            events_out[count].scancode = key;
            events_out[count].meta     = packed_mods | 0x80;
            count++;
        }
    }

    // --- Released keys (present in prev but absent in curr) ---
    for (int i = 2; i < 8; i++) {
        uint8_t key = prev_report[i];
        if (key <= 0x03) continue;

        bool still_held = false;
        for (int j = 2; j < 8; j++) {
            if (curr_report[j] == key) {
                still_held = true;
                break;
            }
        }

        if (!still_held) {
            events_out[count].scancode = key;
            events_out[count].meta     = packed_mods;
            count++;
        }
    }

    return count;
}

// ============================================================================
// 2. LAYOUT-GENERIC CHARACTER RESOLVER (with indexed lookup)
// ============================================================================

/**
 * @brief Map a scancode to its output character using the active layout.
 *
 * @details Uses the O(1) scancode_index table for key_map lookups
 *          instead of linear search.
 */
static void getCharFromLayout(
    uint8_t sc,
    bool shift, bool altgr, bool numlock, bool capslock, bool scroll,
    char* name
) {
    name[0] = '\0';

    // --- 1. AltGr layer: check indexed table for AltGr output ---
    if (altgr) {
        const KeyMapping* km = scancode_index[sc];
        if (km != nullptr && km->altgr != nullptr) {
            strcpy(name, km->altgr);
            return;
        }
        // No AltGr mapping found — fall through to normal lookup
    }

    // --- 2. NumPad (universal, independent of layout) ---
    if (sc >= 0x59 && sc <= 0x63) {
        uint8_t idx = sc - 0x59;
        if (numlock) {
            if (sc <= 0x61) {
                sprintf(name, "[N%d]", idx + 1);
            } else if (sc == 0x62) {
                strcpy(name, "[N0]");
            } else {
                strcpy(name, "[N.]");
            }
        } else {
            strcpy(name, numpad_nav_labels[idx]);
        }
        return;
    }

    // NumPad operators
    if (sc == 0x54) { strcpy(name, "[N/]");   return; }
    if (sc == 0x55) { strcpy(name, "[N*]");   return; }
    if (sc == 0x56) { strcpy(name, "[N-]");   return; }
    if (sc == 0x57) { strcpy(name, "[N+]");   return; }
    if (sc == 0x58) { strcpy(name, "[NENT]"); return; }

    // --- 3. Lock Toggles ---
    if (sc == 0x39) { sprintf(name, "[CAP%c]", capslock ? '-' : '+'); return; }
    if (sc == 0x53) { sprintf(name, "[NUM%c]", numlock  ? '-' : '+'); return; }
    if (sc == 0x47) { sprintf(name, "[SCR%c]", scroll   ? '-' : '+'); return; }

    // --- 4. Indexed key_map table lookup (O(1)) ---
    const KeyMapping* km = scancode_index[sc];
    if (km != nullptr) {
        bool effective_shift = km->caps_aware
                             ? (shift ^ capslock)
                             : shift;
        strcpy(name, effective_shift ? km->shifted : km->normal);
        return;
    }

    // --- 5. Function Keys (F1–F12, universal) ---
    if (sc >= 0x3A && sc <= 0x45) {
        sprintf(name, "[F%d]", sc - 0x3A + 1);
        return;
    }

    // --- 6. Navigation & Special Keys (universal) ---
    for (uint8_t i = 0; i < NAV_KEYS_SIZE; i++) {
        if (nav_keys[i].scancode == sc) {
            strcpy(name, nav_keys[i].label);
            return;
        }
    }

    // --- 7. Unknown Scancode ---
    sprintf(name, "[U:%02X]", sc);
}

// ============================================================================
// 3. PUBLIC DECODER
// ============================================================================

void decodeEventToText(const PackedEvent& event, char* output_buffer) {
    bool is_press = (event.meta & 0x80) != 0;
    bool shift    = (event.meta & (1 << 1)) != 0;
    bool scroll   = (event.meta & (1 << 3)) != 0;
    bool ralt     = (event.meta & (1 << 4)) != 0;
    bool num      = (event.meta & (1 << 5)) != 0;
    bool caps     = (event.meta & (1 << 6)) != 0;

    output_buffer[0] = '\0';

    // --- Modifier Keys (always emit tags for both press and release) ---
    if (event.scancode >= 0xE0 && event.scancode <= 0xE7) {
        int idx = event.scancode - 0xE0;

        if (is_press) {
            sprintf(output_buffer, "[%s]", modifier_names[idx]);
        } else {
            sprintf(output_buffer, "[/%s]", modifier_names[idx]);
        }
        return;
    }

    // --- Regular Keys (emit text only on press) ---
    if (is_press) {
        getCharFromLayout(event.scancode, shift, ralt, num, caps, scroll,
                          output_buffer);
    }
}
