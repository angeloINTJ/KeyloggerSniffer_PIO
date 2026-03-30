/**
 * @file  LayoutDe.h
 * @brief DE keyboard layout — KeyMapping table.
 */

#pragma once
#include "LayoutCommon.h"

static const KeyMapping key_map_de[] = {
    // --- Letters (QWERTZ: y ↔ z swap) ---
    { 0x04, "a", "A", nullptr, true  },
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
    { 0x10, "m", "M", "µ",     true  },
    { 0x11, "n", "N", nullptr, true  },
    { 0x12, "o", "O", nullptr, true  },
    { 0x13, "p", "P", nullptr, true  },
    { 0x14, "q", "Q", "@",     true  },
    { 0x15, "r", "R", nullptr, true  },
    { 0x16, "s", "S", nullptr, true  },
    { 0x17, "t", "T", nullptr, true  },
    { 0x18, "u", "U", nullptr, true  },
    { 0x19, "v", "V", nullptr, true  },
    { 0x1A, "w", "W", nullptr, true  },
    { 0x1B, "x", "X", nullptr, true  },
    { 0x1C, "z", "Z", nullptr, true  },  // QWERTZ: z at y position
    { 0x1D, "y", "Y", nullptr, true  },  // QWERTZ: y at z position

    // --- Number Row ---
    { 0x1E, "1", "!", nullptr, false },
    { 0x1F, "2", "\"","²",     false },
    { 0x20, "3", "§", "³",     false },
    { 0x21, "4", "$", nullptr, false },
    { 0x22, "5", "%", nullptr, false },
    { 0x23, "6", "&", nullptr, false },
    { 0x24, "7", "/", "{",     false },
    { 0x25, "8", "(", "[",     false },
    { 0x26, "9", ")", "]",     false },
    { 0x27, "0", "=", "}",     false },

    // --- Symbols ---
    { 0x2C, " ",  " ",  nullptr, false },
    { 0x2D, "ß",  "?",  "\\",   false },  // ß / ? / backslash
    { 0x2E, "´",  "`",  nullptr, false },  // Acute / grave accent
    { 0x2F, "ü",  "Ü",  nullptr, true  },  // Umlaut ü (caps_aware)
    { 0x30, "+",  "*",  "~",     false },
    { 0x31, "#",  "'",  nullptr, false },
    { 0x33, "ö",  "Ö",  nullptr, true  },  // Umlaut ö (caps_aware)
    { 0x34, "ä",  "Ä",  nullptr, true  },  // Umlaut ä (caps_aware)
    { 0x35, "^",  "°",  nullptr, false },
    { 0x36, ",",  ";",  nullptr, false },
    { 0x37, ".",  ":",  nullptr, false },
    { 0x38, "-",  "_",  nullptr, false },
    { 0x64, "<",  ">",  "|",     false },  // Extra key left of Z
};

static constexpr uint8_t KEY_MAP_SIZE_de =
    sizeof(key_map_de) / sizeof(key_map_de[0]);
