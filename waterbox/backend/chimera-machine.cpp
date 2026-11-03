/* chimera-machine.cpp - ScummVM as a Chimera machine.
 *
 * The OSystem every ScummVM backend provides, for a machine that runs one
 * frame at a time on its own clock:
 *
 * - ScummVM runs on a stack of its own (coroutine.c). Its engines own their
 *   main loops and wait inside them; where a wait reaches the end of the
 *   frame, the machine hands control back to the caller, and the next
 *   FrameAdvance hands it back to ScummVM. Nothing is threaded.
 * - The clock counts microseconds from power-on and moves only where ScummVM
 *   waits (delayMillis) and at the end of each frame: frame k ends at
 *   (k + 1) / 60 s. getMillis() reads it; the date (getTimeAndDate) is the
 *   project's clock start (the sandbox's clock_gettime, read once) plus it,
 *   in UTC. ScummVM's random numbers are seeded from these two, so they are
 *   the same in every run of a movie.
 * - A loop that never waits - one that reads the clock or asks for events
 *   until time has passed - would never see time pass on such a clock. A
 *   loop that reads the clock, asks for events or shows a picture
 *   SCVM_SPIN_LIMIT times without the clock moving ends the frame there, as
 *   a wait to the frame's end would. The calls are counted, so this is the
 *   same in every run.
 * - Timers (TimerManager::installTimerProc, which engines use for music
 *   and animation) run whenever the clock moves, as many times as their
 *   period has passed.
 * - The sound is ScummVM's mixer, run at the end of each frame for exactly
 *   one frame's samples: 735 stereo samples at 44100 Hz.
 * - Input is the controls' levels each frame (SetButton, SetAxis), turned
 *   into the events a real keyboard and mouse would have sent, at the start
 *   of the frame. A frame in which ScummVM did not ask for events is a lag
 *   frame.
 * - Files: the game's folder is the zip the project gave (chimera-fs.h);
 *   saved games live in the machine's memory (chimera-saves.h); ScummVM's
 *   own data files (each engine's .dat, the fonts, the GUI's themes and
 *   translations) are built into the core and read from its memory.
 * - ScummVM is started as `scummvm --path=/game --auto-detect`: its own
 *   detector finds the game in the folder. If it finds none, or the engine
 *   cannot start, the machine is refused at Init with ScummVM's reason.
 *   When the game ends (the player quits it), the machine stops: every
 *   later frame shows the last picture in silence.
 */
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <emulibc.h>
#include <waterbox_settings.h>
#include <waterbox_slots.h>

#include "audio/mixer_intern.h"
#include "backends/base-backend.h"
#include "backends/events/default/default-events.h"
#include "backends/keymapper/hardware-input.h"
#include "backends/modular-backend.h"
#include "backends/mutex/null/null-mutex.h"
#include "backends/timer/default/default-timer.h"
#include "base/main.h"
#include "common/archive.h"
#include "common/compression/unzip.h"
#include "common/config-manager.h"
#include "common/events.h"
#include "common/memstream.h"
#include "common/queue.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "chimera-fs.h"
#include "chimera-graphics.h"
#include "chimera-machine.h"
#include "chimera-saves.h"
#include "chimera-texts.h"
#include "controls.h"
#include "coroutine.h"

/* ScummVM's data files, a zip built into the core (core.mk, data.S) */
extern "C" const unsigned char chimera_engine_data[];
extern "C" const unsigned char chimera_engine_data_end[];

/* ------------------------------------------------------------ the machine's state */

/* The two stacks: ScummVM's own, and whichever called FrameAdvance. */
static void *g_gameSp;
static void *g_hostSp;

static uint64 g_usNow;          // the machine's clock: microseconds since power-on
static uint64 g_frame;          // frames finished
static int64 g_clockStart;      // the project's clock start, seconds since 1970 (UTC)
static uint32 g_spin;           // clock reads and polls since the clock last moved
static bool g_inputRead;        // ScummVM asked for events this frame
static bool g_engineStarted;    // ScummVM started the game's engine
static bool g_ended;            // ScummVM has finished; the machine shows its last picture
static bool g_inTimers;
static Common::String *g_endReason;  // why, when it was not the player's doing

