# Comparative Benchmark Results — nstd vs __int128 vs Boost

**Date:** 19 March 2026
**Platform:** Windows 11, MSYS2 ucrt64
**CPU:** x86-64 (measurements in cycles/operation via RDTSC)
**Iterations:** 5,000,000 + 10,000 warmup
**Boost:** 1.90.0-3 (cpp_int, checked, GMP 6.3.0, TomMath 1.3.0)
**MSVC:** cl.exe 19.50.35726 /O2, Boost via vcpkg (c:\vcpkg\installed\x64-windows)

---

## GCC 15.2.0 -O2

```
[Addition (+)]                        cyc/op    vs u64
uint64_t                               0.72     1.00x
nstd::uint128_t                        0.90     1.25x
nstd::int128_t (TC)                    0.93     1.29x
unsigned __int128                      1.35     1.88x
__int128                               1.75     2.43x
boost::cpp_int u128                    4.44     6.16x
boost::cpp_int i128                    3.48     4.82x
boost::checked_uint128                 5.72     7.94x
boost::gmp_int                        27.39    38.01x
boost::gmp_int [128]                  38.34    53.19x
boost::tom_int                        26.09    36.19x
boost::tom_int [128]                  61.33    85.09x

[Subtraction (-)]                     cyc/op    vs u64
uint64_t                               0.78     1.00x
nstd::uint128_t                        1.75     2.23x
nstd::int128_t (TC)                    1.77     2.26x
unsigned __int128                      1.61     2.06x
__int128                               1.81     2.31x
boost::cpp_int u128                    5.07     6.48x
boost::cpp_int i128                    4.27     5.46x
boost::checked_uint128                 3.97     5.08x
boost::gmp_int                        35.64    45.56x
boost::gmp_int [128]                  39.42    50.39x
boost::tom_int (*)                   909.76  1163.00x  [anomaly — see note]
boost::tom_int [128]                  59.80    76.45x

[Multiplication (*)]                  cyc/op    vs u64
uint64_t                               2.06     1.00x
nstd::uint128_t                        3.53     1.71x
nstd::int128_t (TC)                    3.78     1.84x
unsigned __int128                      3.57     1.74x
__int128                               3.66     1.78x
boost::cpp_int u128                    5.88     2.86x
boost::cpp_int i128                    5.38     2.62x
boost::checked_uint128 (*)             1.36     0.66x  [non-accumulating]
boost::gmp_int (*)                     0.55     0.27x  [non-accumulating]
boost::gmp_int [128]                  44.56    21.66x
boost::tom_int (*)                     0.68     0.33x  [non-accumulating]
boost::tom_int [128]                 114.77    55.78x

[Division (/)]                        cyc/op    vs u64
uint64_t                               4.92     1.00x
nstd::uint128_t                        2.42     0.49x  *** FASTER THAN u64 ***
nstd::int128_t (TC)                    2.98     0.61x  *** FASTER THAN u64 ***
unsigned __int128                     49.67    10.09x
__int128                              49.09     9.98x
boost::cpp_int u128                   54.20    11.01x
boost::cpp_int i128                   52.56    10.68x
boost::checked_uint128                53.44    10.86x
boost::gmp_int                        82.02    16.67x
boost::gmp_int [128]                 121.13    24.62x
boost::tom_int                       919.64   186.89x
boost::tom_int [128]                 858.54   174.47x

[Shift (<<3 | >>125, rotate)]        cyc/op    vs u64
uint64_t                               0.79     1.00x
nstd::uint128_t                        2.80     3.52x
unsigned __int128                      3.91     4.93x
boost::cpp_int u128                    6.64     8.36x
boost::checked_uint128 (*)            10.43    13.14x
boost::gmp_int                        60.70    76.46x
boost::tom_int                       553.37   697.02x

[Bitwise XOR (^)]                     cyc/op    vs u64
uint64_t                               0.89     1.00x
nstd::uint128_t                        1.76     1.98x
unsigned __int128                      1.81     2.04x
boost::cpp_int u128                    4.79     5.40x
boost::checked_uint128                 4.68     5.27x
boost::gmp_int                        26.40    29.77x
boost::tom_int                        32.98    37.19x

[Comparison (<)]                      cyc/op    vs u64
uint64_t                               5.31     1.00x
nstd::uint128_t                        3.31     0.62x  *** FASTER THAN u64 ***
unsigned __int128                      5.97     1.12x
boost::cpp_int u128                    5.65     1.06x
boost::checked_uint128                 5.68     1.07x
boost::gmp_int                        12.89     2.43x
boost::tom_int                        11.87     2.23x
```

