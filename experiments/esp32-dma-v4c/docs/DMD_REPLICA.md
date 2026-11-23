# Opt-in DMD replica

`ttgo_tdisplay_dmd_replica` extends filtered validation plus the hardware-tested
DMA recovery loop. Its opt-in `ttgo_tdisplay_dmd_replica_fault_test` variant
injects one labelled validation failure after 256 good refreshes at slot 8.
Earlier diagnostic environments remain separate. Like their baseline, these
are bounded 4096-validated-refresh runs (~33.55 seconds), followed by a dump
and idle with the last LCD image held; they are not indefinite-uptime firmware.

## Display and SPI isolation

The ST7789 pins, rotation, byte swap, RGB565 palette and pushImage method come
from V1.1 commit 4b23094: MOSI19, SCLK18, CS5, DC16, RST23, BL4, 40 MHz,
135x240 panel rotated to 240x135. MISO is explicitly unused (-1).
The 128x32 image is at (56,51), 1x1 square pixels, with palette
0000/52AA/AD55/FFFF. The calibration strip and status text are outside its ROI.

Capture retains IDF SPI2_HOST (numeric host 1). Installed TFT_eSPI 2.5.43 selects
SPIClass(VSPI), Arduino port 3 / physical SPI3, unless an alternate-port macro
is set. The replica rejects HSPI/FSPI and asserts SPI_PORT==VSPI==3 at compile
time. Startup logs both host mappings. The inspected library source and setup
are preserved with evidence. TFT uses blocking CPU pushImage on SPI3, never
initDMA; the capture DMA channel is not reused for LCD writes.

## Decoder and CRC contract

Only buffers passing all row, filtered-RD, DCLK, SPI ownership/bit length and
worker sequence checks enter the pipeline. The capture worker copies a validated
binary refresh into a fixed pool before returning its original DMA buffer.
Twelve binary slots feed a decoder on core 0, below the capture worker's priority.

The initial candidate follows V1.1: sum three consecutive 512-byte MSB-first
binary refreshes into 4096 row-major intensity bytes, each 0..3. CRC32/IEEE hashes
exactly these 4096 unscaled intensity values, before RGB565 conversion or text.
The grouping origin is the first complete aligned refresh of an acquisition
epoch. No absolute game grayscale phase is asserted. Static periodicity and
three offset comparisons can support grouping stability; physical dim/bright
correctness, scene-transition phase and row/orientation agreement still need
visual comparison with Fish Tales and must be recorded as verified or unknown.

A capture gap increments an epoch immediately. The decoder's owner task clears
any partial group upon the first new-epoch refresh (or candidate discontinuity).
No old partial group can publish across the gap. Previously completed valid
pre-gap CRC records remain legitimate records. The LCD discards stale queued
old-epoch images and keeps its last complete image while waiting for three fresh
validated refreshes. A valid transfer already in flight at fault detection may
finish; the reported hold starts at the last completed transfer before the first
new-epoch draw, rather than claiming an instantaneous physical display latch.

Each complete group gets an increasing GRAY n and micros timestamp; repeated
CRCs are emitted again. Serial record queue capacity is 64, LCD queue capacity
is one newest image. Each uses nonblocking enqueue/overwrite. Every completed
image generates its own CRC record even if the LCD skips it. Binary slot drops,
renderer skips and serial CRC queue losses have separate totals; binary loss
also forces a fresh group. No UART write, TFT write or RGB conversion occurs
inside acquisition or fastReopen. Allocation is checked and fixed at startup;
there is no allocation in capture/decoding/rendering.

`GRAY n=... us=... epoch=... first=... last=... crc=... hist=...` records every
complete grayscale image. Candidate IDs identify its exact three source refreshes.
`GRAY TAIL` retains all 4096 last-complete intensity values as ASCII 0..3 for
read-only evidence (ASCII is not the CRC input). Existing raw CRC/pixel and
publication-ID dumps remain available. Host analysis verifies CRC record counts,
all group boundaries, no crossing of a discarded capture, and reconstruction
of the last ten grayscale images directly from saved binary pixels.

## Counters and recovery evidence

Errors counts validation faults once, Recoveries counts successful realignments,
and Filtered RD counts stable positive raw-minus-filtered edge deltas. Raw-only
observations do not increment Errors. In the opt-in injected test, the artificial
fault takes the same path and increments Errors/Recoveries once, while injected=1
and the LCD title identify it as simulated; genuine fault count is Errors-injected.
Binary/renderer/serial drops do not masquerade as capture faults.

LCD status writes Errors, Recoveries, Filtered RD, renderer/serial drop totals
and latest grayscale CRC. `LCD STATUS` logs the final values actually written by
the status task; this proves software writes, not an independent optical readback.

`GRAY HOLD` logs held image number/CRC, last completed draw, first resumed image,
redraw start/end, hold-to-start, hold-to-complete and detection-to-redraw timings.
These are software timing observations. A held complete image is not cleared or
replaced by a partial group. All trace/summary/tail printing is after capture;
the per-image CRC stream runs through the separate serial task during capture.

Run tests with `python3 -m unittest discover -s scripts -p 'test_*.py'`.
`test_grey_decoder.py` compiles and executes the exact portable C++ decoder,
including four shades, CRC32 compatibility/brightness sensitivity and gap clearing.
Use `scripts/analyse_replica_log.py RAW_LOG` for complete read-only batch audits.
Captures remain local/ignored. Physical LCD feedback is recorded separately from
software counters and reconstruction checks.
