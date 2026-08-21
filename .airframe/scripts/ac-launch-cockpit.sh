#!/usr/bin/env bash
# Launches the project-local AgileCockpit app with the IrisOS Airframe workspace config.
set -euo pipefail
source "$(dirname "$0")/_ac_common.sh"

APP="$REPO_ROOT/../Airframe/demos/LiveDemo/Applications/AgileCockpit.app"

if [[ ! -d "$APP" ]]; then
  echo "error: AgileCockpit.app not found at $APP" >&2
  echo "Run from the sibling Airframe checkout: scripts/install-live-demo.sh" >&2
  exit 1
fi

echo "Launching AgileCockpit with IrisOS Airframe workspace config..."
open -n --fresh \
  --env "AIRFRAME_CONFIG_PATH=$AC_CONFIG" \
  "$APP"
