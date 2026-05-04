# Embedded Firmware Skeleton

This folder shows the intended STM32U5A5 + ThreadX firmware shape for the burst acquisition requirement.

The key design is:

```text
ThreadX acquisition thread
  -> start PSSI/DCMI + DMA capture
  -> wait for DMA-complete semaphore
  -> queue completed buffer to storage thread
  -> sleep until the next 30 ms period
ThreadX storage thread
  -> receive queued buffer
  -> write 2046 12-bit samples to RAM record store
  -> release capture buffer
```

Important files:

- `include/acquisition.h`: acquisition constants, states, status, and ThreadX entry point.
- `src/acquisition_thread.c`: timing-sensitive acquisition loop.
- `include/storage_thread.h`: completed-buffer queue contract.
- `src/storage_thread.c`: RAM-backed storage thread.
- `include/platform_stm32u5.h`: HAL boundary.
- `src/platform_stm32u5_hal.c`: STM32U5A5 HAL-shaped capture/sleep mapping.
- `include/memory_store.h`: RAM record format.
- `src/memory_store.c`: ring buffer storage with CRC32.
- `src/main_threadx.c`: ThreadX object creation.

This is not a generated CubeMX project yet. It is the interview-sized architecture that would be dropped into a CubeMX/IAR project once board details are fixed.
