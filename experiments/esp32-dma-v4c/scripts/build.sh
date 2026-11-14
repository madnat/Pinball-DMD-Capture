#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
exec "${PIO:-pio}" run -e ttgo_tdisplay
