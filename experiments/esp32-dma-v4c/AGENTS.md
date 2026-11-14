# Pinball DMD Capture project guidance

## Objective and baseline
Capture the Williams/Bally 128x32 DMD stream from level-shifted J603 signals
using the original ESP32 TTGO T-Display. Preserve row, dot-clock, RD and DMA
continuity evidence. V4c is the current hardware-tested experimental baseline;
it is not a passing 4096-frame baseline. See docs/ENGINEERING_JOURNAL.md.
Preserve known working variants separately, with source/configuration hashes
and test evidence before proposing firmware changes.
The user confirmed V4c and the v1.2a-acquisition tests must remain separate
projects within the repository, with independent source/configuration/evidence
and builds. Do not substitute one firmware for the other. See the integration
proposal for the distinction between branch names and actual firmware versions.

## Hardware and tools
- PlatformIO: pio
- Environment ttgo_tdisplay; espressif32@6.3.2; esp32dev; Arduino.
- CP2104 USB 10C4:EA60 serial 02073BE4, normally /dev/cu.usbserial-02073BE4.
- Serial 115200 baud. /dev/cu.SLAB_USBtoUART aliases the same adapter.
- RD=26, RCLK=25, DCLK=33, SDATA=13, local CS jumper GPIO27 to GPIO32.
- J603 inputs must be after level shifters; retain common ground.

## Workflow and approval boundaries
Read docs/AUTOMATION.md. Build with `sh scripts/build.sh`. Read-only log analysis
does not require a new hardware run. Record source hashes, tool/environment,
commands, wiring/pattern/mode when known, log paths, reset method, frame/count,
timing, CRC and error evidence in the engineering journal for each hardware run.
Preserve full raw logs in diagnostics/ (ignored), including early failures;
summaries must distinguish measured results from interpretation and unknowns.

Obtain explicit user approval before flashing, pushing, merging, deleting files,
or changing branches. Existing session authorization applies only to its stated
scope; earlier uploads do not authorize a new upload. The upload helper requires
an explicit port, verifies USB identity, and requires --confirm-upload. Upload
resets the board. Never flash automatically as part of build or validation.

Capture/reset starts or may start a hardware test. Obtain authorization for a
new acquisition test before connecting to the real port or using --reset.
Opening a port is not a dependable reset mechanism and some drivers may pulse
control lines. Close other monitors; preserve all bytes; always release the
port. Confirm startup from the saved log, not from a reset assumption.

During infrastructure work, do not alter capture/acquisition logic, timing,
RD phase checks, pins, buffers, or firmware configuration. Do not resynchronize
to hide discontinuities. Do not modify user-level Codex config.toml. Do not
overwrite or discard source, reset Git, or change history. Propose integration
separately and preserve both V4c and repository baselines before approved work.
