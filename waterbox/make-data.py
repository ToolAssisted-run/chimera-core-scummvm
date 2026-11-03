#!/usr/bin/env python3
"""Packs ScummVM's data files into one zip the core carries in its memory.

The files are the ones ScummVM's own Makefile installs for the engines this
build has (scummvm-objs.mk writes the list): each engine's .dat, the fonts,
the GUI's themes and translations. Stored, not compressed (ScummVM reads a
member whole either way, and the package is compressed); in name order with
one date, so the same files make the same bytes.

usage: make-data.py <datafiles.txt> <out.zip>
"""
import os
import sys
import zipfile

listing, out = sys.argv[1], sys.argv[2]
files = sorted({line.strip() for line in open(listing) if line.strip()}, key=os.path.basename)
names = set()
tmp = out + ".tmp"
with zipfile.ZipFile(tmp, "w", zipfile.ZIP_STORED) as z:
    for path in files:
        name = os.path.basename(path)
        if name in names:
            sys.exit("make-data: two data files are called %s" % name)
        names.add(name)
        info = zipfile.ZipInfo(name, date_time=(2000, 1, 1, 0, 0, 0))
        info.external_attr = 0o644 << 16
        with open(path, "rb") as f:
            z.writestr(info, f.read())
os.replace(tmp, out)
print("make-data: %d files, %d bytes" % (len(files), os.path.getsize(out)))
