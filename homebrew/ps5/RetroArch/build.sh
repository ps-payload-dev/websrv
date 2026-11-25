#!/usr/bin/env bash
#   Copyright (C) 2024 John Törnblom
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

VER="1.21.0"
URL="https://github.com/libretro/RetroArch/archive/refs/tags/v$VER.tar.gz"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1
set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/RetroArch.tar.gz "${URL}" || exit 1
tar xf  $TEMPDIR/RetroArch.tar.gz -C $TEMPDIR || exit 1

cd $TEMPDIR/RetroArch-$VER || exit 1
sed -i 's|SDL_RENDERER_ACCELERATED|SDL_RENDERER_SOFTWARE|g' gfx/drivers/sdl2_gfx.c
sed -i 's|$(LIBRETRO_COMM_DIR)/utils/md5.o||g' Makefile.common

export CC
export CXX
export PKG_CONFIG
export CROSS_COMPILE="${CROSS_COMPILE_PREFIX}"
export OS="BSD"
export DISTRO=
export LDFLAGS="-rdynamic"
./configure \
    --prefix="${PREFIX}" \
    --enable-sdl2 \
    --enable-ffmpeg \
    --enable-zlib \
    --enable-freetype \
    --enable-flac \
    --disable-cdrom \
    --enable-mmap \
    --enable-dylib \
    --enable-networking \
    --enable-network_video \
    --disable-update_cores \
    --disable-update_core_info \
    --enable-online_updater \
    --disable-ozone \
    --disable-xmb \
    --disable-materialui \
    --disable-builtinflac \
    --disable-builtinmbedtls \
    --disable-builtinbearssl \
    --disable-builtinzlib \
    --disable-builtinglslang \
    --disable-crtswitchres \
    --disable-discord \
    --disable-qt \
    --disable-ssa \
    --disable-jack \
    --disable-oss \
    --disable-sdl \
    --disable-cg \
    --disable-vulkan \
    --disable-opengl_core \
    --disable-opengl1 \
    --disable-opengl \
    --disable-microphone
    ${MAKE} V=1 || exit 1


mkdir -p "${OUTPUT_DIR}/sce_sys"
mv $TEMPDIR/RetroArch-$VER/media/icons/playstore/icon.png "${OUTPUT_DIR}/sce_sys/icon0.png" || exit 1
mv $TEMPDIR/RetroArch-$VER/retroarch.cfg "${OUTPUT_DIR}/retroarch.cfg"
mv $TEMPDIR/RetroArch-$VER/retroarch "${OUTPUT_DIR}/retroarch.elf" || exit 1

echo 'input_menu_toggle_gamepad_combo = "2"' >> "${OUTPUT_DIR}/retroarch.cfg"
echo 'menu_swap_ok_cancel_buttons = "true"' >> "${OUTPUT_DIR}/retroarch.cfg"
echo 'video_threaded = "true"' >> "${OUTPUT_DIR}/retroarch.cfg"

homebrew_stage_metadata
