#!/bin/bash
# run-gate.sh - the ScummVM core's tests: the native reference (run-native)
# and the sandbox (core.wbx through run-wbx) are the same machine.
#
# The test games: ScummVM's own test engine (testbed, its data made by
# tests/make-testbed.py from ScummVM's tree, so it runs anywhere), and
# scummvm.org's freeware games (tests/fetch-freeware.sh downloads them; give
# the folder with -g or SCUMMVM_GAMES). Without the freeware games their tests
# say SKIP and why. The Roland MT-32 test needs the MT-32's ROMs (-r <folder>
# or SCUMMVM_ROMS, holding MT32_CONTROL.ROM and MT32_PCM.ROM) and says SKIP
# without them.
#
# Every test says what it compared. The tests:
#   testbed:formats   ScummVM's pixel format test, clicked through: native =
#                     sandbox, and each 32-bit format's pattern is the 8-bit
#                     example's, pixel for pixel
#   testbed:saves     ScummVM's save file tests (testbed's SaveGames suite,
#                     chosen in its Customize window): all five pass, native and
#                     sandbox; the files they leave are what Export Save Data
#                     gives, the same in both builds
#   testbed:files     testbed's file reading test: the zip's folders listed
#                     and their files opened (in folders named in four cases;
#                     ScummVM matches the case itself)
#   rerecord, session the sandbox saved and loaded before every frame, and
#                     taken to a new host part way, = straight
#   turbo             the first half not drawn: the rest the same, and the
#                     first half's pictures really not there
#   input:click       a click reaches ScummVM where the mouse axes put it; a
#                     click one frame later is another run (the comparison
#                     sees one frame)
#   input:keys        Escape skips Beneath a Steel Sky's intro
#   time:virtual      the computer stalling between frames changes nothing;
#                     the native build reading the computer's clock (the
#                     control) gives another machine
#   time:date         another clock start (the project's, Chimera's
#                     --clock-start) gives another run of Flight of the Amazon
#                     Queen (ScummVM seeds its random numbers from the date),
#                     native and sandbox the same
#   savedata:import   a project's save data zip is in the machine from frame 0
#   <game>            each freeware game, from power-on through its intro with
#                     keys and clicks: native = sandbox
#   mt32              Lure of the Temptress's music on the MT-32 emulation:
#                     native = sandbox, and it is heard
#   refusals          no game, not a zip, a zip of no game, a member
#                     compressed in a way the core cannot read, a broken save
#                     zip, a Roland device without its ROMs: each says why
#   texts             the language tables: every key the code names is there,
#                     every name is written as it is
#
# usage: run-gate.sh -m <miniBox checkout> [-g <freeware games>] [-r <ROMs>]
set -u
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
mb="${MINIBOX_DIR:-}"
games="${SCUMMVM_GAMES:-}"
roms="${SCUMMVM_ROMS:-}"
while getopts "m:g:r:" opt; do
	case "$opt" in
		m) mb="$OPTARG" ;;
		g) games="$OPTARG" ;;
		r) roms="$OPTARG" ;;
		*) exit 2 ;;
	esac
done
[ -n "$mb" ] && [ -d "$mb" ] || { echo "pass -m <miniBox checkout>" >&2; exit 2; }
mb="$(cd "$mb" && pwd)"

echo "== build (miniBox $mb)"
mkdir -p "$root/build"
step() { # step <what> <log> <command...>
	local what="$1" log="$2"; shift 2
	if ! "$@" > "$log" 2>&1; then
		grep "error:" "$log" | head -10; tail -15 "$log"; echo "$what failed ($log)"; exit 1
	fi
}
jobs="${JOBS:-4}"
step "the native build" "$root/build/gate-native.log" make -C "$here" -f native.mk -j"$jobs" MB="$mb"
step "the guest build" "$root/build/gate-guest.log" make -C "$here" -f guest.mk -j"$jobs" MB="$mb"
native="$root/build/native/run-native"
wbx="$root/build/native/run-wbx"
core="$root/build/guest/core.wbx"

