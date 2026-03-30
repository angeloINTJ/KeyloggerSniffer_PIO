/**
 * @file  I18n.h
 * @brief Internationalization — serial interface language support.
 *
 * Runtime language selection for all user-facing serial messages.
 * Technical output (STAT labels, CSV headers, hex data) stays in
 * English for tool compatibility.
 *
 * Supported: EN, PT-BR, ES, FR, DE, IT, JA, ZH, KO, RU.
 */

#pragma once
#include <Arduino.h>

// =============================================================================
// LANGUAGE IDs — Fixed order, persisted in flash
// =============================================================================

#define LANG_ID_EN    0
#define LANG_ID_PTBR  1
#define LANG_ID_ES    2
#define LANG_ID_FR    3
#define LANG_ID_DE    4
#define LANG_ID_IT    5
#define LANG_ID_JA    6
#define LANG_ID_ZH    7
#define LANG_ID_KO    8
#define LANG_ID_RU    9

#define LANG_COUNT    10
#define LANG_ID_DEFAULT LANG_ID_EN
#define LANG_ID_MAX    (LANG_COUNT - 1)

// =============================================================================
// TRANSLATABLE STRING TABLE
// =============================================================================

/**
 * @struct I18nStrings
 * @brief  All user-facing messages for a single language.
 *
 * @details Grouped by context. Command names (DUMP, LOGIN, etc.) are NOT
 *          translated — they are protocol identifiers. Only human-readable
 *          feedback messages are translated.
 */
struct I18nStrings {

    // --- Boot banner ---
    const char* boot_mode_open;         ///< "OPEN (capturing)"
    const char* boot_mode_locked;       ///< "LOCKED — LOGIN <password>"
    const char* boot_auto_unlocked;     ///< "AUTO-UNLOCKED"
    const char* boot_type_help;         ///< "Type HELP for commands."

    // --- Errors ---
    const char* err_locked;             ///< "Locked. LOGIN first."
    const char* err_wrong_pwd;          ///< "Wrong password."
    const char* err_wrong_pin;          ///< "Wrong PIN."
    const char* err_not_unlocked;       ///< "System not unlocked."
    const char* err_cmd_too_long;       ///< "Command too long, discarded."
    const char* err_unknown_cmd;        ///< "Unknown command. Type HELP."
    const char* err_pwd_too_long;       ///< "Password too long (max %u)."
    const char* err_pin_must_4;         ///< "PIN must be exactly 4 digits."
    const char* err_pin_only_digits;    ///< "PIN must contain only digits 0-9."
    const char* err_already_locked;     ///< "Already locked. Use PASS or UNLOCK."
    const char* err_already_open;       ///< "Already open."
    const char* err_lock_failed;        ///< "Failed. System may already be locked."
    const char* err_wrong_old_pwd;      ///< "Wrong old password or not unlocked."
    const char* err_wrong_pin_or_none;  ///< "Wrong PIN or no PIN set."

    // --- Operations ---
    const char* op_locking;             ///< "LOCKING SYSTEM..."
    const char* op_locked_ok;           ///< "SYSTEM LOCKED"
    const char* op_dek_wiped;           ///< "DEK wiped. Capture PAUSED."
    const char* op_login_next_boot;     ///< "Next boot requires LOGIN."
    const char* op_login_to_resume;     ///< "Use LOGIN to resume capture."
    const char* op_unlocking_kdf;       ///< "Unlocking... (KDF, ~200 ms)"
    const char* op_unlocked_ok;         ///< "UNLOCKED — Capture active"
    const char* op_removing_pwd;        ///< "REMOVING USER PASSWORD..."
    const char* op_pwd_removed;         ///< "SYSTEM UNLOCKED (password-free)"
    const char* op_changing_pwd;        ///< "CHANGING PASSWORD..."
    const char* op_pwd_changed;         ///< "PASSWORD CHANGED"
    const char* op_erasing;             ///< "ERASING FLASH LOG..."
    const char* op_erase_done;          ///< "ERASE COMPLETE"
    const char* op_open_mode;           ///< "Password removed. Open mode."
    const char* op_pin_set;             ///< "READ PIN SET"
    const char* op_pin_removed;         ///< "READ PIN REMOVED"
    const char* op_pin_hashing;         ///< "Hashing PIN (KDF, ~10 ms)..."
    const char* op_pin_required;        ///< "DUMP, HEX, EXPORT, RAWDUMP require PIN."
    const char* op_cancelled;           ///< "CANCELLED"
    const char* op_busy;                ///< "Operation in progress. STOP to cancel."

    // --- Info ---
    const char* info_already_unlocked;  ///< "Already unlocked."
    const char* info_no_pwd;            ///< "No password set."
    const char* info_abnt2_detected;    ///< "ABNT2 detected — auto-switched."
    const char* info_capture_will_pause;///< "Capture will pause until LOGIN."

