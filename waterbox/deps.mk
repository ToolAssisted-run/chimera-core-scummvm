# deps.mk - the libraries ScummVM's engines and codecs link, built from the
# submodule extern/libretro-deps (the copies ScummVM's own libretro port
# builds; its backends/platform/libretro/dependencies.mk gave the file
# lists), once per build (native.mk, guest.mk) with that build's compiler and
# flags. Nothing of the computer's own libraries is used: the two builds must
# be the same program.
#
# Each library becomes $(DEPS)/lib/lib<name>.a with its headers under
# $(DEPS)/include and a pkg-config file under $(DEPS)/lib/pkgconfig, so that
# ScummVM's own configure finds exactly these and nothing else
# (scummvm.mk points PKG_CONFIG_LIBDIR there and nowhere else).
#
# Included by native.mk and guest.mk, which set B, CC and DEPS_CFLAGS.

LRD  := $(ROOT)/extern/libretro-deps
DEPS := $(B)/deps

# what every library is compiled with, on top of the build's own flags (the
# libraries that carry a config.h are given HAVE_CONFIG_H below)
DEPS_COMMON := -O2 -w -fno-strict-aliasing -I$(ROOT)/waterbox/compat

$(call flags_stamp,$(DEPS),$(CC) | $(DEPS_CFLAGS) | $(DEPS_COMMON))

# ---- zlib 1.2.8
ZLIB_NAMES := adler32 compress deflate gzclose gzlib gzread gzwrite inflate inftrees inffast trees uncompr zutil libz-crc32
ZLIB_FLAGS := -I$(LRD)/libz -DHAVE_UNISTD_H -DHAVE_STDARG_H
ZLIB_HDRS := $(LRD)/libz/zlib.h $(LRD)/libz/zconf.h

# ---- libpng 1.6.25beta03, its prebuilt pnglibconf.h; no ARM or Intel code
PNG_NAMES := png pngerror pngget pngmem pngpread pngread pngrio pngrtran pngrutil pngset pngtrans pngwio pngwrite pngwtran pngwutil
PNG_FLAGS := -I$(LRD)/libpng -I$(LRD)/libz -DPNG_ARM_NEON_OPT=0 -DPNG_INTEL_SSE_OPT=0
PNG_HDRS := $(LRD)/libpng/png.h $(LRD)/libpng/pngconf.h $(LRD)/libpng/pnglibconf.h

# ---- libjpeg (libjpeg-turbo's C, API 8), jsimd_none: no SIMD
JPEG_NAMES := jaricom jcapimin jcapistd jcarith jccoefct jccolor jcdctmgr jchuff jcinit jcmainct jcmarker jcmaster \
	jcomapi jcparam jcphuff jcprepct jcsample jdapimin jdapistd jdarith jdatadst jdatasrc jdcoefct jdcolor jddctmgr \
	jdhuff jdinput jdmainct jdmarker jdmaster jdmerge jdphuff jdpostct jdsample jdtrans jerror jfdctflt jfdctfst \
	jfdctint jidctflt jidctfst jidctint jidctred jmemmgr jmemnobs jquant1 jquant2 jutils jsimd_none
JPEG_FLAGS := -I$(LRD)/libjpeg -DSIZEOF_SIZE_T=8
JPEG_HDRS := $(LRD)/libjpeg/jpeglib.h $(LRD)/libjpeg/jconfig.h $(LRD)/libjpeg/jmorecfg.h $(LRD)/libjpeg/jerror.h

# ---- FreeType 2, the modules its ftmodule.h lists
FT_NAMES := autofit/afangles autofit/afblue autofit/afcjk autofit/afdummy autofit/afglobal autofit/afhints \
	autofit/afindic autofit/aflatin autofit/afloader autofit/afmodule autofit/afpic autofit/afranges autofit/afshaper \
	autofit/afwarp base/basepic base/ftadvanc base/ftapi base/ftbitmap base/ftcalc base/ftgloadr base/ftglyph \
	base/fthash base/ftinit base/ftsnames base/ftobjs base/ftsystem base/ftoutln base/ftrfork base/ftstream \
	base/ftstroke base/fttrigon base/ftutil bdf/bdfdrivr bdf/bdflib cid/cidriver cid/cidgload cid/cidload \
	cid/cidobjs cid/cidparse cff/cf2arrst cff/cf2blues cff/cf2error cff/cf2ft cff/cf2font cff/cf2hints cff/cf2intrp \
	cff/cf2read cff/cf2stack cff/cffcmap cff/cffgload cff/cffload cff/cffobjs cff/cffparse cff/cffpic cff/cffdrivr \
	gzip/ftgzip lzw/ftlzw pcf/pcfdrivr pcf/pcfread pcf/pcfutil pfr/pfrcmap pfr/pfrdrivr pfr/pfrgload pfr/pfrload \
	pfr/pfrobjs pfr/pfrsbit psaux/afmparse psaux/psconv psaux/psobjs psaux/t1cmap psaux/t1decode psaux/psauxmod \
	pshinter/pshalgo pshinter/pshglob pshinter/pshmod pshinter/pshpic pshinter/pshrec psnames/psmodule \
	raster/ftrend1 raster/ftraster raster/rastpic sfnt/sfdriver sfnt/sfntpic sfnt/sfobjs sfnt/ttbdf sfnt/ttcmap \
	sfnt/ttkern sfnt/ttload sfnt/ttmtx sfnt/ttpost sfnt/ttsbit smooth/ftgrays smooth/ftsmooth type1/t1afm \
	type1/t1driver type1/t1gload type1/t1load type1/t1objs type1/t1parse type42/t42drivr type42/t42objs \
	type42/t42parse truetype/ttdriver truetype/ttgload truetype/ttgxvar truetype/ttinterp truetype/ttobjs \
	truetype/ttpload winfonts/winfnt
