# Building and testing the ScummVM core

## Requirements

- Linux on x86-64, with GCC (the guest kit's compiler is the system's, over
  miniBox's sysroot), make, Python 3, git, binutils, curl (to fetch Mesa
  once, and the freeware games the tests play).
- For Mesa's build: meson and ninja, bison, flex, and Python's mako and
  packaging (`sudo apt-get install meson ninja-build bison flex
  python3-mako python3-packaging`). Without a meson that has mako,
  `setup-mesa.sh` makes a private Python environment with them
  (`$HOME/.cache/chimera-mesa-build-venv`, or `MESA_BUILD_VENV`), which needs
  python3-venv.
- A Chimera checkout beside this one, with miniBox built: its host library
  and C guest toolchain (`meson-linux`), and its C++ toolchain
  (`meson-cpp`), whose build of musl's math the native reference links:

      mb=<chimera>/extern/chimera-common-minibox
      meson setup "$mb/build/meson-linux" "$mb" && meson compile -C "$mb/build/meson-linux"
      meson setup "$mb/build/meson-cpp" "$mb" -Dguest_cpp=true && meson compile -C "$mb/build/meson-cpp"

- For the frontend test: Chimera built (its `chimera-run` and its window,
  built by `<chimera>/tools/ci-chimera.sh`). The window runs headless and
  needs no display.
- About 12 GB of disk for the two builds and Mesa's two, and time: each
  build of ScummVM is about ten minutes on four cores, each of Mesa about
  five.

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
- Mesa 24.0.9 is not a submodule: `waterbox/setup-mesa.sh` downloads
  `https://archive.mesa3d.org/mesa-24.0.9.tar.xz` into `build/deps/`
  (`MESA_TARBALL=<path>` uses one already on the computer; either way it is
  checked against its SHA256) and unpacks it into `build/mesa/`.

Both submodules are shallow (`shallow = true` in `.gitmodules`).

## The steps

From `waterbox/`, with `MB=<chimera>/extern/chimera-common-minibox`, and
the memory limit the other cores' builds use:

    run() { systemd-run --user --scope --quiet -p MemoryMax=8G -p MemorySwapMax=0 "$@"; }

0. `run ./setup-mesa.sh -m $MB -j 4` and `run ./setup-mesa.sh -n -j 4`:
   Mesa's softpipe behind OSMesa, static, without LLVM, once for the guest
   (`build/mesa/build-guest2`) and once for this computer
   (`build/mesa/build-native`), from the same tarball with the same options
   (the RSDK v4 core's script). Its last step, a shared library, fails for
   the guest; that is expected, and the script checks for what the core
   links instead (the static archives and the OSMesa target's object). A
   second run says `mesa: already built` and stops.
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
ScummVM's make and lists what to link), ScummVM's data files and the 3D
engines' shaders as one zip (`make-data.py`, `build/engine-data.zip`), and
the link with Mesa (`core.mk`, `native.mk`, `guest.mk`).

## The tests

- `waterbox/run-gate.sh -m <mb> [-g <freeware games>] [-r <ROMs>] [-o <owner's games>]`:
  builds both, then the core's tests: native against sandbox, savestates,
  the clock, the input, the sound, save data, the refusals, the texts.
  Each test says what it compared. Without `-g` (or `SCUMMVM_GAMES`) the
  freeware games' tests say SKIP; `tests/fetch-freeware.sh <folder>`
  downloads them. `-r` (or `SCUMMVM_ROMS`) is a folder with the MT-32's
  `MT32_CONTROL.ROM` and `MT32_PCM.ROM`; `-o` (or `SCUMMVM_OWNER_GAMES`)
  a folder with games that may not be distributed (`kq2.zip`). Set
  `CHIMERA_ROOT` for the text keys check (Chimera's
  tools/check-core-texts.py). About 10 minutes once built.
- `tests/run-frontend.sh --chimera-root <chimera> [--package <file>]`: the
  package through `chimera-run` and through Chimera's window (headless).
  The window needs Qt's libraries: `QT_LIB_DIR=<Qt>/gcc_64/lib`.
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

## Changing the texts

Every text a person reads is a key (docs/PLAN.md). The keys of the
declarations are written by `gen-config.py` into `languages/en.json`; the
keys the code names are added to it by hand. `write-languages.py` then
writes the other 25 tables from its own translations, and stops on a key
it has no translation for. `tests/check-names.py` checks that names are
written as they are.

## Troubleshooting

- The guest link fails on `__memcpy_chk` or `__isoc23_strtol`: the guest
  build of ScummVM was compiled against the computer's C library. Its
  configure must be a cross build (`--host=x86_64-linux-musl`, which
  guest.mk passes); `build/guest/scummvm/config.mk` must name the guest
  compiler (`CXX := g++ -specs ...`). Delete `build/guest/scummvm` and build
  again.
- `no Mesa at .../build/mesa/...`: step 0 was not run for that build.
- check-wbx says the core "declares PT_TLS": that is expected. Mesa keeps
  its current context in thread-local variables, and miniBox gives a guest
  with thread-local storage its own `%fs` (the other cores that link Mesa
  are the same).
- check-wbx takes about four minutes: it reads the whole core.
- A change to the core's C++ is compiled by ScummVM's own make; the link
  follows `build/<build>/scummvm/link.stamp`.
