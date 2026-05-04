"""Qt engineering monitor for the STM32U5A5 burst acquisition demo.

Run from the project root after installing requirements:

    pip install -r requirements.txt
    python gui/acquisition_demo_ui.py

The GUI monitors a simulated STM32 memory store. Acquisition writes bursts to
that memory store; the GUI polls and plots the latest memory record.
"""

from __future__ import annotations

from pathlib import Path
import struct
import sys
import time
import zlib

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SIMULATOR_DIR = PROJECT_ROOT / "simulator"
if str(SIMULATOR_DIR) not in sys.path:
    sys.path.insert(0, str(SIMULATOR_DIR))

try:
    from PySide6 import QtCore, QtWidgets
    import pyqtgraph as pg
except ModuleNotFoundError as exc:
    missing = exc.name or "Qt dependency"
    print(f"Missing dependency: {missing}")
    print("Install UI dependencies with: pip install -r requirements.txt")
    raise SystemExit(1) from exc

from device_memory import MemoryRecord, SimulatedStm32U5A5
from simple_acquisition_demo import (
    BURST_PERIOD_SEC,
    SAMPLE_RATE_HZ,
    SAMPLES_PER_BURST,
    burst_duration_us,
)


class AcquisitionMonitor(QtWidgets.QMainWindow):
    def __init__(self) -> None:
        super().__init__()

        self.setWindowTitle("STM32U5A5 Burst Acquisition Monitor")
        self.resize(1120, 720)

        self.device = SimulatedStm32U5A5()
        self.latest_sequence_seen = -1
        self.last_period_start = 0.0

        self.timer = QtCore.QTimer(self)
        self.timer.setInterval(int(BURST_PERIOD_SEC * 1000))
        self.timer.timeout.connect(self._device_period_tick)

        self._build_ui()
        self._update_status()
        self._append_event("Ready. GUI reads latest burst from simulated STM32 memory.")

    def _build_ui(self) -> None:
        root = QtWidgets.QWidget()
        self.setCentralWidget(root)

        layout = QtWidgets.QVBoxLayout(root)
        top = QtWidgets.QHBoxLayout()
        layout.addLayout(top, stretch=1)

        top.addWidget(self._build_control_panel(), stretch=0)

        self.plot = pg.PlotWidget()
        self.plot.setBackground("w")
        self.plot.setTitle("Latest Memory Record: 2046-Sample Burst")
        self.plot.setLabel("bottom", "Sample index per channel")
        self.plot.setLabel("left", "ADC value", units="counts")
        self.plot.setYRange(0, 4095)
        self.plot.showGrid(x=True, y=True, alpha=0.25)
        self.ch0_curve = self.plot.plot(name="CH0/even", pen=pg.mkPen("#1f77b4", width=2))
        self.ch1_curve = self.plot.plot(name="CH1/odd", pen=pg.mkPen("#ff7f0e", width=2))
        self.plot.addLegend()
        top.addWidget(self.plot, stretch=1)

        self.event_log = QtWidgets.QTextEdit()
        self.event_log.setReadOnly(True)
        self.event_log.setMinimumHeight(150)
        layout.addWidget(self.event_log)

    def _build_control_panel(self) -> QtWidgets.QWidget:
        panel = QtWidgets.QWidget()
        panel.setFixedWidth(330)
        layout = QtWidgets.QVBoxLayout(panel)

        title = QtWidgets.QLabel("Acquisition Monitor")
        title.setStyleSheet("font-size: 18px; font-weight: 600;")
        layout.addWidget(title)

        config_group = QtWidgets.QGroupBox("Fixed Demo Configuration")
        config_layout = QtWidgets.QFormLayout(config_group)
        config_layout.addRow("Sample rate", QtWidgets.QLabel(f"{SAMPLE_RATE_HZ:,} samples/s"))
        config_layout.addRow("Samples/burst", QtWidgets.QLabel(str(SAMPLES_PER_BURST)))
        config_layout.addRow("Burst period", QtWidgets.QLabel("30 ms"))
        config_layout.addRow("Burst duration", QtWidgets.QLabel(f"{burst_duration_us():.1f} us"))
        config_layout.addRow("Memory records", QtWidgets.QLabel("8-record RAM ring"))
        layout.addWidget(config_group)

        run_group = QtWidgets.QGroupBox("Run")
        run_layout = QtWidgets.QVBoxLayout(run_group)

        button_row = QtWidgets.QHBoxLayout()
        self.start_button = QtWidgets.QPushButton("Start")
        self.stop_button = QtWidgets.QPushButton("Stop")
        button_row.addWidget(self.start_button)
        button_row.addWidget(self.stop_button)
        run_layout.addLayout(button_row)

        button_row2 = QtWidgets.QHBoxLayout()
        self.capture_once_button = QtWidgets.QPushButton("Capture Once")
        self.load_button = QtWidgets.QPushButton("Load Log")
        self.clear_button = QtWidgets.QPushButton("Clear")
        button_row2.addWidget(self.capture_once_button)
        button_row2.addWidget(self.load_button)
        button_row2.addWidget(self.clear_button)
        run_layout.addLayout(button_row2)

        self.start_button.clicked.connect(self._start_continuous)
        self.stop_button.clicked.connect(self._stop)
        self.capture_once_button.clicked.connect(self._capture_once)
        self.load_button.clicked.connect(self._load_log)
        self.clear_button.clicked.connect(self._clear)
        layout.addWidget(run_group)

        status_group = QtWidgets.QGroupBox("Embedded-Style Status")
        status_layout = QtWidgets.QFormLayout(status_group)
        self.state_label = QtWidgets.QLabel()
        self.bursts_label = QtWidgets.QLabel()
        self.samples_label = QtWidgets.QLabel()
        self.bytes_label = QtWidgets.QLabel()
        self.overwritten_label = QtWidgets.QLabel()
        self.missed_label = QtWidgets.QLabel()
        self.crc_label = QtWidgets.QLabel()
        self.error_label = QtWidgets.QLabel()
        status_layout.addRow("State", self.state_label)
        status_layout.addRow("Records", self.bursts_label)
        status_layout.addRow("Samples", self.samples_label)
        status_layout.addRow("Bytes", self.bytes_label)
        status_layout.addRow("Overwritten", self.overwritten_label)
        status_layout.addRow("Missed", self.missed_label)
        status_layout.addRow("Latest CRC", self.crc_label)
        status_layout.addRow("Last error", self.error_label)
        layout.addWidget(status_group)

        layout.addStretch(1)
        return panel

    def _start_continuous(self) -> None:
        self.device.start()
        self.latest_sequence_seen = -1
        self.last_period_start = time.perf_counter()
        self.timer.start()
        self._update_status()
        self._append_event("Start: continuous 30 ms burst acquisition.")

    def _stop(self) -> None:
        was_running = self.timer.isActive()
        self.timer.stop()
        self.device.stop()
        self._update_status()
        if was_running:
            status = self.device.status()
            self._append_event(
                f"Stop: records={status.records_written}, missed={status.missed_bursts}"
            )

    def _device_period_tick(self) -> None:
        now = time.perf_counter()
        if self.last_period_start and (now - self.last_period_start) > (BURST_PERIOD_SEC * 1.5):
            self.device.inject_missed_burst()
        self.last_period_start = now

        self.device.capture_once()
        self._poll_device_memory()

    def _capture_once(self) -> None:
        if self.timer.isActive():
            return

        self.device.start()
        self.device.capture_once()
        self._poll_device_memory()
        self.device.stop()
        self._update_status()

    def _poll_device_memory(self) -> None:
        record = self.device.memory.latest()
        if record is None:
            self._update_status()
            return

        if record.sequence != self.latest_sequence_seen:
            self.latest_sequence_seen = record.sequence
            self._plot_record(record)
            self._append_event(
                f"Read memory record {record.sequence}: "
                f"{record.sample_count} samples, CRC=0x{record.crc32:08X}"
            )

        self._update_status()

    def _load_log(self) -> None:
        path, _filter = QtWidgets.QFileDialog.getOpenFileName(
            self,
            "Load simple burst log",
            str(SIMULATOR_DIR / "logs"),
            "Burst logs (*.u12bin);;All files (*.*)",
        )
        if not path:
            return

        records = self._read_simple_log(Path(path))
        if not records:
            self._append_event(f"No records found in {path}")
            return

        sequence, timestamp_us, sample_count, crc32, samples = records[-1]
        record = MemoryRecord(
            sequence=sequence,
            timestamp_us=timestamp_us,
            sample_count=sample_count,
            sample_rate_hz=SAMPLE_RATE_HZ,
            crc32=crc32,
            samples=samples,
        )
        self._plot_record(record)
        self._append_event(
            f"Loaded {len(records)} records from {Path(path).name}; "
            f"latest={sequence}, samples={sample_count}"
        )

    def _read_simple_log(self, path: Path) -> list[tuple[int, int, int, int, list[int]]]:
        records: list[tuple[int, int, int, int, list[int]]] = []
        header_size = struct.calcsize("<IQII")

        with path.open("rb") as file:
            while True:
                header = file.read(header_size)
                if not header:
                    break
                if len(header) != header_size:
                    self._append_event("Truncated record header while loading log")
                    break

                sequence, timestamp_us, sample_count, crc32 = struct.unpack("<IQII", header)
                payload = file.read(sample_count * 2)
                if len(payload) != sample_count * 2:
                    self._append_event("Truncated sample payload while loading log")
                    break
                if (zlib.crc32(payload) & 0xFFFF_FFFF) != crc32:
                    self._append_event(f"CRC mismatch in record {sequence}")
                    break

                samples = list(struct.unpack(f"<{sample_count}H", payload))
                records.append((sequence, timestamp_us, sample_count, crc32, samples))

        return records

    def _plot_record(self, record: MemoryRecord) -> None:
        ch0 = record.samples[0::2]
        ch1 = record.samples[1::2]
        self.ch0_curve.setData(list(range(len(ch0))), ch0)
        self.ch1_curve.setData(list(range(len(ch1))), ch1)

    def _update_status(self) -> None:
        status = self.device.status()
        latest = self.device.memory.latest()

        self.state_label.setText(status.state)
        self.bursts_label.setText(str(status.records_written))
        self.samples_label.setText(str(status.samples_written))
        self.bytes_label.setText(str(status.bytes_written))
        self.overwritten_label.setText(str(status.records_overwritten))
        self.missed_label.setText(str(status.missed_bursts))
        self.crc_label.setText(f"0x{latest.crc32:08X}" if latest else "0x00000000")
        self.error_label.setText(status.last_error)

    def _append_event(self, text: str) -> None:
        timestamp = time.strftime("%H:%M:%S")
        self.event_log.append(f"[{timestamp}] {text}")

    def _clear(self) -> None:
        self.timer.stop()
        self.device.start()
        self.device.stop()
        self.latest_sequence_seen = -1
        self.ch0_curve.clear()
        self.ch1_curve.clear()
        self.event_log.clear()
        self._update_status()
        self._append_event("Cleared UI and simulated memory.")

    def closeEvent(self, event) -> None:  # noqa: N802 - Qt override name
        self.timer.stop()
        self.device.stop()
        super().closeEvent(event)


def main() -> int:
    app = QtWidgets.QApplication(sys.argv)
    window = AcquisitionMonitor()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
