# The ScummVM core: the design log

What was decided, why, and what was measured. Newest last.

## The request (chimera#465) and what upstream is (2026-11-02)

ScummVM (https://github.com/scummvm/scummvm) runs the games of its game
engines: 159 in its configure, 139 built by default. The core pins commit a60993f5 (2026-11-03, "ULTIMA8: Remove
collideMove when leaving fast area").

- **Licence.** GPL-3.0-or-later, with some parts under other free licences
  (extern/scummvm/LICENSES: BSD, LGPL, MIT, Lua, TinyGL, MPL, Boost, ISC,
  OFL and Apache for theme files, and the Catharon licence of the AGS
  engine's FreeType autohinter, which is BSD-like). It may be distributed
  as source and binary.
- **One thread, one clock.** No engine starts a thread of its own (the
  Newton physics library in the hpl1 engine has its threads commented out).
  The backends do: the SDL backend's timer thread and audio callback. The
  core is its own backend, so there are none.
- **The picture** is ScummVM's own: engines draw into surfaces the backend
  keeps, and the 3D engines (Grim, Myst 3, The Longest Journey, Freescape,
  Hpl1, Tetraedge, Wintermute 3D, Alcachofa) have TinyGL, a software
  renderer, as one of their renderers. Two engines draw only through
  OpenGL: colony (The Colony) and twp (Thimbleweed Park).
- **What the user brings:** the game's files. No firmware is needed except
  the Roland MT-32 or CM-32L ROMs, for the games that can play their music
  on those (optional, by a setting).
- **What a test can run on a public runner:** ScummVM's own test engine
  (testbed, off by default in ScummVM's builds; on in this core's), whose
  data is in ScummVM's tree; and the freeware games scummvm.org
  distributes (Beneath a Steel Sky, Flight of the Amazon Queen, Lure of the
  Temptress, Drascula, DreamWeb and others), each free to redistribute by
  its own licence (the readme or LICENSE inside each zip). The tests
  download them; nothing of them is in the repository.

## The owner's decisions (user-decided, 2026-11-03)

1. **The mouse is two absolute axes, 0 to 65535** (neutral 32768) across
   and down the screen shown, as in the DOSBox-X and PCem cores.
2. **colony and twp go through the GPU bridge**, after the software
   engines: the core is built first without them, then they are added the
   way the other OpenGL cores do it. What is checked on a real card is
   checked on the GTX 1060 Windows computer, and what was seen only through
   a software GL is said to be so.
3. **A game is given as a zip of its folder.**
4. **The repository stays local** (no remote, nothing public) until the
   owner says.

## The shape

- **ScummVM is built by its own configure and Makefile**, in a folder of
  the build (build/native/scummvm, build/guest/scummvm; the submodule is
  never written to). It is configured for its "null" backend (no window, no
  sound device, no host devices) with every optional library pointed at
  the core's own builds of them and every host feature off (ALSA, MIDI
  ports, network, text to speech, the processor's SSE2 and AVX2 code,
  OpenGL). Everything ScummVM would link into its program but the null
  backend is linked into the core. The guest build is configured as a cross
  build (`--host=x86_64-linux-musl`): configured as a native one, configure
  runs its test programs, which only run in the sandbox, decides the guest
  compiler does not work and silently takes the computer's own (the first
  guest build was made that way and failed to link on glibc's
  `__memcpy_chk`; found 2026-11-03).
- **The libraries** (zlib, libpng, libjpeg-turbo's C, FreeType, FriBidi,
  libogg, libvorbis, libFLAC, libmad, FAAD2, libtheora, libmpeg2, giflib)
  are built from the submodule extern/libretro-deps at the commit ScummVM's
  own libretro port pins (bab7d258), with the file lists that port uses
  (waterbox/deps.mk). Plain C, no assembly, nothing that asks the processor
  what it can do. These are old versions (zlib 1.2.8, libpng 1.6.25):
  moving to current releases of each is a possible later step, for the
  owner.
- **The backend** (waterbox/backend/) is an OSystem, as every ScummVM port
  has: chimera-machine.cpp says how it works (the stack, the clock, the
  timers, the sound, the input, the files). The picture: chimera-graphics.h.
  The game's files: chimera-fs.h. Saved games: chimera-saves.h.
- **Starting the game** is ScummVM's own `--path=/game --auto-detect`: its
  detector finds the game in the zip, the first one when there are several
  (Lure of the Temptress's zip holds the VGA and the EGA version; the VGA
  one is found first). Init runs ScummVM until its engine has started and
  then to the end of that frame (at most 2 minutes of the machine's time);
  if ScummVM finds no game or the engine cannot start, Init refuses with
  ScummVM's reason. ScummVM's launcher and error box never appear: patch
  0001 has ScummVM call the core instead (under CHIMERA_BACKEND, which only
  the core's build of base/main.cpp defines).
- **ScummVM's data files** (each engine's .dat, the fonts, the GUI's
  themes and translations; 46 files, 81 MB, the list ScummVM's own Makefile
  installs for the engines built) are one zip built into core.wbx
  (data.S), read from the core's memory.
- **The same in both builds.** The native reference links the sandbox's
  own math functions (make-native-libm.sh, from the OpenBOR core), and both
  builds answer qsort, bsearch, rand and srand with the core's own
  (compat/libc-same.c): glibc's and musl's sort equal elements
  differently, and Flight of the Amazon Queen and Beneath a Steel Sky sort
  with qsort. ScummVM's build date (__DATE__, __TIME__, shown in its menus)
  is the pinned commit's in both builds (SOURCE_DATE_EPOCH).

## How the machine works (2026-11-03)

- **Frames and the clock.** ScummVM runs on a stack of its own (16 MiB,
  coroutine.c, from the OpenBOR core). The clock counts microseconds from
  power-on; frame k ends at (k + 1) / 60 s. It moves only where ScummVM
  waits (`delayMillis`) and at a frame's end, where ScummVM hands control
  back. `getMillis` reads it. Timers (`installTimerProc`) run each time it
  moves, as often as their period has passed (ScummVM's own
  DefaultTimerManager, called by the core and not by a thread).
- **A loop that never waits.** An engine that waits for time by reading
  the clock in a loop would never see it move. A loop that reads the clock,
  asks for events or shows a picture 4096 times without the clock moving
  ends the frame there, as a wait to the frame's end would (the OpenBOR
  core's rule; calls counted, so the same in every run).
- **The date** is the project's clock start (the sandbox's clock_gettime,
  read once at Init, the native harness's `--clock-start`) plus the
  machine's time, computed in UTC without the C library. ScummVM seeds its
  random numbers from the date and `getMillis`, so another clock start is
  another run (Flight of the Amazon Queen shows it, test time:date); the
  same clock start is the same run.
- **The picture.** An engine draws into the game screen (8, 16 or 32 bits a
  pixel), ScummVM's menus into the overlay. A real screen shows what was
  there at the last `updateScreen`, so that call takes a copy (the game
  screen and its palette, or the overlay, and the pointer's place); at the
  frame's end the copy becomes the BGRA picture Chimera reads, the pointer
  drawn over it. Turbo skips that last step only. The overlay is the game
  screen's size (at least 320x200), so a click names the same place on
  either. Clearing the overlay copies the game screen into it, as ScummVM's
  SDL backend does: ScummVM's dialogs draw over the game, and testbed's
  pixel format test showed an empty screen behind its dialog until it did.
  A game of 200 or 400 lines is shown 4:3 (GetDisplayAspectX/Y), as
  ScummVM's aspect correction does; any other in square pixels.
- **The sound** is ScummVM's mixer, run at the end of each frame for 735
  stereo samples at 44100 Hz. AdLib music is ScummVM's OPL emulation
  (MAME's, DOSBox's or Nuked's: a setting), driven by the samples asked
  for. The Roland MT-32 and CM-32L are ScummVM's mt32emu, with the ROMs the
  project gives as firmware; only the set the Music device setting names is
  shown to ScummVM (it would take the CM-32L's over the MT-32's when it has
  both), and Init refuses a Roland device without its ROMs.
- **Input** is the controls' levels each frame, turned into the events a
  keyboard and mouse would send, at the start of the frame: the pointer
  first (when the axes changed), then each control that changed, in
  waterbox.config's order. The mouse's place is two axes, 0 to 65535
  across and down the screen shown (pixel = axis x width / 65536); the
  wheel's two controls each turn it one step when pressed. A key's
  character is what ScummVM's SDL 2 backend would give with a US keyboard
  (`SdlEventSource::mapKey`: F1 to F9 are ASCII_F1 to F9, F10 to F12 their
  key codes, the keypad's digits only with Num Lock on, which it starts
  as). When the engine moves the pointer itself, a move event follows, as
  SDL sends one. The keymapper gets the keyboard and mouse ScummVM's SDL
  backend declares, so engines' keymaps and ScummVM's own (Ctrl+F5 opens
  its menu) work. A frame in which ScummVM did not ask for events is a lag
  frame.
- **Files.** The game's zip is opened once at Init; its central directory
  (ZIP64 too) is made into a tree under /game, names found whatever their
  case, a zip holding one folder taken as that folder (`__MACOSX` left
  out). A stored member is read where it lies, a deflated one inflated as
  it is read (ScummVM's own inflating stream), through a small buffer, so
  a large game costs the machine's memory only what the engines hold. A
  member that is encrypted or compressed another way (bzip2, LZMA,
  Deflate64) is refused at Init, by name. The folder cannot be written:
  testbed's file writing tests fail, by design; ScummVM's games write
  their saves through the save manager, never into their folder.
- **Saved games** live in the machine's memory (chimera-saves.cpp), as
  ScummVM itself writes them (compressed as its default save manager
  compresses them, with its error messages: testbed's VerifyErrorMessages
  test failed until they were the same). Export Save Data writes each file
  under its own name; a project's Save data slot is one zip of such files,
  put in the machine before ScummVM starts.
- **RAM tools.** ScummVM's engines keep a game's state in objects on the
  heap, so the core offers the heap as a bus, "Heap" (as the Ruffle core
  does): the program break's heap, where musl's allocator keeps everything
  under about 224 KiB. GetBusRanges says how much of it is heap now. It is
  ScummVM's memory, not a game's; its addresses hold for one build of the
  core.

## The tests (2026-11-03)

`waterbox/run-gate.sh` (docs/BUILDING.md), every test saying what it
compared. The content: ScummVM's test engine (testbed), whose data
tests/make-testbed.py makes from ScummVM's tree, so it runs anywhere; and
scummvm.org's freeware games (tests/fetch-freeware.sh), whose tests say
SKIP without them. Driving testbed is clicking through its own windows:
its pixel format test (each format shows a pattern and asks whether it is
the 8-bit example's), and, through its Customize window, its save file
and file reading tests.

What it found while it was being written, each fixed:

- the overlay showed nothing behind ScummVM's dialogs (clearing it must
  copy the game screen in);
- testbed's VerifyErrorMessages test failed: a save file that is not there
  must set ScummVM's error, as its default save manager does.

Measured on an Intel Core Ultra 7 265KF, 2026-11-03: 39 passed, 0 failed,
0 skipped (with the freeware games, the owner's MT-32 ROMs and the owner's
King's Quest II; without them their tests say SKIP).

- native = sandbox: testbed's pixel formats (1600 frames, every picture,
  the sound, the lag, testbed's reports), its save file and file tests;
  Beneath a Steel Sky (floppy) 1500 frames, Flight of the Amazon Queen
  (floppy) 2400, Lure of the Temptress 2400, Drascula 1200, DreamWeb 2400,
  Lure on the MT-32 emulation 3600 (its music heard from frame 2945; on
  AdLib from 1832).
- the three 32-bit formats testbed draws (ABGR8888, ARGB8888, RGBA8888)
  give the 8-bit example's pattern pixel for pixel.
- savestates: saved and loaded before every frame, and taken to a new host
  (testbed at frame 900, Beneath a Steel Sky at 700), the runs are the
  straight ones. A state is 4.6 to 4.8 MB in these runs (testbed, Beneath a
  Steel Sky); 4.2 to 13.8 MB in the five games' first 1500 frames.
- turbo: the first half undrawn, the rest the same.
- input: a click lands where the axes put it (testbed's Proceed starts its
  tests; Foster walks to (100,88), another place another walk; a click a
  frame later is another run); Escape skips Beneath a Steel Sky's intro;
  the wheel scrolls testbed's list.
- time: the computer waiting 0.3 s every 250 frames changes nothing; the
  native build on the computer's clock (`--host-clock`, the control) gives
  another machine with the same waits. The clock starting in 2030 gives
  another Flight of the Amazon Queen, native and sandbox the same.
- save data: the six files testbed's save tests leave are exported the
  same in both builds, and a zip of them given back is in the machine at
  its first frame, byte for byte.
- refusals: no game, not a zip, a zip of no game ScummVM knows, a member
  compressed with bzip2, a save zip that is not a zip, the MT-32 without
  its ROMs: each refused at Init with its key.
- texts: Chimera's tools/check-core-texts.py (44 keys, each named) and
  tests/check-names.py (26 tables, no name bent).
- Chimera's contract tests (test-packages, Chimera 08ee9de1): every one
  passes. The frontend tests (tests/run-frontend.sh, Chimera 08ee9de1, its
  window headless): the testbed movie (four clicks) through chimera-run,
  as a movie made into a project and as a hand-written project, and
  through Chimera's window: the last picture is the native reference's,
  pixel for pixel; the window's bindings are the package's. 5 of 5.

Each test was broken on purpose and seen to fail (2026-11-03):

- formats: red and blue swapped in the 32-bit conversion: all three
  formats' patterns differ (12 of 18 lines).
- files: the zip's subfolders listed empty: ReadingFile fails. (A
  case-sensitive index of the zip did not make it fail: ScummVM matches a
  name's case itself, listing the folder, so the test does not cover the
  core's own case folding.)
- savedata:import: the files not put in the machine: none exported, the
  comparison fails.
- turbo: the picture made with rendering off too: the first half's
  pictures are there, the test fails.
- time:date: the clock start ignored: the 2030 run is the default one, and
  native differs from the sandbox.
- session: the frame count kept where no state carries it: the run taken
  to a new host ends 700 frames behind (clock 801, not 1501). Save and
  load in one process (rerecord) cannot see that by its nature (memory no
  state carries is left as it was), and no break of this core was found
  that it would show: it stands for the sandbox's own save and load.
- testbed:saves failed before the save manager set ScummVM's errors (above).
- time:virtual's control is its own break (the computer's clock).
- refusals: each compares its own key.

A measurement, not a test: with the native build's qsort, bsearch, rand
and srand the C library's own (compat/libc-same.c unlinked), the four
games tried (Flight of the Amazon Queen, Beneath a Steel Sky, DreamWeb,
Lure) still agree with the sandbox over their runs above. The stand-ins
stay, as C leaves the order of equal elements to each library.

Speed (Intel Core Ultra 7 265KF): Beneath a Steel Sky, 6000 frames in
3.5 s natively (about 1700 frames a second) and in 5.3 s in the sandbox
(about 1100 a second, start included); the sandboxed run takes 455 MB of
the computer's memory (core.wbx is 223 MB, 81 MB of it ScummVM's data
files). The package is 120 MB.

## A game of the owner's, and the heap (2026-11-03)

- **King's Quest II** (AGI), the owner's copy: its two floppy images'
  files, extracted and zipped (kept outside the repository, in the owner's
  test content folder): ScummVM finds "King's Quest II: Romancing the
  Throne (2.2 1987-05-07 5.25"/3.5"/DOS/English)" first of two. Through
  the intro, Enter five times, a word typed: the parser answers ("I don't
  understand"), the first letter typed went to closing a message. Native
  = sandbox over 3600 frames. Gate test kq2 (SKIP without the folder: the
  game may not be distributed).
- **The Heap bus**, read through chimera-run (`--ram-search-bench Heap`,
  testbed after 600 frames): one live range, 2.5 MB of the 512 MB arena;
  a search of it in runs (ReadBus) takes 0.4 s to start, against 35 s a
  byte a call; every candidate reads what PeekBus reads.

## Not tried yet (2026-11-03)

- **Windows.** Neither the frontend test's scenario on the Windows build
  nor anything else on Windows has been run.
- **A full game played through**, a CD game with speech (Beneath a Steel
  Sky's CD version, Flight of the Amazon Queen's talkie), a game with
  video (Broken Sword, Toonstruck), the high-resolution and 16/32-bit
  engines on real games, the 3D engines' TinyGL renderer, any game of
  over 2 GB in a zip, and a zip whose names are not UTF-8 or ASCII.
- **CI.** `.github/workflows/chimera.yml` is written (copied from the
  OpenBOR core's, with the freeware games fetched) and has never run: the
  repository has no remote.
- **ScummVM's per-game options** (an engine's own checkboxes, such as
  testbed's "interactive mode" or SCUMM's enhancements) are not offered:
  they could become each game's own settings (GetGameSettings and
  SuggestSettings), a later step.

## Open, for the owner (2026-11-03)

- **The Colony and Thimbleweed Park** (the engines left out; "twp" is
  Thimbleweed Park, not Return to Monkey Island, which ScummVM does not
  run). Neither has a software renderer: Colony has two OpenGL renderers,
  Thimbleweed Park calls OpenGL itself. The owner chose the GPU bridge;
  asked again because Chimera's rule is that a GPU renderer is never the
  only one, and because Thimbleweed Park keeps its textures only on the
  card (a state loaded in another session would come back without them).
  The alternative put: Mesa's softpipe built into the core first
  (deterministic, savestates whole), the bridge after it as a hardware
  renderer.
- **The libraries' versions.** libretro-deps holds old releases (zlib
  1.2.8, libpng 1.6.25 beta, FreeType 2.x of its time); moving each to its
  current release is possible and not done.
- **Publishing**: a repository, CI, the roster, the README's row.
