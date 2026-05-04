#include "acquisition.h"

#include <stdbool.h>

#include "memory_store.h"
#include "platform_stm32u5.h"
#include "threadx_port.h"

static uint16_t g_adc_burst[ACQ_SAMPLES_PER_BURST];
static volatile bool g_capture_error_from_isr;

static acq_status_t g_status = {
    .state = ACQ_STATE_IDLE,
    .last_fault = ACQ_FAULT_NONE,
    .bursts_captured = 0u,
    .missed_bursts = 0u,
    .samples_per_burst = ACQ_SAMPLES_PER_BURST,
    .sample_rate_hz = ACQ_SAMPLE_RATE_HZ,
    .last_capture_tick_ms = 0u,
    .last_write_tick_ms = 0u,
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

void AcquisitionThread_Entry(ULONG thread_input)
{
    (void)thread_input;

    MemoryStore_Init();

    /*
     * The first burst runs immediately after ThreadX starts. After that, the
     * thread sleeps until the next 30 ms period.
     */
    uint32_t next_wake_ms = Platform_GetTickMs();

    while (true)
    {
        g_status.state = ACQ_STATE_ARMED;

        g_status.state = ACQ_STATE_CAPTURING;
        g_status.last_capture_tick_ms = Platform_GetTickMs();
        g_capture_error_from_isr = false;

        /*
         * Real hardware mapping:
         *   AFE5401 -> PSSI/DCMI -> DMA -> g_adc_burst
         *
         * The CPU arms the transfer and waits. It does not poll 15 MSPS data.
         */
        if (Platform_StartAfeCapture(g_adc_burst, ACQ_SAMPLES_PER_BURST) != PLATFORM_OK)
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

        g_status.state = ACQ_STATE_WRITING;

        if (!MemoryStore_WriteBurst(g_adc_burst, ACQ_SAMPLES_PER_BURST))
        {
            enter_fault(ACQ_FAULT_MEMORY_WRITE);
            continue;
        }

        g_status.bursts_captured++;
        g_status.last_fault = ACQ_FAULT_NONE;
        g_status.last_write_tick_ms = Platform_GetTickMs();

        next_wake_ms += ACQ_BURST_PERIOD_MS;
        if (tick_reached(g_status.last_write_tick_ms, next_wake_ms))
        {
            g_status.missed_bursts++;
            next_wake_ms = g_status.last_write_tick_ms + ACQ_BURST_PERIOD_MS;
        }

        g_status.state = ACQ_STATE_SLEEPING;
        Platform_EnterSleepUntilTick(next_wake_ms);
    }
}

void Acquisition_OnCaptureCompleteFromIsr(void)
{
    /*
     * HAL/DMA callbacks should stay tiny. The ThreadX thread owns validation,
     * memory writes, status counters, and power-state transitions.
     */
    (void)tx_semaphore_put(&g_capture_done_semaphore);
}

void Acquisition_OnCaptureErrorFromIsr(void)
{
    g_capture_error_from_isr = true;
    (void)tx_semaphore_put(&g_capture_done_semaphore);
}

const acq_status_t *Acquisition_GetStatus(void)
{
    return &g_status;
}
