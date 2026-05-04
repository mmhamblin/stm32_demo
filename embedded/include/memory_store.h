#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

/**
 * @file memory_store.h
 * @brief RAM record store for completed acquisition bursts.
 *
 * This module models the "write it to memory" part of the prompt. Each
 * completed burst is stored as one record in a small RAM ring buffer.
 *
 * @requirement SRS-MEM-001 Write each completed burst to memory.
 * @requirement SRS-MEM-002 Store sequence, timestamp, count, CRC32, and payload.
 */

#include <stdbool.h>
#include <stdint.h>

#include "acquisition.h"

/** @brief Number of burst records retained in RAM before overwriting. */
#define MEMORY_STORE_RECORD_COUNT 8u

/** @brief One completed acquisition burst stored in RAM. */
typedef struct
{
    uint32_t sequence;
    uint32_t timestamp_ms;
    uint32_t sample_count;
    uint32_t sample_rate_hz;
    uint32_t crc32;
    uint16_t samples[ACQ_SAMPLES_PER_BURST];
} memory_record_t;

/** @brief RAM store counters for monitoring and diagnostics. */
typedef struct
{
    uint32_t records_written;
    uint32_t records_overwritten;
    uint32_t latest_sequence;
} memory_store_status_t;

/**
 * @brief Clear all records and reset store counters.
 *
 * @requirement SRS-MEM-001
 */
void MemoryStore_Init(void);

/**
 * @brief Store one completed acquisition burst in the RAM ring buffer.
 *
 * @param samples Pointer to 2046 12-bit sample words.
 * @param sample_count Number of sample words; must be ACQ_SAMPLES_PER_BURST.
 * @return true when the record is stored; false on invalid input.
 *
 * @requirement SRS-ACQ-001
 * @requirement SRS-ACQ-004
 * @requirement SRS-MEM-001
 * @requirement SRS-MEM-002
 */
bool MemoryStore_WriteBurst(const uint16_t *samples, uint32_t sample_count);

/**
 * @brief Store one completed acquisition burst with an explicit capture time.
 *
 * @param samples Pointer to 2046 12-bit sample words.
 * @param sample_count Number of sample words; must be ACQ_SAMPLES_PER_BURST.
 * @param timestamp_ms Tick captured when the DMA burst was armed/completed.
 * @return true when the record is stored; false on invalid input.
 *
 * @requirement SRS-MEM-002
 * @requirement SRS-RTOS-002
 */
bool MemoryStore_WriteBurstAt(const uint16_t *samples,
                              uint32_t sample_count,
                              uint32_t timestamp_ms);

/**
 * @brief Return the most recently written record.
 *
 * @return Pointer to latest record, or NULL if no record has been written.
 *
 * @requirement SRS-GUI-001
 */
const memory_record_t *MemoryStore_GetLatest(void);

/**
 * @brief Return RAM store counters.
 *
 * @return Current store status.
 *
 * @requirement SRS-GUI-001
 */
memory_store_status_t MemoryStore_GetStatus(void);

#endif
