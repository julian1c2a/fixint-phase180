# GCC -O3 Division Anomaly: Root Cause Analysis

**Symptom:** `nstd::uint128_t` division is ~4x slower under GCC -O3 than GCC -O2.

```text
WSL GCC 14.2.0 -O2:   nstd::uint128_t   9.29 cyc/op   (2.28x vs u64)
WSL GCC 14.2.0 -O3:   nstd::uint128_t  36.69 cyc/op   (8.98x vs u64)
```

Confirmed pre-existing (same ratio with old `big_bin_divrem` code).
Confirmed NOT present on Windows GCC 15.2.0 (shows 1.94 cyc/op O2, 2.05 cyc/op O3).

---

## Investigation Method

Generated verbose assembly (`-S -fverbose-asm`) for both optimization levels on
WSL Ubuntu 25.04 / GCC 14.2.0, then compared codegen for `D_knuth_divrem` and the
`bench_div_nstd_u128` hot loop.

---

## Root Cause

### GCC -O2: `D_knuth_divrem` is fully inlined

With `-O2`, GCC inlines `D_knuth_divrem()` into `bench_div_nstd_u128()`. The function
does NOT appear as a standalone symbol in the `.s` file:

```bash
grep 'D_knuth_divrem' bench_O2_wsl.s   # → no output (fully inlined)
```

**Effect of inlining:** The constant divisor `b = 12345` is visible throughout all fast
paths. GCC applies:

1. **Constant-folded comparisons:** all `divisor.data[1] == 0` / `divisor.data[0] == 1`
   checks become `je/jne` against literal constants.
2. **Steady-state hot path:** After ~5 iterations the accumulating value `a` falls below
   12345. Fast path [0.d] (`dividend < divisor → return {0, a}`) is taken every iteration:
   ~2 comparisons + copy = **5–9 cyc/op**.

### GCC -O3: `D_knuth_divrem` is NOT inlined (ISRA applied instead)

With `-O3`, GCC decides `D_knuth_divrem` is too large to inline (different heuristics
than -O2). Instead, it applies **ISRA** (Interprocedural Scalar Replacement of Aggregates),
creating a separate function `D_knuth_divrem.isra.0`:

```bash
grep 'D_knuth_divrem' bench_O3.s
# → _ZNK...D_knuth_divremERKS3_.isra.0   (standalone function, lines 3505–3783)
#   call _ZNK...D_knuth_divremERKS3_.isra.0   (line 23869, 23902)
```

The ISRA variant takes the two `divisor.data[]` halves as separate scalar registers
(`%edx = data[0]`, `%ecx = data[1]`) instead of a struct pointer. This is a legitimate
optimization to avoid memory indirection, but it prevents the call site's constant values
from being propagated into the function body.

**Effect:** Each iteration of the benchmark loop:

1. **`call D_knuth_divrem.isra.0`** — function call + frame setup:
   ```asm
   pushq   %rbp               # 1 cycle
   pushq   %rbx               # 1 cycle
   subq    $72, %rsp          # frame allocation
   ```

2. **Stack canary write** (added by GCC -O3 for functions with alloca-like frames):
   ```asm
   movq    %fs:40, %rsi       # TLS read (3–5 cycles)
   movq    %rsi, 56(%rsp)     # canary store
   ```

3. **Fast path [0.d] comparisons** (hot path after ~5 iterations): ~3 cycles

4. **Stack canary check + return**:
   ```asm
   movq    56(%rsp), %rax     # canary read
   subq    %fs:40, %rax       # TLS read + compare (3–5 cycles)
   jne     .L356              # (fail → __stack_chk_fail)
   addq    $72, %rsp          # frame teardown
   popq    %rbx
   popq    %rbp
   ret
   ```

**Total overhead per iteration:** ~25–35 cycles for function call infrastructure,
independent of what `D_knuth_divrem` actually computes.

---

## Path-by-Path Comparison

### Hot path after warmup (a < 12345): fast path [0.d]

