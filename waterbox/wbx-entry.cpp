/* wbx-entry.cpp - the chimera guest ABI over the machine (backend/).
 *
 * Compiles identically for the guest (miniBox emulibc) and for the native
 * reference (native-shim/emulibc.h): the same machine and the same exports,
 * one in the sandbox and one out of it.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <emulibc.h>

#include "backend/chimera-machine.h"

static char g_load_error[2048];

/* Turbo: the picture is not made; ScummVM draws whatever happens */
ECL_INVISIBLE static int chimera_render_enabled = 1;

ECL_EXPORT const char *GetLoadError(void) { return g_load_error; }

ECL_EXPORT int Init(void) { return scvm_init(g_load_error, (int)sizeof g_load_error); }

ECL_EXPORT void SetButton(int32_t index, int32_t state) { scvm_set_button(index, state); }

/* the mouse's place: 0..65535 across and down the screen shown */
ECL_EXPORT void SetAxis(int32_t index, int32_t value) { scvm_set_axis(index, value); }

ECL_EXPORT void FrameAdvance(uint64_t packed)
{
	(void)packed;
	scvm_frame(chimera_render_enabled);
}

ECL_EXPORT void SetRenderingEnabled(int on) { chimera_render_enabled = on != 0; }

ECL_EXPORT uint32_t *GetVideoBgra(void)
{
	int w, h;
	return (uint32_t *)scvm_video(&w, &h);
}
ECL_EXPORT int GetVideoWidth(void)
{
	int w, h;
	scvm_video(&w, &h);
	return w;
}
ECL_EXPORT int GetVideoHeight(void)
{
	int w, h;
	scvm_video(&w, &h);
	return h;
}
/* 4:3 for a game of 200 or 400 lines (drawn for a 4:3 monitor), else square pixels */
ECL_EXPORT int GetDisplayAspectX(void)
{
	int x, y;
	scvm_aspect(&x, &y);
	return x;
}
ECL_EXPORT int GetDisplayAspectY(void)
{
	int x, y;
	scvm_aspect(&x, &y);
	return y;
}

ECL_EXPORT int16_t *GetAudio(void)
{
	int n;
	return (int16_t *)scvm_audio(&n);
}
ECL_EXPORT int GetAudioSampleCount(void)
{
	int n;
	scvm_audio(&n);
	return n;
}

ECL_EXPORT int GetVsyncNumerator(void) { return SCVM_FPS; }
ECL_EXPORT int GetVsyncDenominator(void) { return 1; }

/* a frame in which ScummVM did not ask for events is a lag frame */
ECL_EXPORT int InputWasRead(void) { return scvm_input_was_read(); }

/* Emulator > Export Save Data: the saved games, under the names ScummVM gave them */
ECL_EXPORT int32_t GetSaveDataFileCount(void) { return scvm_saves_count(); }
ECL_EXPORT const char *GetSaveDataFileName(int32_t i) { return scvm_saves_name(i); }
ECL_EXPORT int64_t GetSaveDataFileSize(int32_t i) { return scvm_saves_size(i); }
ECL_EXPORT const uint8_t *GetSaveDataFileBuffer(int32_t i) { return scvm_saves_data(i); }

ECL_EXPORT int GetMemoryDomainCount(void) { return 0; }
ECL_EXPORT const char *GetMemoryDomainName(int i) { (void)i; return ""; }
ECL_EXPORT uint8_t *GetMemoryDomainPtr(int i) { (void)i; return NULL; }
ECL_EXPORT int64_t GetMemoryDomainSize(int i) { (void)i; return 0; }
ECL_EXPORT int GetMemoryDomainWritable(int i) { (void)i; return 0; }

/* the machine's own clock, in frames */
ECL_EXPORT uint64_t GetCycleCount(void) { return scvm_frames(); }
