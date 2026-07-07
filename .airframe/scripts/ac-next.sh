#!/usr/bin/env bash
# Prints the next active IrisOS Airframe task.
set -euo pipefail
source "$(dirname "$0")/_ac_common.sh"

ac_run_backend task next --output markdown
