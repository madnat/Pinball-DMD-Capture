> Copied from the original V4c workspace as historical infrastructure documentation.
> Current repository layout/status: ../../../docs/DEVELOPMENT_AUTOMATION_REVIEW.md.
> Prior raw logs and backup archives remain in the original V4c folder; they are not copied here.

# Engineering journal

## 2026-11-14 — V4c hardware evidence from the preceding authorized session

Original ESP32 TTGO T-Display, CP2104 serial 02073BE4, 115200 baud.
PlatformIO ttgo_tdisplay; espressif32@6.3.2; Arduino ESP32 2.0.9; Core 6.1.13.
Existing firmware built and uploaded successfully using the specified pio
executable; flash hashes verified and uploader reset via RTS. No source changes.
The later capture opened before an explicit RTS reset and saved 45 seconds.
Wiring was not physically inspected; actual pinball pattern was not recorded.
Firmware reported MOVING mode, CPU 240 MHz, startup checks passed.

Raw log: diagnostics/ttgo_serial_20261114_130746.txt (9,851 bytes).
Full startup and early-failure/IDLE reports preserved. Port closed after capture.
Result: STOP: RD phase mismatch, 1914/4096 frames, 1914 processed, elapsed
15,682,207 us; rows=61,263, DCLK=7,841,664, RD=1,914. This is not a full pass.
No root-cause investigation is part of this infrastructure task.

Max arm wait=98 us, max reopen=98 us against 100 us deadline, max CS high=79 us;
last arm/reopen=39 us. Callback last/max=35/87 us; direct read last/max=14/19
cycles. Last deltas RCLK/DCLK/RD=1/128/1, DCLKwhileHigh=0, workerError=0,
staticDiff=0 (MOVING mode, not evidence of a static comparison pass).
First CRC=57FF91FB; last printed periodic CRC=5179DD68 at frame 1844, not a
final-frame CRC. No final CRC was printed. CRC variation alone is expected
in MOVING mode. Initial boot bytes contain a partial/garbled ROM fragment;
all raw bytes remain intact and the application startup is readable.

Baseline SHA-256:

```text
9efe4251b760beb2a900f6e2c2655d09940d6cd229996d0852e4dbe4000b6d56  src/main.cpp
6c65d52eb4efa8b71c94f3b5527c502acabdbb434ed622ec81b9532ad28edc22  platformio.ini
9e5dd849302390a950a15ef1faccf771c802133d33e4376bc099038495c2dc8e  README.md (before automation)
5f6716e61e4bbc071107e9d3e78e51ea071b8e3ad35ae69a01d07434f928abe4  diagnostics/ttgo_serial_20261114_130746.txt
```

Backup: diagnostics/baselines/v4c_20261114_before_automation.zip, containing
these four files before documentation edits. Preserve it locally; raw logs and
archives are ignored by default.

## 2026-11-14 — infrastructure only

Added build, explicit verified upload, bounded raw capture and read-only log
analysis helpers, project guidance, documentation, hardware-free tests and a
prepared GitHub Actions build workflow. Corrected the README's obsolete claim
that this workspace was untested and its stale STATIC-mode instruction.
Source/configuration and prior diagnostic evidence were preserved unchanged.
No hardware port was opened, reset requested, firmware uploaded, or acquisition
test started during this task. No Git history/state operations or user-level
Codex configuration changes were performed.

Validation:
- `sh scripts/build.sh`: SUCCESS; RAM 24,172/327,680 bytes (7.4%), flash
  286,689/1,310,720 bytes (21.9%). Expanded cache access was approved. PlatformIO
  warns Core 6.1.13 is older than previously used 6.2.0; no tools were upgraded.
- Shell syntax and Python AST checks passed for every helper/test script.
- Hardware-free unit tests passed: bounded 0.01/0.45/45-second simulated
  captures, byte preservation, controlled reset, closure on open/read/reset
  failures and interruption, invalid duration, USB mismatch and log analysis.
- All helper --help CLIs passed. Invalid durations 0, -1, nan, inf and invalid
  text returned code 2 before serial access. Missing upload confirmation was
  refused before serial access.
- Read-only analysis of the preserved log reported startup=yes, full pass=no
  and the expected complete failure statistics.
- Real-device capture/upload helper behavior remains intentionally untested
  in this task; the prior manual controlled capture is separate evidence.
- GitHub Actions is prepared locally; no remote execution/push was performed.
- Workflow YAML parsed with the already installed PyYAML; triggers and the
  build-only ttgo_tdisplay command verified. No package installation required.
- Git ignore checks confirmed .txt/.json diagnostics and baseline archives are
  excluded. Archive integrity and SHA-256 checks confirmed original source,
  platformio.ini and the existing diagnostic log remain unchanged.
- Final managed repository status remains clean on feature/v1.2a-acquisition;
  V4c remains outside Git. Build artifacts remain under ignored .pio/.

For each future experiment append date/time and timezone, objective, exact
source/config hashes or commit, tool versions, approved command, hardware and
pattern/mode, reset method, raw log path/hash, complete-vs-partial test result,
counts/timing/CRC/error evidence and any limits or unknowns. Never replace raw
failure evidence with a summary or treat absence of an error as a complete pass.

## 2026-11-14 — experiment separation confirmed

The user confirmed V4c and v1.2a-acquisition are different tests and must be
maintained separately in the repository. Updated AGENTS.md and the integration
proposal to require independent projects/configurations/evidence/builds, with
V4c proposed under experiments/ESP32_DMA_Investigation_V4c/. Existing root
firmware remains untouched. The feature branch's V1.2g source and main's V1.2a
RCLK source must retain their actual identities. Documentation only; no source,
managed repository, hardware or Git state changes.

## 2026-11-17 — Fish Tales normal attract sequence investigation
User authorized one diagnostic flash and up to ten 55-second captures, one explicit
ESP32 reset per capture; stop at two RD phase stops plus two passes, or ten runs.
Fish Tales normal attract mode was reported by the user and left running between
ESP32 resets. No game controls or oscilloscope were operated. Physical wiring and
waveforms were not independently inspected. Logged pin configuration remained
RD=26, RCLK=25, DCLK=33, SDATA=13, local CS jumper 27 -> 32; retained
level-shifted J603/common-ground wiring is assumed from the existing setup.

Worktree: /Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence
Branch: experiment/esp32-attract-sequence-recovery; HEAD 415ef81657a8e3d2a3e776f7c29c53620bba6e26
plus recovered RD trace and attract sequence patches. No firmware edits were made
in this batch. GitHub, commits, branches and stale worktree registration unchanged.
All timestamps below are Europe/London (UTC +00:00).

Nine hardware-free Python tests passed. Diagnostic build passed: RAM 58,484 bytes,
flash 288,061 bytes. Python 3.12.2, PlatformIO Core 6.1.13, espressif32 6.3.2,
Arduino ESP32 2.0.9, Xtensa GCC 8.4.0+2021r2-patch5, esptool 4.5.1.
PIO warned that Core 6.1.13 is older than previously used 6.2.0; it installed
required tool-scons 4.40600.0 and removed 4.41101.0 during build resolution.
No tool/core upgrade was requested. Port freedom was checked with lsof against
both adapter aliases before upload and between captures. Upload helper verified
CP2104 10C4:EA60 serial 02073BE4 and flashed once successfully, with hash verification
and an uploader RTS reset. Each subsequent capture explicitly pulsed RTS/EN once
with DTR/GPIO0 deasserted. Startup is confirmed from each raw log, rather than
inferred from reset behavior. Early boot bytes, including a partial initial boot
banner in run 1, remain unfiltered in the raw logs.

Commands from experiments/esp32-dma-v4c (Python/PIO from
/Library/Frameworks/Python.framework/Versions/3.12/bin on PATH):

```sh
export PLATFORMIO_SETTING_ENABLE_TELEMETRY=no
python3 -m unittest discover -s scripts -p 'test_*.py'
sh scripts/build.sh ttgo_tdisplay_attract_sequence
python3 scripts/upload.py --environment ttgo_tdisplay_attract_sequence --port /dev/cu.usbserial-02073BE4 --confirm-upload
# Repeated once per numbered run, always allowed to complete:
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --duration 55 --reset
# Each exact raw path below was passed to:
python3 scripts/analyse_log.py RAW_LOG_PATH
```

The local batch driver /private/tmp/pinball_attract_batch.py invoked these exact
capture commands and validated each saved sidecar and dump before proceeding.
Validation used compare_sequences.load_capture: exactly one startup and FINAL,
MOVING mode, matching complete frame/worker counts, matching SEQ BEGIN/END,
a full per-refresh CRC timeline and up to 32 complete pixel payloads whose CRCs
match. Every valid capture ran 55 seconds, returned zero, recorded startup,
and reported port_closed=true; lsof found no holder afterward. No receive bytes
were purged. No capture/dump verification errors occurred.

### Provenance

```json
{
  "src/main.cpp": "87e4a00a7dd9d49d2859b019630e28290d726579b39dd25ebdfd6b3d91e48e34",
  "platformio.ini": "2231ac775bc3bc42f5d4578451dacc407186836c07baea9c9812c36c9dd20ea1",
  "scripts/capture.py": "451be39946cd2eb92b3dc67956f1942cd46599cd4e1341f9039f89bc19159045",
  "scripts/upload.py": "853d4cb539a305df4df76b81772b4f0d30c7a80b04758cf6ece7a8f705c39363",
  "scripts/analyse_log.py": "a95a2e5c8c5ea3ef32176118121b52eaa10a46afbb610dd5bd28ca4a399daded",
  "scripts/compare_sequences.py": "0a982ed31e534edf2331028f85ef21db3d50a78686df550c804a335063b5d70a",
  ".pio/build/ttgo_tdisplay_attract_sequence/firmware.bin": "9b371fbaf9a533bfa772d1d7c6acaff423de6eed2bc7265f57ed6ad6d64f3958",
  ".pio/build/ttgo_tdisplay_attract_sequence/firmware.elf": "0317630930e9e22ede0950aabe7d83703ab8deed81da5ce968cde4089d47db19"
}
```

Full local batch artifacts: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/attract_batch_20261117_164715+0000`. Source diff, untracked diagnostic source snapshots,
manifest, per-run analyser/capture output and exact comparison command are retained there.
The raw logs and sidecars remain local and ignored.

### Run results

Frames are complete processed refreshes out of 4096. Row indices/slots are zero-based
acquisition positions, not a confirmed physical display-row mapping. Last deltas on
passes describe the final accepted row, not a failure.

| Run | Start UTC | Outcome | Frames | maxReopen µs | Failing row / slot | Last RCLK/DCLK/RD |
|---:|---|---|---:|---:|---|---|
| 1 | 2026-11-17T16:47:15.847686+00:00 | PASS | 4096 | 71 | — | 1/128/1 |
| 2 | 2026-11-17T16:48:11.151591+00:00 | RD phase mismatch | 728 | 73 | 23303/7 | 1/128/1 |
| 3 | 2026-11-17T16:49:06.466998+00:00 | PASS | 4096 | 71 | — | 1/128/1 |
| 4 | 2026-11-17T16:50:01.764512+00:00 | PASS | 4096 | 70 | — | 1/128/1 |
| 5 | 2026-11-17T16:50:57.064250+00:00 | PASS | 4096 | 70 | — | 1/128/1 |
| 6 | 2026-11-17T16:51:52.379451+00:00 | PASS | 4096 | 70 | — | 1/128/1 |
| 7 | 2026-11-17T16:52:47.660348+00:00 | PASS | 4096 | 70 | — | 1/128/1 |
| 8 | 2026-11-17T16:53:42.960506+00:00 | PASS | 4096 | 70 | — | 1/128/1 |
| 9 | 2026-11-17T16:54:38.232309+00:00 | PASS | 4096 | 71 | — | 1/128/1 |
| 10 | 2026-11-17T16:55:33.543153+00:00 | RD phase mismatch | 2557 | 70 | 81836/12 | 1/128/1 |

Full evidence paths, SHA-256 values, timing/count/CRC and RD evidence follow.

Run 1:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164715_847642+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164715_847642+0000.json`
- Raw SHA-256: `df4b09c3a4cc6809939472ac20305b0197863205ce57594ebd0b762567afb251`
- Sidecar SHA-256: `6f1307a0dc85a0540d5b5b61ac6f92b542aa658bfb98b85c67e79bbd1baa6b08`
- Capture window: 55.017 s; 91834 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551465 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=38 us lastReopen=0 us maxReopen=71 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=63 us observedReady=38 us directRead=0 cycles maxDirectRead=24 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=38 us reopen=0 us maxReopen=71 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=38 us directRead=0 cycles maxDirectRead=24 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=67D75B8E lastCRC=67D75B8E elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=67D75B8E lastCRC=404FD954 elapsed=33551443 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 2:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164811_151553+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164811_151553+0000.json`
- Raw SHA-256: `f0b287279908f41d0e035fc55884f68b4acea88e3682ddb212460e31dd91e9ca`
- Sidecar SHA-256: `a56c77ec7432c1c03282b26bc92391acfe6696b1ef7d128cd7dc62dda8227325`
- Capture window: 55.016 s; 61390 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: no
V4c FINAL frames=728/4096 processed=728 rows=23303 DCLK=2982784 RDedges=728 elapsed=5965291 us maxArmWait=72 us maxCSHigh=53 us
Handover: lastArm=46 us lastReopen=46 us maxReopen=73 us reopened=1 DCLKwhileHigh=0 deadline=100 us
Timing: callbackSeen=1 callback=43 us maxCallback=65 us observedReady=46 us directRead=15 cycles maxDirectRead=1806 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
STOP: RD phase mismatch
Saved final: frames=728/4096 processed=728 rows=23303 DCLK=2982784 RD=728
Saved handover: arm=46 us reopen=46 us maxReopen=73 us reopened=1 DCLKwhileHigh=0
Saved timing: callbackSeen=1 callback=43 us observed=46 us directRead=15 cycles maxDirectRead=1806 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
RD trace failure: row index=23303, slot=7 (zero-based), expected RD level/edge=0, sampled level=0, notifications=1, sample end=4931 cycles after RCLK ISR timestamp.
RD trace deltas RCLK/DCLK/RD=1/128/1; sampled RD level 0->0.
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=6C1AE638 lastCRC=6C1AE638 elapsed=8207 us
Last CRC report: V4c processed=616/4096 staticDiff=0 firstCRC=6C1AE638 lastCRC=5F7CA39A elapsed=5045811 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 3:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164906_466961+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164906_466961+0000.json`
- Raw SHA-256: `9780669659ee4ff16a4ccbc1e7a838885edece3ba94ac49a6348c0b2b202a8c9`
- Sidecar SHA-256: `1b88b8b11bedaf6b6f9f4488b3abe80f43ceb1bb1ed0d016443c1b96faa24b7a`
- Capture window: 55.012 s; 91811 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551465 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=38 us lastReopen=0 us maxReopen=71 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=63 us observedReady=38 us directRead=0 cycles maxDirectRead=35 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=38 us reopen=0 us maxReopen=71 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=38 us directRead=0 cycles maxDirectRead=35 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=B3CD8D41 lastCRC=B3CD8D41 elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=B3CD8D41 lastCRC=C0034C7E elapsed=33551443 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 4:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165001_764480+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165001_764480+0000.json`
- Raw SHA-256: `c0538c739f67c2de330e8bd4cff0b83a3d22140195c5a66418c475baf55bcef2`
- Sidecar SHA-256: `b9616518759c12c8aaea7d1264bbd32632e792f618c975c66fee540ad735e308`
- Capture window: 55.016 s; 91811 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551457 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=39 us lastReopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=62 us observedReady=39 us directRead=0 cycles maxDirectRead=18 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=39 us reopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=39 us directRead=0 cycles maxDirectRead=18 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=17BE4CDC lastCRC=17BE4CDC elapsed=8198 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=17BE4CDC lastCRC=1BFA4EEA elapsed=33551435 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 5:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165057_064214+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165057_064214+0000.json`
- Raw SHA-256: `945e5278e5d9fef70154ae2f6107317ad7831efbe5d9662ad738daac31cd4990`
- Sidecar SHA-256: `5a2d25bc2af3dc971a2b0f57f3c010d572972c83953ca1052770de99dbb78a19`
- Capture window: 55.023 s; 91822 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551465 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=39 us lastReopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=62 us observedReady=39 us directRead=0 cycles maxDirectRead=25 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=39 us reopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=39 us directRead=0 cycles maxDirectRead=25 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=3EF7E028 lastCRC=3EF7E028 elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=3EF7E028 lastCRC=BFD0144A elapsed=33551442 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 6:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165152_379415+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165152_379415+0000.json`
- Raw SHA-256: `065322e14fc518712234d719127b5aba4f2c09f1dc14a05d545ac089d44da810`
- Sidecar SHA-256: `2650e201641c7bd2165ff1f7a0d12b3a3c5e5db674cc529d1621aa3e0f958186`
- Capture window: 55.017 s; 91806 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551470 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=38 us lastReopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=62 us observedReady=38 us directRead=0 cycles maxDirectRead=35 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=38 us reopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=38 us directRead=0 cycles maxDirectRead=35 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=F17B957B lastCRC=F17B957B elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=F17B957B lastCRC=BE134293 elapsed=33551442 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 7:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165247_660310+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165247_660310+0000.json`
- Raw SHA-256: `127a51f7f658667bfe6cf3162fb71af524ae72a3b14b7f9b0ebf8fa252f59e8d`
- Sidecar SHA-256: `0c0e2b662ad3eab9c6b5c9e5ffedc71ea49af9dd02b78787d62daa118b6ea5f0`
- Capture window: 55.014 s; 91836 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551464 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=39 us lastReopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=62 us observedReady=39 us directRead=0 cycles maxDirectRead=1812 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=39 us reopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=39 us directRead=0 cycles maxDirectRead=1812 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=23D0FB92 lastCRC=23D0FB92 elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=23D0FB92 lastCRC=6C1AE638 elapsed=33551442 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 8:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165342_960465+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165342_960465+0000.json`
- Raw SHA-256: `93bc6d89ea3b32142207028e316493ee9bbf4dc1e774e91f8e65b09a82a056f9`
- Sidecar SHA-256: `c9977aacf8f4365fbe64ef49db2c70ad58b17bd14e5c7c8b97017beb2bb994a9`
- Capture window: 55.016 s; 91838 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551465 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=38 us lastReopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=62 us observedReady=38 us directRead=0 cycles maxDirectRead=1812 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=38 us reopen=0 us maxReopen=70 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=38 us directRead=0 cycles maxDirectRead=1812 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=CD2206DC lastCRC=CD2206DC elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=CD2206DC lastCRC=57FF91FB elapsed=33551443 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 9:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165438_232276+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165438_232276+0000.json`
- Raw SHA-256: `96d7a82be2a2994d83c5ce193ecb912e27c0f3f7d0094d61376d0bed8f19a232`
- Sidecar SHA-256: `1cd4b99d7e7d29d3ce8cc86992cfacedbe0095f2c4733d0e69fbf462f1875dcd`
- Capture window: 55.016 s; 91822 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551465 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=39 us lastReopen=0 us maxReopen=71 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=63 us observedReady=39 us directRead=0 cycles maxDirectRead=23 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=39 us reopen=0 us maxReopen=71 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=39 us directRead=0 cycles maxDirectRead=23 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=23D0FB92 lastCRC=23D0FB92 elapsed=8206 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=23D0FB92 lastCRC=6824AA8E elapsed=33551442 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

