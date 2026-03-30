/**
 * @file  I18nEn.h
 * @brief English — i18n string table.
 */

#pragma once
#include "I18n.h"

static const I18nStrings __in_flash("i18n") i18n_en = {
    // Boot
    "OPEN (capturing)",
    "LOCKED \xE2\x80\x94 LOGIN <password>",
    "AUTO-UNLOCKED",
    "Type HELP for commands.",

    // Errors
    "[ERROR] Locked. LOGIN first.",
    "[ERROR] Wrong password.",
    "[ERROR] Wrong PIN.",
    "[ERROR] System not unlocked.",
    "[ERROR] Command too long, discarded.",
    "Unknown command. Type HELP.",
    "[ERROR] Password too long (max %u chars).",
    "[ERROR] PIN must be exactly 4 digits.",
    "[ERROR] PIN must contain only digits 0-9.",
    "[INFO] Already locked. Use PASS or UNLOCK.",
    "[INFO] Already open.",
    "[ERROR] Failed. System may already be locked.",
    "[ERROR] Wrong old password or not unlocked.",
    "[ERROR] Wrong PIN or no PIN set.",

    // Operations
    "--- LOCKING SYSTEM WITH USER PASSWORD ---",
    "--- SYSTEM LOCKED ---",
    "[INFO] DEK wiped from RAM. Capture PAUSED.",
    "[INFO] Next boot will require LOGIN <password>.",
    "[INFO] Use LOGIN <password> to resume capture now.",
    "Unlocking... (KDF computation, ~200 ms)",
    "UNLOCKED \xE2\x80\x94 Capture active",
    "--- REMOVING USER PASSWORD ---",
    "--- SYSTEM UNLOCKED (password-free mode) ---",
    "--- CHANGING PASSWORD ---",
    "--- PASSWORD CHANGED SUCCESSFULLY ---",
    "--- ERASING FLASH LOG ---",
    "--- ERASE COMPLETE ---",
    "[INFO] Password removed. Open mode. Capture active.",
    "--- READ PIN SET ---",
    "--- READ PIN REMOVED ---",
    "[INFO] Hashing PIN (KDF, ~10 ms)...",
    "[INFO] DUMP, HEX, EXPORT, RAWDUMP now require PIN.",
    "\n--- CANCELLED ---",
    "[BUSY] Operation in progress. Send STOP to cancel.",

    // Info
    "[INFO] Already unlocked. Capturing to flash.",
    "[INFO] No password set. System is open.",
    "[INFO] ABNT2 keyboard detected \xE2\x80\x94 layout auto-switched.",
    "[WARN] Capture will pause until LOGIN.",

    // Lockout
    "[LOCKOUT] Too many failures. Wait %lu seconds.",

    // LIVE
    "--- LIVE MODE ON ---",
    "--- LIVE MODE OFF ---",
    "[WARN] System locked \xE2\x80\x94 LIVE will activate after LOGIN.",

    // HELP
    "--- COMMANDS (Locked Mode) ---",
    "--- COMMANDS (Open Mode) ---",

    // LANG
    "Language changed to %s.",
    "[ERROR] Unknown language. Type LANG for list.",

    // Dump/Export headers
    "--- TEXT DUMP START ---",
    "--- HEX DUMP START ---",
    "--- RAWDUMP START ---",
    "[ERROR] Failed to set read PIN.",

    // Help descriptions, usage, layout/lang UI, format messages
    "Unlock for capture",
    "Text dump",
    "Hex dump",
    "CSV export (last N optional)",
    "Raw encrypted pages",
    "Change password",
    "Remove password",
    "Set password",
    "Set 4-digit read PIN",
    "Remove read PIN",
    "Show/change keyboard layout",
    "Show/change language",
    "Toggle real-time streaming",
    "Set Unix timestamp",
    "Erase all data + reset",
    "Cancel active operation",
    "System statistics",
    "This help message",
    "<password>",
    "<pin>",
    "<old> <new>",
    "--- KEYBOARD LAYOUTS ---",
    "--- Use: LAYOUT <name> to change ---",
    "--- LAYOUT CHANGED TO %s ---",
    "[ERROR] Unknown layout: '%s'",
    "--- LANGUAGES ---",
    "[ERROR] Wrong PIN. (%u/%u)",
    "[ERROR] Wrong password. (%u/%u)",
    "[INFO] Skipping %lu events...",
    "[INFO] Current epoch: %lu",
    "  %lu / %lu sectors...",
    "--- TIME SET: epoch=%lu ---",
    "[USAGE] RPIN SET <pin> | RPIN CLEAR <pin>",
    "[INFO] Read PIN: %s",
    "[USAGE] TIME <unix_epoch_seconds>",
    "[SAFETY] Erases ALL data. Type 'DEL CONFIRM'."
};
