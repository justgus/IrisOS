#!/usr/bin/env bash
# Prints IrisOS Airframe workspace context, canonical diagnostics, and project summary.
set -euo pipefail
source "$(dirname "$0")/_ac_common.sh"

ac_run context
echo ""
ac_run_backend state diagnostics --output markdown
echo ""
ac_run_backend project summary --output markdown
