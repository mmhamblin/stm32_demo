# Software Requirements

Short SRS-style requirements for the one-hour interview demo.

| ID | Requirement | Demo Evidence |
|---|---|---|
| SRS-ACQ-001 | The demo shall acquire 2046 ADC samples per burst. | Console output and log record sample count. |
| SRS-ACQ-002 | The demo shall model a 15 MSPS acquisition rate during the burst. | Printed burst duration is about 136.4 us. |
| SRS-ACQ-003 | The demo shall repeat one burst every 30 ms. | Loop timing and missed-burst counter. |
| SRS-ACQ-004 | The demo shall store samples as 12-bit ADC values in `uint16_t`-compatible words. | Generated samples are clamped to `0..4095`. |
| SRS-MEM-001 | The demo shall write each completed burst to a memory-style record store. | `simulator/device_memory.py` and `embedded/src/memory_store.c`. |
| SRS-MEM-002 | Each record shall include sequence, timestamp, sample count, CRC32, and sample payload. | `simple_acquisition_demo.py` and `embedded/src/memory_store.c`. |
| SRS-SD-001 | The demo shall optionally append completed bursts to a simulated SD-card log from the storage path. | `simulator/sd_card.py`, `simulator/device_memory.py`, and `embedded/src/sd_log.c`. |
| SRS-HAL-001 | The embedded design shall isolate hardware-specific capture and logging behind a platform boundary. | `embedded/include/platform_stm32u5.h` and `embedded/src/platform_stm32u5_hal.c`. |
| SRS-RTOS-001 | The embedded design shall map naturally to a ThreadX acquisition thread. | `embedded/src/acquisition_thread.c` and `embedded/src/main_threadx.c`. |
| SRS-RTOS-002 | The embedded design shall separate acquisition timing from storage work using a ThreadX queue and storage thread. | `embedded/src/acquisition_thread.c`, `embedded/src/storage_thread.c`, and `embedded/src/main_threadx.c`. |
| SRS-GUI-001 | The demo shall provide a small engineering UI for continuous start/stop, plotting the latest memory record, and showing status counters. | `gui/acquisition_demo_ui.py`. |
| SRS-SAFE-001 | The project shall be presented as a non-clinical acquisition prototype. | Documentation avoids diagnosis, treatment, or patient data claims. |

## Default Demo Settings

| Parameter | Value |
|---|---:|
| Samples per burst | 2046 |
| Burst sample rate | 15,000,000 samples/s |
| Burst period | 30 ms |
| ADC resolution | 12 bits |
| Bursts per demo run | 10 |
