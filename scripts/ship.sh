#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 <topic> \"<commit message>\" [\"<pr title>\"]"
  exit 1
}

if [[ $# -lt 2 || $# -gt 3 ]]; then
  usage
fi

topic="$1"
msg="$2"
pr_title="${3:-$msg}"

normalize_branch() {
  local name="$1"
  if [[ "$name" == codex/* ]]; then
    printf '%s\n' "$name"
  else
    printf 'codex/%s\n' "$name"
  fi
}

current_branch="$(git rev-parse --abbrev-ref HEAD)"
target_branch="$(normalize_branch "$topic")"

if [[ "$current_branch" == "main" ]]; then
  git checkout -b "$target_branch"
  current_branch="$target_branch"
fi

if [[ "$current_branch" == "HEAD" ]]; then
  echo "error: detached HEAD is not supported"
  exit 1
fi

if [[ "$current_branch" != "$target_branch" ]]; then
  echo "info: using current branch '$current_branch' instead of requested '$target_branch'"
fi

has_untracked=0
if [[ -n "$(git ls-files --others --exclude-standard)" ]]; then
  has_untracked=1
fi

if git diff --quiet && git diff --cached --quiet && [[ "$has_untracked" -eq 0 ]]; then
  echo "error: no changes to commit"
  exit 1
fi

git add -A
git commit -m "$msg"
git push -u origin "$current_branch"

pr_url="$(gh pr view --json url --jq .url 2>/dev/null || true)"
if [[ -z "$pr_url" ]]; then
  gh pr create --base main --head "$current_branch" --title "$pr_title" --body "$msg"
else
  echo "info: using existing PR $pr_url"
fi

gh pr checks --watch
gh pr merge --merge --delete-branch

git checkout main
git fetch origin main
git merge --ff-only origin/main

if git show-ref --verify --quiet "refs/heads/$current_branch"; then
  git branch -d "$current_branch"
fi
