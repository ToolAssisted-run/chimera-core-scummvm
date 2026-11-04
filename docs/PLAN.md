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
2. **The Colony and Thimbleweed Park** (the two engines that draw only
   through OpenGL). First answer: through the GPU bridge, after the
   software engines, the way the other OpenGL cores do it. Asked again,
   because Chimera's rule is that a GPU renderer is never a core's only
   one, and because Thimbleweed Park keeps its textures only on the card (a
   state loaded in another session would come back without them), the
   owner chose instead:
   **Mesa's softpipe built into the core**, as the PCSX2, Flycast and Ruffle
   cores do, so these games draw the same picture everywhere, savestates
   hold all of it and no graphics card is needed; then, as a second step,
   **the GPU bridge as an optional "Hardware (OpenGL)" renderer** for speed,
   as PCSX2 has it. What is checked on a real card is checked on the GTX
   1060 Windows computer, and what was seen only through a software OpenGL
   is said to be so. Then, told what the bridge would take for ScummVM
   (below), the owner chose to **stop at the softpipe for now** and keep
   the bridge as a later project.
3. **A game is given as a zip of its folder.**
4. **The repository stays local** (no remote, nothing public) until the
   owner says. Asked again with the core done, the same: not yet. Then, on
   2026-11-04: publish it (the repository, the roster, the website and
   issue 465 are done outside this repository's own work).
5. **The libraries move to their current upstream releases** (zlib, libpng
   and every other one taken from libretro-deps' old copies), each pinned
   by version and the SHA-256 of its tarball (or by commit), with their
   licences in waterbox/licenses, and every test run again.
6. **The published package leaves out ScummVM's two test engines**
   (testbed and Playground 3D); the tests build and use a package with
   them. The build says plainly which is which, and the release package
   passes Chimera's package tests and a chimera-run check too.

## The shape

- **ScummVM is built by its own configure and Makefile**, in a folder of
  the build (build/native/scummvm, build/guest/scummvm; the submodule is
  never written to). It is configured for its "null" backend (no window, no
  sound device, no host devices) with every optional library pointed at
  the core's own builds of them and every host feature off (ALSA, MIDI
  ports, network, text to speech, the processor's SSE2 and AVX2 code).
  OpenGL is the core's own (see "OpenGL in the core" below). Everything
  ScummVM would link into its program but the null
  backend is linked into the core. The guest build is configured as a cross
  build (`--host=x86_64-linux-musl`): configured as a native one, configure
  runs its test programs, which only run in the sandbox, decides the guest
  compiler does not work and silently takes the computer's own (the first
  guest build was made that way and failed to link on glibc's
  `__memcpy_chk`; found 2026-11-03).
- **The libraries** (zlib, libpng, libjpeg-turbo, FreeType, FriBidi,
  libogg, libvorbis, libFLAC, libmad, FAAD2, libtheora, libmpeg2, giflib)
  are each its current upstream release, pinned by the SHA-256 of its
  tarball (waterbox/deps.lock), and compiled by waterbox/deps.mk from the
  file lists each one's own build uses: plain C, no assembly, no
  intrinsics, nothing that asks the processor what it can do. They were
  first the old copies in libretro-deps; see "The libraries at their
  current releases" below.
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
  themes and translations, and the 3D engines' OpenGL shaders in a
  shaders/ folder; the list ScummVM's own Makefile installs for the engines
  built) are one zip built into core.wbx (data.S), read from the core's
  memory.
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
King's Quest II; without them their tests say SKIP). With the 3D tests
(see "OpenGL in the core") and the core linking Mesa, the same day: 52
passed, 0 failed, 0 skipped, every test above as before.

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
- texts: Chimera's tools/check-core-texts.py (44 keys, each named; 49
  with the renderer setting's) and
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

## OpenGL in the core: Mesa's softpipe (2026-11-03)

The owner's decision 2 (above). What was built:

