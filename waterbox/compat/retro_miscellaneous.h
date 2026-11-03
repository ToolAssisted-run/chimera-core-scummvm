/* retro_miscellaneous.h - the two macros of libretro-common's header of this
 * name that libretro-deps' libFLAC uses. */
#ifndef CHIMERA_RETRO_MISCELLANEOUS_H
#define CHIMERA_RETRO_MISCELLANEOUS_H
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#endif
