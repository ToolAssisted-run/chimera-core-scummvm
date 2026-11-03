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

/* The heap, as a bus (as the Ruffle core does, chimera#216): what RAM
 * Search, RAM Watch, the Hex Editor and Lua's memory library can look at.
 * ScummVM's engines keep a game's state in objects on the heap, not in a
 * machine's RAM, so this is ScummVM's memory and not a game's:
 * - it is the program break's heap, from the start of its arena to the
 *   break, where musl's allocator keeps everything under about 224 KiB (the
 *   engines' objects and tables); larger buffers are mapped elsewhere;
 * - an address holds a value for as long as the object lives;
 * - addresses are the same in every run of one build of this core (the
 *   machine is deterministic, its allocator with it) and move with another
 *   build; a project pins its build.
 * The bus is as long as the arena can ever get (its length is read once);
 * past the break it reads as zeros and ignores writes, and GetBusRanges
 * says where the break is now. Reading changes nothing in the machine: a
 * run inside the heap is answered with a pointer to it, and the buffer for
 * a run that crosses the break is in no savestate. The native reference has
 * no such arena, and no bus. */
#ifdef CHIMERA_GUEST
#include <unistd.h>
extern "C" {
struct ScvmRange { unsigned long start, size; };
struct ScvmLayout { struct ScvmRange elf, main_thread, alt_thread, sbrk, sealed, invis, plain, mmap; };
extern struct ScvmLayout __wbxsysinfo;
}
#define HEAP_RUN 65536
ECL_INVISIBLE static uint8_t g_heap_scratch[HEAP_RUN];
ECL_INVISIBLE static int64_t g_heap_ranges[4];
static uint8_t *heap_start(void) { return (uint8_t *)__wbxsysinfo.sbrk.start; }
static int64_t heap_capacity(void) { return (int64_t)__wbxsysinfo.sbrk.size; }
static int64_t heap_live(void)
{
	const int64_t brk = (int64_t)(uintptr_t)sbrk(0) - (int64_t)(uintptr_t)heap_start();
	return brk < 0 ? 0 : brk > heap_capacity() ? heap_capacity() : brk;
}
ECL_EXPORT int32_t GetBusCount(void) { return 1; }
ECL_EXPORT const char *GetBusName(int32_t i) { (void)i; return "Heap"; }
ECL_EXPORT int64_t GetBusSize(int32_t i) { (void)i; return heap_capacity(); }
ECL_EXPORT int32_t GetBusWritable(int32_t i) { (void)i; return 1; }
ECL_EXPORT int32_t PeekBus(int32_t bus, int32_t addr)
{
	if (bus != 0 || addr < 0 || addr >= heap_live()) return 0;
	return heap_start()[addr];
}
ECL_EXPORT void PokeBus(int32_t bus, int32_t addr, int32_t value)
{
	if (bus != 0 || addr < 0 || addr >= heap_live()) return;
	heap_start()[addr] = (uint8_t)value;
}
ECL_EXPORT const uint8_t *ReadBus(int32_t bus, int64_t addr, int32_t len)
{
	const int64_t n = len < 0 ? 0 : len > HEAP_RUN ? HEAP_RUN : len;
	const int64_t live = bus == 0 ? heap_live() : 0;
	if (addr >= 0 && addr + n <= live)
		return heap_start() + addr;
	memset(g_heap_scratch, 0, (size_t)n);
	if (addr >= 0 && addr < live)
		memcpy(g_heap_scratch, heap_start() + addr, (size_t)(live - addr));
	return g_heap_scratch;
}
ECL_EXPORT const int64_t *GetBusRanges(int32_t bus)
{
	if (bus != 0) return NULL;
	g_heap_ranges[0] = 0;
	g_heap_ranges[1] = heap_live();
	g_heap_ranges[2] = 0;
	g_heap_ranges[3] = 0;
	return g_heap_ranges;
}
#endif

/* the machine's own clock, in frames */
ECL_EXPORT uint64_t GetCycleCount(void) { return scvm_frames(); }