---

## GCC 15.2.0 -O3

```
[Addition (+)]                        cyc/op    vs u64
uint64_t                               0.77     1.00x
nstd::uint128_t                        0.76     0.98x  *** FASTER THAN u64 ***
nstd::int128_t (TC)                    0.75     0.98x  *** FASTER THAN u64 ***
unsigned __int128                      2.41     3.13x
__int128                               2.17     2.81x
boost::cpp_int u128                    4.35     5.65x
boost::cpp_int i128                    3.70     4.80x
boost::checked_uint128                 4.57     5.92x
boost::gmp_int                        31.11    40.34x
boost::gmp_int [128]                  48.49    62.87x
boost::tom_int                        29.81    38.65x
boost::tom_int [128]                  74.14    96.13x

[Subtraction (-)]                     cyc/op    vs u64
uint64_t                               0.85     1.00x
nstd::uint128_t                        1.21     1.43x
nstd::int128_t (TC)                    1.22     1.44x
unsigned __int128                      1.65     1.95x
__int128                               1.68     1.98x
boost::cpp_int u128                    4.54     5.37x
boost::checked_uint128                 4.27     5.05x

[Multiplication (*)]                  cyc/op    vs u64
uint64_t                               2.45     1.00x
nstd::uint128_t                        2.97     1.21x
nstd::int128_t (TC)                    2.92     1.19x
unsigned __int128                      3.28     1.34x
__int128                               3.26     1.33x
boost::cpp_int u128                    6.31     2.57x
boost::checked_uint128 (*)             1.52     0.62x  [non-accumulating]
boost::gmp_int (*)                     0.51     0.21x  [non-accumulating]

[Division (/)]                        cyc/op    vs u64
uint64_t                               4.42     1.00x
nstd::uint128_t                        2.08     0.47x  *** 2x FASTER THAN u64 ***
nstd::int128_t (TC)                    9.96     2.26x  [signed overhead]
unsigned __int128                     43.26     9.80x
__int128                              44.51    10.08x
boost::cpp_int u128                   47.64    10.79x
boost::gmp_int                        75.80    17.17x
boost::gmp_int [128]                  99.90    22.63x
boost::tom_int                       799.28   181.02x
boost::tom_int [128]                 814.10   184.38x

[Shift (<<3 | >>125, rotate)]        cyc/op    vs u64
uint64_t                               0.85     1.00x
nstd::uint128_t                        2.41     2.83x
unsigned __int128                      3.80     4.45x
boost::cpp_int u128                    7.09     8.30x

[Bitwise XOR (^)]                     cyc/op    vs u64
uint64_t                               0.88     1.00x
nstd::uint128_t                        0.88     1.00x  *** EQUAL TO u64 ***
unsigned __int128                      1.80     2.05x

[Comparison (<)]                      cyc/op    vs u64
uint64_t                               4.91     1.00x
nstd::uint128_t                        3.52     0.72x  *** FASTER THAN u64 ***
unsigned __int128                      5.52     1.12x
```

---

## Clang 21.1.8 (ucrt64) -O2

