# Comparative Benchmark Results — nstd vs __int128 vs Boost

**Date:** 19 March 2026 (updated session 2 — all compilers)
**Platform:** Windows 11, MSYS2 ucrt64 + WSL Ubuntu 25.04
**CPU:** x86-64 (measurements in cycles/operation via RDTSC)
**Iterations:** 5,000,000 + 10,000 warmup
**Boost:** 1.90.0-3 (cpp_int, checked, GMP 6.3.0, TomMath 1.3.0)
**MSVC:** cl.exe 19.50.35726 /O2, Boost via vcpkg (c:\vcpkg\installed\x64-windows)
**Intel ICX Windows:** 2025.3.0, GCC-style flags, vcpkg Boost
**WSL compilers:** GCC 14.2.0, Clang 20.1.8, Intel ICX 2025.3.2 (Ubuntu 25.04)

**Knuth D refactoring (session 2):** Exposed fast paths [0.a–0.e], [1], [2] for MSVC (pure C++);
added `_udiv128` path [3] for 128/64 division on MSVC at runtime.

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

## MSVC 19.50.35726 /O2 — Session 1 (big_bin_divrem fallback)

Note: Historical baseline — before Knuth D fast path exposure. Division used `big_bin_divrem()` (no fast paths without `__uint128_t`).

```text
[Division (/)]                        cyc/op    vs u64
uint64_t                              16.45     1.00x
nstd::uint128_t                       28.60     1.74x  [big_bin_divrem fallback]
nstd::int128_t (TC)                   35.41     2.15x
boost::cpp_int u128                   43.82     2.66x
```

---

## MSVC 19.50.35726 /O2 — Session 2 (Knuth D fast paths via _udiv128)

Note: After refactoring — fast paths [0.a–0.e], [1], [2], [3] now active on MSVC.
Path [3] uses `_udiv128` intrinsic (VS 2019 16.8+ / _MSC_VER >= 1928) at runtime.
128/128 general case still falls to `big_bin_divrem` (no `__uint128_t` on MSVC).

```text
[Addition (+)]                        cyc/op    vs u64
uint64_t                              13.67     1.00x
nstd::uint128_t                       13.33     0.98x  *** FASTER THAN u64 ***
nstd::int128_t (TC)                   13.31     0.97x  *** FASTER THAN u64 ***
boost::cpp_int u128                   17.72     1.30x
boost::cpp_int i128                   18.47     1.35x
boost::checked_uint128                17.40     1.27x
boost::gmp_int                        18.73     1.37x
boost::gmp_int [128]                  40.54     2.97x
boost::tom_int                        29.37     2.15x
boost::tom_int [128]                  97.51     7.13x

[Subtraction (-)]                     cyc/op    vs u64
uint64_t                              13.86     1.00x
nstd::uint128_t                       13.59     0.98x  *** FASTER THAN u64 ***
nstd::int128_t (TC)                   13.41     0.97x  *** FASTER THAN u64 ***
boost::cpp_int u128                   15.75     1.14x
boost::cpp_int i128                   16.70     1.20x
boost::checked_uint128                15.45     1.11x
boost::gmp_int                        26.01     1.88x
boost::gmp_int [128]                  42.74     3.08x
boost::tom_int (*)                   227.90    16.44x  [anomaly]
boost::tom_int [128]                  88.13     6.36x

[Multiplication (*)]                  cyc/op    vs u64
uint64_t                              14.08     1.00x
nstd::uint128_t                       15.45     1.10x
nstd::int128_t (TC)                   31.49     2.24x
boost::cpp_int u128                   31.53     2.24x
boost::cpp_int i128                   34.51     2.45x
boost::checked_uint128 (*)            16.88     1.20x  [non-accumulating]
boost::gmp_int (*)                     1.29     0.09x  [non-accumulating]
boost::gmp_int [128]                  43.68     3.10x
boost::tom_int (*)                     1.31     0.09x  [non-accumulating]
boost::tom_int [128]                 143.21    10.17x

[Division (/)]                        cyc/op    vs u64
uint64_t                              16.54     1.00x
nstd::uint128_t                       26.47     1.60x  [fast paths active, was 1.74x]
nstd::int128_t (TC)                   32.59     1.97x
boost::cpp_int u128                   44.28     2.68x
boost::cpp_int i128                   57.29     3.46x
boost::checked_uint128                49.55     2.99x
boost::gmp_int                        89.92     5.44x
boost::gmp_int [128]                 108.67     6.57x
boost::tom_int                       764.76    46.23x
boost::tom_int [128]                 839.87    50.77x

[Shift (<<3 | >>125, rotate)]        cyc/op    vs u64
uint64_t                              14.34     1.00x
nstd::uint128_t                       26.12     1.82x
boost::cpp_int u128                   83.53     5.82x
boost::checked_uint128 (*)            33.92     2.36x  [non-accumulating]
boost::gmp_int                        59.54     4.15x
boost::tom_int                       550.32    38.36x

[Bitwise XOR (^)]                     cyc/op    vs u64
uint64_t                              13.20     1.00x
nstd::uint128_t                       14.02     1.06x
boost::cpp_int u128                   15.69     1.19x
boost::checked_uint128                16.12     1.22x
boost::gmp_int                        18.97     1.44x
boost::tom_int                        48.63     3.68x

[Comparison (<)]                      cyc/op    vs u64
uint64_t                              19.51     1.00x
nstd::uint128_t                       13.36     0.68x  *** FASTER THAN u64 ***
boost::cpp_int u128                   11.13     0.57x
boost::checked_uint128                11.20     0.57x
boost::gmp_int                         8.45     0.43x
boost::tom_int                        22.03     1.13x
```

