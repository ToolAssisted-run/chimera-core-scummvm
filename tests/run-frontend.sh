#!/bin/bash
# The frontend half of the tests: the package, as Chimera loads it, playing
# ScummVM's own test engine (its data made by tests/make-testbed.py from
# ScummVM's tree, so it needs no game of anyone's and runs anywhere).
#
# - chimera-run (the engine, no window): a movie made into a project by
#   chimera-run, and the same movie in a hand-written project. The last
#   picture must be the native reference's (run-native) at that frame. And
#   a project of ScummVM's 3D test engine (tests/make-playground3d.py) drawn
#   with the core's OpenGL: its last picture the reference's too.
# - Chimera's window, run headless (no display needed): the same project
#   opened and played through Lua, its last picture compared with the
#   reference's; and the package's bindings becoming Chimera's defaults.
#
# The movie clicks through testbed's welcome window into its pixel format
# test: the mouse's place is the two absolute axes, so a click there is the
# axes set and the left button held.
#
# Nothing is written under the Chimera checkout: the window's settings, its
# cache and recovery files, the Base path and the sandbox's log are all in
# build/frontend.
#
# Usage: tests/run-frontend.sh --chimera-root <chimera checkout> [-m <miniBox dir>] [--package <scummvm.chimeraCore>]
# Needs: the native reference built (make -f waterbox/native.mk), and
# Chimera built (its window and chimera-run). It plays ScummVM's test
# engines, so the package is the test package: --package tests one already
# built (build-package.sh -t), else one is made. The published package,
# which has no test engines, is tests/check-release.sh's. Chimera's window finds Qt through
# QT_LIB_DIR (Qt's lib folder) when its libraries are not where it looks.
set -u

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
wb="$root/waterbox"
chimera_root=""
mb="${MINIBOX_DIR:-}"
given_pkg=""
while [ $# -gt 0 ]; do
	case "$1" in
		--chimera-root) chimera_root="$2"; shift ;;
		--package) given_pkg="$2"; shift ;;
		-m) mb="$2"; shift ;;
		*) echo "unknown option: $1" >&2; exit 2 ;;
	esac
	shift
done
[ -n "$chimera_root" ] && [ -d "$chimera_root" ] || { echo "say where Chimera is: --chimera-root <chimera checkout>" >&2; exit 2; }
chimera_root="$(cd "$chimera_root" && pwd)"
[ -n "$mb" ] || mb="$chimera_root/extern/chimera-common-minibox"

chimera_exe="$chimera_root/build/meson-linux/source/ui/chimera"
crun="$chimera_root/build/dll/chimera-run"
[ -x "$crun" ] || crun="$chimera_root/build/meson-linux/chimera-run"
rn="$root/build/native/run-native"
work="$root/build/frontend"
rm -rf "$work"
mkdir -p "$work/native" "$work/proj"
game="$work/proj/testbed.zip"
python3 "$here/make-testbed.py" "$game"

ok=0
failed=0
skipped=0
report() {
	printf "%-22s %-6s %s\n" "$1" "$2" "$3"
	case "$2" in PASS) ok=$((ok+1)) ;; SKIP) skipped=$((skipped+1)) ;; *) failed=$((failed+1)) ;; esac
}
printf "%-22s %-6s %s\n" "Check" "Result" "Detail"
printf "%-22s %-6s %s\n" "-----" "------" "------"

[ -x "$rn" ] || { echo "the native reference is not built: $rn (make -C waterbox -f native.mk)" >&2; exit 1; }

# ---------------------------------------------------------------- the package
if [ -n "$given_pkg" ] && mkdir -p "$work/pkg" && cp "$given_pkg" "$work/pkg/scummvm.chimeraCore"; then
	report "package" PASS "given: $given_pkg ($(sha1sum "$given_pkg" | cut -c1-40))"
elif bash "$wb/build-package.sh" -t -m "$mb" -o "$work/pkg" > "$work/package.log" 2>&1; then
	report "package" PASS "$(grep 'package sha1' "$work/package.log")"
else
	report "package" FAIL "see build/frontend/package.log"; echo; echo "$ok ok, $failed failed"; exit 1
fi
pkg="$work/pkg/scummvm.chimeraCore"