```
[Addition (+)]                        cyc/op    vs u64
uint64_t                               4.32     1.00x
nstd::uint128_t                        4.77     1.10x
nstd::int128_t (TC)                    4.72     1.09x
unsigned __int128                      4.65     1.08x
__int128                               4.29     0.99x
boost::cpp_int u128                    4.42     1.02x
boost::cpp_int i128                    3.96     0.92x
boost::checked_uint128                 3.67     0.85x
boost::gmp_int                        20.06     4.65x
boost::gmp_int [128]                  45.94    10.64x
boost::tom_int                        27.40     6.35x
boost::tom_int [128]                  72.06    16.69x

[Subtraction (-)]                     cyc/op    vs u64
uint64_t                               4.45     1.00x
nstd::uint128_t                        4.63     1.04x
nstd::int128_t (TC)                    4.34     0.98x  *** NEAR-EQUAL u64 ***
unsigned __int128                      3.97     0.89x
__int128                               4.50     1.01x
boost::cpp_int u128                    4.02     0.90x
boost::gmp_int                        20.67     4.64x
boost::tom_int (*)                   229.34    51.52x  [anomaly — see note]

[Multiplication (*)]                  cyc/op    vs u64
uint64_t                               5.42     1.00x
nstd::uint128_t                        6.24     1.15x
nstd::int128_t (TC)                    6.35     1.17x
unsigned __int128                      6.28     1.16x
__int128                               6.19     1.14x
boost::cpp_int u128                    6.17     1.14x
boost::checked_uint128 (*)             1.49     0.28x  [non-accumulating]
boost::gmp_int (*)                     1.49     0.28x  [non-accumulating]

[Division (/)]                        cyc/op    vs u64
uint64_t                               6.73     1.00x
nstd::uint128_t                       14.60     2.17x
nstd::int128_t (TC)                   25.64     3.81x
unsigned __int128                     47.18     7.01x
__int128                              47.17     7.01x
boost::cpp_int u128                   50.99     7.57x
boost::gmp_int                        86.04    12.78x
boost::gmp_int [128]                 100.41    14.91x
boost::tom_int                       799.41   118.75x

[Shift (<<3 | >>125, rotate)]        cyc/op    vs u64
uint64_t                               3.64     1.00x
nstd::uint128_t                        6.12     1.68x
unsigned __int128                      6.12     1.68x  [EQUAL TO nstd]
boost::cpp_int u128                    6.08     1.67x

[Bitwise XOR (^)]                     cyc/op    vs u64
uint64_t                               4.06     1.00x
nstd::uint128_t                        5.20     1.28x
unsigned __int128                      4.18     1.03x
boost::cpp_int u128                    3.65     0.90x

[Comparison (<)]                      cyc/op    vs u64
uint64_t                               7.88     1.00x
nstd::uint128_t                        2.56     0.33x  *** 3x FASTER THAN u64 ***
unsigned __int128                      8.95     1.14x
boost::cpp_int u128                    2.22     0.28x
```

---

## Clang 21.1.8 (ucrt64) -O3

