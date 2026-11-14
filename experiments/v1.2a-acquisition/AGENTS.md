# Independent branch experiment

Follow the repository root AGENTS.md. Preserve this source as a byte-identical
snapshot of feature/v1.2a-acquisition at 8a8d0d1. platformio.ini retains the
original settings plus the authorized lib_ldf_mode=deep+ build-discovery fix;
original and current configuration hashes are recorded in the baseline manifest.
Its firmware is V1.2g CPU timing diagnostics, not V4c acquisition. Never combine
or replace these experiments. Build only with ttgo_tdisplay in this directory.
Obtain explicit approval before hardware capture/reset/upload, pushing, merging,
deleting files or changing branches. Log each approved hardware test in an
engineering journal with source/configuration hashes and full raw output.
Infrastructure-only tasks must not alter timing or acquisition firmware.
