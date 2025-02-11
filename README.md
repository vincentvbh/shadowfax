
# Shadowfax

## Requirements for compilation
- A C compiler

## Our benchmark environment
- Apple M1 Pro
- Sonoma 14.6.1
- `gcc (Homebrew GCC 13.3.0) 13.3.0`

## Additional compilation tests
- Apple M1 Pro, Sonoma 14.6.1,
    - `gcc (Homebrew GCC 13.3.0) 13.3.0`
    - `Apple clang version 15.0.0 (clang-1500.3.9.4)`
- 11th Gen Intel(R) Core(TM) i7-11700K @ 3.60GHz, Debian GNU/Linux 12 (bookworm)
    - `gcc (Debian 12.2.0-14) 12.2.0`
- Dell Inc. XPS 9320, Ubuntu 22.04.5 LTS,
    - `gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0`
    - `Ubuntu clang version 14.0.0-1ubuntu1.1`

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
Type `./speed_dh_akem`, execute with `sudo` on macOS. Sample output:
```
nike_akem_encap cycles: 629925
nike_akem_decap cycles: 420291
nike_keygen cycles: 208841
nike_sdk cycles: 210194
```

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
Type `./speed_pq_akem`, execute with `sudo` on macOS. Sample output:
```
pq_akem_keygen_expanded_sk cycles: 24523991
pq_akem_keygen cycles: 24702269
pq_akem_encap_expanded_sk cycles: 1215694
pq_akem_encap cycles: 1248185
pq_akem_decap cycles: 331514
kem_keygen cycles: 11705212
kem_encap cycles: 32584
kem_decap cycles: 216229
sign_keygen_expanded_sk cycles: 13142877
sign_keygen cycles: 13274804
Gandalf_sign_expanded_sk cycles: 1108763
Gandalf_sign cycles: 1137925
Gandalf_verify cycles: 94245
sampler cycles: 609349
Gandalf_sample_poly cycles: 412186
```

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
Type `./speed_h_akem`, execute with `sudo` on macOS. Sample output:
```
h_akem_keygen_expanded_sk cycles: 25086634
h_akem_keygen cycles: 24751344
h_akem_encap_expanded_sk cycles: 1845035
h_akem_encap cycles: 1875279
h_akem_decap cycles: 745420
nike_keygen cycles: 207860
nike_sdk cycles: 210034
kem_keygen cycles: 11338464
kem_encap cycles: 32350
kem_decap cycles: 216713
sign_keygen_expanded_sk cycles: 13137087
sign_keygen cycles: 13017963
Gandalf_sign_expanded_sk cycles: 1092342
Gandalf_sign cycles: 1121973
Gandalf_verify cycles: 78888
```




