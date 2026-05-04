"""Simple STM32U5A5 burst acquisition demo.

1. Generate 2046 fake 12-bit ADC samples.
2. Treat the burst as if it was acquired at 15 MSPS.
3. Write the burst to a small binary file.
4. Repeat once every 30 ms.

Run from the project root:

    python simulator/simple_acquisition_demo.py
"""

from __future__ import annotations

from pathlib import Path
import math
import struct
import time
import zlib


SAMPLE_RATE_HZ = 15_000_000
SAMPLES_PER_BURST = 2046
BURST_PERIOD_SEC = 0.030
BURST_COUNT = 10
ADC_MAX = 4095


def burst_duration_us() -> float:
    return (SAMPLES_PER_BURST / SAMPLE_RATE_HZ) * 1_000_000


def generate_burst(burst_index: int) -> list[int]:
    """Generate one burst of fake 12-bit ADC data."""

    samples: list[int] = []
    for i in range(SAMPLES_PER_BURST):
        # Two-channel interleaved shape: even samples look like CH0, odd like CH1.
        channel_offset = -120 if i % 2 == 0 else 120
        wave = math.sin(2.0 * math.pi * (i / 64.0 + burst_index * 0.05))
        value = int(2048 + channel_offset + 700 * wave)
        samples.append(max(0, min(ADC_MAX, value)))
    return samples


def write_burst(file, sequence: int, samples: list[int]) -> int:
    """Write one simple record: sequence, timestamp, count, crc, samples."""

    timestamp_us = int(time.time() * 1_000_000)
    payload = struct.pack(f"<{len(samples)}H", *samples)
    crc32 = zlib.crc32(payload) & 0xFFFF_FFFF

    header = struct.pack("<IQII", sequence, timestamp_us, len(samples), crc32)
    file.write(header)
    file.write(payload)
    return len(header) + len(payload)


def main() -> None:
    output_dir = Path(__file__).resolve().parent / "logs"
    output_dir.mkdir(exist_ok=True)
    output_path = output_dir / "simple_capture.u12bin"

    bursts_written = 0
    samples_written = 0
    bytes_written = 0
    missed_bursts = 0

    print("STM32U5A5 Acquisition Demo")
    print(
        f"Config: {SAMPLES_PER_BURST} samples, 12-bit, "
        f"{SAMPLE_RATE_HZ:,} samples/s, every 30 ms"
    )
    print(f"Calculated burst duration: {burst_duration_us():.1f} us")
    print()

    with output_path.open("wb") as file:
        for sequence in range(BURST_COUNT):
            period_start = time.perf_counter()

            samples = generate_burst(sequence)
            bytes_written += write_burst(file, sequence, samples)
            bursts_written += 1
            samples_written += len(samples)

            elapsed = time.perf_counter() - period_start
            sleep_time = BURST_PERIOD_SEC - elapsed
            if sleep_time < 0:
                missed_bursts += 1
                sleep_time = 0

            print(
                f"Burst {sequence}: captured {len(samples)} samples "
                f"in {burst_duration_us():.1f} us, wrote {len(samples) * 2} sample bytes"
            )
            time.sleep(sleep_time)

    print()
    print("Done.")
    print(f"Output file: {output_path}")
    print(f"Bursts written: {bursts_written}")
    print(f"Samples written: {samples_written}")
    print(f"Bytes written: {bytes_written}")
    print(f"Missed bursts: {missed_bursts}")


if __name__ == "__main__":
    main()