Run 10:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165533_543115+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165533_543115+0000.json`
- Raw SHA-256: `5d71dd7a7d1ba553f062f745cc2878d64001e3fc3773796c5f9826fb8c864576`
- Sidecar SHA-256: `58b948a92d3f6bbf0aa6eba78879bd96d6ab9d5206c77d7288c2600f0a71fbb0`
- Capture window: 55.016 s; 77839 bytes.

```text
Startup captured: yes
Complete 4096-frame passing test: no
V4c FINAL frames=2557/4096 processed=2557 rows=81836 DCLK=10475008 RDedges=2557 elapsed=20948409 us maxArmWait=70 us maxCSHigh=51 us
Handover: lastArm=42 us lastReopen=42 us maxReopen=70 us reopened=1 DCLKwhileHigh=0 deadline=100 us
Timing: callbackSeen=1 callback=39 us maxCallback=63 us observedReady=42 us directRead=15 cycles maxDirectRead=27 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
STOP: RD phase mismatch
Saved final: frames=2557/4096 processed=2557 rows=81836 DCLK=10475008 RD=2557
Saved handover: arm=42 us reopen=42 us maxReopen=70 us reopened=1 DCLKwhileHigh=0
Saved timing: callbackSeen=1 callback=39 us observed=42 us directRead=15 cycles maxDirectRead=27 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
RD trace failure: row index=81836, slot=12 (zero-based), expected RD level/edge=0, sampled level=0, notifications=1, sample end=4799 cycles after RCLK ISR timestamp.
RD trace deltas RCLK/DCLK/RD=1/128/1; sampled RD level 0->0.
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=17BE4CDC lastCRC=17BE4CDC elapsed=8205 us
Last CRC report: V4c processed=2459/4096 staticDiff=0 firstCRC=17BE4CDC lastCRC=740A2DDE elapsed=20142328 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
```

### Sequence comparison

Exact argv (all valid logs from this batch):

```json
[
  "/Library/Frameworks/Python.framework/Versions/3.12/bin/python3",
  "scripts/compare_sequences.py",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164715_847642+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164811_151553+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164906_466961+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165001_764480+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165057_064214+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165152_379415+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165247_660310+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165342_960465+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165438_232276+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165533_543115+0000.txt"
]
```

Full comparison output:

```text
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164715_847642+0000.txt: PASS, 4096 refreshes, 1240 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164811_151553+0000.txt: RD phase mismatch, 728 refreshes, 3 content runs
  RD trace failure: row index=23303, slot=7 (zero-based), expected RD level/edge=0, sampled level=0, notifications=1, sample end=4931 cycles after RCLK ISR timestamp.
  RD trace deltas RCLK/DCLK/RD=1/128/1; sampled RD level 0->0.
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_164906_466961+0000.txt: PASS, 4096 refreshes, 474 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165001_764480+0000.txt: PASS, 4096 refreshes, 794 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165057_064214+0000.txt: PASS, 4096 refreshes, 1024 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165152_379415+0000.txt: PASS, 4096 refreshes, 691 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165247_660310+0000.txt: PASS, 4096 refreshes, 633 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165342_960465+0000.txt: PASS, 4096 refreshes, 1362 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165438_232276+0000.txt: PASS, 4096 refreshes, 497 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_165533_543115+0000.txt: RD phase mismatch, 2557 refreshes, 518 content runs
  RD trace failure: row index=81836, slot=12 (zero-based), expected RD level/edge=0, sampled level=0, notifications=1, sample end=4799 cycles after RCLK ISR timestamp.
  RD trace deltas RCLK/DCLK/RD=1/128/1; sampled RD level 0->0.
Before RD phase mismatch in ttgo_serial_20261117_164811_151553+0000.txt
  6C1AE638 x507 -> B2AA7578 x16 -> 5F7CA39A x205
  The last dwell may end early because capture stopped.
  ttgo_serial_20261117_164715_847642+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  same capture: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_164906_466961+0000.txt: 1 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165001_764480+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165057_064214+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165152_379415+0000.txt: 1 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165247_660310+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165342_960465+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165438_232276+0000.txt: 1 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165533_543115+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
Before RD phase mismatch in ttgo_serial_20261117_165533_543115+0000.txt
  5F14E717 x1 -> F637A38E x2 -> 5F14E717 x1
  The last dwell may end early because capture stopped.
  ttgo_serial_20261117_164715_847642+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_164811_151553+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_164906_466961+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165001_764480+0000.txt: 3 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165057_064214+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165152_379415+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165247_660310+0000.txt: 3 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165342_960465+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_165438_232276+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  same capture: 2 match(es) followed by another content change, 0 other terminal match(es).
```

### Interpretation

Measured outcome: ten full 55-second serial captures, eight complete 4096-frame
MOVING COUNTS/RD STREAM PASS results and two RD phase mismatch stops (runs 2 and
10). No row continuity or handover stops. No capture/dump errors. Both stopping
conditions were reached on run 10; no further captures were started.

The last three collapsed CRC content runs differed between the two RD stops:
- Run 2: 6C1AE638 x507 -> B2AA7578 x16 -> 5F7CA39A x205.
- Run 10: 5F14E717 x1 -> F637A38E x2 -> 5F14E717 x1.
There was no matching three-content-run terminal sequence between the RD stops.
Run 2's suffix occurred followed by another content change in passing runs 3, 6,
and 9. In runs 3 and 6 the matching sequence dwells were 608/16/382 refreshes,
followed by B2AA7578; run 2 started partway through its first dwell and stopped
partway through its last dwell. Run 10's suffix occurred followed by another
content change three times in passing run 4, three times in passing run 7, and
twice earlier within run 10 itself. Thus neither three-signature suffix is a
sufficient trigger for an RD stop. This batch provides no evidence that the
same immediate display sequence precedes both RD failures. It does not rule out
a more subtle content/timing association or a common earlier game state.

Both RD failures reported last deltas RCLK/DCLK/RD=1/128/1, with notify=1,
expected RD level/edge=0, and sampled RD level 0->0. Run 2 failed at index 23303,
slot 7, sample end 4931 cycles after the RCLK ISR timestamp; run 10 failed at
index 81836, slot 12, sample end 4799 cycles. These are different acquisition
row positions. The counter observed an RD rising edge without a sampled high
level. This is consistent with an edge/pulse ending before the GPIO sample or
occurring around the counter/level reads; pulse width and electrical source
cannot be determined from these software snapshots. Scope work remains on hold.

maxReopen was 70–73 us across this batch, below the existing 100 us deadline;
RD stops had 73 us and 70 us. workerError=0 and staticDiff=0 in every run, though
staticDiff in MOVING mode does not validate animated pixel correctness. All
passes checked 131072 rows, 16777216 DCLK edges and 4096 RD rising edges. The
added sequence diagnostic may affect scheduling; prior RD-trace-only raw logs
were not recovered, so an empirical before/after timing comparison is unavailable.

Sequence matching here uses CRC32 signatures with adjacent duplicates collapsed,
not named Fish Tales scenes or a verified full-pixel identity across all frames.
Only the last 32 complete pixel refreshes are retained per run. The incomplete
refresh at each failing row has no validated pixel payload. CRC collisions,
grayscale refresh phases, finite windows and partial first/last dwells limit
inference; counts/RD passes are bounded runs, not proof of indefinite capture.

## 2026-11-17 — raw and 80-cycle filtered RD comparison

Worktree: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence`. Branch `experiment/esp32-attract-sequence-recovery`,
base HEAD `415ef81657a8e3d2a3e776f7c29c53620bba6e26` plus the pre-existing uncommitted
RD trace/attract changes and this authorized opt-in diagnostic. The earlier journal
and 17 November attract batch were retained. Before edits, saved all modified and
untracked source/docs and the previous attract firmware in
`/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/rd_filter_preserved_20261117_171117/prechange_files.zip`, plus `prechange_diff.patch` and
`existing_evidence_sha256.json`. All 51 pre-existing evidence files were SHA-256
verified unchanged. No commits, pushes, merges, branch changes or GitHub operations.

User authorized this diagnostic, build, one verified-device flash and up to ten
55-second captures, each with one explicit ESP32 RTS/EN reset. Fish Tales normal
attract mode was reported by the user; it remained running between ESP32 resets.
No game controls, wiring or scope changes. Physical wiring and pulse waveform
were not independently inspected. Retained configuration: level-shifted J603
RD26/RCLK25/DCLK33/SDATA13, common ground and local CS jumper GPIO27 -> GPIO32.
All timestamps are Europe/London, UTC +00:00.

### Implementation and verification

Added `ttgo_tdisplay_rd_filter_compare`, extending the attract-sequence environment;
all existing environments retained. Only this macro-enabled build configures PCNT
unit 3/channel 0 on GPIO26 with rising increments, falling disabled, KEEP/no control
pin, low limit 0 and high limit 30000, matching raw RD unit 2. Source/dependency
inspection found units 0/1/2 only in use; compile-time assertions keep unit 3
independent. The legacy PCNT API has no ownership query: availability is established
by source audit, not a runtime reservation. No counter interrupts were added.

Unit 3 alone enables the 80-cycle filter; startup checks/logs threshold readback,
hardware filter-enable bit, raw unit-2 filter disabled and APB frequency. Saved
startup records confirm cycles=80/readback=80/enabled=1/rawFilterEnabled=0,
APB_Hz=80000000, settleCpuCycles=240 at CPU=240 MHz. Nominal filter interval is
1 us. API/register layout checked against the installed Arduino ESP32 2.0.9
headers and [Espressif PCNT documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/pcnt.html).

Raw RD, GPIO level and the original row/RD STOP conditions remain authoritative.
No filter discrepancy resynchronizes the stream or changes a STOP decision.
`fastReopen` was compared byte-for-byte as source and is unchanged, as are the
100 us deadline and DMA rearm logic. Rebuilding the original attract environment
retained identical executable disassembly and RAM/flash sizes (58484/288061 bytes).
Its complete firmware binary hash changed after rebuild (ELF/debug metadata affects
the embedded image hash), so disassembly equality is the recorded executable check,
not a claim of full binary equality. The original firmware binary/ELF is preserved.

Twelve Python tests passed, including independent counter wrap/offset and bracket
ambiguity tests; `git diff --check` passed. Filter build RAM=64412 bytes,
flash=291037 bytes. Python 3.12.2; PlatformIO Core 6.1.13; espressif32 6.3.2;
Arduino ESP32 2.0.9; Xtensa GCC 8.4.0+2021r2-patch5; esptool 4.5.1. Existing
Core-version notice remains. Telemetry disabled with
`PLATFORMIO_SETTING_ENABLE_TELEMETRY=no`. Python/PIO executables were selected
from `/Library/Frameworks/Python.framework/Versions/3.12/bin` on PATH.

Port freedom checked with lsof on both adapter aliases before upload and each
capture, and after each capture. Upload helper allowlist was extended to the new
environment; it verified CP2104 10C4:EA60 serial 02073BE4. Exactly one flash
completed with hash verification and an uploader RTS reset. The subsequent capture
helper pulses RTS/EN once per run, DTR/GPIO0 deasserted, at 115200 baud. Boot/startup
was confirmed in saved logs; all raw bytes were retained, not receive-buffer purged.

Commands from experiments/esp32-dma-v4c:

```sh
export PLATFORMIO_SETTING_ENABLE_TELEMETRY=no
python3 -m unittest discover -s scripts -p 'test_*.py'
sh scripts/build.sh ttgo_tdisplay_rd_filter_compare
# Compatibility build, not flashed:
sh scripts/build.sh ttgo_tdisplay_attract_sequence
python3 scripts/upload.py --environment ttgo_tdisplay_rd_filter_compare --port /dev/cu.usbserial-02073BE4 --confirm-upload
# Each numbered run:
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --duration 55 --reset
python3 scripts/analyse_log.py RAW_LOG_PATH
python3 scripts/analyse_filter_log.py RAW_LOG_PATH
```

Capture invocation and complete analyser output are saved per run. The batch
validates filtered diagnostics using the same `analyse()` function as the latter
CLI. Existing `load_capture()` also validates one startup/FINAL, MOVING mode,
complete worker counts, matching SEQ BEGIN/END, all frame CRC records and up to
32 pixel payloads with matching CRC. No future run starts after a validation
error or repeated normal-marker disagreement (two expected markers not counted
as one in both counters, including ambiguous cases).

### Read ordering and ambiguity

Each snapshot retains original RCLK/DCLK -> raw RD -> GPIO ordering, with CPU
ccount timestamps added around raw and GPIO reads. There is no filtered driver
read or settling delay before the original GPIO sample; timestamp instructions
have overhead and may shift timing relative to the earlier diagnostic. The legacy
`sampleEndAfterISR` remains the time through that GPIO read, not through the extra
reads. Then read filtered RD, wait only as needed until at least 80 APB cycles
from rawEnd, read filtered RD again, and finally re-read raw RD. Full ordering and
absolute unsigned-32-bit CPU timestamps are logged at startup and in every
supplemental retained row, alignment anchor and disagreement event.

Each delta is calculated within its own counter using V4c modulo-30000 wrap logic;
absolute raw/filtered count equality is not assumed. A comparison is marked
ambiguous if either current or previous snapshot's raw or filtered bracket moves,
or if row/notification continuity is nonunit. Stable brackets rule out observed
counter movement over these read windows, not all unknown waveforms or pulse
widths. Settled filtered confirmation helps distinguish a pending filter response.
Anchor index=4294967295 is an explicit sentinel; its delta fields are placeholders.

Whole-run summary counts all recorded rows and expected markers, including passing
runs. The first 16 disagreements retain previous/current snapshots even when they
leave the 64-row tail; total/saved counts disclose any event truncation. No per-row
diagnostic printing occurs until capture ends. Existing RD/sequence formats remain
intact with supplemental `RD FILTER` lines. See docs/RD_FILTER_COMPARE.md.

