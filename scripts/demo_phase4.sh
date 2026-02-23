#!/usr/bin/env bash
set -euo pipefail

db_path="${1:-/tmp/iris_demo.db}"
rm -f "${db_path}" "${db_path}-shm" "${db_path}-wal"

output=$(printf "let demo=new Demo::PropulsionSynth name:=PropulsionSynth\nstart demo\nexit\n" | ./bin/conch --db "${db_path}")
summary_id=$(printf "%s\n" "${output}" | awk '/^summary / {print $2; exit}')

echo "${output}"

if [[ -z "${summary_id}" ]]; then
  echo "error: failed to extract demo summary id" >&2
  exit 1
fi

printf "call %s expand 1\nexit\n" "${summary_id}" | ./bin/conch --db "${db_path}"
