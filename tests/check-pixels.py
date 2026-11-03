#!/usr/bin/env python3
"""Is each named pixel of a picture the colour expected? The picture is a
32-bit TGA or a PNG, as compare-pictures.py reads them.

usage: check-pixels.py <picture> <x>,<y>=<RRGGBB>[~<tolerance>] ...
       (exit 0 when every pixel is within its tolerance - 0 when none is
       given - on each of red, green and blue)
"""
import os
import sys

here = os.path.dirname(os.path.abspath(__file__))


def read(path):
    # compare-pictures.py's readers, without running its comparison
    src = open(os.path.join(here, "compare-pictures.py")).read()
    src = src[:src.index("\nargs = sys.argv[1:]")]
    scope = {}
    exec(compile(src, "compare-pictures.py", "exec"), scope)
    return scope["read"](path)


w, h, lines = read(sys.argv[1])
bad = []
for want in sys.argv[2:]:
    place, colour = want.split("=")
    x, y = (int(v) for v in place.split(","))
    tol = 0
    if "~" in colour:
        colour, t = colour.split("~")
        tol = int(t)
    r, g, b = (int(colour[i:i + 2], 16) for i in (0, 2, 4))
    if not (0 <= x < w and 0 <= y < h):
        bad.append(f"({x},{y}) is outside the {w}x{h} picture")
        continue
    p = lines[y][x * 3:x * 3 + 3]
    if max(abs(p[0] - r), abs(p[1] - g), abs(p[2] - b)) > tol:
        bad.append(f"({x},{y}) is {p[0]:02X}{p[1]:02X}{p[2]:02X}, not {colour}" + (f" within {tol}" if tol else ""))
if bad:
    sys.exit("; ".join(bad))
print(f"{len(sys.argv) - 2} pixels of the {w}x{h} picture as expected")