### Batch results

Batch directory: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/rd_filter_batch_20261117_171454+0000`. Source/config/firmware hashes, source snapshots, complete
local diff, upload/build logs, manifest, capture output and analyses are retained
there and ignored. The previous 17 November attract batch is separate and unchanged.

| Run | Start UTC | Outcome | Frames | maxReopen us | Both counters correct at expected markers | All-row disagreements | Ambiguous comparisons |
|---:|---|---|---:|---:|---|---:|---:|
| 1 | 2026-11-17T17:16:08.193866+00:00 | PASS | 4096 | 94 | 4096/4096 | 0 | 0 |
| 2 | 2026-11-17T17:17:03.490657+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |
| 3 | 2026-11-17T17:17:58.795721+00:00 | RD phase mismatch | 2771 | 89 | 2771/2771 | 1 | 0 |
| 4 | 2026-11-17T17:18:54.190002+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |
| 5 | 2026-11-17T17:19:49.515351+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |
| 6 | 2026-11-17T17:20:44.808504+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |
| 7 | 2026-11-17T17:21:40.106752+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |
| 8 | 2026-11-17T17:22:35.387954+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |
| 9 | 2026-11-17T17:23:30.692452+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |
| 10 | 2026-11-17T17:24:25.972145+00:00 | PASS | 4096 | 89 | 4096/4096 | 0 | 0 |

Complete passes=9; comparable raw RD stops=1; halt error=None.

Exact provenance SHA-256 values:

```json
{
  "src/main.cpp": "6fdfc36db5055a54f1a2881d0710342bc3eaf966df818a1db219f3c994b68fcd",
  "platformio.ini": "5c3ab2a1aed191448fb711dcac3c294c59617db82f4bb7ed1e7a61e4bb7895fe",
  "scripts/upload.py": "07582758c8684f077b322733c3febf22213c1acd3413acc5304b9fa84339aa9a",
  "scripts/capture.py": "451be39946cd2eb92b3dc67956f1942cd46599cd4e1341f9039f89bc19159045",
  "scripts/analyse_log.py": "a95a2e5c8c5ea3ef32176118121b52eaa10a46afbb610dd5bd28ca4a399daded",
  "scripts/analyse_filter_log.py": "f9ff5550a59e5cf67c058f4654b904e04aeaaaec98f91dbb4a4678f0aa7810c6",
  "scripts/test_filter_log.py": "a2898270bca600607d679e854101f7ba5e0187185583776f20e4caf42f2da2bc",
  "scripts/compare_sequences.py": "0a982ed31e534edf2331028f85ef21db3d50a78686df550c804a335063b5d70a",
  ".pio/build/ttgo_tdisplay_rd_filter_compare/firmware.bin": "bd9139cc13691799ff7102b6dd74ecd9c508ee9e957fd2da6c90bd1a4849060d",
  ".pio/build/ttgo_tdisplay_rd_filter_compare/firmware.elf": "0e4184e0e220e6ea4cfa1f7f1f9d3bf22e75397c1360f107cfac62b64c0577f4"
}
```

### Filter batch run 1

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171608_193828+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171608_193828+0000.json`

Raw SHA-256: `8ebca3474b71e0200b1e9283bd47fee8fe52a707e51c3947adf6b79c719f570e`

Sidecar SHA-256: `f98880434d580221ca036ee3c5bac49760f452c16148d5958a9394c91f688084`

Capture: 55.013 s, 114646 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551473 us maxArmWait=93 us maxCSHigh=75 us
Handover: lastArm=40 us lastReopen=0 us maxReopen=94 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=78 us observedReady=40 us directRead=0 cycles maxDirectRead=17 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=40 us reopen=0 us maxReopen=94 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=40 us directRead=0 cycles maxDirectRead=17 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=F17B957B lastCRC=F17B957B elapsed=8219 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=F17B957B lastCRC=697A3C94 elapsed=33551443 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=189755107 rawEnd=189755176 levelEnd=189755225 filteredStart=189755227 filteredEnd=189755295 settledStart=189755427 settledEnd=189755496 rawAfterEnd=189755565 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 94,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 2

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171703_490625+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171703_490625+0000.json`

Raw SHA-256: `64f9b6ef7d981ff640d171fd38dc6f0f251ffcb11933f85a67f061ea4dcf4bda`

Sidecar SHA-256: `e03b191bf0e91498b541f571d82c8e7287b777906c59e9bb38c8d27ec4e30c3c`

Capture: 55.019 s, 114676 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551472 us maxArmWait=89 us maxCSHigh=69 us
Handover: lastArm=40 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=79 us observedReady=40 us directRead=0 cycles maxDirectRead=29 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=40 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=40 us directRead=0 cycles maxDirectRead=29 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=CD01A2CB lastCRC=CD01A2CB elapsed=8214 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=CD01A2CB lastCRC=5F7CA39A elapsed=33551441 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=189261461 rawEnd=189261530 levelEnd=189261579 filteredStart=189261581 filteredEnd=189261649 settledStart=189261781 settledEnd=189261850 rawAfterEnd=189261919 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 3

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171758_795689+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171758_795689+0000.json`

Raw SHA-256: `a707758c2d9e3be9db42a99f138ec1b846d5cddecd64015a0c3be4cea7f424c6`

Sidecar SHA-256: `34ed738946bf1265f6de8c9b7c0f272ecb83d15fc3e40c94a6a36e6eb3afcf3f`

Capture: 55.016 s, 103313 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: no
V4c FINAL frames=2771/4096 processed=2771 rows=88680 DCLK=11351040 RDedges=2771 elapsed=22700324 us maxArmWait=88 us maxCSHigh=69 us
Handover: lastArm=41 us lastReopen=41 us maxReopen=89 us reopened=1 DCLKwhileHigh=0 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=79 us observedReady=41 us directRead=15 cycles maxDirectRead=28 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
STOP: RD phase mismatch
Saved final: frames=2771/4096 processed=2771 rows=88680 DCLK=11351040 RD=2771
Saved handover: arm=41 us reopen=41 us maxReopen=89 us reopened=1 DCLKwhileHigh=0
Saved timing: callbackSeen=1 callback=35 us observed=41 us directRead=15 cycles maxDirectRead=28 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
RD trace failure: row index=88680, slot=8 (zero-based), expected RD level/edge=0, sampled level=0, notifications=1, sample end=4941 cycles after RCLK ISR timestamp.
RD trace deltas RCLK/DCLK/RD=1/128/1; sampled RD level 0->0.
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=51F7A1EA lastCRC=51F7A1EA elapsed=8214 us
Last CRC report: V4c processed=2704/4096 staticDiff=0 firstCRC=51F7A1EA lastCRC=8A36DBBC elapsed=22149186 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=190803109 rawEnd=190803178 levelEnd=190803227 filteredStart=190803229 filteredEnd=190803297 settledStart=190803429 settledEnd=190803498 rawAfterEnd=190803567 isrCycles=0
RD FILTER SUMMARY rows=88681 markers=2771 rawMarkerOK=2771 filteredMarkerOK=2771 bothMarkerOK=2771 markerDisagree=0 disagree=1 comparableDisagree=1 ambiguous=0 ambiguousMarkers=0 eventsSaved=1 eventsTotal=1
```

Failing row=88680, slot=8; RCLK/DCLK=1/128, raw/filtered RD deltas=1/0, expected=0, GPIO=0, ambiguity=0.

Previous and failing entries (unmodified trace lines):

```text
RD ROW index=88679 slot=7 expected=0 RCLK=29490 DCLK=21776 RD=2795 level=0 notify=1 isrCycles=1343848084 sampleEndAfterISR=4926 failed=0
RD ROW index=88680 slot=8 expected=0 RCLK=29491 DCLK=21904 RD=2796 level=0 notify=1 isrCycles=1343909521 sampleEndAfterISR=4941 failed=1
RD FILTER ROW index=88679 anchor=0 raw=2795 filtered=2795 rawAfter=2795 filteredAfter=2795 rawDelta=0 filteredDelta=0 expected=0 level=0 ambiguous=0 rawStart=1343852892 rawEnd=1343852961 levelEnd=1343853010 filteredStart=1343853012 filteredEnd=1343853080 settledStart=1343853212 settledEnd=1343853281 rawAfterEnd=1343853350 isrCycles=1343848084
RD FILTER ROW index=88680 anchor=0 raw=2796 filtered=2795 rawAfter=2796 filteredAfter=2795 rawDelta=1 filteredDelta=0 expected=0 level=0 ambiguous=0 rawStart=1343914344 rawEnd=1343914413 levelEnd=1343914462 filteredStart=1343914464 filteredEnd=1343914532 settledStart=1343914664 settledEnd=1343914733 rawAfterEnd=1343914802 isrCycles=1343909521
```

Failing-snapshot timing (unsigned CPU cycle offsets from rawStart; CPU=240 MHz):

- rawStart: 0 cycles (0.0000 us).
- rawEnd: 69 cycles (0.2875 us).
- levelEnd: 118 cycles (0.4917 us).
- filteredStart: 120 cycles (0.5000 us).
- filteredEnd: 188 cycles (0.7833 us).
- settledStart: 320 cycles (1.3333 us).
- settledEnd: 389 cycles (1.6208 us).
- rawAfterEnd: 458 cycles (1.9083 us).

The current and previous raw/filtered brackets were stable.

Full comparison validation result:

```json
{
  "outcome": "RD phase mismatch",
  "frames": 2771,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 88681,
    "markers": 2771,
    "rawMarkerOK": 2771,
    "filteredMarkerOK": 2771,
    "bothMarkerOK": 2771,
    "markerDisagree": 0,
    "disagree": 1,
    "comparableDisagree": 1,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 1,
    "eventsTotal": 1
  },
  "comparable_rd_stop": true,
  "repeated_marker_disagreement": false,
  "failure": {
    "previous_raw": "RD ROW index=88679 slot=7 expected=0 RCLK=29490 DCLK=21776 RD=2795 level=0 notify=1 isrCycles=1343848084 sampleEndAfterISR=4926 failed=0",
    "failing_raw": "RD ROW index=88680 slot=8 expected=0 RCLK=29491 DCLK=21904 RD=2796 level=0 notify=1 isrCycles=1343909521 sampleEndAfterISR=4941 failed=1",
    "previous_filter": {
      "index": 88679,
      "anchor": 0,
      "raw": 2795,
      "filtered": 2795,
      "rawAfter": 2795,
      "filteredAfter": 2795,
      "rawDelta": 0,
      "filteredDelta": 0,
      "expected": 0,
      "level": 0,
      "ambiguous": 0,
      "rawStart": 1343852892,
      "rawEnd": 1343852961,
      "levelEnd": 1343853010,
      "filteredStart": 1343853012,
      "filteredEnd": 1343853080,
      "settledStart": 1343853212,
      "settledEnd": 1343853281,
      "rawAfterEnd": 1343853350,
      "isrCycles": 1343848084
    },
    "failing_filter": {
      "index": 88680,
      "anchor": 0,
      "raw": 2796,
      "filtered": 2795,
      "rawAfter": 2796,
      "filteredAfter": 2795,
      "rawDelta": 1,
      "filteredDelta": 0,
      "expected": 0,
      "level": 0,
      "ambiguous": 0,
      "rawStart": 1343914344,
      "rawEnd": 1343914413,
      "levelEnd": 1343914462,
      "filteredStart": 1343914464,
      "filteredEnd": 1343914532,
      "settledStart": 1343914664,
      "settledEnd": 1343914733,
      "rawAfterEnd": 1343914802,
      "isrCycles": 1343909521
    },
    "row": 88680,
    "slot": 8
  }
}
```

### Filter batch run 4

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171854_189966+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171854_189966+0000.json`

Raw SHA-256: `554ea0a70e3ba72377d4d741bbc8007ae8e97dadd6975a3f7769e8a0ec10554d`

Sidecar SHA-256: `8c89c2ed6c76de4deef356c75a044da9f43956f0b725ed0216b73e918bc5a4a2`

Capture: 55.019 s, 114673 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551471 us maxArmWait=88 us maxCSHigh=69 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=36 us maxCallback=79 us observedReady=41 us directRead=0 cycles maxDirectRead=28 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=41 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=36 us observed=41 us directRead=0 cycles maxDirectRead=28 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=5F7CA39A lastCRC=5F7CA39A elapsed=8213 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=5F7CA39A lastCRC=57FF91FB elapsed=33551441 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=189815462 rawEnd=189815531 levelEnd=189815580 filteredStart=189815582 filteredEnd=189815650 settledStart=189815782 settledEnd=189815851 rawAfterEnd=189815920 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 5

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171949_515315+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171949_515315+0000.json`

Raw SHA-256: `b28015ea3c9cf66dc9bdc363b01153008b70a82e3a6ab3ffa4d011cf1f6ee22c`

Sidecar SHA-256: `36260ca781152d6abd0a60d96dec5dbb2d45f8dcc8edb0afff403db92bb5c4f5`

Capture: 55.02 s, 114671 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551465 us maxArmWait=88 us maxCSHigh=69 us
Handover: lastArm=42 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=36 us maxCallback=79 us observedReady=42 us directRead=0 cycles maxDirectRead=1812 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=42 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=36 us observed=42 us directRead=0 cycles maxDirectRead=1812 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=AFA1691D lastCRC=AFA1691D elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=AFA1691D lastCRC=A1A57E39 elapsed=33551434 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=190576476 rawEnd=190576545 levelEnd=190576594 filteredStart=190576596 filteredEnd=190576664 settledStart=190576796 settledEnd=190576865 rawAfterEnd=190576934 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 6

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172044_808463+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172044_808463+0000.json`

Raw SHA-256: `ab1de26bb09f35764ed4da6b3e94b86c77deb2a7b1ea41765687b742b325c807`

Sidecar SHA-256: `0e870c95c70e778aaf6e807150b876a7b31cc2b37d61765b3b443aee5b97c303`

Capture: 55.014 s, 114639 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551470 us maxArmWait=88 us maxCSHigh=70 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=78 us observedReady=41 us directRead=0 cycles maxDirectRead=28 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=41 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=41 us directRead=0 cycles maxDirectRead=28 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=57FF91FB lastCRC=57FF91FB elapsed=8213 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=57FF91FB lastCRC=EFE70DB4 elapsed=33551440 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=189427051 rawEnd=189427120 levelEnd=189427169 filteredStart=189427171 filteredEnd=189427239 settledStart=189427371 settledEnd=189427440 rawAfterEnd=189427509 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 7

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172140_106721+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172140_106721+0000.json`

Raw SHA-256: `cb8e7e5a3b43edb51564fe7df1c930183e1be4252745aac3823b5d3a5e49ba6c`

Sidecar SHA-256: `15b3a79f39ad44f3d728656f3a5cd0c02a7cf46239089605cd83a0b161117f5d`

Capture: 55.02 s, 114676 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551464 us maxArmWait=88 us maxCSHigh=69 us
Handover: lastArm=40 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=79 us observedReady=40 us directRead=0 cycles maxDirectRead=24 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=40 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=40 us directRead=0 cycles maxDirectRead=24 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=67D75B8E lastCRC=67D75B8E elapsed=8207 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=67D75B8E lastCRC=DB030581 elapsed=33551434 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=190286758 rawEnd=190286827 levelEnd=190286876 filteredStart=190286878 filteredEnd=190286946 settledStart=190287078 settledEnd=190287147 rawAfterEnd=190287216 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 8

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172235_387919+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172235_387919+0000.json`

Raw SHA-256: `341b3d0faa1426d63a0205c8b999ef8739fdf4916753148884835b38243f54cf`

Sidecar SHA-256: `8464eab2573584db79959635206230af50ef978c30656abcf5379b8254ea8eca`

Capture: 55.021 s, 114675 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551471 us maxArmWait=89 us maxCSHigh=70 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=79 us observedReady=41 us directRead=0 cycles maxDirectRead=24 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=41 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=41 us directRead=0 cycles maxDirectRead=24 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=52321B34 lastCRC=52321B34 elapsed=8214 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=52321B34 lastCRC=5F7CA39A elapsed=33551440 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=189908286 rawEnd=189908355 levelEnd=189908404 filteredStart=189908406 filteredEnd=189908474 settledStart=189908606 settledEnd=189908675 rawAfterEnd=189908744 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 9

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172330_692420+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172330_692420+0000.json`

Raw SHA-256: `081d4f41c1bceb148a5a5f649613485f111b9dc58c3cea49d09c29a29c1ed4f0`

Sidecar SHA-256: `fefca1e6b0fbc9fe8994bf17dafa226972f5aa3ddccfb0307732ba2fbc48ec4d`

Capture: 55.018 s, 114652 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551467 us maxArmWait=88 us maxCSHigh=69 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=36 us maxCallback=79 us observedReady=41 us directRead=0 cycles maxDirectRead=32 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=41 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=36 us observed=41 us directRead=0 cycles maxDirectRead=32 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=2D1A264A lastCRC=2D1A264A elapsed=8210 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=2D1A264A lastCRC=F2011251 elapsed=33551437 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=189088870 rawEnd=189088939 levelEnd=189088988 filteredStart=189088990 filteredEnd=189089058 settledStart=189089190 settledEnd=189089259 rawAfterEnd=189089328 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Filter batch run 10

Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172425_972111+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172425_972111+0000.json`

