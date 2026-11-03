# deps.mk - the libraries ScummVM's engines and codecs link, each its
# current upstream release (deps.lock: the version and the SHA-256 of its
# tarball; fetch-deps.sh downloads, checks and unpacks them into build/src),
# built once per build (native.mk, guest.mk) with that build's compiler and
# flags. Nothing of the computer's own libraries is used: the two builds must
# be the same program.
#
# No library's own build system runs. The sources each one's own build
# compiles are listed here, and what that build would generate (a config.h,
# jconfig.h, mad.h...) is in deps-config/, written once by hand (its README).
# Every library is built as plain C: no assembly, no intrinsics, nothing that
# asks the processor what it can do, so the same code runs on every computer.
#
# Each library becomes $(DEPS)/lib/lib<name>.a with its headers under
# $(DEPS)/include and a pkg-config file under $(DEPS)/lib/pkgconfig, so that
# ScummVM's own configure finds exactly these and nothing else
# (scummvm.mk points PKG_CONFIG_LIBDIR there and nowhere else).
#
# Included by native.mk and guest.mk, which set B, CC and DEPS_CFLAGS.

SRC  := $(ROOT)/build/src
DEPS := $(B)/deps
CFG  := $(ROOT)/waterbox/deps-config

# the sources: make reads this file after fetch-deps.sh has made it (and
# starts again), so every library is unpacked before anything is built
include $(SRC)/deps.stamp.mk
$(SRC)/deps.stamp.mk: deps.lock fetch-deps.sh
	bash fetch-deps.sh
	@echo '# written by fetch-deps.sh' > $@

# a library's folder in build/src, by its name in deps.lock
lockdir = $(SRC)/$(shell awk '$$1 == "$(1)" { print $$3 }' $(ROOT)/waterbox/deps.lock)

# what every library is compiled with, on top of the build's own flags; a
# new release (deps.lock) or configuration (deps-config) rebuilds them all
DEPS_COMMON := -O2 -w -fno-strict-aliasing
DEPS_PIN := $(shell find $(CFG) -type f | sort | xargs cat $(ROOT)/waterbox/deps.lock | sha256sum | cut -c1-16)

$(call flags_stamp,$(DEPS),$(CC) | $(DEPS_CFLAGS) | $(DEPS_COMMON) | $(DEPS_PIN))

# ---- zlib
ZLIB_DIR := $(call lockdir,zlib)
ZLIB_NAMES := adler32 compress crc32 deflate gzclose gzlib gzread gzwrite infback inffast inflate inftrees trees \
	uncompr zutil
ZLIB_FLAGS := -I$(ZLIB_DIR) -DHAVE_UNISTD_H -DHAVE_STDARG_H
ZLIB_HDRS := $(ZLIB_DIR)/zlib.h $(ZLIB_DIR)/zconf.h

# ---- libpng, with its prebuilt pnglibconf.h; no ARM, Intel, MIPS,
# PowerPC, LoongArch or RISC-V code
PNG_DIR := $(call lockdir,libpng)
PNG_NAMES := png pngerror pngget pngmem pngpread pngread pngrio pngrtran pngrutil pngset pngtrans pngwio pngwrite \
	pngwtran pngwutil
PNG_FLAGS := -I$(PNG_DIR) -I$(DEPS)/include -I$(ZLIB_DIR) -DPNG_ARM_NEON_OPT=0 -DPNG_INTEL_SSE_OPT=0 \
	-DPNG_MIPS_MSA_OPT=0 -DPNG_MIPS_MMI_OPT=0 -DPNG_POWERPC_VSX_OPT=0 -DPNG_LOONGARCH_LSX_OPT=0 -DPNG_RISCV_RVV_OPT=0
PNG_HDRS := $(PNG_DIR)/png.h $(PNG_DIR)/pngconf.h

