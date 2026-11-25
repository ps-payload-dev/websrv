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

VER="2.4.4"
URL="https://github.com/TerryCavanagh/VVVVVV.git"
ASSETS="https://thelettervsixtim.es/makeandplay/data.zip"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1

set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/data.zip "${ASSETS}"

git clone --recurse-submodules --depth 1 --branch $VER $URL "$TEMPDIR/VVVVVV"

export SDL_CONFIG="${PAYLOAD_SDL_CONFIG}"
sed -i 's|SDL_RENDERER_ACCELERATED|SDL_RENDERER_SOFTWARE|g' $TEMPDIR/VVVVVV/desktop_version/src/Screen.cpp

# Make and Play edition is exempt from the VVVVVV custom license
${CMAKE} -DCMAKE_BUILD_TYPE=Release \
     -DMAKEANDPLAY=ON \
     -DOFFICIAL_BUILD=ON \
     -B $TEMPDIR/build \
     $TEMPDIR/VVVVVV/desktop_version
${MAKE} -C $TEMPDIR/build

mkdir -p "${OUTPUT_DIR}/sce_sys"
unzip $TEMPDIR/data.zip "VVVVVV.png" -d $TEMPDIR
mv $TEMPDIR/VVVVVV.png "${OUTPUT_DIR}/sce_sys/icon0.png"

mv $TEMPDIR/build/VVVVVV "${OUTPUT_DIR}/VVVVVV.elf"
mv $TEMPDIR/data.zip "${OUTPUT_DIR}/"
mv $TEMPDIR/VVVVVV/desktop_version/fonts/ "${OUTPUT_DIR}/"
mv $TEMPDIR/VVVVVV/desktop_version/lang/ "${OUTPUT_DIR}/"

homebrew_stage_metadata