FT_FLAGS := -I$(LRD)/freetype/include -I$(LRD)/libz -DFT2_BUILD_LIBRARY

# ---- FriBidi 1.0.13
FRIBIDI_NAMES := fribidi-arabic fribidi-bidi-types fribidi-bidi fribidi-brackets fribidi-char-sets-cap-rtl \
	fribidi-char-sets-cp1255 fribidi-char-sets-cp1256 fribidi-char-sets-iso8859-6 fribidi-char-sets-iso8859-8 \
	fribidi-char-sets-utf8 fribidi-char-sets fribidi-deprecated fribidi-joining-types fribidi-joining \
	fribidi-mirroring fribidi-run fribidi-shape fribidi
FRIBIDI_FLAGS := -I$(LRD) -I$(LRD)/fribidi -DHAVE_CONFIG_H

# ---- libogg and libvorbis (decoding only: ScummVM never encodes)
OGG_NAMES := bitwise framing
OGG_FLAGS := -I$(LRD)/libogg/include
VORBIS_NAMES := analysis bitrate block codebook envelope floor0 floor1 info lookup lpc lsp mapping0 mdct psy \
	registry res0 sharedbook smallft synthesis window
VORBIS_FLAGS := -I$(LRD)/libvorbis/include -I$(LRD)/libvorbis/lib -I$(LRD)/libogg/include
VORBISFILE_NAMES := vorbisfile
VORBISFILE_FLAGS := $(VORBIS_FLAGS)

# ---- libFLAC (the decoder; no x86 intrinsics: FLAC__NO_ASM)
FLAC_NAMES := bitmath bitreader cpu crc fixed float format lpc md5 memory metadata_object stream_decoder window
FLAC_FLAGS := -DHAVE_CONFIG_H -I$(LRD)/libFLAC -I$(LRD)/libFLAC/include -DFLAC__NO_ASM -DFLAC__NO_DLL -DFLAC__HAS_OGG=0 \
	-DFLAC__NO_SSE -DFLAC__CPU_UNKNOWN

# ---- libmad 0.15.1b, fixed point (its mad.h chooses FPM_INTEL: integer
# multiplies in inline assembly, the same answer everywhere)
MAD_NAMES := bit decoder fixed frame huffman layer12 layer3 stream synth timer
MAD_FLAGS := -I$(LRD)/libmad -DFPM_INTEL -DSIZEOF_INT=4 -DSIZEOF_LONG=8 -DSIZEOF_LONG_LONG=8

# ---- FAAD2 2.7
FAAD_NAMES := bits cfft common decoder drc error filtbank hcr huffman ic_predict is lt_predict mdct mp4 ms output \
	pns pulse ps_dec ps_syntax rvlc sbr_dct sbr_dec sbr_e_nf sbr_fbt sbr_hfadj sbr_hfgen sbr_huff sbr_qmf \
	sbr_syntax sbr_tf_grid specrec syntax tns
FAAD_FLAGS := -DHAVE_CONFIG_H -I$(LRD)/libfaad -I$(LRD)/libfaad/include -I$(LRD)/libfaad/libfaad

# ---- libtheora's decoder (plain C)
THEORA_NAMES := bitpack decinfo decode dequant fragment huffdec idct info internal quant state
THEORA_FLAGS := -I$(LRD)/theora/include -I$(LRD)/libogg/include

# ---- libmpeg2 0.5.1. Its config.h turns on the MMX code on x86, which is
# linked but never chosen: without ACCEL_DETECT it asks the processor
# nothing and runs the C code on every computer.
MPEG2_NAMES := alloc header decode slice motion_comp idct cpu_accel cpu_state motion_comp_mmx idct_mmx
MPEG2_FLAGS := -DHAVE_CONFIG_H -I$(LRD)/libmpeg2/include -I$(LRD)/libmpeg2/include/mpeg2dec -I$(LRD)/libmpeg2/libmpeg2

# ---- giflib 5
GIF_NAMES := dgif_lib egif_lib gifalloc gif_err gif_hash quantize gif_font openbsd-reallocarray qprintf
GIF_FLAGS := -I$(LRD)/giflib

