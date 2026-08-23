# Plan: Aritmética por Constantes en Tiempo de Compilación (GM + Shifts)

> **Versión 2.0** — 22 marzo 2026.
> Ampliación del plan original. API principal: funciones miembro
> `num.divmod<D>()`, `num.mul<K>()` con constantes compiletime.

---

## 0. Resumen Ejecutivo

| Característica | Alcance |
|---|---|
| **API principal** | Funciones miembro: `num.divmod<D>()`, `num.div<D>()`, `num.mod<D>()`, `num.mul<K>()` |
| **Divisores GM** | Tabla lookup consteval de `magic_constants` para **d = 3 .. 1023** (1021 entradas) |
| **Potencias de 2** | `div/mod/mul` optimizado para **2^k, k = 0..127** (shift + mask) |
| **Multiplicaciones** | `num.mul<K>()` para K ∈ [2, 1023] via shifts+adds autogenerados |
| **Representaciones** | unsigned (binnat/TC), signed (TC/MS/EK) — dispatch automático |
| **Constexpr** | `compute_magic()` → consteval; `.divmod<D>()/.mul<K>()` → constexpr |

---

## 1. Estado Actual

### Lo que ya existe

| Componente | Ubicación | Estado |
|---|---|---|
| `fast_div10/3/5/7/9/100/1e19` | `include/algorithms/div_by_const.hpp` | Funcional, hardcodeado |
| `fast_divmod10_limbs`, `fast_divmod_1e19_limbs` | `int128_parameterized.hpp` (to_string) | Integrado en hot path |
| `mulhi_128()` (Karatsuba-based) | `include/int128_param_arithmetic.hpp` | Funcional, inline |
| `karatsuba_full_mul()` | `include/algorithms/karatsuba.hpp` | Funcional, 256-bit |
| Legacy `divrem_D.hpp` (d=3..31) | `legacy-code/divmod_by_constexpr/` | Solo uint8-uint64 |
| `compute_magic` (plan) | Este documento (v1) | No implementado |

### Lo que falta

1. **`compute_magic<uint128_t>(d)` consteval** — generar M, S para cualquier d
2. **Tabla lookup consteval para d = 3..1023** — 1021 entradas precalculadas
3. **API miembro template**: `.divmod<D>()`, `.div<D>()`, `.mod<D>()`, `.mul<K>()`
4. **Optimización potencias de 2**: shift/mask para todo k ∈ [0, 127]
5. **Multiplicación por constante** via descomposición shift+add
6. **Soporte signed** (TC/MS/EK)

---

## 2. Fundamento Teórico

### 2.1 Division por Inverso Multiplicativo (Granlund-Montgomery)

Para un divisor constante `d`, se reemplaza `n / d` por:

```
q = mulhi(n, M) >> S
```

Donde:

- `M = ceil(2^(W+S) / d)` — el "número mágico" (inverso multiplicativo)
- `S` — shift post-multiplicación
- `W` — ancho del tipo (128 bits)
- `mulhi(a, b)` — parte alta de la multiplicación a×b

### 2.2 Tres Métodos Según el Divisor

| Método | Condición | Fórmula | Coste |
|--------|-----------|---------|-------|
| **Shift directo** | d = 2^k | `n >> k`, `n & (2^k - 1)` | 1 op |
| **Multiply-shift** | M cabe en 128 bits | `mulhi(n, M) >> S` | 1 mulhi + 1 shift |
| **Add-shift (overflow)** | M necesita 129 bits | `(t + ((n-t)>>1)) >> (S-1)` | 1 mulhi + 3 ops |

### 2.3 Multiplicación por Constante (Shift-Add Chains)

Para `n * K` donde K es constante conocida en compiletime:

```
K=3:   (n << 1) + n           // 2 ops
K=5:   (n << 2) + n           // 2 ops
K=7:   (n << 3) - n           // 2 ops
K=10:  (n << 3) + (n << 1)    // 3 ops
K=100: (n << 6) + (n << 5) + (n << 2)  // 5 ops
```

Para K arbitrario: **descomposición binaria** (siempre correcta, ≤ popcount(K) adds
- bit_width(K) shifts). Constantes comunes tienen cadenas óptimas más cortas.

