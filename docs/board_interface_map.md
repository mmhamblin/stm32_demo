# Board Interface Map

This project models an STM32U5A5-based acquisition logger for short bursts of 12-bit ADC data from a TI AFE5401 analog front end. The current implementation is simulator-first, but the software is organized around the same interfaces that would be used on hardware.

## Requirement Interpretation

The interview prompt describes this capture pattern:

```text
Read 2046 samples of 12-bit ADC data at 15 MSPS.
Write the captured block to memory.
Return to sleep.
Repeat once every 30 ms.
```

This is a burst capture problem, not continuous streaming.

Key numbers:

| Item | Value |
|---|---:|
| Samples per burst | 2046 |
| ADC resolution | 12 bits |
| Burst sample rate | 15 MSPS |
| Burst duration | 136.4 us |
| Burst period | 30 ms |
| Duty cycle | about 0.46% |
| Stored as `uint16_t` | 4092 bytes per burst |
| Average payload rate | about 136 kB/s |

The hard part is capturing a fast 136 us burst reliably. The easier part is that the system has most of the remaining 30 ms period to write the block, update status, and return to sleep.

## Data Path

```text
TI AFE5401
  -> 12-bit ADC samples
  -> STM32U5A5 parallel capture peripheral
  -> DMA burst buffer
  -> ThreadX acquisition event
  -> memory/log record
  -> GUI status or replay
```

Earlier context mentioned two active AFE channels being interleaved. The simulator keeps that visible in metadata. Current project assumption:

```text
2046 total sample words per burst
optional channel order: CH0, CH1, CH0, CH1, ...
```

If the final interpretation is 2046 samples per channel, the burst size changes to 4092 interleaved sample words.

## Hardware Mapping

| Project role | Real STM32U5A5 interface | Simulator equivalent |
|---|---|---|
| ADC source | TI AFE5401 | Synthetic 12-bit burst generator |
| Parallel capture | PSSI, or DCMI if timing fits better | Filled sample buffer |
| High-speed movement | DMA/GPDMA | Filled-buffer callback |
| Scheduling | ThreadX timer/events | Host timer/task loop |
| Storage | RAM, SDMMC/FileX, or memory-backed log | `.u12log` file |
| GUI control | USBX CDC virtual COM port | TCP localhost command server |
| Integrity check | CRC peripheral or software CRC32 | Software CRC32 |
| Sleep/idle | STM32U5 power modes | Simulated state transition |

## HAL Boundary

The embedded C design keeps hardware-specific calls behind a small platform layer:

```c
bool Platform_AfeBurstStart(uint16_t *buffer, uint32_t sample_count);
bool Platform_LogWrite(const void *data, uint32_t byte_count);
uint32_t Platform_GetTickMs(void);
void Platform_EnterSleepUntilNextBurst(void);
```

In the simulator, `Platform_AfeBurstStart()` fills a buffer with synthetic samples. On STM32U5A5 hardware, the same call would start a PSSI/DCMI DMA transfer.

## ThreadX Role

ThreadX would separate the firmware into small responsibilities:

| Thread | Responsibility |
|---|---|
| Acquisition | Arm the burst capture and handle DMA complete. |
| Logging | Write the completed 2046-sample block. |
| Control | Process GUI commands such as start, stop, and status. |
| Health | Detect missed bursts, storage errors, and timing faults. |

The main design goal is to keep the 136 us capture window simple and deterministic, then do slower work after the burst is complete.

## Open Questions

- Does "2046 samples" mean total samples per burst or 2046 samples per active channel?
- Is the AFE5401 output best captured with PSSI or DCMI on the final STM32U5A5 configuration?
- Is final storage intended to be RAM only, SD/FileX, or both?
