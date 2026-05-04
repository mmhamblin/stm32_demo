from __future__ import annotations

from io import BytesIO
import struct
import sys
from pathlib import Path
import unittest
import zlib

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SIMULATOR_DIR = PROJECT_ROOT / "simulator"
if str(SIMULATOR_DIR) not in sys.path:
    sys.path.insert(0, str(SIMULATOR_DIR))

from simple_acquisition_demo import (
    SAMPLE_RATE_HZ,
    SAMPLES_PER_BURST,
    burst_duration_us,
    generate_burst,
    write_burst,
)


class SimpleAcquisitionDemoTests(unittest.TestCase):
    def test_burst_duration_matches_requirement(self) -> None:
        self.assertAlmostEqual(burst_duration_us(), 136.4, places=1)

    def test_generate_burst_returns_2046_12_bit_samples(self) -> None:
        samples = generate_burst(0)

        self.assertEqual(len(samples), SAMPLES_PER_BURST)
        self.assertTrue(all(0 <= sample <= 4095 for sample in samples))

    def test_generate_burst_has_visible_interleaved_channels(self) -> None:
        samples = generate_burst(0)
        ch0 = samples[0::2]
        ch1 = samples[1::2]

        self.assertEqual(len(ch0), len(ch1))
        self.assertNotEqual(ch0[:16], ch1[:16])

    def test_write_burst_writes_header_payload_and_crc(self) -> None:
        samples = generate_burst(3)
        stream = BytesIO()

        bytes_written = write_burst(stream, 3, samples)

        payload_size = SAMPLES_PER_BURST * 2
        header_size = struct.calcsize("<IQII")
        self.assertEqual(bytes_written, header_size + payload_size)

        data = stream.getvalue()
        sequence, timestamp_us, sample_count, crc32 = struct.unpack(
            "<IQII", data[:header_size]
        )
        payload = data[header_size:]

        self.assertEqual(sequence, 3)
        self.assertGreater(timestamp_us, 0)
        self.assertEqual(sample_count, SAMPLES_PER_BURST)
        self.assertEqual(len(payload), payload_size)
        self.assertEqual(crc32, zlib.crc32(payload) & 0xFFFF_FFFF)


if __name__ == "__main__":
    unittest.main()
