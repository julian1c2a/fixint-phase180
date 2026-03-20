# Phase 1.75 - Representation Forms Investigation

> **Status:** ✅ **ALL 6 PHASES COMPLETE — 12 feature headers, 11 compilers — 23/23 tests pass**
> **Started:** 11 January 2026
> **Last Updated:** 25 June 2026
> **Objective:** Investigate different number representations for IEEE 754 floating-point generalization  
> **Parent Project:** [int128-phase166](../int128-phase166/)

---

## 🎯 Phase 1.75 Overview

This parallel project investigates **representation forms** for 128-bit integers beyond the standard two's complement used in Phase 1.66. The goal is to provide a foundation for understanding and implementing IEEE 754 floating-point generalizations.

### Latest Achievements ✅

**All Phases Complete (18 March 2026):**

- **Phase 6:** 12/12 feature headers validated on 11 compilers (4 Windows + 7 WSL)
- **Intrinsics Audit:** All `__builtin_*` calls unified through intrinsics abstraction layer
- **Knuth Algorithm D:** 6.24x faster division, 0.47x vs uint64_t on GCC
- **Compilers:** GCC 13/14/15, Clang 18/19/20/21, MSVC 19.50, Intel ICX 2025.3.0
- Tests: **250+ PASS** across 56 test files
- **API Documentation:** 14 cppreference-style API docs (~280 public symbols)
- **Cross-Repr Operators:** Full interoperability between binnat/TC/MS/EK

**Completed Phases:**

| Phase | Description | Status | Key Result |
|-------|-------------|--------|------------|
| 1 | Multi-Compiler Validation | ✅ COMPLETE | 4/4 compilers, 9/9 tests each |
| 2 | Benchmarking Framework | ✅ COMPLETE | Baseline established |
| 3 | Knuth Algorithm D | ✅ COMPLETE | **6.24x speedup**, 55/55 tests |
| 4 | Division Operators | ✅ COMPLETE | /=, %=, /, % — 25/25 tests |
| 5 | Additional Operators | ✅ COMPLETE | ++, --, unary — 55/55 tests |
| 6 | Feature Parity (phase166) | ✅ COMPLETE | **12/12 headers**, 11 compilers |

### Representation Forms Under Investigation

| Form | Symbol | Use Case | Phase | Status |
|------|--------|----------|-------|--------|
| **Two's Complement** | TC | Standard integers (Phase 1.66 compat) | 1.66 | ✅ Stable |
| **Magnitude-Sign** | MS | General signed representation | 1.75 | ✅ Operational |
| **Excess-k (Bias)** | EK | IEEE 754 exponents | 1.75 | 📋 Partial |

---

## 📚 Architecture

### Parameterized Template Design

```cpp
template<signedness Sign, representation_form Form>
class int128_param_t { ... };

// Type aliases for each combination:
using int128_ms_t = int128_param_t<signed_type, magnitude_sign>;    // Magnitude-Sign
using int128_tc_t = int128_param_t<signed_type, twos_complement>;   // Two's Complement
using int128_ek_t = int128_param_t<signed_type, excess_k>;          // Excess-k (Bias)
```

### Key Design Decisions

✅ **Orthogonal Parameters:**

- `signedness`: independent of representation form
- `representation_form`: can be changed without affecting signedness semantics

✅ **Backward Compatibility:**

