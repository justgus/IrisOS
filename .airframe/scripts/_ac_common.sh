#!/usr/bin/env bash
# Shared helpers for IrisOS Agile Airframe scripts.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
AICOCKPIT="$REPO_ROOT/../Airframe/demos/LiveDemo/bin/aicockpit"
AC_CONFIG="$REPO_ROOT/.airframe/airframe-workspace.json"
AC_BACKEND="${AC_BACKEND:-canonical}"

if [[ ! -x "$AICOCKPIT" ]]; then
  echo "error: aicockpit binary not found at $AICOCKPIT" >&2
  echo "Run from the sibling Airframe checkout: scripts/install-live-demo.sh" >&2
  exit 1
fi

if [[ ! -f "$AC_CONFIG" ]]; then
  echo "error: airframe config not found at $AC_CONFIG" >&2
  exit 1
fi

ac_run() {
  "$AICOCKPIT" "$@" --config "$AC_CONFIG"
}

ac_run_backend() {
  "$AICOCKPIT" "$@" --config "$AC_CONFIG" --backend "$AC_BACKEND"
}
