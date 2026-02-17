
# Shadowfax

This artifact accompanies the paper ***SHADOWFAX: Hybrid Security and Deniability for AKEMs*** for hybrid deniable AKEMs.
We provide several portable implementations of the hybrid AKEM SHADOWFAX.
When instantiated with standardised components (ML–KEM and FALCON),
SHADOWFAX yields ciphertexts of 1,728 bytes and public keys of 2,036 bytes,
with encapsulation and decapsulation costs of 1.8M and 0.7M cycles on an Apple M1 Pro.

This artifact provides the portable implementations for
- Pre-quantum AKEM with `curve25519`.
- Post-quantum AKEM built from
    - one of the following post-quantum KEMs,
        - `mlkem-512`
        - `bat-257`
    - and a post-quantum ring signature based on one of the following digital signatures
        - `falcon-512` key generation + `falcon-512` sampler in C
        - `falcon-512` key generation + `falcon-512` sampler with platform-specific inline assembly
        - Antrag key generation + Mitaka sampler
- Hybrid AKEM from
    - the pre-quantum NIKE with `curve25519`,
    - and the set of combinations of post-quantum KEM and ring signature.

Regarding the `falcon-512` inline assembly, the assembly parts are guarded by compile-time architecture tests and are portable. Our paper reports the performance of all above except for `falcon-512` in C.

## Software requirements
- `gcc`
- `make`
- `bash`
- `cc`

To check the versions, run the following
- `gcc --version`
- `make --version`
- `bash --version`
- `cc --version`

### Notes on the dependencies

The performance numbers are highly tied to two things:
- The hardware.
- And the compiler.

The compiler `gcc` is used for compiling, and can be overwritten by something else while running the `Makefile`.
The command `cc` is used only for post-processing along with other scripts.
`make` and `bash` are about the automation of the compilation, testing, and benchmarking.

## Our benchmark environment
- Apple M1 Pro
- Sonoma 14.6.1
- `gcc (Homebrew GCC 13.3.0) 13.3.0`
- `GNU Make 3.81`
- `GNU bash, version 3.2.57(1)-release (arm64-apple-darwin23)`

## Additional compilation tests
- Apple M1 Pro, Sonoma 14.6.1.
    - `Apple clang version 15.0.0 (clang-1500.3.9.4)`.
    - Same `bash` and `make` as the benchmark environment.
- Dell Inc. XPS 9320, Ubuntu 22.04.5 LTS.
    - `gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0`.
    - `GNU Make 4.3`.
    - `GNU bash, version 5.1.16(1)-release (x86_64-pc-linux-gnu`.
- Dell Inc. XPS 9320, Ubuntu 22.04.5 LTS.
    - `Ubuntu clang version 14.0.0-1ubuntu1.1`.
    - Same `bash` and `make` as above.
- MacBook Pro 2020 (Intel(R) Core(TM) i7-1068NG7 CPU @ 2.30GHz).
    - `Apple clang version 12.0.0 (clang-1200.0.32.28)`.
    - `GNU Make 3.81`.
    - `GNU bash, version 5.2.37(1)-release (x86_64-apple-darwin22.6.0)`.

## Structure of this artifact

- Pre-quantum AKEM
    - `dh`
    - `akem/dh_akem.c`
    - `akem/dh_akem_api.h`
    - `test/test_dh_akem.c`
    - `speed/speed_dh_akem.c`
- Post-quantum AKEM
    - A post-quantum KEM with the api file `kem_api.h`. One of the following is sufficient.
        - `BAT`
        - `mlkem`
    - A post-quantum ring signature with the api file `rsig_api.h`. One of the following is sufficient.
        - `GandalfFalcon`
        - `GandalfFalconC`
        - `GandalfMitaka`
    - `akem/pq_akem.c`
    - `akem/pq_akem_api.h`
    - `test/test_pq_akem.c`
    - `speed/speed_pq_akem.c`
- Hybrid AKEM
    - Pre-quantum AKEM dependencies (excluding `test_*` and `speed_*`)
    - Post-quantum AKEM dependencies (excluding `test_*` and `speed_*`)
    - `akem/h_akem.c`
    - `akem/h_akem_api.h`
    - `test/test_h_akem.c`
    - `speed/speed_h_akem.c`
- Shared
    - `cycles`: Access to cycle counters on aarch64 (reported in the paper) and x86-64.
    - `hash`: Cryptographic hash functions. FIPS202, BLAKE2, HMAC.
    - `ntru_gen`: NTRU solver used in BAT, Falcon, and Mitaka.
    - `randombytes`: System randombytes and pseudo-random bytes.
    - `symmetric`: AES.

## Scripts for compiling and testing correctness

Run
```
bash ./test_everything.sh
```

`test_everything.sh` will automatically build and benchmark the implementations.
The log will be written to `./log/test_log.txt`.

## Scripts for compiling and benchmarking

Run
```
bash ./bench_everything.sh
```

`bench_everything.sh` will automatically build and benchmark the implementations. Benchmarking requires root access while benchmarking on macOS.
The log will be written to `./log/bench_log.txt`,
and the numbers will be converted into LaTex commands in `./log/bench_latex.tex`.

## How to compile, test, and benchmark without `bash` scripts

See `build_run_doc.md`.

## Additional notes on benchmarking on other platforms

We can also produce the performance numbers on other platforms with the same software (see `./cycles/cycles.[ch]`). The following platforms should also work.
- Raspberry pi with a 64-bit Linux OS. This requires Raspberry pi 3/4/5.
- x86 with a Linux OS.

However, additional steps, such as inserting kernel modules and turning off hyperthreading and Turbo boost, are required to benchmark the performance properly. Performance on these platforms are not part of the artifact.

## Additional notes on the scripts

- `clean_up.sh`: Clean up all the generated files.
- `package_artifact.sh`: Create a standalone `.zip` file with all the generated files and `git`-associated files removed.
- `bench_everything.sh`: Building and benchmarking the implementations. `cc` compiles a post-processing program converting the raw data into LaTeX commands.





