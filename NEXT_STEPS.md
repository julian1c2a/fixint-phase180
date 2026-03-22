# 🔮 NEXT STEPS - Post-Phase 1.75

**Status:** P1 ✅ | P2 ✅ | P3 ✅ KNUTH D | P4 ✅ | P5 ✅ | P6 13/13 ✅ | Intrinsics ✅ | Cross-Repr ✅ | API Docs ✅ | Karatsuba ✅ | Format ✅ | Hash ✅ | Benchmarks ✅ | Replanteamiento ✅
**Last Updated:** 22 July 2025
**Focus:** Replanteamiento complete — Next: optimize subtraction, MSVC/Intel benchmarks, sweep migration

---

## 🎯 COMPLETED PRIORITIES (session 5)

### ✅ Karatsuba API — COMPLETE (12/12 tests, ~57M verifications)

- `nstd::widening_mul(a,b)` → Full 128×128→256 Karatsuba (3 multiplications)
- `nstd::mulhi(a,b)` → Upper 128 bits of 256-bit product
- `nstd::mullo(a,b)` → Lower 128 bits (operator* alias)
- `nstd::uint256_t` → 256-bit result type (4 LE limbs)
- div_by_const.hpp: mulhi_128 migrated from schoolbook (4 muls) to Karatsuba (3 muls)

### ✅ std::format Full Standard Spec — COMPLETE (24/24 tests)

