# The ScummVM core: the design log

What was decided, why, and what was measured. Newest last.

## The request (chimera#465) and what upstream is (2026-11-02)

ScummVM (https://github.com/scummvm/scummvm) runs the games of 141 game
engines. The core pins commit a60993f5 (2026-11-03, "ULTIMA8: Remove
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
  OpenGL: colony (The Colony) and twp (Return to Monkey Island).
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
