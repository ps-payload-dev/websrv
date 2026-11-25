#!/usr/bin/env bash
#   Copyright (C) 2026 John Törnblom
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not see
# <http://www.gnu.org/licenses/>.

URL="https://github.com/Cpasjuste/pemu.git"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

set -e

cd "${TEMPDIR}"
git clone --recurse-submodules $URL
cd pemu

sed -i 's|SDL_WINDOW_SHOWN|SDL_WINDOW_FULLSCREEN|g' external/libcross2d/source/platforms/sdl2/sdl2_renderer.cpp

mkdir cmake-build && cd cmake-build
if [[ "$TARGET_PLATFORM" == "ps4" ]]; then
    ${CMAKE} -DPLATFORM_PS4=ON -DCMAKE_BUILD_TYPE=Release ..
    ${MAKE} pfbneo.deps
    for core in pfbneo pgen pnes psnes pgba; do ${MAKE} "${core}_pkg"; done
    mv *.pkg "${OUTPUT_DIR}/"
else
    ${CMAKE} -DPLATFORM_PS5=ON -DOPTION_MPV_PLAYER=OFF -DCMAKE_BUILD_TYPE=Release ..
    ${MAKE} pfbneo.deps
    for core in pfbneo pgen pnes psnes pgba; do ${MAKE} "${core}_ps5_release"; done
    mv *.zip "${OUTPUT_DIR}/"
fi

homebrew_stage_metadata
