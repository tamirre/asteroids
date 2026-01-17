# aseprite -b ./*.aseprite \
#   --sheet atlas.png \
#   --data atlas.json \
#   --format json-array \
#   --sheet-pack \
#   --border-padding 2 \
#   --shape-padding 2

#!/usr/bin/env bash
set -e

SPRITE_DIR="$1"
OUTDIR="$SPRITE_DIR/build"

mkdir -p "$OUTDIR"

# --sheet-pack \
# --sheet-columns 4 \
aseprite -b \
  "$SPRITE_DIR"/*.aseprite \
  --sheet "$OUTDIR/atlas.png" \
  --data  "$OUTDIR/atlas.json" \
  # --list-layers \
  # --split-tags \
  # --list-slices \
  --sheet-pack \
  --format json-array \
  --border-padding 2 \

# echo "Generating sprites.h"
# python $SPRITE_DIR/../tools/jsonToC.py \
#   "$OUTDIR/atlas.json" \
#   "$OUTDIR/sprites.h"
echo "DONE"
