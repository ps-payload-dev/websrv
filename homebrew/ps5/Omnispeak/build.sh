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

VER="1.1"
URL="https://github.com/sulix/omnispeak/archive/refs/tags/v$VER.tar.gz"
BIN="https://github.com/sulix/omnispeak/releases/download/v$VER/omnispeak-linux-$VER.tar.gz"


SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1
set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/omnispeak.tar.gz "${URL}" || exit 1
tar xf $TEMPDIR/omnispeak.tar.gz -C $TEMPDIR || exit 1

wget -O $TEMPDIR/omnispeak-linux.tar.gz "${BIN}" || exit 1
tar xf $TEMPDIR/omnispeak-linux.tar.gz -C $TEMPDIR || exit 1

${CMAKE} -DCMAKE_BUILD_TYPE=Release \
	 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
	 -DRENDERER=sdl2gl \
	 -B $TEMPDIR/build \
	 -S $TEMPDIR/omnispeak-$VER || exit 1
${MAKE} -C $TEMPDIR/build || exit 1

mv $TEMPDIR/build/omnispeak "${OUTPUT_DIR}/omnispeak.elf" || exit 1
mv $TEMPDIR/omnispeak-linux-$VER/*.CK4 "${OUTPUT_DIR}/" || exit 1
mv $TEMPDIR/omnispeak-linux-$VER/data "${OUTPUT_DIR}/data" || exit 1

cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libGL.so.1" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libEGL.so.1" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libgallium-26.2.0.so" "${OUTPUT_DIR}/"
cp "${PAYLOAD_SDK}/target/user/homebrew/lib/libvulkan.so.1" "${OUTPUT_DIR}/"

homebrew_stage_metadata
