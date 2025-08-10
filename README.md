
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
Type `./speed_dh_akem`. Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
nike_akem_encap average cycles:
679291
nike_akem_decap average cycles:
456717
nike_keygen average cycles:
226693
nike_sdk average cycles:
226852
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
Type `./speed_pq_akem`. Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen_expanded_sk average cycles:
25420489
pq_akem_keygen average cycles:
25632320
pq_akem_encap_expanded_sk average cycles:
1256157
pq_akem_encap average cycles:
1286545
pq_akem_decap average cycles:
349481
kem_keygen average cycles:
12112300
kem_encap average cycles:
56878
kem_decap average cycles:
230360
sign_keygen_expanded_sk average cycles:
13422897
sign_keygen average cycles:
13318055
Gandalf_sign_expanded_sk average cycles:
1113264
Gandalf_sign average cycles:
1139382
Gandalf_verify average cycles:
100216
sampler average cycles:
608754
Gandalf_sample_poly average cycles:
413278
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
Type `./speed_h_akem`. Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen_expanded_sk average cycles:
25655423
h_akem_keygen average cycles:
25608536
h_akem_encap_expanded_sk average cycles:
1935618
h_akem_encap average cycles:
1961060
h_akem_decap average cycles:
795536
nike_keygen average cycles:
225866
nike_sdk average cycles:
226833
kem_keygen average cycles:
12013088
kem_encap average cycles:
57046
kem_decap average cycles:
230453
sign_keygen_expanded_sk average cycles:
13334238
sign_keygen average cycles:
13338869
Gandalf_sign_expanded_sk average cycles:
1103008
Gandalf_sign average cycles:
1135976
Gandalf_verify average cycles:
84583
```




