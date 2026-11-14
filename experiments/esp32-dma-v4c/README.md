# ESP32 DMA Investigation V4c — fast handover and callback timing

Original ESP32 TTGO T-Display only. PlatformIO Arduino. Serial 115200.
Built and uploaded successfully on 2026-11-14. The recorded hardware run stopped
at 1914/4096 frames with RD phase mismatch; this is a tested experiment, not a
passing sustained-capture baseline. See [engineering journal](docs/ENGINEERING_JOURNAL.md).
For local build/upload/capture helpers, see [automation](docs/AUTOMATION.md).
Preserve working V3 plus V4a/V4b source projects in separate directories.

## Evidence motivating the change

V4b initialized correctly and captured/processed one refresh. Its latest
report had arm observed at 85 us, reopen decision at 106 us, no counted DCLK
edges while CS was high, workerError=0, and STOP: CS reopen deadline.
An earlier run reported observed arm=93 us and decision=113 us.
Those times did not isolate the counter API cost from instrumentation and
flash execution, and observed readiness was not an actual callback timestamp.

V4c makes the complete handover a small IRAM routine, with no driver calls,
queues, printing, division or pixel processing in that routine. It uses CPU
cycles for comparisons, leaving unit conversions/reporting until afterwards.
The original 100 us deadline remains unchanged.

## Specific changes

- Timestamp entry to the SPI post_setup callback, before publishing its
  generation counter. This is software callback time after the driver arms
  hardware, not a direct measurement of the hardware-ready instant.
- Wait for readiness in IRAM; read the PCNT unit-1 count directly from its
  read-only register; require no DCLK edges since the closing snapshot.
- Reopen CS before bookkeeping, retaining before/after deadline checks.
- Read-only access targets original ESP32 PCNT_U1_CNT_REG at 0x3FF57064,
  low 16 bits. A build guard rejects other ESP32 variants and a static
  assertion requires DOT_PCNT=PCNT_UNIT_1.
- At startup, perform 16 comparisons between the direct read and bracketing
  driver reads, accounting for counter movement/wrap. Abort if inconsistent.
- Retain all per-row hardware counts, RD checks, SPI length checks, sequence
  checks, static byte comparisons, saved final diagnostics and 4096-frame
  target. The normal driver still initializes/configures the counters.

No counter clears or writes are added during streaming. No SPI data edge,
mode, pin assignment, buffer size or queue depth is changed.

## Wiring — unchanged

All J603 inputs remain AFTER the 5 V -> 3.3 V level shifters.

| Signal | J603 pin | ESP32 GPIO |
|---|---:|---:|
| RD | 3 | 26 |
| RCLK | 5 | 25 |
| DCLK | 9 | 33 |
| SDATA | 11 | 13 |
| Local CS output | No J603 connection | 27 |
| Local CS input | No J603 connection | 32 |

Retain GPIO27 -> GPIO32 and common ground. GPIO27 must never connect to
J603 signals. No LCD rendering, Wi-Fi or Bluetooth is started.

## Run

1. Leave the existing pattern paused throughout the approximately 34 s test.
   Exact line position is not important for this byte-comparison test.
2. Extract/open V4c separately in PlatformIO. The current tested source uses
   EXPECT_STATIC=false (MOVING mode). Changing mode is a firmware change and
   requires a separate experiment; do not change it during infrastructure work.
3. Close other monitors. Build ttgo_tdisplay, upload explicitly to
   /dev/cu.usbserial-02073BE4 and confirm successful upload.
4. Monitor at 115200. Verify the identity says V4c. Reset if necessary to
   capture the full startup trace.
5. Share FINAL, Handover, Timing, Last deltas and RESULT/STOP, or the saved
   final report repeated with IDLE. Include any startup failure/backtrace.

Extra startup checkpoints 09a/09b bracket the direct-read validation.

## Timing interpretation

All offsets are relative to entry into the RCLK ISR, not an oscilloscope
measurement from the physical RCLK edge. ISR latency remains unmeasured.

- callback: recorded at SPI post_setup callback entry.
- observedReady/arm: when the task observes the new callback generation.
- reopen: time of the CS-low write (or last pre-write check if refused).
- directRead: cycles for the direct register read and timing instrumentation.
- maxReopen: largest offset following a CS-low write.
- maxCSHigh: longest observed interval between local CS closing and reopening.

callbackSeen=0 means no new callback generation was observed. A zero callback
value in that case is a placeholder, not evidence of instantaneous readiness.
DCLKwhileHigh=-1 means no counter read occurred, such as when readiness timed
out or no next buffer was requested at the final target boundary.
reopened=0 means the last handover was not attempted or was refused.

Saved summaries repeat timing and count evidence after completion, so a late
monitor connection is sufficient to recover the final diagnostics.

## Pass criteria and limits

A static pass still requires all 4096 frames processed, 131072 checked rows,
16777216 DCLK edges, 4096 RD rising edges, every SPI length=4096 bits,
staticDiff=0, workerError=0, and all deadline/continuity checks passing.
A normal ending prints RESULT: STATIC STREAM PASS.

A failure remains useful evidence. If callback readiness itself is late,
this optimization will not fix that driver latency. If post-opening work
misses a later row notification, the row checks still stop the run. The code
does not resynchronize automatically or hide skipped rows.

Passing establishes one bounded sustained run on the selected pattern with
interrupts enabled and buffers processed on the other core. It does not
establish indefinite uptime, TFT rendering or animated pixel correctness.

## Primary references

Espressif original ESP32 Technical Reference Manual, PCNT register summary:
PCNT_U1_CNT_REG at 0x3FF57064.
https://documentation.espressif.com/esp32_technical_reference_manual_en.pdf

https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/spi_slave.html
https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/pcnt.html
