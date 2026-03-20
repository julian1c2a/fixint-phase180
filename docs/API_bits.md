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