Raw SHA-256: `08b6742fd53e776b015a1d9b3565f3ad696f414bb3ce5d26378df303c86de51b`

Sidecar SHA-256: `c1a61696f4cad5d6ab610c06d848bbe7efa70cf734161ddd4150c42296525930`

Capture: 55.021 s, 114650 bytes; reset_requested=true, startup_captured=true, port_closed=true.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551472 us maxArmWait=88 us maxCSHigh=69 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=36 us maxCallback=79 us observedReady=41 us directRead=0 cycles maxDirectRead=28 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=41 us reopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=36 us observed=41 us directRead=0 cycles maxDirectRead=28 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=5F7CA39A lastCRC=5F7CA39A elapsed=8214 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=5F7CA39A lastCRC=17BE4CDC elapsed=33551442 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.

RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=30000 cycles=80 readback=80 enabled=1 rawFilterEnabled=0 APB_Hz=80000000 settleCpuCycles=240
RD FILTER ANCHOR index=4294967295 anchor=1 raw=24 filtered=24 rawAfter=24 filteredAfter=24 rawDelta=0 filteredDelta=0 expected=0 level=1 ambiguous=0 rawStart=189143224 rawEnd=189143293 levelEnd=189143342 filteredStart=189143344 filteredEnd=189143412 settledStart=189143544 settledEnd=189143613 rawAfterEnd=189143682 isrCycles=0
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full comparison validation result:

```json
{
  "outcome": "PASS",
  "frames": 4096,
  "maxReopen": 89,
  "config": {
    "rawUnit": 2,
    "filteredUnit": 3,
    "pin": 26,
    "limit": 30000,
    "cycles": 80,
    "readback": 80,
    "enabled": 1,
    "rawFilterEnabled": 0,
    "APB_Hz": 80000000,
    "settleCpuCycles": 240
  },
  "summary": {
    "rows": 131072,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "comparable_rd_stop": false,
  "repeated_marker_disagreement": false
}
```

### Sequence comparison (secondary evidence)

Exact argv:

```json
[
  "/Library/Frameworks/Python.framework/Versions/3.12/bin/python3",
  "scripts/compare_sequences.py",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171608_193828+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171703_490625+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171758_795689+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171854_189966+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171949_515315+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172044_808463+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172140_106721+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172235_387919+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172330_692420+0000.txt",
  "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172425_972111+0000.txt"
]
```

```text
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171608_193828+0000.txt: PASS, 4096 refreshes, 824 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171703_490625+0000.txt: PASS, 4096 refreshes, 447 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171758_795689+0000.txt: RD phase mismatch, 2771 refreshes, 617 content runs
  RD trace failure: row index=88680, slot=8 (zero-based), expected RD level/edge=0, sampled level=0, notifications=1, sample end=4941 cycles after RCLK ISR timestamp.
  RD trace deltas RCLK/DCLK/RD=1/128/1; sampled RD level 0->0.
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171854_189966+0000.txt: PASS, 4096 refreshes, 1129 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_171949_515315+0000.txt: PASS, 4096 refreshes, 404 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172044_808463+0000.txt: PASS, 4096 refreshes, 772 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172140_106721+0000.txt: PASS, 4096 refreshes, 1592 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172235_387919+0000.txt: PASS, 4096 refreshes, 440 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172330_692420+0000.txt: PASS, 4096 refreshes, 972 content runs
/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_172425_972111+0000.txt: PASS, 4096 refreshes, 1133 content runs
Before RD phase mismatch in ttgo_serial_20261117_171758_795689+0000.txt
  CD01A2CB x1 -> 52321B34 x12 -> 8A36DBBC x153
  The last dwell may end early because capture stopped.
  ttgo_serial_20261117_171608_193828+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_171703_490625+0000.txt: 1 match(es) followed by another content change, 0 other terminal match(es).
  same capture: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_171854_189966+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_171949_515315+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_172044_808463+0000.txt: 1 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_172140_106721+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_172235_387919+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_172330_692420+0000.txt: 1 match(es) followed by another content change, 0 other terminal match(es).
  ttgo_serial_20261117_172425_972111+0000.txt: 0 match(es) followed by another content change, 0 other terminal match(es).
```

### Interpretation and limits

The ten-capture limit was reached: nine full 4096-frame MOVING COUNTS/RD
passes, one comparable raw RD phase stop (run 3), no row continuity or handover
stops. The requested two-stop sample was not obtained; no captures beyond ten
were started. Every capture ran 55 seconds with a single explicit reset,
confirmed startup/readback/FINAL/SEQ END, complete pixel/CRC and filter dumps,
and port closure. No dump failures or repeated normal-marker disagreements.

Across 1268329 sampled rows, both counters counted exactly one edge at all
39635 expected marker positions. Passing runs had no raw/filtered
disagreements anywhere, not just in the retained tail. Exactly one all-row
disagreement was recorded, on run 3's failing row. No ambiguous comparisons
were flagged anywhere; no disagreement events were truncated.

Run 3 stopped after 2771 complete frames at zero-based index 88680, slot 8.
RCLK/DCLK deltas=1/128; raw RD delta=1, filtered RD delta=0, expected edge count=0;
sampled GPIO remained 0->0, notify=1. Previous raw/filtered counts=2795/2795,
failing counts=2796/2795, and both confirmation reads retained those respective
counts. Counter offsets are immaterial: deltas were computed independently.
The GPIO sample ended 4941 CPU cycles (20.5875 us) after the RCLK ISR timestamp.
The raw-to-first-filtered interval was bracketed by timestamps and stable raw
confirmation; the second filtered read ended 320 CPU cycles (1.3333 us) after
the initial raw read ended, still showing no increment. The previous snapshot
was stable too, so this event meets the diagnostic's comparable-read criterion.
The complete previous/failing entries and all timestamps are retained below.

This is evidence that the extra transition counted by raw unit 2 was not counted
by the 80-APB-cycle filtered unit 3, while normal RD markers were preserved.
It supports a short-pulse/transient hypothesis at the ESP32 RD input rather
than treating the extra raw edge as an established new frame marker. An 80-cycle
threshold at the observed 80 MHz APB is nominally 1 us. This is not a direct
pulse-width or voltage measurement, and one event does not establish repeatability,
electrical origin, whether the machine intentionally emits it, or the correct
production framing/filtering policy. Scope work remains on hold. No acquisition
rule, raw counter filter, STOP condition or resynchronization policy was changed.

New maxReopen values=94 us (run 1), 89 us (runs 2–10), versus 70–73 us in the
previous attract batch. The new range is 16–24 us above that earlier range's
endpoints, with a 6 us smallest observed margin to the unchanged 100 us limit.
The run-3 RD stop had maxReopen=89 us and last reopened=1 with zero DCLK edges
while CS high. No handover deadline failure occurred. This is measurable timing
perturbation associated with the added diagnostic reads/timestamps/copies and
code layout, despite unchanged fastReopen source. It should not be treated as
a timing-neutral production modification or a validated firmware fix.

Secondary CRC sequence comparison: run 3's final three content runs were
CD01A2CB x1 -> 52321B34 x12 -> 8A36DBBC x153. Passing runs 2, 6 and 9 each
continued through a matching three-signature sequence. There is only one RD
stop in this batch, so there is no multiple-stop suffix comparison here. CRC
matches are content signatures, not proof of all-frame pixel identity or named
Fish Tales scenes; the incomplete failing refresh has no validated pixel payload.

## 2026-11-17 — filtered validation and explicit recovery tests

User authorized opt-in filtered RD validation without recovery (A), then injected
validation-fault recovery at two row positions (B), with verified-device flashing
and bounded captures. Fish Tales remained in normal attract mode as reported by
the user; no game controls, wiring or oscilloscope were operated. Physical wiring
and electrical waveforms were not independently inspected. Retained level-shifted
J603/common-ground setup and pins: RD26/RCLK25/DCLK33/SDATA13, CS27 -> CS32.

Worktree `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence`, branch `experiment/esp32-attract-sequence-recovery`,
HEAD `415ef81657a8e3d2a3e776f7c29c53620bba6e26` plus existing uncommitted work.
No commits, branch changes, pushes, merges, GitHub changes or stale-worktree pruning.
Before modifications, preserved all modified/untracked files and previous comparison
firmware in `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/filtered_recovery_preserved_20261117_180142/prechange.zip`; saved the full diff and
`evidence_hashes.json`. Verified all 129 prior local evidence files unchanged
and the entire prior engineering-journal byte prefix intact. Existing environments
and their historical raw logs remain separate. Full source/config/binary snapshots
and SHA-256 values are retained in each new test directory's provenance.json.
All timestamps below are Europe/London (UTC +00:00).

### Implementation and validation

A: `ttgo_tdisplay_filtered_validate` uses the existing unit-3 80-APB-cycle filtered
counter for initial alignment and RD edge validation. Raw unit 2 and GPIO level
are observers. A filtered delta must be zero on slots 0–30 and one on slot 31,
with a stable filtered confirmation read. RCLK/DCLK/notifications, SPI ownership,
bit length, worker sequence, final continuity totals and the original 100 us
handover checks remain enforced; any validated mismatch stops A. Raw-only events
are recorded with row/slot/frame and ambiguity. Existing 16-event detail retention
is explicitly bounded; no event details were truncated in these tests.

B: `ttgo_tdisplay_filtered_recovery` injects once at slot 8;
`ttgo_tdisplay_filtered_recovery_slot20` is the same diagnostic compiled for slot 20.
This required three verified uploads in total: one A image, and two configurations
of diagnostic B for the requested two positions. Each fault occurs after 256
validated output refreshes, with the worker caught up, before accepting/publishing
the suspect refresh. The injected condition and genuine RCLK/DCLK/notification or
filtered RD failures enter the same fault/recovery block. Every event is labelled
injected or genuine. No genuine validation failure occurred in B's recorded runs.

Recovery closes CS, returns exactly the current partial descriptor through the
SPI result API (bounded 2 ms), discards its RX bytes, then resets/requeues only
that returned descriptor. It requires a new setup generation and a different
known armed descriptor, then seeks a new filtered RD marker with CS held high.
It never clears running counters and never frees a live DMA buffer or tears down
the SPI host. New sync/open checks still enforce the 100 us quiet-window deadline.
Every subsequent published refresh must pass 32 row checks, 4096-bit length and
ownership validation. Unknown ownership/alignment/deadline state stops safely.
The unchanged fastReopen source was compared directly with the pre-test snapshot.

The matching upstream ESP-IDF v4.4.4 driver was inspected:
https://github.com/espressif/esp-idf/blob/v4.4.4/components/driver/spi_slave.c .
Its ISR returns completed/partial descriptors and prepares the next descriptor,
including its own DMA workaround before post_setup_cb. The inspected source is
saved in each batch directory. Recovery therefore reclaims results and verifies
the next generation rather than resetting/freeing an active host.

The worker rejects suspect capture IDs and any buffer without 32 validated rows.
A complete PUB IDS dump maps every output sequence to its capture ID. Host audits
verify strict order, no suspect publication, no unexplained gaps, complete CRC/
pixel dumps, exactly one injected fault, valid timing arithmetic and output
continuing after recovery with only one startup and no subsequent boot/reset.
A recovery result is explicitly RECOVERY VALIDATED OUTPUT PASS, not an uninterrupted
4096-refresh stream pass. B's FINAL totals are validated output totals; partial
rows and dropped captures remain explicitly disclosed in separate records.

All 16 Python tests passed (including 12 pre-existing tests). New tests reject
suspect IDs, unreclaimed ownership, unexplained gaps and false timing arithmetic;
valid timing cases above 10 ms are accepted without clipping. All three diagnostic
build configurations passed with PLATFORMIO_SETTING_ENABLE_TELEMETRY=no. Final
combined source also rebuilt A successfully; no additional A flash was performed.
The A hardware runs used the exact source snapshot captured before adding B's
conditional loop/worker code. The final shared source retains the same A validation
logic with updated startup wording; per-upload snapshots distinguish revisions.
`git diff --check` passed.

Build sizes: A (as flashed) RAM=64412, flash=291329 bytes. B slot8/slot20 RAM=98244,
flash=293105 bytes. Toolchain: Python 3.12.2, PlatformIO Core 6.1.13,
espressif32 6.3.2, Arduino ESP32 2.0.9 / IDF 4.4.4, Xtensa GCC
8.4.0+2021r2-patch5, esptool 4.5.1. Existing older-Core notice remains informational.
PATH selected /Library/Frameworks/Python.framework/Versions/3.12/bin.

Each upload used scripts/upload.py --environment ENV --port
/dev/cu.usbserial-02073BE4 --confirm-upload. The allowlist includes these opt-in
configurations. Each upload checked CP2104 10C4:EA60 serial 02073BE4, verified flash
hashes and performed its normal uploader reset. lsof found neither serial alias
occupied before uploads/captures or afterward. Every acquisition command was:

```sh
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --duration 55 --reset
```

Every capture ran the entire 55 seconds, preserved raw bytes and JSON sidecar,
confirmed startup/FINAL/SEQ END and port closure. One explicit helper RTS/EN reset
was requested per acquisition, DTR/GPIO0 deasserted; no ESP32 reset occurs inside
recovery. No bytes were receive-buffer purged. All raw evidence remains local and
ignored. Existing analyse_log.py summaries and new filtered/recovery audits are
saved alongside the manifests. Default sequence-analysis semantics are unchanged;
recovered results require explicit allow_recovery=True and the dedicated audit.

### A — filtering without recovery

| Run | Frames | Result | maxReopen us | Raw-only edges | Filtered markers correct |
|---:|---:|---|---:|---|---|
| 1 | 4096 | PASS | 81 | none | 4096/4096 |
| 2 | 4096 | PASS | 81 | none | 4096/4096 |
| 3 | 4096 | PASS | 81 | row 1416, slot 8, frame 45, ambiguity 0 | 4096/4096 |

A stopped after run 3, meeting the requested early stopping criterion. A natural
raw-only edge at row 1416 (zero-based), slot 8, candidate frame 45 (one-based)
was observed with raw/filtered deltas=1/0 and ambiguity=0. That same capture
completed all 4096 validated refreshes without recovery or another reset.
It checked 131072 rows, 16777216 DCLK edges and 4096 filtered markers;
workerError=0, staticDiff=0. A single event supports this bounded case, not
indefinite robustness or arbitrary short-pulse rejection.

A run 1:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180314_587536+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180314_587536+0000.json`
- Raw SHA-256: `dc49f6573c3499991cf809e68b16f890aa44a2440e1aae49f5b6064cfac788c5`
- Sidecar SHA-256: `3df96fae1459c8df404bd344ccffaebd59cffd53618f894f83b7f0c9955c70db`
- Window: 55.017 s; 114782 bytes; startup/reset/closure confirmed.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551477 us maxArmWait=81 us maxCSHigh=61 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=81 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=71 us observedReady=41 us directRead=0 cycles maxDirectRead=1813 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=41 us reopen=0 us maxReopen=81 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=41 us directRead=0 cycles maxDirectRead=1813 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=FD33234F lastCRC=FD33234F elapsed=8211 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=FD33234F lastCRC=4507F25E elapsed=33551451 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

A run 2:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180409_908988+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180409_908988+0000.json`
- Raw SHA-256: `ee4779dcde58a83fdf7ad86c6c09f4ab8764d9acd3662bad8174f6caf1db32bf`
- Sidecar SHA-256: `6ebc585354e35b827b6a758f7aafde7c4a04e83880465d426dc8ff815a44838b`
- Window: 55.014 s; 114791 bytes; startup/reset/closure confirmed.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551476 us maxArmWait=80 us maxCSHigh=61 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=81 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=71 us observedReady=41 us directRead=0 cycles maxDirectRead=1807 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=41 us reopen=0 us maxReopen=81 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=41 us directRead=0 cycles maxDirectRead=1807 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=F17B957B lastCRC=F17B957B elapsed=8210 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=F17B957B lastCRC=00338E39 elapsed=33551450 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

