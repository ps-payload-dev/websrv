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

VER=7.1
URL=https://github.com/fabiangreffrath/crispy-doom/archive/refs/tags/crispy-doom-$VER.tar.gz
WAD_DOOM=https://archive.org/download/doomworld/doomworld.tar/doomworld%2Fports%2Fshareware_doom_iwad.zip
WAD_HERETIC=https://archive.org/download/doomworld/doomworld.tar/doomworld%2Fports%2Fshareware_heretic_iwad.zip
WAD_HEXEN=https://archive.org/download/doomworld/doomworld.tar/doomworld%2Fports%2Fshareware_hexen_iwad.zip

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "${SCRIPT_PATH}")"
TARGET_PLATFORM="${1:-ps5}"
if [[ $# -gt 0 ]]; then shift; fi
source "${SCRIPT_DIR}/../../shared/build-platform.sh"
homebrew_setup_platform "$TARGET_PLATFORM" || exit 1

set -e


TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

cd $TEMPDIR
wget -O wad_doom.zip "${WAD_DOOM}"
wget -O wad_heretic.zip "${WAD_HERETIC}"
wget -O wad_hexen.zip "${WAD_HEXEN}"
unzip wad_doom.zip
unzip wad_heretic.zip
unzip wad_hexen.zip
mv *.WAD "${OUTPUT_DIR}/"

wget -O crispy-doom.tar.gz "${URL}"
tar xf crispy-doom.tar.gz
cd crispy-doom-crispy-doom-$VER

# Forking a ps5 bigapp crashes the kernel
sed -i 's|childpid = fork();|return -1;|g' src/setup/execute.c

./autogen.sh --prefix="${PREFIX}" --host=x86_64-pc-freebsd \
             --disable-doc --disable-zpool --disable-icons \
             --disable-bash-completion
${MAKE}

mv data/doom.png "${OUTPUT_DIR}/"
mv src/crispy-doom "${OUTPUT_DIR}/crispy-doom.elf"
mv src/crispy-doom-setup "${OUTPUT_DIR}/crispy-doom-setup.elf"

mv data/heretic.png "${OUTPUT_DIR}/"
mv src/crispy-heretic "${OUTPUT_DIR}/crispy-heretic.elf"
mv src/crispy-heretic-setup "${OUTPUT_DIR}/crispy-heretic-setup.elf"

mv data/hexen.png "${OUTPUT_DIR}/"
mv src/crispy-hexen "${OUTPUT_DIR}/crispy-hexen.elf"
mv src/crispy-hexen-setup "${OUTPUT_DIR}/crispy-hexen-setup.elf"

mv data/strife.png "${OUTPUT_DIR}/"
mv src/crispy-strife "${OUTPUT_DIR}/crispy-strife.elf"
mv src/crispy-strife-setup "${OUTPUT_DIR}/crispy-strife-setup.elf"

homebrew_stage_metadata
