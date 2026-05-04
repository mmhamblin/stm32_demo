#ifndef ACQUISITION_H
#define ACQUISITION_H

/**
 * @file acquisition.h
 * @brief Public acquisition thread interface for the STM32U5A5 burst demo.
 *
 * The acquisition module owns the timing-sensitive ThreadX loop: arm capture,
 * wait for completion, queue the completed buffer for storage, and sleep until
 * the next 30 ms period.
 *
 * @requirement SRS-ACQ-001 Acquire 2046 ADC samples per burst.
 * @requirement SRS-ACQ-002 Model a 15 MSPS acquisition rate.
 * @requirement SRS-ACQ-003 Repeat one burst every 30 ms.
 * @requirement SRS-RTOS-001 Map naturally to a ThreadX acquisition thread.
 * @requirement SRS-RTOS-002 Separate acquisition timing from storage work.
 */

#include <stdint.h>

#include "threadx_port.h"

/** @brief Number of ADC sample words captured per acquisition burst. */
#define ACQ_SAMPLES_PER_BURST 2046u

/** @brief AFE acquisition rate during the active burst window. */
#define ACQ_SAMPLE_RATE_HZ    15000000u

/** @brief Period between scheduled acquisition bursts. */
#define ACQ_BURST_PERIOD_MS   30u

/** @brief Timeout used while waiting for the DMA/capture-complete signal. */
#define ACQ_CAPTURE_TIMEOUT_MS 5u

/** @brief ADC resolution modeled by the demo. */
#define ACQ_ADC_BITS          12u

/** @brief Maximum valid 12-bit ADC sample value. */
#define ACQ_ADC_MAX_VALUE     4095u

/** @brief Ping-pong buffers used to avoid overwriting queued capture data. */
#define ACQ_BUFFER_COUNT      2u

/** @brief Acquisition state reported to status/monitoring code. */
typedef enum
{
    ACQ_STATE_IDLE = 0,
    ACQ_STATE_ARMED,
    ACQ_STATE_CAPTURING,
    ACQ_STATE_QUEUING,
    ACQ_STATE_SLEEPING,
    ACQ_STATE_FAULT
} acq_state_t;

/** @brief Fault reasons detected by the acquisition thread. */
typedef enum
{
    ACQ_FAULT_NONE = 0,
    ACQ_FAULT_CAPTURE_START,
    ACQ_FAULT_CAPTURE_ERROR,
    ACQ_FAULT_CAPTURE_TIMEOUT,
    ACQ_FAULT_NO_FREE_BUFFER,
    ACQ_FAULT_STORAGE_QUEUE
} acq_fault_t;

/** @brief Runtime acquisition status for UI/status reporting. */
typedef struct
{
    acq_state_t state;
    acq_fault_t last_fault;
    uint32_t bursts_captured;
    uint32_t missed_bursts;
    uint32_t samples_per_burst;
    uint32_t sample_rate_hz;
    uint32_t last_capture_tick_ms;
    uint32_t last_queue_tick_ms;
} acq_status_t;

/**
 * @brief ThreadX entry function for periodic burst acquisition.
 *
 * This thread waits on the 30 ms acquisition period, starts the HAL capture,
 * waits for the ISR-to-thread semaphore, queues the completed buffer for the
 * storage thread, and then enters the sleep/idle boundary.
 *
 * @param thread_input ThreadX entry parameter. Not used.
 *
 * @requirement SRS-ACQ-001
 * @requirement SRS-ACQ-003
 * @requirement SRS-MEM-001
 * @requirement SRS-RTOS-001
 * @requirement SRS-RTOS-002
 */
void AcquisitionThread_Entry(ULONG thread_input);

/**
 * @brief Signal capture completion from a HAL/DMA ISR callback.
 *
 * The ISR only wakes the ThreadX acquisition thread. The memory write and
 * status updates remain thread-owned.
 *
 * @requirement SRS-RTOS-001
 */
void Acquisition_OnCaptureCompleteFromIsr(void);

/**
 * @brief Signal capture failure from a HAL/DMA ISR callback.
 *
 * @requirement SRS-RTOS-001
 */
void Acquisition_OnCaptureErrorFromIsr(void);

/**
 * @brief Release a capture buffer after the storage thread has copied it.
 *
 * @param buffer_index Ping-pong buffer index released by the storage thread.
 *
 * @requirement SRS-RTOS-002
 */
void Acquisition_OnStorageComplete(uint32_t buffer_index);

/**
 * @brief Return a pointer to the latest acquisition status.
 *
 * @return Pointer to module-owned status storage.
 *
 * @requirement SRS-GUI-001
 */
const acq_status_t *Acquisition_GetStatus(void);

#endif
