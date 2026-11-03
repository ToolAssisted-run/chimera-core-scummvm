#!/usr/bin/env python3
"""Writes the 3D test game: ScummVM's Playground 3D engine (its test of the 3D
renderers: a turning cube, depth offsets, a fade, a viewport, textures in
five pixel formats, a quad strip) as a zip of its folder.

The engine draws everything itself; its detector only looks for a file
called PLAYGROUND3D. In name order with one date, so the same zip every time.

usage: make-playground3d.py <out.zip>
"""
import sys
import zipfile

out = sys.argv[1]
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    info = zipfile.ZipInfo("playground3d/PLAYGROUND3D", date_time=(2000, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o644 << 16
    z.writestr(info, b"ScummVM's 3D playground\n")
