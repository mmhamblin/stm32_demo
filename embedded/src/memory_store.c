#include "memory_store.h"

/**
 * @file memory_store.c
 * @brief RAM ring buffer for acquisition burst records.
 *
 * This module stores completed bursts in RAM, with enough metadata for a GUI
 * or diagnostic interface to identify the latest record and verify integrity.
 *
 * @requirement SRS-MEM-001
 * @requirement SRS-MEM-002
 */

#include <string.h>

#include "platform_stm32u5.h"

static memory_record_t g_records[MEMORY_STORE_RECORD_COUNT];
static memory_store_status_t g_status;
static uint32_t g_write_index;
static const memory_record_t *g_latest;

static uint32_t crc32_samples(const uint16_t *samples, uint32_t sample_count)
{
    /*
     * Software CRC32 for the skeleton. The STM32U5 CRC peripheral could replace
     * this later through the same MemoryStore_WriteBurst interface.
     */
    uint32_t crc = 0xFFFFFFFFu;

    for (uint32_t i = 0; i < sample_count; ++i)
    {
        uint16_t sample = samples[i];

        for (uint32_t byte_index = 0u; byte_index < 2u; ++byte_index)
        {
            uint8_t byte = (uint8_t)((sample >> (8u * byte_index)) & 0xFFu);
            crc ^= byte;

            for (uint32_t bit = 0u; bit < 8u; ++bit)
            {
                uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
                crc = (crc >> 1u) ^ (0xEDB88320u & mask);
            }
        }
    }

    return ~crc;
}

static bool samples_are_12_bit(const uint16_t *samples, uint32_t sample_count)
{
    for (uint32_t i = 0u; i < sample_count; ++i)
    {
        if (samples[i] > ACQ_ADC_MAX_VALUE)
        {
            return false;
        }
    }

    return true;
}

void MemoryStore_Init(void)
{
    memset(g_records, 0, sizeof(g_records));
    memset(&g_status, 0, sizeof(g_status));
    g_write_index = 0u;
    g_latest = 0;
}

bool MemoryStore_WriteBurst(const uint16_t *samples, uint32_t sample_count)
{
    return MemoryStore_WriteBurstAt(samples, sample_count, Platform_GetTickMs());
}

bool MemoryStore_WriteBurstAt(const uint16_t *samples,
                              uint32_t sample_count,
                              uint32_t timestamp_ms)
{
    if ((samples == 0) || (sample_count != ACQ_SAMPLES_PER_BURST))
    {
        return false;
    }

    if (!samples_are_12_bit(samples, sample_count))
    {
        return false;
    }

    if (g_status.records_written >= MEMORY_STORE_RECORD_COUNT)
    {
        g_status.records_overwritten++;
    }

    memory_record_t *record = &g_records[g_write_index];

    record->sequence = g_status.records_written;
    record->timestamp_ms = timestamp_ms;
    record->sample_count = sample_count;
    record->sample_rate_hz = ACQ_SAMPLE_RATE_HZ;
    memcpy(record->samples, samples, sizeof(record->samples));
    record->crc32 = crc32_samples(record->samples, sample_count);

    g_latest = record;
    g_status.latest_sequence = record->sequence;
    g_status.records_written++;
    g_write_index = (g_write_index + 1u) % MEMORY_STORE_RECORD_COUNT;

    return true;
}

const memory_record_t *MemoryStore_GetLatest(void)
{
    return g_latest;
}

memory_store_status_t MemoryStore_GetStatus(void)
{
    return g_status;
}
