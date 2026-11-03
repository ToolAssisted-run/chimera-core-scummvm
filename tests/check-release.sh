#!/bin/bash
# The published package's own checks (the test package's are run-gate.sh and
# run-frontend.sh). The published package is built without ScummVM's two
# test engines (build-package.sh without -t), so it cannot play the tests'
# games; this checks, through Chimera's chimera-run, that:
#
# - release:variant   it says it is the published build (build.json) and its
#                     version does not end in "tests"
# - release:no-tests  a project of testbed's game and one of Playground 3D's
#                     are refused: ScummVM's detector (which knows every
#                     engine's games) finds each, and there is no engine to
#                     run it, "Could not find suitable engine plugin"
# - release:game      Beneath a Steel Sky (scummvm.org's freeware; -g, or SKIP
#                     without it), 1500 frames with Escape and a click, in a
#                     project: the last picture is the native reference's
# - release:contract  Chimera's contract tests (test-packages) on its folder
#
# usage: tests/check-release.sh --chimera-root <chimera> --package <scummvm.chimeraCore> [-g <freeware games>]
# Needs the native reference built (make -f waterbox/native.mk).
set -u

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
chimera_root=""
pkg=""
games="${SCUMMVM_GAMES:-}"
while [ $# -gt 0 ]; do
	case "$1" in
		--chimera-root) chimera_root="$2"; shift ;;
		--package) pkg="$2"; shift ;;
		-g) games="$2"; shift ;;
		*) echo "unknown option: $1" >&2; exit 2 ;;
	esac
	shift
done
[ -n "$chimera_root" ] && [ -d "$chimera_root" ] || { echo "say where Chimera is: --chimera-root <chimera checkout>" >&2; exit 2; }
[ -n "$pkg" ] && [ -f "$pkg" ] || { echo "say which package: --package <scummvm.chimeraCore>" >&2; exit 2; }
chimera_root="$(cd "$chimera_root" && pwd)"
pkg="$(cd "$(dirname "$pkg")" && pwd)/$(basename "$pkg")"
crun="$chimera_root/build/dll/chimera-run"
[ -x "$crun" ] || crun="$chimera_root/build/meson-linux/chimera-run"
contract="$chimera_root/build/meson-linux/source/ui/test-packages"
rn="$root/build/native/run-native"
work="$root/build/release-check"
rm -rf "$work"
mkdir -p "$work"

ok=0; failed=0; skipped=0
report() {
	printf "%-18s %-5s %s\n" "$1" "$2" "$3"
	case "$2" in PASS) ok=$((ok+1)) ;; SKIP) skipped=$((skipped+1)) ;; *) failed=$((failed+1)) ;; esac
}
[ -x "$crun" ] || { echo "no chimera-run at $crun" >&2; exit 2; }

# ---------------------------------------------------------------- what it says it is
unzip -p "$pkg" build.json > "$work/build.json" 2>/dev/null
unzip -p "$pkg" waterbox.config > "$work/waterbox.config" 2>/dev/null
variant="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1])).get("variant",""))' "$work/build.json" 2>/dev/null)"
version="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1])).get("version",""))' "$work/waterbox.config" 2>/dev/null)"
if [ "$variant" = release ] && [ -n "$version" ] && [ "${version%tests}" = "$version" ]; then
	report "release:variant" PASS "build.json says \"release\"; version $version"
else
	report "release:variant" FAIL "variant '$variant', version '$version'"
fi

# ---------------------------------------------------------------- no test engines
refused() { # refused <name> <zip>
	local dir="$work/$1"
	mkdir -p "$dir"
	cp "$2" "$dir/game.zip"
	python3 "$here/make-project.py" "$pkg" "$dir/p.chimeraProject" 10 -- "game=$dir/game.zip" > /dev/null
	( cd "$dir" && timeout 300 "$crun" --project "$dir/p.chimeraProject" "$pkg" --files "$dir" --frames 10 \
		--meta "$dir/p.meta" > "$dir/p.log" 2>&1 )
	if grep -q '^status=ERROR' "$dir/p.meta" 2>/dev/null && grep -q 'Could not find suitable engine plugin' "$dir/p.meta"; then
		echo "refused"
	else
		echo "$(grep -m1 '^status=' "$dir/p.meta" 2>/dev/null) $(grep -m1 '^detail=' "$dir/p.meta" 2>/dev/null)"
	fi
}
python3 "$here/make-testbed.py" "$work/testbed.zip"
python3 "$here/make-playground3d.py" "$work/playground3d.zip"
tb="$(refused testbed "$work/testbed.zip")"
pg="$(refused playground3d "$work/playground3d.zip")"
if [ "$tb" = refused ] && [ "$pg" = refused ]; then
	report "release:no-tests" PASS "testbed's game and Playground 3D's are found and refused, \"Could not find suitable engine plugin\": the test engines are not in it"
