/**
 * @file  SnifferWorker.h
 * @brief Core 1 module — passive USB sniffer (public API).
 */

#pragma once
#include <Arduino.h>

/**
 * @brief Initialize the PIO sniffer on Core 1.
 * @note  Must be called from setup1() before entering the task loop.
 */
void setupSnifferCore1();

/**
 * @brief Core 1 main loop — processes sniffer DMA buffers + pause protocol.
 * @note  This function never returns. Call from setup1() after init.
 *        Implements the inter-core pause handshake for safe flash writes.
 *        Resides in RAM (__not_in_flash_func) to survive XIP invalidation.
 */
void runSnifferCore1();
