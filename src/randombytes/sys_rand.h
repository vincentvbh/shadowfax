#ifndef SYS_RAND_H
#define SYS_RAND_H

#define RAND_URANDOM 1

#include <stddef.h>

/*
 * For seed generation:
 *
 *  - On Linux (glibc-2.25+), FreeBSD 12+ and OpenBSD, use getentropy().
 *  - On other Unix-like systems, use /dev/urandom (also a fallback for
 *    failed getentropy() calls).
 *  - On Windows, use CryptGenRandom().
 */

#ifndef BAT_RAND_GETENTROPY
#if (defined __linux && defined __GLIBC__ \
    && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))) \
    || (defined __FreeBSD__ && __FreeBSD__ >= 12) \
    || defined __OpenBSD__
#define RAND_GETENTROPY   1
#else
#define RAND_GETENTROPY   0
#endif
#endif

#ifndef BAT_RAND_URANDOM
#if defined _AIX \
    || defined __ANDROID__ \
    || defined __FreeBSD__ \
    || defined __NetBSD__ \
    || defined __OpenBSD__ \
    || defined __DragonFly__ \
    || defined __linux__ \
    || (defined __sun && (defined __SVR4 || defined __svr4__)) \
    || (defined __APPLE__ && defined __MACH__)
#define RAND_URANDOM   1
#else
#define RAND_URANDOM   0
#endif
#endif

#ifndef BAT_RAND_WIN32
#if defined _WIN32 || defined _WIN64
#define RAND_WIN32   1
#else
#define RAND_WIN32   0
#endif
#endif



int get_seed(void *seed, size_t len);

#endif

