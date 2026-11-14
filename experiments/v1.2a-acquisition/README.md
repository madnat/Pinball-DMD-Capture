# v1.2a-acquisition branch experiment

Independent snapshot of feature/v1.2a-acquisition at
8a8d0d10cccab54e51a6b5432fbf01a322859b90. Its actual firmware banner is
PINBALL DMD V1.2g: CPU TIMING GAP DIAGNOSTIC. This is an 80 ms CPU timing
measurement with interrupts disabled, not the V1.2a RCLK counter on main and
not the DMA acquisition experiment. The directory name identifies branch
provenance; do not relabel this source as a completed acquisition test.

Source is copied byte-for-byte. platformio.ini retains the original settings
and adds lib_ldf_mode=deep+ so retained TFT_eSPI sources resolve framework SPI
even though the sketch does not include TFT headers. TFT dependencies/build
flags remain intact. Original/current configuration hashes are recorded in
../../docs/EXPERIMENT_BASELINES.json. Root firmware source and the original
branch are preserved separately.
See ../../docs/DEVELOPMENT_AUTOMATION_REVIEW.md for provenance and validation.

Build: `sh scripts/build.sh` from this project directory.
The upload helper requires --port and --confirm-upload and verifies CP2104
10C4:EA60 serial 02073BE4. Obtain explicit approval before flashing.
The capture helper defaults to 45 seconds at 115200; --reset explicitly pulses
RTS after opening. It writes raw bytes and JSON metadata to this project's
ignored diagnostics/. Startup detection uses the PINBALL DMD V1.2g banner.
Opening a port can affect control lines; authorization for a new hardware test
is required even without --reset. No hardware validation was performed here.
The V4c analyser is deliberately not included: these logs have a different
format. Review their raw timing output without applying V4c pass criteria.
