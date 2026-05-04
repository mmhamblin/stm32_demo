#include "platform_stm32u5.h"

/**
 * @file platform_stm32u5_hal.c
 * @brief STM32U5A5 HAL-shaped implementation of the platform boundary.
 *
 * This file shows where CubeMX-generated HAL handles and calls would live.
 * Acquisition code calls Platform_StartAfeCapture() rather than directly
 * depending on PSSI/DCMI/DMA details.
 *
 * @requirement SRS-ACQ-001
 * @requirement SRS-HAL-001
 */

#include "acquisition.h"

/*
 * STM32U5A5 HAL boundary.
 *
 * In a generated CubeMX/IAR project this file would include STM32 HAL headers
 * and use generated handles such as hpssi, hdcmi, hdma_pssi, and timer handles.
 * Here the handles are lightweight placeholders so the intended HAL calls are
 * visible without requiring the full board project.
 */

#ifdef USE_STM32U5_HAL
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_dma.h"
#include "stm32u5xx_hal_pssi.h"

extern PSSI_HandleTypeDef hpssi;
extern DMA_HandleTypeDef hdma_pssi;
#else
typedef enum
{
    HAL_OK = 0,
    HAL_ERROR
} HAL_StatusTypeDef;

typedef struct
{
    int placeholder;
} PSSI_HandleTypeDef;

typedef struct
{
    int placeholder;
} DMA_HandleTypeDef;

static PSSI_HandleTypeDef hpssi;
static DMA_HandleTypeDef hdma_pssi;
static uint32_t g_fake_tick_ms;

static HAL_StatusTypeDef HAL_PSSI_Receive_DMA(PSSI_HandleTypeDef *handle,
                                               uint32_t *buffer,
                                               uint32_t word_count)
{
    (void)handle;
    (void)buffer;
    (void)word_count;
    return HAL_OK;
}

static void HAL_PSSI_Abort(PSSI_HandleTypeDef *handle)
{
    (void)handle;
}

static uint32_t HAL_GetTick(void)
{
    return g_fake_tick_ms++;
}
#endif

void Platform_InitHardware(void)
{
    /*
     * Real CubeMX responsibilities:
     * - system clock setup
     * - GPIO alternate functions for the AFE data bus and clock
     * - PSSI or DCMI peripheral init
     * - DMA/GPDMA init
     * - timer or ThreadX tick source for the 30 ms period
     * - low-power configuration
     */
    (void)hpssi;
    (void)hdma_pssi;
}

platform_result_t Platform_StartAfeCapture(uint16_t *buffer, uint32_t sample_count)
{
    if ((buffer == 0) || (sample_count != ACQ_SAMPLES_PER_BURST))
    {
        return PLATFORM_ERROR;
    }

    /*
     * PSSI is shown as the preferred parallel-capture mapping. If final board
     * timing is more frame/camera-like, this boundary can switch to DCMI
     * without changing acquisition_thread.c.
     *
     * Note: STM32U5 HAL versions differ in exact PSSI type names and whether
     * the DMA size parameter is expressed as bytes or words. This skeleton
     * keeps the call shape visible; the generated CubeMX project should own
     * the exact signature.
     */
    HAL_StatusTypeDef result = HAL_PSSI_Receive_DMA(&hpssi,
                                                    (uint32_t *)buffer,
                                                    sample_count);

    return (result == HAL_OK) ? PLATFORM_OK : PLATFORM_ERROR;
}

void Platform_StopAfeCapture(void)
{
    HAL_PSSI_Abort(&hpssi);
}

void Platform_EnterSleepUntilTick(uint32_t wake_tick_ms)
{
    /*
     * Real implementation options:
     * - program a low-power timer for wake_tick_ms
     * - let ThreadX idle processing enter sleep
     * - call HAL_PWR_EnterSLEEPMode(...) and wake from timer/interrupt
     *
     * The portfolio point is the power-state boundary: capture and memory write
     * finish, then the MCU can sleep until the next 30 ms acquisition period.
     */
    while ((int32_t)(Platform_GetTickMs() - wake_tick_ms) < 0)
    {
        /*
         * Real code:
         *   HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
         */
        break;
    }
}

uint32_t Platform_GetTickMs(void)
{
    return HAL_GetTick();
}

void HAL_PSSI_RxCpltCallback(PSSI_HandleTypeDef *handle)
{
    (void)handle;
    Acquisition_OnCaptureCompleteFromIsr();
}

void HAL_PSSI_ErrorCallback(PSSI_HandleTypeDef *handle)
{
    (void)handle;
    Acquisition_OnCaptureErrorFromIsr();
}
