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

VER="2026.3.0"
URL="https://downloads.scummvm.org/frs/scummvm/$VER/scummvm-$VER.tar.xz"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1
set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT
wget -O $TEMPDIR/scummvm.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/scummvm.tar.xz -C $TEMPDIR || exit 1
cd $TEMPDIR/scummvm-$VER || exit 1

export PKG_CONFIG_DIR=
export PKG_CONFIG_SYSROOT_DIR=${PAYLOAD_SDK}/target
export PKG_CONFIG_LIBDIR=${PKG_CONFIG_SYSROOT_DIR}/user/homebrew/lib/pkgconfig
export PKG_CONFIG_PATH=${PKG_CONFIG_SYSROOT_DIR}/user/homebrew/libdata/pkgconfig

./configure --prefix="${PREFIX}" --host=x86_64-pc-freebsd \
	    --enable-static --enable-release --enable-all-engines \
	    --disable-seq-midi \
	    --with-sdl-prefix="${PAYLOAD_SDK}/target/user/homebrew/bin" \
	    --with-libcurl-prefix="${PAYLOAD_SDK}/target/user/homebrew/bin" \
	    --with-freetype2-prefix="${PAYLOAD_SDK}/target/user/homebrew/bin" \
	    --opengl-mode=gl \
	    --enable-vkeybd
${MAKE} dist-generic || exit 1

cp dist-generic/scummvm/scummvm "${OUTPUT_DIR}/scummvm.elf" || exit 1

cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libGL.so.1" "${OUTPUT_DIR}/" || exit 1
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libEGL.so.1" "${OUTPUT_DIR}/" || exit 1
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libgallium-26.2.0.so" "${OUTPUT_DIR}/" || exit 1
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libvulkan.so.1" "${OUTPUT_DIR}/" || exit 1

cp -r dist-generic/scummvm/data "${OUTPUT_DIR}/data" || exit 1
mkdir -p "${OUTPUT_DIR}/extras" && touch "${OUTPUT_DIR}/extras/.keep" || exit 1
mkdir -p "${OUTPUT_DIR}/icons" && touch "${OUTPUT_DIR}/icons/.keep" || exit 1
mkdir -p "${OUTPUT_DIR}/saves" && touch "${OUTPUT_DIR}/saves/.keep" || exit 1
mkdir -p "${OUTPUT_DIR}/screenshots" && touch "${OUTPUT_DIR}/screenshots/.keep" || exit 1
mkdir -p "${OUTPUT_DIR}/themes" && touch "${OUTPUT_DIR}/themes/.keep" || exit 1

homebrew_stage_metadata
