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

LIB_URL="https://ftp.gnu.org/gnu/libmicrohttpd/"
LIB_VER="1.0.1"

PLATFORM="${PLATFORM:-ps5}"

if [[ "$PLATFORM" == "ps4" ]]; then
    if [[ -z "$PS4_PAYLOAD_SDK" ]]; then
        echo "error: PS4_PAYLOAD_SDK is not set"
        exit 1
    fi
    source ${PS4_PAYLOAD_SDK}/toolchain/orbis.sh || exit 1
    HBROOT="${PS4_HBROOT}"
    SDK="${PS4_PAYLOAD_SDK}"
elif [[ "$PLATFORM" == "ps5" ]]; then
    if [[ -z "$PS5_PAYLOAD_SDK" ]]; then
        echo "error: PS5_PAYLOAD_SDK is not set"
        exit 1
    fi
    source ${PS5_PAYLOAD_SDK}/toolchain/prospero.sh || exit 1
    HBROOT="${PS5_HBROOT}"
    SDK="${PS5_PAYLOAD_SDK}"
else
    echo "error: unknown PLATFORM '$PLATFORM', expected ps4 or ps5"
    exit 1
fi

TEMPDIR=$(mktemp -d)
trap 'rm -rf -- "$TEMPDIR"' EXIT

wget -O $TEMPDIR/lib.tar.gz $LIB_URL/libmicrohttpd-$LIB_VER.tar.gz || exit 1
tar xf $TEMPDIR/lib.tar.gz -C $TEMPDIR || exit 1

if [[ "$PLATFORM" == "ps4" ]]; then
    # Both send() and write() crash unconditionally on this PS4 kernel
    # (confirmed by calling them directly, outside of libmicrohttpd, with
    # valid arguments). websrv.elf links both libkernel_sys.sprx (explicit)
    # and libkernel_web.sprx (pulled in by default by the orbis-clang
    # wrapper), and both export write/send/recv under the same names -
    # a likely symbol collision. Bypass the ambiguous import entirely by
    # invoking the raw write(2) syscall directly, the same way websrv's own
    # code already does successfully for SYS_thr_set_name.
    cat >> $TEMPDIR/libmicrohttpd-$LIB_VER/src/microhttpd/mhd_sockets.h <<'EOF'

#include <unistd.h>
#include <sys/syscall.h>
#undef MHD_send4_
#define MHD_send4_(s,b,l,f) ((ssize_t) syscall(SYS_write, (s), (b), (l)))
EOF
fi

# Optimizing with default -O2 causes issues
export CFLAGS="-O1 -g -fno-omit-frame-pointer"

cd $TEMPDIR/libmicrohttpd-$LIB_VER
./configure --prefix="${HBROOT}" --host=x86_64 \
	    --disable-shared --enable-static \
	    --disable-curl --disable-examples
${MAKE} install DESTDIR="${SDK}/target"
