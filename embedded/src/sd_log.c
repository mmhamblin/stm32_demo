#include "sd_log.h"

/**
 * @file sd_log.c
 * @brief SD-card logging skeleton for storage-thread owned writes.
 *
 * In hardware, this file would use STM32 SDMMC plus FileX to append records.
 * The fallback implementation only tracks counters so the architecture remains
 * readable without a generated CubeMX project.
 *
 * @requirement SRS-SD-001
 */

#include "acquisition.h"

static sd_log_status_t g_sd_status;

bool SdLog_Init(void)
{
    g_sd_status.mounted = true;
    g_sd_status.records_written = 0u;
    g_sd_status.bytes_written = 0u;
    g_sd_status.write_errors = 0u;
    return true;
}

bool SdLog_WriteBurstAt(uint32_t sequence,
                        uint32_t timestamp_ms,
                        const uint16_t *samples,
                        uint32_t sample_count)
{
    (void)sequence;
    (void)timestamp_ms;

    if (!g_sd_status.mounted ||
        (samples == 0) ||
        (sample_count != ACQ_SAMPLES_PER_BURST))
    {
        g_sd_status.write_errors++;
        return false;
    }

    for (uint32_t i = 0u; i < sample_count; ++i)
    {
        if (samples[i] > ACQ_ADC_MAX_VALUE)
        {
            g_sd_status.write_errors++;
            return false;
        }
    }

    /*
     * Real implementation:
     * - format a fixed record header
     * - append header and sample payload through FileX
     * - flush/close according to the logging policy
     */
    g_sd_status.records_written++;
    g_sd_status.bytes_written += (uint32_t)(16u + (sample_count * sizeof(uint16_t)));
    return true;
}

sd_log_status_t SdLog_GetStatus(void)
{
    return g_sd_status;
}
