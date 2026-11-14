# Pinball DMD development

Objective: Williams/Bally 128x32 DMD capture using an original ESP32 TTGO
T-Display and level-shifted inputs. Environment ttgo_tdisplay uses
espressif32@6.3.2, esp32dev and Arduino; serial 115200.
Local PlatformIO executable:
pio
TTGO adapter: /dev/cu.usbserial-02073BE4, CP2104 10C4:EA60 serial 02073BE4.

## Independent projects
- Root: preserve origin/main's existing V1.2a RCLK counter source; the build
  configuration adds documented deep+ dependency discovery.
- experiments/v1.2a-acquisition: snapshot of the feature branch, whose actual
  firmware is V1.2g CPU timing diagnostics. Preserve its firmware source;
  the configuration has a documented deep+ dependency-discovery override.
- experiments/esp32-dma-v4c: independently tested V4c DMA experiment, with its
  own source/configuration, helpers and engineering journal. Its recorded
  run stopped early; do not describe it as a passing 4096-frame baseline.

Never merge or substitute experiment firmware. Build each project independently
using `pio run -d <project-directory> -e ttgo_tdisplay`. From an experiment,
`sh scripts/build.sh` uses that experiment's configuration. Read the experiment
README and docs/DEVELOPMENT_AUTOMATION_REVIEW.md before operating hardware.

Preserve known working firmware and test evidence. Before changes, record hashes
or commits and retain backups. For every authorized hardware run record date,
timezone, exact source/configuration/tool versions, wiring/pattern/mode when
known, command, reset method, full raw log path/hash, completion status, counts,
timing/CRC/errors and unknowns. Raw diagnostics stay ignored by default; preserve
all startup and early failure bytes. Keep reviewed summaries in an engineering
journal. V4c's analyser is specific to its log format.

Obtain explicit user approval before flashing, pushing, merging, deleting files
or changing branches. Prior uploads do not authorize new ones. Capture/reset
can start a test: get authorization before opening a hardware port. Verify USB
identity, close other monitors, use explicit --reset only when authorized,
confirm startup from the saved log, and always close the port. The upload helper
requires --confirm-upload and an explicit verified port. CI must only build/test.

During infrastructure work do not alter acquisition logic, RD phase checks,
timing, pins or buffers. Build configuration fixes require user authorization
and documented provenance; the current authorized override is lib_ldf_mode=deep+
for root and the v1.2a-acquisition snapshot, leaving V4c configuration unchanged. Do not resynchronize to hide
failures. Preserve Git history; do not reset/discard/overwrite existing work.
Do not change user-level Codex config.toml. Keep the original project folders
intact. Current authorization allows preparing this branch/worktree, not commits,
pushes, merges or hardware access.