work="$root/build/gate"
rm -rf "$work"
mkdir -p "$work"
passed=0
fails=0
skips=0
pass() { echo "PASS $*"; passed=$((passed + 1)); }
fail() { echo "FAIL $*"; fails=$((fails + 1)); }
skip() { echo "SKIP $*"; skips=$((skips + 1)); }

# what a run says: the digests (standard output) and what ScummVM's engines
# reported (testbed's results, on the error output)
digests() { { grep -E '^[A-Za-z]+(\[[^]]*\])?=' "$1"; grep -E '^(Result|Passed|Subsystem|Info! (Executing|Testing))' "${1%.txt}.err" 2>/dev/null; } || true; }
same() { # same <test> <a.txt> <b.txt> <what>
	digests "$2" > "$2.d"
	digests "$3" > "$3.d"
	if grep -q '^frames=' "$2.d" && cmp -s "$2.d" "$3.d"; then
		pass "$1: $4"
	else
		fail "$1: $4"; diff "$2.d" "$3.d" | head -20
	fi
}
differ() { # differ <test> <a.txt> <b.txt> <what>
	digests "$2" > "$2.d"
	digests "$3" > "$3.d"
	if grep -q '^frames=' "$2.d" && grep -q '^frames=' "$3.d" && ! cmp -s "$2.d" "$3.d"; then
		pass "$1: $4"
	else
		fail "$1: $4 (the runs are the same, or one did not run)"
	fi
}
value() { grep -E "^$2=" "$1" | head -1 | sed "s/^$2=//"; }
mkwork() { # mkwork <dir> <game zip> [<settings JSON>] [<save zip>]
	mkdir -p "$work/$1"
	cp "$2" "$work/$1/game.zip"
	if [ -n "${4:-}" ]; then
		cp "$4" "$work/$1/saves.zip"
		printf '{"game":["game.zip"],"savedata":["saves.zip"]}' > "$work/$1/slots"
	else
		printf '{"game":["game.zip"]}' > "$work/$1/slots"
	fi
	printf '%s' "${3:-{\}}" > "$work/$1/settings"
}
# the screen is 320x200 in these tests: a click at pixel (x, y) is the mouse
# axes at x and y scaled to 0..65535, and the button held 3 frames
click() { # click <frame> <x> <y>
	local ax=$(($2 * 65536 / 320)) ay=$(($3 * 65536 / 200))
	echo "--press $1:X=$ax,Y=$ay:1 --press $(($1 + 5)):X=$ax,Y=$ay,MouseLeftButton:3"
}
both() { # both <name> <dir> <frames> <options...>: n.txt and w.txt in <dir>
	local name="$1" dir="$2" frames="$3"; shift 3
	"$native" "$work/$dir" --frames "$frames" "$@" > "$work/$name.n.txt" 2> "$work/$name.n.err"
	"$wbx" "$core" "$work/$dir" --frames "$frames" "$@" > "$work/$name.w.txt" 2> "$work/$name.w.err"
}

testbed="$work/testbed.zip"
python3 "$root/tests/make-testbed.py" "$testbed"

# ---------------------------------------------------------------- testbed
# The welcome window: Proceed (119,142) runs every suite, starting with the
# pixel formats; OK (119,142) shows the 8-bit example; then each format asks
# whether its pattern is the example's: Yes (118,127), and a new group of
# formats shows its 8-bit example first: OK (160,157). Clicks at both places
# every 200 frames go through the six formats.
formats=($(click 300 119 142) $(click 600 119 142) $(click 800 160 157))
shots=(--screenshot 795:"$work/fmt-example.tga")
for k in 0 1 2; do
	at=$((1000 + k * 200))
	formats+=($(click "$at" 118 127))
	shots+=(--screenshot "$((at - 5)):$work/fmt-$k.tga")
