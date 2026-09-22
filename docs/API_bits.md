# API Reference — int128_param_bits.hpp

> Bit counting, width, and rotation functions for 128-bit integers (mirrors `<bit>`).

## Synopsis

```cpp
#include "int128_param_bits.hpp"

namespace nstd {

template <signedness S, representation_form F>
inline constexpr int popcount(const int128_param_t<S, F>& x) noexcept;

template <signedness S, representation_form F>
inline constexpr int countl_zero(const int128_param_t<S, F>& x) noexcept;

template <signedness S, representation_form F>
inline constexpr int countr_zero(const int128_param_t<S, F>& x) noexcept;

template <signedness S, representation_form F>
inline constexpr int bit_width(const int128_param_t<S, F>& x) noexcept;

template <signedness S, representation_form F>
inline constexpr bool is_power_of_2(const int128_param_t<S, F>& x) noexcept;

template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> rotl(const int128_param_t<S, F>& x, int s) noexcept;

template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> rotr(const int128_param_t<S, F>& x, int s) noexcept;

}
```

---

## Functions

### `popcount`

```cpp
template <signedness S, representation_form F>
inline constexpr int popcount(const int128_param_t<S, F>& x) noexcept;
```

Returns the number of 1-bits in `x`.

### `countl_zero`

```cpp
template <signedness S, representation_form F>
inline constexpr int countl_zero(const int128_param_t<S, F>& x) noexcept;
```

Counts consecutive zero bits from the most significant bit. Returns 128 for zero.

### `countr_zero`

```cpp
template <signedness S, representation_form F>
inline constexpr int countr_zero(const int128_param_t<S, F>& x) noexcept;
```

Counts consecutive zero bits from the least significant bit. Returns 128 for zero.

### `bit_width`

```cpp
template <signedness S, representation_form F>
inline constexpr int bit_width(const int128_param_t<S, F>& x) noexcept;
```

Returns `128 - countl_zero(x)`. Equivalent to `floor(log2(x)) + 1` for nonzero x.

### `is_power_of_2`

```cpp
template <signedness S, representation_form F>
inline constexpr bool is_power_of_2(const int128_param_t<S, F>& x) noexcept;
```

Returns `true` if `x` is a power of two (exactly one bit set and x > 0).

### `rotl`

```cpp
template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> rotl(const int128_param_t<S, F>& x, int s) noexcept;
```

Rotates bits left by `s` positions.

### `rotr`

```cpp
template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> rotr(const int128_param_t<S, F>& x, int s) noexcept;
```

Rotates bits right by `s` positions.

---

## Member Function Equivalents

The `int128_param_t` class also provides member methods:

| Free function | Member method |
|---------------|---------------|
| `nstd::popcount(x)` | `x.popcount()` / `x.count_ones()` |
| `nstd::countl_zero(x)` | `x.leading_zeros()` |
| `nstd::countr_zero(x)` | `x.trailing_zeros()` |
| `nstd::bit_width(x)` | `x.bit_width()` |
| `nstd::is_power_of_2(x)` | `x.is_power_of_2()` |
| `nstd::rotl(x, s)` | `x.rotate_left(s)` |
| `nstd::rotr(x, s)` | `x.rotate_right(s)` |

---

## Example

```cpp
#include "int128_param_bits.hpp"
using namespace nstd;

constexpr uint128_t val{0xFF00};

static_assert(nstd::popcount(val) == 8);
static_assert(nstd::countl_zero(val) == 112);
static_assert(nstd::countr_zero(val) == 8);
static_assert(nstd::bit_width(val) == 16);
static_assert(!nstd::is_power_of_2(val));

constexpr uint128_t pow2{uint128_t{1} << 64};
static_assert(nstd::is_power_of_2(pow2));
static_assert(nstd::bit_width(pow2) == 65);
```

---

## Nombre canonico (ADR-021)

Las mismas operaciones se llamaban distinto en esta familia y en `fixed_int_t`.
Desde el 23 sep, **el nombre canonico existe en las dos** y el antiguo se queda
solo donde ya estaba, documentado. No llevan `[[deprecated]]`: el CI compila con
`-Werror` y la propia biblioteca usa algunos de los antiguos.

| Operacion | Antiguo aqui | **Canonico** |
|---|---|---|
| raiz entera | `isqrt` | **`sqrt`** |
| producto ancho | `widening_mul` | **`mul_wide`** |
| potencia de dos | `is_power_of_2` | **`has_single_bit`** |
| potencia | `pow(T, unsigned)` | **`pow` con las dos firmas** |

Los dos nombres de cada par dan **el mismo valor**, no solo compilan: la matriz
de paridad lo vigila, porque un alias que se desincroniza compila igual.

> `int128_param_t` **no entra** en el objetivo de 24/24 del acompanamiento de
> `std` ([ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md): esta
> familia se retira). Los nombres canonicos si se le anaden, porque el objetivo
> de ADR-021 es que mover codigo entre familias no obligue a saberse dos
> vocabularios.

### `has_single_bit` aqui

`nstd::has_single_bit(x)` reenvia a `is_power_of_2(x)`. Hereda su nota: el
resultado **no es significativo en Exceso-K** sin convertir antes a complemento
a dos.