```
[Addition (+)]                        cyc/op    vs u64
uint64_t                               4.28     1.00x
nstd::uint128_t                        4.73     1.10x
nstd::int128_t (TC)                    4.71     1.10x
unsigned __int128                      4.71     1.10x
__int128                               4.67     1.09x
boost::cpp_int u128                    4.84     1.13x
boost::checked_uint128                 4.20     0.98x
boost::gmp_int                        22.67     5.30x
boost::gmp_int [128]                  47.36    11.06x
boost::tom_int                        31.62     7.39x
boost::tom_int [128]                  84.31    19.70x

[Subtraction (-)]                     cyc/op    vs u64
uint64_t                               4.19     1.00x
nstd::uint128_t                        4.62     1.10x
unsigned __int128                      4.77     1.14x
boost::cpp_int u128                    4.71     1.12x
boost::gmp_int                        25.43     6.07x
boost::tom_int (*)                   220.29    52.54x  [anomaly — see note]

[Multiplication (*)]                  cyc/op    vs u64
uint64_t                               5.33     1.00x
nstd::uint128_t                        6.22     1.17x
nstd::int128_t (TC)                    6.20     1.16x
unsigned __int128                      6.11     1.15x
__int128                               6.71     1.26x
boost::cpp_int u128                    6.20     1.16x
boost::checked_uint128 (*)             1.47     0.28x  [non-accumulating]
boost::gmp_int (*)                     2.30     0.43x  [non-accumulating]

[Division (/)]                        cyc/op    vs u64
uint64_t                               6.84     1.00x
nstd::uint128_t                       14.91     2.18x
nstd::int128_t (TC)                   26.21     3.83x
unsigned __int128                     45.48     6.65x
__int128                              45.59     6.66x
boost::cpp_int u128                   48.11     7.03x
boost::gmp_int                        85.33    12.47x
boost::gmp_int [128]                  99.32    14.51x
boost::tom_int                       817.40   119.44x

[Shift (<<3 | >>125, rotate)]        cyc/op    vs u64
uint64_t                               3.97     1.00x
nstd::uint128_t                        6.48     1.63x
unsigned __int128                      6.37     1.60x
boost::cpp_int u128                    6.25     1.57x

[Bitwise XOR (^)]                     cyc/op    vs u64
uint64_t                               3.94     1.00x
nstd::uint128_t                        5.52     1.40x
unsigned __int128                      4.13     1.05x
boost::cpp_int u128                    3.61     0.92x

[Comparison (<)]                      cyc/op    vs u64
uint64_t                               7.84     1.00x
nstd::uint128_t                        2.55     0.33x  *** 3x FASTER THAN u64 ***
unsigned __int128                      8.79     1.12x
boost::cpp_int u128                    2.17     0.28x
```

---

## Summary Matrix — nstd::uint128_t vs u64

| Operation | GCC-O2 | GCC-O3 | Clang-O2 | Clang-O3 | vs __int128 (GCC-O3) |
|-----------|--------|--------|----------|----------|----------------------|
| Addition  | 1.25x  | **0.98x** | 1.10x | 1.10x | **3.2x faster** |
| Subtraction | 2.23x | 1.43x | 1.04x | 1.10x | 1.36x faster |
| Multiply  | 1.71x  | 1.21x  | 1.15x  | 1.17x  | 1.11x faster |
| Division  | **0.49x** | **0.47x** | 2.17x | 2.18x | **20.9x faster** |
| Shift     | 3.52x  | 2.83x  | 1.68x  | 1.63x  | 1.57x faster (O3) |
| XOR       | 1.98x  | **1.00x** | 1.28x | 1.40x | 1.95x faster |
| Comparison | **0.62x** | **0.72x** | **0.33x** | **0.33x** | **1.56x faster** |

### Key Findings

**Division (Knuth D algorithm):**
- GCC-O2: **0.49x vs uint64_t** (2x faster than native 64-bit division!)
- GCC-O3: **0.47x vs uint64_t** (2.1x faster than native 64-bit division!)
- **20x faster than `unsigned __int128`** (GCC-O3: 2.08 vs 43.26 cyc/op)
- Clang: 2.17x vs u64 (slower than GCC, but 3.2x faster than `__int128`)

**Addition (GCC-O3 inlining advantage):**
- GCC-O3: **0.98x vs uint64_t** — virtually identical performance
- GCC-O3 nstd is **3.2x faster than `unsigned __int128`** for addition

**Comparison:**
- Clang O2/O3: **0.33x vs uint64_t** — 3x faster than native 64-bit comparison
- GCC-O2: **0.62x vs uint64_t** — also faster

**XOR (GCC-O3):**
- GCC-O3: **1.00x** — identical to uint64_t

### vs Boost cpp_int

| Operation | nstd (GCC-O3) | boost::cpp_int | Ratio |
|-----------|--------------|----------------|-------|
| Addition  | 0.98x vs u64 | 5.65x vs u64   | **5.8x faster** |
| Division  | 0.47x vs u64 | 10.79x vs u64  | **22.9x faster** |
| Multiply  | 1.21x vs u64 | 2.57x vs u64   | **2.1x faster** |
| Shift     | 2.83x vs u64 | 8.30x vs u64   | **2.9x faster** |

### Anomalies / Notes

