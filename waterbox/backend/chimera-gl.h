/* chimera-gl.h - the OpenGL the machine has.
 *
 * ScummVM's 3D engines draw through OpenGL, which a sandbox does not have:
 * no graphics card, no driver. The core links its own, Mesa's softpipe
 * behind OSMesa (setup-mesa.sh), the same Mesa in both builds. It is plain
 * C code: the picture is decided by code the core compiled, so it is the
 * same on every computer, and everything it holds (textures, shaders, the
 * picture) is in the machine's memory, so a savestate carries it.
 *
 * OSMesa draws into a buffer in memory instead of a window. The buffer is
 * the screen the engine draws on; when the engine shows its picture
 * (updateScreen), the drawing is finished and the buffer read.
 */
#ifndef CHIMERA_GL_H
#define CHIMERA_GL_H

#include "graphics/surface.h"

/* Whether engines that can draw either way are offered OpenGL (the
 * renderer setting): an engine with no other way draws with it regardless. */
void chimeraGlSetOffered(bool offered);
bool chimeraGlOffered();

/* The screen is w x h: the first call makes the OpenGL (and tells
 * ScummVM's OpenGL code about it), later ones give it a screen of the new
 * size. False if Mesa would not start. */
bool chimeraGlScreen(int w, int h);

/* An OpenGL function by name, for ScummVM's loader (GLAD). */
void *chimeraGlProcAddress(const char *name);

/* Finishes the drawing and copies the screen into dst (w x h, 0xAARRGGBB,
 * the top row first). */
void chimeraGlRead(Graphics::Surface &dst);

#endif
