#ifndef THREADX_PORT_H
#define THREADX_PORT_H

/*
 * Thin ThreadX include boundary.
 *
 * In the real STM32U5A5/IAR project, define USE_REAL_THREADX and include
 * Azure RTOS ThreadX normally through tx_api.h. For this portfolio skeleton,
 * the fallback declarations keep the code readable without requiring the
 * generated CubeMX project.
 */

#ifdef USE_REAL_THREADX
#include "tx_api.h"
#else
#include <stdint.h>

typedef unsigned int UINT;
typedef unsigned long ULONG;

typedef struct
{
    const char *name;
} TX_THREAD;

typedef struct
{
    const char *name;
    ULONG count;
} TX_SEMAPHORE;

#define TX_SUCCESS 0u
#define TX_AUTO_START 1u
#define TX_NO_TIME_SLICE 0u
#define TX_WAIT_FOREVER ((ULONG)0xFFFFFFFFu)

UINT tx_thread_create(TX_THREAD *thread_ptr,
                      char *name_ptr,
                      void (*entry_function)(ULONG),
                      ULONG entry_input,
                      void *stack_start,
                      ULONG stack_size,
                      UINT priority,
                      UINT preempt_threshold,
                      ULONG time_slice,
                      UINT auto_start);

UINT tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr,
                         char *name_ptr,
                         ULONG initial_count);

UINT tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option);
UINT tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr);
void tx_thread_sleep(ULONG timer_ticks);
#endif

/*
 * Placeholder conversion. In a real project, this should use the configured
 * ThreadX tick rate, for example TX_TIMER_TICKS_PER_SECOND.
 */
#define MS_TO_TICKS(ms) ((ULONG)(ms))

extern TX_SEMAPHORE g_capture_done_semaphore;

#endif
