# AI Agent Instructions for int128 Project

## ⚠️ CRITICAL RULES

### 0. Compiler Paths (CRITICAL - DO NOT CHANGE)
**Visual Studio 2026 uses version "18", NOT "2022"!**
- **MSVC:** `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\`
- **vcvarsall.bat:** `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat`
- **Intel oneAPI:** `C:\Program Files (x86)\Intel\oneAPI\` (depends on MSVC)
- **GCC:** `/c/msys64/ucrt64/bin/g++`
- **Clang:** `/c/msys64/clang64/bin/clang++`

### 1. Keep Root Directory CLEAN
**NEVER create in project root:** `*.exe`, `*.obj`, `*.pdb`, `*.o`  
**Use instead:** `build_temp/`, `build/build_[tests|benchs]/[compiler]/[mode]/`

### 2. ASCII-Only Console Output
All C++ console output must use ASCII only:
- NO Unicode box drawing → Use `+=-|`
- NO Unicode checkmarks → Use `[OK]` and `[FAIL]`
- Run `python fix_unicode_chars.py` to clean files

### 3. Documentation Updates
- **CHANGELOG.md**: Update hourly during active development
- **PROMPT.md**: Reference for all build conventions and naming patterns

### 4. Interactive Work Mode
**Report status every ~3 minutes** during active development:
- Brief summary of progress
- Any issues encountered
- Next steps planned
- Wait for user feedback before major changes

**User compiles in MSYS2 terminal** and reports logs back. Do NOT run long compilations autonomously.

### 5. std::byte for Byte Arrays (MANDATORY)
**ALWAYS use std::byte for byte buffers, NEVER uint8_t:**

```cpp
// ✅ CORRECTO:
std::array<std::byte, 16> to_bytearray() const;
static int128_base_t from_bytearray(const std::array<std::byte, N>&);

// ❌ INCORRECTO:
std::array<uint8_t, 16> to_bytearray() const;  // NO usar uint8_t
```

**Rationale:**
- `std::byte` representa datos sin interpretar (C++17 standard)
- Previene operaciones aritméticas accidentales
- Type safety superior (require conversiones explícitas)
- Semántica correcta para serialización y buffers

**Conversiones:**
```cpp
// uint8_t → std::byte
std::byte b = static_cast<std::byte>(0xFF);

// std::byte → uint8_t
uint8_t u8 = std::to_integer<uint8_t>(b);

// ❌ NUNCA usar reinterpret_cast con rvalues:
std::byte b = reinterpret_cast<std::byte>(0xFF);  // ERROR
```

**Documentación:** [STDBYTE_MIGRATION_GUIDE.md](../STDBYTE_MIGRATION_GUIDE.md)

### 6. Byteswap Utilities for Endianness
**Use built-in byteswap functions, NEVER manual byte reversal:**

```cpp
// ✅ CORRECTO - Usar funciones byteswap:
auto network_value = value.to_big_endian();          // Para TCP/UDP
auto host_value = uint128_t::from_big_endian(bytes); // Desde red

// ✅ CORRECTO - Acceso a bytes individuales:
std::byte lsb = value.get_byte(0);                   // LSB (índice 0)
value.set_byte(15, std::byte{0xFF});                 // MSB (índice 15)

