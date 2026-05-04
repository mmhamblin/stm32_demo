#include "storage_thread.h"

/**
 * @file storage_thread.c
 * @brief RAM-backed storage worker for completed acquisition bursts.
 *
 * Acquisition queues a completed buffer descriptor. This thread copies the
 * samples into the memory record store, updates counters, and releases the
 * ping-pong buffer back to acquisition.
 *
 * @requirement SRS-MEM-001
 * @requirement SRS-MEM-002
 * @requirement SRS-RTOS-002
 */

#include "memory_store.h"
#include "platform_stm32u5.h"

static storage_status_t g_storage_status;

bool StorageThread_EnqueueBurst(const storage_request_t *request)
{
    if ((request == 0) || (request->samples == 0))
    {
        return false;
    }

    UINT result = tx_queue_send(&g_storage_queue, (void *)request, TX_NO_WAIT);

    if (result == TX_SUCCESS)
    {
        g_storage_status.records_queued++;
        return true;
    }

    g_storage_status.queue_full_count++;
    return false;
}

void StorageThread_Entry(ULONG thread_input)
{
    (void)thread_input;

    MemoryStore_Init();

    while (true)
    {
        storage_request_t request;
        UINT result = tx_queue_receive(&g_storage_queue,
                                       &request,
                                       TX_WAIT_FOREVER);

        if (result != TX_SUCCESS)
        {
            continue;
        }

        if (MemoryStore_WriteBurstAt(request.samples,
                                     request.sample_count,
                                     request.capture_tick_ms))
        {
            g_storage_status.records_written++;
            g_storage_status.last_write_tick_ms = Platform_GetTickMs();
        }
        else
        {
            g_storage_status.records_dropped++;
        }

        Acquisition_OnStorageComplete(request.buffer_index);
    }
}

const storage_status_t *StorageThread_GetStatus(void)
{
    return &g_storage_status;
}
