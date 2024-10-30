#ifndef CONFIG_H
#define CONFIG_H

#if defined BAT_AVX2 && BAT_AVX2
/*
 * This implementation uses AVX2 intrinsics.
 */
#include <immintrin.h>
#ifndef BAT_LE
#define BAT_LE   1
#endif
#ifndef BAT_UNALIGNED
#define BAT_UNALIGNED   1
#endif
#if defined __GNUC__
#define TARGET_AVX2    __attribute__((target("avx2")))
#define ALIGNED_AVX2   __attribute__((aligned(32)))
#elif defined _MSC_VER && _MSC_VER
#pragma warning( disable : 4752 )
#endif
#endif

#ifndef TARGET_AVX2
#define TARGET_AVX2
#endif
#ifndef ALIGNED_AVX2
#define ALIGNED_AVX2
#endif

/*
 * Disable warning on applying unary minus on an unsigned type.
 */
#if defined _MSC_VER && _MSC_VER
#pragma warning( disable : 4146 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4334 )
#endif

/*
 * Auto-detect 64-bit architectures.
 */
#ifndef BAT_64
#if defined __x86_64__ || defined _M_X64 \
    || defined __ia64 || defined __itanium__ || defined _M_IA64 \
    || defined __powerpc64__ || defined __ppc64__ || defined __PPC64__ \
    || defined __64BIT__ || defined _LP64 || defined __LP64__ \
    || defined __sparc64__ \
    || defined __aarch64__ || defined _M_ARM64 \
    || defined __mips64
#define BAT_64   1
#else
#define BAT_64   0
#endif
#endif

/*
 * Auto-detect endianness and support of unaligned accesses.
 */
#if defined __i386__ || defined _M_IX86 \
    || defined __x86_64__ || defined _M_X64 \
    || (defined _ARCH_PWR8 \
        && (defined __LITTLE_ENDIAN || defined __LITTLE_ENDIAN__))

#ifndef BAT_LE
#define BAT_LE   1
#endif
#ifndef BAT_UNALIGNED
#define BAT_UNALIGNED   1
#endif

#elif (defined __LITTLE_ENDIAN && __LITTLE_ENDIAN__) \
    || (defined __BYTE_ORDER__ && defined __ORDER_LITTLE_ENDIAN__ \
        && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#ifndef BAT_LE
#define BAT_LE   1
#endif
#ifndef BAT_UNALIGNED
#define BAT_UNALIGNED   0
#endif

#else

#ifndef BAT_LE
#define BAT_LE   0
#endif
#ifndef BAT_UNALIGNED
#define BAT_UNALIGNED   0
#endif

#endif

/*
 * Ensure all macros are defined, to avoid warnings with -Wundef.
 */
#ifndef BAT_AVX2
#define BAT_AVX2   0
#endif

/*
 * MSVC 2015 does not known the C99 keyword 'restrict'.
 */
#if defined _MSC_VER && _MSC_VER
#ifndef restrict
#define restrict   __restrict
#endif
#endif

#if __GNUC__ || __clang__
#define UNUSED   __attribute__ ((unused))
#else
#define UNUSED
#endif

#endif