// ❌ INCORRECTO - Manual byte reversal:
for (int i = 0; i < 8; i++) {
    std::swap(data[i], data[15-i]);  // NO hacer esto manualmente
}
```

**7 funciones disponibles:**
- `byteswap()` - Invierte orden de bytes
- `to_big_endian()` / `from_big_endian()` - Network byte order
- `to_little_endian()` / `from_little_endian()` - Host byte order
- `get_byte(index)` / `set_byte(index, value)` - Acceso individual

**Casos de uso:**
- Serialización de red (TCP/UDP) → `to_big_endian()`
- Lectura de disco → `from_little_endian()`
- Protocolos binarios → `get_byte()` / `set_byte()`

**Documentación:** [API_INT128_BASE_TT.md](../API_INT128_BASE_TT.md#operaciones-con-bytes-y-endianness)

### 7. Build Workflow Hierarchy (MANDATORY) 🔴

**⚠️ CRITICAL - El flujo de trabajo de 4 capas es NO NEGOCIABLE. Nunca compiles manualmente.**

La biblioteca int128 implementa un sistema de compilación jerárquico que DEBE respetarse:

```
┌─────────────────────────────────────────────────┐
│ Layer 1: Python Scripts (run_tests.py, ...)     │ ← Usuario invoca aquí
├─────────────────────────────────────────────────┤
│ Layer 2: make.py (orchestración)                │ ← Selecciona compilador/modo
├─────────────────────────────────────────────────┤
│ Layer 3: CMake (detección, configuración)       │ ← Genera sistema de build
├─────────────────────────────────────────────────┤
│ Layer 4: Ninja (compilación paralela)           │ ← Ejecuta compilación
└─────────────────────────────────────────────────┘
```

**3 REGLAS FUNDAMENTALES (NON-NEGOTIABLE):**

1. ❌ **NUNCA compilar manualmente:**
   ```bash
   # ❌ PROHIBIDO:
   g++ -std=c++20 -Iinclude tests/int128_bits_extracted_tests.cpp -o test.exe
   clang++ -std=c++20 ...
   icx -std=c++20 ...
   
   # ✅ CORRECTO:
   python make.py build uint128 bits tests gcc release
   python make.py check uint128 bits gcc release
   ```

2. ❌ **NUNCA saltar capas del workflow:**
   ```bash
   # ❌ PROHIBIDO - Usar CMake directamente:
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ninja
   
   # ❌ PROHIBIDO - Usar Ninja directamente:
   ninja -C build tests
   
   # ✅ CORRECTO - Respetar jerarquía:
   python make.py build uint128 bits tests gcc release
   # Internamente: Python → make.py → CMake → Ninja
   ```

3. 📝 **Documentar excepciones obligatoriamente:**
   - Si DEBES compilar manualmente por debugging, documenta en [DEBUG] tag:
     ```bash
     # [DEBUG] - Bypassing workflow due to: <reason>
     # CMake error related to: <issue>
     # Temporary workaround: manual compilation
     g++ -std=c++20 ...
     ```
   - Excepciones permitidas (SOLO con documentación):
     - Debug de problemas de CMake (requiere [DEBUG] tag)
     - Testing de nuevos compiladores (requiere [DEBUG] tag)
     - Investigación de errores de libreía (requiere [DEBUG] tag)

**JERARQUÍA DETALLADA:**

| Capa | Herramienta | Responsabilidad | Invocación |
|------|-------------|-----------------|-----------|
| **1** | Python scripts | Interface para usuario | `python make.py ...` |
| **2** | make.py | Orquestación, selección compilador | Invoca CMake automáticamente |
| **3** | CMake | Detección compilador, configuración | Genera archivo build.ninja |
| **4** | Ninja | Compilación paralela | Ejecuta automaticamente |

**EJEMPLOS CORRECTOS vs INCORRECTOS:**

✅ **CORRECTO - Desarrollar característica:**
```bash
# 1. Editar código
vim include/int128_base_tt.hpp

# 2. Compilar con workflow
python make.py build uint128 bits tests gcc release

# 3. Ejecutar tests
python make.py check uint128 bits gcc release

# 4. Benchmarkear si necesario
python make.py run uint128 bits gcc release
```

❌ **INCORRECTO - Compilación manual:**
```bash
g++ -std=c++20 -Iinclude tests/int128_bits_extracted_tests.cpp -o test
./test
```

✅ **CORRECTO - WSL con detección automática:**
```bash
# El workflow automáticamente detecta WSL y aplica flags correctos
python make.py build uint128 bits tests gcc release
# Salida incluye: [INT128_PLATFORM=wsl-gcc-15] en mensaje de compilación
```

❌ **INCORRECTO - Compilación manual en WSL:**
```bash
# NO hagas esto - CMake no se ejecuta, no se detecta WSL
g++ -std=c++20 ...
```

✅ **CORRECTO - Sanitizers integrados:**
```bash
# El workflow configura ASan/UBSan automáticamente
python make.py build uint128 bits tests gcc debug
# CMake configura: -fsanitize=address -fsanitize=undefined
```

**REFERENCIA COMPLETA:**

Ver [WORKFLOW_OBLIGATORIO.md](../WORKFLOW_OBLIGATORIO.md) para:
- Explicación completa de las 4 capas
- 5 ejemplos de workflow completos
- Guía de troubleshooting
- Integración CI/CD con GitHub Actions
- Justificación de por qué este workflow existe

---

## 📜 License Header (MANDATORY)

**ALL headers (.hpp) and library source files (.cpp) MUST include this header:**

```cpp
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
// 
// Copyright (c) 2024-2026 Julián Calderón Almendros
// Email: julian.calderon.almendros@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
// @file       filename.hpp
// @brief      Brief description of the file
// @author     Julián Calderón Almendros
// @date       YYYY-MM-DD (last edit)
// @version    1.0.0
// =============================================================================
```

**For test files and demos**, use a simplified header:
```cpp
// =============================================================================
// Test/Demo: brief description
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================
```

---

## 📚 Documentation Standards (Doxygen)

### For Library Users
Document public API with:
```cpp
/**
 * @brief One-line description of function/class
 * 
 * @details Longer explanation if needed. Explain behavior,
 *          edge cases, and any important notes.
 * 
 * @param[in]  param1  Description of input parameter
 * @param[out] param2  Description of output parameter
 * @return Description of return value
 * 
 * @throws None (noexcept) OR list exceptions
 * 
 * @note Any important notes for users
 * @see Related functions or classes
 * 
 * @code
 * // Usage example
 * const uint128_t x{42};
 * const auto result = x.to_string();
 * @endcode
 */