done
mkwork tb "$testbed"
echo "== testbed: the pixel formats (1600 frames)"
both fmt tb 1600 "${formats[@]}" "${shots[@]}"
same testbed:formats "$work/fmt.n.txt" "$work/fmt.w.txt" "native = sandbox, 1600 frames: every picture, the sound, the steps, the lag, testbed's reports"
for k in 0 1 2; do
	fmtname="$(grep -o 'Testing Pixel Format: [A-Za-z0-9@]*' "$work/fmt.n.err" | sed -n "$((k + 1))p" | sed 's/.*: //')"
	if python3 "$root/tests/compare-pictures.py" --region 2,9,316,18 "$work/fmt-example.tga" "$work/fmt-$k.tga" > "$work/fmt-$k.cmp" 2>&1; then
		pass "testbed:formats: ${fmtname:-format $k}: the pattern is the 8-bit example's, every pixel"
	else
		fail "testbed:formats: ${fmtname:-format $k}: $(cat "$work/fmt-$k.cmp")"
	fi
done

# Customize (201,142), Deselect All (52,160), the SaveGames suite's box
# (29,130), Run tests (182,160)
saves=($(click 300 201 142) $(click 400 52 160) $(click 500 29 130) $(click 600 182 160))
echo "== testbed: the save file suite"
mkdir -p "$work/sv.n" "$work/sv.w"
"$native" "$work/tb" --frames 1500 "${saves[@]}" --savedata-out "$work/sv.n" > "$work/sv.n.txt" 2> "$work/sv.n.err"
"$wbx" "$core" "$work/tb" --frames 1500 "${saves[@]}" --savedata-out "$work/sv.w" > "$work/sv.w.txt" 2> "$work/sv.w.err"
if grep -q "Passed: 5 Skipped: 0 Failed: 0" "$work/sv.n.err" && grep -q "Passed: 5 Skipped: 0 Failed: 0" "$work/sv.w.err"; then
	pass "testbed:saves: ScummVM's five save file tests pass in both builds"
else
	fail "testbed:saves: $(grep '^Passed' "$work/sv.n.err") / $(grep '^Passed' "$work/sv.w.err")"
fi
same testbed:saves "$work/sv.n.txt" "$work/sv.w.txt" "native = sandbox: the pictures, the reports and the $(grep -c '^savedata' "$work/sv.n.txt") files Export Save Data gives"
if [ "$(grep -c '^savedata' "$work/sv.n.txt")" -ge 5 ] && diff -r "$work/sv.n" "$work/sv.w" > /dev/null; then
	pass "savedata:export: the exported files are the same bytes in both builds ($(cd "$work/sv.n" && ls | tr '\n' ' '))"
else
	fail "savedata:export"
fi

# the saves the suite left, given back as a project's save data zip, are in
# the machine from its first frame
python3 - "$work/sv.n" "$work/saves.zip" <<'PY'
import os, sys, zipfile
src, out = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for n in sorted(os.listdir(src)):
        z.write(os.path.join(src, n), n)
PY
mkwork tbs "$testbed" '{}' "$work/saves.zip"
mkdir -p "$work/imp.out"
"$wbx" "$core" "$work/tbs" --frames 1 --savedata-out "$work/imp.out" > "$work/imp.txt" 2> "$work/imp.err"
if [ "$(grep -c '^savedata' "$work/imp.txt")" -ge 5 ] && diff -r "$work/sv.n" "$work/imp.out" > /dev/null; then
	pass "savedata:import: a save data zip of $(grep -c '^savedata' "$work/imp.txt") files is in the machine at its first frame, byte for byte"
else
	fail "savedata:import: $(grep -c '^savedata' "$work/imp.txt") files"
fi

# the file system suite's reading test opens names of four cases
files=($(click 300 201 142) $(click 400 52 160) $(click 500 29 106) $(click 600 182 160))
both fs tb 1500 "${files[@]}"
reading="$(awk '/Executing Test: ReadingFile/ { on = 1 } on && /^Result:/ { print; exit }' "$work/fs.w.err")"
if [ "$reading" = "Result: Passed" ]; then
	pass "testbed:files: testbed lists the zip's folders and reads test1/file.txt, Test2/File.txt, TEST3/FILE.txt and tEST4/fILe.txt out of it"