# ---- libjpeg-turbo (its libjpeg API 8, no SIMD). Its sources are compiled
# once for each precision the library offers (8, 12 and 16 bits a sample),
# through the small wrapper files in src/wrapper, as its CMake does.
JPEG_DIR := $(call lockdir,libjpeg-turbo)
JPEG_NAMES := jcapimin wrapper/jcapistd-8 wrapper/jcapistd-12 wrapper/jcapistd-16 wrapper/jccoefct-8 \
	wrapper/jccoefct-12 wrapper/jccolor-8 wrapper/jccolor-12 wrapper/jccolor-16 wrapper/jcdctmgr-8 \
	wrapper/jcdctmgr-12 wrapper/jcdiffct-8 wrapper/jcdiffct-12 wrapper/jcdiffct-16 jchuff jcicc jcinit jclhuff \
	wrapper/jclossls-8 wrapper/jclossls-12 wrapper/jclossls-16 wrapper/jcmainct-8 wrapper/jcmainct-12 \
	wrapper/jcmainct-16 jcmarker jcmaster jcomapi jcparam jcphuff wrapper/jcprepct-8 wrapper/jcprepct-12 \
	wrapper/jcprepct-16 wrapper/jcsample-8 wrapper/jcsample-12 wrapper/jcsample-16 jctrans jdapimin \
	wrapper/jdapistd-8 wrapper/jdapistd-12 wrapper/jdapistd-16 jdatadst jdatasrc wrapper/jdcoefct-8 \
	wrapper/jdcoefct-12 wrapper/jdcolor-8 wrapper/jdcolor-12 wrapper/jdcolor-16 wrapper/jddctmgr-8 \
	wrapper/jddctmgr-12 wrapper/jddiffct-8 wrapper/jddiffct-12 wrapper/jddiffct-16 jdhuff jdicc jdinput jdlhuff \
	wrapper/jdlossls-8 wrapper/jdlossls-12 wrapper/jdlossls-16 wrapper/jdmainct-8 wrapper/jdmainct-12 \
	wrapper/jdmainct-16 jdmarker jdmaster wrapper/jdmerge-8 wrapper/jdmerge-12 jdphuff wrapper/jdpostct-8 \
	wrapper/jdpostct-12 wrapper/jdpostct-16 wrapper/jdsample-8 wrapper/jdsample-12 wrapper/jdsample-16 jdtrans \
	jerror jfdctflt wrapper/jfdctfst-8 wrapper/jfdctfst-12 wrapper/jfdctint-8 wrapper/jfdctint-12 \
	wrapper/jidctflt-8 wrapper/jidctflt-12 wrapper/jidctfst-8 wrapper/jidctfst-12 wrapper/jidctint-8 \
	wrapper/jidctint-12 wrapper/jidctred-8 wrapper/jidctred-12 jmemmgr jmemnobs jpeg_nbits wrapper/jquant1-8 \
	wrapper/jquant1-12 wrapper/jquant2-8 wrapper/jquant2-12 wrapper/jutils-8 wrapper/jutils-12 wrapper/jutils-16 \
	jaricom jcarith jdarith
JPEG_FLAGS := -I$(CFG)/jpeg -I$(JPEG_DIR)/src
JPEG_HDRS := $(JPEG_DIR)/src/jpeglib.h $(CFG)/jpeg/jconfig.h $(JPEG_DIR)/src/jmorecfg.h $(JPEG_DIR)/src/jerror.h

# ---- FreeType: its base and every module its default ftmodule.h lists,
# each module as its one amalgamated file (docs/INSTALL.ANY); compressed
# fonts through deps' zlib
FT_DIR := $(call lockdir,freetype)
FT_NAMES := base/ftsystem base/ftinit base/ftdebug base/ftbase base/ftbbox base/ftglyph base/ftbdf base/ftbitmap \
	base/ftcid base/ftfstype base/ftgasp base/ftgxval base/ftmm base/ftotval base/ftpatent base/ftpfr \
	base/ftstroke base/ftsynth base/fttype1 base/ftwinfnt autofit/autofit bdf/bdf cff/cff cid/type1cid \
	hvf/hvf pcf/pcf pfr/pfr sfnt/sfnt truetype/truetype type1/type1 type42/type42 winfonts/winfnt \
	smooth/smooth raster/raster sdf/sdf svg/svg cache/ftcache gzip/ftgzip lzw/ftlzw psaux/psaux \
	pshinter/pshinter psnames/psnames
