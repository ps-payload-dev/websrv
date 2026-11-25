#!/usr/bin/env bash
# Copyright (C) 2026 ItsJokerZz
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.


homebrew_setup_platform() {
    local platform="${1:-ps5}"

    case "$platform" in
        ps4)
            export PS4_PAYLOAD_SDK="${PS4_PAYLOAD_SDK:-/opt/ps-payload-sdk/ps4}"
            [[ -f "$PS4_PAYLOAD_SDK/toolchain/orbis.sh" ]] || { echo "error: PS4 SDK toolchain not found at $PS4_PAYLOAD_SDK" >&2; return 1; }
            source "$PS4_PAYLOAD_SDK/toolchain/orbis.sh"
            export PAYLOAD_SDK="$PS4_PAYLOAD_SDK"
            export PAYLOAD_SYSROOT="$PS4_SYSROOT"
            export PAYLOAD_HBROOT="$PS4_HBROOT"
            export PAYLOAD_DEPLOY="$PS4_DEPLOY"
            export PAYLOAD_SDL_CONFIG="$PAYLOAD_SYSROOT$PAYLOAD_HBROOT/bin/sdl2-config"
            export CROSS_COMPILE_PREFIX="$PAYLOAD_SDK/bin/orbis-"
            export OPENORBIS="$PAYLOAD_SDK"
            ;;
        ps5)
            export PS5_PAYLOAD_SDK="${PS5_PAYLOAD_SDK:-/opt/ps-payload-sdk/ps5}"
            [[ -f "$PS5_PAYLOAD_SDK/toolchain/prospero.sh" ]] || { echo "error: PS5 SDK toolchain not found at $PS5_PAYLOAD_SDK" >&2; return 1; }
            source "$PS5_PAYLOAD_SDK/toolchain/prospero.sh"
            export PAYLOAD_SDK="$PS5_PAYLOAD_SDK"
            export PAYLOAD_SYSROOT="$PS5_SYSROOT"
            export PAYLOAD_HBROOT="$PS5_HBROOT"
            export PAYLOAD_DEPLOY="$PS5_DEPLOY"
            export PAYLOAD_SDL_CONFIG="$PAYLOAD_SDK/bin/prospero-sdl2-config"
            export CROSS_COMPILE_PREFIX="$PAYLOAD_SDK/bin/prospero-"
            ;;
        *)
            echo "error: platform must be ps4 or ps5 (got '$platform')" >&2
            return 2
            ;;
    esac

    export TARGET_PLATFORM="$platform"
    export REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
    export OUTPUT_DIR="${REPO_ROOT}/build/${platform}/homebrew/$(basename "${SCRIPT_DIR}")"
    mkdir -p "$OUTPUT_DIR"

    if [[ ! -d "$PAYLOAD_SDK" ]]; then
        echo "error: SDK not found at $PAYLOAD_SDK" >&2
        return 1
    fi
}

homebrew_stage_metadata() {
    local source_dir="${1:-${SCRIPT_DIR}}"

    if [[ -f "${source_dir}/homebrew.js" ]]; then
        cp "${source_dir}/homebrew.js" "${OUTPUT_DIR}/homebrew.js"
    fi
    if [[ -d "${source_dir}/sce_sys" ]]; then
        mkdir -p "${OUTPUT_DIR}/sce_sys"
        cp -a "${source_dir}/sce_sys/." "${OUTPUT_DIR}/sce_sys/"
    fi
}