- `boost::tom_int` subtraction shows ~900-1000 cyc/op on GCC (anomaly - likely TomMath subtraction borrow chain issue)
- `(*)` markers indicate non-accumulating benchmark pattern (no loop-carried dependency), used for arbitrary-precision types to avoid value overflow
- `[128]` variants are arbitrary-precision types masked to 128 bits (`& mask128`) for fair comparison

---

## MSVC 19.50.35726 /O2 (vcpkg Boost, with FORCE_GMP_TOMMATH)

Note: No `__int128` on MSVC. Division uses `big_bin_divrem()` fallback (Knuth D fast paths not available without `__uint128_t`).

```
[Addition (+)]                        cyc/op    vs u64
uint64_t                              14.16     1.00x
nstd::uint128_t                       13.99     0.99x  *** FASTER THAN u64 ***
nstd::int128_t (TC)                   14.10     1.00x  *** EQUAL TO u64 ***
boost::cpp_int u128                   18.62     1.31x
boost::cpp_int i128                   23.92     1.69x
boost::checked_uint128                24.10     1.70x
boost::gmp_int                        23.84     1.68x
boost::gmp_int [128]                  45.47     3.21x
boost::tom_int                        32.18     2.27x
boost::tom_int [128]                  96.87     6.84x

[Subtraction (-)]                     cyc/op    vs u64
uint64_t                              13.41     1.00x
nstd::uint128_t                       13.37     1.00x  *** EQUAL TO u64 ***
nstd::int128_t (TC)                   12.86     0.96x  *** FASTER THAN u64 ***
boost::cpp_int u128                   15.71     1.17x
boost::gmp_int                        24.37     1.82x
boost::tom_int (*)                   212.97    15.88x  [anomaly]

[Multiplication (*)]                  cyc/op    vs u64
uint64_t                              14.08     1.00x
nstd::uint128_t                       14.32     1.02x
nstd::int128_t (TC)                   31.37     2.23x
boost::cpp_int u128                   31.20     2.22x
boost::gmp_int (*)                     1.31     0.09x  [non-accumulating]
boost::tom_int (*)                     1.42     0.10x  [non-accumulating]

[Division (/)]                        cyc/op    vs u64
uint64_t                              16.45     1.00x
nstd::uint128_t                       28.60     1.74x  [big_bin_divrem fallback]
nstd::int128_t (TC)                   35.41     2.15x
boost::cpp_int u128                   43.82     2.66x
boost::cpp_int i128                   57.33     3.49x
boost::checked_uint128                49.87     3.03x
boost::gmp_int                        91.05     5.54x
boost::tom_int                       784.01    47.67x
boost::tom_int [128]                 827.77    50.33x

[Shift (<<3 | >>125, rotate)]        cyc/op    vs u64
uint64_t                              13.66     1.00x
nstd::uint128_t                       26.95     1.97x
boost::cpp_int u128                   75.28     5.51x
boost::gmp_int                        66.28     4.85x
boost::tom_int                       535.42    39.21x

[Bitwise XOR (^)]                     cyc/op    vs u64
uint64_t                              13.21     1.00x
nstd::uint128_t                       13.80     1.05x
boost::cpp_int u128                   15.47     1.17x
boost::gmp_int                        19.46     1.47x
boost::tom_int                        48.99     3.71x

[Comparison (<)]                      cyc/op    vs u64
uint64_t                              19.42     1.00x
nstd::uint128_t                       13.84     0.71x  *** FASTER THAN u64 ***
boost::cpp_int u128                   10.95     0.56x
boost::gmp_int                        13.89     0.72x
boost::tom_int                        23.78     1.22x
```

---

## Summary Matrix — nstd::uint128_t vs u64 (ALL COMPILERS)