FT_FLAGS := -I$(FT_DIR)/include -I$(ZLIB_DIR) -DFT2_BUILD_LIBRARY -DFT_CONFIG_OPTION_SYSTEM_ZLIB

# ---- FriBidi (its release carries the generated Unicode tables)
FRIBIDI_DIR := $(call lockdir,fribidi)
FRIBIDI_NAMES := fribidi-arabic fribidi-bidi-types fribidi-bidi fribidi-brackets fribidi-char-sets-cap-rtl \
	fribidi-char-sets-cp1255 fribidi-char-sets-cp1256 fribidi-char-sets-iso8859-6 fribidi-char-sets-iso8859-8 \
	fribidi-char-sets-utf8 fribidi-char-sets fribidi-deprecated fribidi-joining-types fribidi-joining \
	fribidi-mirroring fribidi-run fribidi-shape fribidi
FRIBIDI_FLAGS := -I$(CFG)/fribidi -I$(FRIBIDI_DIR)/lib -DHAVE_CONFIG_H -DFRIBIDI_BUILD -DFRIBIDI_LIB_STATIC

# ---- libogg and libvorbis (decoding only: ScummVM never encodes)
OGG_DIR := $(call lockdir,libogg)
OGG_NAMES := bitwise framing
OGG_FLAGS := -I$(CFG)/ogg -I$(OGG_DIR)/include
VORBIS_DIR := $(call lockdir,libvorbis)
VORBIS_NAMES := analysis bitrate block codebook envelope floor0 floor1 info lookup lpc lsp mapping0 mdct psy \
	registry res0 sharedbook smallft synthesis window
VORBIS_FLAGS := -I$(VORBIS_DIR)/include -I$(VORBIS_DIR)/lib $(OGG_FLAGS)
VORBISFILE_NAMES := vorbisfile
VORBISFILE_FLAGS := $(VORBIS_FLAGS)

# ---- libFLAC: its decoder, no intrinsics (FLAC__NO_ASM), no Ogg
FLAC_DIR := $(call lockdir,flac)
FLAC_NAMES := bitmath bitreader cpu crc fixed float format lpc md5 memory metadata_object stream_decoder window
FLAC_FLAGS := -I$(CFG)/flac -DHAVE_CONFIG_H -I$(FLAC_DIR)/src/libFLAC/include -I$(FLAC_DIR)/include -DFLAC__NO_ASM \
	-DFLAC__NO_DLL

# ---- libmad (Tenacity's continuation of it): 64-bit fixed-point C
# (FPM_64BIT, in deps-config/mad/mad.h), as its CMake chooses on x86-64
MAD_DIR := $(call lockdir,libmad)
MAD_NAMES := bit decoder fixed frame huffman layer12 layer3 stream synth timer version
MAD_FLAGS := -I$(CFG)/mad -I$(MAD_DIR)/src -DOPT_SPEED -DHAVE_SYS_TYPES_H -DHAVE_SYS_WAIT_H -DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_STAT_H -DHAVE_UNISTD_H -DHAVE_ASSERT_H -DHAVE_FCNTL_H -DHAVE_LIMITS_H -DHAVE_FTRUNCATE -DHAVE_PIPE \
	-DHAVE_FORK -DHAVE_WAITPID

# ---- FAAD2: floating point, with dynamic range control (its CMake's
# "faad" library: every file in libfaad/)
FAAD_DIR := $(call lockdir,faad2)
FAAD_NAMES := bits cfft common decoder drc drm_dec error filtbank hcr huffman ic_predict is lt_predict mdct mp4 ms \
	output pns ps_dec ps_syntax pulse rvlc sbr_dct sbr_dec sbr_e_nf sbr_fbt sbr_hfadj sbr_hfgen sbr_huff sbr_qmf \
	sbr_syntax sbr_tf_grid specrec ssr ssr_fb ssr_ipqf syntax tns