- **Mesa 24.0.9**, softpipe behind OSMesa, static, without LLVM: the RSDK v4
  core's `setup-mesa.sh` (itself the touchHLE and PCSX2 cores'), the same
  release, checksum and options, built twice from one tarball, for the
  guest and for this computer. The native reference links the same Mesa,
  because a game can ask OpenGL questions and act on the answers, and reads
  its own pictures back (The Colony keeps the screen behind its message
  windows that way, Thimbleweed Park makes its saved games' thumbnails):
  OpenGL is part of the machine. Mesa's own zlib is left
  out of the link (ScummVM's serves both); its expat is not pulled in.
- **ScummVM's configure** gives OpenGL only to backends it knows (SDL,
  Android and a few more). Patch 0002 lets the null backend have desktop
  OpenGL through GLAD when asked with `--opengl-mode=gl`; without that
  option nothing changes. GLAD loads every function through
  `OSystem::getOpenGLProcAddress`, which the core answers from OSMesa.
- **The engines this adds**: The Colony and Thimbleweed Park (OpenGL only),
  Penumbra (hpl1: its build asks for OpenGL with shaders although it can
  draw with TinyGL, so it had been left out silently), and ScummVM's 3D test
  engine, Playground 3D, for the tests. Their shaders are in the data zip,
  in the shaders/ folder ScummVM reads them from.
- **How a 3D engine draws** (backend/chimera-gl.cpp): `initGraphics3d` asks
  for OpenGL inside its graphics transaction; the core then makes one OSMesa
  context (24-bit depth, 8-bit stencil, the compatibility profile, which
  has both the fixed-function calls and the shaders) on a buffer the size of
  the game's screen, the first row the top. The engine draws; when it shows
  its picture (`updateScreen`), the core finishes the drawing (glFinish:
  OSMesa then copies what was drawn into the buffer) and reads the buffer
  as the game's screen. The overlay (ScummVM's menus) and the pointer are
  drawn over it as for any game. Everything OpenGL holds is in the
  machine's memory, so a savestate holds it all.
- **The renderer setting** is ScummVM's own Game 3D Renderer choice:
  Software (TinyGL, the default, which these engines used before), OpenGL,
  OpenGL with shaders. With Software, OpenGL is not offered to engines that
  can draw either way (`kFeatureOpenGLForGame` is false), so each takes
  TinyGL; The Colony and Thimbleweed Park do not ask and always get the
  core's OpenGL (The Colony its fixed-function renderer, its own default;
  Thimbleweed Park its shaders). With the other two, OpenGL is offered and
  ScummVM is started with `--renderer=opengl` or `opengl_shaders`; an engine
  without the one asked for takes the nearest it has, by ScummVM's own rule.
- **The image has thread-local storage now.** Mesa keeps its current
  context in thread locals, so core.wbx has a TLS segment and its own `%fs`,
  which miniBox gives it, as for every core that links Mesa (check-wbx says
  so and passes). On Windows this is the path that once crashed PCSX2's
  softpipe after about a thousand frames (Windows does not keep a program's
  `%fs` across a thread switch); miniBox has repaired it since 2026-09-12.
  Not tried on Windows for this core.

Tested with Playground 3D (tests/make-playground3d.py makes its one file):
its six scenes (a turning cube, depth offsets, a fade, a moving viewport,
textures in five pixel formats, a quad strip and a triangle), a click
apiece, through each renderer:

- native = sandbox, every picture, 720 frames, each renderer.
- the engine says which renderer it made ("Initializing Software 3D
  Renderer", "OpenGL Renderer", "OpenGL Renderer with shaders"), in both
  builds, and the three draw three different runs of pictures.
- where the scenes are drawn: scene 1 on its grey, scene 3 on red, scene 5's
  first texture top left in red, green and blue bars, scene 6's green quad
  and its red triangle wide at the top: 9 pixels checked in each renderer's
  pictures, all as expected.
- savestates with OpenGL with shaders: saved and loaded before every frame
  = straight (22.9 MB states, OpenGL's own state among them); saved
  at frame 300, loaded into a new host = straight.
- through Chimera (tests/run-frontend.sh, Chimera 08ee9de1): a project of
  Playground 3D with OpenGL with shaders, played by chimera-run: the last
  picture is the native reference's, every pixel (the frontend tests 6 of
  6). Chimera's contract tests (test-packages) pass with this package.

Each check was broken on purpose once and seen to fail:

- the buffer's rows left bottom first (OSMesa's default): scene 5's
  textures and scene 6's triangle are not where they should be (the three
  bars read the grey behind them; the triangle's wide end is at the bottom).