```

### For Maintainers/Contributors
Add implementation notes with:
```cpp
/**
 * @internal
 * @brief Implementation detail - not for public use
 * 
 * @par Implementation Notes:
 * - Why this approach was chosen
 * - Performance considerations
 * - Platform-specific behavior
 * 
 * @par Complexity:
 * - Time: O(n)
 * - Space: O(1)
 */
```

### File-Level Documentation
Every header must have:
```cpp
/**
 * @file int128_base_bits.hpp
 * @brief Bit manipulation functions for 128-bit integers
 * 
 * @details This module provides:
 * - popcount, countl_zero, countr_zero
 * - rotl, rotr (rotation)
 * - bit_width, has_single_bit
 * 
 * @par Dependencies:
 * - int128_base_tt.hpp (core type)
 * - <bit> (C++20 bit operations)
 * 
 * @par Example:
 * @code
 * #include "int128_base_bits.hpp"
 * const uint128_t x{0xFF00};
 * const int zeros = nstd::countl_zero(x);  // 112
 * @endcode
 */
```

---

## 📖 API Documentation Files (API_*.md)

Maintain **cppreference-style** documentation in `API_[header].md`:

### Structure Template:
````markdown
# API Reference - [Header Name]

## Synopsis
```cpp
namespace nstd {
    // Declarations with syntax highlighting
    constexpr int popcount(uint128_t x) noexcept;
}
```

## Functions

### popcount
```cpp
constexpr int popcount(uint128_t x) noexcept;
```
**Brief:** Counts the number of 1-bits in x.

**Parameters:**
- `x` - Value to examine

**Return value:** Number of 1-bits

**Example:**
```cpp
const uint128_t val{0b11110000};
assert(nstd::popcount(val) == 4);
```
````

---

## 🎭 Demo Categories

Demos serve different purposes - maintain all categories:

| Category | Purpose | Audience |
|----------|---------|----------|
| **tutorials/** | Step-by-step learning | Beginners |
| **examples/** | Real-world use cases | Users |
| **showcase/** | Advanced features | Power users |
| **comparison/** | vs other libraries | Evaluators |
| **performance/** | Benchmark demos | Performance-focused |
| **integration/** | With other libs | Integrators |

---

## Coding Standards

### 1. Const Correctness (MANDATORY)
**Every variable that CAN be const MUST be declared const:**
```cpp
// CORRECT
const uint64_t mask = 0xFFFFFFFFFFFFFFFFull;
const auto result = compute_value();
for (const auto& item : container) { ... }

// WRONG - missing const
uint64_t mask = 0xFFFFFFFFFFFFFFFFull;  // Should be const
auto result = compute_value();           // Should be const
for (auto& item : container) { ... }     // Should be const& if not modified
```

### 2. Initialize Variables at Declaration (BRACE INITIALIZATION)
**Variables MUST be initialized when declared using brace initialization `{}`:**

> **📋 RULE:** Always use `{}` for initialization, never `()` or `=`.
> This is mandatory for consistency and to work with explicit constructors.

```cpp
// CORRECT - initialize at declaration with braces
const uint64_t value{compute_something()};
const auto result{(condition) ? option_a : option_b};
const int128_t x{42};
const uint128_t big{0xDEADBEEF};
const auto pair{std::make_pair(a, b)};
std::vector<int> vec{1, 2, 3, 4, 5};

// CORRECT - function arguments with braces
process_value(uint128_t{42});
return int128_t{result};