# The movie: Proceed (119,142) at frame 300, OK (119,142) at 600 and OK
# (160,157) at 800 (testbed's pixel format test shows its 8-bit example),
# then Yes (118,127) at 1000 (the first 32-bit format looked at). A click is
# the mouse put there a frame before the left button is held for 3 frames.
frames=1100
clicks=(300:119:142 600:119:142 800:160:157 1000:118:127)
press=()
proj=()
for c in "${clicks[@]}"; do
	IFS=: read -r at x y <<< "$c"
	ax=$((x * 65536 / 320)); ay=$((y * 65536 / 200))
	press+=(--press "$at:X=$ax,Y=$ay:1" --press "$((at + 5)):X=$ax,Y=$ay,MouseLeftButton:3")
	proj+=("axis=Mouse Position X:$at:$ax" "axis=Mouse Position Y:$at:$ay" "press=Mouse Left Button:$((at + 5)):3")
done
cp "$game" "$work/native/testbed.zip"
printf '{"game":["testbed.zip"]}' > "$work/native/slots"
printf '{}' > "$work/native/settings"
"$rn" "$work/native" --frames "$frames" "${press[@]}" \
	--screenshot "$((frames - 1)):$work/native.last.tga" > "$work/native.txt" 2> "$work/native.err"
python3 "$here/make-project.py" "$pkg" "$work/proj/test.chimeraProject" "$frames" "${proj[@]}" log="$work/test.log" \
	-- "game=$game"

# ---------------------------------------------------------------- chimera-run
if [ ! -x "$crun" ]; then
	report "engine" SKIP "no chimera-run at $crun"
else
	# a game runs only in a project (Chimera, 2026-10-26): the game and the movie
	# made into one by chimera-run
	( cd "$work" && "$crun" --new-project "$work/movie.chimeraProject" "$pkg" "$game" test.log > "$work/movie.make.log" 2>&1 \
		&& timeout 900 "$crun" --project "$work/movie.chimeraProject" "$pkg" --files "$(dirname "$game")" --frames "$frames" \
		--screenshot "$((frames - 1))=$work/run.last.tga" --meta "$work/run.meta" > "$work/run.log" 2>&1 )
	if grep -q '^status=OK' "$work/run.meta" 2>/dev/null &&
	   python3 "$here/compare-pictures.py" "$work/native.last.tga" "$work/run.last.tga" > "$work/run.pic" 2>&1; then
		report "engine:movie" PASS "$frames frames from a movie made into a project (testbed's welcome, its pixel format test, four clicks): the last picture is the reference's, $(cat "$work/run.pic")"
	else
		report "engine:movie" FAIL "$(grep -m1 detail= "$work/run.meta" 2>/dev/null) $(cat "$work/run.pic" 2>/dev/null) (build/frontend/run.log)"
	fi

	( cd "$work" && timeout 900 "$crun" --project "$work/proj/test.chimeraProject" "$pkg" --files "$work/proj" --frames "$frames" \
		--screenshot "$((frames - 1))=$work/proj.last.tga" --meta "$work/proj.meta" > "$work/proj.log" 2>&1 )
	if grep -q '^status=OK' "$work/proj.meta" 2>/dev/null &&
	   python3 "$here/compare-pictures.py" "$work/native.last.tga" "$work/proj.last.tga" > "$work/proj.pic" 2>&1; then
		report "engine:project" PASS "the same movie in a hand-written project, the zip in its slot: the same last picture"
	else
		report "engine:project" FAIL "$(grep -m1 detail= "$work/proj.meta" 2>/dev/null) $(cat "$work/proj.pic" 2>/dev/null) (build/frontend/proj.log)"
	fi
fi

# The 3D test engine with the core's OpenGL (shaders): a click a scene, the
# last one (a green quad and a red triangle) pictured
if [ -x "$crun" ]; then
	mkdir -p "$work/native3d" "$work/proj3d"
	python3 "$here/make-playground3d.py" "$work/proj3d/playground3d.zip"
	cp "$work/proj3d/playground3d.zip" "$work/native3d/"
	printf '{"game":["playground3d.zip"]}' > "$work/native3d/slots"
	printf '{"renderer":"opengl_shaders"}' > "$work/native3d/settings"
	p3=(); r3=()
	for k in 1 2 3 4 5; do p3+=("press=Mouse Left Button:$((k * 120)):3"); r3+=(--press "$((k * 120)):MouseLeftButton:3"); done
	"$rn" "$work/native3d" --frames 720 "${r3[@]}" --screenshot "719:$work/native3d.last.tga" > "$work/native3d.txt" 2> "$work/native3d.err"
	python3 "$here/make-project.py" "$pkg" "$work/proj3d/pg.chimeraProject" 720 renderer='"opengl_shaders"' "${p3[@]}" \
		-- "game=$work/proj3d/playground3d.zip"
	( cd "$work" && timeout 900 "$crun" --project "$work/proj3d/pg.chimeraProject" "$pkg" --files "$work/proj3d" --frames 720 \
		--screenshot "719=$work/proj3d.last.tga" --meta "$work/proj3d.meta" > "$work/proj3d.log" 2>&1 )
	if grep -q '^status=OK' "$work/proj3d.meta" 2>/dev/null &&
	   python3 "$here/compare-pictures.py" "$work/native3d.last.tga" "$work/proj3d.last.tga" > "$work/proj3d.pic" 2>&1; then
		report "engine:3d" PASS "Playground 3D, OpenGL with shaders, 720 frames and five clicks in a project: the last picture is the reference's, $(cat "$work/proj3d.pic")"
	else
		report "engine:3d" FAIL "$(grep -m1 detail= "$work/proj3d.meta" 2>/dev/null) $(cat "$work/proj3d.pic" 2>/dev/null) (build/frontend/proj3d.log)"
	fi
