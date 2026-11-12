#!/usr/bin/env bash
# Send a PDF to the BOOX ereader's "Drop" web UI so it appears under
# http://<device>:8085/#/pc/recent ("Recent" = the Books/ folder on the
# device) for proofreading.
#
# Usage:  book/send_to_ereader.sh [file.pdf]
#         Default file is book/book.pdf; if it does not exist yet it is
#         built first with build_book.sh.
#
# The upload API is the one the Drop UI itself uses (see js/chunk-0c4fb349.js
# served by the device): POST /api/storage/upload with multipart fields
# "dir", "sender" and "file". Re-uploading the same name overwrites the file.

set -euo pipefail

BOOK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EREADER="${EREADER:-http://192.168.0.53:8085}"  # override with EREADER=... if the device IP changes
DIR=Recent                                       # upload target; shows up as "Recent" in #/pc/recent
PDF="${1:-$BOOK_DIR/book.pdf}"

if [ ! -f "$PDF" ]; then
  echo "$PDF not found - building it first"
  "$BOOK_DIR/build_book.sh"
fi

if ! curl -sS -m 5 -o /dev/null "$EREADER/"; then
  echo "error: ereader not reachable at $EREADER (is it on the same network?)" >&2
  exit 1
fi

echo "sending $PDF ($(du -h "$PDF" | cut -f1)) to $EREADER, dir=$DIR"
resp="$(curl -sS -m 300 -w '\n%{http_code}' \
  -F "dir=$DIR" -F 'sender=web' -F "file=@$PDF" \
  "$EREADER/api/storage/upload" || true)"
http="${resp##*$'\n'}"
body="${resp%$'\n'*}"
echo "HTTP $http: $body"

if [ "$http" != 200 ]; then
  echo "error: upload failed (HTTP $http)" >&2
  exit 1
fi
ok="$(printf '%s' "$body" | python3 -c 'import json,sys; d=json.load(sys.stdin); print(bool(d.get("successful")) and d.get("code")==0)' 2>/dev/null || echo False)"
if [ "$ok" != True ]; then
  echo "error: upload rejected by device: $body" >&2
  exit 1
fi
path="$(printf '%s' "$body" | python3 -c 'import json,sys; print(json.load(sys.stdin)["message"])')"
echo "ok - on device at: $path"
