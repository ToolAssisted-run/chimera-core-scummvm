#!/bin/sh
# Builds the ScummVM core package, scummvm.chimeraCore.
#
# A package is core.wbx (fixed name) + waterbox.config + default_keybinds.json +
# file_slots.json + the licences, loaded through chimera's one built-in generic
# adapter. It carries no game: a project brings the game as a zip of its
# folder. ScummVM's own data files are built into core.wbx.
#
# Two packages are made from the same sources. The published one (the
# default) leaves out ScummVM's two test engines, testbed and Playground 3D
# (built by guest.mk TEST_ENGINES=no, in build/guest-release). The test
# package (-t) has them: the tests and the frontend tests play them. Its
# version ends in "tests", and build.json says which package it is.
#
# Usage: ./build-package.sh [-t] [-m <miniBox dir>] [-r <chimera root>] [-o <out dir>]
#   -t makes the test package.
#   -r installs the package into <chimera root>/build/Cores (what CI's publish
#      job does); -o writes it to <out dir> instead. Neither: build/package
#      (the published one) or build/package-tests (-t).
set -eu
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
mb="${MINIBOX_DIR:-}"
chimera_root=""
out=""
tests=no
while getopts "tm:r:o:" opt; do
	case "$opt" in
		t) tests=yes ;;
		m) mb="$OPTARG" ;;
		r) chimera_root="$OPTARG" ;;
		o) out="$OPTARG" ;;
		*) exit 2 ;;
	esac
done

if [ -n "$chimera_root" ]; then
	[ -d "$chimera_root" ] || { echo "no chimera checkout at $chimera_root" >&2; exit 1; }
	chimera_root="$(cd "$chimera_root" && pwd)"
	[ -n "$mb" ] || mb="$chimera_root/extern/chimera-common-minibox"
	[ -n "$out" ] || out="$chimera_root/build/Cores"
fi
[ -n "$mb" ] || { echo "give miniBox: -m <path>, MINIBOX_DIR, or -r <chimera root>" >&2; exit 1; }
[ -d "$mb" ] || { echo "miniBox not found at $mb; pass -m <path> or set MINIBOX_DIR" >&2; exit 1; }
mb="$(cd "$mb" && pwd)"
if [ "$tests" = yes ]; then
	variant=tests
	guest="$root/build/guest"
	[ -n "$out" ] || out="$root/build/package-tests"
else
	variant=release
	guest="$root/build/guest-release"
	[ -n "$out" ] || out="$root/build/package"
fi

# the guest (guest.mk runs check-wbx before it calls core.wbx built)
mkdir -p "$root/build"
make -C "$here" -f guest.mk MB="$mb" TEST_ENGINES="$tests" -j"${JOBS:-4}" > "$root/build/package-make-$variant.log" 2>&1 || {
	tail -20 "$root/build/package-make-$variant.log" >&2; echo "the guest build failed (build/package-make-$variant.log)" >&2; exit 1; }
# where the code reads its thread pointer, listed in the image: an Intel Mac
# rewrites those reads as the core loads (miniBox's wbx-tp-rewrite.py)
python3 "$mb/source/guest/wbx-tp-rewrite.py" "$guest/core.wbx"
sh "$mb/source/guest/check-wbx.sh" "$guest/core.wbx"

staging="$root/build/package-staging-$variant"
rm -rf "$staging"
mkdir -p "$staging"
cp "$guest/core.wbx" "$staging/core.wbx"
cp "$here/waterbox.config" "$staging/waterbox.config"
cp "$here/default_keybinds.json" "$staging/default_keybinds.json"
# The core-declared file form for the project wizard - the frontend renders it,
# this file decides it.
cp "$here/file_slots.json" "$staging/file_slots.json"
# the texts a person reads, a table per language (chimera docs/translations.md)
cp -r "$here/languages" "$staging/languages"
# the terms travel with the binary: ScummVM and the libraries it is built with
# are somebody else's work (see
# waterbox/package-licenses.json)
python3 "$mb/source/guest/package-licenses.py" "$root" "$staging"

# ---- version (see chimera docs: commit-as-version, stamped by CD) ----
core_version="${CORE_VERSION:-}"
if [ -z "$core_version" ]; then
	if commit="$(git -C "$root" rev-parse --short=12 HEAD 2>/dev/null)"; then
		# the build applies patches/ inside extern/scummvm, which is not an edit of
		# this repo: a submodule's own working tree does not make the package dirty
		git -C "$root" diff --quiet --ignore-submodules=dirty HEAD 2>/dev/null || commit="$commit-dirty"
		core_version="$commit+local"
	else
		core_version="unversioned+local"
	fi
