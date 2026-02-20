
# How to build and run the binaries

This files explains how the binaries are built with the `Makefile` in `src`.
Example usages are included.

## Compilation

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
- `make KEM_PATH=mlkem RSIG_PATH=GandalfFalcon` (default)
- `make KEM_PATH=mlkem RSIG_PATH=GandalfFalconC`
- `make KEM_PATH=mlkem RSIG_PATH=GandalfMitaka`
- `make KEM_PATH=BAT RSIG_PATH=GandalfFalcon`
- `make KEM_PATH=BAT RSIG_PATH=GandalfFalconC`
- `make KEM_PATH=BAT RSIG_PATH=GandalfMitaka`

One can also overwrite the compiler (defaulted to `gcc`) with `CC=[compiler]`.

## Running the binaries

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