A run 3:

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180505_203189+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180505_203189+0000.json`
- Raw SHA-256: `d13487d8bf625a0ea57078ce8fd16db856e8a95f76a15c0cce8edadae14a45b3`
- Sidecar SHA-256: `7fc4ca08a1b4d01ecbf60d05659b94965e5b939a575b7ea54bbf6d156878fc35`
- Window: 55.013 s; 115509 bytes; startup/reset/closure confirmed.

```text
Startup captured: yes
Complete 4096-frame passing test: yes
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551469 us maxArmWait=80 us maxCSHigh=61 us
Handover: lastArm=40 us lastReopen=0 us maxReopen=81 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=71 us observedReady=40 us directRead=0 cycles maxDirectRead=17 cycles
Last deltas: RCLK=1 DCLK=128 RD=1 | staticDiff=0 workerError=0
RESULT: MOVING COUNTS/RD STREAM PASS - all 4096 buffer transactions checked
Saved final: frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RD=4096
Saved handover: arm=40 us reopen=0 us maxReopen=81 us reopened=0 DCLKwhileHigh=-1
Saved timing: callbackSeen=1 callback=35 us observed=40 us directRead=0 cycles maxDirectRead=17 cycles
Saved deltas: RCLK=1 DCLK=128 RD=1 staticDiff=0 workerError=0
First CRC report: V4c processed=1/4096 staticDiff=0 firstCRC=52321B34 lastCRC=52321B34 elapsed=8205 us
Last CRC report: V4c processed=4096/4096 staticDiff=0 firstCRC=52321B34 lastCRC=5F7CA39A elapsed=33551443 us
Last CRC report may precede the final frame; MOVING CRC changes are expected.
RD FILTER SUMMARY rows=131072 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=1 comparableDisagree=1 ambiguous=0 ambiguousMarkers=0 eventsSaved=1 eventsTotal=1
RD FILTER EVENT_PREV index=1415 anchor=0 raw=68 filtered=68 rawAfter=68 filteredAfter=68 rawDelta=0 filteredDelta=0 expected=0 level=0 ambiguous=0 rawStart=280171289 rawEnd=280171358 levelEnd=280171407 filteredStart=280171409 filteredEnd=280171477 settledStart=280171609 settledEnd=280171678 rawAfterEnd=280171747 isrCycles=280166619
RD FILTER EVENT index=1416 anchor=0 raw=69 filtered=68 rawAfter=69 filteredAfter=68 rawDelta=1 filteredDelta=0 expected=0 level=0 ambiguous=0 rawStart=280232708 rawEnd=280232777 levelEnd=280232826 filteredStart=280232828 filteredEnd=280232896 settledStart=280233028 settledEnd=280233097 rawAfterEnd=280233166 isrCycles=280228053
RD RAW_ONLY row=1416 slot=8 frame=45 rawDelta=1 filteredDelta=0 ambiguous=0
```

### B — controlled injected-failure recovery

All times below are measured, not capped at 10 ms. Capture endpoints are the
software timestamp immediately after CS closes at the end of a complete good
refresh. Validated timestamps are after descriptor/length checks. Worker
publication timestamps are immediately before releasing CRC/pixel history and
processed count; timestamp/fence overhead is not a physical output-pin measurement.
Gaps compare the last good refresh before the fault with the first good one after it.

| Fault slot | Fault to good capture end ms | Fault to validated ms | Capture gap ms | Fault to publication ms | Publication gap ms | maxReopen us |
|---:|---:|---:|---:|---:|---:|---:|
| 8 | 14.108 | 14.117 | 16.406 | 14.266 | 16.411 | 76 |
| 20 | 11.042 | 11.052 | 16.411 | 11.195 | 16.406 | 76 |

Both injected failures occurred in capture 257 after 256 good refreshes. Capture
257 is absent from every published output ID. The next complete good capture,
258, is output sequence 257. Output continues through capture 4097/output sequence
4096, yielding 3840 complete validated outputs after the discard. There are no
other missing, duplicated or reordered capture IDs. Neither run reset/rebooted
after its initial capture reset. All 4096 expected filtered markers validated.
No actual signal-loss/filtered-RD mismatch recovery was observed: the injected
failure tests the same code path and partial-DMA termination, not every possible
electrical or driver fault.

B8: raw `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180940_669658+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180940_669658+0000.json`

Raw SHA-256 `9329fa22835e5f206de250de305531ec63fbb6200698319bfa14fd66aa163913`

Sidecar SHA-256 `41f2915a35cc9f4fdddd059862d0aeb3a240f50cd8de39f2b313ba2e4a284fc9`

Capture window 55.013 s, 136449 bytes.

The driver reclaimed the suspect descriptor in 63 us; dropped length=1152 bits. New generation=258, max safe-alignment opening=53 us.

```text
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33559653 us maxArmWait=75 us maxCSHigh=57 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=76 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=35 us maxCallback=66 us observedReady=41 us directRead=0 cycles maxDirectRead=1805 cycles
RECOVERY TOTALS validatedRefreshes=4096 discarded=1 maxAlignmentOpen=53 deadline=100 countersNeverCleared=1
RECOVERY EVENT id=0 injected=1 cause=INJECTED validation failure candidate=257 frame=257 slot=8 rawDelta=0 filteredDelta=0 RCLKdelta=1 DCLKdelta=128 level=0 reclaimed=1 droppedBits=1152 aligned=1 generation=258
RECOVERY TIMING id=0 detectedUs=2904102 reclaimedUs=2904165 syncUs=2910000 lastGoodEndUs=2901804 firstGoodSequence=257 firstGoodCandidate=258 firstGoodEndUs=2918210 firstGoodValidatedUs=2918219 firstGoodPublishedUs=2918368 lastGoodPublishedUs=2901957 detectionToGoodUs=14108 goodGapUs=16406 detectionToPublishUs=14266 publicationGapUs=16411
RECOVERY ANCHOR id=0 RCLK=8994 DCLK=8288 rawRD=280 filteredRD=280 level=1
RESULT: RECOVERY VALIDATED OUTPUT PASS - discontinuities explicitly discarded and logged
RD FILTER SUMMARY rows=131081 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full host audit:

```json
{
  "log": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180940_669658+0000.txt",
  "sidecar": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_180940_669658+0000.json",
  "metadata": {
    "port": "/dev/cu.usbserial-02073BE4",
    "baud": 115200,
    "duration_seconds": 55.0,
    "reset_requested": true,
    "started": "2026-11-17T18:09:40.669692+00:00",
    "elapsed_seconds": 55.013,
    "bytes": 136449,
    "port_closed": true,
    "startup_captured": true,
    "boot_banner_captured": true
  },
  "sha256": "9329fa22835e5f206de250de305531ec63fbb6200698319bfa14fd66aa163913",
  "sidecar_sha256": "41f2915a35cc9f4fdddd059862d0aeb3a240f50cd8de39f2b313ba2e4a284fc9",
  "outcome": "RECOVERED_PASS",
  "frames": 4096,
  "events": [
    {
      "id": 0,
      "injected": 1,
      "candidate": 257,
      "frame": 257,
      "slot": 8,
      "rawDelta": 0,
      "filteredDelta": 0,
      "RCLKdelta": 1,
      "DCLKdelta": 128,
      "level": 0,
      "reclaimed": 1,
      "droppedBits": 1152,
      "aligned": 1,
      "generation": 258
    }
  ],
  "timings": [
    {
      "id": 0,
      "detectedUs": 2904102,
      "reclaimedUs": 2904165,
      "syncUs": 2910000,
      "lastGoodEndUs": 2901804,
      "firstGoodSequence": 257,
      "firstGoodCandidate": 258,
      "firstGoodEndUs": 2918210,
      "firstGoodValidatedUs": 2918219,
      "firstGoodPublishedUs": 2918368,
      "lastGoodPublishedUs": 2901957,
      "detectionToGoodUs": 14108,
      "goodGapUs": 16406,
      "detectionToPublishUs": 14266,
      "publicationGapUs": 16411
    }
  ],
  "maxReopen": 76,
  "maxAlignmentOpen": 53,
  "summary": {
    "rows": 131081,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "suspect_published": false,
  "continued_without_reset": true,
  "publication_ids_count": 4096,
  "last_candidate": 4097
}
```

B20: raw `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_181232_866418+0000.txt`

Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_181232_866418+0000.json`

Raw SHA-256 `1a66fad78aa4b7604406a81b2cc3baa15c239c42a1f4b275c8d3e1b1b04fce00`

Sidecar SHA-256 `403ae978e95ecbdc2247b49329b80e294d73d42c8e394845fc8fe6d0dad4eedf`

Capture window 55.015 s, 136437 bytes.

The driver reclaimed the suspect descriptor in 67 us; dropped length=2688 bits. New generation=258, max safe-alignment opening=53 us.

```text
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33559653 us maxArmWait=75 us maxCSHigh=57 us
Handover: lastArm=42 us lastReopen=0 us maxReopen=76 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
Timing: callbackSeen=1 callback=36 us maxCallback=66 us observedReady=42 us directRead=0 cycles maxDirectRead=22 cycles
RECOVERY TOTALS validatedRefreshes=4096 discarded=1 maxAlignmentOpen=53 deadline=100 countersNeverCleared=1
RECOVERY EVENT id=0 injected=1 cause=INJECTED validation failure candidate=257 frame=257 slot=20 rawDelta=0 filteredDelta=0 RCLKdelta=1 DCLKdelta=128 level=0 reclaimed=1 droppedBits=2688 aligned=1 generation=258
RECOVERY TIMING id=0 detectedUs=2903659 reclaimedUs=2903726 syncUs=2906486 lastGoodEndUs=2898290 firstGoodSequence=257 firstGoodCandidate=258 firstGoodEndUs=2914701 firstGoodValidatedUs=2914711 firstGoodPublishedUs=2914854 lastGoodPublishedUs=2898448 detectionToGoodUs=11042 goodGapUs=16411 detectionToPublishUs=11195 publicationGapUs=16406
RECOVERY ANCHOR id=0 RCLK=8980 DCLK=6496 rawRD=280 filteredRD=280 level=1
RESULT: RECOVERY VALIDATED OUTPUT PASS - discontinuities explicitly discarded and logged
RD FILTER SUMMARY rows=131093 markers=4096 rawMarkerOK=4096 filteredMarkerOK=4096 bothMarkerOK=4096 markerDisagree=0 disagree=0 comparableDisagree=0 ambiguous=0 ambiguousMarkers=0 eventsSaved=0 eventsTotal=0
```

Full host audit:

```json
{
  "log": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_181232_866418+0000.txt",
  "sidecar": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_181232_866418+0000.json",
  "metadata": {
    "port": "/dev/cu.usbserial-02073BE4",
    "baud": 115200,
    "duration_seconds": 55.0,
    "reset_requested": true,
    "started": "2026-11-17T18:12:32.866452+00:00",
    "elapsed_seconds": 55.015,
    "bytes": 136437,
    "port_closed": true,
    "startup_captured": true,
    "boot_banner_captured": true
  },
  "sha256": "1a66fad78aa4b7604406a81b2cc3baa15c239c42a1f4b275c8d3e1b1b04fce00",
  "sidecar_sha256": "403ae978e95ecbdc2247b49329b80e294d73d42c8e394845fc8fe6d0dad4eedf",
  "outcome": "RECOVERED_PASS",
  "frames": 4096,
  "events": [
    {
      "id": 0,
      "injected": 1,
      "candidate": 257,
      "frame": 257,
      "slot": 20,
      "rawDelta": 0,
      "filteredDelta": 0,
      "RCLKdelta": 1,
      "DCLKdelta": 128,
      "level": 0,
      "reclaimed": 1,
      "droppedBits": 2688,
      "aligned": 1,
      "generation": 258
    }
  ],
  "timings": [
    {
      "id": 0,
      "detectedUs": 2903659,
      "reclaimedUs": 2903726,
      "syncUs": 2906486,
      "lastGoodEndUs": 2898290,
      "firstGoodSequence": 257,
      "firstGoodCandidate": 258,
      "firstGoodEndUs": 2914701,
      "firstGoodValidatedUs": 2914711,
      "firstGoodPublishedUs": 2914854,
      "lastGoodPublishedUs": 2898448,
      "detectionToGoodUs": 11042,
      "goodGapUs": 16411,
      "detectionToPublishUs": 11195,
      "publicationGapUs": 16406
    }
  ],
  "maxReopen": 76,
  "maxAlignmentOpen": 53,
  "summary": {
    "rows": 131093,
    "markers": 4096,
    "rawMarkerOK": 4096,
    "filteredMarkerOK": 4096,
    "bothMarkerOK": 4096,
    "markerDisagree": 0,
    "disagree": 0,
    "comparableDisagree": 0,
    "ambiguous": 0,
    "ambiguousMarkers": 0,
    "eventsSaved": 0,
    "eventsTotal": 0
  },
  "suspect_published": false,
  "continued_without_reset": true,
  "publication_ids_count": 4096,
  "last_candidate": 4097
}
```

### Handover and interpretation

Worst observed normal handover was 81 us (A runs 1–3), versus the unchanged
100 us deadline: 19 us observed margin. Both B runs had maxReopen=76 us and
max safe alignment opening=53 us. These are software timings, not scope-derived
latencies. No deadline or continuity check was relaxed. Recovery deliberately
pauses acquisition and drops the suspect refresh; its >10 ms output gaps are
reported separately and are not disguised as 100 us handovers. The recovered
output is not an uninterrupted-stream baseline.

Filtering handled the observed raw-only edge without interruption in A. Recovery
safely excluded one injected suspect refresh at each tested position and resumed
complete validated publication without an ESP32 reset. Publication latencies were
14.266 ms and 11.195 ms, and publication gaps 16.411 ms and 16.406 ms: all exceed
10 ms. No faster-recovery claim is supported. Scope remains on hold; electrical
pulse width/source, animated pixel correctness and indefinite uptime are untested.

### Exact local evidence directories and source/configuration hashes

A: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/filtered_test_A_20261117_180231+0000`

Per-upload SHA-256 provenance:

```json
{
  "src/main.cpp": "34cfab12fecf38264b707525789c4549f924fbe9915e772c285031b747ae1a9d",
  "platformio.ini": "35b4f997966e7a6b6ebd46d165b6fe5667bf723bc2020424f0875582db4a20e6",
  "scripts/upload.py": "cc3ea020e015a1015877e7f4393e637448f730798c31f6ec10a7a092960b47ba",
  ".pio/build/ttgo_tdisplay_filtered_validate/firmware.bin": "47ed57c3e16f920aa04eae5b300d820b975a44360187b98cf453067ad9d2038b",
  ".pio/build/ttgo_tdisplay_filtered_validate/firmware.elf": "d80a9a0dc1a75e909885aed0835d6e9216c4995dc51937c4e8f6cf0c2ab480dd"
}
```
B8: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/filtered_test_B8_20261117_180855+0000`

Per-upload SHA-256 provenance:

```json
{
  "src/main.cpp": "a26cdef5791d7d4c3088c3da90197fa62a9c596a505be49a08cfa1a3e7e04a44",
  "platformio.ini": "35b4f997966e7a6b6ebd46d165b6fe5667bf723bc2020424f0875582db4a20e6",
  "scripts/upload.py": "cc3ea020e015a1015877e7f4393e637448f730798c31f6ec10a7a092960b47ba",
  "src/recovery_state.h": "020cc9c8adc602de55d19c2101ada6f8933255a1bb59ea050325201383b21bc3",
  "src/recovery_loop.h": "47265a1912621cbc3914bca7535be64f4919bcda0ac6ecc2b721ae4316d86f62",
  ".pio/build/ttgo_tdisplay_filtered_recovery/firmware.bin": "f6a24781ff57d89dd77607ef4a551b439d513ca8a8302addbd716994e80741d0",
  ".pio/build/ttgo_tdisplay_filtered_recovery/firmware.elf": "56a6bcb2d07dc7bb2b18bf553660dbd00b0c670c4f14298bcc9b3253cba1ca51"
}
```
B20: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/filtered_test_B20_20261117_181133+0000`

Per-upload SHA-256 provenance:

```json
{
  "src/main.cpp": "a26cdef5791d7d4c3088c3da90197fa62a9c596a505be49a08cfa1a3e7e04a44",
  "platformio.ini": "35b4f997966e7a6b6ebd46d165b6fe5667bf723bc2020424f0875582db4a20e6",
  "scripts/upload.py": "cc3ea020e015a1015877e7f4393e637448f730798c31f6ec10a7a092960b47ba",
  "src/recovery_state.h": "020cc9c8adc602de55d19c2101ada6f8933255a1bb59ea050325201383b21bc3",
  "src/recovery_loop.h": "47265a1912621cbc3914bca7535be64f4919bcda0ac6ecc2b721ae4316d86f62",
  ".pio/build/ttgo_tdisplay_filtered_recovery_slot20/firmware.bin": "72743d566464aaec1231487d72f035f3e8f49f0ea98f8514e058624f205da95d",
  ".pio/build/ttgo_tdisplay_filtered_recovery_slot20/firmware.elf": "9606230b8ed823f96d4d6165c58acb3ec561e0a047f66229ec25094426389991"
}
```

