#include "acquisition.h"

/**
 * @file main_threadx.c
 * @brief ThreadX object creation for the acquisition demo.
 *
 * The application creates an acquisition thread, a storage thread, the capture
 * semaphore used by HAL/DMA callbacks, and the queue used for buffer handoff.
 *
 * @requirement SRS-RTOS-001
 * @requirement SRS-RTOS-002
 */
#include "platform_stm32u5.h"
#include "storage_thread.h"
#include "threadx_port.h"

#define ACQUISITION_THREAD_STACK_BYTES 2048u
#define STORAGE_THREAD_STACK_BYTES 2048u
#define ACQUISITION_THREAD_PRIORITY 5u
#define STORAGE_THREAD_PRIORITY 6u
#define STORAGE_QUEUE_DEPTH 4u
#define STORAGE_QUEUE_MESSAGE_ULONGS \
    ((UINT)((sizeof(storage_request_t) + sizeof(ULONG) - 1u) / sizeof(ULONG)))

static TX_THREAD g_acquisition_thread;
static TX_THREAD g_storage_thread;
TX_SEMAPHORE g_capture_done_semaphore;
TX_QUEUE g_storage_queue;

static unsigned char g_acquisition_stack[ACQUISITION_THREAD_STACK_BYTES];
static unsigned char g_storage_stack[STORAGE_THREAD_STACK_BYTES];
static ULONG g_storage_queue_memory[STORAGE_QUEUE_DEPTH * STORAGE_QUEUE_MESSAGE_ULONGS];

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    Platform_InitHardware();

    (void)tx_semaphore_create(&g_capture_done_semaphore,
                              "capture_done",
                              0u);

    (void)tx_queue_create(&g_storage_queue,
                          "storage_queue",
                          STORAGE_QUEUE_MESSAGE_ULONGS,
                          g_storage_queue_memory,
                          sizeof(g_storage_queue_memory));

    (void)tx_thread_create(&g_storage_thread,
                           "storage",
                           StorageThread_Entry,
                           0u,
                           g_storage_stack,
                           sizeof(g_storage_stack),
                           STORAGE_THREAD_PRIORITY,
                           STORAGE_THREAD_PRIORITY,
                           TX_NO_TIME_SLICE,
                           TX_AUTO_START);

    (void)tx_thread_create(&g_acquisition_thread,
                           "acquisition",
                           AcquisitionThread_Entry,
                           0u,
                           g_acquisition_stack,
                           sizeof(g_acquisition_stack),
                           ACQUISITION_THREAD_PRIORITY,
                           ACQUISITION_THREAD_PRIORITY,
                           TX_NO_TIME_SLICE,
                           TX_AUTO_START);
}
