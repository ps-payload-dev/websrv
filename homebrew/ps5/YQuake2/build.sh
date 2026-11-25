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

VER="ps5"
URL="https://github.com/ps5-payload-dev/yquake2/archive/refs/heads/$VER.tar.gz"
DEMO="https://ftp.gwdg.de/pub/misc/ftp.idsoftware.com/idstuff/quake2/q2-314-demo-x86.exe"
MODELS="https://deponie.yamagi.org/quake2/assets/texturepack/models.zip"
TEXTURES="https://deponie.yamagi.org/quake2/assets/texturepack/textures.zip"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1
set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

mkdir -p "${OUTPUT_DIR}/baseq2"
wget -O "${OUTPUT_DIR}/baseq2/models.zip" "${MODELS}" || exit 1
wget -O "${OUTPUT_DIR}/baseq2/textures.zip" "${TEXTURES}" || exit 1

wget -O $TEMPDIR/yquake2.tar.gz "${URL}" || exit 1
wget -O $TEMPDIR/install.exe "${DEMO}" || exit 1

tar xf $TEMPDIR/yquake2.tar.gz -C $TEMPDIR || exit 1
7z x -o$TEMPDIR $TEMPDIR/install.exe || exit 1

sed -i 's|SDL2_LIBRARY|SDL2_LIBRARIES|g' $TEMPDIR/yquake2-$VER/CMakeLists.txt

cd $TEMPDIR

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
	 -DCMAKE_EXE_LINKER_FLAGS="-rdynamic" \
	 -DSOFT_RENDERER=NO \
	 -DGL1_RENDERER=NO \
	 -DGL3_RENDERER=YES \
	 -DGLES1_RENDERER=NO \
	 -DGLES3_RENDERER=NO \
	 -B $TEMPDIR/build \
         -S $TEMPDIR/yquake2-$VER
${CMAKE} --build $TEMPDIR/build

mv $TEMPDIR/build/release/quake2 "${OUTPUT_DIR}/quake2.elf" || exit 1
mv $TEMPDIR/build/release/ref_gl3.so "${OUTPUT_DIR}/" || exit 1
mv $TEMPDIR/build/release/baseq2/game.so "${OUTPUT_DIR}/baseq2/" || exit 1
mv $TEMPDIR/Install/Data/baseq2/pak0.pak "${OUTPUT_DIR}/baseq2/" || exit 1
mv $TEMPDIR/Install/Data/baseq2/players "${OUTPUT_DIR}/baseq2/" || exit 1

cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libGL.so.1" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libEGL.so.1" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libgallium-26.2.0.so" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libvulkan.so.1" "${OUTPUT_DIR}/"

homebrew_stage_metadata
