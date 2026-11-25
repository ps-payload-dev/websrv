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

VER="20260203-10664-ba6b7bb1d"
URL="http://dukeworld.duke4.net/eduke32/synthesis/$VER/eduke32_src_$VER.tar.xz"
DEMO="http://dukeworld.duke4.net/classic%20dukeworld/share/3dduke13.zip"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1
set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/demo.zip "${DEMO}" || exit 1
cd $TEMPDIR
unzip demo.zip || exit 1
unzip -Luq "DN3DSW13.SHR" defs.con duke.rts game.con license.txt user.con duke3d.grp
mv defs.con duke.rts game.con license.txt user.con duke3d.grp "${OUTPUT_DIR}/"


wget -O $TEMPDIR/eduke32.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/eduke32.tar.xz -C $TEMPDIR || exit 1
cd $TEMPDIR/eduke32_$VER || exit 1

sed -i 's|#include <malloc.h>|#include <stdlib.h>|g' source/build/include/compat.h
sed -i 's|-fuse-ld=lld||g' Common.mak
sed -i 's|-lexecinfo||g' GNUmakefile
sed -i 's|define PRINTSTACKONSEGV 1||g' source/build/src/sdlayer.cpp
sed -i 's|pthread_get_name_np|//|g' source/build/src/loguru.cpp
patch -p0 < $SCRIPT_DIR/eduke32-vfs-without-relative-paths.diff

CFLAGS="$(${PAYLOAD_SDL_CONFIG} --cflags)"
LDFLAGS="$(${PAYLOAD_SDL_CONFIG} --libs)"
${MAKE} PRETTY_OUTPUT=0 USE_OPENGL=1 HAVE_GTK2=0 \
	USE_MIMALLOC=0 PLATFORM=BSD LTO=0 \
	SDLCONFIG="${PAYLOAD_SDL_CONFIG}" \
	CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" STRIP=$STRP || exit 1

mv eduke32 "${OUTPUT_DIR}/eduke32.elf" || exit 1

cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libGL.so.1" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libEGL.so.1" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libgallium-26.2.0.so" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libvulkan.so.1" "${OUTPUT_DIR}/"

homebrew_stage_metadata