static uint8 g_buttons[SCVM_BTN_COUNT], g_prevButtons[SCVM_BTN_COUNT];
static int32 g_axes[SCVM_AXIS_COUNT], g_prevAxes[SCVM_AXIS_COUNT];
static bool g_capsLock, g_numLock = true;

static int16 g_audio[SCVM_SAMPLES_PER_FRAME * 2];
static int g_audioCount;
static uint32 *g_video;
static int g_videoW = 320, g_videoH = 200;

static ChimeraSaveFileManager *g_saves;
static Common::String *g_gameZip;

static uint64 frameEndUs(uint64 k) { return (k + 1) * 1000000ull / SCVM_FPS; }

class OSystem_Chimera;
static OSystem_Chimera *g_chimera;

/* The firmware the project gave (the Roland ROMs), as ScummVM looks for it:
 * files by name in its search path. Only the set the Music device setting
 * names is there, so the setting alone decides which synthesizer plays
 * (ScummVM takes the CM-32L's ROMs over the MT-32's when it has both). The
 * files are read whole when ScummVM opens them. */
static const char *const kMt32Roms[] = { "MT32_CONTROL.ROM", "MT32_PCM.ROM", nullptr };
static const char *const kCm32lRoms[] = { "CM32L_CONTROL.ROM", "CM32L_PCM.ROM", nullptr };

class ChimeraRomArchive final : public Common::Archive {
public:
	explicit ChimeraRomArchive(const char *const *names) {
		for (int i = 0; names && names[i]; i++) {
			struct stat st;
			if (stat(names[i], &st) == 0)
				_names.push_back(names[i]);
		}
	}
	bool empty() const { return _names.empty(); }
	bool hasFile(const Common::Path &path) const override { return find(path) >= 0; }
	int listMembers(Common::ArchiveMemberList &list) const override {
		for (const Common::String &n : _names)
			list.push_back(Common::ArchiveMemberPtr(new Common::GenericArchiveMember(n, *this)));
		return (int)_names.size();
	}
	const Common::ArchiveMemberPtr getMember(const Common::Path &path) const override {
		const int i = find(path);
		return i < 0 ? Common::ArchiveMemberPtr() : Common::ArchiveMemberPtr(new Common::GenericArchiveMember(_names[i], *this));
	}
	Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override {
		const int i = find(path);
		if (i < 0)
			return nullptr;
		FILE *f = fopen(_names[i].c_str(), "rb");
		if (!f)
			return nullptr;
		fseek(f, 0, SEEK_END);
		const long size = ftell(f);
		fseek(f, 0, SEEK_SET);
		byte *data = (byte *)malloc(size > 0 ? size : 1);
		const size_t got = size > 0 ? fread(data, 1, (size_t)size, f) : 0;
		fclose(f);
		return new Common::MemoryReadStream(data, (uint32)got, DisposeAfterUse::YES);
	}

private:
	int find(const Common::Path &path) const {
		const Common::String name = path.baseName();
		for (uint i = 0; i < _names.size(); i++)
			if (_names[i].equalsIgnoreCase(name))
				return (int)i;
		return -1;
	}
	Common::StringArray _names;
};
static const char *const *g_roms;

#ifndef CHIMERA_GUEST
/* The virtual-time test's control, in the native reference only: with it
 * the clock ScummVM reads is the computer's (milliseconds since Init), and
 * a computer that stalls between frames gives another machine. */
extern "C" uint64_t (*scvm_host_ms)(void);
uint64_t (*scvm_host_ms)(void);
static uint64_t g_hostMsStart;
#endif

/* ------------------------------------------------------------ the OSystem */

