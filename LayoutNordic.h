/**
 * @file  LayoutNordic.h
 * @brief NORDIC keyboard layout — KeyMapping table.
 */

#pragma once
#include "LayoutCommon.h"

static const KeyMapping key_map_nordic[] = {
    // --- Letters ---
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
    { 0x10, "m", "M", nullptr, true  },
    { 0x11, "n", "N", nullptr, true  },
    { 0x12, "o", "O", nullptr, true  },
    { 0x13, "p", "P", nullptr, true  },
    { 0x14, "q", "Q", nullptr, true  },
    { 0x15, "r", "R", nullptr, true  },
    { 0x16, "s", "S", nullptr, true  },
    { 0x17, "t", "T", nullptr, true  },
    { 0x18, "u", "U", nullptr, true  },
    { 0x19, "v", "V", nullptr, true  },
    { 0x1A, "w", "W", nullptr, true  },
    { 0x1B, "x", "X", nullptr, true  },
    { 0x1C, "y", "Y", nullptr, true  },
    { 0x1D, "z", "Z", nullptr, true  },

    // --- Number Row ---
    { 0x1E, "1", "!", nullptr, false },
    { 0x1F, "2", "\"","@",     false },
    { 0x20, "3", "#", "£",     false },
    { 0x21, "4", "¤", "$",     false },
    { 0x22, "5", "%", "€",     false },
    { 0x23, "6", "&", nullptr, false },
    { 0x24, "7", "/", "{",     false },
    { 0x25, "8", "(", "[",     false },
    { 0x26, "9", ")", "]",     false },
    { 0x27, "0", "=", "}",     false },

    // --- Symbols ---
    { 0x2C, " ",  " ",  nullptr, false },
    { 0x2D, "+",  "?",  "\\",    false },
    { 0x2E, "´",  "`",  nullptr, false },  // Dead acute / grave
    { 0x2F, "å",  "Å",  nullptr, true  },  // å (caps_aware)
    { 0x30, "¨",  "^",  "~",     false },  // Dead diaeresis / circumflex
    { 0x31, "'",  "*",  nullptr, false },
    { 0x33, "ö",  "Ö",  nullptr, true  },  // ö (caps_aware)
    { 0x34, "ä",  "Ä",  nullptr, true  },  // ä (caps_aware)
    { 0x35, "§",  "½",  nullptr, false },
    { 0x36, ",",  ";",  nullptr, false },
    { 0x37, ".",  ":",  nullptr, false },
    { 0x38, "-",  "_",  nullptr, false },
    { 0x64, "<",  ">",  "|",     false },
};

static constexpr uint8_t KEY_MAP_SIZE_nordic =
    sizeof(key_map_nordic) / sizeof(key_map_nordic[0]);
