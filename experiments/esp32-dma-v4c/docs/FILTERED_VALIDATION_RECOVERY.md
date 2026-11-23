# Opt-in filtered validation and controlled recovery

All earlier environments remain available. `ttgo_tdisplay_filtered_validate`
(test A) inherits the 80-cycle unit-3 RD comparison and changes only the RD
validation authority: filtered-counter deltas establish the initial marker and
must be zero except for one edge at slot 31. The filtered confirmation read
must be stable. Raw unit 2 and GPIO level are observers, not vetoes. RCLK,
DCLK, notification, DMA descriptor/length/sequence, worker and 100 us handover
checks remain active. A mismatch stops test A; it does not recover.
`RD RAW_ONLY` records are printed after capture with zero-based row/slot and
one-based candidate output frame. Existing disagreement retention is bounded
at 16 detailed events with explicit saved/total counts; any truncation must be
reported rather than claiming every event has a retained row record.

Test B uses `ttgo_tdisplay_filtered_recovery` (slot 8) and its configuration
variant `ttgo_tdisplay_filtered_recovery_slot20`. Each image injects exactly
one software validation failure at its chosen slot after at least 256 complete
validated refreshes and with the worker caught up. Injection occurs before
accepting the candidate row or publishing its suspect refresh. A genuine
RCLK/DCLK/notification or filtered-RD mismatch enters the same fault block.
Every recovery is labelled by cause and `injected`; no synthetic hardware
edge or clock is claimed. Eight recovery events are the hard audit limit;
unknown ownership or failure to align stops the diagnostic.

Recovery policy in src/recovery_loop.h:

1. Timestamp detection, close local CS and identify the current suspect capture.
2. Reclaim exactly that descriptor through `spi_slave_get_trans_result`, with a
   bounded 2 ms wait. Never clear/free/reuse its RX memory before ownership returns.
3. Discard its data, reset that returned descriptor and requeue it. Nothing from
   the suspect refresh is sent to the worker, even if it happened to have 4096 bits.
4. Require a new setup generation and a different known armed descriptor. No
   host driver teardown or direct DMA reset occurs. The SPI slave ISR handles its
   own ESP32 DMA workaround when the partial transaction terminates.
5. Keep CS high, explicitly start a new filtered alignment epoch without clearing
   any running counter, then open only at a verified quiet marker within 100 us
   of its RCLK ISR timestamp. Drain stale task notifications before seeking sync.
6. Validate all 32 rows and all 4096 SPI bits of the next candidate before queueing
   it for publication. Normal subsequent handovers use the unchanged fastReopen.

The matching ESP-IDF v4.4.4 SPI driver was inspected at
https://github.com/espressif/esp-idf/blob/v4.4.4/components/driver/spi_slave.c .
Its ISR stores transaction length, applies any DMA workaround, returns the
finished descriptor to ret_queue and prepares a queued next transaction before
calling post_setup_cb. The exact inspected source is retained with local evidence.
This is why reclaiming the partial result and verifying the next callback generation
is used instead of freeing a driver with active/queued buffers.

The worker additionally rejects buffers without 32 validated rows or whose
monotonic capture ID appears in the suspect list. `PUB IDS` lists every output
sequence's capture ID: output sequences remain contiguous; discarded capture
IDs must be missing. The host audit rejects suspect IDs, duplicates, unrecorded
gaps, incomplete dumps or a recovery followed by no further publication.

Timing definitions (all unsigned 32-bit micros values, wrap handled by subtraction):

- detectionToGoodUs: first complete good refresh's CS-close timestamp minus fault
  detection. It measures the capture endpoint, not completion of pixel processing.
- goodGapUs: that first good CS-close timestamp minus the preceding good refresh's
  CS-close timestamp. This includes the discarded refresh and reacquisition gap.
- firstGoodValidatedUs records the later descriptor/length-validation point.
- detectionToPublishUs and publicationGapUs use the worker's timestamp immediately
  before releasing its CRC/pixel history publication. This includes processing;
  timestamp/fence overhead is not a physical output-pin measurement.

Only a single initial helper reset is requested for each 55-second serial capture;
recovery never resets the ESP32. Every fault is printed after capture ends. First
post-recovery candidate IDs, dropped bit lengths, new alignment anchors, timestamps
and all output IDs are retained. Scope work and electrical pulse diagnosis remain
separate. A successful recovery run is labelled `RECOVERY VALIDATED OUTPUT PASS`,
not an uninterrupted stream pass. V4c FINAL totals in this build describe only
complete validated output refreshes; discarded/partial rows are disclosed separately
by fault records and the filter summary. The old sequence loader accepts this result
only when explicitly called with allow_recovery=True; default legacy semantics remain.