Documentation: docs/FILTERED_VALIDATION_RECOVERY.md. All build/upload output, complete raw dumps, manifests and host validation outputs remain local.

## 2026-11-17 — ST7789 DMD replica, grayscale CRCs and recovery hold

User authorized the new ttgo_tdisplay_dmd_replica variant, verified-device
flashing, local captures, and its opt-in existing fault-injection test. Worktree:
`/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence`, branch `experiment/esp32-attract-sequence-recovery`, HEAD
`415ef81657a8e3d2a3e776f7c29c53620bba6e26` plus existing local work.
No commits, branch changes, pushes, merges, GitHub changes or scope operations.
Fish Tales stayed in normal attract mode as reported by the user. Physical wiring
was not changed: level-shifted J603/common ground, RD26/RCLK25/DCLK33/SDATA13,
local CS27 -> CS32. All times are Europe/London (UTC +00:00).

Before changes, saved current modified/untracked files and previous recovery ELF/bin
in `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/replica_preserved_20261117_183619/prechange.zip`, plus full diff and evidence_hashes.json.
Verified all 208 prior evidence files unchanged and the old journal's
complete byte prefix intact. Existing diagnostic environments and full raw batches
were preserved. V1.1 source/config from commit 4b23094 were saved there as references.

### Implementation and bus verification

Added ttgo_tdisplay_dmd_replica and ttgo_tdisplay_dmd_replica_fault_test, extending
the hardware-tested filtered-RD/recovery code. Normal mode disables deliberate
injection; the test enables one labelled slot-8 validation failure after 256 good
refreshes, through the same ownership-reclamation/alignment path. The original
fastReopen source and 100 us deadline were compared and are unchanged. All row,
filtered-RD, DCLK, SPI ownership/length and worker checks remain active.

Reused V1.1 ST7789 setup: TFT MOSI19, SCLK18, CS5, DC16, RST23, BL4, 40 MHz,
135x240 panel, rotation 1, swapBytes=true. MISO explicitly disabled (-1).
RGB565 palette 0000/52AA/AD55/FFFF. Image is 128x32 at (56,51), scale 1x1,
centred with square pixels. The shade strip and status text are outside the ROI.

Capture stays on IDF SPI2_HOST (host enum 1). Installed TFT_eSPI 2.5.43 source
selects SPIClass(VSPI), Arduino port 3, physical SPI3; alternative HSPI/FSPI
selection is rejected and SPI_PORT==VSPI==3 is asserted. Saved runtime startup
logs confirm captureHost=1 SPI2, TFT SPI_PORT=3 VSPI/SPI3, TFT_DMA=0. The LCD uses
CPU pushImage, never initDMA. Relevant installed library source is retained in
TFT_source under each evidence directory; no capture DMA channel is reassigned.

The validated-buffer worker (core 0 priority 3) copies each completed binary
refresh into a bounded 12-slot pool before returning the original DMA buffer.
A lower-priority decoder sums three consecutive binary refreshes into 4096
row-major MSB-first uint8 intensities 0..3, following the V1.1 reference. It hashes
exactly those 4096 unscaled bytes with CRC32/IEEE before RGB565 conversion or text.
A 64-record nonblocking serial queue receives every complete image including
repeated CRCs; a separate newest-image LCD queue has capacity one. Decoder,
UART task and renderer run on core 0; none of their CRC/conversion/TFT/serial
operations occurs in the acquisition task or fastReopen path. Renderer skips,
serial queue losses and validated-binary slot drops have separate counters.
There is no allocation during acquisition/decoding/rendering; fixed work memory
is allocated and checked only at startup.

A gap increments the acquisition epoch immediately. The decoder owner clears its
partial group when the next validated epoch/candidate discontinuity arrives;
no partial group spans the gap. Stale queued old-epoch LCD images are skipped.
The last complete LCD image is held until three fresh complete validated refreshes
form a new image. Already completed pre-gap CRC records remain legitimate. An
in-flight valid LCD transfer may finish; hold measurements use completed draw
boundaries, not an asserted physical latch or optical brightness timing.

Errors counts validation faults once, Recoveries successful realignments, and
Filtered RD stable positive raw-minus-filtered edge deltas. Raw-only observations
and renderer/serial drops do not become capture Errors. The test increments Errors
once for its simulated fault and records injected=1, so genuine Errors=Errors-injected.
LCD status writes all three counters, separate drop counts and latest grayscale CRC.
Saved LCD STATUS values confirm what the software wrote, not an optical readback.

### Build and tests

All 17 Python tests passed, including all 16 existing tests. New tests compile
and run the actual portable C++ decoder with clang++, assert all four intensities
and distinct RGB565 shades, verify a brightness change changes CRC, compare CRC32
with Python zlib, and verify a gap clears a two-refresh partial while retaining
the last complete image. Host audits independently reconstruct the last ten
complete grayscale images per run from raw binary pixel payloads and verify
both CRCs and histograms. They check every numbered record/group boundary against
all 4096 published candidate IDs, complete raw/sequence dumps and hold arithmetic.
Legacy sequence-loader defaults remain unchanged; replica results require explicit
allow_replica=True. git diff --check passed.

Telemetry was disabled with PLATFORMIO_SETTING_ENABLE_TELEMETRY=no. Both builds
passed: normal RAM 116684 bytes / flash 320225 bytes; test RAM 116684 / flash
320477. Python 3.12.2, PlatformIO Core 6.1.13, espressif32 6.3.2, Arduino ESP32
2.0.9 / IDF 4.4.4, Xtensa GCC 8.4.0+2021r2-patch5, TFT_eSPI 2.5.43, SPI 2.0.0.
PIO's existing older-Core notice and TFT_eSPI's unused-touch warning remain.
No touch hardware/functionality is used.

The first link exceeded the ESP32 static DRAM region by 8512 bytes; it was not
flashed. Its full log is preserved in the prechange directory. Fixed decoder and
RGB work buffers were moved to checked internal 8-bit heap allocations at startup,
allowing the build to fit without unbounded buffering or allocation in capture.

Three verified uploads: normal, fault-test, then the same normal image restored.
Each used scripts/upload.py --environment ENV --port /dev/cu.usbserial-02073BE4
--confirm-upload. The helper verified CP2104 10C4:EA60 serial 02073BE4 and flash
hashes; normal uploader resets are separate from the explicit capture resets.
The upload helper allowlist includes both opt-in environments. lsof verified both
serial aliases free before each upload/capture and after capture. Each raw capture
used the unchanged helper for 55 seconds with --reset, at 115200 baud; startup,
FINAL, full sequence dump, REPLICA END and closed-port metadata were verified.
No bytes were receive-buffer purged. No reset occurred within recovery.

These remain bounded 4096-refresh diagnostic runs (~33.55 s), followed by dumps
and idle while holding the final image. The board is finally restored to normal
replica mode, injection disabled; the last capture has completed and its image
is static by design. No indefinite-runtime claim is made.

### Hardware results

| Run | Validated refreshes | Gray images / CRC records | CRC lost | Binary drops | LCD drops | Errors / Recoveries / Filtered RD | maxReopen us |
|---|---:|---|---:|---:|---:|---|---:|
| normal | 4096 | 1365 / 1365 | 0 | 0 | 0 | 0 / 0 / 0 | 85 |
| fault | 4096 | 1365 / 1365 | 0 | 0 | 0 | 1 / 1 / 0 | 89 |
| restored | 4096 | 1365 / 1365 | 0 | 0 | 0 | 0 / 0 / 0 | 85 |

All 4095 complete grayscale images across the three runs had a received numbered,
timestamped CRC record, including repeated signatures. All were rendered in these
runs; the bounded architecture still decouples CRC coverage from optional redraw
skips. All incomplete groups emitted no image CRC. Each normal run ended with one
uncompleted binary refresh in its partial group; the fault-test cleared one partial
at the gap and ended with no remaining partial. No event/detail truncation or lost
CRC records were observed. All expected filtered RD markers agreed in these runs.

Worst handover=89 us versus the unchanged 100 us deadline (11 us observed margin).
The normal/restored runs had 85 us. These software timings are above the earlier
no-LCD recovery runs' 76 us but within the original limit; no deadline was relaxed.

### Grayscale grouping evidence and visual status

V1.1's three-equal-refresh sum is supported by captured Fish Tales periodicity.
In the first normal run, lag-1/2/3 changed-bit totals were 1286681/1388365/328554
with nearly equal sample counts; lag 3 was substantially more repeatable. In its
last nine refreshes, two distinct binary patterns repeated exactly every three
refreshes. All three grouping offsets produced CRC 7CD7A50D and histogram
OFF/L1/L2/FULL=2930/106/50/1010. Thus the static tail contains genuine distinct
intermediate levels, and three-refresh reconstruction is stable for all cyclic
phase offsets in that sampled static content. Phase comparison is saved in
phase_check.json. Transition groups can differ between offsets as content changes.

The user reported: "the brightness looks good. the image is currently static".
Explained that the held static image is expected after the bounded test completes.
This is positive brightness feedback for the displayed image, not a confirmed
comparison of every dim/bright attract scene. Absolute logical grayscale phase
through scene transitions, any dynamic flicker/ghosting, and independent physical
row/orientation matching were not confirmed by the user. They remain UNVERIFIED;
the LCD startup phase label deliberately remains unverified. No claim of universally
correct animated shades or a game-provided absolute grayscale phase marker is made.

### Fault injection: grayscale hold and resumed CRC stream

One labelled software validation failure was injected at capture 257, slot 8,
after 256 complete good refreshes. RCLK/DCLK deltas remained 1/128 and raw/filtered
RD deltas 0/0: this is deliberate validation injection, not a claimed electrical
edge. The suspect 1152-bit transaction was reclaimed and discarded; capture ID 257
is absent from all published IDs. Capture 258 became validated binary sequence 257;
output continued through capture 4097/sequence 4096 without an ESP32 reset.

Before the gap, complete grayscale image 85 ended at candidate 255; valid candidate
256 was only a partial next group. That partial was discarded. Image 86 used fresh
consecutive candidates 258,259,260 in epoch 1; it did not mix candidate 256 or the
suspect 257. The host audit verified this against publication IDs and every GRAY
record. Errors=1, Recoveries=1, injected=1, genuine faults=0, FilteredRD=0 were both
recorded and written by the LCD status task. No renderer or serial losses occurred.

Measured hold (software draw boundaries):

- Held image 85, CRC ED6224C7; first resumed image 86.
- Last completed old draw -> first new draw start: **38.881 ms**.
- Last completed old draw -> first new draw complete: **40.700 ms**.
- Fault detection -> first new grayscale draw start: **33.009 ms**.
- Fault detection -> first good binary refresh end: 14.089 ms;
  binary publication: 14.249 ms.
- Binary last-good -> first-good capture gap: 16.384 ms;
  binary publication gap: 16.379 ms.

Waiting for a new complete three-refresh group is reflected in the longer grayscale
hold; no <=10 ms claim is supported. The hold was verified by image IDs/CRCs and
render instrumentation, not with a scope or independent video measurement.

### Full local evidence and source provenance


normal: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/replica_normal_20261117_184258+0000`

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184435_703484+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184435_703484+0000.json`
- Raw SHA-256: `7d68a666cda8aa99bf243193b24d9c5c54dbad4239aa589aa989e574397b0ec3`
- Sidecar SHA-256: `711946da96a25953c238ddf82066d5f4140b192f84c7ef709c6bdb14e6e64581`
- Window: 55.019 s, 255394 bytes; startup and closure confirmed.

Key authoritative raw lines:

```text
DMD REPLICA enabled: captureHost=1 SPI2; TFT SPI_PORT=3 VSPI/SPI3; ST7789 240x135 rotation=1; origin=56,51 scale=1,1; TFT_DMA=0
GRAY CONFIG group=3 equal-refresh-sum intensity=0..3 row-major-MSB-first crc=CRC32_IEEE_4096_bytes phase=UNVERIFIED; queues binary=12 crc=64 lcd=1
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551450 us maxArmWait=84 us maxCSHigh=65 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=85 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
RESULT: REPLICA VALIDATED OUTPUT PASS - grayscale groups require consecutive validated refreshes
REPLICA SUMMARY grayBuilt=1365 crcEmitted=1365 crcLost=0 binaryDrops=0 rendererDrops=0 rendered=1365 Errors=0 Recoveries=0 FilteredRD=0 injected=0 partialDiscarded=0 partialRemaining=1 allFourImages=394 maxStableRun=115
LCD STATUS Errors=0 Recoveries=0 FilteredRD=0 image=1365 imageCRC=7CD7A50D latestCRC=7CD7A50D
GRAY PERIOD lag=1 samples=4095 changedBits=1286681
GRAY PERIOD lag=2 samples=4094 changedBits=1388365
GRAY PERIOD lag=3 samples=4093 changedBits=328554
GRAY PERIOD lag=6 samples=4090 changedBits=656756
```

Read-only host audit:

```json
{
  "log": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184435_703484+0000.txt",
  "sidecar": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184435_703484+0000.json",
  "metadata": {
    "port": "/dev/cu.usbserial-02073BE4",
    "baud": 115200,
    "duration_seconds": 55.0,
    "reset_requested": true,
    "started": "2026-11-17T18:44:35.703522+00:00",
    "elapsed_seconds": 55.019,
    "bytes": 255394,
    "port_closed": true,
    "startup_captured": true,
    "boot_banner_captured": true
  },
  "sha256": "7d68a666cda8aa99bf243193b24d9c5c54dbad4239aa589aa989e574397b0ec3",
  "sidecar_sha256": "711946da96a25953c238ddf82066d5f4140b192f84c7ef709c6bdb14e6e64581",
  "outcome": "REPLICA_PASS",
  "frames": 4096,
  "maxReopen": 85,
  "summary": {
    "grayBuilt": 1365,
    "crcEmitted": 1365,
    "crcLost": 0,
    "binaryDrops": 0,
    "rendererDrops": 0,
    "rendered": 1365,
    "Errors": 0,
    "Recoveries": 0,
    "FilteredRD": 0,
    "injected": 0,
    "partialDiscarded": 0,
    "partialRemaining": 1,
    "allFourImages": 394,
    "maxStableRun": 115
  },
  "crcCoverageComplete": true,
  "rawTailGrayImagesVerified": 10,
  "lcdStatus": {
    "Errors": 0,
    "Recoveries": 0,
    "FilteredRD": 0,
    "image": 1365,
    "imageCRC": 2094507277,
    "latestCRC": 2094507277
  },
  "holds": [],
  "recoveryEvents": [],
  "recoveryTimings": [],
  "periodicity": [
    {
      "lag": 1,
      "samples": 4095,
      "changedBits": 1286681
    },
    {
      "lag": 2,
      "samples": 4094,
      "changedBits": 1388365
    },
    {
      "lag": 3,
      "samples": 4093,
      "changedBits": 328554
    },
    {
      "lag": 6,
      "samples": 4090,
      "changedBits": 656756
    }
  ],
  "grouping": "three consecutive equal refresh sum; physical phase/brightness needs visual confirmation"
}
```

Per-upload source/config/firmware SHA-256 provenance:

```json
{
  "src/main.cpp": "169b2b776a45794a5df4dea4b21ce586626616e4193bf8eda2127378fec89d51",
  "platformio.ini": "6f7dcdd05160c728f00d737de4bcb64960736b1e6fa41c7bd24b2d39e36f124a",
  "scripts/upload.py": "4903890ffacf35f33121dd389761a2792298af4ba1433394f559418f465656d3",
  "src/grey_decoder.h": "5520cbdb146afb25a88d0f41c28af42210864bb3bd7b48a83abe7ad462c21fc9",
  "src/recovery_state.h": "020cc9c8adc602de55d19c2101ada6f8933255a1bb59ea050325201383b21bc3",
  "src/recovery_loop.h": "668f3eeb930872b7bf92c235065a6d9f8653775fa304bb4308e6ef386a1bff84",
  "src/replica_pipeline.h": "2fb556c9d6f760cadd7664a97e334b5e5064226bc4e3d41f010ba13fa8c02c06",
  ".pio/build/ttgo_tdisplay_dmd_replica/firmware.bin": "ba54ff13d4ea402191ba80f538aa1a3b170305a96d906d99d70548627af12005",
  ".pio/build/ttgo_tdisplay_dmd_replica/firmware.elf": "87f6039d51a1c5a0cb9d57d5f52d1c925d75a35ca75acb3e3d5416cc06434822"
}
```