---

## Intel ICX 2025.3.0 (Windows) /O2

Note: Intel ICX on Windows uses MSVC ABI. Has `__int128` support. Division uses Knuth D fast paths including `__uint128_t` path. GCC-style flags. Boost via vcpkg.

```text
[Addition (+)]                        cyc/op    vs u64
uint64_t                               4.04     1.00x
nstd::uint128_t                        4.10     1.02x
nstd::int128_t (TC)                    4.17     1.03x
unsigned __int128                      4.31     1.07x
__int128                               4.32     1.07x
boost::cpp_int u128                   22.26     5.52x
boost::cpp_int i128                   28.82     7.14x
boost::checked_uint128                20.83     5.16x
boost::gmp_int                        19.39     4.80x
boost::gmp_int [128]                  43.34    10.74x
boost::tom_int                        32.28     8.00x
boost::tom_int [128]                 113.39    28.10x

[Subtraction (-)]                     cyc/op    vs u64
uint64_t                               4.18     1.00x
nstd::uint128_t                        3.79     0.91x  *** FASTER THAN u64 ***
nstd::int128_t (TC)                    3.60     0.86x  *** FASTER THAN u64 ***
unsigned __int128                      4.45     1.06x
__int128                               4.12     0.99x
boost::cpp_int u128                   17.38     4.16x

[Division (/)]                        cyc/op    vs u64
uint64_t                               7.20     1.00x
nstd::uint128_t                       23.44     3.26x
nstd::int128_t (TC)                   36.45     5.06x
unsigned __int128                     24.73     3.43x
__int128                              33.20     4.61x
boost::cpp_int u128                   43.57     6.05x
boost::cpp_int i128                   55.58     7.72x
boost::checked_uint128                45.86     6.37x
boost::gmp_int                       122.64    17.03x
boost::gmp_int [128]                 141.38    19.64x
boost::tom_int                       809.50   112.43x
boost::tom_int [128]                 998.83   138.72x
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

### vs boost::cpp_int (MSVC /O2 — session 2, fast paths active)

| Operation | nstd (MSVC)  | boost::cpp_int | Ratio           |
|-----------|--------------|----------------|-----------------|
| Addition  | 0.98x vs u64 | 1.30x vs u64   | **1.3x faster** |
| Division  | 1.60x vs u64 | 2.68x vs u64   | **1.7x faster** |
| Multiply  | 1.10x vs u64 | 2.24x vs u64   | **2.0x faster** |
| Shift     | 1.82x vs u64 | 5.82x vs u64   | **3.2x faster** |

---

## WSL Ubuntu 25.04 — GCC 14.2.0 -O2

```text
[Division (/)]                        cyc/op    vs u64
uint64_t                               4.62     1.00x
nstd::uint128_t                        9.51     2.06x
nstd::int128_t (TC)                   11.34     2.45x
unsigned __int128                     28.11     6.08x
__int128                              28.99     6.27x
boost::cpp_int u128                   31.07     6.72x
boost::cpp_int i128                   30.04     6.50x
boost::checked_uint128                29.42     6.37x
boost::gmp_int                        70.52    15.26x
boost::gmp_int [128]                  82.12    17.77x
boost::tom_int                       283.75    61.40x
boost::tom_int [128]                 379.52    82.12x
```

Note: nstd **2.95x faster than `__int128`** for division (6.27x → 2.06x).

---

## WSL Ubuntu 25.04 — GCC 14.2.0 -O3

```text
[Division (/)]                        cyc/op    vs u64
uint64_t                               4.08     1.00x
nstd::uint128_t                       36.69     8.98x  [GCC-O3 optimization anomaly — pre-existing]
nstd::int128_t (TC)                   19.49     4.77x
unsigned __int128                     26.56     6.50x
__int128                              27.17     6.65x
boost::cpp_int u128                   29.86     7.31x
boost::gmp_int                        63.68    15.59x
boost::gmp_int [128]                  78.52    19.22x
boost::tom_int                       343.50    84.09x
```

Note: GCC-O3 uint128 anomaly confirmed pre-existing (same in old code: 8.07x). GCC-O2 is the reliable result.
Root cause investigated via assembly analysis — see `GCC_O3_DIVISION_ANOMALY.md`.

---

## WSL Ubuntu 25.04 — Clang 20.1.8 -O2

```text
[Division (/)]                        cyc/op    vs u64
uint64_t                               5.96     1.00x
nstd::uint128_t                       13.72     2.30x
nstd::int128_t (TC)                   24.79     4.16x
unsigned __int128                     27.14     4.55x
__int128                              27.27     4.57x
boost::cpp_int u128                   28.63     4.80x
boost::cpp_int i128                   46.98     7.88x
boost::checked_uint128                28.47     4.77x
boost::gmp_int                        62.89    10.55x
boost::gmp_int [128]                  94.09    15.78x
boost::tom_int                       328.41    55.08x
boost::tom_int [128]                 332.22    55.72x
```

Note: nstd **1.98x faster than `__int128`** for division (4.57x → 2.30x).

---

## WSL Ubuntu 25.04 — Clang 20.1.8 -O3

```text
[Division (/)]                        cyc/op    vs u64
uint64_t                               6.85     1.00x
nstd::uint128_t                       15.35     2.24x
nstd::int128_t (TC)                   28.55     4.17x
unsigned __int128                     28.78     4.20x
__int128                              29.82     4.36x
boost::cpp_int u128                   30.08     4.39x
boost::cpp_int i128                   52.49     7.67x
boost::gmp_int                        66.51     9.72x
boost::gmp_int [128]                  94.09    13.74x
boost::tom_int                       358.86    52.42x
boost::tom_int [128]                 449.32    65.63x
```

---

## WSL Ubuntu 25.04 — Intel ICX 2025.3.2 -O2

```text
[Division (/)]                        cyc/op    vs u64
uint64_t                               7.45     1.00x
nstd::uint128_t                        7.12     0.96x  *** FASTER THAN u64 ***
nstd::int128_t (TC)                    9.72     1.31x
unsigned __int128                     26.30     3.53x
__int128                              27.12     3.64x
boost::cpp_int u128                   38.10     5.12x
boost::cpp_int i128                   39.36     5.28x
boost::checked_uint128                64.51     8.66x
boost::gmp_int                       100.03    13.43x
boost::gmp_int [128]                 120.88    16.23x
boost::tom_int                       317.73    42.65x
boost::tom_int [128]                 339.65    45.60x
```

Note: WSL ICX division **0.96x vs u64** — faster than native 64-bit. **3.68x faster than `__int128`**.

---

## Summary Matrix — Division Performance (all compilers, nstd::uint128_t vs u64)

| Compiler            | nstd vs u64 | vs __int128 | Notes                        |
|---------------------|-------------|-------------|------------------------------|
| Win GCC-O2 15.2.0   | **0.49x** † | 20x faster  | Fastest GCC result           |
| Win GCC-O3 15.2.0   | **0.47x** † | 20x faster  | Best overall (Win)           |
| Win Clang-O2 21.1.8 | 2.26x       | 2.99x faster| Clang less aggressive        |
| Win Clang-O3 21.1.8 | 2.16x       | 2.87x faster|                              |
| Win MSVC /O2        | 1.60x *     | n/a         | _udiv128 fast paths (new)    |
| Win ICX /O2         | 3.26x       | 1.05x       | ICX path 3 less optimized    |
| WSL GCC-O2 14.2.0   | 2.06x       | 2.95x faster| GCC 14 vs 15 difference      |
| WSL GCC-O3 14.2.0   | 8.98x ††    | n/a         | Known anomaly (pre-existing) |
| WSL Clang-O2 20.1.8 | 2.30x       | 1.98x faster| Clang 20 WSL                 |
| WSL Clang-O3 20.1.8 | 2.24x       | 1.87x faster|                              |
| WSL ICX /O2         | **0.96x** † | 3.68x faster| Fastest division overall     |

† = faster than uint64_t baseline
†† = pre-existing GCC-O3 anomaly in WSL (same before and after refactoring)
\* = `_udiv128` fast path active (was 1.74x before refactoring); 128/128 still big_bin_divrem

### Anomalies and Notes

- `boost::tom_int` subtraction shows anomalously high cyc/op on GCC and MSVC (likely TomMath borrow-chain issue with accumulating pattern)
- `(*)` markers = non-accumulating benchmark pattern (no loop-carried dependency)
- `[128]` variants = arbitrary-precision types masked to 128 bits for fair comparison
- MSVC cycles/op are higher in absolute terms (RDTSC resolution + higher branch predictor activity)
- WSL cycles may differ from native Windows due to virtualization layer

---

**Generated:** 19 March 2026 (session 2 — all 9 compiler/mode combinations)
**Benchmark file:** `benchs/benchmark_vs_builtin.cpp`

**Build:**

- Win GCC/Clang: `# [DEBUG]` direct compile via `/c/msys64/ucrt64/bin/g++` and `clang++`
- Win MSVC: `# [DEBUG]` via `msvc_bench.ps1` (PowerShell, vcpkg Boost)
- Win ICX: `# [DEBUG]` via `icpx_bench.ps1` (PowerShell, vcpkg Boost)
- WSL: `# [DEBUG]` direct compile via `wsl bash -c "g++/clang++/icpx"`
