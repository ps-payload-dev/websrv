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

VER="master"
URL="https://github.com/libretro/FBNeo/archive/refs/heads/master.tar.gz"
INFO="https://raw.githubusercontent.com/libretro/libretro-core-info/refs/heads/master/fbneo_libretro.info"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1
set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/FBNeo.tar.gz "${URL}" || exit 1
tar xf  $TEMPDIR/FBNeo.tar.gz -C $TEMPDIR || exit 1

cd $TEMPDIR/FBNeo-$VER/src/burner/libretro || exit 1
${MAKE} || exit 1

mkdir -p "${OUTPUT_DIR}/.config/retroarch/cores" || exit 1
mv $TEMPDIR/FBNeo-$VER//src/burner/libretro/fbneo_libretro.so "${OUTPUT_DIR}/.config/retroarch/cores/" || exit 1
wget $INFO -O "${OUTPUT_DIR}/.config/retroarch/cores/fbneo_libretro.info"

homebrew_stage_metadata
