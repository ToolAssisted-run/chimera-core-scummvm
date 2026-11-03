# scummvm.mk - ScummVM itself, configured by its own configure script in a
# folder of this build (build/<flavor>/scummvm; the submodule is never
# written to) and built by its own Makefile.
#
# ScummVM is configured for its "null" backend, the one with no window, no
# sound card and no host devices, and with every optional library pointed at
# deps.mk's copies (pkg-config sees those and nothing else). What is linked
# into the core is every object ScummVM's Makefile would link into its own
# program except the null backend's (which holds main() and an OSystem on the
# computer's clock): scummvm-objs.mk lists them, and the core brings its own
# OSystem (backend/).
#
# Engines: ScummVM's default set, plus testbed (its test engine, which the
# tests run), less colony and twp, which draw only through OpenGL (left out
# until the owner decides; docs/PLAN.md).

SVM_SRC := $(ROOT)/extern/scummvm
SVM     := $(B)/scummvm

SVM_ENGINES := --enable-engine=testbed --disable-engine=colony,twp

# The guest build is a cross build (guest.mk sets SVM_HOST): configure must
# not try to run what the guest compiler makes, which only runs in the
# sandbox, or it falls back to the computer's own compiler.

# What ScummVM must not look for on this computer: devices, network, the
# desktop, the processor's extensions, OpenGL, and the libraries deps.mk does
# not build.
SVM_OPTIONS := $(SVM_HOST) --backend=null --enable-release --disable-debug --opengl-mode=none \
	--disable-alsa --disable-seq-midi --disable-sndio --disable-timidity --disable-opl2lpt --disable-retrowave \
	--disable-nfm --disable-tremor --disable-mikmod --disable-openmpt --disable-mpcdec --disable-a52 --disable-vpx \
	--disable-fluidsynth --disable-fluidlite --disable-sonivox --disable-readline --disable-taskbar \
	--disable-system-dialogs --disable-sdlnet --disable-libcurl --disable-enet --disable-cloud --disable-dlc \
	--disable-scummvmdlc --disable-updates --disable-libunity --disable-tts --disable-gtk --disable-imgui \
	--disable-opengl-game --disable-opengl-game-classic --disable-opengl-game-shaders --disable-discord \
	--disable-eventrecorder --disable-system-printing --disable-text-console --disable-ext-sse2 \
	--disable-ext-avx2 --disable-nasm \
	--with-zlib-prefix=$(DEPS) --with-png-prefix=$(DEPS) --with-jpeg-prefix=$(DEPS) --with-ogg-prefix=$(DEPS) \
	--with-vorbis-prefix=$(DEPS) --with-flac-prefix=$(DEPS) --with-mad-prefix=$(DEPS) --with-faad-prefix=$(DEPS) \
	--with-theoradec-prefix=$(DEPS) --with-mpeg2-prefix=$(DEPS) --with-gif-prefix=$(DEPS) \
	--with-fribidi-prefix=$(DEPS) \
	$(SVM_ENGINES)

SVM_ENV := CC="$(CC)" CXX="$(CXX)" LD="$(CXX)" CXXFLAGS="$(SVM_CXXFLAGS)" CPPFLAGS="" LDFLAGS="$(SVM_LDFLAGS)" \
	PKG_CONFIG_LIBDIR="$(DEPS)/lib/pkgconfig" PKG_CONFIG_PATH="" PKG_CONFIG_SYSROOT_DIR=""

$(call flags_stamp,$(SVM),$(SVM_ENV) $(SVM_OPTIONS))

# configure, again whenever its options or the libraries change
$(SVM)/config.mk: $(SVM)/flags $(DEP_ARCHIVES) $(DEPS)/include.stamp $(PATCH_STAMP)
	cd $(SVM) && $(SVM_ENV) $(SVM_SRC)/configure $(SVM_OPTIONS) > configure.out 2>&1 || \
		{ tail -30 $(SVM)/configure.out; echo "ScummVM's configure failed: $(SVM)/config.log" >&2; exit 1; }
	@grep -q '^#define USE_PNG' $(SVM)/config.h || { echo "configure did not take deps.mk's libraries (see $(SVM)/config.log)" >&2; exit 1; }

# ScummVM's own Makefile builds its objects; scummvm-objs.mk adds the target
# that stops before its program and writes what would have been linked.
# __DATE__ and __TIME__ (ScummVM's build date, which its menus show and
# some engines write) are the pinned commit's, in both builds: the compiler
# takes them from SOURCE_DATE_EPOCH.
SVM_EPOCH := $(shell git -C $(SVM_SRC) log -1 --format=%ct)
# the core's C++ sees the machine's headers: emulibc (miniBox's in the
# guest, native-shim's natively), the settings and slots readers
CORE_INCS := -I$(ROOT)/waterbox/backend -I$(ROOT)/waterbox $(EMULIBC_INC) -I$(MB)/source/guest/include -I$(MB)/extern/jsmn
$(SVM)/objs.txt: $(SVM)/config.mk scummvm-objs.mk $(PATCH_STAMP) FORCE
	+SOURCE_DATE_EPOCH=$(SVM_EPOCH) $(MAKE) -C $(SVM) -f Makefile -f $(ROOT)/waterbox/scummvm-objs.mk chimera-objs \
		CHIMERA_DIR=$(ROOT)/waterbox CHIMERA_INCS="$(CORE_INCS)"

scummvm: $(SVM)/objs.txt
.PHONY: scummvm FORCE
FORCE:
