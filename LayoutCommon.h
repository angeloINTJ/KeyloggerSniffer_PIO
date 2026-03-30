/**
 * @file  LayoutCommon.h
 * @brief Universal HID elements shared across all keyboard layouts.
 *
 * Navigation keys, modifier names, numpad labels, and the KeyMapping
 * struct are layout-independent (based on HID Usage IDs).
 */

#pragma once
#include <Arduino.h>

// =============================================================================
// UNIFIED KEY MAPPING STRUCTURE
// =============================================================================

/**
 * @struct KeyMapping
 * @brief  Maps a HID scancode to its output characters for a specific layout.
 *
 * @details Each layout defines a table of these entries covering letters,
 *          numbers, and symbol keys. The decoder searches this table for
 *          the scancode and applies the appropriate modifier logic.
 *
 * @note    caps_aware keys use (Shift XOR CapsLock) to determine case.
 *          Non-caps_aware keys use Shift alone.
 */
struct KeyMapping {
    uint8_t     scancode;    ///< HID Usage ID
    const char* normal;      ///< Unshifted output
    const char* shifted;     ///< Shifted output
    const char* altgr;       ///< AltGr output (nullptr = none)
    bool        caps_aware;  ///< true for letters (CapsLock XOR Shift)
};

// =============================================================================
// MODIFIER KEY NAMES (universal — HID scancodes 0xE0–0xE7)
// =============================================================================

static const char* modifier_names[] = {
    "LCTL", "LSFT", "LALT", "LGUI",
    "RCTL", "RSFT", "RALT", "RGUI"
};

// =============================================================================
// NUMPAD LABELS — Num Lock OFF (navigation mode)
// =============================================================================

static const char* numpad_nav_labels[] = {
    "[NEND]", "[NDWN]", "[NPGD]", "[NLFT]", "[NCEN]",
    "[NRGT]", "[NHOM]", "[NUP]",  "[NPGU]", "[NINS]", "[NDEL]"
};

// =============================================================================
// NAVIGATION & SPECIAL KEYS (universal HID scancodes)
// =============================================================================

struct NavKeyMapping {
    uint8_t     scancode;
    const char* label;
};

static const NavKeyMapping nav_keys[] = {
    { 0x28, "[ENT]"  },
    { 0x29, "[ESC]"  },
    { 0x2A, "[BKSP]" },
    { 0x2B, "[TAB]"  },
    { 0x46, "[PRSC]" },
    { 0x48, "[PAUS]" },
    { 0x49, "[INS]"  },
    { 0x4A, "[HOME]" },
    { 0x4B, "[PGUP]" },
    { 0x4C, "[DEL]"  },
    { 0x4D, "[END]"  },
    { 0x4E, "[PGDN]" },
    { 0x4F, "[RGHT]" },
    { 0x50, "[LEFT]" },
    { 0x51, "[DOWN]" },
    { 0x52, "[UP]"   },
    { 0x65, "[MENU]" },
};

static constexpr uint8_t NAV_KEYS_SIZE =
    sizeof(nav_keys) / sizeof(nav_keys[0]);
