#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
action="${1:-gmake}"

case "${action}" in
    gmake) ;;
    *) echo "Unsupported Premake action '${action}'. Use gmake." >&2; exit 2 ;;
esac

premake_path="$("${root_dir}/Scripts/Setup.sh" --print-premake-path)"
"${premake_path}" --file="${root_dir}/premake5.lua" "${action}"
