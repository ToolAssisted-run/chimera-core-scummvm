# AGENTS.md - chimera-core-scummvm

ScummVM as a Chimera core: ScummVM's engines with a backend of the core's
own, built for miniBox's sandbox and driven a frame at a time. The user
brings a game as a zip of its folder. Read Chimera's `docs/new-core.md` and
this repository's `docs/PLAN.md` first.

## Layout

- `extern/scummvm` - ScummVM, a submodule at a pinned commit. The build
  applies `patches/` to it in place (`waterbox/apply-patches.sh`, which
  checks the whole series) and never writes anything else into it.
- `extern/libretro-deps` - the libraries' sources (zlib, libpng, libjpeg,
  FreeType, FriBidi, libogg, libvorbis, libFLAC, libmad, FAAD2, libtheora,
  libmpeg2, giflib), a submodule at the commit ScummVM's libretro port pins.
- `patches/` - numbered patches to ScummVM (0001: no launcher, no error box;
  0002: the null backend may have OpenGL when it brings its own).
- Mesa 24.0.9 (`waterbox/setup-mesa.sh`, into `build/mesa`): the OpenGL the
  3D engines draw with, softpipe behind OSMesa, built for the guest and for
  this computer from one tarball.
- `waterbox/` - the core:
  - `backend/` - the OSystem: `chimera-machine.cpp` (the stack, the clock,
    the timers, the sound, the input, starting ScummVM, the firmware),
    `chimera-graphics.cpp` (the screen), `chimera-gl.cpp` (the OpenGL a 3D
    engine draws with), `chimera-fs.cpp` (the game's folder
    out of its zip), `chimera-saves.cpp` (saved games in memory),
    `coroutine.c` (ScummVM's own stack), `controls.h` (written by
    gen-config.py);
  - `wbx-entry.cpp` - the exports; `compat/` - the C library calls both
    builds answer the same way;
  - the makefiles: `native.mk` and `guest.mk` (the entry points), with
    `common.mk`, `deps.mk` (the libraries), `scummvm.mk` (ScummVM's own
    configure and Makefile, in `build/<build>/scummvm`), `scummvm-objs.mk`
    (read by ScummVM's Makefile: the core's C++ and the lists to link),
    `core.mk` (the link), `data.S` and `make-data.py` (ScummVM's data files
    built in);
  - the declarations: `gen-config.py` writes `waterbox.config`,
    `default_keybinds.json`, `backend/controls.h`, `input-names.h` and the
    English texts; `write-languages.py` writes the other 25 language
    tables; `file_slots.json`, `package-licenses.json` (and `licenses/`)
    by hand;
  - the harness: `gate-harness.h`, `run-native.c`, `run-wbx.c`; and
    `run-gate.sh`, `build-package.sh`.
- `tests/` - `make-testbed.py` and `make-playground3d.py` (the test games:
  ScummVM's testbed data, and its 3D test engine's one file),
  `fetch-freeware.sh`, `run-frontend.sh` and its helpers, `check-names.py`,
  `compare-pictures.py` and `check-pixels.py`.
- `build/` - every output. Ignored by git.

## Commands

    mb=<chimera>/extern/chimera-common-minibox
    run() { systemd-run --user --scope --quiet -p MemoryMax=8G -p MemorySwapMax=0 "$@"; }
    run waterbox/setup-mesa.sh -m $mb -j 4 && run waterbox/setup-mesa.sh -n -j 4   # Mesa, once
    run make -C waterbox -f native.mk -j4 MB=$mb       # the native reference (10 minutes the first time)
    run make -C waterbox -f guest.mk -j4 MB=$mb        # core.wbx (the same; check-wbx takes 4 minutes more)
    tests/fetch-freeware.sh <folder>                   # the freeware games the tests play
    CHIMERA_ROOT=<chimera> waterbox/run-gate.sh -m $mb -g <folder> [-r <MT-32 ROMs>]
    waterbox/build-package.sh -m $mb [-o <folder>]     # the package
    QT_LIB_DIR=<Qt lib> tests/run-frontend.sh --chimera-root <chimera> [--package <file>]
    CHIMERA_CORES_DIR=<folder> <chimera>/build/meson-linux/source/ui/test-packages <folder>

## Rules

- Everything the game can observe comes from the core: the clock, the
  date, the files, the language. A new call ScummVM makes to the outside
  is answered by the backend the same way in both builds.
- What differs between C libraries is answered by the core in both builds
  (`compat/libc-same.c`, wrapped at the link); the native reference links
  the sandbox's own math functions (`make-native-libm.sh`).
- A change to the controls is a change to `gen-config.py`; run it, and
  `write-languages.py` after any text changes (every key in 26 languages).
- A 3D engine draws on the core's own OpenGL (Mesa, the same in both
  builds), never on a graphics card: the picture and everything OpenGL holds
  are machine memory, in every savestate.
- The guest build is configured as a cross build; if `config.mk` in
  `build/guest/scummvm` names plain `g++`, the guest objects were built
  against the computer's C library (see docs/PLAN.md).
- Every new test is broken on purpose once and seen to fail.
- Write in plain language; prose is ASCII. Commits: `type(scummvm): what
  changed`, body with how it was tested, Chimera's trailers.
- Stop a run by its PID, never with `pkill -f`.
