"""Validate the core acquisition demo without Qt or hardware."""

from __future__ import annotations

from pathlib import Path
import sys

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SIMULATOR_DIR = PROJECT_ROOT / "simulator"
if str(SIMULATOR_DIR) not in sys.path:
    sys.path.insert(0, str(SIMULATOR_DIR))

from device_memory import MEMORY_RECORD_COUNT, SimulatedStm32U5A5
from simple_acquisition_demo import SAMPLE_RATE_HZ, SAMPLES_PER_BURST, burst_duration_us


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    device = SimulatedStm32U5A5()
    device.start()

    records = [device.capture_once() for _ in range(12)]
    latest = device.memory.latest()
    status = device.status()

    require(latest is not None, "latest record should exist")
    require(records[0].sample_count == SAMPLES_PER_BURST, "first record sample count")
    require(latest.sample_count == SAMPLES_PER_BURST, "latest record sample count")
    require(latest.sequence == 11, "latest sequence should be 11 after 12 captures")
    require(latest.sample_rate_hz == SAMPLE_RATE_HZ, "sample rate metadata")
    require(all(0 <= sample <= 4095 for sample in latest.samples), "12-bit sample range")
    require(latest.crc32 != 0, "CRC should be populated")
    require(status.capture_mode == "CIRCULAR_DMA_WINDOW", "capture mode")
    require(status.dma_ring_samples >= SAMPLES_PER_BURST, "DMA ring size")
    require(status.dma_total_samples == 12 * 450000, "continuous DMA sample advance")
    require(status.records_written == 12, "records_written should be 12")
    require(status.sd_records_written == 12, "SD records_written should be 12")
    require(status.sd_write_errors == 0, "SD write errors should be 0")
    require(status.sd_bytes_written > 0, "SD bytes_written should be populated")
    require(status.sd_file_path.exists(), "simulated SD log should exist")
    require(status.records_overwritten == 12 - MEMORY_RECORD_COUNT, "ring overwrite count")
    require(status.samples_written == 12 * SAMPLES_PER_BURST, "samples_written count")
    require(status.missed_bursts == 0, "missed burst count")

    print("Validation passed.")
    print(f"Samples per burst: {SAMPLES_PER_BURST}")
    print(f"Burst sample rate: {SAMPLE_RATE_HZ:,} samples/s")
    print(f"Calculated burst duration: {burst_duration_us():.1f} us")
    print(f"Capture mode: {status.capture_mode}")
    print(f"DMA ring samples: {status.dma_ring_samples}")
    print(f"Continuous samples advanced: {status.dma_total_samples}")
    print(f"Records written: {status.records_written}")
    print(f"SD records written: {status.sd_records_written}")
    print(f"SD bytes written: {status.sd_bytes_written}")
    print(f"SD log path: {status.sd_file_path}")
    print(f"Records overwritten in RAM ring: {status.records_overwritten}")
    print(f"Latest sequence: {latest.sequence}")
    print(f"Latest CRC: 0x{latest.crc32:08X}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
