# Attract-mode sequence capture

Build environment: `ttgo_tdisplay_attract_sequence`. It extends the RD trace
build and preserves its 64-row trace and stop checks. At the end of a run, it
writes one CRC32 for each complete 512-byte refresh and the raw pixels of the
last 32 complete refreshes. The worker reuses the existing CRC computation and
copies pixels before returning the DMA buffer. The history is printed only
once capture ends. A mid-refresh failure has no validated pixels for the
incomplete refresh. The capture records display-stream data and RD timing, not
the game's internal state or commands.

Use the verified-device upload helper with the new environment and collect
`--duration 55 --reset` for each run. The longer serial capture gives the
sequence dump time to finish after a 33.55 s run. Confirm `ATTRACT SEQUENCE
enabled` at startup, `mode=MOVING` and a matching `SEQ END`. Keep both successful
and failed captures, with a separate directory or explicit filename list for
the new logs. Use the existing analyser for the counts, handover timing and
failing row, then run:

```sh
python3 scripts/compare_sequences.py path/to/run-1.txt path/to/run-2.txt
```

The comparison collapses adjacent equal signatures into content runs,
retains their dwell, and looks for the last three content runs before each
stop anywhere in the supplied histories. `--length` changes the comparison
length. A match followed by another content change is a useful control.
The final dwell or a capture's first dwell can be partial. A matching suffix
across failures is a candidate association to retest. CRC differences within
grayscale refresh sequences, finite capture windows, and inconsistent start
points can hide a relationship. Group RD phase stops separately from
row continuity and buffer handover stops. Compare maxReopen with the prior
trace build because the added worker copies can affect timing.
