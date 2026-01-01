#!/usr/bin/env python3
import sys
import json
import re
from pathlib import Path
from collections import defaultdict

# ------------------------------------------------------------
# Args
# ------------------------------------------------------------
if len(sys.argv) != 3:
    print("usage: generate_sprites_rows.py <atlas.json> <output.inc>")
    sys.exit(1)

json_path = Path(sys.argv[1]).resolve()
out_path  = Path(sys.argv[2]).resolve()

if not json_path.exists():
    print("ERROR: atlas.json not found:", json_path)
    sys.exit(1)

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------
def base_name(filename: str) -> str:
    """
    'rocketNew 3.aseprite' -> 'rocketNew'
    'bullet.png'          -> 'bullet'
    """
    name = Path(filename).stem
    return re.sub(r"\s+\d+$", "", name)

def sprite_id(name: str) -> str:
    return "SPRITE_" + name.upper()

# ------------------------------------------------------------
# Load JSON
# ------------------------------------------------------------
with open(json_path, "r", encoding="utf-8") as f:
    data = json.load(f)

frames = data.get("frames", [])
if not frames:
    print("ERROR: no frames in atlas.json")
    sys.exit(1)

# ------------------------------------------------------------
# Group frames per sprite
# ------------------------------------------------------------
sprites = defaultdict(list)

for f in frames:
    name = base_name(f["filename"])
    sprites[name].append(f["frame"])

# ------------------------------------------------------------
# Validate + normalize rows
# ------------------------------------------------------------
sprite_names = sorted(sprites.keys())

with open(out_path, "w", encoding="utf-8") as out:
    out.write("// AUTO-GENERATED — DO NOT EDIT\n\n")

    for name in sprite_names:
        rects = sprites[name]
        sid   = sprite_id(name)

        # Sort left-to-right
        rects.sort(key=lambda r: r["x"])

        # Validate: all frames on same row
        ys = {r["y"] for r in rects}
        if len(ys) != 1:
            print(f"ERROR: sprite '{name}' spans multiple rows — "
                  f"use packed workflow instead")
            sys.exit(1)

        # Validate: consistent frame size
        ws = {r["w"] for r in rects}
        hs = {r["h"] for r in rects}
        if len(ws) != 1 or len(hs) != 1:
            print(f"ERROR: sprite '{name}' has inconsistent frame sizes")
            sys.exit(1)

        frame_w = rects[0]["w"]
        frame_h = rects[0]["h"]
        frame_count = len(rects)

        x0 = rects[0]["x"]
        y0 = rects[0]["y"]
        total_w = frame_w * frame_count

        out.write(f"case {sid}:")
        out.write(
            f"{{s.coords = (Rectangle)"
            f"{{{x0}, {y0}, {total_w}, {frame_h}}};"
        )
        out.write(f" s.frameCount = {frame_count};")
        out.write(" break;}}\n")

print("Generated:", out_path)

