#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${script_dir}/../DependencyLock.sh"

temporary_root="$(mktemp -d)"
trap 'rm -rf -- "${temporary_root}"' EXIT
test_count=0
sha="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
entry="premake|tool|linux-x64|1.0|BSD-3-Clause|https://example.com/premake.tar.gz|${sha}|premake5"

assert_parse_succeeds() {
    local name="$1"
    local content="$2"
    local expected_count="$3"
    local path="${temporary_root}/${name}.lock"

    ((test_count += 1))
    printf '%b' "${content}" > "${path}"
    read_project_dependency_lock "${path}"
    if [[ ${project_dependency_count} -ne ${expected_count} ]]; then
        echo "${name} expected ${expected_count} dependencies, received ${project_dependency_count}." >&2
        exit 1
    fi
}

assert_parse_fails() {
    local name="$1"
    local content="$2"
    local expected_message="$3"
    local path="${temporary_root}/${name}.lock"
    local output

    ((test_count += 1))
    printf '%b' "${content}" > "${path}"
    if output="$(read_project_dependency_lock "${path}" 2>&1)"; then
        echo "${name} unexpectedly parsed successfully." >&2
        exit 1
    fi
    if [[ "${output}" != *"${expected_message}"* ]]; then
        echo "${name} failed with an unexpected message: ${output}" >&2
        exit 1
    fi
}

assert_parse_succeeds 'valid-bom-crlf' "\xEF\xBB\xBFPROJECT_TEMPLATE_DEPENDENCIES_V1\r\n# comment\r\n\r\n${entry}\r\n" 1
assert_parse_succeeds 'case-sensitive-keys' "PROJECT_TEMPLATE_DEPENDENCIES_V1\n${entry}\nPremake|tool|linux-x64|1.0|BSD-3-Clause|https://example.com/other.tar.gz|${sha}|other\n" 2

assert_parse_fails 'missing-schema' '# comment' 'does not contain schema'
assert_parse_fails 'unsupported-schema' "PROJECT_TEMPLATE_DEPENDENCIES_V2\n${entry}" 'Unsupported dependency lock schema'
assert_parse_fails 'wrong-field-count' "PROJECT_TEMPLATE_DEPENDENCIES_V1\n${entry}|extra" 'exactly eight'
assert_parse_fails 'empty-field' "PROJECT_TEMPLATE_DEPENDENCIES_V1\npremake|tool||1.0|BSD-3-Clause|https://example.com/premake.tar.gz|${sha}|premake5" 'empty field'
assert_parse_fails 'invalid-kind' "PROJECT_TEMPLATE_DEPENDENCIES_V1\npremake|library|linux-x64|1.0|BSD-3-Clause|https://example.com/premake.tar.gz|${sha}|premake5" 'unknown kind'
assert_parse_fails 'invalid-sha' 'PROJECT_TEMPLATE_DEPENDENCIES_V1\npremake|tool|linux-x64|1.0|BSD-3-Clause|https://example.com/premake.tar.gz|invalid|premake5' 'invalid SHA-256'
assert_parse_fails 'non-https-url' "PROJECT_TEMPLATE_DEPENDENCIES_V1\npremake|tool|linux-x64|1.0|BSD-3-Clause|http://example.com/premake.tar.gz|${sha}|premake5" 'absolute HTTPS URL'
assert_parse_fails 'whitespace-url' "PROJECT_TEMPLATE_DEPENDENCIES_V1\npremake|tool|linux-x64|1.0|BSD-3-Clause|https://example.com/premake file.tar.gz|${sha}|premake5" 'absolute HTTPS URL'
assert_parse_fails 'rooted-entry' "PROJECT_TEMPLATE_DEPENDENCIES_V1\npremake|tool|linux-x64|1.0|BSD-3-Clause|https://example.com/premake.tar.gz|${sha}|/bin/premake5" 'unsafe installed entry'
assert_parse_fails 'parent-entry' "PROJECT_TEMPLATE_DEPENDENCIES_V1\npremake|tool|linux-x64|1.0|BSD-3-Clause|https://example.com/premake.tar.gz|${sha}|bin/../premake5" 'unsafe installed entry'
assert_parse_fails 'duplicate-key' "PROJECT_TEMPLATE_DEPENDENCIES_V1\n${entry}\n${entry}" 'duplicates'

printf 'Passed %d dependency lock parser tests.\n' "${test_count}"
