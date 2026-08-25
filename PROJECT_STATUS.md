# PROJECT STATUS: v1.90.1 — PUBLICADA (tag anotado, 24 ago 2026)

**Last Updated:** 25 August 2026

> Este documento es **la instantánea del estado actual**. No acumula historia:
> lo ya hecho vive en [`CHANGELOG.md`](CHANGELOG.md); lo que viene, en
> [`ROADMAP.md`](ROADMAP.md) y [`NEXT_STEPS.md`](NEXT_STEPS.md).
**Last Session:** Auditoría del proyecto y ejecución de las 8 fases del plan resultante (branch `phase-1.80`)
**Overall Progress:** v1.80 ✅ + v1.81 MS-INTEROP ✅ + v1.90 `fixed_int_t<N>` ✅ + **v1.90.1 auditoría ✅**
**Current Status:** 🚀 **suite completa 55/55** · `div`/`mod` constexpr · integración STL completa · 0 avisos de Doxygen desde `include/`

---

## Instantánea — 25 August 2026

### Compilación

| Comprobación | Resultado |
|---|---|
| `python make.py test gcc release-O2` | **55/55 ficheros** (162 s, GCC 16.2.0 ucrt64) |
| `scripts/check_headers_selfcontained.py` | **31/31** headers compilan aislados |
| Clang 22.1.8 **sin flags no estándar** | 31/31 headers + suite ✅ |
| MSVC 19.5x (VS 18) | tests tocados ✅ |
| `clang-format --dry-run --Werror` (árbol entero) | 0 ficheros sin formatear |
| `scripts/check_docs_consistency.py --doxygen` | **9/9** comprobaciones |
| Avisos de Doxygen desde `include/` | **0** (eran 48) |

### Cifras de la suite

| Test | Asserts |
|---|---|
| `test_fixed_basic` | 843 |
| `test_fixed_signed` | 966 |
| `test_fixed_vs_param` | 804 |
| `test_fixed_divmod` | 218 + 30 `static_assert` de constexpr |
| `test_cross_operators` | 206 |
| `test_fixed_string_io` | 104 |
| `test_fixed_stl_integration` | 95 |
| `test_fixed_differential` | **46.800 comprobaciones** contra oráculo independiente |
| `test_fixed_karatsuba` | 49 |

### Lo que cambió en v1.90.1

**Correctitud** — tres fallos que corrompían valores en silencio:

| Fallo | Antes | Ahora |
|---|---|---|
| `from_string` sin detección de desbordamiento | `from_string("2^256")` → `0` | `parse_error::overflow` / `std::out_of_range` |
| Constructor desde `float` con `inf` | UB (`static_cast<uint64_t>(NaN)`) | satura: NaN→0, +inf→`max()`, −inf→`min()` |
| Contador de desplazamiento truncado | `x << u256{2^64}` → `x` | satura a 64N → `0` |

**Objetivo de rama alcanzado:** `div` y `mod` son `constexpr`, sin regresión de
rendimiento (7 rondas intercaladas, mínimo por caso).

**Rendimiento:** eliminada `GM_TABLE`, que era código muerto y obligaba a
`-fconstexpr-steps=100000000` en todo Clang. Compilar un TU que incluya
`int128_param_divmod.hpp`: **2,4 s → 0,95 s**. Suite completa: ~230 s → ~162 s.

**API nueva:** `fixed_int_iostreams.hpp`, `fixed_int_format.hpp`,
`fixed_int_hash.hpp`, `to_string(base)` / `from_string(base)` en bases 2..36,
`try_from_string`.

**Ruptura:** `data` pasa a privado (`limb()` / `set_limb()` / `limbs()` /
`limbs_ref()`). Se deja decaer el uso como parámetro no-tipo de plantilla,
recuperando el comportamiento de phase-1.75.

**Infraestructura:** `LICENSE.txt` (no existía), SPDX en 31/31 headers,
`.clang-format`, `.clangd`, `toolchains.json`, tres verificadores nuevos en
`scripts/`, comandos `/proyecta` `/documenta` `/actualiza_doc`, CI con las
puertas cerradas y dos jobs nuevos, Docker en Ubuntu 24.04 con GCC 14 / Clang 19.

### Pendiente (decisiones del autor, no trabajo técnico)

- **T6.7** — consolidar los ~9.300 renglones de documentación de raíz repartidos
  en 10 ficheros solapados.
- **T7.6 (resto)** — mover a `scripts/archive/` los 32 scripts superados que
  identifica `scripts/README.md`.
- `CONTRIBUTING.md`, `SECURITY.md` y `ROADMAP.md` los lista `AI-GUIDE.md` pero no
  existen; están marcados como pendientes en su tabla.
- Cobertura Doxygen miembro a miembro de `fixed_width_int_t.hpp` (hoy: fichero,
  clase y los miembros con semántica no evidente).

---

## Compiler Validation Summary

### ✅ GCC 15.2.0 (-O2)

- Phase 1: 9/9 tests PASS ✅
- Phase 2: Benchmark framework SUCCESS ✅
- Phase 3: Knuth D - 55/55 PASS ✅ (0.47x vs uint64_t — faster than native!)
- Phase 4: 25/25 division tests PASS ✅
- Status: **FULLY VALIDATED**

