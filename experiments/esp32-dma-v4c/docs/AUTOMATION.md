> Copied from the original V4c workspace as historical infrastructure documentation.
> Current repository layout/status: ../../../docs/DEVELOPMENT_AUTOMATION_REVIEW.md.
> Prior raw logs and backup archives remain in the original V4c folder; they are not copied here.

# Local automation

Run from V4c. Use Python 3.12 with PlatformIO and pyserial available. Helpers
use python3 and pio from PATH; build/upload accept an optional PIO environment
variable naming the executable. No workstation-specific interpreter is required.
Only pyserial is needed beyond the standard library for hardware helpers. Helpers also locate their project root when
invoked from another directory. No helper edits firmware or Git state.

```sh
sh scripts/build.sh
python3 scripts/upload.py --help
python3 scripts/capture.py --help
python3 scripts/analyse_log.py diagnostics/ttgo_serial_20261114_130746.txt
```

Only after explicit approval for a new flash:

```sh
python3 scripts/upload.py --port /dev/cu.usbserial-02073BE4 --confirm-upload
```

Upload enumerates the specified port and refuses a missing/mismatched VID, PID,
or USB serial number. It then invokes PlatformIO's combined build/upload
target; build failure prevents flashing. USB identity identifies the adapter,
not the firmware or wiring. There is a small enumeration-to-open race: keep
the adapter connected and do not change connections during upload. No serial
monitor is started. The uploader normally resets the ESP32 afterward.

Only after approval for a new acquisition test:

```sh
# Open the port first, then explicitly pulse RTS/EN; GPIO0 stays deasserted.
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --duration 45 --reset
# Attach without requesting a reset (startup may already have passed):
python3 scripts/capture.py --duration 45
```

Default baud is fixed at 115200; duration accepts any finite positive seconds
(default 45). The monotonic deadline includes the reset pulse; read timeouts
are capped at 0.2 seconds and the remaining window. OS device open/close and
disk operations may add wall-clock overhead. Very short windows may miss
startup. Close other serial users first. Exclusive access is requested on
macOS/POSIX; do not open the alternate adapter alias concurrently.

Opening the port is not assumed to reset the board. DTR/RTS are set inactive
before opening, but USB drivers may briefly change control lines, so even a
capture without --reset can affect hardware. Do not use the real port for
infrastructure-only validation. --reset explicitly pulses RTS for up to 100 ms
after opening; test on this TTGO previously captured the full V4c startup.

Capture writes unfiltered bytes immediately to a unique, timestamped .txt
file in diagnostics/, with a separate .json sidecar reporting duration, reset
request, bytes, error, port closure, and observed boot/startup markers. Timestamps
include the host's local UTC offset. Startup is confirmed by V4c's first setup
checkpoint; inspect the remaining checkpoints for full setup completion.
No receive-buffer purge discards early messages. The port closes on timeout,
Ctrl-C, and Python read/reset errors. Abrupt process kill/power loss cannot
guarantee metadata, but raw bytes already flushed remain available. A startup
failure may produce only a boot trace; preserve it even without a FINAL record.

The analyser reads only and reports FINAL/timing/errors and first/last printed
CRC reports. A full pass requires both 4096/4096 processed and a STREAM PASS
result. CRC reports may precede the final frame; MOVING-mode CRC changes alone
do not establish corruption. Raw logs are authoritative, especially for
backtraces or messages the summary does not recognize.

diagnostics/ is ignored by default, including metadata and baseline archives.
Do not force-add logs without reviewing their contents and obtaining approval
for publication. Keep reviewed engineering summaries in docs/ENGINEERING_JOURNAL.md.

Hardware-free validation:

```sh
sh -n scripts/build.sh
python3 -m unittest discover -s scripts -p 'test_*.py'
```

The tests use a simulated port and clock; they do not connect/reset/flash TTGO.
The repository-root .github/workflows/platformio.yml builds and tests only; it does not upload
or capture. It will run after approved incorporation/publication to GitHub.

Portable helper example: `PIO=/path/to/pio sh scripts/build.sh`.
Use the same Python environment for capture/upload so pyserial is available.
