# Command Protocol

This protocol lets the GUI control either the simulator or a future STM32U5A5 firmware build using the same commands.

```text
Simulator: GUI -> TCP localhost -> simulator
Hardware:  GUI -> USB CDC virtual COM port -> STM32U5A5
```

The protocol is line-based ASCII so it is easy to test, easy to type manually, and easy to parse in embedded C.

## Default Acquisition Settings

| Field | Value |
|---|---:|
| Sample rate | 15,000,000 samples/s during burst |
| Samples per burst | 2046 |
| Burst period | 30 ms |
| Sample format | 12-bit ADC data stored as `uint16_le` |
| Preview data | Decimated, not full-rate |

## States

| State | Meaning |
|---|---|
| `IDLE` | Ready, not configured. |
| `CONFIGURED` | Valid acquisition settings loaded. |
| `ARMED` | Waiting for the next 30 ms capture period. |
| `CAPTURING` | 2046-sample burst in progress. |
| `WRITING` | Burst is being written to memory/log storage. |
| `FAULT` | Acquisition stopped because of an error. |

Normal loop:

```text
ARMED -> CAPTURING -> WRITING -> ARMED
```

## Commands

| Command | Purpose | Example response |
|---|---|---|
| `PING` | Check connection. | `OK PONG device=STM32U5A5_SIM proto=0.2` |
| `CONFIG 15000000 2046 30 32` | Set sample rate, burst size, period, preview decimation. | `OK CONFIG ...` |
| `START` | Start periodic burst acquisition. | `OK STARTED state=ARMED` |
| `STOP` | Stop after current burst/write completes. | `OK STOPPED bursts_written=12 missed_bursts=0` |
| `STATUS?` | Report state and counters. | `STATUS state=ARMED bursts_written=12 missed_bursts=0 last_error=NONE` |
| `RESET` | Clear fault state. | `OK RESET state=IDLE` |
| `FAULT STORAGE_WRITE` | Simulator-only fault injection. | `OK FAULT injected=STORAGE_WRITE state=FAULT` |

## Example Session

```text
> PING
OK PONG device=STM32U5A5_SIM proto=0.2

> CONFIG 15000000 2046 30 32
OK CONFIG sample_rate_hz=15000000 samples_per_burst=2046 burst_period_ms=30 preview_decimation=32

> START
OK STARTED state=ARMED log=logs/capture_0001.u12log

> STATUS?
STATUS state=ARMED bursts_written=12 samples_written=24552 written_bytes=49104 missed_bursts=0 last_error=NONE

> STOP
OK STOPPED bursts_written=12 missed_bursts=0
```

## Error Format

All errors use one simple format:

```text
ERR code reason=detail
```

Examples:

```text
ERR INVALID_CONFIG reason=samples_per_burst_out_of_range
ERR BAD_STATE reason=cannot_start_from_fault
ERR IO_ERROR reason=log_open_failed
```

## Log Contract

Each 30 ms period produces at most one burst record.

Each record contains:

- burst sequence number
- timestamp
- 2046 sample words
- CRC32
- status flags

Samples are stored as `uint16_le`; the lower 12 bits contain ADC data.

## First Milestone Criteria

The simulator protocol is complete enough for interview demonstration when:

- `CONFIG 15000000 2046 30 32` is accepted.
- `START` produces one burst every 30 ms.
- each burst logs 2046 12-bit samples.
- `STATUS?` reports burst counters and missed bursts.
- fault injection can force `FAULT` state.
- the command parser is simple enough to port to embedded C.