### ✅ Clang 19.x (-O2)

- Phase 1: 9/9 tests PASS ✅
- Phase 3: Knuth D - 55/55 PASS ✅ (2.29x vs uint64_t)
- Phase 4: 25/25 division tests PASS ✅
- Status: **FULLY VALIDATED**

### ✅ MSVC 2026 (/O2)

- Phase 1: 9/9 tests PASS ✅
- Phase 3/4: Portable (uses big_bin_divrem fallback, no `__uint128_t`)
- Phase 6: 12/12 feature headers PASS ✅ (thread_safety N/A)
- Status: **FULLY VALIDATED (Phase 6 sweep)**

### ✅ Intel oneAPI (/O2)

- Phase 1: 9/9 tests PASS ✅
- Has `__int128` support, Knuth D works
- Phase 6: 12/12 feature headers PASS ✅ (thread_safety N/A)
- Status: **FULLY VALIDATED (Phase 6 sweep)**

## Test Suite Summary

### Complete Test Inventory (post-consolidation v1.77)

**test_param_* — Feature Tests:**

| File | Coverage |
|------|----------|
| test_param_algorithm.cpp | STL algorithm integration |
| test_param_arithmetic.cpp | Karatsuba, widening_mul, mulhi |
| test_param_array.cpp | Array/container usage |
| test_param_bits.cpp | Bit ops: popcount, clz, ctz, rotl, rotr |
| test_param_cmath.cpp | Math: abs, fma, isqrt |
| test_param_concepts.cpp | C++20 concepts |
| test_param_core_operators.cpp | Bitwise, shift, bitops |
| test_param_divmod.cpp | Division: /%%/=/%%=, Knuth D, GM div\<D\>/mod\<D\>/divmod_const\<D\> (75 tests) |
| test_param_ek.cpp | Excess-K representation |
| test_param_float.cpp | Float assignment and constructors |
| test_param_format.cpp | std::format full spec |
| test_param_friends.cpp | Friend operator interop |
| test_param_iostreams.cpp | Stream I/O |
| test_param_limits.cpp | std::numeric_limits |
| test_param_ms.cpp | Magnitude-Sign representation |
| test_param_numeric.cpp | gcd, lcm, midpoint |
| test_param_ranges.cpp | C++20 ranges |
| test_param_safe.cpp | Overflow-checked arithmetic (34 tests) |
| test_param_string_io.cpp | to_string / from_string all bases |
| test_param_thread_safety.cpp | Atomic / concurrent access |
| test_param_traits.cpp | STL type_traits specializations |

**test_sweep_* — Property-Based Sweep Tests (~588M+ value checks):**

| File | Tests |
|------|-------|
| test_sweep_arithmetic.cpp | Arithmetic invariants |
| test_sweep_bits.cpp | Bit operation properties |
| test_sweep_bitwise.cpp | Bitwise properties |
| test_sweep_comparison.cpp | Reflexivity, trichotomy, antisymmetry (11 tests) |
| test_sweep_division.cpp | q*d+r=n, r\<d, pow2 equiv (13 tests) |
| test_sweep_ek.cpp | EK: +, -, ++, -- (11 tests, 88M verifications) |
| test_sweep_framework_validation.cpp | Framework self-validation |
| test_sweep_ms.cpp | MS: +, -, *, /, ++, -- (15 tests, 133M verifications) — Added 16 May 2026 |
| test_sweep_shift.cpp | Identity, roundtrip, composition (16 tests) |
| test_sweep_string.cpp | Decimal/hex/octal/binary roundtrip (8 tests) |
| test_sweep_unary_ops.cpp | inc/dec, negation, bool (12 tests) |

**Other Tests:**

| File | Coverage |
|------|----------|
| test_coverage_all_bases.cpp | Cross-representation base coverage |
| test_intrinsics.cpp | Intrinsics abstraction layer |
| test_karatsuba.cpp | Karatsuba correctness |
| test_native_arithmetic.cpp | Native arithmetic operations |
| test_phase5_operators.cpp | Phase 5 operator suite |
| test_representation_conversions.cpp | Cross-repr conversions |
| test_template_type.cpp | Template type system |
| test_tostring_fast.cpp | GM-based to_string performance |
| test_traits_specializations.cpp | Traits specializations |

**Benchmarks (2 files):**

1. benchmark_divmod_algorithms.cpp — Knuth D vs Binary comparison
2. benchmark_vs_builtin.cpp — nstd vs uint64_t/int128/__int128/Boost

## Code Quality Metrics

- ✅ Compilation: 0 errors, 0 warnings
- ✅ Tests: 250+ passing across 56 files (12,158 lines of test code)
- ✅ Library: 24 headers, ~11,500 lines (main header: ~4,500 lines)
- ✅ Correctness: 100% verified
- ✅ Cross-platform: All Windows/Unix toolchains validated (11 compilers)
- ✅ Production status: Knuth D + GM constexpr division production-ready
- ✅ Documentation: 15 API reference docs (~300 public symbols)