- the drawing not finished before the buffer is read: every picture black.
- OpenGL offered with the Software setting too: the engine makes the
  shader renderer instead of TinyGL, and the check of what it made fails.
- the frontend's 3D check, given a project whose renderer is Software: its
  last picture differs from the reference's on all 480 lines. (OpenGL and
  OpenGL with shaders draw that last scene alike, so this check tells
  TinyGL from OpenGL, not the two OpenGL renderers apart; the core's own
  tests do that.)

Speed (Intel Core Ultra 7 265KF): Playground 3D, 720 frames at 640x480:
2.8 s natively with TinyGL, 5.2 s with OpenGL and 5.4 s with shaders; in
the sandbox 5.0, 7.1 and 7.3 s (start included). These scenes are small;
a real game's speed on softpipe is not known (see Not tried). core.wbx
grew from 223 to 250 MB.

**The Colony and Thimbleweed Park are built in and untried**: no copy of
either is in the owner's folders, so neither has been started, and nothing
here says they draw correctly or how fast.

## A later project: the GPU bridge (user-decided, 2026-11-03)

The owner's second step was the GPU bridge as an optional "Hardware
(OpenGL)" renderer, for speed, as the PCSX2 core has it. Looked into before
building it, it is a project of its own for ScummVM, not wiring, and the
owner chose to stop at the softpipe for now and keep this as a later
project. What it would take:

1. **The core profile.** Chimera's bridge gives a core-profile context
   (owner, 2026-10-19), which has no fixed-function OpenGL. The Colony's
   default renderer, and every engine's plain "OpenGL" renderer (Grim's,
   Myst III's, Playground 3D's, ...), are fixed-function: they cannot draw
   through the bridge at all. Only the "OpenGL with shaders" renderers
   could, and The Colony's own code calls its shader renderer not yet
   complete.
2. **Finding what OpenGL can do.** ScummVM reads its features from
   `glGetString(GL_EXTENSIONS)` (graphics/opengl/context.cpp), which a core
   profile does not answer (it gives the extensions one by one, through
   glGetStringi). It would find no framebuffer objects, and Thimbleweed
   Park refuses to start without them. A patch to ScummVM would read them
   the core profile's way.
