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

VER="1.32.1"
URL="https://mednafen.github.io/releases/files/mednafen-$VER.tar.xz"

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1
set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT
wget -O $TEMPDIR/mednafen.tar.xz "${URL}" || exit 1
tar xf $TEMPDIR/mednafen.tar.xz -C $TEMPDIR || exit 1
cd $TEMPDIR/mednafen || exit 1

sed -i 's|defined(HAVE_PTHREAD_SETNAME_NP)|0|g' src/mthreading/MThreading_POSIX.cpp

./configure --prefix="${PREFIX}" --host=x86_64-pc-freebsd \
	    --without-libiconv-prefix \
	    --disable-debugger --disable-nls
${MAKE}

cp src/mednafen "${OUTPUT_DIR}/mednafen.elf"

homebrew_stage_metadata
