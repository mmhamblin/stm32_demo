#include "acquisition.h"
#include "platform_stm32u5.h"
#include "threadx_port.h"

#define ACQUISITION_THREAD_STACK_BYTES 2048u
#define ACQUISITION_THREAD_PRIORITY 5u

static TX_THREAD g_acquisition_thread;
TX_SEMAPHORE g_capture_done_semaphore;

static unsigned char g_acquisition_stack[ACQUISITION_THREAD_STACK_BYTES];

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    Platform_InitHardware();

    (void)tx_semaphore_create(&g_capture_done_semaphore,
                              "capture_done",
                              0u);

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