else
	report "release:no-tests" FAIL "testbed: $tb; Playground 3D: $pg"
fi

# ---------------------------------------------------------------- a game plays
if [ -n "$games" ] && [ -f "$games/BASS-Floppy-1.3.zip" ]; then
	if [ ! -x "$rn" ]; then
		report "release:game" FAIL "the native reference is not built ($rn)"
	else
		dir="$work/sky"
		mkdir -p "$dir/native"
		cp "$games/BASS-Floppy-1.3.zip" "$dir/sky.zip"
		cp "$games/BASS-Floppy-1.3.zip" "$dir/native/sky.zip"
		printf '{"game":["sky.zip"]}' > "$dir/native/slots"
		printf '{}' > "$dir/native/settings"
		# Escape at 300 skips the intro; a click at (100,88) at 480 (the
		# mouse put there a frame before the button is held 3 frames)
		ax=$((100 * 65536 / 320)); ay=$((88 * 65536 / 200))
		"$rn" "$dir/native" --frames 1500 --press 300:KeyEscape:2 --press "480:X=$ax,Y=$ay:1" \
			--press "485:X=$ax,Y=$ay,MouseLeftButton:3" --screenshot "1499:$dir/native.tga" > "$dir/native.txt" 2> "$dir/native.err"
		python3 "$here/make-project.py" "$pkg" "$dir/p.chimeraProject" 1500 "press=Key Escape:300:2" \
			"axis=Mouse Position X:480:$ax" "axis=Mouse Position Y:480:$ay" "press=Mouse Left Button:485:3" \
			-- "game=$dir/sky.zip" > /dev/null
		( cd "$dir" && timeout 900 "$crun" --project "$dir/p.chimeraProject" "$pkg" --files "$dir" --frames 1500 \
			--screenshot "1499=$dir/run.tga" --meta "$dir/p.meta" > "$dir/p.log" 2>&1 )
		if grep -q '^status=OK' "$dir/p.meta" 2>/dev/null &&
		   python3 "$here/compare-pictures.py" "$dir/native.tga" "$dir/run.tga" > "$dir/pic.txt" 2>&1; then
			report "release:game" PASS "Beneath a Steel Sky, 1500 frames (Escape, a click) in a project: the last picture is the native reference's, $(cat "$dir/pic.txt")"
		else
			report "release:game" FAIL "$(grep -m1 '^detail=' "$dir/p.meta" 2>/dev/null) $(cat "$dir/pic.txt" 2>/dev/null) (build/release-check/sky)"
		fi
	fi
else
	report "release:game" SKIP "needs BASS-Floppy-1.3.zip in a folder given with -g (tests/fetch-freeware.sh)"
fi

# ---------------------------------------------------------------- Chimera's contract tests
if [ -x "$contract" ]; then
	mkdir -p "$work/cores"
	cp "$pkg" "$work/cores/scummvm.chimeraCore"
	if CHIMERA_CORES_DIR="$work/cores" timeout 1500 "$contract" "$work/cores" > "$work/contract.txt" 2>&1; then
		report "release:contract" PASS "$(grep -c '^PASS' "$work/contract.txt") of Chimera's package checks pass ($(tail -1 "$work/contract.txt"))"
	else
		report "release:contract" FAIL "$(grep -m3 'FAIL' "$work/contract.txt" | tr '\n' ' ')"
	fi
else
	report "release:contract" SKIP "Chimera's test-packages is not built ($contract)"
fi

echo
echo "$ok ok, $failed failed, $skipped skipped"
[ "$failed" -eq 0 ]