class OSystem_Chimera final : public ModularGraphicsBackend, public Common::EventSource {
public:
	OSystem_Chimera() { _fsFactory = new ChimeraFilesystemFactory(); }

	void initBackend() override {
		_timerManager = new DefaultTimerManager();
		_eventManager = new DefaultEventManager(this);
		_savefileManager = g_saves;
		_graphicsManager = new ChimeraGraphicsManager();
		_mixer = new Audio::MixerImpl(SCVM_AUDIO_RATE, true, SCVM_SAMPLES_PER_FRAME);
		_mixer->setReady(true);
		BaseBackend::initBackend();
	}

	ChimeraGraphicsManager *gfx() { return (ChimeraGraphicsManager *)_graphicsManager; }
	Audio::MixerImpl *mixer() { return _mixer; }

	bool pollEvent(Common::Event &event) override {
		g_inputRead = true;
		spin();
		if (_events.empty())
			return false;
		event = _events.pop();
		return true;
	}
	void push(const Common::Event &e) { _events.push(e); }

	uint32 getMillis(bool skipRecord = false) override {
		if (!skipRecord)
			spin();
#ifndef CHIMERA_GUEST
		if (scvm_host_ms)
			return (uint32)(scvm_host_ms() - g_hostMsStart);
#endif
		return (uint32)(g_usNow / 1000);
	}
	void delayMillis(uint msecs) override;
	void getTimeAndDate(TimeDate &td, bool skipRecord = false) const override;
	Common::MutexInternal *createMutex() override { return new NullMutexInternal(); }
	Audio::Mixer *getMixer() override { return _mixer; }
	void quit() override;
	void fatalError() override;
	void logMessage(LogMessageType::Type type, const char *message) override {
		fputs(message, stderr);
		if (type == LogMessageType::kError) {
			// ScummVM's reason when it gives up (error() logs, then fatalError())
			*g_endReason = message;
			while (!g_endReason->empty() && (g_endReason->lastChar() == '\n' || g_endReason->lastChar() == ' '))
				g_endReason->deleteLastChar();
		}
	}
	void addSysArchivesToSearchSet(Common::SearchSet &s, int priority) override {
		Common::Archive *data = Common::makeZipArchive(new Common::MemoryReadStream(
			chimera_engine_data, (uint32)(chimera_engine_data_end - chimera_engine_data)));
		if (data)
			s.add("chimera-data", data, priority);
		ChimeraRomArchive *roms = new ChimeraRomArchive(g_roms);
		if (roms->empty())
			delete roms;
		else
			s.add("chimera-firmware", roms, priority + 1);
	}
	Common::SeekableReadStream *createConfigReadStream() override { return nullptr; }
	Common::WriteStream *createConfigWriteStream() override { return nullptr; }
	Common::String getSystemLanguage() const override { return "en_US"; }
	void engineInit() override { g_engineStarted = true; }

	Common::HardwareInputSet *getHardwareInputSet() override {
		Common::CompositeHardwareInputSet *set = new Common::CompositeHardwareInputSet();
		set->addHardwareInputSet(new Common::MouseHardwareInputSet(Common::defaultMouseButtons));
		set->addHardwareInputSet(new Common::KeyboardHardwareInputSet(Common::defaultKeys, Common::defaultModifiers));
		return set;
	}

	/* the clock moved: the timers whose time came run, in order */
	void runTimers() {
		if (g_inTimers || !_timerManager)
			return;
		g_inTimers = true;
		((DefaultTimerManager *)_timerManager)->handler();
		g_inTimers = false;
	}

	void spin();
	void endFrame();
	void endMachine() __attribute__((noreturn));

private:
	Audio::MixerImpl *_mixer = nullptr;
	Common::Queue<Common::Event> _events;
};

/* The end of a frame: the clock is at the frame's end, the frame's sound is
 * mixed, and the caller gets control back. Returns when the next frame
 * starts. */
