#!/usr/bin/env bash
# Build the book: convert every chapter .md in this directory to PDF with
# pandoc and merge the chapter PDFs into book.pdf (in this directory).
#
# Usage:  book/build_book.sh   (may be run from anywhere)
#
# Images in the chapters are referenced relative to the chapter file
# (figures/...), so pandoc runs from this directory with --resource-path=.,
# as documented in docs/pdf.md.
#
# lualatex is used instead of the default pdflatex because chapters contain
# Unicode box-drawing characters in code blocks, which pdflatex rejects;
# monofont=DejaVu Sans Mono provides those glyphs.

set -euo pipefail

BOOK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$BOOK_DIR"

PDF_ENGINE=lualatex
MONOFONT='DejaVu Sans Mono'

# Chapters: all *.md files in this directory, in alphabetical order
# (prefix the file names with numbers, e.g. 01_intro.md, to set the order).
shopt -s nullglob
CHAPTERS=()
for f in *.md; do CHAPTERS+=("${f%.md}"); done
shopt -u nullglob
if [ ${#CHAPTERS[@]} -eq 0 ]; then
  echo "error: no chapters (*.md) found in $BOOK_DIR" >&2
  exit 1
fi

PDFS=()
for chap in "${CHAPTERS[@]}"; do
  echo "pandoc: $chap.md -> $chap.pdf"
  pandoc --pdf-engine="$PDF_ENGINE" -V monofont="$MONOFONT" \
    --resource-path=. "$chap.md" -o "$chap.pdf"
  PDFS+=("$chap.pdf")
done

echo "merging ${#PDFS[@]} chapters -> book.pdf"
rm -f book.pdf
pdfunite "${PDFS[@]}" book.pdf

if command -v pdfinfo >/dev/null; then
  echo "wrote $BOOK_DIR/book.pdf ($(pdfinfo book.pdf | awk '/^Pages:/{print $2}') pages)"
else
  echo "wrote $BOOK_DIR/book.pdf"
fi
