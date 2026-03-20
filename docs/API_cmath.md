# API Reference — int128_param_cmath.hpp

> Mathematical functions for 128-bit integers (mirrors `<cmath>` and `<numeric>`).

## Synopsis

```cpp
#include "int128_param_cmath.hpp"

namespace nstd {

template <S, F> inline constexpr int128_param_t<S, F> abs(const int128_param_t<S, F>&) noexcept;
template <S, F> inline constexpr int128_param_t<S, F> min(const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> inline constexpr int128_param_t<S, F> max(const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> inline constexpr int128_param_t<S, F> clamp(const int128_param_t<S, F>&, const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr int128_param_t<S, F> gcd(const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr int128_param_t<S, F> lcm(const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr int128_param_t<S, F> midpoint(const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr int128_param_t<S, F> pow(const int128_param_t<S, F>&, unsigned int) noexcept;

}
```

---

## Functions

### `abs`

```cpp
template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> abs(const int128_param_t<S, F>& x) noexcept;
```

Returns the absolute value. For unsigned types, returns `x` unchanged.

### `min` / `max`

```cpp
template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> min(const int128_param_t<S, F>& a, const int128_param_t<S, F>& b) noexcept;

template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> max(const int128_param_t<S, F>& a, const int128_param_t<S, F>& b) noexcept;
```

### `clamp`

```cpp
template <signedness S, representation_form F>
inline constexpr int128_param_t<S, F> clamp(
    const int128_param_t<S, F>& v,
    const int128_param_t<S, F>& lo,
    const int128_param_t<S, F>& hi) noexcept;
```

Returns `v` clamped to `[lo, hi]`.

### `gcd`

```cpp
// Same type
template <signedness S, representation_form F>
constexpr int128_param_t<S, F> gcd(const int128_param_t<S, F>& a, const int128_param_t<S, F>& b) noexcept;

// Mixed with integral
template <signedness S, representation_form F, typename T>
constexpr int128_param_t<S, F> gcd(const int128_param_t<S, F>& a, T b) noexcept;

template <signedness S, representation_form F, typename T>
constexpr int128_param_t<S, F> gcd(T a, const int128_param_t<S, F>& b) noexcept;
```

Greatest common divisor using the Euclidean algorithm.

### `lcm`

```cpp
template <signedness S, representation_form F>
constexpr int128_param_t<S, F> lcm(const int128_param_t<S, F>& a, const int128_param_t<S, F>& b) noexcept;

// Mixed overloads same as gcd
```

Least common multiple: `lcm(a,b) = |a| * (|b| / gcd(a,b))`.

### `midpoint`

```cpp
template <signedness S, representation_form F>
constexpr int128_param_t<S, F> midpoint(const int128_param_t<S, F>& a, const int128_param_t<S, F>& b) noexcept;
```

Returns the midpoint without overflow: `a + (b - a) / 2` (rounded towards `a`).

### `pow`

```cpp
template <signedness S, representation_form F>
constexpr int128_param_t<S, F> pow(const int128_param_t<S, F>& base, unsigned int exp) noexcept;
```

Integer exponentiation by squaring.

---

## Example

```cpp
#include "int128_param_cmath.hpp"
using namespace nstd;

constexpr uint128_t a{48};
constexpr uint128_t b{36};

static_assert(nstd::gcd(a, b) == uint128_t{12});
static_assert(nstd::lcm(a, b) == uint128_t{144});
static_assert(nstd::midpoint(uint128_t{10}, uint128_t{20}) == uint128_t{15});
static_assert(nstd::pow(uint128_t{2}, 64) == uint128_t{0, 1});  // 2^64
static_assert(nstd::clamp(uint128_t{150}, uint128_t{0}, uint128_t{100}) == uint128_t{100});
```