else
	fail "testbed:files: ReadingFile: ${reading:-not run}"
fi

echo "== savestates (the pixel formats run)"
"$wbx" "$core" "$work/tb" --frames 1600 "${formats[@]}" --rerecord > "$work/fmt.r.txt" 2> "$work/fmt.r.err"
same rerecord "$work/fmt.r.txt" "$work/fmt.w.txt" "save+load before every frame = straight ($(sed -n 's/stateBytes=//p' "$work/fmt.r.err") state bytes)"
"$wbx" "$core" "$work/tb" --frames 1600 "${formats[@]}" --session-at 900 > "$work/fmt.s.txt" 2> "$work/fmt.s.err"
same session "$work/fmt.s.txt" "$work/fmt.w.txt" "saved at frame 900 (in the pixel format test), loaded into a new host = straight"

"$native" "$work/tb" --frames 1600 "${formats[@]}" --turbo > "$work/fmt.t.txt" 2> "$work/fmt.t.err"
digests "$work/fmt.t.txt" | grep -v '^videoHash=' > "$work/fmt.t.d"
digests "$work/fmt.n.txt" | grep -v '^videoHash=' > "$work/fmt.nn.d"
if cmp -s "$work/fmt.t.d" "$work/fmt.nn.d" && [ "$(value "$work/fmt.t.txt" videoHash)" != "$(value "$work/fmt.n.txt" videoHash)" ]; then
	pass "turbo: the first half not drawn - the second half's pictures, the sound, the steps and the reports the same; the first half's pictures not there"
else
	fail "turbo"; diff "$work/fmt.t.d" "$work/fmt.nn.d" | head
fi

late=($(click 301 119 142) $(click 600 119 142) $(click 800 160 157))
"$native" "$work/tb" --frames 1000 "${late[@]}" > "$work/late.txt" 2> "$work/late.err"
"$native" "$work/tb" --frames 1000 "${formats[@]}" > "$work/early.txt" 2> "$work/early.err"
differ input:click "$work/late.txt" "$work/early.txt" "Proceed clicked one frame later is another run"
"$native" "$work/tb" --frames 1000 > "$work/idle.txt" 2> "$work/idle.err"
if grep -q "Executing Test: pixelFormatsSupported" "$work/early.err" && ! grep -q "Executing Test" "$work/idle.err"; then
	pass "input:click: the click on Proceed, where the mouse axes put it, starts testbed's tests; without it they never start"
else
	fail "input:click: the click did not start the tests, or they started without it"
fi

# ---------------------------------------------------------------- refusals
echo "== refusals"
refused() { # refused <test> <dir> <key> <what>
	"$wbx" "$core" "$work/$2" --frames 5 > "$work/$2.txt" 2> "$work/$2.err"
	if grep -q "^loadError=$3" "$work/$2.txt"; then
		pass "refusal:$1: $4 ($(value "$work/$2.txt" loadError | tr '\037' ' '))"
	else
		fail "refusal:$1: $4 - $(value "$work/$2.txt" loadError | tr '\037' ' ')"
	fi
}
mkdir -p "$work/r-noslot"; printf '{}' > "$work/r-noslot/slots"; printf '{}' > "$work/r-noslot/settings"
refused no-game r-noslot error.no_game_slot "a project with no game"
mkdir -p "$work/r-text"; printf 'not a zip\n' > "$work/r-text/game.zip"; printf '{"game":["game.zip"]}' > "$work/r-text/slots"; printf '{}' > "$work/r-text/settings"
refused not-zip r-text error.not_zip "a file that is not a zip"
python3 -c 'import zipfile,sys; zipfile.ZipFile(sys.argv[1],"w").writestr("readme.txt","no game here\n")' "$work/nogame.zip"
mkwork r-nogame "$work/nogame.zip"
refused unknown r-nogame error.no_game "a zip of no game ScummVM knows"
python3 -c 'import zipfile,sys; zipfile.ZipFile(sys.argv[1],"w",zipfile.ZIP_BZIP2).writestr("TESTBED","ScummVM rocks!\n")' "$work/bzip2.zip"
mkwork r-bzip2 "$work/bzip2.zip"
refused method r-bzip2 error.zip_method "a zip member compressed with bzip2"
printf 'broken\n' > "$work/badsave.zip"
mkwork r-badsave "$testbed" '{}' "$work/badsave.zip"
refused save r-badsave error.not_zip "a save data file that is not a zip"
mkwork r-rom "$testbed" '{"musicDriver":"mt32"}'
refused rom r-rom error.rom_missing "the MT-32 chosen without its ROMs"

