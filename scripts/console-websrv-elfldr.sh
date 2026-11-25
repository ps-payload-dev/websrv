#!/usr/bin/env bash
# Copyright (C) 2026 ItsJokerZz
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.

set -euo pipefail

usage() {
    echo "Usage: $(basename "$0") -h HOST PAYLOAD [ARGS...]" >&2
    echo "PAYLOAD may be a local ELF file or an HTTP(S) URL." >&2
}

HOST=""
while getopts ":h:" opt; do
    case "$opt" in
        h) HOST="$OPTARG" ;;
        *) usage; exit 2 ;;
    esac
done
shift $((OPTIND - 1))

if [[ -z "$HOST" || $# -lt 1 ]]; then
    usage
    exit 2
fi

PAYLOAD="$1"
shift
PAYLOAD_NAME="$(basename "${PAYLOAD%%\?*}")"
ARGS="$PAYLOAD_NAME"
for ARG in "$@"; do
    ARGS+=" $ARG"
done

URL="http://${HOST}:8080/elfldr"
CURL_ARGS=(--fail --show-error --silent --url-query "pipe=1" --url-query "args=$ARGS")
if [[ "$PAYLOAD" == http://* || "$PAYLOAD" == https://* ]]; then
    CURL_ARGS+=(--get --data-urlencode "elf=$PAYLOAD" "$URL")
else
    if [[ ! -f "$PAYLOAD" ]]; then
        echo "error: payload file not found: $PAYLOAD" >&2
        exit 1
    fi
    CURL_ARGS+=(--form "elf=@${PAYLOAD}" "$URL")
fi

curl "${CURL_ARGS[@]}"