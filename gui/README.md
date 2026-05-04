# GUI

The GUI is a small engineering monitor for the burst acquisition demo. It is intentionally not styled as a clinical product.

Run:

```powershell
pip install -r requirements.txt
python gui/acquisition_demo_ui.py
```

What it shows:

- fixed acquisition settings from the email: 2046 samples, 15 MSPS, every 30 ms
- continuous Start/Stop behavior
- latest burst read from simulated STM32 memory
- even samples plotted as CH0 and odd samples plotted as CH1
- embedded-style status counters
- event log with sequence and CRC information

The modeled data flow is:

```text
simulated acquisition -> simulated STM32 RAM ring buffer -> GUI polling/plotting
```

That mirrors the embedded C skeleton:

```text
PSSI/DCMI + DMA -> MemoryStore_WriteBurst() -> GUI reads latest record/status
```

The real hardware version would replace the in-process simulator with USB CDC or another debug/telemetry path to the STM32U5A5 firmware.
