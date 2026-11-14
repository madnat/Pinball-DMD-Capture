# Pinball DMD Capture — independent development experiments

The original root firmware source is preserved. Root and the older experiment
add deep+ dependency discovery to their configurations; see the review for the
SPI.h build fix and configuration provenance. Each experiment has its own
source and platformio.ini; no acquisition firmware is merged or moved.

| Project | Actual firmware | Provenance |
| --- | --- | --- |
| . | V1.2a RCLK hardware counter | origin/main at 763c700 |
| experiments/v1.2a-acquisition | V1.2g CPU timing-gap diagnostic | feature/v1.2a-acquisition at 8a8d0d1 |
| experiments/esp32-dma-v4c | V4c SPI slave DMA experiment | Original local V4c workspace, preserved by SHA-256 |

Build one project with `pio run -d <directory> -e ttgo_tdisplay`.
See [automation review](docs/DEVELOPMENT_AUTOMATION_REVIEW.md),
[baseline manifest](docs/EXPERIMENT_BASELINES.json), and each experiment's README.
All hardware operations require separate authorization. CI builds each project
independently and runs hardware-free automation tests.
