#include "acquisition.h"

/**
 * @file acquisition_thread.c
 * @brief ThreadX-style acquisition loop for burst capture.
 *
 * The thread captures one 2046-sample burst every 30 ms. A HAL/DMA callback
 * releases a semaphore when the burst buffer is ready. The thread then queues
 * the completed buffer for the storage thread and enters the sleep boundary.
 *
 * @requirement SRS-ACQ-001
 * @requirement SRS-ACQ-003
 * @requirement SRS-MEM-001
 * @requirement SRS-RTOS-001
 * @requirement SRS-RTOS-002
 */

#include <stdbool.h>

#include "platform_stm32u5.h"
#include "storage_thread.h"
#include "threadx_port.h"

static uint16_t g_adc_buffers[ACQ_BUFFER_COUNT][ACQ_SAMPLES_PER_BURST];
static bool g_buffer_owned_by_storage[ACQ_BUFFER_COUNT];
static uint32_t g_next_buffer_index;
static volatile bool g_capture_error_from_isr;

static acq_status_t g_status = {
    .state = ACQ_STATE_IDLE,
    .last_fault = ACQ_FAULT_NONE,
    .bursts_captured = 0u,
    .missed_bursts = 0u,
    .samples_per_burst = ACQ_SAMPLES_PER_BURST,
    .sample_rate_hz = ACQ_SAMPLE_RATE_HZ,
    .last_capture_tick_ms = 0u,
    .last_queue_tick_ms = 0u,
};

static bool tick_reached(uint32_t now_ms, uint32_t target_ms)
{
    return ((int32_t)(now_ms - target_ms) >= 0);
}

static void enter_fault(acq_fault_t fault)
{
    g_status.state = ACQ_STATE_FAULT;
    g_status.last_fault = fault;
    g_status.missed_bursts++;
}

static bool wait_for_capture_complete(void)
{
    UINT result = tx_semaphore_get(&g_capture_done_semaphore,
                                   MS_TO_TICKS(ACQ_CAPTURE_TIMEOUT_MS));

    return (result == TX_SUCCESS);
}

static bool reserve_free_buffer(uint32_t *buffer_index)
{
    for (uint32_t attempt = 0u; attempt < ACQ_BUFFER_COUNT; ++attempt)
    {
        uint32_t candidate = (g_next_buffer_index + attempt) % ACQ_BUFFER_COUNT;

        if (!g_buffer_owned_by_storage[candidate])
        {
            *buffer_index = candidate;
            g_next_buffer_index = (candidate + 1u) % ACQ_BUFFER_COUNT;
            return true;
        }
    }

    return false;
}

void AcquisitionThread_Entry(ULONG thread_input)
{
    (void)thread_input;

    /*
     * The first burst runs immediately after ThreadX starts. After that, the
     * thread sleeps until the next 30 ms period.
     */
    uint32_t next_wake_ms = Platform_GetTickMs();

    while (true)
    {
        g_status.state = ACQ_STATE_ARMED;

        uint32_t buffer_index = 0u;
        if (!reserve_free_buffer(&buffer_index))
        {
            enter_fault(ACQ_FAULT_NO_FREE_BUFFER);
            continue;
        }

        uint16_t *capture_buffer = g_adc_buffers[buffer_index];

        g_status.state = ACQ_STATE_CAPTURING;
        g_status.last_capture_tick_ms = Platform_GetTickMs();
        g_capture_error_from_isr = false;

        /*
         * Real hardware mapping:
         *   AFE5401 -> PSSI/DCMI -> DMA -> capture_buffer
         *
         * The CPU arms the transfer and waits. It does not poll 15 MSPS data.
         */
        if (Platform_StartAfeCapture(capture_buffer, ACQ_SAMPLES_PER_BURST) != PLATFORM_OK)
        {
            enter_fault(ACQ_FAULT_CAPTURE_START);
            continue;
        }

        if (!wait_for_capture_complete())
        {
            Platform_StopAfeCapture();
            enter_fault(ACQ_FAULT_CAPTURE_TIMEOUT);
            continue;
        }

        if (g_capture_error_from_isr)
        {
            Platform_StopAfeCapture();
            enter_fault(ACQ_FAULT_CAPTURE_ERROR);
            continue;
        }

        g_status.state = ACQ_STATE_QUEUING;

        storage_request_t request = {
            .buffer_index = buffer_index,
            .capture_tick_ms = g_status.last_capture_tick_ms,
            .sample_count = ACQ_SAMPLES_PER_BURST,
            .samples = capture_buffer,
        };

        g_buffer_owned_by_storage[buffer_index] = true;

        if (!StorageThread_EnqueueBurst(&request))
        {
            g_buffer_owned_by_storage[buffer_index] = false;
            enter_fault(ACQ_FAULT_STORAGE_QUEUE);
            continue;
        }

        g_status.bursts_captured++;
        g_status.last_fault = ACQ_FAULT_NONE;
        g_status.last_queue_tick_ms = Platform_GetTickMs();

        next_wake_ms += ACQ_BURST_PERIOD_MS;
        if (tick_reached(g_status.last_queue_tick_ms, next_wake_ms))
        {
            g_status.missed_bursts++;
            next_wake_ms = g_status.last_queue_tick_ms + ACQ_BURST_PERIOD_MS;
        }

        g_status.state = ACQ_STATE_SLEEPING;
        Platform_EnterSleepUntilTick(next_wake_ms);
    }
}

void Acquisition_OnCaptureCompleteFromIsr(void)
{
    /*
     * HAL/DMA callbacks should stay tiny. Thread context owns validation,
     * storage handoff, status counters, and power-state transitions.
     */
    (void)tx_semaphore_put(&g_capture_done_semaphore);
}

void Acquisition_OnCaptureErrorFromIsr(void)
{
    g_capture_error_from_isr = true;
    (void)tx_semaphore_put(&g_capture_done_semaphore);
}

void Acquisition_OnStorageComplete(uint32_t buffer_index)
{
    if (buffer_index < ACQ_BUFFER_COUNT)
    {
        g_buffer_owned_by_storage[buffer_index] = false;
    }
}

const acq_status_t *Acquisition_GetStatus(void)
{
    return &g_status;
}
