#ifndef ACQUISITION_H
#define ACQUISITION_H

#include <stdint.h>

#include "threadx_port.h"

#define ACQ_SAMPLES_PER_BURST 2046u
#define ACQ_SAMPLE_RATE_HZ    15000000u
#define ACQ_BURST_PERIOD_MS   30u
#define ACQ_CAPTURE_TIMEOUT_MS 5u
#define ACQ_ADC_BITS          12u
#define ACQ_ADC_MAX_VALUE     4095u

typedef enum
{
    ACQ_STATE_IDLE = 0,
    ACQ_STATE_ARMED,
    ACQ_STATE_CAPTURING,
    ACQ_STATE_WRITING,
    ACQ_STATE_SLEEPING,
    ACQ_STATE_FAULT
} acq_state_t;

typedef enum
{
    ACQ_FAULT_NONE = 0,
    ACQ_FAULT_CAPTURE_START,
    ACQ_FAULT_CAPTURE_ERROR,
    ACQ_FAULT_CAPTURE_TIMEOUT,
    ACQ_FAULT_MEMORY_WRITE
} acq_fault_t;

typedef struct
{
    acq_state_t state;
    acq_fault_t last_fault;
    uint32_t bursts_captured;
    uint32_t missed_bursts;
    uint32_t samples_per_burst;
    uint32_t sample_rate_hz;
    uint32_t last_capture_tick_ms;
    uint32_t last_write_tick_ms;
} acq_status_t;

void AcquisitionThread_Entry(ULONG thread_input);
void Acquisition_OnCaptureCompleteFromIsr(void);
void Acquisition_OnCaptureErrorFromIsr(void);
const acq_status_t *Acquisition_GetStatus(void);

#endif
