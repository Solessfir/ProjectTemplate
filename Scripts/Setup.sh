#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repository_root="$(cd -- "${script_dir}/.." && pwd)"
lock_path="${repository_root}/Config/Dependencies.lock"
print_premake_path=false
validate_only=false

source "${script_dir}/DependencyLock.sh"

for argument in "$@"; do
    case "${argument}" in
        --print-premake-path) print_premake_path=true ;;
        --validate-only) validate_only=true ;;
        *) echo "Unknown Setup argument: ${argument}" >&2; exit 2 ;;
    esac
done

read_project_dependency_lock "${lock_path}"
get_project_premake_dependency "linux-x64"

if [[ "${validate_only}" == true ]]; then
    echo "Validated ${project_dependency_count} dependency lock entries."
    exit 0
fi

if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
    echo 'Setup.sh currently supports Linux x86_64.' >&2
    exit 1
fi

install_directory="${repository_root}/External/Premake/Linux/${premake_version}"
premake_path="${install_directory}/${premake_entry}"
download_directory="${repository_root}/External/Premake/.Downloads"
archive_path="${download_directory}/$(basename -- "${premake_url}")"
temporary_archive=""
temporary_directory=""
compiler_probe_directory=""

cleanup() {
    [[ -z "${temporary_archive}" || ! -e "${temporary_archive}" ]] || rm -f -- "${temporary_archive}"
    [[ -z "${temporary_directory}" || ! -e "${temporary_directory}" ]] || rm -rf -- "${temporary_directory}"
    [[ -z "${compiler_probe_directory}" || ! -e "${compiler_probe_directory}" ]] || rm -rf -- "${compiler_probe_directory}"
}
trap cleanup EXIT

validate_premake() {
    local executable="$1"
    local version_output

    [[ -x "${executable}" ]] || return 1
    version_output="$(cd /tmp && "${executable}" --version 2>&1)" || return 1
    [[ "${version_output}" == *"${premake_version}"* ]]
}

print_prerequisite_command() {
    if [[ -f /etc/os-release ]]; then source /etc/os-release; fi
    case "${ID:-}:${ID_LIKE:-}" in
        *arch*) echo 'Install required packages with: sudo pacman -S --needed base-devel git curl pkgconf mesa libx11 libxrandr libxinerama libxcursor libxi libxkbcommon wayland' >&2 ;;
        *ubuntu*)
            echo 'Install required packages with: sudo apt-get update && sudo apt-get install -y gcc-14 g++-14 git make curl pkg-config libgl1-mesa-dev xorg-dev libwayland-dev libxkbcommon-dev wayland-protocols' >&2
            echo 'Then select GCC 14 with: export CC=gcc-14 CXX=g++-14' >&2
            ;;
        *debian*) echo 'Install required packages with: sudo apt-get update && sudo apt-get install -y build-essential git curl pkg-config libgl1-mesa-dev xorg-dev libwayland-dev libxkbcommon-dev wayland-protocols' >&2 ;;
        *fedora*|*rhel*) echo 'Install required packages with: sudo dnf install -y gcc-c++ git make curl pkgconf-pkg-config mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libxkbcommon-devel wayland-devel wayland-protocols-devel' >&2 ;;
        *) echo 'Install Git, Make, a C++23 compiler, curl, pkg-config, OpenGL, X11 extension, Wayland, and xkbcommon development packages.' >&2 ;;
    esac
}

if [[ "${print_premake_path}" == true ]]; then
    if ! validate_premake "${premake_path}"; then
        echo "Premake is not installed at '${premake_path}'. Run Setup.sh first." >&2
        exit 1
    fi
    printf '%s\n' "${premake_path}"
    exit 0
fi

missing_tools=()
for required_tool in git make sha256sum tar curl pkg-config wayland-scanner; do
    command -v "${required_tool}" >/dev/null 2>&1 || missing_tools+=("${required_tool}")
