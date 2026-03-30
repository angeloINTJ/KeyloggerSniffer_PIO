/**
 * @file  LayoutRegistry.h
 * @brief Runtime keyboard layout registry — extern declarations.
 */

#pragma once
#include "LayoutCommon.h"

// =============================================================================
// LAYOUT IDs — Fixed order, persisted in flash
// =============================================================================

#define LAYOUT_ID_US      0
#define LAYOUT_ID_UK      1
#define LAYOUT_ID_DE      2
#define LAYOUT_ID_FR      3
#define LAYOUT_ID_ES      4
#define LAYOUT_ID_IT      5
#define LAYOUT_ID_PT      6
#define LAYOUT_ID_ABNT2   7
#define LAYOUT_ID_NORDIC  8
#define LAYOUT_ID_JP      9

/** @brief Default layout for first boot. */
#define LAYOUT_ID_DEFAULT LAYOUT_ID_ABNT2

/** @brief Total number of available layouts. */
#define LAYOUT_COUNT      10

// =============================================================================
// LAYOUT DESCRIPTOR
// =============================================================================

/**
 * @struct LayoutDescriptor
 * @brief  Descriptor for a keyboard layout: display name + table pointer.
 */
struct LayoutDescriptor {
    const char*       name;      ///< Short name for display (e.g. "ABNT2")
    const KeyMapping* key_map;   ///< Pointer to the KeyMapping array
    uint8_t           map_size;  ///< Number of entries in the array
};

// =============================================================================
// REGISTRY TABLE — Extern declaration (defined in layout_registry.cpp)
// =============================================================================

extern const LayoutDescriptor layout_registry[LAYOUT_COUNT];
