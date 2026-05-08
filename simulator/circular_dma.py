"""Lightweight circular DMA model for a continuous AFE sample stream."""

from __future__ import annotations

import math

from simple_acquisition_demo import ADC_MAX, BURST_PERIOD_SEC, SAMPLE_RATE_HZ, SAMPLES_PER_BURST


DMA_RING_SAMPLE_COUNT = 8192
PERIOD_SAMPLE_ADVANCE = int(SAMPLE_RATE_HZ * BURST_PERIOD_SEC)


class CircularDmaSimulator:
    """Continuously clocked sample stream written into a circular DMA ring."""

    def __init__(self, ring_sample_count: int = DMA_RING_SAMPLE_COUNT) -> None:
        if ring_sample_count < SAMPLES_PER_BURST:
            raise ValueError("ring_must_fit_one_window")

        self.ring_sample_count = ring_sample_count
        self.ring: list[int] = [0] * ring_sample_count
        self.total_samples_written = 0
        self.write_index = 0

    def reset(self) -> None:
        self.ring = [0] * self.ring_sample_count
        self.total_samples_written = 0
        self.write_index = 0

    def advance_one_period(self) -> None:
        """Advance the continuous stream by one 30 ms acquisition period."""

        self.advance_samples(PERIOD_SAMPLE_ADVANCE)

    def advance_samples(self, sample_count: int) -> None:
        """Write the latest part of a continuous stream into the DMA ring."""

        if sample_count <= 0:
            return

        end_sample = self.total_samples_written + sample_count
        samples_to_materialize = min(sample_count, self.ring_sample_count)
        start_sample = end_sample - samples_to_materialize

        for absolute_index in range(start_sample, end_sample):
            self.ring[absolute_index % self.ring_sample_count] = sample_at(absolute_index)

        self.total_samples_written = end_sample
        self.write_index = self.total_samples_written % self.ring_sample_count

    def latest_window(self, sample_count: int = SAMPLES_PER_BURST) -> list[int]:
        """Return the latest captured window from the circular DMA ring."""

        if sample_count > self.ring_sample_count:
            raise ValueError("window_larger_than_ring")
        if self.total_samples_written < sample_count:
            raise ValueError("not_enough_samples")

        start = (self.write_index - sample_count) % self.ring_sample_count
        return [
            self.ring[(start + offset) % self.ring_sample_count]
            for offset in range(sample_count)
        ]


def sample_at(absolute_index: int) -> int:
    """Generate one deterministic 12-bit sample from a continuous stream."""

    channel_offset = -120 if absolute_index % 2 == 0 else 120
    carrier = math.sin(2.0 * math.pi * (absolute_index / 64.0))
    slow_envelope = math.sin(2.0 * math.pi * (absolute_index / 4096.0))
    value = int(2048 + channel_offset + 620 * carrier + 120 * slow_envelope)
    return max(0, min(ADC_MAX, value))