3. **A new context at every load.** Since chimera#43 every state load gives
   a bridged core a new OpenGL context, so that objects the state's
   engine made in another context are never reached. A bridged core must
   then make its objects again. PCSX2, Dolphin and the others have a
   renderer that can rebuild from the machine's memory; ScummVM's engines
   make their textures, shaders, buffers and OpenGL settings once and never
   again, so after any rewind the picture would be missing them. The core
   would need a layer between the engines and the bridge that records
   every object (each texture's pixels, each buffer's bytes, each shader's
   source and program's uniforms, framebuffers, vertex arrays, samplers)
   and every setting in the machine's memory, copies back what the card
   drew into textures before each state is taken, and makes all of it again
   on a new context after a load.
4. **What else changes.** Games that read their picture back (The Colony
   for the screen behind its message windows, Thimbleweed Park for its
   saved games' thumbnails) would put a driver's pixels into the machine,
   so a movie could replay differently on another card or driver; a movie
   made this way says a card drew it (Chimera's GpuRenderer header).

Estimate: several days for 2 and 3, the layer being most of it, tested
here only on llvmpipe (Mesa on the processor, through EGL) and for real
on the GTX 1060 Windows computer. Whether it is worth it depends on how
slow softpipe is for a real game, which nobody has measured: Playground
3D's scenes run at about 100 frames a second in the sandbox at 640x480,
and no 3D game has been tried.

## The libraries at their current releases (user-decided, 2026-11-03)

The libraries came first from libretro-deps (bab7d258), the copies
ScummVM's libretro port builds: zlib 1.2.8, a libpng 1.6.25 beta, FreeType
2.7.0, FAAD2 2.7 and others years old. The owner chose their current upstream
releases. The submodule is gone; instead:

- **waterbox/deps.lock** names each release, its tarball's address (more
  than one where the first may go away) and the tarball's SHA-256;
  **fetch-deps.sh** downloads each once (into build/deps, or a folder
  given with CHIMERA_DEPS_DIR), refuses one that is not the pinned tarball,
  and unpacks it into build/src. deps.mk runs it before anything else.
- **No library's own build system runs** (CMake, Meson, configure): what
  each would generate is in waterbox/deps-config/, written once by hand for
  x86-64 Linux, where glibc and musl give the same answers, with a note in
  each file of the template it fills. A new release is checked against its
  templates again.
- **Their licences** are copied from the same tarballs into
  waterbox/licenses, and package-licenses.json names each by version and
  tarball.

The releases (2026-11-03):

| library | before | now |
|---|---|---|
| zlib | 1.2.8 | 1.3.2 (its SHA-256 checked against zlib.net's) |
| libpng | 1.6.25 beta 3 | 1.6.59 |
| libjpeg-turbo | not stated in the copy; API 8 | 3.2.1, API 8 |
| FreeType | 2.7.0 | 2.15.0 |
| FriBidi | 1.0.13 | 1.0.18 |
| libogg | not stated in the copy | 1.3.6 |
| libvorbis | 1.3.5 | 1.3.7 (its newest, from 2020) |
| libFLAC | not stated in the copy | 1.5.0 |
| libmad | 0.15.1b | 0.16.5 (see below) |
| FAAD2 | 2.7 | 2.11.4 |
| libtheora | not stated in the copy | 1.2.0 |
| libmpeg2 | 0.5.1 | 0.5.1 (see below) |
| giflib | 5.2.2 | 6.1.3 |

What each needed:

- **libjpeg-turbo 3** compiles most of its sources three times, for 8, 12
  and 16 bits a sample, through small wrapper files (src/wrapper), as its
  CMake does; the 8-bit API ScummVM uses calls into the others. No SIMD:
  jconfig.h leaves WITH_SIMD undefined, as libjpeg-turbo's own non-SIMD
  build does.
- **FreeType 2.15** is built as its docs/INSTALL.ANY says: the base and one
  amalgamated file a module, every module its default ftmodule.h lists
  (new since the old copy: hvf, sdf, svg), compressed fonts through the
  core's zlib (FT_CONFIG_OPTION_SYSTEM_ZLIB) rather than its own copy.
- **libmad**: Underbit's last release, 0.15.1b, is from 2004 and its site
  is gone. Tenacity's continuation (codeberg.org/tenacityteam/libmad) is
  where it is maintained now, and its 0.16.5 (2026-10-11) is the release
  used. Its CMake chooses 64-bit fixed-point C on x86-64 (FPM_64BIT); the
  old build used FPM_INTEL, integer multiplies in inline assembly. Both
  are integer arithmetic, the same on every computer.
- **libmpeg2 0.5.1** (2008) is still its last release. Its own site no
  longer has the tarball; the one used is VideoLAN's mirror of it, whose
  SHA-256 is the one Debian's archive has for the same file. Its config.h
  now names no architecture, so its MMX code is not compiled at all (the
  old copy's config.h named x86, and the MMX files were built and never
  chosen).
- **giflib 6**: ScummVM's use of it (DGifOpen, DGifSlurp, DGifCloseFile
  with an error pointer) is unchanged from giflib 5.1.
- **FAAD2 2.11** is built as its CMake's floating-point "faad" library:
  every file in libfaad/, dynamic range control on.
- The two stand-ins for libretro's headers (retro_inline.h,
  retro_miscellaneous.h) are gone.

Tested with the new libraries (2026-11-03): run-gate.sh 52 passed, 0
failed, 0 skipped (with the freeware games, the owner's MT-32 ROMs and King's
Quest II), every test as before; the frontend tests 6 of 6 on the test
package; Chimera's contract tests pass on both packages; check-release.sh 4
of 4 on the published one. The states are a little smaller (testbed's 4.1
MB, from 4.7; Beneath a Steel Sky's 4.8 MB; OpenGL with shaders' 22.6 MB,
from 22.9). Which library each test's game reaches was not measured. From
ScummVM's code: saved games are compressed with zlib (testbed's save tests
write some), and the decoders for MP3, AAC, FLAC, Ogg Vorbis, Theora,
MPEG-2, JPEG, PNG and GIF run only for games that have such files. None of
the tests' games was checked for those.

