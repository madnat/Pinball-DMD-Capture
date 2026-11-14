# Development automation review — 2026-11-14

## Scope and original folders

Original V4c: original-local-workspaces/ESP32_DMA_Investigation_V4c
Original managed checkout: original-local-workspaces/Pinball-DMD-Capture
Original local paths were verified; workstation-specific absolute paths are
intentionally omitted from this repository copy. The labels above identify
external originals, not paths inside this repository. Neither original folder's firmware or documentation was
changed by this review. V4c remains outside Git; its diagnostics and local backup
archive stay intact. The original managed checkout stays clean on
feature/v1.2a-acquisition at 8a8d0d1, tracking the matching remote branch.

Existing V4c infrastructure inspected:
- AGENTS.md
- scripts/build.sh, upload.py, capture.py, analyse_log.py, test_automation.py
- .github/workflows/platformio.yml
- README.md, docs/AUTOMATION.md, docs/ENGINEERING_JOURNAL.md,
  docs/INTEGRATION_PROPOSAL.md; .gitignore excludes raw diagnostics.
- diagnostics/ttgo_serial_20261114_130746.txt and
  diagnostics/baselines/v4c_20261114_before_automation.zip remain local.

The original managed checkout has no AGENTS.md, automation shell/Python scripts,
GitHub Actions workflow or Markdown documentation. Its existing journal documents
remain tracked under docs/journal/; no documents were moved or removed.

## Git state and prepared worktree

New worktree:
Pinball-DMD-Capture-automation
New branch: feature/separate-experiments-automation
Base and HEAD: origin/main at 763c700b9e2f95f57269ebde4abc97e706626c95.
No upstream is configured for the new unpublished branch, avoiding accidental
tracking of origin/main. No commits have been performed; the 29-file allowlist is staged for user review. Root src/main.cpp
is unchanged from this base. Root platformio.ini adds the authorized deep+
dependency-discovery fix described below. Existing refs/history were
preserved; worktree registration and the requested new branch are the only
intentional Git metadata additions.

Origin: https://github.com/madnat/Pinball-DMD-Capture.git
Read-only ls-remote confirmed remote heads match local tracking refs:
- main: 763c700b9e2f95f57269ebde4abc97e706626c95
- feature/v1.2a-acquisition: 8a8d0d10cccab54e51a6b5432fbf01a322859b90
The existing feature branch is 1 ahead / 7 behind origin/main. Existing local
main at 4b23094 is 8 behind origin/main. No fetch, reset, rebase or merge occurred.

## Independent repository layout

```text
AGENTS.md
README.md
.github/workflows/platformio.yml
docs/
  DEVELOPMENT_AUTOMATION_REVIEW.md
  EXPERIMENT_BASELINES.json
  READY_FOR_GITHUB.txt
src/main.cpp                        # unchanged root V1.2a RCLK counter
platformio.ini                      # root configuration + deep+ build fix
experiments/
  v1.2a-acquisition/
    AGENTS.md
    README.md
    .gitignore
    platformio.ini                  # feature configuration + deep+ build fix
    src/main.cpp                    # exact feature-branch source
    scripts/{build.sh,upload.py,capture.py}
  esp32-dma-v4c/
    AGENTS.md
    README.md
    .gitignore
    platformio.ini                  # exact original V4c configuration
    src/main.cpp                    # exact original V4c source
    scripts/{build.sh,upload.py,capture.py,analyse_log.py,test_automation.py}
    docs/{AUTOMATION.md,ENGINEERING_JOURNAL.md,INTEGRATION_PROPOSAL.md}
```

v1.2a-acquisition identifies branch provenance; its current source is V1.2g CPU
timing-gap diagnostics. Root main is V1.2a RCLK hardware counting. V4c is a
separate SPI slave DMA experiment whose previous recorded test failed early.
No firmware was merged, renamed, moved, rewritten or substituted.
Original baseline SHA-256 values and provenance are in EXPERIMENT_BASELINES.json;
its configuration_override fields separately record the two authorized worktree
configuration hashes. The original provenance values were not replaced.

Each experiment has an independent build helper, verified explicit upload helper
and bounded raw capture helper. Copies resolve their own project roots. V1.2g's
capture startup marker was adapted to its own banner in the script only; its
acquisition/timing source was not edited. Only V4c includes its log analyser,
since its FINAL/CRC semantics do not apply to the CPU timing diagnostic.
Raw captures/build outputs/bytecode remain ignored. No raw logs or local archives
are included among the proposed Git changes. Copied V4c documents are annotated
as historical notes; this review supersedes their old repository-layout proposal.

## Validation and resolved build issues

- Python syntax and shell syntax passed for all copied automation helpers.
- --help passed for each helper. Both captures reject 0, -1, nan, inf and invalid
  durations before serial access. Both upload helpers reject missing explicit
  --confirm-upload before serial access.
- Six hardware-free unit tests passed for capture deadlines, byte preservation,
  port closure on open/read/reset errors and interruption, reset deassertion,
  duration checks, USB identity refusal and V4c log analysis.
- All firmware source hashes and the V4c configuration match their original
  baseline. The two authorized configuration overrides are recorded separately.
- Ignore rules verified for both experiments' raw diagnostics and build outputs.
- Workflow YAML parsed and its three-project matrix verified. Each job uses its
  own PlatformIO directory; fail-fast is disabled. No flash/capture steps exist.
- Initial failures reproduced in both root and v1.2a with complete verbose
  compiler output preserved locally. Both configs declare TFT_eSPI@2.5.43 and
  use default chain LDF; the sketches do not include TFT headers. TFT_eSPI's
  manifest does not declare SPI, and chain scanning did not discover its SPI
  include dependency. The installed framework contains libraries/SPI/src/SPI.h,
  but the failing TFT compiler command had no -I path for that directory.
