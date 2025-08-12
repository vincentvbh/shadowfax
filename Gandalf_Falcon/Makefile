# Possible options:
#   -DFNDSA_AVX2=0         disable AVX2 support
#   -DFNDSA_SSE2=0         disable SSE2 support
#   -DFNDSA_NEON=0         disable NEON support
#   -DFNDSA_RV64D=0        disable use of floating-point hardware on RISC-V
#
#   -DFNDSA_NEON_SHA3=1    enable NEON optimizations for parallel SHAKE256
#   -DFNDSA_DIV_EMU=1      force integer emulation of divisions (RISC-V only)
#   -DFNDSA_SQRT_EMU=1     force integer emulation of square roots (RISC-V only)
#
#   -DFNDSA_SHAKE256X4=1   use four parallel SHAKE256 as internal PRNG
#
# AVX2 support is compiled on x86 and x86_64 but is gated at runtime
# with a check that AVX2 is supported by the current CPU (and not
# disabled by the operating system); if AVX2 cannot be used, then the
# fallback code (normally with SSE2) is used. Thus, support of AVX2 does
# not prevent the code from running on non-AVX2 machines.
#
# SSE2 intrinsics are used if supported by the target architecture at
# compile-time (no runtime test); this is normally the case for 64-bit
# builds, since SSE2 is part of the 64-bit ABI. On 32-bit builds, this
# depends on the compiler's defaults, which are OS dependent (e.g. MSVC
# on Windows enables SSE2 by default, and so does Clang on 32-bit MacOS,
# but on 32-bit Linux this is not done by default, and you have to add
# the '-msse2' flag to the compiler to get SSE2). You can force SSE2
# usage even if not detected by setting '-DFNDSA_SSE2=1'.
#
# NEON intrisics are used if supported by the target architecture at
# compile-time (no runtime test) _and_ the build is 64-bit. Since NEON is
# part of the 64-bit ARMv8 ABI, you normally don't have to fiddle with
# that.
#
# An optional NEON-optimized SHAKE256 implementation can be enabled (it
# runs two SHAKE256 implementations in parallel). It is disabled by default
# because it turns out to be slower than the plain code on ARM Cortex-A55
# and Cortex-A76 test systems. It _might_ be faster on some other ARM
# systems. To enable it, use '-DFNDSA_NEON_SHA3=1'.
#
# On 64-bit RISC-V systems, the floating-point hardware is used if
# detected at compile-time (i.e. the target architecture includes the 'D'
# extension, which is part of the usual "RV64GC" package). When these
# instructions are used, the divisions and square roots may optionally
# be done with only integer computations, which is slower but possibly
# safer with regard to timing attacks.
#
# An internal PRNG is used during key pair generation (to generate
# candidate (f,g) polynomial pairs) and during signature generation (to
# power the Gaussian sampling). By default, that PRNG is a simple
# SHAKE256. An alternate PRNG is enabled by setting
# '-DFNDSA_SHAKE256X4=1': this new PRNG uses four SHAKE256 in parallel,
# with interleaved outputs. The alternate PRNG speeds up signature
# generation by about 20% when runing on an x86 CPU with AVX2 support;
# however, it also increases stack usage by aout 1.1 kB, which can be a
# problem on small embedded systems such as microcontrollers, which is
# why it is not the default. Moreover, using the alternate PRNG
# necessarily changes the keys and signatures obtained from a given seed
# (note that reproducibility of keys and signatures should not be relied
# upon, at least until the FN-DSA standard is finalized, as things are
# expected to change again in some areas).
#
# By default, this code compiles 'test_fndsa' (a test framework to validate
# that all computations are correct) and 'speed_fndsa' (speed benchmarks).

CC        = gcc
CFLAGS    = -W -Wextra -Wundef -Wshadow -O2
LD        = gcc
LDFLAGS   =

RAND_PATH   = ../randombytes
HASH_PATH   = ../hash

CFLAGS   += -I. -I$(RAND_PATH) -I$(HASH_PATH)

HEADERS   = $(wildcard *.h)
HEADERS  += $(wildcard $(RAND_PATH)/*.h)
HEADERS  += $(wildcard $(HASH_PATH)/*.h)

SOURCES   = $(filter-out $(wildcard test*) $(wildcard speed*) samplerZ_table.c, $(wildcard *.c))
SOURCES  += $(wildcard $(RAND_PATH)/*.c)
SOURCES  += $(wildcard $(HASH_PATH)/*.c)

OBJS      = $(patsubst %.c, %.o, $(SOURCES))
TESTOBJ   = test_fndsa.o test_sampler.o test_sign.o
SPEEDOBJ  = speed_fndsa.o

all: test test_fndsa speed_fndsa

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

test_fndsa: $(OBJS) $(TESTOBJ)
	$(LD) $(LDFLAGS) -o test_fndsa $(OBJS) $(TESTOBJ)

speed_fndsa: $(OBJS) $(SPEEDOBJ)
	$(LD) $(LDFLAGS) -o speed_fndsa $(OBJS) $(SPEEDOBJ)

test: $(OBJS) $(HEADERS) test.c
	$(CC) $(CFLAGS) $(OBJS) test.c -o test

.PHONY: clean

clean:
	-rm -f $(OBJS)
	-rm -f $(TESTOBJ)
	-rm -f $(SPEEDOBJ)
	-rm -f test_fndsa
	-rm -f speed_fndsa
	-rm -f test