void OSystem_Chimera::endFrame() {
	g_usNow = frameEndUs(g_frame);
	runTimers();
	if (_mixer) {
		memset(g_audio, 0, sizeof g_audio);
		g_audioCount = _mixer->mixCallback((byte *)g_audio, sizeof g_audio);
		if (g_audioCount < SCVM_SAMPLES_PER_FRAME) {
			memset(g_audio + g_audioCount * 2, 0, (SCVM_SAMPLES_PER_FRAME - g_audioCount) * 4);
			g_audioCount = SCVM_SAMPLES_PER_FRAME;
		}
	} else {
		memset(g_audio, 0, sizeof g_audio);
		g_audioCount = SCVM_SAMPLES_PER_FRAME;
	}
	g_spin = 0;
	chimera_co_switch(&g_gameSp, g_hostSp);
	g_spin = 0;
}

void OSystem_Chimera::spin() {
	if (++g_spin >= SCVM_SPIN_LIMIT)
		endFrame();
}

void OSystem_Chimera::delayMillis(uint msecs) {
	if (msecs == 0) {
		spin();
		return;
	}
	const uint64 target = g_usNow + (uint64)msecs * 1000;
	for (;;) {
		const uint64 end = frameEndUs(g_frame);
		if (target < end) {
			g_usNow = target;
			runTimers();
			break;
		}
		endFrame();
		if (g_usNow >= target)
			break;
	}
	g_spin = 0;
}

/* days since 1970-01-01 to a civil date (Howard Hinnant's algorithm), so the
 * date never goes near the C library's time zone */
static void civilFromDays(int64 z, int &y, int &m, int &d) {
	z += 719468;
	const int64 era = (z >= 0 ? z : z - 146096) / 146097;
	const int64 doe = z - era * 146097;
	const int64 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	const int64 doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	const int64 mp = (5 * doy + 2) / 153;
	d = (int)(doy - (153 * mp + 2) / 5 + 1);
	m = (int)(mp < 10 ? mp + 3 : mp - 9);
	y = (int)(yoe + era * 400 + (m <= 2));
}

void OSystem_Chimera::getTimeAndDate(TimeDate &td, bool skipRecord) const {
	const int64 secs = g_clockStart + (int64)(g_usNow / 1000000);
	int64 days = secs / 86400, rem = secs % 86400;
	if (rem < 0) {
		rem += 86400;
		days--;
	}
	int y, m, d;
	civilFromDays(days, y, m, d);
	td.tm_sec = (int)(rem % 60);
	td.tm_min = (int)(rem / 60 % 60);
	td.tm_hour = (int)(rem / 3600);
	td.tm_mday = d;
	td.tm_mon = m - 1;
	td.tm_year = y - 1900;
	td.tm_wday = (int)(((days % 7) + 11) % 7);  // 1970-01-01 was a Thursday
}

void OSystem_Chimera::endMachine() {
	g_ended = true;
	for (;;)
		endFrame();
}

void OSystem_Chimera::quit() { endMachine(); }

void OSystem_Chimera::fatalError() {
	if (g_endReason->empty())
		*g_endReason = "fatal error";
	endMachine();
}

/* base/main.cpp's two hooks (patches/0001): no launcher, and a game that
 * cannot run says why instead of showing ScummVM's error box */
void chimera_launcher_reached() {
	if (g_endReason->empty() && !g_engineStarted)
		*g_endReason = chimeraText1("error.no_game", *g_gameZip);
	g_chimera->endMachine();
}

void chimera_game_error(const char *desc) {
	*g_endReason = desc;
	g_chimera->endMachine();
}

/* ------------------------------------------------------------ the graphics manager's calls */

void chimeraMachineMouseWarped(int x, int y) {
	if (!g_chimera)
		return;
	Common::Event e;
	e.type = Common::EVENT_MOUSEMOVE;
	e.mouse = Common::Point(x, y);
	g_chimera->push(e);
}

void chimeraMachineScreenUpdated() {
	if (g_chimera)
		g_chimera->spin();
}

/* ------------------------------------------------------------ ScummVM's stack */