## The published package and the test package (user-decided, 2026-11-03)

ScummVM's two test engines, testbed and Playground 3D, are what the tests
play; a user has no use for them. The published package leaves them out
and the tests build and use a package with them:

- `make -f guest.mk TEST_ENGINES=no` configures ScummVM with
  `--disable-engine=testbed,playground3d`, in its own folder
  (build/guest-release); without it, build/guest has them, as the native
  reference always does.
- `build-package.sh` makes the published package (build/package);
  `build-package.sh -t` the test package (build/package-tests), whose
  version ends in "tests". Each package's build.json says
  `"variant": "release"` or `"tests"`. The sources, settings and texts are
  the same.
- run-gate.sh and run-frontend.sh use the test builds; tests/check-release.sh
  checks the published package through chimera-run: build.json says
  "release" and the version does not end in "tests"; projects of testbed's
  and Playground 3D's games are refused, "Could not find suitable engine
  plugin" (ScummVM's detector knows every engine's games, built or not, so
  it finds them and has no engine to run them); Beneath a Steel Sky, 1500
  frames with Escape and a click in a project, ends on the native
  reference's picture; and Chimera's contract tests pass.
- Measured (2026-11-03): 4 of 4 on the published package (127.4 MB; the
  test package is 127.7 MB). Run on the test package instead, the first two
  checks fail (it says "tests", and it plays both games). With the
  project's click moved to another place, the game check fails (61 of 200
  lines differ).

## Ready to publish (2026-11-04)

Run again against Chimera's main of the day (f0cc3a1e; the rounds above
used 08ee9de1, and nothing between the two touches what a core declares or
how it is run), with its own miniBox build: run-gate.sh 52 passed, 0
failed, 0 skipped (with the freeware games, the owner's MT-32 ROMs and
King's Quest II; 13 minutes, the builds included); the frontend tests 6 of
6 on the test package; check-release.sh 4 of 4 on the published package;
Chimera's contract tests pass on both.

Two things a CI runner does differently, found or checked before the
first run:

- **Chimera's checkout lies inside this repository on a runner**
  (chimera-checkout), and Chimera's text checker reads every source file
  under the folder it is given: given this repository, it read Chimera's
  own sources too and reported 62 problems (keys like "slot.help" in
  Chimera's tests). run-gate.sh now gives it a copy of this repository's
  tracked files, and .gitignore leaves chimera-checkout out.
- **A clean runner fetches everything**: tests/fetch-freeware.sh into an
  empty folder with nothing in its environment but PATH (all five games,
  each checked by SHA-1), fetch-deps.sh into an empty cache (all 13
  tarballs, each checked by SHA-256), Mesa's tarball address answering.

## Not tried yet (2026-11-03)

- **Windows.** Neither the frontend test's scenario on the Windows build
  nor anything else on Windows has been run.
- **A full game played through**, a CD game with speech (Beneath a Steel
  Sky's CD version, Flight of the Amazon Queen's talkie), a game with
  video (Broken Sword, Toonstruck), the high-resolution and 16/32-bit
  engines on real games, any game of over 2 GB in a zip, and a zip whose
  names are not UTF-8 or ASCII.
- **Any real 3D game**, on any renderer: The Colony and Thimbleweed Park
  (no copy of either is in the owner's folders), Grim Fandango, Myst III,
  The Longest Journey, Syberia, Penumbra. Only ScummVM's own 3D test engine
  was drawn, so how fast softpipe draws a real game is not known.
- **CI.** `.github/workflows/chimera.yml` is written (copied from the
  OpenBOR core's, with the freeware games fetched) and has never run: the
  repository has no remote.
- **ScummVM's per-game options** (an engine's own checkboxes, such as
  testbed's "interactive mode" or SCUMM's enhancements) are not offered:
  they could become each game's own settings (GetGameSettings and
  SuggestSettings), a later step.

## Open, for the owner (2026-11-03)

- **Publishing**: decided by the owner on 2026-11-04 (above).
