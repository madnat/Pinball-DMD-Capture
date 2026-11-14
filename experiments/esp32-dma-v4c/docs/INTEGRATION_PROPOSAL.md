> Copied from the original V4c workspace as historical infrastructure documentation.
> Current repository layout/status: ../../../docs/DEVELOPMENT_AUTOMATION_REVIEW.md.
> Prior raw logs and backup archives remain in the original V4c folder; they are not copied here.

# Proposed GitHub incorporation — no repository changes performed

Discovery: 2026-11-14, Europe/London. Both supplied paths exist on this Mac:

- original-local-workspaces/ESP32_DMA_Investigation_V4c
- original-local-workspaces/Pinball-DMD-Capture

Path.resolve() succeeds with the supplied spelling. PlatformIO/Platformio case
differs in the supplied paths; use the actual verified paths and quoted absolute
paths rather than assuming another machine's filesystem case behavior.

V4c has no .git repository or remote tracking; it has no commit ancestry to
merge/cherry-pick. The managed project has a clean tracked/untracked status and
origin=https://github.com/madnat/Pinball-DMD-Capture.git.

## Branch evidence

Read-only `git ls-remote --heads origin` confirmed the same heads as the local
tracking refs, without fetching or updating Git state:

| Ref | Commit | State |
| --- | --- | --- |
| feature/v1.2a-acquisition and origin/feature/v1.2a-acquisition | 8a8d0d10cccab54e51a6b5432fbf01a322859b90 | checked out, clean, tracking equal |
| origin/main | 763c700b9e2f95f57269ebde4abc97e706626c95 | current remote main |
| local main | 4b23094 | 8 commits behind origin/main |

`git rev-list --left-right --count origin/main...feature/v1.2a-acquisition`
returned 7 / 1: feature is seven commits behind and one ahead of origin/main.
History includes a prior merge of this feature branch; its current tip remains
distinct. Do not infer that a previously merged branch is now synchronized.
No reset, checkout, fetch, merge, commit, push or history rewrite was performed.

## Content differences and likely conflicts

| Path | V4c | Managed feature branch / integration concern |
| --- | --- | --- |
| src/main.cpp | 22,249-byte DMA V4c; MOVING mode; SPI slave, eight buffers, PCNT, 4096-frame target | 4,695-byte V1.2g CPU timing-gap diagnostic; no DMD acquisition. Same destination, incompatible experiment; never overwrite as infrastructure work. |
| platformio.ini | ttgo_tdisplay, espressif32@6.3.2, esp32dev, Arduino, 115200, no TFT dependency | Same environment/platform/board/framework/speeds, plus TFT_eSPI@2.5.43 and ST7789/pin/font build flags. Preserve both configurations. |
| README.md | V4c documentation, now linked to automation/journal | No tracked root README.md on feature; new documentation needs review for canonical project scope. |
| .gitignore | Existing PlatformIO exclusions plus diagnostics and Python bytecode | Existing tracked ignore file needs additive reconciliation, not replacement. |
| AGENTS.md, scripts/, .github/workflows/platformio.yml | New infrastructure | Not tracked in feature; copy only reviewed infrastructure on a proposed branch. Hardware helpers target this Mac; CI uses portable pio command. |
| docs/journal/ | V4c uses new Markdown journal in docs/ | Feature retains three Word documents; preserve them. Main additionally has Williams_Babcock_DMD_Capture_Investigation_V1.pdf. |
| .vscode/ and include/, lib/, test/ | V4c has local editor files | Feature tracks extensions.json and include/lib/test README placeholders; preserve project assets and avoid copying generated editor state. |

Between feature and origin/main, the tip-to-tip differences are src/main.cpp
and the PDF present only on main. Main's source is V1.2a RCLK hardware counter,
not V4c or the feature's V1.2g diagnostic. A future merge of the divergent
branches may conflict in src/main.cpp; this report identifies content conflicts,
not a tested merge result. Do not automatically resolve by choosing either side.

## Preservation already prepared

Ignored local archive:
diagnostics/baselines/v4c_20261114_before_automation.zip contains the original
V4c source, platformio.ini, pre-automation README and the complete existing raw
diagnostic log. Hashes are in docs/ENGINEERING_JOURNAL.md. Source, configuration
and original log remain byte-for-byte unchanged. This is a local backup, not
an off-machine backup; keep an approved additional copy before future migration.
The managed repository remains untouched; its existing committed baselines
and documents remain available at the recorded refs.

## Confirmed separation decision — 2026-11-14

The user confirmed that V4c and v1.2a-acquisition are different tests and must
be maintained separately within the repository. Preserve each as an independent
PlatformIO project with its own source, configuration, documentation and test
evidence. Do not combine their acquisition logic or replace one with the other.
Separate branches alone are not the intended long-term experiment catalogue;
use distinct project directories when incorporating both into a common tree.

The proposed V4c destination is experiments/ESP32_DMA_Investigation_V4c/.
Keep existing root firmware untouched during its addition. Any later relocation
of the older tests needs a separate reviewed change. Name older experiment
directories by their actual firmware identity: the current feature branch tip
contains V1.2g CPU timing diagnostics despite its v1.2a-acquisition branch name;
origin/main contains the V1.2a RCLK counter. Preserve both identities/history.
CI should build each incorporated project separately with its own configuration.

This confirms preservation/layout intent; no files have been incorporated into
the managed repository, and no branch switch, push or merge is authorized by
this documentation update.

## Safe next steps requiring separate authorization

1. Recheck cleanliness and remote heads immediately before integration. Back up
   the managed working directory including untracked files, and save a Git
   bundle containing all refs outside the working tree. Preserve the V4c archive
   separately and verify its hashes. Do not discard any unexpected local changes.
2. Obtain approval to create/switch to a new infrastructure branch (or isolated
   worktree) from current origin/main. Avoid merging the old feature branch as
   part of automation setup. Its unique work can remain preserved at its tip.
3. Copy reviewed scripts, AGENTS.md and workflow, add documentation, and merge
   ignore rules additively. Adapt guidance to distinguish the canonical root
   firmware from the preserved V4c experiment. Keep src/main.cpp and the root
   platformio.ini untouched in this infrastructure-only change.
4. Build the root ttgo_tdisplay environment on that branch and validate scripts
   without hardware. The prepared Actions workflow will build whichever source
   occupies the repository root, not automatically the archived V4c experiment.
5. Incorporate V4c as a separate reviewed experiment addition:
   add a complete standalone experiments/ESP32_DMA_Investigation_V4c project
   (source + its own configuration and evidence), preserving root firmware.
   Run its build explicitly via `pio run -d experiments/ESP32_DMA_Investigation_V4c
   -e ttgo_tdisplay` and add a separate CI job only once that layout is approved.
   No acquisition-logic edits are needed to preserve the experiment.
6. Review the exact diff and engineering notes; obtain explicit approval before
   pushing or merging. GitHub Actions execution remains unverified until an
   approved push/PR or workflow dispatch. No hardware upload is part of CI.

No repository-write permission or remote publishing authorization has been
requested for this proposal. Editing the managed folder is outside the current
workspace write roots and would require expanded filesystem permission.
