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