- Default aliases point to Phase 1.66 (two's complement)
- Phase 1.66 remains untouched and stable
- New representations are opt-in

✅ **Representation-Aware Operations:**

- Negation, magnitude extraction, sign checking are form-specific
- Comparison operators adapted per representation
- Conversions between representations provided

✅ **Knuth Algorithm D Division (Phase 3):**

- `D_knuth_divrem()` with 5 fast paths + MSVC fallback
- All division operators chain through: `op` → `divmod()` → `D_knuth_divrem()`
- GCC/Clang use `__uint128_t` native; MSVC uses binary long division fallback

### Core Implementation

**File:** `include/int128_parameterized.hpp` (4,345 lines, 170KB)
**Total library:** 22 headers, 10,689 lines

- 7 constructor variants + cross-representation copy/move constructors
- 6 comparison operators (cross-representation aware)
- 5 arithmetic operators (+ - * / %)
- 4 bitwise operators (& | ^ ~)
- 2 shift operators (<< >>)
- Division: Knuth Algorithm D with `__uint128_t` native support
- Cross-representation assignment operators and explicit conversion methods
- String I/O methods
- MS-specific utility methods
- 250+ tests passing across 56 test files (12,158 lines of test code)

---

## 📂 Directory Structure

```
int128-phase175/
├── include/                            # 22 headers, 10,689 lines
│   ├── int128_parameterized.hpp        # Main template (4,345 lines)
│   ├── int128_param_safe.hpp           # Overflow-checked arithmetic
│   ├── int128_param_traits_specializations.hpp  # STL type traits
│   ├── int128_param_cmath.hpp          # Math functions
│   ├── int128_param_limits.hpp         # numeric_limits
│   ├── int128_param_numeric.hpp        # Numeric algorithms
│   ├── int128_param_iostreams.hpp      # I/O streams
│   ├── int128_param_algorithm.hpp      # STL algorithm integration
│   ├── int128_param_bits.hpp           # Bit manipulation
│   ├── int128_param_concepts.hpp       # C++20 concepts
│   ├── int128_param_format.hpp         # C++20 std::format
│   ├── int128_param_ranges.hpp         # C++20 ranges
│   ├── int128_param_thread_safety.hpp  # Concurrent support
│   ├── representation.hpp              # Representation forms enum
│   └── intrinsics/                     # Cross-compiler low-level ops
│       ├── arithmetic_operations.hpp
│       ├── bit_operations.hpp
│       └── compiler_detection.hpp
├── docs/                               # 14 API docs + plan
│   ├── API_parameterized.md            # Main class API (~280 symbols)
│   ├── API_*.md                        # Feature module API (13 files)
│   └── PLAN_DIVMOD_CONSTEXPR.md        # Granlund-Montgomery plan
├── tests/                              # 56 test files, 12,158 lines
│   ├── test_priority[1-11]_*.cpp       # Core feature tests (172 tests)
│   ├── test_division_operators.cpp     # 25 division operator tests
│   ├── test_knuth_d_correctness.cpp    # 30 Knuth D tests
│   ├── test_param_*.cpp               # Feature module tests
│   └── ...
├── benchs/
│   ├── benchmark_divmod_algorithms.cpp # Knuth D vs Binary comparison
│   └── benchmark_vs_builtin.cpp        # nstd vs builtin/Boost
├── .github/
│   └── copilot-instructions.md         # AI agent instructions
└── build_temp/                         # Build artifacts
```

---

## 🔬 Research Focus Areas

### Phase 1.75.1 - Magnitude-Sign (Current)

**Objective:** Implement and validate magnitude-sign representation

**Key Characteristics:**

- Separate sign bit (MSB) and magnitude (127 bits unsigned)
- Range: [-2^127+1, 2^127-1]
- Both +0 and -0 exist (distinct encodings)
- No hardware optimization (purely software)

**Implementation Areas:**

1. Representation traits and conversion functions
2. Basic operations (negation, absolute value, sign extraction)
3. Arithmetic (add, subtract, multiply, divide)
4. Comparison operators (representation-aware)
5. String conversions

**Success Criteria:**

- [ ] All basic operations implemented
- [ ] Test suite: 100+ tests for magnitude-sign operations
- [ ] Comparison with two's complement (correct semantics)
- [ ] Documentation of representation differences
- [ ] Demo showing practical use cases

### Phase 1.75.2 - Excess-k / Bias Notation (Future)

**Objective:** Implement bias-based representation for exponents

**Key Characteristics:**

- Value = stored_bits - bias
- Typical bias: 2^(n-1) for n-bit field
- Used in IEEE 754 exponents
- All bits participate in value encoding

**Use Cases:**

- Floating-point exponent representation
- Self-adjoint matrix eigenvalues
- Robust statistics (biased medians)

### Phase 1.75.3 - IEEE 754 Floating Point Generalization (Future)

**Objective:** Build 128-bit floating-point types using representations

**Components:**

- Sign bit (magnitude-sign or two's complement)
- Exponent field (excess-k)
- Mantissa field (normalized or denormalized)

---

## 🛠️ Build & Test

### Compilers Supported

| Compiler | Version | Platform | `__int128` | Status |
|----------|---------|----------|-----------|--------|
| GCC | 15.2.0 | MSYS2/ucrt64 | ✅ Yes | ✅ Fully validated |
| Clang | 19.x | MSYS2/clang64 | ✅ Yes | ✅ Fully validated |
| MSVC | 2026 (v19.50) | Visual Studio 18 | ❌ No | ✅ Phase 1 validated |
| Intel ICX | 2025.3.0 | oneAPI | ✅ Yes | ✅ Phase 1 validated |

### Quick Start (GCC)

```bash
cd int128-phase175

# Compile a test
g++ -std=c++20 -O2 -Iinclude tests/test_knuth_d_correctness.cpp -o build_temp/test_knuth.exe
./build_temp/test_knuth.exe

# Compile benchmark (requires libgmp, libtommath)
g++ -std=c++20 -O2 -Iinclude benchs/benchmark_vs_builtin.cpp -lgmp -ltommath -o build_temp/bench.exe
./build_temp/bench.exe

# Run all priority tests
for f in tests/test_priority*.cpp; do
    g++ -std=c++20 -O2 -Iinclude "$f" -o build_temp/test.exe && ./build_temp/test.exe
done
```

### Using make.py (Recommended)

```bash
python make.py build uint128 bits tests gcc release
python make.py check uint128 bits gcc release
python make.py test  # All tests
```

---

## 📚 Documentation

### API Reference (14 cppreference-style docs, ~280 public symbols)

| Document | Coverage |
|----------|----------|
| [API_parameterized.md](docs/API_parameterized.md) | Main class: constructors, operators, conversions, division |
| [API_representation.md](docs/API_representation.md) | `representation_form` enum, traits, conversions |
| [API_concepts.md](docs/API_concepts.md) | C++20 concepts: `integral_param`, `arithmetic_param` |
| [API_traits.md](docs/API_traits.md) | STL type traits specializations |
| [API_limits.md](docs/API_limits.md) | `std::numeric_limits` specializations |
| [API_algorithm.md](docs/API_algorithm.md) | STL algorithm integration |
| [API_bits.md](docs/API_bits.md) | Bit manipulation: popcount, clz, ctz, rotl, rotr |
| [API_cmath.md](docs/API_cmath.md) | Math functions: abs, fma, isqrt |
| [API_numeric.md](docs/API_numeric.md) | Numeric algorithms: gcd, lcm, midpoint |
| [API_ranges.md](docs/API_ranges.md) | C++20 ranges integration |
| [API_safe.md](docs/API_safe.md) | Overflow-checked arithmetic |
| [API_thread_safety.md](docs/API_thread_safety.md) | Atomic operations, concurrent access |
| [API_iostreams.md](docs/API_iostreams.md) | Stream I/O operators |
| [API_format.md](docs/API_format.md) | C++20 `std::format` integration |

### Plans & Guides

| Document | Purpose |
|----------|---------|
| [PLAN_DIVMOD_CONSTEXPR.md](docs/PLAN_DIVMOD_CONSTEXPR.md) | Granlund-Montgomery constexpr division optimization plan |
| [REPRESENTATION_GUIDE.md](docs/REPRESENTATION_GUIDE.md) | Detailed explanation of each form |
| [MAGNITUDE_SIGN_TUTORIAL.md](docs/MAGNITUDE_SIGN_TUTORIAL.md) | Implementation guide |

### Source Files

| Document | Purpose |
|----------|---------|
| [representation.hpp](include/representation.hpp) | Representation forms enum & traits |
| [int128_parameterized.hpp](include/int128_parameterized.hpp) | Main template & type aliases |

### Example: Magnitude-Sign Basics

```cpp
#include "int128_parameterized.hpp"
using namespace nstd;

// Create magnitude-sign integers
int128_ms_t a(42);      // Positive: sign=0, magnitude=42
int128_ms_t b(-42);     // Negative: sign=1, magnitude=42

// Check representation
assert(a.is_negative() == false);
assert(b.is_negative() == true);
assert(a.magnitude() == b.magnitude());  // Same magnitude

// Arithmetic (to be implemented)
// int128_ms_t c = a + b;  // Should be zero (or -0 + 0)

// Conversions
std::string a_str = a.to_string();  // "42"
std::string b_str = b.to_string();  // "-42"
```

---

## 🔄 Relationship to Other Phases

```
Phase 1.66 (Stable)
    ↓ uses two's complement
    ├─ Phase 1.75 (Current - Representation Research)
    │   ├─ 1.75.1: Magnitude-Sign
    │   ├─ 1.75.2: Excess-k
    │   └─ 1.75.3: IEEE 754 Floating Point
    │
    └─ Phase 1.8 (Divisibility & Number Theory)
        ├─ 1.8.1: Divisibility operations
        ├─ 1.8.2: Modular arithmetic
        └─ 1.8.3: Magic numbers & p-adic
    
    └─ Phase 1.83 (Metaprogramming for N-width)
        ├─ Generalize concepts
        └─ Prepare for 256/512 bits
```

---

## 🚀 Benchmark Results

### Division Performance (Knuth D vs Binary)

| Test Case | Binary (ns) | Knuth D (ns) | Speedup |
|-----------|-------------|--------------|---------|
| Power-of-2 | ~7.0 | ~0.6 | **12x** |
| 64-bit values | ~7.0 | ~1.0 | **7x** |
| 128/64 hybrid | ~7.0 | ~1.1 | **6.4x** |
| Large 128/128 | ~7.0 | ~1.2 | **5.8x** |
| **Average** | **7.17** | **1.15** | **6.24x** |

### vs Builtin Types (GCC -O2)

| Type | Division Time vs uint64_t |
|------|--------------------------|
| nstd::uint128_t | **0.47x** (faster than native!) |
| unsigned __int128 | 9.56x |
| Boost cpp_int | ~50x |

---

## 🚀 Upcoming Milestones

### Completed ✅

- [x] **Phase 1:** Multi-compiler validation (GCC, Clang, MSVC, Intel)
- [x] **Phase 2:** Benchmarking framework + baseline results
- [x] **Phase 3:** Knuth Algorithm D (6.24x division speedup)
- [x] **Phase 4:** Division operators (/=, %=, /, %) — 25/25 tests
- [x] **Phase 5:** Additional operators (++, --, unary -) — 55/55 tests
- [x] **Phase 6:** Feature parity (12/12 headers) on 11 compilers
- [x] **Priority 1:** Float/double assignment operators — 25/25 tests
- [x] **Priority 2:** Type traits specializations — 35/35 tests
- [x] **safe.hpp:** Overflow-checked arithmetic — 34/34 tests
- [x] **limits.hpp:** std::numeric_limits specialization — 34/34 tests
- [x] **format.hpp:** C++20 std::format integration — 10/10 tests
- [x] **numeric.hpp:** Numeric algorithms — 11/11 tests
- [x] **algorithm.hpp:** STL algorithm integration — 9/9 tests
- [x] **thread_safety.hpp:** Concurrent programming support — 43/43 tests
- [x] **concepts.hpp + ranges.hpp:** C++20 concepts and ranges — 26/26 tests
- [x] **Intrinsics audit:** All `__builtin_*` unified through abstraction layer
- [x] **Cross-repr operators:** Assignment, copy/move constructors, conversion methods
- [x] **API Reference:** 14 cppreference-style docs (~280 public symbols)

### Next Up

- [ ] **Granlund-Montgomery constexpr division:** Fast division by compile-time constants (plan: `docs/PLAN_DIVMOD_CONSTEXPR.md`)
- [ ] **MS operator*=:** Multiplication for Magnitude-Sign representation
- [ ] **EK arithmetic:** Remaining `*`, `/`, `%` with bias adjustment
- [ ] **Phase 1.80:** N*64-bit generalization (arbitrary width integers)
- [ ] **CI/CD:** GitHub Actions pipeline

---

## 🔗 Linking to Phase 1.66

This project is **independent** of Phase 1.66:

✅ Does NOT modify Phase 1.66  
✅ Shares testing infrastructure  
✅ Can be built separately  
✅ Uses same CMake/Ninja build system  
✅ Compatible license (BSL-1.0)

To switch between projects:

```bash
# Build Phase 1.66 (two's complement, stable)
cd int128-phase166
mkdir build && cd build
cmake -G Ninja ..
cmake --build .

# Build Phase 1.75 (representations, research)
cd ../int128-phase175
mkdir build && cd build
cmake -G Ninja ..
cmake --build .
```

---

## 📊 Progress Tracking

### Core Implementation

| Component | Status | Tests | Comments |
|-----------|--------|-------|----------|
| Representation enum | ✅ Complete | — | `representation_form` defined |
| Traits system | ✅ Complete | 35/35 | `representation_traits<>` + STL traits |
| Main template | ✅ Complete | — | `int128_param_t<>` (4,345 lines) |
| Constructors | ✅ Complete | 20/20 | All 7 variants |
| Accessors | ✅ Complete | — | `high()`, `low()`, `sign()` |
| Arithmetic | ✅ Complete | 24/24 | +, -, *, /, % — all with Knuth D |
| Comparisons | ✅ Complete | — | ==, !=, <, >, <=, >= |
| Bitwise | ✅ Complete | 24/24 | &, |, ^, ~ |
| Shifts | ✅ Complete | 28/28 | <<, >> |
| Division (Knuth D) | ✅ Complete | 55/55 | 6.24x faster than binary |
| String I/O | ✅ Complete | 41/41 | `to_string()`, parsing |
| Float assignment | ✅ Complete | 25/25 | float, double, long double |
| Safe arithmetic | ✅ Complete | 34/34 | checked, saturating, try |
| Benchmarks | ✅ Complete | — | Knuth D + vs-builtin |

### Feature Headers (Phase 6) — ALL COMPLETE ✅

| Header | Status | Tests |
|--------|--------|-------|
| int128_param_safe.hpp | ✅ Complete | 37/37 |
| int128_param_limits.hpp | ✅ Complete | 34/34 |
| int128_param_bits.hpp | ✅ Complete | 8/8 |
| int128_param_cmath.hpp | ✅ Complete | 8/8 |
| int128_param_iostreams.hpp | ✅ Complete | 28+OK |
| int128_param_traits.hpp | ✅ Complete | 27/27 |
| int128_param_format.hpp | ✅ Complete | 10/10 |
| int128_param_numeric.hpp | ✅ Complete | 11/11 |
| int128_param_algorithm.hpp | ✅ Complete | 9/9 |
| int128_param_concepts.hpp | ✅ Complete | 13/13 |
| int128_param_ranges.hpp | ✅ Complete | 13/13 |
| int128_param_thread_safety.hpp | ✅ Complete | 43/43 |

---

## 🎯 Evaluación de Objetivos del Proyecto

Progreso evaluado contra los 12 objetivos y 9 etapas definidos en [`AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`](AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md):

### Objetivos (12 definidos)

| # | Objetivo | Estado | Notas |
|---|----------|--------|-------|
| 1 | Tipo entero 128-bit similar a nativos | ✅ Conseguido | `int128_param_t` con operadores completos, 0.47x vs uint64_t |
| 2 | Portabilidad absoluta | ✅ Conseguido | 11 compiladores (GCC 13-15, Clang 18-21, MSVC 2026, Intel ICX). ARM/RISC-V pendiente |
| 3 | Sentirse parte del lenguaje C++ | ✅ Conseguido | STL traits, numeric_limits, concepts, ranges, format, hash |
| 4 | Compiladores + CI/CD | 🔄 Parcial | 11 compiladores validados. GitHub Actions pipeline pendiente |
| 5 | Sanitizers + análisis estático | 🔄 Parcial | ASan/UBSan integrados. cppcheck listo. Clang-Tidy, GCov, LCOV pendientes |
| 6 | Tests unitarios + benchmarks | ✅ Conseguido | 250+ tests (56 archivos, 12K líneas), benchmarks vs __int128 y Boost |
| 7 | Estructura clara y organizada | ✅ Conseguido | include/, tests/, benchs/, docs/, scripts/, debugging/ |
| 8 | Build system con scripts Python | ✅ Conseguido | make.py → CMake → Ninja. CI/CD script pendiente |
| 9 | Fácil de usar, compatible STL | ✅ Conseguido | Todos los operadores, conversiones, integración completa |
| 10 | Base para precisión arbitraria | 🔄 En progreso | Template parametrizado Sign+Form. M&S operativo, EK parcial |
| 11 | Algoritmos optimizados documentados | ✅ Conseguido | Knuth D (6.24x), plan Granlund-Montgomery documentado |
| 12 | Tipos decimales BCD | ⬜ No iniciado | Previsto para etapa posterior |

**Resumen:** 8/12 conseguidos, 3/12 parcial, 1/12 no iniciado.

### Etapas (9 definidas)

| # | Etapa | Estado | Notas |
|---|-------|--------|-------|
| 1 | Implementación básica int128/uint128 | ✅ Terminado | |
| 2 | Unificación template | ✅ Terminado | `int128_param_t<Sign, Form>` |
| 3 | Representaciones M&S y Exceso-K | 🔄 En progreso | M&S operativo con operadores cross-repr; EK parcial |
| 4 | Arrays N×64 bits (fase 1.80) | ⬜ No iniciado | |
| 5 | Punto fijo configurable | ⬜ No iniciado | |
| 6 | IEEE 754 Generalizado | ⬜ No iniciado | |
| 7 | Big integers (longitud arbitraria) | ⬜ No iniciado | |
| 8 | Racionales exactos | ⬜ No iniciado | |
| 9 | Tipos decimales BCD | ⬜ No iniciado | |

**Resumen:** Etapas 1-2 terminadas, Etapa 3 en progreso avanzado, Etapas 4-9 pendientes.

---

## 📝 Contributing

This is a research phase. To contribute:

1. Follow C++20 standards (same as Phase 1.66)
2. Use `nstd::` namespace exclusively
3. Add comprehensive Doxygen documentation
4. Include test cases for every feature
5. Add benchmarks comparing representations

See Phase 1.66's [PROMPT.md](../int128-phase166/PROMPT.md) for detailed code guidelines.

---

## 📄 License

Boost Software License 1.0 (BSL-1.0)  
Copyright © 2024-2026 Julián Calderón Almendros

---

**Last Updated:** 25 June 2026  
**Phase Lead:** int128 Project Contributors  
**Status:** 🔬 Active Research