# ---------------------------------------------------------------- the freeware games
game_zip() { [ -n "$games" ] && [ -f "$games/$1" ] && echo "$games/$1"; }
if [ -z "$games" ]; then
	skip "games: no folder of freeware games (tests/fetch-freeware.sh <folder>, then -g <folder>)"
fi

# Beneath a Steel Sky: Escape at 300 skips the intro; a click at (100,88)
# walks Foster along the gantry
if zip="$(game_zip BASS-Floppy-1.3.zip)"; then
	echo "== Beneath a Steel Sky (floppy, freeware)"
	mkwork sky "$zip"
	walk=(--press 300:KeyEscape:2 $(click 480 100 88))
	both sky sky 1500 "${walk[@]}"
	same sky "$work/sky.n.txt" "$work/sky.w.txt" "native = sandbox, 1500 frames: the intro skipped, Foster walks where clicked"
	"$native" "$work/sky" --frames 1500 > "$work/sky-idle.txt" 2> /dev/null
	differ input:keys "$work/sky-idle.txt" "$work/sky.n.txt" "Escape and a click change the game"
	"$native" "$work/sky" --frames 1500 --press 300:KeyEscape:2 $(click 480 200 88) > "$work/sky-there.txt" 2> /dev/null
	differ input:axes "$work/sky-there.txt" "$work/sky.n.txt" "a click at another place (200,88) is another walk"
	GATE_STALL_US=300000 GATE_STALL_EVERY=250 "$native" "$work/sky" --frames 1500 "${walk[@]}" > "$work/sky-stall.txt" 2> "$work/sky-stall.err"
	same time:virtual "$work/sky-stall.txt" "$work/sky.n.txt" "the computer waiting 0.3 s every 250 frames changes nothing"
	GATE_STALL_US=300000 GATE_STALL_EVERY=250 "$native" "$work/sky" --frames 1500 "${walk[@]}" --host-clock > "$work/sky-host.txt" 2> "$work/sky-host.err"
	differ time:virtual "$work/sky-host.txt" "$work/sky.n.txt" "the control: on the computer's clock, the same waits give another machine"
	"$wbx" "$core" "$work/sky" --frames 1500 "${walk[@]}" --rerecord > "$work/sky.r.txt" 2> "$work/sky.r.err"
	same rerecord "$work/sky.r.txt" "$work/sky.w.txt" "Beneath a Steel Sky: save+load before every frame = straight ($(sed -n 's/stateBytes=//p' "$work/sky.r.err") state bytes)"
	"$wbx" "$core" "$work/sky" --frames 1500 "${walk[@]}" --session-at 700 > "$work/sky.s.txt" 2> /dev/null
	same session "$work/sky.s.txt" "$work/sky.w.txt" "Beneath a Steel Sky: saved at frame 700, loaded into a new host = straight"
else
	[ -n "$games" ] && skip "sky: no BASS-Floppy-1.3.zip in $games"
fi

