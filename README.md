
# Shadowfax

## Requirements for compilation
- A C compiler

## Our benchmark environment
- Apple M1 Pro
- Sonoma 14.6.1
- `gcc (Homebrew GCC 13.2.0) 13.2.0`

## Additional compilation tests
- Apple M1 Pro, Sonoma 14.6.1, `gcc (Homebrew GCC 13.3.0) 13.3.0`
- Apple M1 Pro, Sonoma 14.6.1, `Apple clang version 15.0.0 (clang-1500.3.9.4)`
- Dell Inc. XPS 9320, Ubuntu 22.04.5 LTS, `gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0`
- Dell Inc. XPS 9320, Ubuntu 22.04.5 LTS, `Ubuntu clang version 14.0.0-1ubuntu1.1`

## How to compile
Type `make`. Six binary files will be produced.
- `test_dh_akem`: test the correctness of the DH-AKEM.
- `test_pq_akem`: test the correctness of the PQ-AKEM.
- `test_h_akem`: test the correctness of the hybrid AKEM (Shadowfax).
- `speed_dh_akem`: benchmark the DH-AKEM.
- `speed_pq_akem`: benchmark the PQ-AKEM.
- `speed_h_akem`: benchmark the hybrid AKEM (Shadowfax).

## DH-AKEM

### Test for correctness
Type `./test_dh_akem`. Sample output:
```
2048/2048 compatible shared secret pairs. (ok).

0/4096 success decapsulation + compatible shared secret pairs. (ok).

0/2048 compatible shared secret pairs. (ok).
```

### Benchmark
Type `./speed_dh_akem`.

Benchmarks can be found in the following files:
- `bench_pq_akem_BAT_Gandalf_Falcon.txt`
- `bench_pq_akem_BAT_Gandalf_Mitaka.txt`
- `bench_pq_akem_mlkem_Gandalf_Falcon.txt`
- `bench_pq_akem_mlkem_Gandalf_Mitaka.txt`

## PQ-AKEM

### Test for correctness
Type `./test_pq_akem`. Sample output:
```
2048/2048 compatible shared secret pairs. (ok).

2048/2048 compatible shared secret pairs. (ok).

0/4096 success decapsulation + compatible shared secret pairs. (ok).

0/2048 compatible shared secret pairs. (ok).
```

### Benchmark
Type `./speed_pq_akem`.


## Hybrid AKEM

### Test for correctness
Type `./test_h_akem`. Sample output:
```
2048/2048 compatible shared secret pairs. (ok).

2048/2048 compatible shared secret pairs. (ok).

0/4096 success decapsulation + compatible shared secret pairs. (ok).

0/2048 compatible shared secret pairs. (ok).
```

### Benchmark
Type `./speed_h_akem`.

Benchmarks can be found in the following files:
- `bench_h_akem_BAT_Gandalf_Falcon.txt`
- `bench_h_akem_BAT_Gandalf_Mitaka.txt`
- `bench_h_akem_mlkem_Gandalf_Falcon.txt`
- `bench_h_akem_mlkem_Gandalf_Mitaka.txt`

