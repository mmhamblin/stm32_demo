from __future__ import annotations

import sys
from pathlib import Path
import struct
import unittest
import zlib

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SIMULATOR_DIR = PROJECT_ROOT / "simulator"
if str(SIMULATOR_DIR) not in sys.path:
    sys.path.insert(0, str(SIMULATOR_DIR))

from device_memory import (
    CAPTURE_MODE_BURST,
    CAPTURE_MODE_CONTINUOUS,
    MEMORY_RECORD_COUNT,
    DeviceMemory,
    SimulatedStm32U5A5,
)
from circular_dma import CircularDmaSimulator, DMA_RING_SAMPLE_COUNT, PERIOD_SAMPLE_ADVANCE
from simple_acquisition_demo import SAMPLE_RATE_HZ, SAMPLES_PER_BURST, generate_burst


class DeviceMemoryTests(unittest.TestCase):
    def test_empty_memory_has_no_latest_record(self) -> None:
        memory = DeviceMemory()

        self.assertIsNone(memory.latest())

    def test_write_burst_creates_record_metadata(self) -> None:
        memory = DeviceMemory()
        samples = generate_burst(0)

        record = memory.write_burst(samples)

        self.assertEqual(record.sequence, 0)
        self.assertEqual(record.sample_count, SAMPLES_PER_BURST)
        self.assertEqual(record.sample_rate_hz, SAMPLE_RATE_HZ)
        self.assertEqual(record.samples, samples)
        self.assertNotEqual(record.crc32, 0)
        self.assertIs(memory.latest(), record)

    def test_rejects_wrong_sample_count(self) -> None:
        memory = DeviceMemory()

        with self.assertRaisesRegex(ValueError, "wrong_sample_count"):
            memory.write_burst([0] * 10)

    def test_rejects_out_of_range_12_bit_samples(self) -> None:
        memory = DeviceMemory()
        samples = generate_burst(0)
        samples[7] = 4096

        with self.assertRaisesRegex(ValueError, "sample_out_of_12_bit_range"):
            memory.write_burst(samples)

    def test_ring_buffer_overwrite_count(self) -> None:
        memory = DeviceMemory()

        for i in range(MEMORY_RECORD_COUNT + 4):
            memory.write_burst(generate_burst(i))

        latest = memory.latest()
        self.assertIsNotNone(latest)
        self.assertEqual(latest.sequence, MEMORY_RECORD_COUNT + 3)
        self.assertEqual(memory.records_written, MEMORY_RECORD_COUNT + 4)
        self.assertEqual(memory.records_overwritten, 4)
        self.assertEqual(memory.samples_written, (MEMORY_RECORD_COUNT + 4) * SAMPLES_PER_BURST)


class SimulatedDeviceTests(unittest.TestCase):
    def test_start_capture_stop_state_flow(self) -> None:
        device = SimulatedStm32U5A5()

        self.assertEqual(device.status().state, "IDLE")
        device.set_capture_mode(CAPTURE_MODE_CONTINUOUS)

        device.start()
        self.assertEqual(device.status().state, "ARMED")

        record = device.capture_once()
        status = device.status()
        self.assertEqual(status.state, "ARMED")
        self.assertEqual(status.records_written, 1)
        self.assertEqual(status.latest_sequence, record.sequence)
        self.assertEqual(status.capture_mode, CAPTURE_MODE_CONTINUOUS)
        self.assertEqual(status.dma_ring_samples, DMA_RING_SAMPLE_COUNT)
        self.assertEqual(status.dma_total_samples, PERIOD_SAMPLE_ADVANCE)
        self.assertEqual(status.storage_queue_depth, 0)
        self.assertEqual(status.storage_writes, 1)
        self.assertEqual(status.storage_queue_full, 0)
        self.assertEqual(status.sd_records_written, 1)
        self.assertEqual(status.sd_write_errors, 0)
        self.assertGreater(status.sd_bytes_written, 0)
        self.assertTrue(status.sd_file_path.exists())
        self.assertEqual(status.samples_written, SAMPLES_PER_BURST)
        self.assertEqual(status.bytes_written, SAMPLES_PER_BURST * 2)

        device.stop()
        self.assertEqual(device.status().state, "IDLE")

    def test_sd_log_contains_written_record(self) -> None:
        device = SimulatedStm32U5A5()
        device.start()

        record = device.capture_once()
        sd_path = device.status().sd_file_path

        header_size = struct.calcsize("<IQII")
        data = sd_path.read_bytes()
        sequence, timestamp_us, sample_count, crc32 = struct.unpack(
            "<IQII", data[:header_size]
        )
        payload = data[header_size:]

        self.assertEqual(sequence, record.sequence)
        self.assertEqual(timestamp_us, record.timestamp_us)
        self.assertEqual(sample_count, SAMPLES_PER_BURST)
        self.assertEqual(len(payload), SAMPLES_PER_BURST * 2)
        self.assertEqual(zlib.crc32(payload) & 0xFFFF_FFFF, crc32)

    def test_missed_burst_status(self) -> None:
        device = SimulatedStm32U5A5()
        device.start()
        device.inject_missed_burst()

        status = device.status()
        self.assertEqual(status.missed_bursts, 1)
        self.assertEqual(status.last_error, "MISSED_BURST")

    def test_can_capture_in_burst_mode(self) -> None:
        device = SimulatedStm32U5A5()
        device.set_capture_mode(CAPTURE_MODE_BURST)
        device.start()

        record = device.capture_once()
        status = device.status()

        self.assertEqual(status.capture_mode, CAPTURE_MODE_BURST)
        self.assertEqual(record.sample_count, SAMPLES_PER_BURST)
        self.assertEqual(status.records_written, 1)
        self.assertEqual(status.dma_total_samples, 0)


class CircularDmaTests(unittest.TestCase):
    def test_latest_window_has_expected_size_after_one_period(self) -> None:
        dma = CircularDmaSimulator()

        dma.advance_one_period()
        window = dma.latest_window()

        self.assertEqual(len(window), SAMPLES_PER_BURST)
        self.assertEqual(dma.total_samples_written, PERIOD_SAMPLE_ADVANCE)
        self.assertTrue(all(0 <= sample <= 4095 for sample in window))

    def test_ring_write_index_wraps(self) -> None:
        dma = CircularDmaSimulator(ring_sample_count=4096)

        dma.advance_samples(5000)

        self.assertEqual(dma.write_index, 5000 % 4096)
        self.assertEqual(len(dma.latest_window()), SAMPLES_PER_BURST)


if __name__ == "__main__":
    unittest.main()
