# Building and testing the ScummVM core

## Requirements

- Linux on x86-64, with GCC (the guest kit's compiler is the system's, over
  miniBox's sysroot), make, Python 3, git, binutils, curl (only to fetch the
  freeware games the tests play).
- A Chimera checkout beside this one, with miniBox built: its host library
  and C guest toolchain (`meson-linux`), and its C++ toolchain
  (`meson-cpp`), whose build of musl's math the native reference links:

      mb=<chimera>/extern/chimera-common-minibox
      meson setup "$mb/build/meson-linux" "$mb" && meson compile -C "$mb/build/meson-linux"
      meson setup "$mb/build/meson-cpp" "$mb" -Dguest_cpp=true && meson compile -C "$mb/build/meson-cpp"

- For the frontend test: Chimera built (its `chimera-run` and its window,
  built by `<chimera>/tools/ci-chimera.sh`). The window runs headless and
  needs no display.
- About 10 GB of disk for the two builds, and time: each build of ScummVM
  is about ten minutes on four cores.

## Sources

    git clone --recursive https://github.com/ToolAssisted-run/chimera-core-scummvm

(The repository is local only for now; the URL is where it would be.)

- `extern/scummvm` is ScummVM at commit a60993f5 (2026-11-03). The build
  applies `patches/` to it in place; `waterbox/apply-patches.sh` leaves a
  tree that already has the whole series alone and refuses one that has
  part of it.
- `extern/libretro-deps` is libretro's collection of library sources at
  bab7d258, the commit ScummVM's own libretro port builds its libraries
  from.

Both submodules are shallow (`shallow = true` in `.gitmodules`).

## The steps

From `waterbox/`, with `MB=<chimera>/extern/chimera-common-minibox`, and
the memory limit the other cores' builds use:

    run() { systemd-run --user --scope --quiet -p MemoryMax=8G -p MemorySwapMax=0 "$@"; }

1. `run make -f native.mk -j4 MB=$MB`: the native reference,
   `build/native/run-native` (the libraries, ScummVM and the core compiled
   for this computer, with the sandbox's own math functions, made by
   `make-native-libm.sh`), and `build/native/run-wbx`, which runs a
   `core.wbx` in the sandbox.
2. `run make -f guest.mk -j4 MB=$MB`: `build/guest/core.wbx`, checked by
   miniBox's `check-wbx.sh`.
3. `./build-package.sh -m $MB` (or `-o <folder>`): `scummvm.chimeraCore`.
   It does step 2 itself.

Each build does, in order: the libraries (`deps.mk`, into
`build/<build>/deps`), ScummVM's configure and Makefile (`scummvm.mk`,
in `build/<build>/scummvm`; `scummvm-objs.mk` adds the core's own C++ to
ScummVM's make and lists what to link), ScummVM's data files as one zip
(`make-data.py`, `build/engine-data.zip`), and the link (`core.mk`,
`native.mk`, `guest.mk`).

## The tests

- `waterbox/run-gate.sh -m <mb> [-g <folder of freeware games>]`: builds
  both, then the core's tests: native against sandbox, savestates, the
  clock, the input, the sound, the refusals. Each test says what it
  compared. The games' tests SKIP, saying so, without `-g` (or
  `SCUMMVM_GAMES`); `tests/fetch-freeware.sh <folder>` downloads them.
- `tests/run-frontend.sh --chimera-root <chimera>`: the package through
  `chimera-run` and through Chimera's window.
- Chimera's contract tests, against the package's folder:

      CHIMERA_CORES_DIR=<folder> <chimera>/build/meson-linux/source/ui/test-packages

The runners take a work folder holding what Chimera would mount: the
game's zip, a `slots` file (`{"game":["game.zip"]}`, with
`"savedata":["saves.zip"]` for saved games) and a `settings` file. A movie
for them is `--press AT:TOKENS:LEN` or a file of lines naming what is held
each frame (`--movie`); the tokens are waterbox.config's control names
without spaces (`MouseLeftButton`, `KeyEscape`) and the mouse's place
`X=<0..65535>`, `Y=<0..65535>`.

ScummVM's log goes to the error output: run-native shows it, and in
Chimera it is the core's log.