// CORRECT - member initialization in constructors
MyClass() : member{0}, other{compute()} {}

// WRONG - declare then assign
uint64_t value;       // Uninitialized!
value = compute_something();

// WRONG - parentheses instead of braces
const uint64_t value(42);  // Use braces: {42}

// WRONG - = initialization (fails with explicit constructors)
const uint128_t x = 42;    // ERROR with explicit ctors!
const auto result = 42;    // Prefer braces: {42}
```

### 3. Constexpr/Consteval Everywhere
**All functions/methods should be `constexpr` when possible, `consteval` when appropriate:**
```cpp
// CORRECT
template <integral_builtin T>
constexpr uint64_t high() const noexcept { return data[1]; }

static consteval int128_base_t max() noexcept { ... }
```

### 4. Error Handling - No Exceptions (MANDATORY)
**REGLA FUNDAMENTAL: Evitar excepciones en código de biblioteca. Usar excepciones SOLO en la interfaz externa (I/O).**

**Preferir siempre:**
1. `std::pair<error_code, T>` - Cuando el error es parte del flujo normal
2. `std::optional<T>` - Cuando el error es raro pero posible

**Excepciones SOLO permitidas en:**
- Funciones `*_or_throw()` - Wrappers explícitos para interfaz externa
- Operadores `<<` y `>>` (iostream) - Interfaz I/O estándar
- Constructores desde string que lanzan `std::invalid_argument`

**Patrón recomendado:**
```cpp
// CORRECTO - Función core sin excepciones
static std::optional<int128_base_t> from_cstr(const char* str) noexcept;

// CORRECTO - Wrapper explícito para usuarios que prefieren excepciones
static int128_base_t from_cstr_or_throw(const char* str); // throw std::invalid_argument

// CORRECTO - Par error/valor para casos complejos
enum class parse_error : uint8_t { success = 0, invalid_char, overflow };
static constexpr std::pair<parse_error, int128_base_t> parse(const char* str) noexcept;

// INCORRECTO - Lanzar excepciones en función core
static int128_base_t from_cstr(const char* str) { throw std::invalid_argument(...); }
```

**Rationale:**
- Bibliotecas numéricas deben ser exception-free para máximo rendimiento
- El código sin excepciones es más fácil de razonar en contextos constexpr
- Los usuarios tienen control total sobre manejo de errores
- Los wrappers `*_or_throw()` satisfacen casos de uso con excepciones

### 5. Always Use Braces
**Every `if`, `else if`, `else` MUST have braces, even for single statements:**
```cpp
// CORRECT
if (value < 0) {
    return -value;
}

// WRONG
if (value < 0)
    return -value;