fault: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/replica_fault_20261117_184650+0000`

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184751_211786+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184751_211786+0000.json`
- Raw SHA-256: `2eef150269b5a46147ff367d74391f761ab0ed5100db85be25c3de804249b977`
- Sidecar SHA-256: `8e28f4a37bf24bffb86feb33be6fbbbdbbfd79dc16d5598b495c76e4e8ce3bfd`
- Window: 55.018 s, 255380 bytes; startup and closure confirmed.

Key authoritative raw lines:

```text
DMD REPLICA enabled: captureHost=1 SPI2; TFT SPI_PORT=3 VSPI/SPI3; ST7789 240x135 rotation=1; origin=56,51 scale=1,1; TFT_DMA=0
GRAY CONFIG group=3 equal-refresh-sum intensity=0..3 row-major-MSB-first crc=CRC32_IEEE_4096_bytes phase=UNVERIFIED; queues binary=12 crc=64 lcd=1
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33559636 us maxArmWait=88 us maxCSHigh=70 us
Handover: lastArm=42 us lastReopen=0 us maxReopen=89 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
RECOVERY EVENT id=0 injected=1 cause=INJECTED validation failure candidate=257 frame=257 slot=8 rawDelta=0 filteredDelta=0 RCLKdelta=1 DCLKdelta=128 level=0 reclaimed=1 droppedBits=1152 aligned=1 generation=258
RECOVERY TIMING id=0 detectedUs=3502710 reclaimedUs=3502805 syncUs=3508616 lastGoodEndUs=3500415 firstGoodSequence=257 firstGoodCandidate=258 firstGoodEndUs=3516799 firstGoodValidatedUs=3516809 firstGoodPublishedUs=3516959 lastGoodPublishedUs=3500580 detectionToGoodUs=14089 goodGapUs=16384 detectionToPublishUs=14249 publicationGapUs=16379
RESULT: REPLICA VALIDATED OUTPUT PASS - grayscale groups require consecutive validated refreshes
REPLICA SUMMARY grayBuilt=1365 crcEmitted=1365 crcLost=0 binaryDrops=0 rendererDrops=0 rendered=1365 Errors=1 Recoveries=1 FilteredRD=0 injected=1 partialDiscarded=1 partialRemaining=0 allFourImages=367 maxStableRun=139
LCD STATUS Errors=1 Recoveries=1 FilteredRD=0 image=1365 imageCRC=7CA02063 latestCRC=7CA02063
GRAY HOLD id=0 detectedUs=3502710 heldN=85 heldCRC=ED6224C7 lastCompleteUs=3496838 firstNewN=86 drawStartUs=3535719 drawEndUs=3537538 holdToStartUs=38881 holdToCompleteUs=40700 detectionToDrawUs=33009
GRAY PERIOD lag=1 samples=4094 changedBits=353287
GRAY PERIOD lag=2 samples=4092 changedBits=439026
GRAY PERIOD lag=3 samples=4090 changedBits=265933
GRAY PERIOD lag=6 samples=4084 changedBits=531228
```

Read-only host audit:

```json
{
  "log": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184751_211786+0000.txt",
  "sidecar": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_184751_211786+0000.json",
  "metadata": {
    "port": "/dev/cu.usbserial-02073BE4",
    "baud": 115200,
    "duration_seconds": 55.0,
    "reset_requested": true,
    "started": "2026-11-17T18:47:51.211830+00:00",
    "elapsed_seconds": 55.018,
    "bytes": 255380,
    "port_closed": true,
    "startup_captured": true,
    "boot_banner_captured": true
  },
  "sha256": "2eef150269b5a46147ff367d74391f761ab0ed5100db85be25c3de804249b977",
  "sidecar_sha256": "8e28f4a37bf24bffb86feb33be6fbbbdbbfd79dc16d5598b495c76e4e8ce3bfd",
  "outcome": "REPLICA_PASS",
  "frames": 4096,
  "maxReopen": 89,
  "summary": {
    "grayBuilt": 1365,
    "crcEmitted": 1365,
    "crcLost": 0,
    "binaryDrops": 0,
    "rendererDrops": 0,
    "rendered": 1365,
    "Errors": 1,
    "Recoveries": 1,
    "FilteredRD": 0,
    "injected": 1,
    "partialDiscarded": 1,
    "partialRemaining": 0,
    "allFourImages": 367,
    "maxStableRun": 139
  },
  "crcCoverageComplete": true,
  "rawTailGrayImagesVerified": 10,
  "lcdStatus": {
    "Errors": 1,
    "Recoveries": 1,
    "FilteredRD": 0,
    "image": 1365,
    "imageCRC": 2090868835,
    "latestCRC": 2090868835
  },
  "holds": [
    {
      "id": 0,
      "detectedUs": 3502710,
      "heldN": 85,
      "heldCRC": 3982632135,
      "lastCompleteUs": 3496838,
      "firstNewN": 86,
      "drawStartUs": 3535719,
      "drawEndUs": 3537538,
      "holdToStartUs": 38881,
      "holdToCompleteUs": 40700,
      "detectionToDrawUs": 33009
    }
  ],
  "recoveryEvents": [
    {
      "id": 0,
      "injected": 1,
      "candidate": 257,
      "frame": 257,
      "slot": 8,
      "rawDelta": 0,
      "filteredDelta": 0,
      "RCLKdelta": 1,
      "DCLKdelta": 128,
      "level": 0,
      "reclaimed": 1,
      "droppedBits": 1152,
      "aligned": 1,
      "generation": 258
    }
  ],
  "recoveryTimings": [
    {
      "id": 0,
      "detectedUs": 3502710,
      "reclaimedUs": 3502805,
      "syncUs": 3508616,
      "lastGoodEndUs": 3500415,
      "firstGoodSequence": 257,
      "firstGoodCandidate": 258,
      "firstGoodEndUs": 3516799,
      "firstGoodValidatedUs": 3516809,
      "firstGoodPublishedUs": 3516959,
      "lastGoodPublishedUs": 3500580,
      "detectionToGoodUs": 14089,
      "goodGapUs": 16384,
      "detectionToPublishUs": 14249,
      "publicationGapUs": 16379
    }
  ],
  "periodicity": [
    {
      "lag": 1,
      "samples": 4094,
      "changedBits": 353287
    },
    {
      "lag": 2,
      "samples": 4092,
      "changedBits": 439026
    },
    {
      "lag": 3,
      "samples": 4090,
      "changedBits": 265933
    },
    {
      "lag": 6,
      "samples": 4084,
      "changedBits": 531228
    }
  ],
  "grouping": "three consecutive equal refresh sum; physical phase/brightness needs visual confirmation"
}
```

Per-upload source/config/firmware SHA-256 provenance:

```json
{
  "src/main.cpp": "169b2b776a45794a5df4dea4b21ce586626616e4193bf8eda2127378fec89d51",
  "platformio.ini": "6f7dcdd05160c728f00d737de4bcb64960736b1e6fa41c7bd24b2d39e36f124a",
  "scripts/upload.py": "4903890ffacf35f33121dd389761a2792298af4ba1433394f559418f465656d3",
  "src/grey_decoder.h": "5520cbdb146afb25a88d0f41c28af42210864bb3bd7b48a83abe7ad462c21fc9",
  "src/recovery_state.h": "020cc9c8adc602de55d19c2101ada6f8933255a1bb59ea050325201383b21bc3",
  "src/recovery_loop.h": "668f3eeb930872b7bf92c235065a6d9f8653775fa304bb4308e6ef386a1bff84",
  "src/replica_pipeline.h": "2fb556c9d6f760cadd7664a97e334b5e5064226bc4e3d41f010ba13fa8c02c06",
  ".pio/build/ttgo_tdisplay_dmd_replica_fault_test/firmware.bin": "1c06d26cde4b3a2286ef7ea2526c2a60f3f68295b84ddb499a3dd4126746ab81",
  ".pio/build/ttgo_tdisplay_dmd_replica_fault_test/firmware.elf": "94ee024e6004980c99a8696afd7585938baa85725143e68da2731ac62bb191a6"
}
```

restored: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/replica_restored_20261117_184956+0000`

- Raw: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_185116_202747+0000.txt`
- Sidecar: `/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_185116_202747+0000.json`
- Raw SHA-256: `e8d31ccac36287dccb4a11f500cc36b9858d71b2eea2d8784eeaca458da74434`
- Sidecar SHA-256: `8cb1d55b71857c7683f3b854dce44b6daba7e0e20c8e94de3a6964f68ba388f4`
- Window: 55.023 s, 255051 bytes; startup and closure confirmed.

Key authoritative raw lines:

```text
DMD REPLICA enabled: captureHost=1 SPI2; TFT SPI_PORT=3 VSPI/SPI3; ST7789 240x135 rotation=1; origin=56,51 scale=1,1; TFT_DMA=0
GRAY CONFIG group=3 equal-refresh-sum intensity=0..3 row-major-MSB-first crc=CRC32_IEEE_4096_bytes phase=UNVERIFIED; queues binary=12 crc=64 lcd=1
V4c FINAL frames=4096/4096 processed=4096 rows=131072 DCLK=16777216 RDedges=4096 elapsed=33551441 us maxArmWait=84 us maxCSHigh=65 us
Handover: lastArm=41 us lastReopen=0 us maxReopen=85 us reopened=0 DCLKwhileHigh=-1 deadline=100 us
RESULT: REPLICA VALIDATED OUTPUT PASS - grayscale groups require consecutive validated refreshes
REPLICA SUMMARY grayBuilt=1365 crcEmitted=1365 crcLost=0 binaryDrops=0 rendererDrops=0 rendered=1365 Errors=0 Recoveries=0 FilteredRD=0 injected=0 partialDiscarded=0 partialRemaining=1 allFourImages=262 maxStableRun=92
LCD STATUS Errors=0 Recoveries=0 FilteredRD=0 image=1365 imageCRC=898DED5E latestCRC=898DED5E
GRAY PERIOD lag=1 samples=4095 changedBits=1429235
GRAY PERIOD lag=2 samples=4094 changedBits=1502576
GRAY PERIOD lag=3 samples=4093 changedBits=238633
GRAY PERIOD lag=6 samples=4090 changedBits=476790
```

Read-only host audit:

```json
{
  "log": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_185116_202747+0000.txt",
  "sidecar": "/Users/michaellewis/Documents/PlatformIO/Projects/Pinball-DMD-Capture-attract-sequence/experiments/esp32-dma-v4c/diagnostics/ttgo_serial_20261117_185116_202747+0000.json",
  "metadata": {
    "port": "/dev/cu.usbserial-02073BE4",
    "baud": 115200,
    "duration_seconds": 55.0,
    "reset_requested": true,
    "started": "2026-11-17T18:51:16.202780+00:00",
    "elapsed_seconds": 55.023,
    "bytes": 255051,
    "port_closed": true,
    "startup_captured": true,
    "boot_banner_captured": true
  },
  "sha256": "e8d31ccac36287dccb4a11f500cc36b9858d71b2eea2d8784eeaca458da74434",
  "sidecar_sha256": "8cb1d55b71857c7683f3b854dce44b6daba7e0e20c8e94de3a6964f68ba388f4",
  "outcome": "REPLICA_PASS",
  "frames": 4096,
  "maxReopen": 85,
  "summary": {
    "grayBuilt": 1365,
    "crcEmitted": 1365,
    "crcLost": 0,
    "binaryDrops": 0,
    "rendererDrops": 0,
    "rendered": 1365,
    "Errors": 0,
    "Recoveries": 0,
    "FilteredRD": 0,
    "injected": 0,
    "partialDiscarded": 0,
    "partialRemaining": 1,
    "allFourImages": 262,
    "maxStableRun": 92
  },
  "crcCoverageComplete": true,
  "rawTailGrayImagesVerified": 10,
  "lcdStatus": {
    "Errors": 0,
    "Recoveries": 0,
    "FilteredRD": 0,
    "image": 1365,
    "imageCRC": 2307779934,
    "latestCRC": 2307779934
  },
  "holds": [],
  "recoveryEvents": [],
  "recoveryTimings": [],
  "periodicity": [
    {
      "lag": 1,
      "samples": 4095,
      "changedBits": 1429235
    },
    {
      "lag": 2,
      "samples": 4094,
      "changedBits": 1502576
    },
    {
      "lag": 3,
      "samples": 4093,
      "changedBits": 238633
    },
    {
      "lag": 6,
      "samples": 4090,
      "changedBits": 476790
    }
  ],
  "grouping": "three consecutive equal refresh sum; physical phase/brightness needs visual confirmation"
}
```

Per-upload source/config/firmware SHA-256 provenance:

```json
{
  "src/main.cpp": "169b2b776a45794a5df4dea4b21ce586626616e4193bf8eda2127378fec89d51",
  "platformio.ini": "6f7dcdd05160c728f00d737de4bcb64960736b1e6fa41c7bd24b2d39e36f124a",
  "scripts/upload.py": "4903890ffacf35f33121dd389761a2792298af4ba1433394f559418f465656d3",
  "src/grey_decoder.h": "5520cbdb146afb25a88d0f41c28af42210864bb3bd7b48a83abe7ad462c21fc9",
  "src/recovery_state.h": "020cc9c8adc602de55d19c2101ada6f8933255a1bb59ea050325201383b21bc3",
  "src/recovery_loop.h": "668f3eeb930872b7bf92c235065a6d9f8653775fa304bb4308e6ef386a1bff84",
  "src/replica_pipeline.h": "2fb556c9d6f760cadd7664a97e334b5e5064226bc4e3d41f010ba13fa8c02c06",
  ".pio/build/ttgo_tdisplay_dmd_replica/firmware.bin": "ba54ff13d4ea402191ba80f538aa1a3b170305a96d906d99d70548627af12005",
  ".pio/build/ttgo_tdisplay_dmd_replica/firmware.elf": "87f6039d51a1c5a0cb9d57d5f52d1c925d75a35ca75acb3e3d5416cc06434822"
}
```

Documentation and implementation: docs/DMD_REPLICA.md, src/grey_decoder.h and src/replica_pipeline.h. Raw logs and sidecars remain local and ignored; existing journals/evidence preserved.

## 2026-11-23 Freezy export on Fish Tales (GMT)

Source/configuration before the change are in
`diagnostics/freezy_20261123/baseline.tar` with `baseline.sha256`. The two
flashed builds have manifests `run-921600.sha256` and `run.sha256` in the
same directory, with binaries and upload logs. PlatformIO Core 6.1.13,
espressif32 6.3.2, Arduino, esptool 4.5.1, Python 3.12.2 and pySerial 3.5.
USB adapter 10C4:EA60 serial 02073BE4. Firmware reported GPIO26 RD, GPIO25
RCLK, GPIO33 DCLK, GPIO13 SDATA and local CS jumper GPIO27 to GPIO32.
Physical wiring was inferred from the existing setup. Spot checks of
exported images show the Fish Tales logo, free-play messages and credits.

For each approved flash:

```sh
python3 scripts/upload.py --environment ttgo_tdisplay_dmd_replica_freezy --port /dev/cu.usbserial-02073BE4 --confirm-upload
```

The uploader reset via RTS. Each acquisition below requested an explicit
RTS reset with a 55-second serial capture. Raw files and JSON sidecars are
under `diagnostics/freezy_20261123/`, with checksums in `capture.sha256`.

| Raw log suffix | Capture command arguments to `python3 scripts/capture.py` | Measured result |
| --- | --- | --- |
| `115708_861933+0000.txt` | `--port /dev/cu.usbserial-02073BE4 --baud 921600 --duration 55 --reset` | 4096/4096 binary refreshes, 33,551,387 µs, max arm 85 µs, max CS high 66 µs. Corrupt serial packets and no usable export summary. |
| `115820_957714+0000.txt` | `--port /dev/cu.SLAB_USBtoUART --baud 921600 --duration 55 --reset` | 4096/4096 binary refreshes, 33,551,383 µs, max arm 85 µs, max CS high 66 µs. Corrupt serial packets and no usable export summary. |
| `115952_190944+0000.txt` | `--port /dev/cu.usbserial-02073BE4 --baud 460800 --duration 55 --reset` | 4096/4096 binary refreshes, 33,551,399 µs, max arm 83 µs, max CS high 64 µs. Corrupt serial packets before the host timeout fix. |
| `120046_723335+0000.txt` | `--port /dev/cu.usbserial-02073BE4 --baud 460800 --duration 55 --reset` | Exclusive-open lock refused while the earlier capture was closing. Zero bytes and no reset pulse. |
| `120056_769865+0000.txt` | `--port /dev/cu.usbserial-02073BE4 --baud 460800 --duration 55 --reset` | Fixed host read timeout. Startup captured, 1,535,194 raw bytes, port closed. |