static const char *g_argv[16];
static int g_argc;

extern "C" void chimera_co_entry(void) {
	scummvm_main(g_argc, g_argv);
	// the game is over (or never started)
	if (g_endReason->empty() && !g_engineStarted)
		*g_endReason = chimeraText1("error.no_game", *g_gameZip);
	g_chimera->endMachine();
}

/* ------------------------------------------------------------ input */

/* the character a key types, by the rule of ScummVM's SDL 2 backend
 * (SdlEventSource::mapKey), with the US layout: what SDL's text input would
 * give when neither Ctrl nor Alt is held, else the key's own code */
static uint16 keyAscii(const struct scvm_control &c, bool shift, bool ctrl, bool alt) {
	const int code = c.code;
	if (code >= Common::KEYCODE_F1 && code <= Common::KEYCODE_F9)
		return (uint16)(code - Common::KEYCODE_F1 + Common::ASCII_F1);
	if (code >= Common::KEYCODE_KP0 && code <= Common::KEYCODE_KP9)
		return g_numLock ? (uint16)(code - Common::KEYCODE_KP0 + '0') : 0;
	if (code >= Common::KEYCODE_UP && code <= Common::KEYCODE_PAGEDOWN)
		return (uint16)code;
	int unicode = 0;
	if (!ctrl && !alt && c.ascii >= 0x20) {
		if (c.ascii >= 'a' && c.ascii <= 'z')
			unicode = (shift != g_capsLock) ? c.shifted : c.ascii;
		else
			unicode = shift ? c.shifted : c.ascii;
	}
	if (unicode)
		return (uint16)unicode;
	if (code >= 'a' && code <= 'z' && shift)
		return (uint16)(code & ~0x20);
	if (code >= Common::KEYCODE_NUMLOCK && code < Common::KEYCODE_LAST)
		return 0;
	return (uint16)code;
}

static void queueKey(const struct scvm_control &c, bool down) {
	const bool shift = g_buttons[SCVM_KEY_LSHIFT] || g_buttons[SCVM_KEY_RSHIFT];
	const bool ctrl = g_buttons[SCVM_KEY_LCTRL] || g_buttons[SCVM_KEY_RCTRL];
	const bool alt = g_buttons[SCVM_KEY_LALT] || g_buttons[SCVM_KEY_RALT];
	if (down && c.code == Common::KEYCODE_CAPSLOCK)
		g_capsLock = !g_capsLock;
	if (down && c.code == Common::KEYCODE_NUMLOCK)
		g_numLock = !g_numLock;
	byte flags = 0;
	if (shift) flags |= Common::KBD_SHIFT;
	if (ctrl) flags |= Common::KBD_CTRL;
	if (alt) flags |= Common::KBD_ALT;
	if (g_capsLock) flags |= Common::KBD_CAPS;
	if (g_numLock) flags |= Common::KBD_NUM;
	Common::Event e;
	e.type = down ? Common::EVENT_KEYDOWN : Common::EVENT_KEYUP;
	e.kbd = Common::KeyState((Common::KeyCode)c.code, keyAscii(c, shift, ctrl, alt), flags);
	g_chimera->push(e);
}

/* the levels of this frame against the last frame's, as the events a real
 * keyboard and mouse would have sent: the pointer first, then each control
 * that changed, in waterbox.config's order */
