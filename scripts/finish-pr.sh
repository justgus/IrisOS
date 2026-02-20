#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 \"<commit message>\""
  exit 1
fi

msg="$1"

branch="$(git rev-parse --abbrev-ref HEAD)"
if [[ "$branch" == "main" ]]; then
  echo "error: refusing to run on main"
  exit 1
fi

if ! git diff --quiet || ! git diff --cached --quiet; then
  git add -A
  git commit -m "$msg"
fi

git push -u origin "$branch"

gh pr create --base main --head "$branch" --fill
gh pr checks --watch
gh pr merge --merge

git checkout main
git branch -d "$branch"
git push origin --delete "$branch"
git fetch origin main
git merge --ff-only origin/main
