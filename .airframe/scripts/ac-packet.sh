#!/usr/bin/env bash
# Prints a compact task packet for a canonical IrisOS Airframe task.
set -euo pipefail
source "$(dirname "$0")/_ac_common.sh"

if [[ $# -ne 1 ]]; then
  echo "usage: $0 T-XXXX" >&2
  exit 2
fi

ac_run_backend task packet "$1" --output markdown
