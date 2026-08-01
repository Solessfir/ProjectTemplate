#!/usr/bin/env bash

project_dependency_lock_schema="PROJECT_TEMPLATE_DEPENDENCIES_V1"

read_project_dependency_lock() {
    local lock_path="$1"

    if [[ ! -f "${lock_path}" ]]; then
        echo "Dependency lock file not found: ${lock_path}" >&2
        return 1
    fi

    project_dependency_names=()
    project_dependency_kinds=()
    project_dependency_platforms=()
    project_dependency_versions=()
    project_dependency_licenses=()
    project_dependency_urls=()
    project_dependency_sha256s=()
    project_dependency_installed_entries=()
    project_dependency_count=0

    local -A dependency_keys=()
    local schema_found=false
    local line_number=0
    local raw_line line pipe_characters name kind platform version license url sha256 installed_entry field key

    while IFS= read -r raw_line || [[ -n "${raw_line}" ]]; do
        ((line_number += 1))
        line="${raw_line%$'\r'}"
        if [[ ${line_number} -eq 1 ]]; then
            line="${line#$'\xEF\xBB\xBF'}"
        fi

        if [[ -z "${line}" || "${line}" == \#* ]]; then
            continue
        fi

        if [[ "${schema_found}" == false ]]; then
            if [[ "${line}" != "${project_dependency_lock_schema}" ]]; then
                echo "Unsupported dependency lock schema '${line}' at line ${line_number}." >&2
                return 1
            fi
            schema_found=true
            continue
        fi

        pipe_characters="${line//[^|]/}"
        if [[ ${#pipe_characters} -ne 7 ]]; then
            echo "Dependency line ${line_number} must contain exactly eight pipe-delimited fields." >&2
            return 1
        fi

        IFS='|' read -r name kind platform version license url sha256 installed_entry <<< "${line}"
        for field in "${name}" "${kind}" "${platform}" "${version}" "${license}" "${url}" "${sha256}" "${installed_entry}"; do
            if [[ -z "${field}" ]]; then
                echo "Dependency line ${line_number} contains an empty field." >&2
                return 1
            fi
        done

        if [[ "${kind}" != tool && "${kind}" != sdk ]]; then
            echo "Dependency line ${line_number} has unknown kind '${kind}'." >&2
            return 1
        fi
        if [[ ! "${sha256}" =~ ^[0-9a-fA-F]{64}$ ]]; then
            echo "Dependency line ${line_number} has an invalid SHA-256 value." >&2
            return 1
        fi
        if [[ ! "${url}" =~ ^https://[^/[:space:]]+(/.*)?$ || "${url}" =~ [[:space:]] ]]; then
            echo "Dependency line ${line_number} must use an absolute HTTPS URL." >&2
            return 1
        fi
        if [[ "${installed_entry}" == /* || "${installed_entry}" == \\* ||
              "${installed_entry}" =~ ^[[:alpha:]]: ||
              "${installed_entry}" =~ (^|[\\/])\.\.([\\/]|$) ]]; then
            echo "Dependency line ${line_number} has an unsafe installed entry path." >&2
            return 1
        fi

        key="${name}|${platform}"
        if [[ -n "${dependency_keys[${key}]+present}" ]]; then
            echo "Dependency line ${line_number} duplicates '${key}'." >&2
            return 1
        fi
        dependency_keys["${key}"]=1

        project_dependency_names+=("${name}")
        project_dependency_kinds+=("${kind}")
        project_dependency_platforms+=("${platform}")
        project_dependency_versions+=("${version}")
        project_dependency_licenses+=("${license}")
        project_dependency_urls+=("${url}")
        project_dependency_sha256s+=("${sha256,,}")
        project_dependency_installed_entries+=("${installed_entry}")
        ((project_dependency_count += 1))
    done < "${lock_path}"

    if [[ "${schema_found}" == false ]]; then
        echo "Dependency lock file does not contain schema '${project_dependency_lock_schema}'." >&2
        return 1
    fi
}

get_project_premake_dependency() {
    local platform="$1"
    local match_count=0
    local index

    premake_version=""
    premake_url=""
    premake_sha256=""
    premake_entry=""

    for ((index = 0; index < project_dependency_count; index += 1)); do
        if [[ "${project_dependency_names[index]}" == premake &&
              "${project_dependency_platforms[index]}" == "${platform}" ]]; then
            ((match_count += 1))
            premake_version="${project_dependency_versions[index]}"
            premake_url="${project_dependency_urls[index]}"
            premake_sha256="${project_dependency_sha256s[index]}"
            premake_entry="${project_dependency_installed_entries[index]}"
        fi
    done

    if [[ ${match_count} -ne 1 ]]; then
        echo "Dependencies.lock must contain exactly one premake entry for ${platform}." >&2
        return 1
    fi
}