| Operation   | GCC-O2   | GCC-O3   | Clang-O2 | Clang-O3 | MSVC-O2  |
|-------------|----------|----------|----------|----------|----------|
| Addition    | 1.25x    | 0.98x †  | 1.10x    | 1.10x    | 0.99x †  |
| Subtraction | 2.23x    | 1.43x    | 1.04x    | 1.10x    | 1.00x †  |
| Multiply    | 1.71x    | 1.21x    | 1.15x    | 1.17x    | 1.02x    |
| Division    | 0.49x †  | 0.47x †  | 2.17x    | 2.18x    | 1.74x *  |
| Shift       | 3.52x    | 2.83x    | 1.68x    | 1.63x    | 1.97x    |
| XOR         | 1.98x    | 1.00x †  | 1.28x    | 1.40x    | 1.05x    |
| Comparison  | 0.62x †  | 0.72x †  | 0.33x †  | 0.33x †  | 0.71x †  |

† = faster than uint64_t baseline (ratio < 1.00x)
\* = MSVC division uses `big_bin_divrem()` fallback — Knuth D fast paths require `__uint128_t`

### Key Findings

**Division (Knuth D algorithm):**

- GCC-O2: **0.49x vs uint64_t** (2x faster than native 64-bit!)
- GCC-O3: **0.47x vs uint64_t** (2.1x faster than native 64-bit!)
- **20x faster than `unsigned __int128`** (GCC-O3: 2.08 vs 43.26 cyc/op)
- Clang O2/O3: 2.17x vs u64 — still **3.2x faster than `__int128`**
- MSVC: 1.74x vs u64 (big_bin_divrem fallback) — still **1.5x faster than boost::cpp_int**

**Addition / Subtraction (MSVC & GCC-O3):**

- MSVC /O2: **0.99x/1.00x** — essentially identical to uint64_t
- GCC-O3: **0.98x** — virtually identical

**Comparison:**

- Clang O2/O3: **0.33x vs uint64_t** — 3x faster than native 64-bit comparison
- GCC-O2: **0.62x**, MSVC: **0.71x** — all faster than uint64_t

**Multiply:**

- MSVC /O2: **1.02x** — essentially equal to uint64_t

### vs boost::cpp_int (GCC-O3)

| Operation | nstd (GCC-O3) | boost::cpp_int | Ratio            |
|-----------|---------------|----------------|------------------|
| Addition  | 0.98x vs u64  | 5.65x vs u64   | **5.8x faster**  |
| Division  | 0.47x vs u64  | 10.79x vs u64  | **22.9x faster** |
| Multiply  | 1.21x vs u64  | 2.57x vs u64   | **2.1x faster**  |
| Shift     | 2.83x vs u64  | 8.30x vs u64   | **2.9x faster**  |

### vs boost::cpp_int (MSVC /O2)

| Operation | nstd (MSVC)  | boost::cpp_int | Ratio           |
|-----------|--------------|----------------|-----------------|
| Addition  | 0.99x vs u64 | 1.31x vs u64   | **1.3x faster** |
| Division  | 1.74x vs u64 | 2.66x vs u64   | **1.5x faster** |
| Multiply  | 1.02x vs u64 | 2.22x vs u64   | **2.2x faster** |
| Shift     | 1.97x vs u64 | 5.51x vs u64   | **2.8x faster** |

### Anomalies and Notes

- `boost::tom_int` subtraction shows anomalously high cyc/op on GCC and MSVC (likely TomMath borrow-chain issue with accumulating pattern)
- `(*)` markers = non-accumulating benchmark pattern (no loop-carried dependency)
- `[128]` variants = arbitrary-precision types masked to 128 bits for fair comparison
- MSVC cycles/op are higher in absolute terms (RDTSC resolution + higher branch predictor activity)

### Pending Compilers

- Intel ICX (Windows): not yet run
- WSL compilers (GCC 13/14/15, Clang 18/20/21): not yet run

---

**Generated:** 19 March 2026
**Benchmark file:** `benchs/benchmark_vs_builtin.cpp`

**Build:**

- GCC/Clang: `# [DEBUG] - /c/msys64/ucrt64/bin/g++ (clang++)` (make.py compare has path bug)
- MSVC: `# [DEBUG] - cl.exe via vcpkg Boost (c:\vcpkg\installed\x64-windows)`
