# API Reference — int128_param_numeric.hpp

> Numeric utility functions: sign, parity, integer square root, factorial, etc.

## Synopsis

```cpp
#include "int128_param_numeric.hpp"

namespace nstd {

template <S, F> constexpr int sign(const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr bool is_even(const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr bool is_odd(const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr int128_param_t<S, F> abs_diff(const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr int ilog2(const int128_param_t<S, F>&) noexcept;
template <S, F> int128_param_t<S, F> isqrt(const int128_param_t<S, F>&) noexcept;
template <S, F> int128_param_t<S, F> factorial(unsigned int) noexcept;
template <S, F> constexpr std::pair<int128_param_t<S, F>, int128_param_t<S, F>> divmod(const int128_param_t<S, F>&, const int128_param_t<S, F>&) noexcept;
template <S, F> constexpr int128_param_t<S, F> power(const int128_param_t<S, F>&, unsigned int) noexcept;

}
```

---

## Functions

### `sign`

```cpp
template <signedness S, representation_form F>
constexpr int sign(const int128_param_t<S, F>& x) noexcept;
```

Returns `-1` if negative, `0` if zero, `+1` if positive.

### `is_even` / `is_odd`

```cpp
template <signedness S, representation_form F>
constexpr bool is_even(const int128_param_t<S, F>& x) noexcept;

template <signedness S, representation_form F>
constexpr bool is_odd(const int128_param_t<S, F>& x) noexcept;
```

Tests parity via `x.low() & 1`.

### `abs_diff`

```cpp
template <signedness S, representation_form F>
constexpr int128_param_t<S, F> abs_diff(
    const int128_param_t<S, F>& a,
    const int128_param_t<S, F>& b) noexcept;
```

Returns `|a - b|` without risk of signed underflow.

### `ilog2`

```cpp
template <signedness S, representation_form F>
constexpr int ilog2(const int128_param_t<S, F>& x) noexcept;
```

Returns `floor(log2(x))`. Undefined for `x == 0`.

### `isqrt`

```cpp
template <signedness S, representation_form F>
int128_param_t<S, F> isqrt(const int128_param_t<S, F>& x) noexcept;
```

Returns `floor(sqrt(x))` — integer square root via Newton's method.

### `factorial`

```cpp
template <signedness S, representation_form F>
int128_param_t<S, F> factorial(unsigned int n) noexcept;
```

Returns `n!`. Overflows silently for large `n` (uint128 fits up to 34!).

### `divmod`

```cpp
template <signedness S, representation_form F>
constexpr std::pair<int128_param_t<S, F>, int128_param_t<S, F>>
    divmod(const int128_param_t<S, F>& dividend,
           const int128_param_t<S, F>& divisor) noexcept;
```

Returns `{quotient, remainder}` in a single operation.

### `power`

```cpp
template <signedness S, representation_form F>
constexpr int128_param_t<S, F> power(
    const int128_param_t<S, F>& base,
    unsigned int exponent) noexcept;
```

Integer exponentiation by squaring. Same as `cmath::pow` but in the numeric module.

---

## Example

```cpp
#include "int128_param_numeric.hpp"
using namespace nstd;

constexpr uint128_t val{100};

static_assert(nstd::is_even(val));
static_assert(!nstd::is_odd(val));
static_assert(nstd::sign(val) == 1);
static_assert(nstd::ilog2(val) == 6);  // floor(log2(100)) = 6

const auto [q, r] = nstd::divmod(val, uint128_t{7});
// q == 14, r == 2

const auto root = nstd::isqrt(uint128_t{144});
// root == 12

const auto fact = nstd::factorial<signedness::unsigned_type, representation_form::binnat>(20u);
// fact == 2432902008176640000
```