- Fix: add only lib_ldf_mode = deep+ (with an explanatory comment) to root and
  experiments/v1.2a-acquisition/platformio.ini. Keep TFT version, flags, framework,
  board and platform unchanged. Do not add absolute include paths or a registry
  replacement for the framework SPI library. V4c declares no TFT dependency,
  needs no SPI library header, and builds with its unchanged chain configuration.
- After the fix, verbose dependency graphs show TFT_eSPI -> SPI@2.0.0 and the
  compiler command includes framework-arduinoespressif32/libraries/SPI/src.
  All three explicit clean targets and subsequent builds exited 0 using the
  CI-pinned PlatformIO Core 6.1.13 / Python 3.12 / espressif32@6.3.2 setup.

| Project | Clean build | RAM bytes | Flash bytes |
| --- | --- | ---: | ---: |
| Root | PASS | 22,424 | 268,593 |
| experiments/v1.2a-acquisition | PASS | 23,480 | 264,697 |
| experiments/esp32-dma-v4c | PASS | 24,172 | 286,689 |

Full before/after verbose and clean logs are ignored local evidence under
`diagnostics/build-validation/` (root-before-verbose.log, v1.2a-before-verbose.log,
root-clean.log, v1.2a-clean.log, v4c-clean.log, root-after-verbose.log,
v1.2a-after-verbose.log, v4c-after-verbose.log). No compiler errors or compiler
warnings were found in the final three build logs. The existing PlatformIO
Core-version notice remains informational.

- PlatformIO Core 6.1.13 warns a newer 6.2.0 was previously used. No core upgrade
  or Codex user-level configuration change was made. TFT_eSPI was installed by
  PlatformIO into the worktree's ignored per-project .pio/libdeps for validation.
- No serial port was opened, board reset, firmware uploaded or acquisition test
  started. Real-device script operation is intentionally not revalidated here.
- Workflow execution on GitHub remains untested; nothing was committed/pushed.

## Review-ready files and next steps

READY_FOR_GITHUB.txt is the exact list of added/modified candidate files,
excluding generated artifacts and unchanged existing repository files. All
changes are staged from the explicit allowlist in this isolated worktree. The three local clean builds and six hardware-free tests now pass. Remote CI
execution remains pending an approved push; the original checkout and V4c
folder remain intact.

Example build commands from this worktree:

```sh
sh experiments/esp32-dma-v4c/scripts/build.sh
sh experiments/v1.2a-acquisition/scripts/build.sh
python3 -m unittest discover -s experiments/esp32-dma-v4c/scripts -p 'test_*.py'
```

Helpers use python3 and pio from PATH; build/upload accept a PIO executable
override through the environment. CI uses its pinned Python/PlatformIO setup. Hardware actions require separate approval.
Obtain explicit approval before committing, pushing, merging, relocating/removing
original files, or accessing hardware. No Git history rewrite is needed.

## Exact files changed for the build-fix follow-up

- platformio.ini: deep+ dependency discovery and comment.
- experiments/v1.2a-acquisition/platformio.ini: same build-only fix.
- .github/workflows/platformio.yml: explicitly clean each matrix project before building.
- AGENTS.md and experiments/v1.2a-acquisition/AGENTS.md: document the authorized
  configuration exceptions while retaining immutable firmware provenance.
- README.md and experiments/v1.2a-acquisition/README.md: correct configuration
  preservation claims and explain the fix.
- docs/EXPERIMENT_BASELINES.json: retain original hashes and record override hashes.
- docs/DEVELOPMENT_AUTOMATION_REVIEW.md: actual diagnosis and validation results.
- docs/READY_FOR_GITHUB.txt: regenerate the exact candidate file list.

No automation helper code, source snapshots, V4c configuration, original folders,
Git commits/history or hardware state were changed by this follow-up.

## Pre-publication review

The 29 manifest entries were reviewed, including all untracked files. Firmware
sources remain byte-identical to provenance; configuration overrides retain
separate hashes. The staged allowlist must exactly match READY_FOR_GITHUB.txt.
No commit or push is authorized until the user approves the staged result.

Repository copies now omit personal absolute paths and use portable Python
shebangs and PATH-based pio commands. Set `PIO=/path/to/pio` for build/upload
if PlatformIO is not on PATH; run Python helpers in the environment containing
pyserial. Firmware pins and the explicitly verified TTGO USB serial identity
are intentional project hardware configuration, not workstation credentials.
The original local project folders and helpers remain unchanged.

Local diagnostics, .pio artifacts, Python bytecode, workstation settings and
credential-file patterns are excluded. Existing already-tracked repository
files are preserved; no editor or machine-local files are included in the new
staged changes. Candidate text is checked for common private-key/token formats
and personal absolute paths in addition to manual review; this is not a proof
that arbitrary undocumented secrets cannot exist.

Remaining limits: GitHub-hosted CI has not run yet; hardware helper operation
has not been revalidated against the board. V4c's recorded acquisition failure
remains a separate known experiment result, unaffected by this infrastructure.

Publication-cleanup validation: all six existing hardware-free tests passed
again. Portable build helpers for both experiments succeeded with an explicit
PIO environment override. Helper CLI guards, syntax, YAML/matrix checks,
manifest accuracy, provenance hashes and common-secret/personal-path checks
passed. Explicit staging contains only the 29 manifest entries; commit and push
remain pending user approval. Intended commit message:
`Add separate acquisition experiments and automated PlatformIO workflows`.

Staged whitespace review: Git reports one inherited blank line at EOF in the
byte-preserved V4c platformio.ini. It is retained to preserve the recorded
configuration checksum; all other whitespace checks pass.
