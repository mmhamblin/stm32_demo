# Interview Walkthrough

This is the short path for a one-hour interview.

## 1. Start With The Requirement

Open `docs/software_requirements.md`.

Explain:

- The prompt is a burst acquisition problem.
- The demo focuses on 2046 samples, 12-bit data, 15 MSPS, once every 30 ms.
- Requirement IDs are short enough to reference from code and tests.

## 2. Show The Timing Insight

Open `docs/board_interface_map.md`.

Key point:

```text
2046 samples / 15 MSPS = about 136.4 us
```

So the difficult part is the short high-speed capture window. The system has most of the 30 ms period to write memory, update status, and return to sleep.

## 3. Run The Core Validation

```powershell
python tools/validate_demo.py
```

This proves:

- 2046 samples per burst
- 12-bit sample range
- memory records written
- RAM ring overwrite behavior
- CRC metadata exists

Optional unit-test command:

```powershell
python -m unittest discover -s tests
```

## 4. Show The Embedded C Shape

Open these files:

- `embedded/src/acquisition_thread.c`
- `embedded/src/platform_stm32u5_hal.c`
- `embedded/src/memory_store.c`

Explain:

```text
ThreadX acquisition thread
  -> starts PSSI/DCMI + DMA capture
  -> waits for DMA-complete semaphore
  -> writes 2046 samples to RAM ring buffer
  -> sleeps until the next 30 ms period
```

The CPU does not poll 15 MSPS data.

## 5. Show The GUI

```powershell
pip install -r requirements.txt
python gui/acquisition_demo_ui.py
```

Explain:

- The GUI is an engineering monitor, not a clinical UI.
- Start runs continuously.
- Acquisition writes to simulated STM32 memory.
- The GUI reads the latest memory record and plots it.
- Even samples are displayed as CH0 and odd samples as CH1 to represent possible interleaved AFE data.

## 6. Be Clear About Simulation

Say:

> This is not a generated CubeMX hardware project yet. I used the simulator to make the data contract and workflow visible, then wrote the C modules in the shape I would map into a ThreadX + STM32U5A5 HAL project.

## 7. Reasonable Next Steps

- Confirm whether 2046 samples means total burst samples or samples per active channel.
- Confirm the exact AFE5401 output mode.
- Choose PSSI or DCMI based on timing/framing.
- Generate the CubeMX/IAR project.
- Replace the in-process simulator with USB CDC status/data transfer.