static void applyInput() {
	ChimeraGraphicsManager *gfx = g_chimera->gfx();
	const int w = gfx->shownWidth(), h = gfx->shownHeight();
	if (g_axes[0] != g_prevAxes[0] || g_axes[1] != g_prevAxes[1]) {
		const int mouseX = (int)((int64)CLIP<int32>(g_axes[0], 0, 65535) * w / 65536);
		const int mouseY = (int)((int64)CLIP<int32>(g_axes[1], 0, 65535) * h / 65536);
		gfx->setMousePosition(mouseX, mouseY);
		Common::Event e;
		e.type = Common::EVENT_MOUSEMOVE;
		e.mouse = Common::Point(mouseX, mouseY);
		g_chimera->push(e);
		g_prevAxes[0] = g_axes[0];
		g_prevAxes[1] = g_axes[1];
	}
	const Common::Point here(gfx->mouseX(), gfx->mouseY());
	for (int i = 0; i < SCVM_BTN_COUNT; i++) {
		if (g_buttons[i] == g_prevButtons[i])
			continue;
		const bool down = g_buttons[i] != 0;
		const struct scvm_control &c = scvm_controls[i];
		Common::Event e;
		e.mouse = here;
		switch (c.kind) {
		case SCVM_CTRL_MOUSE:
			if (c.code == 0) e.type = down ? Common::EVENT_LBUTTONDOWN : Common::EVENT_LBUTTONUP;
			else if (c.code == 1) e.type = down ? Common::EVENT_RBUTTONDOWN : Common::EVENT_RBUTTONUP;
			else e.type = down ? Common::EVENT_MBUTTONDOWN : Common::EVENT_MBUTTONUP;
			g_chimera->push(e);
			break;
		case SCVM_CTRL_WHEEL:
			if (down) {
				e.type = c.code == 0 ? Common::EVENT_WHEELUP : Common::EVENT_WHEELDOWN;
				g_chimera->push(e);
			}
			break;
		default:
			queueKey(c, down);
			break;
		}
		g_prevButtons[i] = g_buttons[i];
	}
}

/* ------------------------------------------------------------ the calls of wbx-entry.c */

/* one frame: ScummVM runs from where it stopped to the end of the frame */
static void runFrame() {
	g_inputRead = false;
	chimera_co_switch(&g_hostSp, g_gameSp);
	g_frame++;
}

/* the project's saved games: a zip of ScummVM's save files (what Export Save
 * Data writes), put in the machine's memory before ScummVM starts */
static bool readSaves(Common::String &error) {
	char name[512];
	if (!wbx_slot_first("savedata", name, sizeof name))
		return true;
	ChimeraZip zip;
	if (!zip.open(name, error))
		return false;
	for (int i = 2; i < zip.count(); i++) {
		const ChimeraZipEntry *e = zip.entry(i);
		if (e->dir)
			continue;
		Common::SeekableReadStream *in = zip.createReadStream(i);
		if (!in) {
			error = chimeraText1("error.not_zip", name);
			return false;
		}
		Common::Array<byte> data;
		data.resize((uint)in->size());
		if (data.size() && in->read(data.data(), data.size()) != data.size()) {
			delete in;
			error = chimeraText1("error.not_zip", name);
			return false;
		}
		delete in;
		g_saves->put(e->name, data.data(), data.size());
	}
	return true;
}

static void setting(const char *key, char *out, int outsz, const char *dflt) {
	if (!wbx__setting_raw(key, out, outsz))
		snprintf(out, outsz, "%s", dflt);
}