The final run processed 4096/4096 binary refreshes in 33,551,398 µs and
reported REPLICA VALIDATED OUTPUT PASS. RCLK rows 131,072, DCLK edges
16,777,216 and RD edges 4096. Max arm 83 µs, max CS high 64 µs, max reopen
84 µs, max callback 70 µs. The decoder built and rendered 1365 complete
images with one binary refresh left in an incomplete group. Zero capture
errors, recoveries, input drops or renderer drops. The export queue dropped
58 images and reported zero UART send errors. All 1307 received images
matched the per-frame CRC, including final image ED6224C7. The early runs
used a serial read loop that reassigned the timeout. Inspection of pySerial
showed that the reassignment can briefly set the fallback serial speed on
macOS. Holding the timeout fixed produced intact transport at 460800.

`fish-tales-attract.txt` contains the first 18 seconds of the usable run,
bounded by sampled frame times. The 706 frames span 17.988 seconds with
27 one-frame export-queue gaps of about 49.15 ms. The recorded acquisition
times retain those gaps. The file was reread for format verification and
compared byte-for-byte with a second export from the preserved raw log.
The full gap list is in `export-report.txt`. Windows playback was unavailable
on this workstation.

## 2026-11-23 manual flash check (GMT)

At 12:54 the user reported a manual compile and flash. Attached with
`python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --baud 115200 --duration 16`.
No explicit reset was requested. A boot banner and V4c startup were observed
during the attach window. The build hash of the running firmware is unknown.
The finite 4096-refresh startup and per-second CRC reports identify the
baseline path. There were no WebSocket startup or status records.

Raw log `diagnostics/ttgo_serial_20261123_125424_701333+0000.txt`,
SHA-256 `f753185dceb8f97f34fb368dcfa278c181f995bb62b3902b383e38e8c0a32038`.
Last observed progress was 1721/4096 at 14,097,167 µs with staticDiff=0,
first CRC 67D75B8E and last CRC 57FF91FB. The port closed before a final
result. The running pattern and physical wiring were not independently verified.

Built `ttgo_tdisplay_dmd_replica_ws` with the saved local settings using
`sh scripts/build.sh ttgo_tdisplay_dmd_replica_ws`. Firmware SHA-256
`15408629127078c220a736eed0d503cee5d63e32dec16adfba2c9cb738e9d5c7`.

### Approved live Loki test

PlatformIO Core 6.1.13, espressif32 6.3.2, Arduino esp32dev, TFT_eSPI
2.5.43, esptool 4.5.1. Used the WebSocket firmware hash above and
the non-secret source hashes in `diagnostics/ws_20261123/source.sha256`.
Wiring was inferred from the existing setup. Game mode was not independently
verified during this run.

```sh
python3 scripts/upload.py --environment ttgo_tdisplay_dmd_replica_ws --port /dev/cu.usbserial-02073BE4 --confirm-upload
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --baud 115200 --duration 35 --reset
```

Upload verified flash contents and reset via RTS. Capture pulsed RTS,
recorded boot and application startup, and closed the port after 35.015 s.
Raw log `diagnostics/ttgo_serial_20261123_125637_486432+0000.txt` and
its JSON sidecar were preserved. Log SHA-256
`b804a108c4eb3d7aebe11a4d3a1c983b4103843310a702a91a4aa7b7dd6264f8`.

Wi-Fi joined Loki. At ws://192.168.0.125:8765/dmd the client reported
three attempts, two transport connection failures and zero connections
or frames sent. Two outbound frames were counted as offline drops.
The continuous capture independently stopped after seven validated
refreshes on the 100 µs handover/clock validation check at 1,511,506 µs.
Largest recorded reopen 71 µs and alignment 61 µs. Both complete
grayscale frames had CRC 51169014. Decoder input drops were zero and
renderer drops were one. The log gives no cause for the capture fault
beyond the validation check, and does not establish a link to Wi-Fi.

### 13:18 crash report follow-up (GMT)

Attached to the existing WebSocket build with
`python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --baud 115200 --duration 25`.
No explicit reset requested. The log began with a power-on reset banner and
application startup. Port closed after 25.015 s. The raw log and sidecar are
`diagnostics/ttgo_serial_20261123_131845_401768+0000.{txt,json}`.
Raw SHA-256 `cfdd424d339a0148031ce650bd389995f257c941561c3615ecac082b6516e8ac`.
Firmware configuration and hashes are those of the preceding live test.

Capture repeated the 100 µs handover/clock validation stop at 1,512,506 µs,
after 7 validated refreshes (max recorded reopen 71 µs, max alignment 62 µs).
Two grayscale frames both had CRC 24EC0FD7. One renderer drop and zero
decoder input drops. The console continued running, with no panic,
watchdog report, or second boot in this 25-second window. Loki and the
receiver were connected and dimensions had been sent (`wifi=1 ready=1`,
one connection). Two frames were completed and counted as offline drops
before the socket was ready. Zero frames were sent. The collapsed fatal
message leaves SPI readiness, dot clocks and the exact deadline failure
undistinguished. Game mode and physical wiring were not independently verified.

### 13:27 silent-input watchdog comparison (GMT)

User confirmed Fish Tales off and approved two uploads while TTGO USB power
remained connected. J603 stayed in the existing setup, without an independent
measurement of its idle levels. PlatformIO Core 6.1.13, espressif32 6.3.2,
Arduino esp32dev, esptool 4.5.1. No firmware edits. The SDK configuration has
the Core 1 interrupt watchdog enabled at 300 ms. Capture's 100 µs handover
check remains enabled. The two configurations are
`ttgo_tdisplay_dmd_replica_ws` and `ttgo_tdisplay_dmd_replica_continuous`.

Private archive `diagnostics/ws_20261123/fingerprint_8b1b55f9f23b0050/`
contains the exact ELF and app image, source and build hashes, upload logs,
and complete `addr2line.txt`. ELF SHA-256
`8b1b55f9f23b0050ed4cea669a61a614cbfacdc7f39010e5f326cac817938ed3`.
The packed app descriptor has the same SHA-256. The command used PlatformIO's
`xtensa-esp32-elf-addr2line -a -f -C -i -p -e firmware.elf` with the seven
reported addresses. EPC1 0x400e6b8f resolves to `uart_hal_write_txfifo`.
0x400814e6 and 0x40081542 resolve to `gpio_ll_get_intr_status_high` in
`gpio_intr_service`. 0x400814a3 resolves to `gpio_intr_service` entry.
Core 0 resolves to `cpu_ll_waiti` via `esp_pm_impl_waiti`,
`esp_vApplicationIdleHook`, and `prvIdleTask`.

Marker 20 prints and flushes UART. The task then raises priority, stores its
handle and installs the rising-edge interrupt for RCLK GPIO25 before marker
22 prints and flushes UART. The handler records the cycle count, notifies the
acquisition task and can request a yield. There is no application critical
section between markers. After marker 22 the inactive path yields 5 ms per
poll, samples PCNT, and briefly holds contMux to mark silence. RCLK is a plain
input and its ISR is armed before the code checks for clock activity. The
shared GPIO interrupt PCs identify neither a triggering pin nor the cause.

Each capture used
`python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --baud 115200 --duration 25 --reset`
and saved its raw log and JSON sidecar in `diagnostics/`. No frames were
captured in any window and no live DMD test was run.

- A, original WebSocket image. `ttgo_serial_20261123_132730_647947+0000.txt`,
  SHA-256 `6fdb613b6bf2529f39913a78b4a98cdd53612dc5e9eecf2be6fb85234edc0a77`.
  11 reset banners, six explicit Core 1 watchdog panics with matching
  fingerprints, nine marker 20 lines, zero marker 22 lines. Early WAITING
  telemetry recurred across boots and did not demonstrate stable startup.
- B, WebSocket disabled. First upload via
  `python3 scripts/upload.py --environment ttgo_tdisplay_dmd_replica_continuous --port /dev/cu.usbserial-02073BE4 --confirm-upload`
  failed before writing (boot mode 0x13). User entered ROM download mode
  without unplugging USB. Verified the USB identity and identical partition
  tables, then used PlatformIO's esptool with
  `--chip esp32 --port /dev/cu.usbserial-02073BE4 --baud 115200 --before no_reset --after hard_reset write_flash -z 0x10000 diagnostics/ws_20261123/fingerprint_8b1b55f9f23b0050/b-off-firmware.bin`.
  `ttgo_serial_20261123_132930_962680+0000.txt`,
  SHA-256 `816b0194e67166d5622a8235328156599b744edcb3516f19bf7ea049d5d1013b`.
  One boot, marker 22 at 1439 ms and 23 WAITING reports through 23.667 s
  uptime, with zero frames, errors, losses, or decoder input drops.
- A2, restored original WebSocket image using
  `python3 scripts/upload.py --environment ttgo_tdisplay_dmd_replica_ws --port /dev/cu.usbserial-02073BE4 --confirm-upload`.
  Verified ELF and app binary byte-for-byte against the archive after upload.
  `ttgo_serial_20261123_133053_952411+0000.txt`,
  SHA-256 `bad44bff04da5ed804d5c01b55ead09f4fe1a4bd281f10c9bdfe58a24d096a66`.
  One boot, marker 22 at 1450 ms and 23 WAITING reports through 23.770 s
  uptime, with zero frames, errors, losses, or decoder input drops.
  Loki and the WebSocket handshake succeeded, with zero frames sent.

The watchdog fault was reproduced with WebSocket enabled in A and did not
recur with the identical image in A2. These runs establish that both builds
can remain in WAITING for the observed windows. They do not isolate the
dependency on networking, GPIO edge activity, or boot history.

### 13:41–13:44 shared handover diagnostic, Fish Tales off-to-on (GMT)

Both continuous environments were built with `sh scripts/build.sh` using
PlatformIO Core 6.1.13, espressif32 6.3.2, Arduino esp32dev and esptool 4.5.1.
Before-change sources, new artifact/source hashes, matching private ELFs, and
the exact command runner are in `diagnostics/ws_handover_20261123/`. The
100 µs handover limit and watchdog configuration were retained. The terminal
transaction snapshot records final row deltas, the separate CS-high dot
audit when available, completion length and the cycle count relative to the
row ISR. It prints after CS is raised, the row ISR is detached and task
priority is restored. Existing recoverable row validation remains separate.

User confirmed Fish Tales off and USB held connected for each load. Wiring
was inferred from the existing setup. The first automatic WebSocket-off
upload failed before writing in boot mode 0x13. For both successful uploads
the user entered manual download mode. The runner verified USB identity and
equal partition tables, then used PlatformIO esptool with
`--chip esp32 --port /dev/cu.usbserial-02073BE4 --baud 115200 --before no_reset --after hard_reset write_flash -z 0x10000 IMAGE`.
Each run used `python3 scripts/capture_continuous.py --port /dev/cu.usbserial-02073BE4 --duration 180 --reset --stop-file diagnostics/ws_handover_20261123/MODE.stop`
with MODE off or on. Serial RTS reset was explicit. Boot and continuous
startup were captured in both logs. After observing WAITING with zero frames,
the user was signalled to switch Fish Tales on. Both ports closed cleanly.

- **WebSocket off**, `ttgo_tdisplay_dmd_replica_continuous`.
  Raw `diagnostics/ttgo_continuous_20261123_134109_644032+0000.txt`,
  SHA-256 `8ba820ecc113d09a1ee5e357cd93d011d315ce754b69731be8ef296a4ffbdb20`.
  ALIGNED at 30.138547 s, last active status at 60.664231 s, with 3726
  validated refreshes and 1242 grayscale frames. Max alignment 56 µs,
  max reopen 83 µs. Zero capture errors, losses, recoveries and queue
  drops. First grayscale CRC 10617B20, last status LCD CRC 898DED5E.
  No terminal snapshot.
- **WebSocket on**, `ttgo_tdisplay_dmd_replica_ws`.
  Raw `diagnostics/ttgo_continuous_20261123_134354_994165+0000.txt`,
  SHA-256 `8b5c0e8def87b538f50148b4c74adfa21a42798aaed8f8efc5dc6de8044ed710`.
  Loki and receiver were ready before power-on. ALIGNED at 28.459286 s
  with 64 µs alignment. Transaction 1, candidate 1 stopped on the SPI
  setup callback observed too late. Observation was at 26,451 cycles
  (110.2125 µs at 240 MHz) against the 100 µs limit. Final row expected
  and observed 1, dot expected and observed 128, SPI bits expected and
  observed 4096, SPI result ESP_OK. The CS-high dot audit was not reached.
  Zero refreshes published and zero grayscale frames or CRCs. The
  receiver stayed connected with zero frames sent and zero outbound drops.
  The log contained one snapshot, and no watchdog panic in the run.

The callback observation time measures when capture code observed setup
ready. It does not identify when the callback first executed or what caused
the delay. This comparison associates a deadline miss with the WS-on test
and does not isolate the mechanism.

### 13:48–13:56 handover timestamp comparison (GMT)

Both continuous environments were built with `sh scripts/build.sh` using
PlatformIO Core 6.1.13, espressif32 6.3.2, Arduino esp32dev, esptool 4.5.1.
Private ELFs, images, source hashes and the exact upload/capture runner
are in `diagnostics/ws_timestamps_20261123/`. The runner verifies the
CP2104 and equal partition tables, then writes the app image with
PlatformIO's esptool `--before no_reset --after hard_reset write_flash -z 0x10000`
after the user puts the TTGO in ROM download mode. The watchdog and
100 µs handover limit remained enabled. No panic occurred in the six runs.

For the continuous build, the first generation change sampled by the
capture task is timestamped immediately after the read. A final sample is
taken at the timeout boundary if necessary. The existing callback entry
cycle write was reused without any new callback work. Deadline start is the
existing final row ISR stamp. Arduino setup and loop share Core 1, and
ESP-IDF runs the SPI ISR on the core that initialized the driver. The row
ISR is installed by that same loop task. The three timestamps therefore use
the same 32-bit CCOUNT timebase and are compared modulo 2^32. A missing
callback is marked invalid. Reporting occurs after CS is raised, the row
ISR detached and task priority restored.

Each boot used `python3 scripts/capture_continuous.py --port /dev/cu.usbserial-02073BE4 --duration 180 --reset --stop-file diagnostics/ws_timestamps_20261123/RUN.stop`.
The user kept TTGO USB connected and switched Fish Tales off and on upon
request. Wiring and game mode were not independently verified. In all
six runs we observed WAITING with zero frames before power-on, plus the
receiver handshake for WS-on. Captures ended cleanly after roughly ten
seconds from ALIGNED or two seconds after a terminal stop. The first off
serial attempt was stopped by the host watcher on overlapping upload and
capture reset banners, then repeated. The first on upload got no serial
data and wrote nothing. The user re-entered download mode for the retry.
Evidence for both attempts remains in diagnostics.

| Mode/run | Raw log suffix | Result | Callback entry / observed / difference (µs) |
| --- | --- | --- | --- |
| Off 1 | 134827_311426+0000 | 1309 refreshes, 436 grayscale, max reopen 77 µs, LCD CRC A85C340B | No failure |
| Off 2 | 135029_122372+0000 | 1322 refreshes, 440 grayscale, max reopen 76 µs, LCD CRC A7320CBC | No failure |
| Off 3 | 135108_382532+0000 | 1227 refreshes, 409 grayscale, max reopen 78 µs, LCD CRC 8A21E3DA | No failure |
| On 1 | 135414_615346+0000 | Late setup, transaction 1 | 103.992 / 114.050 / 10.058 |
| On 2 | 135511_317444+0000 | Late setup, transaction 1 | 103.004 / 112.388 / 9.383 |
| On 3 | 135621_828163+0000 | Late setup, transaction 1 | 103.979 / 113.363 / 9.383 |

On all three failures the final row delta 1 and dot delta 128 matched
expectations, and SPI completion supplied 4096/4096 bits. The deadline
branch preceded the CS-high DCLK audit. Nothing was published or sent, so
there are no grayscale CRCs for the on runs. The off runs reported no
capture errors or decoder input drops. Full raw log names, SHA-256 values,
state/CRC lines and absolute timestamp records are indexed in
`diagnostics/ws_timestamps_20261123/run-summary.txt`. The completed logs
and their sidecars are in diagnostics. The WebSocket-on build remains loaded.

Callback entry itself was beyond 100 µs every time. The entry-to-observation
interval includes callback execution and ISR/task return, so it is not a
pure polling delay. These tests do not distinguish SPI hardware readiness,
interrupt delivery, or driver work before invoking the callback.
