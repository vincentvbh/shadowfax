
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
NIKE AKEM public key bytes:   32
NIKE AKEM secret key bytes:   32
NIKE AKEM ciphertext bytes:   32
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
nike_akem_encap average cycles:
681381
nike_akem_decap average cycles:
457801
nike_keygen average cycles:
225816
nike_sdk average cycles:
227496
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
Type `./speed_pq_akem`.

#### `bat-257` + `mitaka`

Sample output:
```
Post-quantum AKEM public key bytes: 1417
Post-quantum AKEM secret key bytes: 5001
Post-quantum AKEM ciphertext bytes: 1749
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen average cycles:
25120603
pq_akem_encap average cycles:
1290788
pq_akem_decap average cycles:
351922
kem_keygen average cycles:
12151852
kem_encap average cycles:
57535
kem_decap average cycles:
231445
sign_keygen average cycles:
13448897
Gandalf_sign average cycles:
1167662
Gandalf_verify average cycles:
101712
```

#### `bat-257` + `falcon-512`

Sample output:
```
Post-quantum AKEM public key bytes: 1417
Post-quantum AKEM secret key bytes: 5001
Post-quantum AKEM ciphertext bytes: 1749
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen average cycles:
24276542
pq_akem_encap average cycles:
1081467
pq_akem_decap average cycles:
351621
kem_keygen average cycles:
11763624
kem_encap average cycles:
56611
kem_decap average cycles:
229732
sign_keygen average cycles:
12177889
Gandalf_sign average cycles:
925096
Gandalf_verify average cycles:
100266
```

#### `mlkem-512` + `mitaka`

Sample output:
```
Post-quantum AKEM public key bytes: 1696
Post-quantum AKEM secret key bytes: 3680
Post-quantum AKEM ciphertext bytes: 2044
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen average cycles:
13544901
pq_akem_encap average cycles:
1346046
pq_akem_decap average cycles:
235515
kem_keygen average cycles:
74437
kem_encap average cycles:
83871
kem_decap average cycles:
105130
sign_keygen average cycles:
13460974
Gandalf_sign average cycles:
1168235
Gandalf_verify average cycles:
110579
```

#### `mlkem-512` + `falcon-512`

Sample output:
```
Post-quantum AKEM public key bytes: 1696
Post-quantum AKEM secret key bytes: 3680
Post-quantum AKEM ciphertext bytes: 2044
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen average cycles:
12363039
pq_akem_encap average cycles:
1116529
pq_akem_decap average cycles:
245677
kem_keygen average cycles:
78575
kem_encap average cycles:
91845
kem_decap average cycles:
117715
sign_keygen average cycles:
12205463
Gandalf_sign average cycles:
940177
Gandalf_verify average cycles:
110492
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
Type `./speed_h_akem`.

#### `bat-257` + `mitaka`

Sample output:
```
Hybrid AKEM public key bytes: 1449
Hybrid AKEM secret key bytes: 5033
Hybrid AKEM ciphertext bytes: 1781
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
25470899
h_akem_encap average cycles:
1986834
h_akem_decap average cycles:
800436
nike_keygen average cycles:
227480
nike_sdk average cycles:
228262
kem_keygen average cycles:
12025343
kem_encap average cycles:
56816
kem_decap average cycles:
229760
sign_keygen average cycles:
13477674
Gandalf_sign average cycles:
1136711
Gandalf_verify average cycles:
84190
```

#### `bat-257` + `falcon-512`

Sample output:
```
Hybrid AKEM public key bytes: 1449
Hybrid AKEM secret key bytes: 5033
Hybrid AKEM ciphertext bytes: 1781
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
24556113
h_akem_encap average cycles:
1747890
h_akem_decap average cycles:
798843
nike_keygen average cycles:
226467
nike_sdk average cycles:
226700
kem_keygen average cycles:
11770465
kem_encap average cycles:
56926
kem_decap average cycles:
230062
sign_keygen average cycles:
12213002
Gandalf_sign average cycles:
906337
Gandalf_verify average cycles:
84256
```

#### `mlkem-512` + `mitaka`

Sample output:
```
Hybrid AKEM public key bytes: 1728
Hybrid AKEM secret key bytes: 3712
Hybrid AKEM ciphertext bytes: 2076
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
13804389
h_akem_encap average cycles:
2033621
h_akem_decap average cycles:
674641
nike_keygen average cycles:
226747
nike_sdk average cycles:
229655
kem_keygen average cycles:
73514
kem_encap average cycles:
83346
kem_decap average cycles:
104510
sign_keygen average cycles:
13351576
Gandalf_sign average cycles:
1152319
Gandalf_verify average cycles:
92063
```

#### `mlkem-512` + `falcon-512`

Sample output:
```
Hybrid AKEM public key bytes: 1728
Hybrid AKEM secret key bytes: 3712
Hybrid AKEM ciphertext bytes: 2076
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
12645496
h_akem_encap average cycles:
1797365
h_akem_decap average cycles:
675321
nike_keygen average cycles:
227139
nike_sdk average cycles:
228670
kem_keygen average cycles:
74296
kem_encap average cycles:
83721
kem_decap average cycles:
105104
sign_keygen average cycles:
12188683
Gandalf_sign average cycles:
922625
Gandalf_verify average cycles:
91986
```


