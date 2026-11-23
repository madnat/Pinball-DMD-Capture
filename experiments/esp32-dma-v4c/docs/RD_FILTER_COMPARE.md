# RD filter comparison diagnostic

`ttgo_tdisplay_rd_filter_compare` extends the attract-sequence environment with
`DMD_RD_FILTER_COMPARE`. Only this build adds PCNT unit 3, channel 0, on GPIO26:
rising increments, falling disabled, control disabled/KEEP, limits 0/30000.
The sketch and its SPI/Arduino dependencies allocate no other PCNT unit 3 user;
compile-time checks keep it distinct from RCLK=0, DCLK=1, raw RD=2.
The legacy driver has no unit ownership query: availability is a source audit,
not a claim that a runtime allocator reserved it. No PCNT interrupt is added.

The filter threshold is 80 APB cycles on unit 3 only. Startup checks the API
threshold readback, unit-3 hardware enable bit, raw-unit-2 disabled filter bit,
and APB frequency. At 80 MHz this is nominally 1 microsecond. Startup records
actual APB Hz and a CPU-cycle settling interval. Counter start offsets may differ;
all comparisons use each counter's own modulo-30000 deltas, never raw-minus-filtered
absolute values. Original ESP32 only, as required by V4c's existing build guard.
Reference: https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/pcnt.html
Installed declarations/register layout were checked in driver/pcnt.h and
soc/pcnt_struct.h of Arduino ESP32 2.0.9.

The existing raw RD read still precedes the GPIO level sample. Added CPU timestamp
instructions have overhead; no filtered read or settling wait is inserted before
the GPIO sample. `sampleEndAfterISR` still measures through that GPIO sample,
rather than the extra diagnostic reads. Supplemental timestamps log the exact
software order (CPU ccount, unsigned modulo 2^32):

1. Original RCLK and DCLK driver reads.
2. rawStart -> raw RD read -> rawEnd -> GPIO level -> levelEnd.
3. filteredStart -> first filtered read -> filteredEnd.
4. Wait only if necessary until rawEnd + one 80-APB-cycle interval.
5. settledStart -> filtered confirmation read -> settledEnd.
6. Raw confirmation read -> rawAfterEnd.

A comparison is ambiguous if either endpoint snapshot has a moving raw or
filtered bracket, or row/notification continuity is nonunit. Stable brackets
exclude observed movement across the sequential read window. They do not prove
waveform shape, source, exact pulse width, or the absence of an edge immediately
before the first raw read. The delayed confirmation helps expose a pending
filter response. A raw/filtered difference is diagnostic evidence, never a new
STOP rule or permission to resynchronize.

No DMA readiness/reopen code, deadline, pin assignment, raw row/phase test,
worker/sequence rules or legacy log lines are replaced. Extra reads/copies can
still change scheduling, so compare maxReopen with the previous 70–73 us range.

`RD FILTER ANCHOR` supplements the alignment anchor; `index=4294967295 anchor=1`
is an anchor sentinel, not a sampled candidate row. `RD FILTER ROW` supplements
every retained `RD ROW`, including any failing row. Raw/filtered deltas in the
anchor are placeholders; its bracket counts/times remain valid. The whole-run
`RD FILTER SUMMARY` counts every recorded row, expected marker agreement,
disagreements and ambiguous comparisons, including passing runs. The first
16 disagreements retain both previous/current supplemental snapshots as
`EVENT_PREV`/`EVENT`; totals flag any truncation. The normal 64-row trace and full
CRC/pixel sequence dump remain intact. All diagnostic printing happens after
capture stops. `scripts/analyse_filter_log.py` validates these records in addition
to the existing sequence/pixel CRC validation; it reports JSON without serial access.

Hardware batch stops after ten captures or two comparable raw RD phase stops and
two complete passes. Any incomplete dump halts the batch. Two expected-marker
failures to count one edge in both units also halt it for review, including
ambiguous cases. A comparable RD stop requires stable read brackets at both
endpoints and agreement on all preceding expected markers; it does not establish
an electrical pulse-width measurement. All raw bytes/sidecars remain local.
