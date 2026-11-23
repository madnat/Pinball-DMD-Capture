#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
exec "${PIO:-pio}" run -e "${1:-ttgo_tdisplay}"
