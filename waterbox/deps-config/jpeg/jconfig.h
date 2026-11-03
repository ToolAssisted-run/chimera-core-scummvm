/* libjpeg-turbo 3.2.1: src/jconfig.h.in as its CMake fills it in with
 * WITH_JPEG8=1, arithmetic coding on, no SIMD. */
#define JPEG_LIB_VERSION  80
#define LIBJPEG_TURBO_VERSION  3.2.1
#define LIBJPEG_TURBO_VERSION_NUMBER  3002001
#define C_ARITH_CODING_SUPPORTED 1
#define D_ARITH_CODING_SUPPORTED 1
#define MEM_SRCDST_SUPPORTED  1
/* WITH_SIMD: not defined, the C code on every computer */
#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif
/* RIGHT_SHIFT_IS_UNSIGNED: not defined (GCC shifts signed values arithmetically) */
