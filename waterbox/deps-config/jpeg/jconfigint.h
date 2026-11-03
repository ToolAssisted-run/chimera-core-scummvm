/* libjpeg-turbo 3.2.1: src/jconfigint.h.in as its CMake fills it in for
 * GCC on x86-64 Linux, no SIMD. BUILD is fixed (CMake writes the day it
 * ran), and there is no thread-local storage (only the TurboJPEG API, which
 * is not built, uses it). */
#define BUILD  "chimera"
#define HIDDEN  __attribute__((visibility("hidden")))
#undef inline
#define INLINE  __inline__ __attribute__((always_inline))
#define THREAD_LOCAL
#define PACKAGE_NAME  "libjpeg-turbo"
#define VERSION  "3.2.1"
#define SIZEOF_SIZE_T  8
#define HAVE_BUILTIN_CTZL
#if defined(__has_attribute)
#if __has_attribute(fallthrough)
#define FALLTHROUGH  __attribute__((fallthrough));
#else
#define FALLTHROUGH
#endif
#else
#define FALLTHROUGH
#endif
#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif
#undef C_ARITH_CODING_SUPPORTED
#undef D_ARITH_CODING_SUPPORTED
#undef WITH_SIMD
#if BITS_IN_JSAMPLE == 8
#define C_ARITH_CODING_SUPPORTED 1
#define D_ARITH_CODING_SUPPORTED 1
#define SIMD_ARCHITECTURE  NONE
#endif