fi
# the test package says so in its version
if [ "$variant" = tests ]; then
	case "$core_version" in *+*) core_version="$core_version.tests" ;; *) core_version="$core_version+tests" ;; esac
fi
# When that commit was made, beside it (chimera issue #67): the COMMIT's date, in
# UTC, and never the build's - the same commit must still make the same package.
core_version_date="$(TZ=UTC git -C "$root" log -1 --date=format-local:%Y-%m-%dT%H:%M:%SZ --format=%cd HEAD 2>/dev/null || true)"
python3 - "$staging/waterbox.config" "$core_version" "$core_version_date" <<'PYVER'
import json, sys
path, version, date = sys.argv[1], sys.argv[2], sys.argv[3]
with open(path) as f:
    cfg = json.load(f)
cfg["version"] = version
if date:
    cfg["versionDate"] = date
with open(path, "w") as f:
    json.dump(cfg, f, indent=2)
    f.write("\n")
PYVER

# ---- provenance: what built this exact package (inputs only) ----
gccver="$(gcc -dumpfullversion)"
musl_version="$(cat "$mb/extern/musl/VERSION" 2>/dev/null || echo unknown)"
binutils_version="$(ld --version | head -1 | grep -o '[0-9][0-9.]*$' || echo unknown)"
os_id="$(. /etc/os-release 2>/dev/null && printf '%s %s' "${ID:-unknown}" "${VERSION_ID:-}" || echo unknown)"
guest_kit="$(git -C "$mb" rev-parse --short=12 HEAD 2>/dev/null || echo unknown)"
svm_pin="$(git -C "$root/extern/scummvm" rev-parse --short=12 HEAD 2>/dev/null || echo unknown)"
python3 - "$staging/build.json" <<PYPROV
import json, subprocess
def git(*args, default="unknown"):
    try:
        return subprocess.run(["git", "-C", "$root", *args], capture_output=True,
                              text=True, check=True).stdout.strip()
    except Exception:
        return default
# where the source came from, when it is a repository on the network; a
# clone of a folder on this machine would put that folder's path (and the
# user's name in it) into the package
origin = git("config", "--get", "remote.origin.url", default="")
if not (("://" in origin and not origin.startswith("file://")) or origin.startswith("git@")):
    origin = ""
json.dump({
    "version": "$core_version",
    "variant": "$variant",
    "source": {"commit": git("rev-parse", "HEAD"),
               "origin": origin,
               "dirty": "-dirty" in "$core_version"},
    "toolchain": {"compiler": "gcc $gccver", "binutils": "$binutils_version",
                  "target": "x86_64-linux-musl", "musl": "$musl_version"},
    "guestKit": {"name": "miniBox", "commit": "$guest_kit"},
    "upstream": {"name": "ScummVM", "pin": "$svm_pin"},
    "builtOn": "$os_id",
}, open("$staging/build.json", "w"), indent=2, sort_keys=True)
PYPROV

mkdir -p "$out"
zip_path="$out/scummvm.chimeraCore"
rm -f "$zip_path"
# deterministic packaging: sorted entries, fixed timestamp/permissions, pinned
# compression - the package's SHA1 is the core's identity (movies cite it)
python3 - "$staging" "$zip_path" <<'PYEOF'
import hashlib, os, sys, tempfile, zipfile

staging, zip_path = sys.argv[1], sys.argv[2]
FIXED_DATE = (1980, 1, 1, 0, 0, 0)

def write_package(path):
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for root, dirs, files in os.walk(staging):
            dirs.sort()
            for name in sorted(files):
                full = os.path.join(root, name)
                info = zipfile.ZipInfo(os.path.relpath(full, staging), date_time=FIXED_DATE)
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = 0o644 << 16
                with open(full, "rb") as f:
                    z.writestr(info, f.read())

write_package(zip_path)
with tempfile.NamedTemporaryFile(suffix=".zip") as tmp:
    write_package(tmp.name)
    again = hashlib.sha1(open(tmp.name, "rb").read()).hexdigest()
first = hashlib.sha1(open(zip_path, "rb").read()).hexdigest()
if first != again:
    sys.exit(f"packaging is not deterministic: {first} then {again}")
print(f"package sha1 {first}")
PYEOF

if [ -n "$chimera_root" ]; then
	for cache in "$chimera_root"/build/CoreCache/scummvm-*; do
		[ -d "$cache" ] && rm -rf "$cache" || true
	done
fi
echo "packaged -> $zip_path"
