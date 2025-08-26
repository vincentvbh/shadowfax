
# Shadowfax

## Software requirements
- `cc`
- `makefile`
- `bash`

## Our benchmark environment
- Apple M1 Pro
- Sonoma 14.6.1
- `gcc (Homebrew GCC 13.3.0) 13.3.0`

## Additional compilation tests
- Apple M1 Pro, Sonoma 14.6.1, `gcc (Homebrew GCC 13.3.0) 13.3.0`
- Apple M1 Pro, Sonoma 14.6.1, `Apple clang version 15.0.0 (clang-1500.3.9.4)`
- Dell Inc. XPS 9320, Ubuntu 22.04.5 LTS, `gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0`
- Dell Inc. XPS 9320, Ubuntu 22.04.5 LTS, `Ubuntu clang version 14.0.0-1ubuntu1.1`
- MacBook Pro 2020 (Intel(R) Core(TM) i7-1068NG7 CPU @ 2.30GHz), `Apple clang version 12.0.0 (clang-1200.0.32.28)`,
`GNU Make 3.81`, `GNU bash, version 5.2.37(1)-release (x86_64-apple-darwin22.6.0)`

## How to compile
Type `make`. Six binary files will be produced.
- `test_dh_akem`: test the correctness of the DH-AKEM.
- `test_pq_akem`: test the correctness of the PQ-AKEM.
- `test_h_akem`: test the correctness of the hybrid AKEM (Shadowfax).
- `speed_dh_akem`: benchmark the DH-AKEM.
- `speed_pq_akem`: benchmark the PQ-AKEM.
- `speed_h_akem`: benchmark the hybrid AKEM (Shadowfax).

## Scripts for benchmarking

Running `bash ./make_bench.sh` will automatically build and benchmark the implementations. This requires root access while benchmarking on macOS.
Thousands of cycles are written to the file `bench.txt` and converted into latex commands in `bench_latex.tex`.

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



