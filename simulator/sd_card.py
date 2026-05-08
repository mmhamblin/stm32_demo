"""File-backed SD card simulation for the acquisition demo."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
import zlib


@dataclass(frozen=True)
class SdStatus:
    mounted: bool
    file_path: Path
    records_written: int
    bytes_written: int
    write_errors: int
    last_error: str


class SimulatedSdCard:
    """Append-only file that models SD-card burst logging."""

    def __init__(self, file_path: Path) -> None:
        self.file_path = file_path
        self.mounted = False
        self.records_written = 0
        self.bytes_written = 0
        self.write_errors = 0
        self.last_error = "NONE"

    def mount(self, *, reset_file: bool = False) -> None:
        self.file_path.parent.mkdir(parents=True, exist_ok=True)
        if reset_file:
            self.file_path.write_bytes(b"")
            self.records_written = 0
            self.bytes_written = 0
        self.mounted = True
        self.last_error = "NONE"

    def unmount(self) -> None:
        self.mounted = False

    def write_record(
        self,
        *,
        sequence: int,
        timestamp_us: int,
        sample_count: int,
        samples: list[int],
    ) -> bool:
        if not self.mounted:
            self.write_errors += 1
            self.last_error = "SD_NOT_MOUNTED"
            return False

        try:
            payload = struct.pack(f"<{len(samples)}H", *samples)
            crc32 = zlib.crc32(payload) & 0xFFFF_FFFF
            header = struct.pack("<IQII", sequence, timestamp_us, sample_count, crc32)

            with self.file_path.open("ab") as file:
                file.write(header)
                file.write(payload)

            self.records_written += 1
            self.bytes_written += len(header) + len(payload)
            self.last_error = "NONE"
            return True
        except OSError:
            self.write_errors += 1
            self.last_error = "SD_WRITE_FAILED"
            return False

    def status(self) -> SdStatus:
        return SdStatus(
            mounted=self.mounted,
            file_path=self.file_path,
            records_written=self.records_written,
            bytes_written=self.bytes_written,
            write_errors=self.write_errors,
            last_error=self.last_error,
        )
