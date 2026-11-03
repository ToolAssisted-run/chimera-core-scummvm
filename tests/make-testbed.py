#!/usr/bin/env python3
"""Writes the test game: ScummVM's testbed engine's data, as a zip of its
folder, the way a user gives the core a game.

The data is what ScummVM's own dists/engine-data/create-testbed-data.sh
makes (a TESTBED file the detector looks for, folders of differently cased
names for the file-system tests, and the audio CD test's files), made here
from the submodule's copy, so it is ScummVM's (GPL) and anyone may have it.
In name order with one date, so the same files make the same zip.

usage: make-testbed.py <out.zip> [<scummvm checkout>]
"""
import os
import sys
import zipfile

here = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1]
svm = sys.argv[2] if len(sys.argv) > 2 else os.path.join(here, "..", "extern", "scummvm")
audiocd = os.path.join(svm, "dists", "engine-data", "testbed-audiocd-files")
if not os.path.isdir(audiocd):
    sys.exit("make-testbed: no %s (is the submodule checked out?)" % audiocd)

files = {
    "TESTBED": b"ScummVM rocks!\n",
    "test1/file.txt": b"It works!\n",
    "Test2/File.txt": b"It works!\n",
    "TEST3/FILE.txt": b"It works!\n",
    "tEST4/fILe.txt": b"It works!\n",
    "test5/file.": b"It works!\n",
}
for root, dirs, names in os.walk(audiocd):
    dirs.sort()
    for n in sorted(names):
        rel = os.path.relpath(os.path.join(root, n), audiocd)
        data = open(os.path.join(root, n), "rb").read()
        # the script moves music.mid to the top of the folder
        files["music.mid" if rel == "music.mid" else "audiocd-files/" + rel.replace(os.sep, "/")] = data

with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for name in sorted(files):
        info = zipfile.ZipInfo("testbed/" + name, date_time=(2000, 1, 1, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o644 << 16
        z.writestr(info, files[name])