done
if [[ ${#missing_tools[@]} -gt 0 ]]; then
    echo "Setup is missing: ${missing_tools[*]}" >&2
    print_prerequisite_command
    exit 1
fi

required_modules=(gl x11 xrandr xinerama xcursor xi wayland-client wayland-cursor wayland-egl xkbcommon)
if ! pkg-config --exists "${required_modules[@]}"; then
    echo 'One or more required OpenGL, X11, Wayland, or xkbcommon development packages are missing.' >&2
    pkg-config --print-errors --exists "${required_modules[@]}" || true
    print_prerequisite_command
    exit 1
fi

cxx="${CXX:-}"
if [[ -z "${cxx}" ]]; then
    for candidate in c++ g++ clang++; do
        if command -v "${candidate}" >/dev/null 2>&1; then cxx="${candidate}"; break; fi
    done
fi
if [[ -z "${cxx}" ]]; then
    echo 'Setup requires a C++23-capable GCC or Clang compiler.' >&2
    print_prerequisite_command
    exit 1
fi

compiler_probe_directory="$(mktemp -d)"
cat > "${compiler_probe_directory}/Probe.cpp" <<'EOF'
#include <expected>
#include <print>
int main()
{
    const std::expected<int, int> Value = 42;
    std::println("ProjectTemplate C++23 probe");
    return Value.value() == 42 ? 0 : 1;
}
EOF
if ! "${cxx}" -std=c++23 -Wall -Wextra -Werror "${compiler_probe_directory}/Probe.cpp" -o "${compiler_probe_directory}/Probe" ||
   ! "${compiler_probe_directory}/Probe" >/dev/null; then
    echo "Compiler '${cxx}' failed the required C++23 library probe for <expected> and <print>." >&2
    print_prerequisite_command
    exit 1
fi
rm -rf -- "${compiler_probe_directory}"
compiler_probe_directory=""

git -C "${repository_root}" submodule sync --recursive
git -C "${repository_root}" submodule update --init --recursive

if ! validate_premake "${premake_path}"; then
    mkdir -p -- "${download_directory}" "$(dirname -- "${install_directory}")"
    if [[ -f "${archive_path}" ]] && ! echo "${premake_sha256}  ${archive_path}" | sha256sum --check --status; then
        rm -f -- "${archive_path}"
    fi

    if [[ ! -f "${archive_path}" ]]; then
        temporary_archive="${archive_path}.$$.tmp"
        echo "Downloading Premake ${premake_version}..."
        curl --fail --location --retry 3 --output "${temporary_archive}" "${premake_url}"
        echo "${premake_sha256}  ${temporary_archive}" | sha256sum --check --status || {
            echo 'Premake archive SHA-256 mismatch.' >&2
            exit 1
        }
        mv -- "${temporary_archive}" "${archive_path}"
        temporary_archive=""
    fi

    if [[ -e "${install_directory}" ]]; then
        echo "Premake install directory exists but is invalid: ${install_directory}" >&2
        exit 1
    fi

    temporary_directory="$(dirname -- "${install_directory}")/.${premake_version}.$$.tmp"
    mkdir -- "${temporary_directory}"
    tar -xzf "${archive_path}" -C "${temporary_directory}"
    chmod +x "${temporary_directory}/${premake_entry}"
    validate_premake "${temporary_directory}/${premake_entry}"
    mv -- "${temporary_directory}" "${install_directory}"
    temporary_directory=""
fi

rm -f -- "${archive_path}"
rmdir -- "${download_directory}" 2>/dev/null || true

generated_directory="${repository_root}/Intermediate/Generated/GLFW"
mkdir -p -- "${generated_directory}"
protocols=(
    wayland.xml
    viewporter.xml
    xdg-shell.xml
    idle-inhibit-unstable-v1.xml
    pointer-constraints-unstable-v1.xml
    relative-pointer-unstable-v1.xml
    fractional-scale-v1.xml
    xdg-activation-v1.xml
    xdg-decoration-unstable-v1.xml
)

for protocol in "${protocols[@]}"; do
    source_path="${repository_root}/External/glfw/deps/wayland/${protocol}"
    base_name="${protocol%.xml}"
    header_path="${generated_directory}/${base_name}-client-protocol.h"
    code_path="${generated_directory}/${base_name}-client-protocol-code.h"
    if [[ ! -f "${header_path}" || "${source_path}" -nt "${header_path}" ]]; then
        wayland-scanner client-header "${source_path}" "${header_path}.tmp"
        mv -- "${header_path}.tmp" "${header_path}"
    fi
    if [[ ! -f "${code_path}" || "${source_path}" -nt "${code_path}" ]]; then
        wayland-scanner private-code "${source_path}" "${code_path}.tmp"
        mv -- "${code_path}.tmp" "${code_path}"
    fi
done

echo "C++ compiler: $(command -v "${cxx}")"
echo "Premake ${premake_version}: ${premake_path}"
echo 'ProjectTemplate setup completed.'