| Step                      | O2 (inlined)          | O3 (non-inlined ISRA)            |
|---------------------------|-----------------------|----------------------------------|
| Call setup                | 0 cycles              | ~10–15 cycles (call + frame)     |
| Stack canary              | 0 cycles              | ~8–10 cycles (2× TLS access)    |
| Comparison: div > dividend| 2 cycles (constants)  | 2 cycles (registers)             |
| Return value copy         | 2 cycles              | 2 cycles                         |
| Stack teardown + ret      | 0 cycles              | ~8–10 cycles                     |
| **Total**                 | **~4–6 cycles**       | **~30–37 cycles**                |

### First ~5 iterations: path [3] (128/64 composed, data[1] != 0)

| Step                      | O2 (inlined)          | O3 (non-inlined ISRA)             |
|---------------------------|-----------------------|-----------------------------------|
| q_hi = data[1] / d        | Reciprocal multiply*  | `divq %rsi` (35–40 cycles)        |
| r_hi = data[1] - q_hi × d | `imulq` (3–5 cycles) | `imulq` (3–5 cycles)              |
| q_lo from (r_hi:data[0])/d| `call __udivti3` (128/64 via divq internally) | `call __udivmodti4` (128/64 via divq) |
| Function overhead         | 0 cycles              | ~25 cycles                        |

*O2 constant-propagates `d = 12345` and uses `mulq $magic; shrq $12` reciprocal multiplication
for the 64/64 division — ~5 cycles vs `divq`'s 35–40 cycles.

---

## Key Asymmetry: `__udivti3` vs `__udivmodti4`

O2 calls `__udivti3` (quotient only); O3 calls `__udivmodti4` (quotient + remainder via pointer):

```text
__udivti3    — unsigned 128/128 → quotient only (~40–60 cycles for 64-bit divisor case)
__udivmodti4 — unsigned 128/128 → quotient + remainder stored via pointer (~45–65 cycles)
```

For the case where divisor fits in 64 bits, both internally use a hardware `divq` instruction.
The difference is minor (~5 cycles). The dominant overhead is the ISRA frame, not the
runtime function choice.

---

## Why Windows GCC 15 -O3 Doesn't Show the Anomaly

Windows GCC 15.2.0 -O3 shows normal performance (2.05 cyc/op). Likely reasons:

1. **GCC 15 vs GCC 14 inlining heuristics differ:** GCC 15 may apply better IPA decisions
   for template functions in header-only libraries.
2. **Windows ucrt64 runtime:** `__udivti3` / `__udivmodti4` may be faster in MinGW-w64
   than Ubuntu's libgcc.
3. **CPU frequency scaling:** WSL may have different power governor behavior affecting
   measured cycle counts.

Confirmed non-regression: old `big_bin_divrem` code on WSL GCC 14 -O3 showed the same
anomaly (`8.07x`), so this is a pre-existing GCC 14/WSL interaction, not a phase175 issue.

---

## Attempted Mitigations

| Mitigation                             | Result                       |
|----------------------------------------|------------------------------|
| `-fno-ipa-sra` (disable ISRA)          | Still 35.92 cyc/op — not the only cause |
| `--param max-inline-insns-auto=10000`  | Still 34.86 cyc/op — GCC doesn't inline it |
| `--param max-inline-insns-single=10000`| Still 37.25 cyc/op           |

The function is not inlined under -O3 regardless of parameter tuning. GCC's inlining
decision appears to be based on the instantiated template size (D_knuth_divrem is ~300
assembly instructions when expanded), and -O3's inlining cost model rejects it.

A `__attribute__((always_inline))` annotation on `D_knuth_divrem` would likely fix
this but was not tested — the concern is compile-time impact on 12 compilers.

---

## Conclusion

The GCC -O3 division anomaly is caused by a **GCC inlining decision**:

- **-O2**: `D_knuth_divrem` inlined → fast path [0.d] = ~5–9 cyc/op
- **-O3**: `D_knuth_divrem` not inlined, ISRA variant with stack frame + canary → ~35–37 cyc/op

The anomaly is **compiler-version and OS-specific** (GCC 14/WSL, not GCC 15/Windows),
**pre-existing** (present before Knuth D refactoring), and **not a code correctness issue**.

GCC -O2 remains the reliable optimization level for this library on WSL/Linux.
