# chimera-core-scummvm

A core for [Chimera](https://github.com/ToolAssisted-run/chimera), the
frontend for tool-assisted speedruns, that plays the games
**[ScummVM](https://www.scummvm.org/)** plays: point-and-click adventures
and many other games of their time, through ScummVM's own engines (SCUMM,
SCI, AGI, AGS, Wintermute and over a hundred more). It is ScummVM itself,
built for Chimera's sandbox with a backend of its own, and produces one
file, `scummvm.chimeraCore`, that goes in Chimera's `cores` folder.

## Status

- ScummVM at commit a60993f5 (2026-11-03), with every engine ScummVM builds
  by default. In the tests the sandboxed core gives the same pictures and
  sound as a plain native build, and savestates land where the straight
  run does.
- Tried: scummvm.org's freeware Beneath a Steel Sky, Flight of the Amazon
  Queen, Lure of the Temptress, Drascula and DreamWeb, and King's Quest II,
  each for its first minute or so; no game has been played through.
- 3D games draw with ScummVM's TinyGL or with an OpenGL built into the core
  (Mesa's softpipe), never on a graphics card. The Colony and Thimbleweed
  Park always use the core's OpenGL.
- On Windows, Beneath a Steel Sky's first 1500 frames end on the same
  picture as on Linux; nothing else was run there.
- Not tried: CD games with speech; games with video; any real 3D game
  (only ScummVM's 3D test engine was drawn, and not on Windows). The Colony and
  Thimbleweed Park have never been started: no copy of either was
  available. [docs/PLAN.md](docs/PLAN.md) has what was measured.

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
  emulator (MAME, DOSBox or Nuked OPL), Subtitles, 3D renderer (Software
  (TinyGL), OpenGL, OpenGL with shaders). Each is part of the machine, so a
  movie needs the same value.

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
giflib, Mesa), each its current release, are each under their own licence. The package carries every licence
text. See [LICENSE](LICENSE).