FAAD_FLAGS := -I$(CFG)/faad -I$(FAAD_DIR)/include -I$(FAAD_DIR)/libfaad -DHAVE_INTTYPES_H=1 -DHAVE_MEMCPY=1 \
	-DHAVE_STRING_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 '-DPACKAGE_VERSION="2.11.4"' \
	-DAPPLY_DRC -DHAVE_LRINTF=1 -ffloat-store

# ---- libtheora's decoder (plain C: no x86 code without OC_X86_ASM)
THEORA_DIR := $(call lockdir,libtheora)
THEORA_NAMES := apiwrapper bitpack decapiwrapper decinfo decode dequant fragment huffdec idct info internal quant state
THEORA_FLAGS := -I$(THEORA_DIR)/include $(OGG_FLAGS)

# ---- libmpeg2: the C code alone (deps-config/mpeg2/config.h names no
# architecture, so no MMX code is built or chosen)
MPEG2_DIR := $(call lockdir,libmpeg2)
MPEG2_NAMES := alloc header decode slice motion_comp idct cpu_accel cpu_state
MPEG2_FLAGS := -I$(CFG)/mpeg2 -DHAVE_CONFIG_H -I$(MPEG2_DIR)/include -I$(MPEG2_DIR)/libmpeg2

# ---- giflib
GIF_DIR := $(call lockdir,giflib)
GIF_NAMES := dgif_lib egif_lib gifalloc gif_err gif_font gif_hash openbsd-reallocarray quantize
GIF_FLAGS := -I$(GIF_DIR)

define dep_lib
$(1)_OBJS := $$(addprefix $$(DEPS)/obj/$(1)/,$$(addsuffix .o,$$($(1)_NAMES)))
$$(DEPS)/obj/$(1)/%.o: $(2)/%.c $$(DEPS)/flags $$(DEPS)/include.stamp
	@mkdir -p $$(dir $$@)
	$$(CC) $$(DEPS_CFLAGS) $$(DEPS_COMMON) $$($(1)_FLAGS) -c -o $$@ $$<
$$(DEPS)/lib/lib$(3).a: $$($(1)_OBJS)
	@mkdir -p $$(dir $$@)
	rm -f $$@
	ar rcs $$@ $$^
endef

$(eval $(call dep_lib,ZLIB,$(ZLIB_DIR),z))
$(eval $(call dep_lib,PNG,$(PNG_DIR),png))
$(eval $(call dep_lib,JPEG,$(JPEG_DIR)/src,jpeg))
$(eval $(call dep_lib,FT,$(FT_DIR)/src,freetype))
$(eval $(call dep_lib,FRIBIDI,$(FRIBIDI_DIR)/lib,fribidi))
$(eval $(call dep_lib,OGG,$(OGG_DIR)/src,ogg))
$(eval $(call dep_lib,VORBIS,$(VORBIS_DIR)/lib,vorbis))
$(eval $(call dep_lib,VORBISFILE,$(VORBIS_DIR)/lib,vorbisfile))
$(eval $(call dep_lib,FLAC,$(FLAC_DIR)/src/libFLAC,FLAC))
$(eval $(call dep_lib,MAD,$(MAD_DIR)/src,mad))
$(eval $(call dep_lib,FAAD,$(FAAD_DIR)/libfaad,faad))
$(eval $(call dep_lib,THEORA,$(THEORA_DIR)/lib,theoradec))
$(eval $(call dep_lib,MPEG2,$(MPEG2_DIR)/libmpeg2,mpeg2))
$(eval $(call dep_lib,GIF,$(GIF_DIR),gif))

