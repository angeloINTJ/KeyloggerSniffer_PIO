/**
 * @file  KeyEncoder.h
 * @brief HID report processor and layout-aware text decoder (public API).
 */

#pragma once
#include <Arduino.h>
#include "KeylogConfig.h"

/**
 * @brief Diff two consecutive HID keyboard reports and emit PackedEvents.
 *
 * @param[in]  prev_report  Previous 8-byte HID report (read-only).
 * @param[in]  curr_report  Current 8-byte HID report (read-only).
 * @param[in]  led_status   Current LED state (Num/Caps/Scroll Lock bits).
 * @param[out] events_out   Output array — must hold at least 14 elements
 *                          (8 modifier changes + 6 key changes).
 * @return Number of events written to events_out.
 *
 * @note  The caller is responsible for setting delta_ms on the returned
 *        events — this function only populates scancode and meta.
 */
uint8_t processHIDReport(
    const uint8_t* prev_report,
    const uint8_t* curr_report,
    uint8_t        led_status,
    PackedEvent*   events_out
);

/**
 * @brief Decode a PackedEvent into a human-readable text string.
 *
 * @param[in]  event         The packed event to decode.
 * @param[out] output_buffer Output buffer — must be at least 35 bytes.
 *                           Empty string for release events (except modifiers).
 */
void decodeEventToText(const PackedEvent& event, char* output_buffer);

/**
 * @brief Set the active keyboard layout by ID.
 *
 * @param layout_id  Layout ID (LAYOUT_ID_US .. LAYOUT_ID_JP).
 *                   Invalid IDs are silently clamped to LAYOUT_ID_DEFAULT.
 */
void setActiveLayout(uint8_t layout_id);

/**
 * @brief Get the currently active layout ID.
 * @return Layout ID (0–9).
 */
uint8_t getActiveLayoutId();