define dep_lib
$(1)_OBJS := $$(addprefix $$(DEPS)/obj/$(1)/,$$(addsuffix .o,$$($(1)_NAMES)))
$$(DEPS)/obj/$(1)/%.o: $(2)/%.c $$(DEPS)/flags
	@mkdir -p $$(dir $$@)
	$$(CC) $$(DEPS_CFLAGS) $$(DEPS_COMMON) $$($(1)_FLAGS) -c -o $$@ $$<
$$(DEPS)/lib/lib$(3).a: $$($(1)_OBJS)
	@mkdir -p $$(dir $$@)
	rm -f $$@
	ar rcs $$@ $$^
endef

$(eval $(call dep_lib,ZLIB,$(LRD)/libz,z))
$(eval $(call dep_lib,PNG,$(LRD)/libpng,png))
$(eval $(call dep_lib,JPEG,$(LRD)/libjpeg,jpeg))
$(eval $(call dep_lib,FT,$(LRD)/freetype/src,freetype))
$(eval $(call dep_lib,FRIBIDI,$(LRD)/fribidi,fribidi))
$(eval $(call dep_lib,OGG,$(LRD)/libogg/src,ogg))
$(eval $(call dep_lib,VORBIS,$(LRD)/libvorbis/lib,vorbis))
$(eval $(call dep_lib,VORBISFILE,$(LRD)/libvorbis/lib,vorbisfile))
$(eval $(call dep_lib,FLAC,$(LRD)/libFLAC,FLAC))
$(eval $(call dep_lib,MAD,$(LRD)/libmad,mad))
$(eval $(call dep_lib,FAAD,$(LRD)/libfaad/libfaad,faad))
$(eval $(call dep_lib,THEORA,$(LRD)/theora/lib,theoradec))
$(eval $(call dep_lib,MPEG2,$(LRD)/libmpeg2/libmpeg2,mpeg2))
$(eval $(call dep_lib,GIF,$(LRD)/giflib,gif))

DEP_LIBS := z png jpeg freetype fribidi ogg vorbis vorbisfile FLAC mad faad theoradec mpeg2 gif
DEP_ARCHIVES := $(foreach l,$(DEP_LIBS),$(DEPS)/lib/lib$(l).a)

# The headers, where ScummVM's sources look for them, and a pkg-config file
# for each library configure asks pkg-config about.
$(DEPS)/include.stamp: $(DEPS)/flags
	rm -rf $(DEPS)/include $(DEPS)/lib/pkgconfig
	mkdir -p $(DEPS)/include/freetype2 $(DEPS)/include/fribidi $(DEPS)/include/ogg $(DEPS)/include/vorbis \
		$(DEPS)/include/FLAC $(DEPS)/include/theora $(DEPS)/include/mpeg2dec $(DEPS)/lib/pkgconfig
	cp $(ZLIB_HDRS) $(PNG_HDRS) $(JPEG_HDRS) $(DEPS)/include/
	cp -r $(LRD)/freetype/include/ft2build.h $(LRD)/freetype/include/freetype $(DEPS)/include/freetype2/
	cp $(LRD)/fribidi/*.h $(DEPS)/include/fribidi/
	cp $(LRD)/libogg/include/ogg/*.h $(DEPS)/include/ogg/
	cp $(LRD)/libvorbis/include/vorbis/*.h $(DEPS)/include/vorbis/
	cp $(LRD)/libFLAC/include/FLAC/*.h $(DEPS)/include/FLAC/
	cp $(LRD)/libmad/mad.h $(LRD)/libfaad/include/faad.h $(LRD)/libfaad/include/neaacdec.h $(LRD)/giflib/gif_lib.h $(DEPS)/include/
	cp $(LRD)/theora/include/theora/*.h $(DEPS)/include/theora/
	cp $(LRD)/libmpeg2/include/mpeg2dec/mpeg2.h $(DEPS)/include/mpeg2dec/
	for pc in "zlib:z" "libpng:png -lz" "freetype2:freetype -lz:/freetype2" "fribidi:fribidi" "ogg:ogg" \
		"vorbis:vorbis -logg" "vorbisfile:vorbisfile -lvorbis -logg" "flac:FLAC" "mad:mad" "libgif:gif" \
		"theoradec:theoradec -logg" "libmpeg2:mpeg2"; do \
		name=$${pc%%:*}; rest=$${pc#*:}; libs=$${rest%%:*}; sub=$$(printf '%s' "$$rest" | sed -n 's/^[^:]*://p'); \
		printf 'prefix=%s\nName: %s\nDescription: %s\nVersion: 99\nLibs: -L%s/lib -l%s\nCflags: -I%s/include%s\n' \
			"$(abspath $(DEPS))" "$$name" "$$name" "$(abspath $(DEPS))" "$$libs" "$(abspath $(DEPS))" "$$sub" \
			> $(DEPS)/lib/pkgconfig/$$name.pc; \
	done
	touch $@

deps: $(DEP_ARCHIVES) $(DEPS)/include.stamp
.PHONY: deps
