#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <topic>"
  exit 1
fi

topic="$1"
branch="$topic"
if [[ "$topic" != codex/* ]]; then
  branch="codex/$topic"
fi

git checkout -b "$branch"
