# RD phase investigation

On 6 November the Fish Tales attract-mode capture stopped after 107 complete
refreshes and four accepted rows. On 14 November V4c stopped after 1914 complete
refreshes and 15 accepted rows. The machine pattern for the second run was not
recorded. Both failing rows reported RCLK/DCLK/RD deltas of 1/128/1. The RD
checks expected zero rising edges at those row positions. The earlier logs did
not print the sampled RD level or the preceding row history.

## Run the trace on the Mac mini

Open the repository in local VS Code with the TTGO attached. Close any serial
monitor, keep the V4c wiring and local CS jumper, and put Fish Tales in normal
attract mode. Use the project helpers from `experiments/esp32-dma-v4c`:

```sh
sh scripts/build.sh ttgo_tdisplay_rd_trace &&
python3 scripts/upload.py --environment ttgo_tdisplay_rd_trace \
    --port /dev/cu.usbserial-02073BE4 --confirm-upload &&
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --duration 45 --reset
```

Run `python3 scripts/analyse_log.py` with the exact `.txt` path printed by
capture. Confirm `RD TRACE enabled` in the saved startup and retain the raw log
and JSON metadata locally. Record the commit or diff, firmware/configuration
hashes, PIO version, local timestamp, actual machine mode, reset method, final
result and raw log SHA-256 in the investigation record. Use the same tracing
firmware for a run in diagnostic mode with a known displayed pattern.

## Interpret the trace

The optional `ttgo_tdisplay_rd_trace` environment retains the existing checks
and stops on the first failure. It prints the last 64 complete counter snapshots
after chip select is closed, including a failing snapshot on a row continuity
or RD phase mismatch. The usual `ttgo_tdisplay` environment compiles without
trace instrumentation. The original firmware and config are recorded at
`415ef81657a8e3d2a3e776f7c29c53620bba6e26` and in `EXPERIMENT_BASELINES.json`.

- `index` is the zero-based candidate row after initial alignment, `slot` is
  its index modulo 32, and `expected` is the level and rising-edge delta checked
  by V4c. These are acquisition positions until physical row mapping is confirmed.
- `RCLK`, `DCLK` and `RD` are raw hardware counter snapshots, each wrapping at
  30000. `level` is RD read after the counters. The analyser calculates modular
  deltas from adjacent complete trace records when available.
- `isrCycles` is the latest RCLK ISR timestamp sampled just before the counter
  reads. `sampleEndAfterISR` measures cycles through those reads and the GPIO
  level read. These are software observations. `notify>1` or unexpected clock
  deltas mean the snapshot cannot be assigned cleanly to one RCLK boundary.
- An unexpected RD count with level still low suggests an edge completed before
  sampling or occurred near the counter/level reads. A high level at the
  unexpected slot shows RD was high at the sample. Either result needs a
  waveform to establish pulse width and source.

Compare the first failure with the preceding accepted row and previous expected
RD marker. If needed, scope level-shifted RD and RCLK at the ESP32 and correlate
against the trace. Compare pulse width and row interval with the diagnostic
pattern before changing filtering or the 32-row framing rule. Compare
`maxReopen` with the existing 100 µs deadline because instrumentation adds
some work and may alter scheduling.

## Validation on 14 November 2026

Both V4c environments built with PlatformIO Core 6.1.13, Espressif32 6.3.2,
Arduino ESP32 2.0.9 and Xtensa GCC 8.4.0 (2021r2 patch 5). The registry was
unreachable from the development workspace, so the build used an isolated
cache populated from the upstream releases. RD tracing added 1536 bytes of
static RAM and 756 bytes of flash in this build. After stripping debug metadata,
the default build's application object matched the one built from the original
source at the recorded main commit. The seven hardware-free automation tests
passed, including RD trace decoding across counter wrap. The ESP32 serial port
is on the Mac mini. The trace has yet to be flashed and run there.
