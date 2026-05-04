# Simulator

Main interview demo:

```powershell
python simulator/simple_acquisition_demo.py
```

This runs a short burst-acquisition model:

- 2046 fake 12-bit ADC samples per burst
- 15 MSPS burst-rate metadata
- one burst every 30 ms
- binary log output in `simulator/logs/simple_capture.u12bin`

The GUI uses `device_memory.py` to model the STM32 RAM path:

```text
burst acquisition -> storage queue -> RAM ring buffer -> GUI reads latest record
                                -> simulated SD log file
```

The simulated SD-card log is written to:

```text
simulator/logs/sd_capture.u12bin
```

The `advanced/` folder contains an optional larger simulator with command parsing and a fuller `.u12log` format. The simple demo is the recommended one-hour interview path.
