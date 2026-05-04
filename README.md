# STM32U5A5 Medical Device Acquisition Demo

This is a small portfolio project based on an embedded medical-device style prompt:

```text
Using ThreadX types and an STM32U5A5 microcontroller, read 2046 samples of
12-bit ADC data acquired at 15 MSPS, write it to memory, and go back to sleep.
Do this once every 30 ms.
```

The project is a non-clinical engineering demo. It does not diagnose, treat, image patients, or store patient data.

## What This Demonstrates

- Translating an email requirement into short SRS-style requirements.
- Modeling a 2046-sample, 12-bit, 15 MSPS burst capture.
- Writing captured bursts into a memory-style RAM ring buffer.
- Appending completed bursts to a simulated SD-card log from the storage path.
- Showing a ThreadX/HAL-shaped STM32U5A5 firmware architecture.
- Providing a small Python/Qt GUI that reads the latest record from simulated device memory.

## System Shape

```text
AFE5401-style sample source
  -> STM32U5A5 PSSI/DCMI + DMA concept
  -> ThreadX acquisition thread
  -> ThreadX queue
  -> ThreadX storage thread
  -> RAM ring buffer
  -> GUI monitor reads latest record/status
```

In this repository, the hardware is simulated:

```text
simple Python burst generator
  -> simulator/device_memory.py acquisition/storage model
  -> simulator/logs/sd_capture.u12bin
  -> gui/acquisition_demo_ui.py
```

## Run The Simple Simulator

```powershell
python simulator/simple_acquisition_demo.py
```

Expected output includes:

```text
2046 samples
15,000,000 samples/s
136.4 us burst duration
30 ms burst period
```

## Run The GUI

Install UI dependencies:

```powershell
pip install -r requirements.txt
```

Launch:

```powershell
python gui/acquisition_demo_ui.py
```

The GUI supports:

- continuous Start/Stop
- Capture Once
- latest burst plot
- simulated memory status
- CRC/status event log

## Validate The Core Demo

```powershell
python tools/validate_demo.py
```

This checks the core behavior without needing Qt:

- 2046 samples per burst
- samples remain in 12-bit range
- simulated memory records are written
- the 8-record ring buffer overwrites as expected
- CRC metadata is present

## Run Unit Tests

```powershell
python -m unittest discover -s tests
```

The tests cover the burst generator, simple log record format, simulated memory store, ring-buffer overwrite behavior, and simulated device status flow.

## Key Files

| File | Purpose |
|---|---|
| `docs/software_requirements.md` | Short SRS-style requirement IDs. |
| `docs/board_interface_map.md` | How the requirement maps to STM32U5A5 concepts. |
| `simulator/simple_acquisition_demo.py` | Minimal terminal demo. |
| `simulator/device_memory.py` | Simulated STM32 RAM record store. |
| `simulator/sd_card.py` | File-backed SD-card logging simulation. |
| `tests/` | Standard-library unit tests for the simulator and memory model. |
| `embedded/src/acquisition_thread.c` | ThreadX acquisition-loop shape. |
| `embedded/src/storage_thread.c` | ThreadX storage-thread shape. |
| `embedded/src/platform_stm32u5_hal.c` | STM32U5A5 HAL boundary. |
| `embedded/src/memory_store.c` | C RAM ring buffer shape. |
| `gui/acquisition_demo_ui.py` | Python/Qt monitor UI. |

## What Is Simulated vs Hardware

| Area | Demo version | Real STM32U5A5 version |
|---|---|---|
| ADC source | Synthetic 12-bit samples | TI AFE5401 data bus |
| Capture | Python generator | PSSI or DCMI with DMA |
| Memory | Python RAM ring buffer | C RAM ring buffer |
| SD card | File-backed simulated SD log | SDMMC/FileX in storage thread |
| Scheduling | Qt/Python timer | ThreadX acquisition and storage threads |
| GUI link | In-process simulator | USB CDC/status interface |

## Next Hardware Steps

- Confirm whether 2046 samples means total burst samples or samples per active channel.
- Confirm whether PSSI or DCMI best matches the AFE5401 output timing.
- Generate a CubeMX/IAR STM32U5A5 project and map the skeleton files into it.
- Replace simulated memory polling with a USB CDC status/data path.
