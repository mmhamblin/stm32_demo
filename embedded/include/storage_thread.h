#ifndef STORAGE_THREAD_H
#define STORAGE_THREAD_H

/**
 * @file storage_thread.h
 * @brief ThreadX storage-thread interface for completed acquisition buffers.
 *
 * The acquisition thread owns capture timing. The storage thread owns the
 * slower memory-record write, so burst capture is not coupled to CRC/copy work.
 *
 * @requirement SRS-MEM-001
 * @requirement SRS-RTOS-002
 */

#include <stdbool.h>
#include <stdint.h>

#include "acquisition.h"
#include "threadx_port.h"

/** @brief One completed capture buffer handed from acquisition to storage. */
typedef struct
{
    uint32_t buffer_index;
    uint32_t capture_tick_ms;
    uint32_t sample_count;
    uint16_t *samples;
} storage_request_t;

/** @brief Storage-thread counters for status and debugging. */
typedef struct
{
    uint32_t records_queued;
    uint32_t records_written;
    uint32_t records_dropped;
    uint32_t queue_full_count;
    uint32_t last_write_tick_ms;
} storage_status_t;

/**
 * @brief ThreadX entry function for RAM-backed burst storage.
 *
 * @param thread_input ThreadX entry parameter. Not used.
 *
 * @requirement SRS-MEM-001
 * @requirement SRS-MEM-002
 * @requirement SRS-RTOS-002
 */
void StorageThread_Entry(ULONG thread_input);

/**
 * @brief Queue one completed acquisition buffer for storage.
 *
 * @param request Completed-buffer descriptor copied into the ThreadX queue.
 * @return true when the descriptor is queued; false when the queue is full.
 *
 * @requirement SRS-RTOS-002
 */
bool StorageThread_EnqueueBurst(const storage_request_t *request);

/**
 * @brief Return a pointer to the latest storage-thread status.
 *
 * @return Pointer to module-owned status storage.
 *
 * @requirement SRS-GUI-001
 */
const storage_status_t *StorageThread_GetStatus(void);

#endif
