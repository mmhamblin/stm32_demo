#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#include <stdbool.h>
#include <stdint.h>

#include "acquisition.h"

#define MEMORY_STORE_RECORD_COUNT 8u

typedef struct
{
    uint32_t sequence;
    uint32_t timestamp_ms;
    uint32_t sample_count;
    uint32_t sample_rate_hz;
    uint32_t crc32;
    uint16_t samples[ACQ_SAMPLES_PER_BURST];
} memory_record_t;

typedef struct
{
    uint32_t records_written;
    uint32_t records_overwritten;
    uint32_t latest_sequence;
} memory_store_status_t;

void MemoryStore_Init(void);
bool MemoryStore_WriteBurst(const uint16_t *samples, uint32_t sample_count);
const memory_record_t *MemoryStore_GetLatest(void);
memory_store_status_t MemoryStore_GetStatus(void);

#endif