- Complete `[[fill]align][sign][#][0][width][type]` per C++20 standard
- Types: d, x/X, b/B, o | Fill/align/sign/alt (#)/zero-pad (0)/width

### ✅ std::hash STL Integration — COMPLETE (14 new assertions)

- All 4 types hashable in `std::unordered_map`, `std::unordered_set`
- Both `nstd::hash<T>` and `std::hash<T>` specialized
- Fixed: nstd::hash was invisible on Clang/libc++ (inside wrong preprocessor guard)

### ✅ Multicompiler Benchmarks — COMPLETE (5 benchmarks × 2 compilers)

**Headline:** nstd::uint128_t 19.8x faster than __int128 for division (GCC -O2)

- Full RDTSC cycle measurements across GCC 15 and Clang 21
- Results: `build/benchmark_results_multicompiler.md`

---

## 🎯 COMPLETED PRIORITIES

### ✅ Float/Double Assignment Operators - COMPLETE (25/25 tests)

### ✅ Type Traits Specializations - COMPLETE (35/35 tests)

### ✅ Phase 3: Knuth Algorithm D - COMPLETE (17 March 2026)

**Achievement:** D_knuth_divrem() with `__uint128_t` native division.

- **6.24x faster** than binary long division (7.17 → 1.15 ns/op)
- GCC-O2: **0.47x vs uint64_t** (faster than native 64-bit!)
- nstd **20x faster** than `__int128` for division
- 55/55 division tests passing (GCC + Clang)

---

## ✅ COMPLETED: Intrinsics Audit (18 March 2026)

**7 critical issues fixed:**

- `int128_param_bits.hpp`: 6 `__builtin_*` calls → `intrinsics::popcount64/clz64/ctz64`
- `int128_param_numeric.hpp`: Removed `detail::portable_clzll()`, replaced with `intrinsics::clz64()`
- All 12 feature headers validated on 11 compilers post-fix

---

## ✅ COMPLETED: Cross-Representation Operators (June 2026)

- Cross-repr copy/move constructors (binnat/TC/MS/EK ↔ binnat/TC/MS/EK)
- Cross-repr assignment operators
- Explicit conversion methods between all forms
- Built-in integral interop with all representation forms

## ✅ COMPLETED: API Reference Documentation (June 2026)

- 14 cppreference-style API docs covering ~280 public symbols
- Main class doc: `API_parameterized.md`
- 13 feature module docs (concepts, traits, limits, algorithm, bits, cmath, numeric, ranges, safe, thread_safety, iostreams, format, representation)

## ✅ COMPLETED: Granlund-Montgomery Division Plan (June 2026)

- `docs/PLAN_DIVMOD_CONSTEXPR.md` — Comprehensive plan for constexpr division by compile-time constants
- Analysis of 17 legacy headers in `legacy-code/divmod_by_constexpr/`

---

## ⏳ CURRENT PRIORITIES

### Implementation

Added 3 explicit assignment operators that delegate to existing constructors:

```cpp
int128_param_t& operator=(float value) noexcept {
    *this = int128_param_t{value};  // Reuses constructor with EK support
    return *this;
}

int128_param_t& operator=(double value) noexcept {
    *this = int128_param_t{value};
    return *this;
}

int128_param_t& operator=(long double value) noexcept {
    *this = int128_param_t{value};
    return *this;
}
```

### Test Results

**25/25 tests passing (100%)** ✅

- Float assignment: 6/6 ✅
- Double assignment: 6/6 ✅
- Long double assignment: 4/4 ✅
- Special values: 3/3 ✅
- Multiple assignments: 3/3 ✅
- Post-construction: 3/3 ✅

### Files Modified

- `include/int128_parameterized.hpp` (+39 lines)
- `tests/test_float_assignment.cpp` (new, 200 lines)

---

## ✅ PRIORITY 2: Type Traits Specializations - COMPLETE

### Implementation

Created comprehensive STL type traits integration:

**int128_param_traits_specializations.hpp** (~474 lines):

- Variadic macros for template types with commas
- 3 code generation macros: `NSTD_DEFINE_INT128_TRAITS`, `NSTD_DEFINE_INT128_ASSIGNABLE`, `NSTD_DEFINE_INT128_HASH`
- Specializations for **4 valid types**: `binnat` (unsigned), `TC`/`MS`/`EK` (signed)

**Traits implemented:**

- One-parameter: `is_integral`, `is_arithmetic`, `is_signed`, `is_unsigned`, 9 trivial traits
- Two-parameter: `is_trivially_assignable` (16 specializations)
- Conversions: `make_signed` / `make_unsigned` (binnat ↔ TC conversions)
- Hash: `nstd::hash<T>` for `std::unordered_map` support
- Helper variables: `_v` suffixes (C++17)
- Type aliases: `_t` suffixes

### Test Results

**35/35 tests passing (100%)** ✅

- Group 1: is_integral (4/4) ✅
- Group 2: is_signed/unsigned (4/4) ✅
- Group 3: is_arithmetic (4/4) ✅
- Group 4: Trivial properties (4/4) ✅
- Group 5: make_signed/unsigned (4/4) ✅
- Group 6: Hash (5/5) ✅ - includes std::unordered_map integration
- Group 7: Backward compatibility (6/6) ✅ - uint128_t/int128_t aliases
- Group 8: Builtin types (4/4) ✅ - nstd:: delegates to std::

### Files Created/Modified

- `include/int128_param_traits_specializations.hpp` (new, ~474 lines)
- `tests/test_traits_specializations.cpp` (rewritten, 215 lines, 35 tests)

### Design Clarification (CRITICAL)

**4 valid type combinations** (not 8 as initially assumed):

- `binnat` - unsigned only (binary natural, no sign encoding)
- `twos_complement` - signed only
- `magnitude_sign` - signed only
- `excess_k` - signed only

**Constraint:** `static_assert((Sign == unsigned_type) == (Form == binnat))` enforces this design

---

## ✅ PRIORITY A: Phase 5 - Additional Operators — COMPLETE

- Increment/decrement (++/--), unary minus for all representations
- 55/55 tests passing on 4 Windows compilers
- Multi-compiler validation complete

---

## ✅ PRIORITY B: Phase166 Feature Parity - 12/12 COMPLETE ✅

### Already Ported ✅

1. Constructors (integral, string, pair)
2. Arithmetic operators (+, -, *, /, %)
3. Comparison operators (<, >, <=, >=, ==, !=)
4. Bitwise operators (&, |, ^, ~)
5. Shift operators (<<, >>)
6. Friend operators (symmetric operations)
7. Helper methods (divmod, abs, swap)
8. Bit manipulation (trailing_zeros, leading_zeros, popcount, rotate)
9. Math functions (int128_param_cmath.hpp)
10. ✅ **Float/double/long double assignment operators** - COMPLETE
11. ✅ **Type traits specializations (nstd::)** - COMPLETE
12. Numeric limits (int128_param_limits.hpp)
13. Numeric algorithms (int128_param_numeric.hpp)
14. I/O streams (int128_param_iostreams.hpp)

### Header 1: ✅ int128_param_safe.hpp - COMPLETE (34/34 tests)

**Status:** ✅ **PRODUCTION READY**  
**Completion Date:** February 4, 2026  
**Time Spent:** ~3.5 hours (estimated 4h)

**Implementation:**

- 3 API styles: `checked_*`, `saturating_*`, `try_*`
- Overflow detection for +, -, *, /
- `checked_result<Sign, Form>` struct with `{value, overflow}`
- Full TC and unsigned support
- MS addition/subtraction working (multiplication blocked by operator*= issue)
- C++20 constexpr throughout

**Test Results:** 34/34 passing (100%) ✅

**Files Created:**

- `include/int128_param_safe.hpp` (380 lines)
- `tests/test_param_safe.cpp` (398 lines, 34 tests)
- `docs/archive/PRIORITY_3_HEADER_1_COMPLETION.md` (~780 lines)

**Files Modified:**

- `include/int128_parameterized.hpp` (+152 lines: max/min methods, divmod fix)

**Bugs Fixed:**

1. Missing max()/min() static methods (added +152 lines)
2. divmod() negates unsigned values (fixed with `if constexpr`)
3. checked_mul() infinite loop (division-based → sign-based)
4. Unsigned overflow detection too strict (AND → OR logic)

**Known Issues:**

- ✅ MS operator*= — FIXED (20 March 2026): zero×neg=-0 bug and magnitude overflow into sign bit
- ✅ EK arithmetic — VERIFIED: `*`, `/`, `%` and compound variants are `= delete` (compile-time error)

**Documentation:** See `docs/archive/PRIORITY_3_HEADER_1_COMPLETION.md` for full report

---

### Remaining Headers ✅ ALL COMPLETE

| Phase166 Header | Phase175 Target | Status |
|-----------------|-----------------|--------|
| `int128_base_safe.hpp` | `int128_param_safe.hpp` | ✅ 34/34 tests, 11 compilers |
| `int128_base_limits.hpp` | `int128_param_limits.hpp` | ✅ 34/34 tests, 11 compilers |
| `int128_base_format.hpp` | `int128_param_format.hpp` | ✅ 10/10 tests, 11 compilers |
| `int128_base_numeric.hpp` | `int128_param_numeric.hpp` | ✅ 11/11 tests, 11 compilers |
| `int128_base_algorithm.hpp` | `int128_param_algorithm.hpp` | ✅ 9/9 tests, 11 compilers |
| `int128_base_thread_safety.hpp` | `int128_param_thread_safety.hpp` | ✅ 43/43 tests, 9 compilers |
| `int128_base_concepts.hpp` | `int128_param_concepts.hpp` | ✅ 13/13 tests, 11 compilers |
| `int128_base_ranges.hpp` | `int128_param_ranges.hpp` | ✅ 13/13 tests, 11 compilers |
| `int128_base_bits.hpp` | `int128_param_bits.hpp` | ✅ 8/8 tests, 11 compilers |
| `int128_base_iostreams.hpp` | `int128_param_iostreams.hpp` | ✅ 28+OK tests, 11 compilers |
| `int128_base_traits.hpp` | `int128_param_traits.hpp` | ✅ 27/27 tests, 11 compilers |
| `int128_base_cmath.hpp` | `int128_param_cmath.hpp` | ✅ 8/8 tests, 11 compilers |

**Progress:** 12/12 complete (100%) ✅  
**Bugs fixed:** iostreams `uint128_tc_t` → `uint128_t`, numeric MSVC `portable_clzll()`

### Recommended Order

1. ✅ **safe** - DONE (overflow-checked arithmetic)
2. ✅ **limits** - DONE (`std::numeric_limits` specialization)
3. ✅ **format** - DONE (Modern C++20 formatting)
4. ✅ **numeric** - DONE (Additional numeric algorithms)
5. ✅ **algorithm** - DONE (STL integration)
6. ✅ **thread_safety** - DONE (Concurrent programming support)
7. ✅ **concepts** - DONE (C++20 concepts)
8. ✅ **ranges** - DONE (C++20 ranges support)

---

## ✅ PRIORITY C: Intrinsics Transplant — COMPLETE

- 5 intrinsics headers already ported and integrated
- Audit completed 18 March 2026: all `__builtin_*` calls unified
- 33 public functions across compiler_detection, arithmetic_operations, bit_operations, byte_operations, fallback_portable

---

## 📅 FUTURE WORK ITEMS

### NEW: Benchmark & Testing Methodology Overhaul

**Status:** ✅ Core Complete | **Priority:** High | **Docs:** `docs/PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md`

1. **RDTSC-only benchmarks:** ✅ Todos los benchmarks migrados a ciclos CPU directos (RDTSC).
   - ✅ `benchmark_vs_builtin.cpp` — usa RDTSC via `bench_common.hpp`.
   - ✅ `benchmark_divmod_algorithms.cpp` — migrado a `bench_common.hpp` (sesión 21 Mar 2026).
   - ✅ `bench_common.hpp` validado con 4 compiladores: GCC, Clang, MSVC, Intel (Crítica 5 resuelta).
2. **Cobertura sistemática 3-regiones:** ✅ Implementada.
   - ✅ `tests/test_sweep_framework.hpp` — Framework completo (SplitMix64, regiones, sweep_unary/binary).
   - ✅ `tests/test_sweep_framework_validation.cpp` — 20/20 PASS en 4 compiladores (Crítica 3 resuelta).
   - ⬜ Migrar tests existentes al framework de 3-regiones (trabajo futuro, incremental).
3. **Regla formalizada** en `AI_PROMPT/ai-instructions.md` como Regla 8 (Test & Benchmark Methodology).

### NEW: BCD Decimal Types (Base-10 Parameterized)

**Status:** Prototype Complete, Design Planned | **Priority:** Medium-High | **Docs:** `docs/PLAN_BCD_DECIMAL_TYPES.md`

Tipo parametrizado en base 10 con codificación BCD dentro de 128 bits (32 dígitos decimales):

- ✅ **Prototipo validado:** `build_temp/prototype_bcd_conversion.cpp` — Double-Dabble (bin→BCD)
  y Horner (BCD→bin) en 29/29 tests, 4 compiladores.
- **BCD Natural (8-4-2-1):** Para unsigned y Excess-K — aritmética decimal estándar.
- **BCD Aiken (2-4-2-1):** Para signed (TC, MS) — auto-complementario: `~d = 9-d`.
- **Proyección futura:** Base para punto flotante decimal (IEEE 754-2008 decimal128).
- **Casos de uso:** Aritmética financiera, protocolos telecom/banking, conversión string O(n).
- **Limitación descubierta:** BCD128 (32 nibbles) soporta max 10^32-1; uint128_t max (39 dígitos)
  desborda. Considerar BCD160 (40 nibbles) o packed BCD para cobertura completa.
- **Bug Clang:** Funciones BCD no deben ser `constexpr` — el evaluador constexpr de Clang
  produce resultados incorrectos en comparaciones de uint128_t ≥2^64. Usar `inline`.

### NEW: Granlund-Montgomery Division Optimization

**Status:** Planned | **Docs:** `docs/PLAN_DIVMOD_CONSTEXPR.md`

División constexpr por constantes en tiempo de compilación usando multiplicación recíproca.
17 headers legacy analizados en `legacy-code/divmod_by_constexpr/`.

### NEW: Multiplicación Karatsuba

**Status:** Planned | **Priority:** Medium | **Docs:** `docs/PLAN_BCD_DECIMAL_TYPES.md` §11

Algoritmo Karatsuba para multiplicación sub-cuadrática O(n^1.585). Necesario para:

- Tipos BCD más grandes (bcd256_t, bcd512_t) donde schoolbook O(n²) es costoso.
- Futuro `uint256_t` binario.
- Umbral schoolbook/Karatsuba a determinar empíricamente con benchmarks RDTSC.
- Plan detallado: header genérico `include/algorithms/karatsuba.hpp`.

---

## ~~🔴 CRÍTICAS ATACABLES~~ ✅ TODAS RESUELTAS (5/5)

Puntos débiles identificados — **todos resueltos en sesiones del 21-22 Jul 2025**:

### ~~Crítica 1: benchmark_divmod_algorithms.cpp aún usa std::chrono~~ ✅ RESUELTA

- **Problema:** El benchmark de algoritmos de división (`benchs/benchmark_divmod_algorithms.cpp`)
  medía con `std::chrono::high_resolution_clock`, no con RDTSC, incumpliendo la Regla 8.
- **Resolución (sesión 2025-07-21):** Migrado completamente a `#include "bench_common.hpp"`.
  Ahora usa `CycleTimer` + `doNotOptimize()`, 5M iteraciones, 10K warmup,
  reporte tabular con cyc/op + ratio vs baseline. Compilado limpio con GCC 15 y Clang 21.
  Resultado: Knuth D 1.92x más rápido que big_bin en promedio.

### ~~Crítica 2: No existe un test runner unificado~~ ✅ RESUELTA

- **Problema:** Los tests se ejecutan individualmente; no hay un único ejecutable
  o script que corra todos los tests y reporte pass/fail global.
- **Resolución (sesión 2025-07-21):** `python make.py test` ahora funciona como runner
  unificado. Compila y ejecuta todos los 49 test files automáticamente.
  Validado: 49/49 PASS con GCC 15, 49/49 PASS con Clang 21.

### ~~Crítica 3: Cobertura de tests no sigue 3-regiones sistemáticamente~~ ✅ RESUELTA

- **Problema:** Los tests existentes usan valores ad-hoc, no la cobertura de
  3 regiones × 2^21 valores descrita en la metodología.
- **Resolución (sesión 2025-07-22):** Creado `tests/test_sweep_framework.hpp` (~250 líneas)
  con SplitMix64 PRNG determinista, regiones first/last/random de 2^21 valores,
  `sweep_unary()` (6.3M valores) y `sweep_binary()` (12.6M pares + edge cases).
  Validación: `tests/test_sweep_framework_validation.cpp` — 20/20 PASS en los
  4 compiladores (GCC, Clang, MSVC, Intel ICX).

### ~~Crítica 4: Conversiones BCD ↔ binario no implementadas~~ ✅ RESUELTA

- **Problema:** El plan BCD documenta double-dabble e inverso multiplicativo,
  pero no hay implementación ni prototipo que valide la viabilidad constexpr.
- **Resolución (sesión 2025-07-22):** Creado `build_temp/prototype_bcd_conversion.cpp`
  (~370 líneas) con `bcd128_raw` struct, `double_dabble()` (bin→BCD) y
  `horner_bcd_to_binary()` (BCD→bin via Horner mul×10). 29/29 PASS en los
  4 compiladores. Funciones declaradas `inline` (no `constexpr`) debido a bug
  en evaluador constexpr de Clang con operaciones uint128_t ≥2^64.
  **Nota:** BCD128 (32 nibbles) soporta hasta 10^32-1 (32 dígitos); uint128_t max
  (39 dígitos) desborda el rango BCD.

### ~~Crítica 5: bench_common.hpp no se ha compilado con MSVC ni Intel~~ ✅ RESUELTA

- **Problema:** El header compartido de benchmarks se creó y refactorizó
  `benchmark_vs_builtin.cpp`, pero no se ha validado la compilación con los
  4 compiladores (GCC, Clang, MSVC, Intel).
- **Resolución (sesión 2025-07-21):** `bench_common.hpp` compilado y ejecutado
  exitosamente con los 4 compiladores: GCC 15.2.0 ✅, Clang 21.1.8 ✅,
  MSVC 19.50 ✅ (3 pragma warnings suprimidos), Intel ICX 2025.3 ✅.
  El header es totalmente cross-compiler.

---

### 1. Comparative Benchmarking — ✅ COMPLETE (19 March 2026, session 2)

All 9 compiler/mode combinations benchmarked:

- ✅ Win GCC 15.2.0 -O2/-O3: nstd division **0.49x/0.47x vs u64** (2x faster than native!)
- ✅ Win Clang 21.1.8 -O2/-O3: 2.26x/2.16x vs u64 (3x faster than `__int128`)
- ✅ Win MSVC /O2: **1.60x** vs u64 (improved from 1.74x — Knuth D fast paths via `_udiv128`)
- ✅ Win Intel ICX /O2: 3.26x vs u64
- ✅ WSL GCC 14.2.0 -O2/-O3: 2.06x/-O2 (GCC-O3 has pre-existing anomaly)
- ✅ WSL Clang 20.1.8 -O2/-O3: 2.30x/2.24x vs u64
- ✅ WSL Intel ICX 2025.3.2 -O2: **0.96x vs u64** (faster than native! 3.68x faster than `__int128`)
- Results documented: `docs/archive/COMPARATIVE_BENCHMARK_RESULTS.md`
- Phase166 vs phase175 regression analysis: `docs/archive/PHASE166_VS_PHASE175_REGRESSION_ANALYSIS.md`
- GCC -O3 WSL anomaly root cause: `docs/archive/GCC_O3_DIVISION_ANOMALY.md`

**Knuth D refactoring (session 2):** Exposed fast paths [0–3] for MSVC; added `_udiv128` for path [3].
All tests passing: GCC ✅ 30/30 + 25/25, Clang ✅ 30/30 + 25/25, MSVC ✅ 30/30 + 25/25.

### 2. Test Suite Strengthening

- Review test_priority*.cpp for weak assertions
- Add exact value checks instead of non-zero checks
- Add more edge cases for MS and EK representations

### 3. Known Issues: MS/EK

- **MS operator*=**: ✅ FIXED (20 March 2026) — Two bugs corrected:
  1. Zero × negative produced -0 instead of +0 (sign bit set on zero magnitude)
  2. Magnitude overflow into sign bit (bit 63) corrupted sign for large products
  - Fix: Clear sign bit after magnitude multiplication, only set if result non-zero
  - Tests: 13 MS multiplication tests + safe.hpp `ms_mul_mixed_signs` enabled
- **EK arithmetic**: ✅ VERIFIED (20 March 2026) — `*`, `*=`, `/`, `/=`, `%`, `%=` are `= delete` with `requires(is_excess_k)`. All 8 operator forms (including friend `operator*` with builtins) produce compile-time errors. Validated GCC + Clang.
- **Cross-representation casts (MS↔TC↔EK↔binnat)**: ✅ IMPLEMENTED (20 March 2026) — Explicit copy/move constructors between all valid `int128_param_t` instantiations. Uses `representation.hpp` conversion functions (TC as pivot). 72 signed round-trips + 15 unsigned round-trips validated. `test_casts_between_representations` enabled in `test_ms_storage.cpp`.
- See [OPERATOR_SEMANTICS.md](OPERATOR_SEMANTICS.md) for details

### 4. MSYS2 ucrt64 GCC Platform Issue (documented)

- `std::ofstream` inside a non-main function at -O1+ causes segfault on MSYS2 ucrt64 GCC 15.2.0
- Root cause: Windows C++ runtime initialization order with MSYS2 ucrt64 runtime
- Workaround: avoid `std::ofstream` in non-main functions in test files
- `-fno-inline` suppresses the crash but is not a fix

### 4. Phase 5 WSL Validation — ✅ COMPLETE

- 14 test files × 8 WSL compilers (GCC 13/14/15, Clang 18/19/20/21, Intel icpx 2025.3.2) — ALL PASS
- icpx WSL -O2 inlining bug in multi-arg `std::format` — workaround applied (split format calls)

---

## 🔄 REPLANTEAMIENTO — Sesión 5 (22 Julio 2025)

### Estado Actual del Proyecto

| Métrica | Valor |
|---------|-------|
| Headers (include/) | 23 (.hpp): 16 raíz + 5 intrinsics/ + 2 algorithms/ |
| Feature headers operativos | 13/13 |
| Tests (.cpp) | 58 archivos, 58/58 PASS (GCC + Clang) |
| API docs (docs/) | 15 archivos (14 previos + API_arithmetic.md) |
| Benchmarks (benchs/) | 5 archivos: vs_builtin, divmod_algorithms, to_string, from_string, granlund_montgomery |
| Compiladores validados | 12 (4 Windows MSYS2 + 8 WSL) |
| Representaciones | 4/4 (TC, MS, EK, binnat) |

### Análisis de Benchmarks — Fortalezas y Debilidades

**Fortalezas demostradas (nstd vs __int128):**

| Operación | GCC -O2 | Clang -O2 | Veredicto |
|-----------|---------|-----------|-----------|
| División (/) | **19.8x** más rápido | **1.4x** más rápido | Corona de la librería — Knuth D + fast paths |
| Comparación | **2.3x** más rápido | **3.6x** más rápido | Excelente en ambos compiladores |
| Suma (+) | **1.9x** más rápido | Paridad | GCC genera código más tight |
| Multiplicación (*) | Paridad | Paridad | Esperado — misma instrucción `mul` subyacente |
| to_string() | **1.3x** más rápido | **5.8x** más rápido | Algoritmo de pares de dígitos funciona bien |
| Shift | **1.7x** más rápido | Paridad | — |

**Debilidades identificadas:**

| Operación | GCC -O2 | Clang -O2 | Causa raíz |
|-----------|---------|-----------|------------|
| Resta (-) | 1.3x más lento | Paridad | GCC optimiza __int128 sub como instrucción nativa `sbb` |
| XOR | Paridad | 1.4x más lento | Clang optimiza __int128 XOR nativamente |

**Observaciones clave de compilador:**

- GCC produce aritmética simple ~5x más rápida que Clang (add: 1.19 vs 5.69 cyc/op)
- Clang produce string parsing ~40% más rápido que GCC
- Granlund-Montgomery es **esencial** en Clang: `operator/` es 10-60x más lento que en GCC
- Boost es 3-90x más lento que nstd en todas las operaciones básicas

### Inventario de Trabajo Futuro

Consolidación de todos los items pendientes de NEXT_STEPS y el plan original:

#### Prioridad ALTA (impacto directo en calidad/rendimiento)

| # | Item | Estado | Impacto |
|---|------|--------|---------|
| A1 | **Optimizar resta (GCC)** | Pendiente | Única operación donde __int128 gana; investigar inline asm/intrinsics |
| A2 | **Benchmarks MSVC + Intel** | Pendiente | Solo GCC/Clang benchmarkeados; MSVC/Intel sin ciclos medidos esta sesión |
| A3 | **Granlund-Montgomery constexpr completo** | Parcial | div_by_const.hpp usa Karatsuba; falta constexpr puro (sin __uint128_t) |
| A4 | **Migrar tests existentes a sweep framework** | Parcial | Sweep funciona; 58 tests legacy aún usan framework antiguo |

#### Prioridad MEDIA (extensión de funcionalidad)

| # | Item | Estado | Impacto |
|---|------|--------|---------|
| M1 | **BCD Decimal Types completos** | Prototipo | Funciones BCD↔binary existentes; falta tipo BCD128 completo con aritmética |
| M2 | **benchmark_comparison.bash** | Roto | `python make.py compare` falla; benchmarks se ejecutaron manualmente |
| M3 | **Test Suite Strengthening** | Pendiente | Revisar test_priority*.cpp para assertions débiles; agregar más edge cases |
| M4 | **Clang constexpr bug workaround** | Documentado | Funciones con uint128_t ops ≥2^64 producen comparaciones incorrectas si constexpr |

#### Prioridad BAJA (futuro/investigación)

| # | Item | Estado | Impacto |
|---|------|--------|---------|
| B1 | **ARM64/ARM32/RISC-V ports** | No iniciado | Requiere intrinsics nuevos; objetivo plan original |
| B2 | **Decimal128 (IEEE 754-2008)** | No iniciado | DPD/BID encoding; Phase 1.85+ |
| B3 | **int256_t / int512_t extensión** | No iniciado | uint256_t existe como struct básico; falta tipo completo |
| B4 | **Conan/vcpkg packaging** | No iniciado | conanfile.txt existe; falta publicación |

### Propuesta de Siguiente Sesión (Sesión 6)

**Enfoque recomendado:** Calidad + Rendimiento (antes de agregar funcionalidad nueva)

1. **Optimizar resta en GCC** (A1) — Cerrar la única brecha vs __int128
2. **Benchmarks MSVC + Intel** (A2) — Completar cobertura de 4 compiladores
3. **Fix benchmark_comparison.bash** (M2) — Automatizar comparaciones
4. **Migrar 5-10 tests a sweep framework** (A4) — Incrementar robustez
5. **Commit + tag v1.76** — Consolidar todo el trabajo de sesiones 4-6

### Decisiones Arquitectónicas Pendientes

1. **¿Merece la resta una instrucción inline ASM?** — Riesgo: romper portabilidad; beneficio: cerrar brecha con __int128
2. **¿Granlund-Montgomery debe ser siempre constexpr?** — Requiere eliminar dependencia de __uint128_t para mulhi
3. **¿BCD como phase 1.80 o postergar a 2.0?** — El prototipo funciona; ¿vale la inversión antes de ARM ports?
4. **¿uint256_t completo o solo como soporte interno?** — Actualmente solo struct de 4 limbs para Karatsuba

---

## 📈 SUCCESS METRICS

Phase 1.75 has achieved:

- ✅ **100% feature parity** with phase166 (12/12 headers)
- ✅ **Equal or superior performance** (Knuth D: 6.24x faster division)
- ✅ **3 complete representations** (TC, MS, EK)
- ✅ **Full STL integration** (traits, concepts, algorithms)
- ✅ **Production-ready** (safe arithmetic, thread safety)
- ✅ **Modern C++20** (format, ranges, concepts)
- ✅ **12-compiler validation** (4 Windows + 8 WSL)
- ✅ **Intrinsics unified** (cross-compiler abstraction layer)
- ✅ **14 API reference docs** (~280 public symbols documented)

## 📊 PROJECT PROJECTIONS

### Roadmap Estimado

| Phase | Contenido | Estado |
|-------|-----------|--------|
| **1.75** | Parameterized type (4 reprs), 13 feature headers, Knuth D, intrinsics, cross-repr, API docs, Karatsuba, format, hash | ✅ **COMPLETE** |
| **1.76** | Subtraction optimization, MSVC/Intel benchmarks, sweep migration, benchmark automation | 📋 NEXT SESSION |
| **1.80** | BCD base-10 types (Natural + Aiken), benchmark methodology overhaul, Granlund-Montgomery constexpr | 📋 PLANNED |
| **1.85** | Decimal128 floating point (IEEE 754-2008), DPD/BID encoding | 📋 FUTURE |
| **2.0** | Production release: full numeric tower (binary + decimal, integer + float) | 📋 FUTURE |

### Objetivos Pendientes (del plan original de 12)

- **Objetivo 9 (Etapa 9):** Diseño e implementación de tipos BCD → Phase 1.80
- **Objetivo 12:** Punto flotante decimal → Phase 1.85
- **Objetivo 10:** Benchmark methodology sistemática → Phase 1.80

### Métricas de Progreso Global

| Métrica | Actual | Objetivo 2.0 |
|---------|--------|-------------|
| Representaciones binarias | 4/4 ✅ | 4/4 |
| Feature headers (binario) | 12/12 ✅ | 12/12 |
| Tipos BCD | 0/3 | 3/3 (Natural, Aiken-TC, Aiken-MS) |
| Decimal float | 0/1 | 1/1 (decimal128) |
| Compiladores validados | 12 | 12+ |
| API docs coverage | ~280 symbols | 400+ |

---

## 📋 ARCHIVED CONTENT

Previous session plans, execution plans, and recommendations have been archived.
See `docs/archive/` for historical session documentation.

---

**Report generated:** 21 March 2026