```

### 6. Template Declaration Layout
**`template<...>` on its own line, then signature:**
```cpp
// CORRECT
template <integral_builtin T>
constexpr int128_base_t& operator+=(T other) noexcept {

// WRONG (all on one line)
template <integral_builtin T> constexpr int128_base_t& operator+=(T other) noexcept {
```

### 7. K&R Brace Style (Compact)
**Opening brace on same line as statement/signature:**
```cpp
// CORRECT
if (condition) {
    ...
} else if (other) {
    ...
} else {
    ...
}

constexpr uint64_t high() const noexcept {
    return data[1];
}

// WRONG (Allman style - too vertical)
if (condition)
{
    ...
}
```

### 8. Noexcept Everywhere
**All functions that don't throw must be marked `noexcept`:**
```cpp
constexpr uint64_t low() const noexcept { return data[0]; }
```

### 9. Naming Conventions
**Consistent naming across the codebase:**

| Element | Style | Examples |
|---------|-------|----------|
| **Types/Classes** | `snake_case_t` | `uint128_t`, `int128_base_t`, `parse_error` |
| **Template params** | `PascalCase` or `UPPER` | `S`, `T`, `T1`, `T2` |
| **Template types** | `snake_case_tt` | `int128_base_tt.hpp` (template type file) |
| **Enums (enum class)** | `snake_case_ec_t` | `signedness_ec_t`, `parse_error_ec_t` |
| **Enum values** | `snake_case` | `unsigned_type`, `signed_type`, `success` |
| **Functions/Methods** | `snake_case` | `to_string()`, `leading_zeros()`, `is_negative()` |
| **Variables** | `snake_case` | `const uint64_t carry`, `const auto result` |
| **Static constants** | `UPPER_SNAKE` | `BITS`, `BYTES`, `LIMBS`, `MSULL`, `LSULL` |
| **Numeric limits** | `UI64_MAX` pattern | `UI64_MAX`, `I64_MIN`, `UI_I64_MAX` |
| **Namespaces** | `snake_case` | `nstd`, `intrinsics` |
| **Files** | `snake_case.hpp` | `int128_base_tt.hpp`, `type_traits.hpp` |
| **Macros** (avoid) | `UPPER_SNAKE` | `INT128_BASE_TT_HPP` (include guards only) |

**Special patterns:**
- Type aliases end with `_t`: `uint128_t`, `int128_t`
- Template types end with `_tt`: `int128_base_tt` (parametrized types)
- Enum classes end with `_ec_t`: `parse_error_ec_t`, `signedness_ec_t`
- Boolean methods start with `is_`: `is_zero()`, `is_negative()`, `is_power_of_2()`
- Conversion methods: `to_string()`, `to_uint128()`
- Parse/factory methods: `from_string()`, `from_bytes()`
- Index constants: `MSULL` (Most Significant ULongLong), `LSULL` (Least Significant)

**Why snake_case for classes?**
Classes use `snake_case_t` (not `PascalCase`) to match C++ standard library style (`std::string`, `std::vector`). This makes `uint128_t` feel like a native type.

### 10. Builtin-Like Behavior
**Types must feel like native integers:**
- Implicit conversions where standard integers allow them
- Same overflow behavior (wrap-around for unsigned, two's complement for signed)
- Compatible with STL algorithms, `std::numeric_limits`, type traits
- Operator symmetry: `uint128_t + int` AND `int + uint128_t` must both work

### 11. Explicit Conversions and Constructors (DESIGN DECISION)
**All conversions and eligible constructors MUST be explicit:**

> **📋 DESIGN RATIONALE:**
> Although the C++ standard permits implicit conversions and `= value` initialization
> syntax (which is common in everyday code), this library enforces **explicit constructors**
> as a deliberate design choice for the following reasons:
>
> 1. **Type Safety:** Prevents accidental conversions that could hide precision loss
> 2. **Clarity of Intent:** `uint128_t x{42}` clearly shows construction intent
> 3. **Consistency:** All numeric types in this library follow the same pattern
> 4. **Bug Prevention:** Avoids silent narrowing conversions in arithmetic expressions
> 5. **Modern C++ Style:** Aligns with C++ Core Guidelines (ES.23, ES.46)
>
> This means `uint128_t x = 42;` is **NOT allowed** - use `uint128_t x{42};` instead.

```cpp
// CORRECT - explicit constructor with brace initialization
template <integral_builtin T>
explicit constexpr int128_base_t(T value) noexcept;

const uint128_t x{42};           // OK - explicit brace init
const uint128_t y{some_value};   // OK - explicit brace init
const auto z{uint128_t{100}};    // OK - explicit construction

// CORRECT - explicit conversion operator
explicit constexpr operator bool() const noexcept;
explicit constexpr operator uint64_t() const noexcept;

// WRONG - implicit conversion (avoid)
constexpr int128_base_t(int value) noexcept;  // Missing explicit!
constexpr operator double() const noexcept;   // Missing explicit!

// WRONG - = initialization syntax (not allowed with explicit constructors)
const uint128_t x = 42;          // ERROR: implicit conversion
const uint128_t y = some_value;  // ERROR: implicit conversion
```

**Exceptions (implicit allowed):**
- Copy constructor: `int128_base_t(const int128_base_t&)`
- Move constructor: `int128_base_t(int128_base_t&&)`

### 12. Prefer Immutability
**If avoiding a mutable variable is easy, do it:**
```cpp
// CORRECT - const + early return, no mutation
const int64_t result{42};
// ... operations ...
if (result < 0) {
    return -result;
} else {
    return result;
}

// CORRECT - compute directly with ternary
const auto result{(condition) ? value_a : value_b};

// WRONG - unnecessary mutation
int64_t result{42};      // Not const!
// ...
if (result < 0) {
    result = -result;    // Mutation!
}
return result;

// WRONG - mutable then conditional assign
auto result{value_a};    // Not const!
if (!condition) {
    result = value_b;    // Mutation!
}
```

### 13. Explicit Casts Only
**All type conversions MUST use C++ style casts, never C-style:**
```cpp
// CORRECT - explicit C++ casts
const uint64_t high{static_cast<uint64_t>(value)};
const auto ptr{reinterpret_cast<char*>(buffer)};
const auto& ref{const_cast<int128_t&>(cref)};

// WRONG - C-style cast (forbidden)
const uint64_t high{(uint64_t)value};      // Use static_cast!
const auto ptr{(char*)buffer};              // Use reinterpret_cast!
```

### 14. Explicit std:: Namespace
**Always use explicit `std::` prefix, except for arithmetic types imported via `using`:**
```cpp
// At file scope - import arithmetic types
using std::uint64_t;
using std::int64_t;
using std::size_t;

// CORRECT - explicit std:: for everything else
std::string result{};
std::vector<uint64_t> data{};
std::cout << value << std::endl;
const auto it{std::find(v.begin(), v.end(), x)};

// WRONG - using namespace (forbidden)
using namespace std;           // Never!
using std::string;             // Only for arithmetic types!
cout << value;                 // Missing std::!
```

---

## Project Overview

Header-only C++20 library implementing 128-bit integers (`uint128_t` and `int128_t`) with full STL integration. Cross-compiler support: GCC, Clang, Intel, MSVC on Windows/MSYS2.

**Current Phase**: 1.66 (branch `phase-1.66`)  
**Previous**: 1.5 (template unification base) | **Next**: 1.75  
**Namespace**: `nstd` | **Storage**: `uint64_t data[2]` (little-endian: `data[0]` = low)

## Architecture

### Header Organization (Phase 1.66 - Unified Template)
```
include/
├── int128_base_tt.hpp                    # ⭐ Unified template: int128_base_t<signedness S>
├── int128_base_[feature].hpp             # Feature modules (11 complete)
├── int128_base_traits_specializations.hpp # ⚠️ Include FIRST (before stdlib)
├── type_traits.hpp                       # integral_builtin concept
├── intrinsics/                           # Cross-compiler low-level ops
│   ├── arithmetic_operations.hpp
│   ├── bit_operations.hpp
│   └── compiler_detection.hpp
└── specializations/                      # Optimized algorithms
```

**Type Aliases (defined in int128_base_tt.hpp):**
```cpp
using uint128_t = int128_base_t<signedness::unsigned_type>;
using int128_t = int128_base_t<signedness::signed_type>;
```

### Feature Modules (14 total)
`t`, `traits`, `limits`, `concepts`, `algorithm`, `iostreams`, `cmath`, `numeric`, `ranges`, `format`, `safe`, `thread_safety`, `comparison_boost`, `interop`

## Build System

### Recommended: make.py (Isolated Environments)
```bash
python make.py init                                    # First time: detect compilers
python make.py build uint128 bits tests gcc release    # Build specific test
python make.py check uint128 bits                      # Run tests
python make.py run uint128 algorithm benchs            # Run benchmarks
python make.py test                                    # ALL tests
python make.py demo tutorials 01_basic_operations      # Compile + run demo
```

### Optimization Levels (Release Modes)
**All release builds MUST test multiple optimization levels:**

| Mode | GCC/Clang | MSVC | Intel | Purpose |
|------|-----------|------|-------|---------|
| `release-O1` | `-O1` | `/O1` | `-O1` | Size optimization |
| `release-O2` | `-O2` | `/O2` | `-O2` | Standard optimization |
| `release-O3` | `-O3` | `/Ox` | `-O3` | Aggressive optimization |
| `release-Ofast` | `-Ofast -fexpensive-optimizations` | `/Ox /fp:fast` | `-Ofast` | Maximum speed (may break IEEE) |

**GCC aggressive flags (release-O3 and release-Ofast):**
```bash
-O3 -fexpensive-optimizations -funroll-loops -ftree-vectorize -march=native
```

### Benchmark Comparisons
**Benchmarks MUST compare against:**

1. **Builtin integer types:**
   - `int8_t`, `int16_t`, `int32_t`, `int64_t`
   - `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`

2. **Compiler extensions (when available):**
   - `__int128` (GCC/Clang signed)
   - `unsigned __int128` (GCC/Clang unsigned)

3. **Boost.Multiprecision backends:**
   - `cpp_int` (pure C++, header-only)
   - `gmp_int` (GNU MP backend, fastest)
   - `tommath_int` (libtommath backend)
   - `checked_uint128_t` (overflow-checked)

```cpp
// Benchmark types to compare
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/tommath.hpp>

using boost_cpp_uint128 = boost::multiprecision::uint128_t;      // cpp_int backend
using boost_gmp_uint128 = boost::multiprecision::number<
    boost::multiprecision::gmp_int, 
    boost::multiprecision::et_off>;
using boost_tom_uint128 = boost::multiprecision::tom_int;

#ifdef __SIZEOF_INT128__
using builtin_uint128 = unsigned __int128;
using builtin_int128 = __int128;
#endif
```

### Demo Development
**All demos MUST be built and enriched creatively:**
- Build all demos with every compiler
- Add new demos showcasing real-world use cases
- Categories: `tutorials`, `examples`, `showcase`, `comparison`, `performance`, `integration`

```bash
# Build ALL demos
python make.py build demos all gcc release-O2
python make.py build demos all clang release-O3

# List available demos
python make.py list demos
```

### Alternative: Makefile
```bash
make build_tests TYPE=uint128 FEATURE=cmath COMPILER=gcc MODE=release
make check TYPE=int128 FEATURE=traits COMPILER=all MODE=all
make demo CATEGORY=tutorials DEMO=01_basic_operations
```

### Generic Bash Scripts
```bash
scripts/build_generic.bash uint128 cmath tests gcc release
scripts/check_generic.bash int128 traits all all
scripts/run_generic.bash demos tutorials 01_basic_operations gcc release
```

### Direct Compilation
```bash
g++ -std=c++20 -Iinclude tests/uint128_cmath_extracted_tests.cpp -o build/test

# Thread safety requires extra flags:
g++ -std=c++20 -Iinclude -pthread tests/uint128_thread_safety_extracted_tests.cpp -latomic -o build/test
```

## File Naming Conventions

### Tests & Benchmarks
```
tests/[type]_[feature]_extracted_tests.cpp
benchs/[type]_[feature]_extracted_benchs.cpp
→ build/build_[tests|benchs]/[compiler]/[mode]/[type]_[feature]_[target]_[compiler].exe
```

### Demos (35+ in 7 categories)
```
demos/tutorials/01_basic_operations.cpp    # Sequential learning
demos/examples/ipv6_address.cpp            # Real-world use cases
demos/showcase/showcase_cryptography.cpp   # Advanced demonstrations
→ build/build_demos/[compiler]/[mode]/[demo_name].exe
```

## Key Code Patterns

### 1. Trait Specializations MUST Come First
```cpp
#include "int128_base_traits_specializations.hpp"  // BEFORE any stdlib
#include <iostream>
```

### 2. Constexpr + Noexcept Everywhere
```cpp
constexpr uint64_t high() const noexcept { return data[1]; }
```

### 3. Sign Extension Pattern (unified template)
```cpp
template <integral_builtin T>
constexpr int128_base_t(T value) noexcept : data{static_cast<uint64_t>(value), 0ull} {
    if constexpr (is_signed && std::is_signed_v<T>) {
        if (value < 0) data[1] = ~0ull;  // Extend sign
    }
}
```

### 4. Operations Requiring `if constexpr (is_signed)`
- `operator-()` - negation (signed only)
- `operator>>=` - arithmetic vs logical shift
- `abs()`, `from_string("-123")` - signed only
- `operator/`, `%` - signed division

### 5. Friend Operators for Symmetry
```cpp
// Enables: 50u + uint128_t (not just uint128_t + 50u)
friend constexpr int128_base_t operator+(const int128_base_t& lhs, const int128_base_t& rhs) noexcept;
```

## Compiler-Specific Notes

### Intel OneAPI (Complex Setup)
```bash
source scripts/setup_intel_combined.bash x64  # MUST run first in MSYS2
```

### Thread Safety Flags
**Always required for thread_safety feature:**
```bash
-pthread -latomic
```
Scripts auto-detect `<thread>`, `<atomic>`, `thread_safety.hpp` and add flags.

### Intrinsics Selection
`include/intrinsics/compiler_detection.hpp` auto-selects:
- **MSVC**: `_umul128()`, `_udiv128()`, `_BitScanReverse64()`
- **GCC/Clang**: `__builtin_clzll()`, `__builtin_ctzll()`, `__uint128_t`
- **Fallback**: Portable C++20 implementations

---

## ⚠️ MANDATORY: Sanitizers & Static Analysis

### Build Requirements
**Every compilation MUST:**
1. Compile with ALL 4 compilers: GCC, Clang, MSVC (cl), Intel (icx/icpx)
2. Pass ALL tests with ALL 4 compilers
3. Run static analysis tools

### Sanitizers by Compiler

**GCC/Clang (MSYS2 & WSL):**
```bash
# AddressSanitizer - buffer overflow, use-after-free
-fsanitize=address -fno-omit-frame-pointer

# UndefinedBehaviorSanitizer - undefined behavior
-fsanitize=undefined

# ThreadSanitizer - data races (incompatible with ASan)
-fsanitize=thread

# Combined (recommended for debug builds)
-fsanitize=address,undefined -fno-omit-frame-pointer
```

**MSVC (cl.exe):**
```bash
# AddressSanitizer
/fsanitize=address

# Runtime checks (debug only)
/RTC1
```

**Intel (icx/icpx):**
```bash
# AddressSanitizer
-fsanitize=address

# UndefinedBehaviorSanitizer  
-fsanitize=undefined
```

### Static Analysis Tools

**cppcheck (run on every build):**
```bash
cppcheck --enable=all --std=c++20 --error-exitcode=1 \
    --suppress=missingIncludeSystem \
    -I include/ tests/*.cpp
```

**Infer (Facebook static analyzer):**
```bash
infer run -- g++ -std=c++20 -Iinclude tests/uint128_t_extracted_tests.cpp
infer report
```

**PVS-Studio (commercial, free for OSS):**
```bash
# First time setup (get free OSS license at pvs-studio.com)
pvs-studio-analyzer credentials PVS-Studio Free FREE-FREE-FREE-FREE

# Analyze
pvs-studio-analyzer analyze -o report.log -j4
plog-converter -a GA:1,2 -t fullhtml report.log -o pvs-report
```

### WSL Compilation (Ubuntu 25.04)

**Required compilers in WSL:**
- GCC: 13, 14, 15
- Clang: 18, 19, 20, 21
- Intel: icpx (oneAPI)

**WSL build command:**
```bash
# From Windows, run in WSL
wsl -d Ubuntu-25.04 bash -c "cd /mnt/c/path/to/project && make test COMPILER=all"
```

### CI/CD Validation Matrix

| Platform | Compilers | Sanitizers | Static Analysis |
|----------|-----------|------------|-----------------|
| **MSYS2 (Windows)** | GCC 15, Clang 19, MSVC, Intel ICX | ASan, UBSan | cppcheck |
| **WSL (Ubuntu 25.04)** | GCC 13-15, Clang 18-21, icpx | ASan, UBSan, TSan | cppcheck, Infer |

### Build Modes with Sanitizers

```bash
# Debug + Sanitizers (development)
python make.py build uint128 t tests gcc debug-asan
python make.py build uint128 t tests gcc debug-ubsan

# Release (production)
python make.py build uint128 t tests gcc release

# Full validation (all compilers, all tests)
python make.py test all all
```

## Quick Start: Adding a New Feature

1. Create: `include/int128_base_[feature].hpp`
2. Create: `tests/uint128_[feature]_extracted_tests.cpp`
3. Create: `benchs/uint128_[feature]_extracted_benchs.cpp`
4. Add to `Makefile` `VALID_FEATURES` list
5. Build & validate:
   ```bash
   make build_tests TYPE=uint128 FEATURE=[feature] COMPILER=all MODE=all
   make check TYPE=uint128 FEATURE=[feature] COMPILER=all MODE=all
   ```

## Key Documentation

| Document | Purpose |
|----------|---------|
| [PROMPT.md](../PROMPT.md) | Build conventions, naming patterns (1000+ lines) |
| [TODO.md](../TODO.md) | Current phase progress, roadmap |
| [CHANGELOG.md](../CHANGELOG.md) | Hourly development log |
| [DEV_ENV_VARS.md](../DEV_ENV_VARS.md) | Compiler environment setup |
| [API_INT128_BASE_TT.md](../API_INT128_BASE_TT.md) | Complete API reference |

## Known Issues & Gotchas

1. **Intel linking in MSYS2**: Always `source scripts/setup_intel_combined.bash x64` first
2. **Thread safety flags**: `-pthread -latomic` mandatory for thread_safety feature (auto-detected by scripts)
3. **Signed overflow**: Intentionally wraps around (two's complement), not UB
4. **Legacy scripts**: Prefer `build_generic.bash` over 128 individual legacy scripts in `scripts/individualized/`
5. **`to_cstr()` buffer rotation**: Allows 4 concurrent calls without stomping (e.g., `printf("%s %s", a.to_cstr(), b.to_cstr())`)