DEP_LIBS := z png jpeg freetype fribidi ogg vorbis vorbisfile FLAC mad faad theoradec mpeg2 gif
DEP_ARCHIVES := $(foreach l,$(DEP_LIBS),$(DEPS)/lib/lib$(l).a)

# The headers, where ScummVM's sources look for them, and a pkg-config file
# for each library configure asks pkg-config about (its version the
# release's, from deps.lock).
lockver = $(shell awk '$$1 == "$(1)" { print $$2 }' $(ROOT)/waterbox/deps.lock)
$(DEPS)/include.stamp: $(DEPS)/flags
	rm -rf $(DEPS)/include $(DEPS)/lib/pkgconfig
	mkdir -p $(DEPS)/include/freetype2 $(DEPS)/include/fribidi $(DEPS)/include/ogg $(DEPS)/include/vorbis \
		$(DEPS)/include/FLAC $(DEPS)/include/theora $(DEPS)/include/mpeg2dec $(DEPS)/lib/pkgconfig
	cp $(ZLIB_HDRS) $(PNG_HDRS) $(JPEG_HDRS) $(DEPS)/include/
	cp $(PNG_DIR)/scripts/pnglibconf.h.prebuilt $(DEPS)/include/pnglibconf.h
	cp -r $(FT_DIR)/include/ft2build.h $(FT_DIR)/include/freetype $(DEPS)/include/freetype2/
	cp $(FRIBIDI_DIR)/lib/fribidi*.h $(CFG)/fribidi/fribidi-config.h $(DEPS)/include/fribidi/
	cp $(OGG_DIR)/include/ogg/ogg.h $(OGG_DIR)/include/ogg/os_types.h $(CFG)/ogg/ogg/config_types.h $(DEPS)/include/ogg/
	cp $(VORBIS_DIR)/include/vorbis/*.h $(DEPS)/include/vorbis/
	cp $(FLAC_DIR)/include/FLAC/*.h $(DEPS)/include/FLAC/
	cp $(CFG)/mad/mad.h $(CFG)/faad/faad.h $(FAAD_DIR)/include/neaacdec.h $(GIF_DIR)/gif_lib.h $(DEPS)/include/
	cp $(THEORA_DIR)/include/theora/*.h $(DEPS)/include/theora/
	cp $(MPEG2_DIR)/include/mpeg2.h $(DEPS)/include/mpeg2dec/
	for pc in "zlib:z:$(call lockver,zlib)" "libpng:png -lz:$(call lockver,libpng)" \
		"freetype2:freetype -lz:$(call lockver,freetype):/freetype2" "fribidi:fribidi:$(call lockver,fribidi)" \
		"ogg:ogg:$(call lockver,libogg)" "vorbis:vorbis -logg:$(call lockver,libvorbis)" \
		"vorbisfile:vorbisfile -lvorbis -logg:$(call lockver,libvorbis)" "flac:FLAC:$(call lockver,flac)" \
		"mad:mad:$(call lockver,libmad)" "libgif:gif:$(call lockver,giflib)" \
		"theoradec:theoradec -logg:$(call lockver,libtheora)" "libmpeg2:mpeg2:$(call lockver,libmpeg2)"; do \
		name=$${pc%%:*}; rest=$${pc#*:}; libs=$${rest%%:*}; rest=$${rest#*:}; ver=$${rest%%:*}; \
		sub=$$(printf '%s' "$$rest" | sed -n 's/^[^:]*://p'); \
		printf 'prefix=%s\nName: %s\nDescription: %s\nVersion: %s\nLibs: -L%s/lib -l%s\nCflags: -I%s/include%s\n' \
			"$(abspath $(DEPS))" "$$name" "$$name" "$$ver" "$(abspath $(DEPS))" "$$libs" "$(abspath $(DEPS))" "$$sub" \
			> $(DEPS)/lib/pkgconfig/$$name.pc; \
	done
	touch $@

deps: $(DEP_ARCHIVES) $(DEPS)/include.stamp
.PHONY: deps