fi

# ---------------------------------------------------------------- Chimera itself
if [ ! -x "$chimera_exe" ]; then
	report "window" SKIP "Chimera's window is not built ($chimera_exe)"
	echo; echo "$ok ok, $failed failed, $skipped skipped"; [ "$failed" -eq 0 ]; exit
fi
[ -n "${QT_LIB_DIR:-}" ] && export LD_LIBRARY_PATH="$QT_LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# the window run headless: the package's own folder as the cores folder, its settings,
# cache and recovery files the run's own, and nothing carried over from a
# Chimera installed on this machine
export ALSOFT_DRIVERS=null CHIMERA_CORES_DIR="$work/pkg" CHIMERA_QT_CACHE="$work/qt-cache" CHIMERA_QT_RECOVERY="$work/qt-recovery" \
	CHIMERA_QT_HASHES="$work/qt-file-hashes.txt" CHIMERA_QT_CHIMERA_SETTINGS="$work/no-chimera-config.ini"
export MINIBOX_LOG="$work/minibox.log"

# settings of its own, with the Base path (and so every folder Chimera
# writes by default) in the work dir
config="$work/settings.json"
python3 -c 'import json, sys; json.dump({"paths.base": sys.argv[2]}, open(sys.argv[1], "w"), indent=2)' "$config" "$work"

gui() {
	# gui <tag> <config> <project> <frames>
	local tag="$1" cfg="$2" project="$3" nframes="$4"
	mkdir -p "$work/$tag"
	printf 'frames=%s\nmeta=%s\nshot=%s\n' "$nframes" "$work/$tag/meta.txt" "$work/$tag/last.png" > "$work/$tag/job.txt"
	( cd "$work" && MINIHAWK_JOB="$work/$tag/job.txt" timeout 900 "$chimera_exe" --headless "--config=$cfg" \
		"--core=$pkg" "--project=$project" "--lua=$here/frontend.lua" ) > "$work/$tag.log" 2>&1
}
meta() { sed -n "s/^$2=//p" "$work/$1/meta.txt" 2>/dev/null; }

gui gui "$config" "$work/proj/test.chimeraProject" "$frames"
if [ "$(meta gui status)" = "OK" ] && python3 "$here/compare-pictures.py" "$work/native.last.tga" "$work/gui/last.png" > "$work/gui.pic" 2>&1; then
	report "window:project" PASS "the project opened in Chimera: after $(meta gui frames) frames, $(meta gui width)x$(meta gui height), the picture is the reference's"
else
	report "window:project" FAIL "status $(meta gui status) $(meta gui detail); $(cat "$work/gui.pic" 2>/dev/null) (build/frontend/gui.log)"
fi

# the bindings the package ships are the machine's when nobody changed them
# (the window keeps only the keys a person changed; fresh settings have none)
mkdir -p "$work/keys"
printf 'out=%s/keys/controls.json\nmeta=%s/keys/meta.txt\n' "$work" "$work" > "$work/keys/job.txt"
( cd "$work" && MINIHAWK_JOB="$work/keys/job.txt" timeout 900 "$chimera_exe" --headless "--config=$work/settings.keys.json" \
	"--core=$pkg" "--project=$work/proj/test.chimeraProject" "--lua=$here/dump-controls.lua" ) > "$work/keys.log" 2>&1
if grep -q "^status=OK" "$work/keys/meta.txt" 2>/dev/null && python3 "$here/check-keybinds.py" "$work/keys/controls.json" "$wb/default_keybinds.json" "ScummVM" "Mouse Left Button" > "$work/keys.txt" 2>&1; then
	report "window:keybinds" PASS "$(cat "$work/keys.txt")"
else
	report "window:keybinds" FAIL "$(head -1 "$work/keys.txt" 2>/dev/null) (build/frontend/keys.log)"
fi

echo
echo "$ok ok, $failed failed, $skipped skipped"
[ "$failed" -eq 0 ]
