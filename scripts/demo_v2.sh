#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
conch="${CONCH:-${repo_root}/bin/conch}"

if [[ ! -x "${conch}" ]]; then
  echo "error: ${conch} not found; build bin/conch first" >&2
  exit 1
fi

workdir="${1:-$(mktemp -d /tmp/iris-v2-demo-XXXXXX)}"
mkdir -p "${workdir}"

producer_db="${workdir}/producer.db"
consumer_db="${workdir}/consumer.db"
bundle_path="${workdir}/v2-demo.bundle"

rm -f "${producer_db}" "${producer_db}-shm" "${producer_db}-wal"
rm -f "${consumer_db}" "${consumer_db}-shm" "${consumer_db}-wal"
rm -rf "${producer_db}.segments" "${consumer_db}.segments"
rm -f "${bundle_path}"

producer_script="$(cat <<EOF
define type --json '{"name":"Widget","namespace":"DemoV2","fields":[{"name":"title","type":"String","required":true},{"name":"enabled","type":"Bool","required":false}]}'
let widget=new --json '{"type":"DemoV2::Widget","payload":{"title":"alpha","enabled":true}}'
show widget
demo v1
task list
task profile
task trace
bundle export ${bundle_path}
exit
EOF
)"

producer_output="$(printf '%s\n' "${producer_script}" | "${conch}" --db "${producer_db}")"
printf '== producer ==\n%s\n' "${producer_output}"

widget_id="$(printf '%s\n' "${producer_output}" | awk '/^widget = / { print $3; exit }')"
summary_id="$(printf '%s\n' "${producer_output}" | awk '/^summary / { print $2; exit }')"

if [[ -z "${widget_id}" || -z "${summary_id}" ]]; then
  echo "error: failed to extract widget or summary id from producer output" >&2
  exit 1
fi

consumer_script="$(cat <<EOF
bundle import ${bundle_path}
show ${widget_id}
debug graph ${summary_id}
debug dispatch Demo::Summary expand U64
show type DemoV2::Widget
exit
EOF
)"

consumer_output="$(printf '%s\n' "${consumer_script}" | "${conch}" --db "${consumer_db}")"
printf '== consumer ==\n%s\n' "${consumer_output}"

printf 'DEMO_V2_OK workdir=%s widget=%s summary=%s bundle=%s\n' \
  "${workdir}" "${widget_id}" "${summary_id}" "${bundle_path}"
