
# Shadowfax

## Software requirements
- `gcc`
- `make`
- `bash`

To check the versions, run the following
- `gcc --version`
- `make --version`
- `bash --version`

### Notes on the software dependencies

The performance numbers are highly tied to two things:
- The hardware.
- And the compiler.

Other software dependencies are about the automation of the benchmarking.

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

## How to compile
Type `make`. Six binary files will be produced.
- `test_dh_akem`: test the correctness of the DH-AKEM.
- `test_pq_akem`: test the correctness of the PQ-AKEM.
- `test_h_akem`: test the correctness of the hybrid AKEM (Shadowfax).
- `speed_dh_akem`: benchmark the DH-AKEM.
- `speed_pq_akem`: benchmark the PQ-AKEM.
- `speed_h_akem`: benchmark the hybrid AKEM (Shadowfax).

### Options for the underlying KEM and ring signature

- `KEM_PATH` specifies the path to the KEM.
- `RSIG_PATH` specifies the path to the ring signature.

Examples:
- `make BAT GandalfMitaka`
- `make BAT GandalfFalcon`
- `make BAT GandalfFalconC`
- `make mlkem GandalfMitaka`
- `make mlkem GandalfFalcon`
- `make mlkem GandalfFalconC`

## Scripts for benchmarking

Running `bash ./make_bench.sh` will automatically build and benchmark the implementations. This requires root access while benchmarking on macOS.
Thousands of cycles are written to the file `bench.txt` and converted into latex commands in `bench_latex.tex`.

We can also produce the performance numbers on other platforms with the same software (see `./cycles/cycles.[ch]`). The following platforms should also work.
- Raspberry pi with a 64-bit Linux OS. This requires Raspberry pi 3/4/5.
- x86 with a Linux OS.

However, additional steps, such as inserting kernel modules and turning off hyperthreading and Turbo boost, are required to benchmark the performance properly. Performance on these platforms are not part of the artifact.

## Example usage without scripts

### DH-AKEM

#### Test for correctness
Type `./test_dh_akem`. Sample output:
```
2048/2048 compatible shared secret pairs. (ok).

0/4096 success decapsulation + compatible shared secret pairs. (ok).

0/2048 compatible shared secret pairs. (ok).
```

#### Benchmark
Type `./speed_dh_akem` or `sudo ./speed_dh_akem` on macOS.

### PQ-AKEM

#### Test for correctness
Type `./test_pq_akem`. Sample output:
```
2048/2048 compatible shared secret pairs. (ok).

2048/2048 compatible shared secret pairs. (ok).

0/4096 success decapsulation + compatible shared secret pairs. (ok).

0/2048 compatible shared secret pairs. (ok).
```

#### Benchmark
Type `./speed_pq_akem` or `sudo ./speed_pq_akem` on macOS

### Hybrid AKEM (Shadowfax)

#### Test for correctness
Type `./test_h_akem`. Sample output:
```
2048/2048 compatible shared secret pairs. (ok).

2048/2048 compatible shared secret pairs. (ok).

0/4096 success decapsulation + compatible shared secret pairs. (ok).

0/2048 compatible shared secret pairs. (ok).
```

#### Benchmark
Type `./speed_h_akem` or `sudo ./speed_h_akem` on macOS.



