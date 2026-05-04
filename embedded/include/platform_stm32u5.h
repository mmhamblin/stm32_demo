#ifndef PLATFORM_STM32U5_H
#define PLATFORM_STM32U5_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PLATFORM_OK = 0,
    PLATFORM_ERROR
} platform_result_t;

void Platform_InitHardware(void);
platform_result_t Platform_StartAfeCapture(uint16_t *buffer, uint32_t sample_count);
void Platform_StopAfeCapture(void);
void Platform_EnterSleepUntilTick(uint32_t wake_tick_ms);
uint32_t Platform_GetTickMs(void);

#endif
