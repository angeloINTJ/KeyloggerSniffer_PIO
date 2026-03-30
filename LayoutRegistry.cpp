/**
 * @file  LayoutRegistry.cpp
 * @brief Layout registry — single definition of all keyboard layout tables.
 */

#include "LayoutCommon.h"
#include "LayoutUs.h"
#include "LayoutUk.h"
#include "LayoutDe.h"
#include "LayoutFr.h"
#include "LayoutEs.h"
#include "LayoutIt.h"
#include "LayoutPt.h"
#include "LayoutAbnt2.h"
#include "LayoutNordic.h"
#include "LayoutJp.h"
#include "LayoutRegistry.h"

// =============================================================================
// REGISTRY TABLE — Single definition, indexed by LAYOUT_ID_*
// =============================================================================

// Note: On RP2040 with XIP, static const arrays in the individual
// layout_*.h files are already placed in flash by the linker. The
// __in_flash annotation here makes it explicit for the registry table.

const LayoutDescriptor __in_flash("layouts") layout_registry[LAYOUT_COUNT] = {
    /* 0 */ { "US",     key_map_us,     KEY_MAP_SIZE_us     },
    /* 1 */ { "UK",     key_map_uk,     KEY_MAP_SIZE_uk     },
    /* 2 */ { "DE",     key_map_de,     KEY_MAP_SIZE_de     },
    /* 3 */ { "FR",     key_map_fr,     KEY_MAP_SIZE_fr     },
    /* 4 */ { "ES",     key_map_es,     KEY_MAP_SIZE_es     },
    /* 5 */ { "IT",     key_map_it,     KEY_MAP_SIZE_it     },
    /* 6 */ { "PT",     key_map_pt,     KEY_MAP_SIZE_pt     },
    /* 7 */ { "ABNT2",  key_map_abnt2,  KEY_MAP_SIZE_abnt2  },
    /* 8 */ { "NORDIC", key_map_nordic, KEY_MAP_SIZE_nordic },
    /* 9 */ { "JP",     key_map_jp,     KEY_MAP_SIZE_jp     },
};
