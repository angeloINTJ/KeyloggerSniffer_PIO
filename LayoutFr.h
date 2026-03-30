/**
 * @file  LayoutFr.h
 * @brief FR keyboard layout — KeyMapping table.
 */

#pragma once
#include "LayoutCommon.h"

static const KeyMapping key_map_fr[] = {
    // --- Letters (AZERTY remapping) ---
    { 0x04, "q", "Q", nullptr, true  },  // US 'a' position → 'q'
    { 0x05, "b", "B", nullptr, true  },
    { 0x06, "c", "C", nullptr, true  },
    { 0x07, "d", "D", nullptr, true  },
    { 0x08, "e", "E", "€",     true  },
    { 0x09, "f", "F", nullptr, true  },
    { 0x0A, "g", "G", nullptr, true  },
    { 0x0B, "h", "H", nullptr, true  },
    { 0x0C, "i", "I", nullptr, true  },
    { 0x0D, "j", "J", nullptr, true  },
    { 0x0E, "k", "K", nullptr, true  },
    { 0x0F, "l", "L", nullptr, true  },
    { 0x10, ",", "?", nullptr, false },  // US 'm' pos → comma (NOT a letter!)
    { 0x11, "n", "N", nullptr, true  },
    { 0x12, "o", "O", nullptr, true  },
    { 0x13, "p", "P", nullptr, true  },
    { 0x14, "a", "A", nullptr, true  },  // US 'q' position → 'a'
    { 0x15, "r", "R", nullptr, true  },
    { 0x16, "s", "S", nullptr, true  },
    { 0x17, "t", "T", nullptr, true  },
    { 0x18, "u", "U", nullptr, true  },
    { 0x19, "v", "V", nullptr, true  },
    { 0x1A, "z", "Z", nullptr, true  },  // US 'w' position → 'z'
    { 0x1B, "x", "X", nullptr, true  },
    { 0x1C, "y", "Y", nullptr, true  },
    { 0x1D, "w", "W", nullptr, true  },  // US 'z' position → 'w'

    // --- Number Row (AZERTY: symbols unshifted, numbers shifted!) ---
    { 0x1E, "&",  "1", nullptr, false },
    { 0x1F, "é",  "2", "~",     false },
    { 0x20, "\"", "3", "#",     false },
    { 0x21, "'",  "4", "{",     false },
    { 0x22, "(",  "5", "[",     false },
    { 0x23, "-",  "6", "|",     false },
    { 0x24, "è",  "7", "`",     false },
    { 0x25, "_",  "8", "\\",    false },
    { 0x26, "ç",  "9", "^",     false },
    { 0x27, "à",  "0", "@",     false },

    // --- Symbols ---
    { 0x2C, " ",  " ",  nullptr, false },
    { 0x2D, ")",  "°",  "]",     false },
    { 0x2E, "=",  "+",  "}",     false },
    { 0x2F, "^",  "¨",  nullptr, false },  // Dead circumflex / diaeresis
    { 0x30, "$",  "£",  "¤",     false },
    { 0x31, "*",  "µ",  nullptr, false },
    { 0x33, "m",  "M",  nullptr, true  },  // US ';' pos → letter 'm'!
    { 0x34, "ù",  "%",  nullptr, false },
    { 0x35, "²",  "",   nullptr, false },  // Superscript 2
    { 0x36, ";",  ".",  nullptr, false },
    { 0x37, ":",  "/",  nullptr, false },
    { 0x38, "!",  "§",  nullptr, false },
    { 0x64, "<",  ">",  nullptr, false },
};

static constexpr uint8_t KEY_MAP_SIZE_fr =
    sizeof(key_map_fr) / sizeof(key_map_fr[0]);
