# chimera-core-scummvm

A core for [Chimera](https://github.com/ToolAssisted-run/chimera), the
frontend for tool-assisted speedruns, that plays the games
**[ScummVM](https://www.scummvm.org/)** plays: point-and-click adventures
and many other games of their time, through ScummVM's own engines (SCUMM,
SCI, AGI, AGS, Wintermute and over a hundred more). It is ScummVM itself,
built for Chimera's sandbox with a backend of its own, and produces one
file, `scummvm.chimeraCore`, that goes in Chimera's `cores` folder.

## Status

Local only: this repository has no remote yet, and nothing is published.

- ScummVM at commit a60993f5 (2026-11-03), with every engine ScummVM builds
  by default (and its test engine) except two that draw only through OpenGL
  (The Colony and Thimbleweed Park): how they will draw is open
  (docs/PLAN.md).
- Tried: ScummVM's test engine (testbed), scummvm.org's freeware games
  Beneath a Steel Sky, Flight of the Amazon Queen, Lure of the Temptress,
  Drascula and DreamWeb, and King's Quest II. The sandboxed core and a
  plain native build agree on every picture and the sound; savestates land
  where the straight run does. Each was played for its first minute or so:
  no game has been played through.
- Not tried: Windows; CD games with speech; games with video; the 3D
  engines. See [docs/PLAN.md](docs/PLAN.md) for what was measured and what was
  not tried.

## What you bring

The game's folder, as a zip: every file of the game, as ScummVM would use
them from a folder (a zip holding one folder is that folder). ScummVM's own
detector finds out which game it is. The freeware games on scummvm.org come
as such zips already.

For the music of games that can play it on a Roland MT-32 or CM-32L, that
synthesizer's ROMs, as firmware (only when the Music device setting asks
for it).

## Buttons and settings

- **The mouse**: its place is two axes, 0 to 65535 across and down the
  screen shown (the game, or ScummVM's menus over it); its three buttons;
  the wheel, a step a press.
- **The keyboard**: a PC keyboard, each key the same key in ScummVM.
  Ctrl+F5 opens ScummVM's menu (save, load, options) in every game.
- **Settings**: Music device (as the game prefers, AdLib, PC speaker, IBM
  PCjr, Creative Music System, Roland MT-32 or CM-32L, or none), AdLib
  emulator (MAME, DOSBox or Nuked OPL), Subtitles. Each is part of the
  machine, so a movie needs the same value.

ScummVM's saved games live in the core's memory and are carried by
savestates. Emulator > Export Save Data writes them out, and a project's
"Save data" slot (a zip of save files) puts them back before the game
starts.

## Building and testing

[docs/BUILDING.md](docs/BUILDING.md) has every step. [docs/PLAN.md](docs/PLAN.md)
is the design log: what was decided, why, and what was measured.
[AGENTS.md](AGENTS.md) is the same for someone working here.

## Licence

This repository's own code is MIT; the patches to ScummVM are GPL-3.0-or-later,
as ScummVM is. The built core is distributed under the GPL-3.0-or-later.
The libraries it is built with (zlib, libpng, libjpeg-turbo, FreeType,
FriBidi, libogg, libvorbis, libtheora, libFLAC, libmad, FAAD2, libmpeg2,
giflib) are each under their own licence. The package carries every licence
text. See [LICENSE](LICENSE).
