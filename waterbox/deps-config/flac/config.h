/* libFLAC 1.5.0: config.cmake.h.in as its CMake fills it in on x86-64
 * Linux, for the decoder alone: no Ogg (ScummVM reads native FLAC), no
 * intrinsics (FLAC__HAS_X86INTRIN 0, and FLAC__NO_ASM on the command line:
 * the C code on every computer), no threads, no processor questions
 * (HAVE_CPUID_H left out). */
#define CPU_IS_BIG_ENDIAN 0
#define ENABLE_64_BIT_WORDS 1
#define OGG_FOUND 0
#define FLAC__HAS_OGG OGG_FOUND
#define FLAC__HAS_X86INTRIN 0
#define FLAC__HAS_NEONINTRIN 0
#define FLAC__HAS_A64NEONINTRIN 0
#define FLAC__SYS_LINUX
#define HAVE_BSWAP16
#define HAVE_BSWAP32
#define HAVE_BYTESWAP_H
#define HAVE_FSEEKO
#define HAVE_INTTYPES_H
#define HAVE_LROUND 1
#define HAVE_STDINT_H
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_SYS_PARAM_H
#define HAVE_SYS_STAT_H
#define HAVE_SYS_TIME_H
#define HAVE_SYS_TYPES_H
#define HAVE_TYPEOF
#define HAVE_UNISTD_H
#define PACKAGE_VERSION "1.5.0"
#define SIZEOF_OFF_T 8
#define SIZEOF_VOIDP 8
#ifndef _ALL_SOURCE
#define _ALL_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define WORDS_BIGENDIAN CPU_IS_BIG_ENDIAN
