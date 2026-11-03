/* run-native.c - the native reference for the test scripts.
 *
 * Links the SAME ScummVM, libraries and core the guest build uses, compiled
 * for this computer, with the sandbox's own math functions
 * (make-native-libm.sh), and drives the core's calls directly. The work dir
 * holds what the sandbox would see mounted: the game's zip, the "slots"
 * file naming it, the "settings" JSON, and any save files.
 *
 * usage: run-native <workdir> [gate-harness options]
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "gate-harness.h"

extern int Init(void);
extern const char *GetLoadError(void);
extern void SetButton(int32_t index, int32_t state);
extern void SetAxis(int32_t index, int32_t value);
extern void FrameAdvance(uint64_t packed);
extern uint32_t *GetVideoBgra(void);
extern int GetVideoWidth(void);
extern int GetVideoHeight(void);
extern int16_t *GetAudio(void);
extern int GetAudioSampleCount(void);
extern int InputWasRead(void);
extern void SetRenderingEnabled(int on);
extern int GetVsyncNumerator(void);
extern int GetVsyncDenominator(void);
extern uint64_t GetCycleCount(void);
extern int32_t GetSaveDataFileCount(void);
extern const char *GetSaveDataFileName(int32_t i);
extern int64_t GetSaveDataFileSize(int32_t i);
extern const uint8_t *GetSaveDataFileBuffer(int32_t i);

/* The sandbox's clock is the project's start and stands still (MACHINE-SPEC,
 * clock_gettime); the native link wraps the C library's clocks to say the
 * same. 1495889068 is the sandbox's own start. */
static int64_t g_clock_start = 1495889068;
int __wrap_clock_gettime(clockid_t id, struct timespec *ts)
{
	(void)id;
	ts->tv_sec = (time_t)g_clock_start;
	ts->tv_nsec = 0;
	return 0;
}
int __wrap_gettimeofday(struct timeval *tv, void *tz)
{
	(void)tz;
	if (tv)
	{
		tv->tv_sec = (time_t)g_clock_start;
		tv->tv_usec = 0;
	}
	return 0;
}
time_t __wrap_time(time_t *t)
{
	if (t)
		*t = (time_t)g_clock_start;
	return (time_t)g_clock_start;
}

/* --host-clock: the virtual-time test's control (chimera-machine.cpp) */
extern uint64_t (*scvm_host_ms)(void);
int __real_clock_gettime(clockid_t id, struct timespec *ts);
static uint64_t host_ms(void)
{
	struct timespec ts;
	__real_clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static void frame(void) { FrameAdvance(0); }
static const uint32_t *video(int *w, int *h)
{
	*w = GetVideoWidth();
	*h = GetVideoHeight();
	return GetVideoBgra();
}
static const int16_t *audio(int *n)
{
	*n = GetAudioSampleCount();
	return GetAudio();
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: run-native <workdir> [options]\n");
		return 2;
	}
	static struct gate_opts o;
	if (!gate_parse_opts(argc, argv, 2, &o))
		return 2;
	if (o.clockStart)
		g_clock_start = o.clockStart;
	if (o.hostClock)
		scvm_host_ms = host_ms;
	/* the sandbox's environment and nothing of this computer's (but the
	 * harness's own stall settings, which gate_run reads) */
	char stall[32] = "", every[32] = "";
	if (getenv("GATE_STALL_US")) snprintf(stall, sizeof stall, "%s", getenv("GATE_STALL_US"));
	if (getenv("GATE_STALL_EVERY")) snprintf(every, sizeof every, "%s", getenv("GATE_STALL_EVERY"));
	clearenv();
	setenv("WATERBOX", "1", 1);
	setenv("TZ", "UTC0", 1);
	if (stall[0]) setenv("GATE_STALL_US", stall, 1);
	if (every[0]) setenv("GATE_STALL_EVERY", every, 1);
	if (chdir(argv[1]) != 0)
	{
		perror(argv[1]);
		return 1;
	}

	struct gate_core c = {
		.init = Init,
		.load_error = GetLoadError,
		.set_button = SetButton,
		.set_axis = SetAxis,
		.frame = frame,
		.video = video,
		.audio = audio,
		.input_was_read = InputWasRead,
		.vsync_numerator = GetVsyncNumerator,
		.vsync_denominator = GetVsyncDenominator,
		.pre_frame = NULL,
		.set_rendering = SetRenderingEnabled,
		.clock = GetCycleCount,
		.savedata_count = GetSaveDataFileCount,
		.savedata_name = GetSaveDataFileName,
		.savedata_size = GetSaveDataFileSize,
		.savedata_buffer = GetSaveDataFileBuffer,
	};
	return gate_run(&c, &o);
}