int scvm_init(char *err, int errsz) {
	g_endReason = new Common::String();
	g_gameZip = new Common::String();
	g_video = (uint32 *)calloc((size_t)SCVM_MAX_W * SCVM_MAX_H, 4);

	// the project's clock start: what the sandbox's clock says before any
	// time has passed (Chimera's --clock-start; the native harness says the same)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	g_clockStart = (int64)ts.tv_sec;

	char zip[512];
	if (!wbx_slot_first("game", zip, sizeof zip)) {
		snprintf(err, errsz, "error.no_game_slot");
		return 0;
	}
	*g_gameZip = zip;
	g_chimeraZip = new ChimeraZip();
	Common::String why;
	if (!g_chimeraZip->open(zip, why)) {
		snprintf(err, errsz, "%s", why.c_str());
		return 0;
	}

	g_saves = new ChimeraSaveFileManager();
	if (!readSaves(why)) {
		snprintf(err, errsz, "%s", why.c_str());
		return 0;
	}

	// the settings, as ScummVM's command line takes them
	static char music[64], opl[64], subs[16], args[4][96];
	setting("musicDriver", music, sizeof music, "auto");
	setting("oplEmulator", opl, sizeof opl, "auto");
	setting("subtitles", subs, sizeof subs, "true");
	g_roms = !strcmp(music, "mt32") ? kMt32Roms : !strcmp(music, "cm32l") ? kCm32lRoms : nullptr;
	for (int i = 0; g_roms && g_roms[i]; i++) {
		struct stat st;
		if (stat(g_roms[i], &st) != 0) {
			snprintf(err, errsz, "%s", chimeraText1("error.rom_missing", g_roms[i]).c_str());
			return 0;
		}
	}
	snprintf(args[0], sizeof args[0], "--music-driver=%s", strcmp(music, "cm32l") ? music : "mt32");
	snprintf(args[1], sizeof args[1], "--opl-driver=%s", opl);
	snprintf(args[2], sizeof args[2], "%s", strcmp(subs, "false") ? "--subtitles" : "--no-subtitles");
	g_argc = 0;
	g_argv[g_argc++] = "scummvm";
	g_argv[g_argc++] = "--path=" CHIMERA_GAME_PATH;
	g_argv[g_argc++] = args[0];
	g_argv[g_argc++] = args[1];
	g_argv[g_argc++] = args[2];
	g_argv[g_argc++] = "--auto-detect";
	g_argv[g_argc] = nullptr;

	g_chimera = new OSystem_Chimera();
	g_system = g_chimera;
#ifndef CHIMERA_GUEST
	if (scvm_host_ms)
		g_hostMsStart = scvm_host_ms();
#endif

	g_gameSp = chimera_co_create(16 << 20);
	if (!g_gameSp) {
		snprintf(err, errsz, "error.no_memory");
		return 0;
	}

	// ScummVM starts: detection, the engine, up to the end of the frame in
	// which the engine began
	for (int f = 0; f < SCVM_START_FRAMES && !g_engineStarted && !g_ended; f++)
		runFrame();
	if (g_ended || !g_engineStarted) {
		if (g_endReason->hasPrefix("error."))
			snprintf(err, errsz, "%s", g_endReason->c_str());
		else if (!g_endReason->empty())
			snprintf(err, errsz, "%s", chimeraText1("error.scummvm", *g_endReason).c_str());
		else
			snprintf(err, errsz, "%s", chimeraText1("error.no_game", *g_gameZip).c_str());
		return 0;
	}
	return 1;
}

void scvm_set_button(int index, int on) {
	if (index >= 0 && index < SCVM_BTN_COUNT)
		g_buttons[index] = on ? 1 : 0;
}

void scvm_set_axis(int index, int value) {
	if (index >= 0 && index < SCVM_AXIS_COUNT)
		g_axes[index] = value;
}

void scvm_frame(int render) {
	if (!g_ended)
		applyInput();
	runFrame();
	if (render)
		g_chimera->gfx()->compose(g_video, SCVM_MAX_W, SCVM_MAX_H, g_videoW, g_videoH);
}

const uint32_t *scvm_video(int *w, int *h) {
	*w = g_videoW;
	*h = g_videoH;
	return g_video;
}

const int16_t *scvm_audio(int *n) {
	*n = g_audioCount;
	return g_audio;
}

void scvm_aspect(int *x, int *y) { g_chimera->gfx()->aspect(*x, *y); }

int scvm_input_was_read(void) { return g_inputRead ? 1 : 0; }

uint64_t scvm_frames(void) { return g_frame; }

int32_t scvm_saves_count(void) { return g_saves ? (int32_t)g_saves->files().size() : 0; }
const char *scvm_saves_name(int32_t i) { return g_saves->files()[i].name.c_str(); }
int64_t scvm_saves_size(int32_t i) { return g_saves->files()[i].data.size(); }
const uint8_t *scvm_saves_data(int32_t i) { return g_saves->files()[i].data.data(); }