### 2.4 Cálculo de las Constantes Mágicas

Algoritmo (Hacker's Delight, cap.10 / Granlund-Montgomery):

```
Input:  d (divisor), W (ancho en bits = 128)
Output: M (multiplicador), S (shift), método (multiply o add)

1. p = W
2. Mientras 2^p < d × (2^W - rem(2^p, d)):
       p += 1
3. M = (2^p + d - rem(2^p, d)) / d
4. Si M < 2^W:
       método = multiply, S = p - W
   Sino:
       M' = M - 2^W
       método = add, S = p - W
```

---

## 3. Diseño de la API

### 3.1 Funciones Miembro Template (API Principal)

```cpp
template <signedness S, representation_form F>
class int128_param_t {
public:
    // === Division por constante compiletime ===

    /// @brief Division: this / D
    /// @tparam D divisor constante (D > 0)
    /// @return cociente floor(this / D)
    template <std::uint64_t D>
        requires (D > 0)
    [[nodiscard]] constexpr int128_param_t div() const noexcept;

    /// @brief Módulo: this % D
    /// @tparam D divisor constante (D > 0)
    /// @return resto this - floor(this/D) * D
    template <std::uint64_t D>
        requires (D > 0)
    [[nodiscard]] constexpr int128_param_t mod() const noexcept;

    /// @brief Division + Módulo simultáneos
    /// @tparam D divisor constante (D > 0)
    /// @return {cociente, resto}
    template <std::uint64_t D>
        requires (D > 0)
    [[nodiscard]] constexpr std::pair<int128_param_t, int128_param_t>
        divmod() const noexcept;

    // === Multiplicación por constante compiletime ===

    /// @brief Multiplicación: this * K (via shifts+adds)
    /// @tparam K factor constante (K > 0)
    /// @return this * K (wrapping, como operator*)
    template <std::uint64_t K>
        requires (K > 0)
    [[nodiscard]] constexpr int128_param_t mul() const noexcept;
};
```

### 3.2 Ejemplos de Uso

```cpp
const uint128_t n{some_value};

// Division por constante — zero runtime overhead para seleccionar método
const auto q10{n.div<10>()};           // GM multiply-shift
const auto r10{n.mod<10>()};           // GM + subtract
const auto [q, r]{n.divmod<10>()};     // GM, un solo mulhi

// Potencias de 2 — compila a shift/mask puro
const auto half{n.div<2>()};           // n >> 1
const auto low_byte{n.mod<256>()};     // n & 0xFF

// Multiplicación por constante — shifts+adds, sin mulhi
const auto times7{n.mul<7>()};         // (n << 3) - n
const auto times100{n.mul<100>()};     // shift-add chain

// Cadena de operaciones (BCD conversion pattern)
auto [chunk, remainder]{n.divmod<10000000000000000000ULL>()};  // div by 10^19
```

### 3.3 Dispatch Interno por `if constexpr`

```cpp
template <std::uint64_t D>
    requires (D > 0)
constexpr int128_param_t div() const noexcept
{
    if constexpr (D == 1) {
        return *this;                              // trivial
    } else if constexpr ((D & (D - 1)) == 0) {
        return *this >> detail::ctz(D);            // power of 2: shift
    } else if constexpr (D <= GM_LOOKUP_MAX) {
        return detail::gm_div(*this,
                              detail::GM_TABLE<D>);  // lookup table
    } else {
        // Compute magic on-the-fly (still consteval, no runtime cost)
        constexpr auto magic{detail::compute_magic_128(D)};
        return detail::gm_div(*this, magic);
    }
}
```

---

## 4. Tabla Lookup de Constantes Mágicas

### 4.1 Diseño de la Tabla

```cpp
namespace detail {

/// Resultado del cálculo GM: contiene todo lo necesario para dividir
struct gm_entry {
    uint64_t M_hi;           // multiplier high limb
    uint64_t M_lo;           // multiplier low limb
    uint8_t  shift;          // post-shift amount S
    bool     needs_overflow; // true = usar add-shift correction
};

/// Constante: máximo divisor en la tabla lookup
static constexpr std::uint64_t GM_LOOKUP_MAX{1023};

/// Tabla lookup: GM_TABLE[d] contiene las constantes para divisor d
/// Índices 0, 1 y 2 están vacíos (d=0 inválido, d=1 trivial, d=2 es pot-2)
/// Índices 3..1023: constantes GM precalculadas en consteval
/// Total: 1021 entradas útiles × 24 bytes = ~24 KB (aceptable para header)
inline constexpr std::array<gm_entry, GM_LOOKUP_MAX + 1>
    GM_TABLE = []() consteval {
        std::array<gm_entry, GM_LOOKUP_MAX + 1> table{};
        for (std::uint64_t d{3}; d <= GM_LOOKUP_MAX; ++d) {
            if ((d & (d - 1)) == 0) continue;  // skip powers of 2
            table[d] = compute_magic_128(d);
        }
        return table;
    }();

} // namespace detail
```

### 4.2 Justificación del Rango 3..1023

| Rango | Entradas | Memoria | Cobertura |
|-------|----------|---------|-----------|
| 3..255 | 253 | ~6 KB | Divisores de 1 byte |
| 3..1023 | 1021 | ~24 KB | Divisores de 10 bits — cubre primos hasta 1021, potencias de 10 hasta 1000, todos los denominadores "prácticos" |
| 3..4095 | 4093 | ~96 KB | Excesivo para header-only |

**¿Por qué 1023?** Porque:

1. Cubre **todos los primos < 1024** (168 primos)
2. Cubre denominadores financieros: 100, 360, 365, 1000
3. Cubre bases numéricas: 8, 10, 16, 32, 60, 64, 100, 256, 1000
4. Cubre divisores de BCD conversion: 10, 100, 1000
5. ~24 KB es aceptable como constexpr data en un header-only library
6. Para D > 1023, `compute_magic_128(D)` se ejecuta igualmente en consteval — sin coste runtime

### 4.3 Estructura de Memoria

```
GM_TABLE[0]    = {} (unused)
GM_TABLE[1]    = {} (unused, d=1 is trivial)
GM_TABLE[2]    = {} (unused, d=2 is power-of-2)
GM_TABLE[3]    = {M_hi, M_lo, shift=1, overflow=false}  // d=3: simple
GM_TABLE[4]    = {} (unused, d=4 is power-of-2)
GM_TABLE[5]    = {M_hi, M_lo, shift=3, overflow=true}   // d=5: overflow
...
GM_TABLE[10]   = {0xCCCC..., 0xCCCC...D, shift=3, overflow=false}
...
GM_TABLE[1000] = {computed}
GM_TABLE[1023] = {computed}
```

---

## 5. Optimización de Potencias de 2

### 5.1 Detección en Compiletime

```cpp
/// @brief Detect power-of-2 and compute shift amount at compile time
template <std::uint64_t D>
struct pow2_traits {
    static constexpr bool is_pow2{D > 0 && (D & (D - 1)) == 0};
    static constexpr int shift{is_pow2 ? detail::constexpr_ctz(D) : -1};
};
```

### 5.2 Operaciones Optimizadas Para Potencias de 2

| Operación | D = 2^k | Implementación | Coste |
|-----------|---------|---------------|-------|
| `div<D>()` | `this >> k` | shift right | 1 op |
| `mod<D>()` | `this & (D-1)` | AND mask | 1 op |
| `divmod<D>()` | `{this >> k, this & (D-1)}` | shift + AND | 2 ops |
| `mul<D>()` | `this << k` | shift left | 1 op |

**Potencias de 2 cubiertas (128 bits):** 2^0 = 1 hasta 2^127.
Todas se resuelven con un único shift/mask — cero multiplicaciones.

### 5.3 Signed Power-of-2 Adjustments

Para signed (TC), la división por potencia de 2 requiere ajuste:

```cpp
// Signed div by 2^k: arithmetic shift + rounding towards zero
if constexpr (is_signed) {
    // Need to add (2^k - 1) when negative to round towards zero
    const auto bias{is_negative() ? int128_param_t{(1ULL << k) - 1} : int128_param_t{0}};
    return (*this + bias) >> k;  // arithmetic shift
}
```

---

## 6. Multiplicación por Constante (num.mul<K>())

### 6.1 Estrategia de Descomposición

Para K constante:

1. **K = 0**: return 0 (trivial)
2. **K = 1**: return *this (trivial)
3. **K = 2^k**: return `*this << k` (shift)
4. **K = 2^a - 1** (mersenne-like): `(*this << a) - *this` (shift + sub)
5. **K = 2^a + 1**: `(*this << a) + *this` (shift + add)
6. **K = 2^a + 2^b**: `(*this << a) + (*this << b)` (2 shifts + add)
7. **K con popcount(K) ≤ 4**: descomposición en suma de potencias de 2
8. **K general**: descomposición binaria estándar (siempre correcta)

### 6.2 Implementación Genérica

```cpp
template <std::uint64_t K>
    requires (K > 0)
constexpr int128_param_t mul() const noexcept
{
    if constexpr (K == 1) {
        return *this;
    } else if constexpr ((K & (K - 1)) == 0) {
        // Power of 2
        return *this << detail::constexpr_ctz(K);
    } else if constexpr (K == 3) {
        return (*this << 1) + *this;
    } else if constexpr (K == 5) {
        return (*this << 2) + *this;
    } else if constexpr (K == 7) {
        return (*this << 3) - *this;
    } else if constexpr (K == 9) {
        return (*this << 3) + *this;
    } else if constexpr (K == 10) {
        return (*this << 3) + (*this << 1);
    } else if constexpr (K == 15) {
        return (*this << 4) - *this;
    } else if constexpr (K == 100) {
        // (n * 25) << 2, where n*25 = (n << 5) - (n << 2) - (n << 1) - n
        // Simpler: (n << 6) + (n << 5) + (n << 2)
        return (*this << 6) + (*this << 5) + (*this << 2);
    } else {
        // Generic: binary decomposition via shifts+adds
        // popcount(K) additions + bit_width(K) shifts maximum
        return detail::generic_mul_const<K>(*this);
    }
}
```

### 6.3 Descomposición Binaria Genérica (constexpr)

```cpp
namespace detail {

/// Multiplicación por constante genérica via descomposición binaria.
/// Genera ≤ popcount(K) additions y ≤ floor(log2(K)) shifts.
/// Totalmente constexpr — el compilador reduce a shifts+adds concretos.
template <std::uint64_t K, signedness S, representation_form F>
constexpr int128_param_t<S, F>
generic_mul_const(const int128_param_t<S, F>& n) noexcept
{
    int128_param_t<S, F> result{0ULL};
    int128_param_t<S, F> shifted{n};
    std::uint64_t k{K};
    while (k != 0) {
        if (k & 1) {
            result += shifted;
        }
        shifted <<= 1;
        k >>= 1;
    }
    return result;
    // NOTA: El compilador unroll+constant-fold este loop completamente
    // porque K es constexpr. El resultado es una secuencia fija de
    // shifts y adds sin ningún branch runtime.
}

} // namespace detail
```

---

## 7. `compute_magic_128(d)` — El Corazón Consteval

### 7.1 Interfaz

```cpp
namespace detail {

struct gm_entry {
    std::uint64_t M_hi;
    std::uint64_t M_lo;
    std::uint8_t  shift;
    bool          needs_overflow;
};

/// @brief Calcula constantes Granlund-Montgomery para uint128 / d.
/// @param d divisor (d >= 3, d no es potencia de 2)
/// @return gm_entry con {M_hi, M_lo, shift, needs_overflow}
/// @note consteval: se ejecuta SOLO en compiletime.
consteval gm_entry compute_magic_128(std::uint64_t d) noexcept;

} // namespace detail
```

### 7.2 Algoritmo (Adaptado a 128 bits)

El cálculo requiere aritmética de 256+ bits para calcular `ceil(2^(128+S) / d)`.
En consteval esto es factible porque:

1. Podemos usar loops largos sin penalty de runtime
2. Podemos implementar bignum básico con arrays de uint64_t
3. La complejidad O(n²) para n=4 limbs es trivial en compiletime

```cpp
consteval gm_entry compute_magic_128(std::uint64_t d) noexcept
{
    // Trabajamos con aritmética de 256 bits (4 limbs de 64 bits)
    // para calcular: M = ceil(2^(128+S) / d)
    //
    // Paso 1: Encontrar S mínimo tal que M < 2^128 (multiply method)
    //         o M < 2^129 (add method)
    // Paso 2: Si M >= 2^128, usar overflow correction
    //         M' = M - 2^128, needs_overflow = true
    // Paso 3: Retornar {M_hi, M_lo, S, needs_overflow}
    //
    // Requiere: div_256_by_64(), shift_left_256(), comparison_256()
    // Todo implementable en consteval con arrays fijos.
}
```

### 7.3 Aritmética Auxiliar Consteval (256-bit)

```cpp
namespace detail::bignum {

/// 256-bit unsigned integer as 4 × uint64_t limbs (little-endian)
struct uint256_ce {
    std::uint64_t limbs[4]{};

    consteval uint256_ce operator<<(int shift) const noexcept;
    consteval uint256_ce operator>>(int shift) const noexcept;
    consteval uint256_ce operator+(const uint256_ce& o) const noexcept;
    consteval uint256_ce operator-(const uint256_ce& o) const noexcept;
    consteval bool operator<(const uint256_ce& o) const noexcept;
    consteval bool operator>=(const uint256_ce& o) const noexcept;

    /// Division: this / d (d is 64-bit scalar)
    consteval std::pair<uint256_ce, std::uint64_t>
        divmod_scalar(std::uint64_t d) const noexcept;
};

} // namespace detail::bignum
```

Esta aritmética solo se ejecuta en consteval — no genera código runtime.
La implementación es straightforward (schoolbook) porque solo necesitamos
4 limbs y el divisor es siempre un uint64_t escalar.

---

## 8. Funciones `gm_div` y `gm_mod` Internas

### 8.1 Método Multiply-Shift (M cabe en 128 bits)

```cpp
/// q = mulhi(n, M) >> S
inline constexpr uint128_t gm_div_simple(
    const uint128_t& n,
    std::uint64_t M_hi, std::uint64_t M_lo,
    int shift) noexcept
{
    const uint128_t M{M_hi, M_lo};
    return mulhi_128(n, M) >> shift;
}
```

### 8.2 Método Add-Shift (Overflow Correction)

```cpp
/// t = mulhi(n, M'); q = (t + ((n - t) >> 1)) >> (S - 1)
inline constexpr uint128_t gm_div_overflow(
    const uint128_t& n,
    std::uint64_t M_hi, std::uint64_t M_lo,
    int shift_minus_1) noexcept
{
    const uint128_t M{M_hi, M_lo};
    const auto t{mulhi_128(n, M)};
    return (t + ((n - t) >> 1)) >> shift_minus_1;
}
```

### 8.3 Dispatch Unificado

```cpp
inline constexpr uint128_t gm_div(
    const uint128_t& n,
    const gm_entry& e) noexcept
{
    if (e.needs_overflow) {
        return gm_div_overflow(n, e.M_hi, e.M_lo, e.shift - 1);
    } else {
        return gm_div_simple(n, e.M_hi, e.M_lo, e.shift);
    }
}
```

---

## 9. Soporte Signed (TC, MS, EK)

### 9.1 Two's Complement Signed

```cpp
template <std::uint64_t D>
constexpr int128_tc_t div_signed_tc(const int128_tc_t& n) noexcept
{
    // C++ truncation-towards-zero semantics
    const bool neg{n.is_negative()};
    const auto abs_n{neg ? -n : n};
    // Reinterpret as unsigned for GM division
    const auto abs_n_u{uint128_t{abs_n.high(), abs_n.low()}};
    const auto q_u{abs_n_u.template div<D>()};
    const int128_tc_t q{q_u.high(), q_u.low()};
    return neg ? -q : q;
}
```

### 9.2 Magnitude-Sign

```cpp
// MS: extraer magnitud, dividir como unsigned, reconstruir signo
const bool sign{n.is_negative()};
const auto magnitude{n.magnitude()};  // uint128_t sin sign bit
const auto q{magnitude.template div<D>()};
return reconstruct_ms(q, sign);
```

### 9.3 Excess-K

```cpp
// EK: no tiene sentido dividir el valor almacenado
// Convertir a TC, dividir, reconvertir
static_assert(!is_excess_k, "divmod<D>() no está definido para Excess-K");
// O: dejar como = delete para EK (consistente con operator/)
```

---

## 10. Fases de Implementación

### Fase A — Aritmética Bignum Consteval + `compute_magic_128`

**Objetivo:** Poder generar constantes GM para cualquier d en consteval.

- [ ] `detail::bignum::uint256_ce` con +, -, <<, >>, <, divmod_scalar
- [ ] `compute_magic_128(d)` que retorna `gm_entry`
- [ ] `static_assert` que verifica constantes conocidas (d=3,5,7,9,10,100,10^19)
- [ ] Tests: comparar `compute_magic_128(d)` contra hardcoded de `div_by_const.hpp`
- **Fichero**: `include/int128_param_divmod.hpp`

### Fase B — Tabla Lookup GM_TABLE[3..1023]

**Objetivo:** 1021 entradas precalculadas accesibles en O(1).

- [ ] Lambda consteval que llena `std::array<gm_entry, 1024>`
- [ ] Verificar que la tabla se evalúa en compiletime (static_assert spot checks)
- [ ] Tests: sweep_unary para cada divisor 3..1023: `n.div<D>() * D + n.mod<D>() == n`
- **Fichero**: mismo header, sección nueva

### Fase C — Funciones Miembro `div<D>()`, `mod<D>()`, `divmod<D>()`

**Objetivo:** API principal integrada en `int128_param_t`.

- [ ] `if constexpr` dispatch: trivial → power-of-2 → lookup → compute-on-fly
- [ ] `divmod<D>()` retorna `std::pair<T, T>`, computa un solo mulhi
- [ ] Tests sweep: divisores primos (3,5,7,11,...,1021), compuestos (6,10,12,...,1000)
- [ ] Tests sweep: potencias de 2 (2,4,8,...,2^63)
- **Fichero**: `include/int128_param_divmod.hpp` + forward-declare en parameterized

### Fase D — Función Miembro `mul<K>()`

**Objetivo:** Multiplicación sin overhead de dispatch.

- [ ] Constantes comunes hardcoded: K = 3,5,7,9,10,15,100
- [ ] Generic `detail::generic_mul_const<K>()` para K arbitrario
- [ ] Potencias de 2: compila a shift puro
- [ ] Tests sweep: verificar `n.mul<K>() == n * uint128_t{K}` para K = 2..1023
- **Fichero**: mismo header

### Fase E — Soporte Signed

**Objetivo:** TC y MS signed correctos.

- [ ] TC: abs → unsigned div → re-sign (truncation towards zero)
- [ ] MS: magnitude → unsigned div → reconstruct sign bit
- [ ] EK: `= delete` (consistente con operator/ existente)
- [ ] Tests: edge cases (0, 1, -1, MIN, MAX) para TC y MS
- **Fichero**: extensión en divmod header

### Fase F — Integración `to_string()` y Benchmarks

**Objetivo:** Beneficio medible en el hot path de conversión.

- [ ] Reemplazar `fast_divmod10_limbs` interno con `divmod<10>()`
- [ ] Reemplazar `fast_divmod_1e19_limbs` con `divmod<10000000000000000000ULL>()`
- [ ] Benchmarks RDTSC: GM miembro vs operator/ vs fast_divN hardcoded
- [ ] Benchmarks: to_string() antes/después
- **Fichero**: integración en `int128_parameterized.hpp`

---

## 11. Dependencias y Orden de Trabajo

```text
detail::bignum::uint256_ce (consteval only)     [FASE A]
         |
         v
compute_magic_128(d) consteval                  [FASE A]
         |
         v
GM_TABLE[3..1023] consteval array               [FASE B]
         |
         v
.div/.mod/.divmod con D constante              [FASE C]
         |
         v
.mul<K>() miembro                               [FASE D]
         |
         v
Signed support (TC, MS, = delete EK)            [FASE E]
         |
         v
to_string() integration + benchmarks            [FASE F]
```

---

## 12. Archivos a Crear/Modificar

| Archivo | Acción | Descripción |
|---------|--------|-------------|
| `include/int128_param_divmod.hpp` | **CREAR** | Header principal: bignum consteval, compute_magic, GM_TABLE, member functions |
| `include/int128_parameterized.hpp` | Modificar | `#include` + friend declarations para .div<D>() etc |
| `include/algorithms/div_by_const.hpp` | Refactor | Reescribir usando GM_TABLE (eliminar hardcoded) |
| `tests/test_sweep_divmod_const.cpp` | **CREAR** | Sweep tests para `div/mod/divmod<D>`, D=3..1023 |
| `tests/test_sweep_mul_const.cpp` | **CREAR** | Sweep tests para `mul<K>`, K=2..1023 |
| `benchs/benchmark_divmod_const.cpp` | **CREAR** | Benchmarks RDTSC: miembro vs operator/ vs hardcoded |
| `docs/API_divmod_const.md` | **CREAR** | Referencia API cppreference-style |

---

## 13. Métricas de Éxito

| Métrica | Objetivo |
|---------|----------|
| **Correctitud** | 100% para TODOS los divisores 2..1023 × sweep 3 regiones (~6.3M valores por divisor) |
| **Correctitud mul** | 100% para TODOS K = 2..1023 × sweep 3 regiones |
| **Speedup `/` por const** | ≥ 2x vs `operator/` software para divisores constantes |
| **Speedup `to_string()`** | ≥ 1.5x mejora en conversión a decimal |
| **Tabla compiletime** | `GM_TABLE` debe evaluarse 100% en consteval (verified via `static_assert`) |
| **Constexpr puro** | `compute_magic_128()` → consteval; `.div<D>()` etc → constexpr |
| **Compiladores** | GCC, Clang, MSVC, Intel — todos compilan y pasan tests |
| **Zero runtime para D ≤ 1023** | Lookup directo, sin cómputo de constantes en runtime |
| **Zero runtime para D > 1023** | consteval compute_magic, sin cómputo en runtime tampoco |

---

## 14. Notas de Diseño

### ¿Por qué funciones miembro y no `const_div<D>` tags?

La versión 1 del plan proponía `x / const_div<10>{}` con operator overloads.
La versión 2 prefiere **funciones miembro** `x.div<10>()` por:

1. **Consistencia**: sigue el patrón `x.is_zero()`, `x.leading_zeros()`, `x.rotate_left(k)`
2. **Descubrimiento**: autocompletado IDE muestra `.div<` inmediatamente
3. **Sin ambigüedad**: `x / const_div<10>{}` puede confundirse con el operator/ regular
4. **Sin overhead sintáctico**: `x.div<10>()` vs `x / const_div<10>{}`
5. **Template argument deduction**: NTTP `<10>` es limpio y familiar

Los tags `const_div<D>` con operator overloads **se eliminan del plan** — una sola API.

### ¿Por qué uint64_t para D/K en vez de uint128_t?

1. Divisores > 2^64 son extremadamente raros para constantes compiletime
2. NTTP de tipo uint128_t no es portátil (no es tipo fundamental en el estándar)
3. Para D = 10^19 (el mayor divisor práctico), uint64_t basta
4. Simplifica enormemente `compute_magic_128`: el divisor es un escalar de 64 bits,
   lo que permite `divmod_scalar` en vez de bignum / bignum
5. Si se necesita D > 2^64 en el futuro, se puede añadir un overload

### Relación con `div_by_const.hpp` existente

El header `algorithms/div_by_const.hpp` con `fast_div3()`, `fast_div10()`, etc.
se **reescribirá** para ser un thin wrapper sobre la nueva infraestructura:

```cpp
// Nuevo div_by_const.hpp (refactored):
namespace nstd::algorithms {
    inline uint128_t fast_div10(const uint128_t& n) noexcept {
        return n.div<10>();   // Delegado al miembro template
    }
    // ... etc para backward compatibility
}
```

Esto elimina la duplicación de constantes hardcodeadas y garantiza
que todo use el mismo pipeline GM verificado.
| Zero overhead | Sin penalizacion cuando se usa division normal (no constexpr_divisor) |
