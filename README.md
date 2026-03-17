# Phase 1.75 - Representation Forms Investigation

> **Status:** ✅ **KNUTH ALGORITHM D COMPLETE — 6-20x Division Speedup**  
> **Started:** 11 January 2026  
> **Last Updated:** 17 March 2026
> **Objective:** Investigate different number representations for IEEE 754 floating-point generalization  
> **Parent Project:** [int128-phase166](../int128-phase166/)

---

## 🎯 Phase 1.75 Overview

This parallel project investigates **representation forms** for 128-bit integers beyond the standard two's complement used in Phase 1.66. The goal is to provide a foundation for understanding and implementing IEEE 754 floating-point generalizations.

### Latest Achievements ✅

**Knuth Algorithm D Division (17 March 2026):**

- D_knuth_divrem() with `__uint128_t` native division for GCC/Clang
- 5-level fast path cascade: power-of-2, 64/64, 128/64, 128/128, fallback
- **6.24x faster** than binary long division (7.17 → 1.15 ns/op)
- GCC-O2: **0.47x vs uint64_t** (faster than native 64-bit division!)
- nstd **20x faster** than compiler `__int128` for division
- Tests: **55/55 PASS** (GCC + Clang)

**Completed Phases:**

| Phase | Description | Status | Key Result |
|-------|-------------|--------|------------|
| 1 | Multi-Compiler Validation | ✅ COMPLETE | 4/4 compilers, 9/9 tests each |
| 2 | Benchmarking Framework | ✅ COMPLETE | Baseline established |
| 3 | Knuth Algorithm D | ✅ COMPLETE | **6.24x speedup**, 55/55 tests |
| 4 | Division Operators | ✅ COMPLETE | /=, %=, /, % — 25/25 tests |
| 5 | Additional Operators | ⏳ TODO | ++, --, unary operators |
| 6 | Feature Parity (phase166) | ⏳ 1/7 | safe.hpp done (34/34 tests) |

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

**File:** `include/int128_parameterized.hpp` (3,534 lines)

- 7 constructor variants
- 6 comparison operators
- 5 arithmetic operators (+ - * / %)
- 4 bitwise operators (& | ^ ~)
- 2 shift operators (<< >>)
- Division: Knuth Algorithm D with `__uint128_t` native support
- String I/O methods
- MS-specific utility methods
- 200+ tests passing across 44 test files

---

## 📂 Directory Structure

```
int128-phase175/
├── include/
│   ├── int128_parameterized.hpp        # Main template (3,534 lines)
│   ├── int128_param_safe.hpp           # Overflow-checked arithmetic
│   ├── int128_param_traits_specializations.hpp  # STL type traits
│   ├── int128_param_cmath.hpp          # Math functions
│   ├── int128_param_limits.hpp         # numeric_limits
│   ├── int128_param_numeric.hpp        # Numeric algorithms
│   ├── int128_param_iostreams.hpp      # I/O streams
│   ├── representation.hpp              # Representation forms enum
│   └── intrinsics/                     # Cross-compiler low-level ops
│       ├── arithmetic_operations.hpp
│       ├── bit_operations.hpp
│       └── compiler_detection.hpp
├── tests/                              # 44 test files
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

### Key Files

| Document | Purpose |
|----------|---------|
| [representation.hpp](include/representation.hpp) | Representation forms enum & traits |
| [int128_parameterized.hpp](include/int128_parameterized.hpp) | Main template & type aliases |
| [REPRESENTATION_GUIDE.md](docs/REPRESENTATION_GUIDE.md) | Detailed explanation of each form |
| [MAGNITUDE_SIGN_TUTORIAL.md](docs/MAGNITUDE_SIGN_TUTORIAL.md) | Implementation guide |

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
- [x] **Priority 1:** Float/double assignment operators — 25/25 tests
- [x] **Priority 2:** Type traits specializations — 35/35 tests
- [x] **safe.hpp:** Overflow-checked arithmetic — 34/34 tests

### Next Up

- [ ] **Phase 5:** Additional operators (++, --, unary -)
- [ ] **limits.hpp:** std::numeric_limits specialization
- [ ] **format.hpp:** C++20 std::format integration
- [ ] **numeric.hpp:** Additional numeric algorithms
- [ ] **algorithm.hpp:** STL algorithm integration
- [ ] **thread_safety.hpp:** Concurrent programming support
- [ ] **concepts.hpp + ranges.hpp:** C++20 concepts and ranges
- [ ] **Intrinsics transplant:** Cross-compiler optimization layer

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
| Main template | ✅ Complete | — | `int128_param_t<>` (3,534 lines) |
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

### Feature Headers (Phase 6)

| Header | Status | Tests |
|--------|--------|-------|
| int128_param_safe.hpp | ✅ Complete | 34/34 |
| int128_param_limits.hpp | ⏳ TODO | — |
| int128_param_format.hpp | ⏳ TODO | — |
| int128_param_numeric.hpp | ⏳ TODO | — |
| int128_param_algorithm.hpp | ⏳ TODO | — |
| int128_param_thread_safety.hpp | ⏳ TODO | — |
| int128_param_concepts.hpp | ⏳ TODO | — |
| int128_param_ranges.hpp | ⏳ TODO | — |

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

**Last Updated:** 17 March 2026  
**Phase Lead:** int128 Project Contributors  
**Status:** 🔬 Active Research
