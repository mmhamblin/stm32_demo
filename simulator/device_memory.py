"""Simulated STM32U5A5 acquisition and memory store.

The C firmware skeleton separates acquisition timing from storage work. This
Python module mirrors that idea with a small acquisition-to-storage queue so
the GUI can read "device memory" instead of directly owning acquisition data.
"""

from __future__ import annotations

from dataclasses import dataclass
import struct
import time
import zlib

from simple_acquisition_demo import SAMPLE_RATE_HZ, SAMPLES_PER_BURST, generate_burst


MEMORY_RECORD_COUNT = 8
STORAGE_QUEUE_DEPTH = 4


@dataclass(frozen=True)
class MemoryRecord:
    sequence: int
    timestamp_us: int
    sample_count: int
    sample_rate_hz: int
    crc32: int
    samples: list[int]


@dataclass(frozen=True)
class MemoryStatus:
    state: str
    records_written: int
    records_overwritten: int
    latest_sequence: int
    storage_queue_depth: int
    storage_writes: int
    storage_queue_full: int
    samples_written: int
    bytes_written: int
    missed_bursts: int
    last_error: str


class DeviceMemory:
    """RAM ring buffer matching the embedded memory_store.c intent."""

    def __init__(self, record_count: int = MEMORY_RECORD_COUNT) -> None:
        self._records: list[MemoryRecord | None] = [None] * record_count
        self._write_index = 0
        self.records_written = 0
        self.records_overwritten = 0
        self.samples_written = 0
        self.bytes_written = 0

    def write_burst(self, samples: list[int], timestamp_us: int | None = None) -> MemoryRecord:
        if len(samples) != SAMPLES_PER_BURST:
            raise ValueError("wrong_sample_count")
        if any(sample < 0 or sample > 4095 for sample in samples):
            raise ValueError("sample_out_of_12_bit_range")

        payload = struct.pack(f"<{len(samples)}H", *samples)
        record_timestamp_us = (
            timestamp_us if timestamp_us is not None else int(time.time() * 1_000_000)
        )
        record = MemoryRecord(
            sequence=self.records_written,
            timestamp_us=record_timestamp_us,
            sample_count=len(samples),
            sample_rate_hz=SAMPLE_RATE_HZ,
            crc32=zlib.crc32(payload) & 0xFFFF_FFFF,
            samples=samples,
        )

        if self.records_written >= len(self._records):
            self.records_overwritten += 1

        self._records[self._write_index] = record
        self._write_index = (self._write_index + 1) % len(self._records)
        self.records_written += 1
        self.samples_written += len(samples)
        self.bytes_written += len(payload)
        return record

    def latest(self) -> MemoryRecord | None:
        if self.records_written == 0:
            return None

        latest_index = (self._write_index - 1) % len(self._records)
        return self._records[latest_index]

    def reset(self) -> None:
        self._records = [None] * len(self._records)
        self._write_index = 0
        self.records_written = 0
        self.records_overwritten = 0
        self.samples_written = 0
        self.bytes_written = 0


class SimulatedStm32U5A5:
    """Tiny device model: acquisition queues bursts; storage writes memory."""

    def __init__(self) -> None:
        self.memory = DeviceMemory()
        self.storage_queue: list[tuple[int, list[int]]] = []
        self.state = "IDLE"
        self.storage_writes = 0
        self.storage_queue_full = 0
        self.missed_bursts = 0
        self.last_error = "NONE"

    def start(self) -> None:
        self.memory.reset()
        self.storage_queue.clear()
        self.state = "ARMED"
        self.storage_writes = 0
        self.storage_queue_full = 0
        self.missed_bursts = 0
        self.last_error = "NONE"

    def stop(self) -> None:
        self.state = "IDLE"

    def capture_once(self) -> MemoryRecord:
        self.state = "CAPTURING"
        burst_index = self.memory.records_written
        samples = generate_burst(burst_index)
        capture_timestamp_us = int(time.time() * 1_000_000)

        self.state = "QUEUING"
        if len(self.storage_queue) >= STORAGE_QUEUE_DEPTH:
            self.storage_queue_full += 1
            self.last_error = "STORAGE_QUEUE_FULL"
            raise RuntimeError("storage_queue_full")

        self.storage_queue.append((capture_timestamp_us, samples))

        self.state = "STORING"
        record = self._storage_step()
        self.state = "ARMED"
        return record

    def _storage_step(self) -> MemoryRecord:
        capture_timestamp_us, samples = self.storage_queue.pop(0)
        record = self.memory.write_burst(samples, timestamp_us=capture_timestamp_us)
        self.storage_writes += 1
        return record

    def inject_missed_burst(self) -> None:
        self.missed_bursts += 1
        self.last_error = "MISSED_BURST"

    def status(self) -> MemoryStatus:
        latest = self.memory.latest()
        latest_sequence = latest.sequence if latest else 0

        return MemoryStatus(
            state=self.state,
            records_written=self.memory.records_written,
            records_overwritten=self.memory.records_overwritten,
            latest_sequence=latest_sequence,
            storage_queue_depth=len(self.storage_queue),
            storage_writes=self.storage_writes,
            storage_queue_full=self.storage_queue_full,
            samples_written=self.memory.samples_written,
            bytes_written=self.memory.bytes_written,
            missed_bursts=self.missed_bursts,
            last_error=self.last_error,
        )
