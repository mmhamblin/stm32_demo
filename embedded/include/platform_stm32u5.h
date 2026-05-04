#ifndef PLATFORM_STM32U5_H
#define PLATFORM_STM32U5_H

/**
 * @file platform_stm32u5.h
 * @brief STM32U5A5 HAL boundary for acquisition, timing, and sleep.
 *
 * The rest of the firmware calls this small platform API instead of directly
 * depending on generated HAL handles. That keeps acquisition logic testable
 * and makes the eventual CubeMX/IAR mapping explicit.
 *
 * @requirement SRS-HAL-001 Isolate hardware-specific behavior.
 */

#include <stdbool.h>
#include <stdint.h>

/** @brief Platform operation result. */
typedef enum
{
    PLATFORM_OK = 0,
    PLATFORM_ERROR
} platform_result_t;

/**
 * @brief Initialize board-level hardware needed by the acquisition path.
 *
 * @requirement SRS-HAL-001
 */
void Platform_InitHardware(void);

/**
 * @brief Start one AFE burst capture into the provided RAM buffer.
 *
 * The real STM32U5A5 implementation should arm PSSI or DCMI plus DMA. The CPU
 * must not poll the 15 MSPS data stream.
 *
 * @param buffer Destination for ADC samples.
 * @param sample_count Number of sample words requested.
 * @return PLATFORM_OK if capture was armed.
 *
 * @requirement SRS-ACQ-001
 * @requirement SRS-HAL-001
 */
platform_result_t Platform_StartAfeCapture(uint16_t *buffer, uint32_t sample_count);

/**
 * @brief Abort or stop an active AFE capture.
 *
 * @requirement SRS-HAL-001
 */
void Platform_StopAfeCapture(void);

/**
 * @brief Enter the platform sleep/idle boundary until a target tick.
 *
 * @param wake_tick_ms Tick value for the next acquisition period.
 *
 * @requirement SRS-ACQ-003
 * @requirement SRS-HAL-001
 */
void Platform_EnterSleepUntilTick(uint32_t wake_tick_ms);

/**
 * @brief Return the platform millisecond tick.
 *
 * @return Current tick in milliseconds.
 *
 * @requirement SRS-ACQ-003
 */
uint32_t Platform_GetTickMs(void);

#endif
