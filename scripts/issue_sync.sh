#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/issue_sync.sh <doc_path>

Creates or updates a GitHub Issue for an AR/ER/DR doc and keeps labels/status in sync.
The doc must contain a "GitHub-Issue:" front-matter line and a "- Status:" line.

Examples:
  scripts/issue_sync.sh docs/ER/ER-0009-Phase3-Vizier-Routing.md
  scripts/issue_sync.sh docs/AR/accepted/AR-0005-Exec-Runtime.md
EOF
}

if [[ $# -ne 1 ]]; then
  usage
  exit 1
fi

doc_path="$1"
if [[ ! -f "$doc_path" ]]; then
  echo "error: file not found: $doc_path"
  exit 1
fi

doc_type=""
case "$doc_path" in
  docs/AR/*) doc_type="ar" ;;
  docs/ER/*) doc_type="er" ;;
  docs/DR/*) doc_type="dr" ;;
  *)
    echo "error: document must be under docs/AR, docs/ER, or docs/DR"
    exit 1
    ;;
esac

title_line="$(grep -m1 -E '^# ' "$doc_path" || true)"
if [[ -z "$title_line" ]]; then
  echo "error: missing document title"
  exit 1
fi
title="${title_line#\# }"

status_line="$(grep -m1 -E '^- Status:' "$doc_path" || true)"
if [[ -z "$status_line" ]]; then
  echo "error: missing Status line"
  exit 1
fi
status_raw="${status_line#- Status: }"
status_raw="${status_raw%% *}"

issue_line="$(grep -m1 -E '^GitHub-Issue:' "$doc_path" || true)"
if [[ -z "$issue_line" ]]; then
  echo "error: missing GitHub-Issue line"
  exit 1
fi
issue_value="${issue_line#GitHub-Issue: }"

deps=()
deps_line="$(grep -m1 -E '^ER-Dependencies:' "$doc_path" || true)"
if [[ -n "$deps_line" ]]; then
  deps_raw="${deps_line#ER-Dependencies: }"
  IFS=',' read -ra deps_items <<< "$deps_raw"
  for item in "${deps_items[@]}"; do
    dep="$(echo "$item" | xargs)"
    if [[ -n "$dep" ]]; then
      deps+=("$dep")
    fi
  done
fi

map_status_label() {
  local status="$1"
  status="${status// /}"
  case "$status" in
    Draft|Proposed) echo "status:proposed" ;;
    Accepted|Approved) echo "status:accepted" ;;
    In|In-Progress|In_Progress|InProgress) echo "status:in-progress" ;;
    Implemented|Complete|Verified|Done) echo "status:done" ;;
    Rejected) echo "status:done" ;;
    *) echo "status:in-progress" ;;
  esac
}

status_label="$(map_status_label "$status_raw")"

issue_body="$(cat <<EOF
Doc: \`$doc_path\`

Status: $status_raw
EOF
)"
if [[ ${#deps[@]} -gt 0 ]]; then
  deps_joined="$(printf "%s, " "${deps[@]}")"
  deps_joined="${deps_joined%, }"
  issue_body="${issue_body}"$'\n'"ER-Dependencies: ${deps_joined}"
fi

ensure_labels() {
  local issue_number="$1"
  local labels=(
    "ar"
    "er"
    "dr"
    "status:proposed"
    "status:accepted"
    "status:in-progress"
    "status:done"
  )
  local remove_args=()
  for label in "${labels[@]}"; do
    remove_args+=(--remove-label "$label")
  done
  gh issue edit "$issue_number" "${remove_args[@]}" >/dev/null
  gh issue edit "$issue_number" --add-label "$doc_type" --add-label "$status_label" >/dev/null
}

er_verified() {
  local er_id="$1"
  local er_doc
  er_doc="$(ls "docs/ER/${er_id}-"*.md 2>/dev/null | head -n1 || true)"
  if [[ -z "$er_doc" ]]; then
    return 2
  fi
  local er_status_line
  er_status_line="$(grep -m1 -E '^- Status:' "$er_doc" || true)"
  if [[ -z "$er_status_line" ]]; then
    return 1
  fi
  local er_status
  er_status="${er_status_line#- Status: }"
  er_status="${er_status%% *}"
  if [[ "$er_status" == "Verified" ]]; then
    return 0
  fi
  return 1
}

close_or_reopen() {
  local issue_number="$1"
  if [[ "$doc_type" == "ar" && ${#deps[@]} -gt 0 ]]; then
    for dep in "${deps[@]}"; do
      er_verified "$dep"
      case $? in
        0) ;;
        1)
          gh issue reopen "$issue_number" >/dev/null
          return
          ;;
        2)
          echo "error: missing ER doc for dependency: $dep"
          exit 1
          ;;
      esac
    done
    gh issue close "$issue_number" --comment "Auto-closed by issue_sync: all ER dependencies verified." >/dev/null
    return
  fi
  if [[ "$doc_type" == "ar" && ${#deps[@]} -eq 0 ]]; then
    return
  fi
  if [[ "$status_label" == "status:done" ]]; then
    gh issue close "$issue_number" --comment "Auto-closed by issue_sync: status=$status_raw" >/dev/null
  else
    gh issue reopen "$issue_number" >/dev/null
  fi
}

if [[ "$issue_value" == "N/A" || -z "$issue_value" ]]; then
  issue_url="$(gh issue create --title "$title" --body "$issue_body" --label "$doc_type" --label "$status_label")"
  issue_number="${issue_url##*/}"
  perl -pi -e "s/^GitHub-Issue:.*/GitHub-Issue: #$issue_number/" "$doc_path"
  close_or_reopen "$issue_number"
  echo "created issue #$issue_number"
  exit 0
fi

if [[ "$issue_value" =~ ^#([0-9]+)$ ]]; then
  issue_number="${BASH_REMATCH[1]}"
else
  echo "error: GitHub-Issue line must be N/A or #<number>"
  exit 1
fi

gh issue edit "$issue_number" --title "$title" --body "$issue_body" >/dev/null
ensure_labels "$issue_number"
close_or_reopen "$issue_number"
echo "updated issue #$issue_number"