freeware() { # freeware <name> <zip> <frames> <what> <options...>
	local name="$1" file="$2" frames="$3" what="$4"; shift 4
	local zip
	if zip="$(game_zip "$file")"; then
		mkwork "$name" "$zip"
		both "$name" "$name" "$frames" "$@"
		same "$name" "$work/$name.n.txt" "$work/$name.w.txt" "native = sandbox, $frames frames: $what"
	elif [ -n "$games" ]; then
		skip "$name: no $file in $games"
	fi
}
freeware queen FOTAQ_Floppy.zip 2400 "the intro, then a click" $(click 2000 160 100)
if zip="$(game_zip FOTAQ_Floppy.zip)"; then
	"$native" "$work/queen" --frames 2400 $(click 2000 160 100) --clock-start 1893456000 > "$work/queen2030.n.txt" 2> /dev/null
	"$wbx" "$core" "$work/queen" --frames 2400 $(click 2000 160 100) --clock-start 1893456000 > "$work/queen2030.w.txt" 2> /dev/null
	differ time:date "$work/queen2030.n.txt" "$work/queen.n.txt" "the clock starting in 2030 gives another Flight of the Amazon Queen (its random numbers come from the date)"
	same time:date "$work/queen2030.n.txt" "$work/queen2030.w.txt" "the clock starting in 2030: native = sandbox (the sandbox told by the host, the native build by its own clock)"
fi
freeware lure lure-1.1.zip 2400 "the intro with its music (from frame 1832), then Escape" --press 2200:KeyEscape:2
freeware drascula drascula-1.0.zip 1200 "ScummVM's CD audio message, OK clicked" $(click 600 66 159)
freeware dreamweb dreamweb-uk-1.1.zip 2400 "the intro, then Escape" --press 1800:KeyEscape:2

if zip="$(game_zip lure-1.1.zip)" && [ -n "$roms" ] && [ -f "$roms/MT32_CONTROL.ROM" ] && [ -f "$roms/MT32_PCM.ROM" ]; then
	mkwork mt32 "$zip" '{"musicDriver":"mt32"}'
	cp "$roms/MT32_CONTROL.ROM" "$roms/MT32_PCM.ROM" "$work/mt32/"
	"$native" "$work/mt32" --frames 3600 --audio "$work/mt32.raw" > "$work/mt32.n.txt" 2> "$work/mt32.n.err"
	"$wbx" "$core" "$work/mt32" --frames 3600 > "$work/mt32.w.txt" 2> "$work/mt32.w.err"
	peak="$(python3 -c 'import array,sys; a=array.array("h",open(sys.argv[1],"rb").read()); print(max(abs(x) for x in a))' "$work/mt32.raw")"
	same mt32 "$work/mt32.n.txt" "$work/mt32.w.txt" "Lure of the Temptress on the MT-32 emulation, 3600 frames: native = sandbox (loudest sample $peak)"
	if [ "$peak" -gt 0 ]; then pass "mt32: the music is heard"; else fail "mt32: silence"; fi
else
	skip "mt32: needs lure-1.1.zip (-g) and the MT-32's ROMs (-r <folder> with MT32_CONTROL.ROM, MT32_PCM.ROM)"
fi

# ---------------------------------------------------------------- texts
echo "== texts"
if [ -n "${CHIMERA_ROOT:-}" ] && [ -f "$CHIMERA_ROOT/tools/check-core-texts.py" ]; then
	if python3 "$CHIMERA_ROOT/tools/check-core-texts.py" "$root" > "$work/texts.txt" 2>&1; then
		pass "texts:keys: every key the code names is in the English table, and every key is named ($(tail -1 "$work/texts.txt"))"
	else
		fail "texts:keys: $(head -5 "$work/texts.txt")"
	fi
else
	skip "texts:keys: needs CHIMERA_ROOT (Chimera's tools/check-core-texts.py)"
fi
if python3 "$root/tests/check-names.py" > "$work/names.txt" 2>&1; then
	pass "texts:names: $(tail -1 "$work/names.txt")"
else
	fail "texts:names: $(head -5 "$work/names.txt")"
fi

echo
echo "$passed passed, $fails failed, $skips skipped"
[ "$fails" -eq 0 ]
