#!/usr/bin/env python3
"""Packs ScummVM's data files into one zip the core carries in its memory.

The files are the ones ScummVM's own Makefile installs for the engines this
build has (scummvm-objs.mk writes the list): each engine's .dat, the fonts,
the GUI's themes and translations, at the zip's top; the 3D engines' OpenGL
shaders in its shaders/ folder, where ScummVM looks for them (a line of the
list that reads "shaders/=<file>"). Stored, not compressed (ScummVM reads a
member whole either way, and the package is compressed); in name order with
one date, so the same files make the same bytes.

usage: make-data.py <datafiles.txt> <out.zip>
"""
import os
import sys
import zipfile

listing, out = sys.argv[1], sys.argv[2]
def entry(line):
    folder, sep, path = line.partition("=")
    if sep and folder.endswith("/"):
        return folder + os.path.basename(path), path
    return os.path.basename(line), line


files = sorted({entry(line.strip()) for line in open(listing) if line.strip()})
names = set()
tmp = out + ".tmp"
with zipfile.ZipFile(tmp, "w", zipfile.ZIP_STORED) as z:
    for name, path in files:
        if name in names:
            sys.exit("make-data: two data files are called %s" % name)
        names.add(name)
        info = zipfile.ZipInfo(name, date_time=(2000, 1, 1, 0, 0, 0))
        info.external_attr = 0o644 << 16
        with open(path, "rb") as f:
            z.writestr(info, f.read())
os.replace(tmp, out)
print("make-data: %d files, %d bytes" % (len(files), os.path.getsize(out)))
