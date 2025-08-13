
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
Type `./speed_pq_akem`.

#### `bat-257` + `mitaka`

Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen average cycles:
25345954
pq_akem_encap average cycles:
1297677
pq_akem_decap average cycles:
344635
kem_keygen average cycles:
11851947
kem_encap average cycles:
58497
kem_decap average cycles:
230252
sign_keygen average cycles:
13436393
Gandalf_sign average cycles:
1158665
Gandalf_verify average cycles:
100989
```

#### `bat-257` + `falcon-512`

Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen average cycles:
24724761
pq_akem_encap average cycles:
1074998
pq_akem_decap average cycles:
352785
kem_keygen average cycles:
12192381
kem_encap average cycles:
57009
kem_decap average cycles:
230135
sign_keygen average cycles:
12273693
Gandalf_sign average cycles:
928048
Gandalf_verify average cycles:
101234
```

#### `mlkem-512` + `mitaka`

Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
pq_akem_keygen average cycles:
13482697
pq_akem_encap average cycles:
1332821
pq_akem_decap average cycles:
232536
kem_keygen average cycles:
72894
kem_encap average cycles:
82562
kem_decap average cycles:
103366
sign_keygen average cycles:
13378206
Gandalf_sign average cycles:
1159259
Gandalf_verify average cycles:
109688
```

#### `mlkem-512` + `falcon-512`

Sample output:
```
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
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
25761718
h_akem_encap average cycles:
1994683
h_akem_decap average cycles:
792853
nike_keygen average cycles:
227623
nike_sdk average cycles:
226948
kem_keygen average cycles:
12281158
kem_encap average cycles:
57655
kem_decap average cycles:
230753
sign_keygen average cycles:
13434740
Gandalf_sign average cycles:
1144247
Gandalf_verify average cycles:
84319
```

#### `bat-257` + `falcon-512`

Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
24132786
h_akem_encap average cycles:
1755577
h_akem_decap average cycles:
799076
nike_keygen average cycles:
227143
nike_sdk average cycles:
227968
kem_keygen average cycles:
12113522
kem_encap average cycles:
57433
kem_decap average cycles:
232074
sign_keygen average cycles:
12297471
Gandalf_sign average cycles:
916199
Gandalf_verify average cycles:
84785
```

#### `mlkem-512` + `mitaka`

Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
13783450
h_akem_encap average cycles:
2005094
h_akem_decap average cycles:
671695
nike_keygen average cycles:
226398
nike_sdk average cycles:
228291
kem_keygen average cycles:
73798
kem_encap average cycles:
83341
kem_decap average cycles:
103906
sign_keygen average cycles:
13353077
Gandalf_sign average cycles:
1137731
Gandalf_verify average cycles:
91000
```

#### `mlkem-512` + `falcon-512`

Sample output:
```
loaded db: a14 (Apple A14/M1)
number of fixed counters: 2
number of configurable counters: 8
h_akem_keygen average cycles:
12644717
h_akem_encap average cycles:
1792949
h_akem_decap average cycles:
678492
nike_keygen average cycles:
227720
nike_sdk average cycles:
227161
kem_keygen average cycles:
75330
kem_encap average cycles:
84935
kem_decap average cycles:
105796
sign_keygen average cycles:
12183916
Gandalf_sign average cycles:
921453
Gandalf_verify average cycles:
91992
```