    // --- Lockout ---
    const char* lockout_wait;           ///< "Too many failures. Wait %lu seconds."

    // --- LIVE mode ---
    const char* live_on;                ///< "LIVE MODE ON"
    const char* live_off;               ///< "LIVE MODE OFF"
    const char* live_warn_locked;       ///< "Locked — LIVE after LOGIN."

    // --- HELP headers ---
    const char* help_header_locked;     ///< "COMMANDS (Locked Mode)"
    const char* help_header_open;       ///< "COMMANDS (Open Mode)"

    // --- LANG ---
    const char* lang_changed;           ///< "Language changed to %s."
    const char* lang_unknown;

    // --- Dump/Export headers ---
    const char* op_text_dump_start;     ///< "TEXT DUMP START"
    const char* op_hex_dump_start;      ///< "HEX DUMP START"
    const char* op_rawdump_start;       ///< "RAWDUMP START"
    const char* err_pin_set_failed;

    // --- Help descriptions (command syntax stays English) ---
    const char* help_desc_login;      ///< "Unlock for capture"
    const char* help_desc_dump;       ///< "Text dump"
    const char* help_desc_hex;        ///< "Hex dump"
    const char* help_desc_export;     ///< "CSV export (last N optional)"
    const char* help_desc_rawdump;    ///< "Raw encrypted pages"
    const char* help_desc_pass;       ///< "Change password"
    const char* help_desc_unlock;     ///< "Remove password"
    const char* help_desc_lock;       ///< "Set password"
    const char* help_desc_rpin_set;   ///< "Set 4-digit read PIN"
    const char* help_desc_rpin_clear; ///< "Remove read PIN"
    const char* help_desc_layout;     ///< "Show/change keyboard layout"
    const char* help_desc_lang;       ///< "Show/change language"
    const char* help_desc_live;       ///< "Toggle real-time streaming"
    const char* help_desc_time;       ///< "Set Unix timestamp"
    const char* help_desc_del;        ///< "Erase all data + reset"
    const char* help_desc_stop;       ///< "Cancel active operation"
    const char* help_desc_stat;       ///< "System statistics"
    const char* help_desc_help;       ///< "This help message"

    // --- Usage argument labels ---
    const char* arg_password;         ///< "<password>"
    const char* arg_pin;              ///< "<pin>"
    const char* arg_old_new;          ///< "<old> <new>"

    // --- Layout/Lang UI ---
    const char* layout_header;        ///< "KEYBOARD LAYOUTS"
    const char* layout_footer;        ///< "Use: LAYOUT <name> to change"
    const char* layout_changed_to;    ///< "Layout changed to %s."
    const char* err_unknown_layout;   ///< "Unknown layout: '%s'"
    const char* lang_header;          ///< "LANGUAGES"

    // --- Format messages ---
    const char* err_wrong_pin_fmt;    ///< "Wrong PIN. (%u/%u)"
    const char* err_wrong_pwd_fmt;    ///< "Wrong password. (%u/%u)"
    const char* info_skipping;        ///< "Skipping %lu events..."
    const char* info_epoch;           ///< "Current epoch: %lu"
    const char* op_erase_progress;    ///< "%lu / %lu sectors..."
    const char* op_time_set;          ///< "TIME SET: epoch=%lu"
    const char* usage_rpin;           ///< "RPIN SET <pin> | RPIN CLEAR <pin>"
    const char* info_rpin_status;     ///< "Read PIN: %s"
    const char* usage_time;           ///< "[USAGE] TIME <unix_epoch>"
    const char* safety_del;           ///< "[SAFETY] Erases ALL data..."
     ///< "Failed to set read PIN."
           ///< "Unknown language."
};

// =============================================================================
// LANGUAGE DESCRIPTOR
// =============================================================================

/**
 * @struct LangDescriptor
 * @brief  Display info + string table pointer for one language.
 */
struct LangDescriptor {
    const char*        code;     ///< Short code: "EN", "PT-BR", etc.
    const char*        name;     ///< Native name: "Português (BR)"
    const I18nStrings* strings;  ///< Pointer to the string table
};

// =============================================================================
// REGISTRY — Extern declaration (defined in i18n.cpp)
// =============================================================================

extern const LangDescriptor lang_registry[LANG_COUNT];

// =============================================================================
// ACTIVE LANGUAGE — Global accessor
// =============================================================================

/** @brief Pointer to the active language's string table. */
extern const I18nStrings* i18n;

/** @brief Set the active language by ID. Invalid IDs fall back to English. */
void setActiveLanguage(uint8_t lang_id);

/** @brief Get the currently active language ID. */
uint8_t getActiveLangId();
