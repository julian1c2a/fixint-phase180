# API Reference — int128_param_limits.hpp

> `std::numeric_limits` specializations for all 128-bit integer types.

## Synopsis

```cpp
#include "int128_param_limits.hpp"

namespace std {
template <> class numeric_limits<nstd::uint128_t>;
template <> class numeric_limits<nstd::int128_tc_t>;
template <> class numeric_limits<nstd::int128_ms_t>;
template <> class numeric_limits<nstd::int128_ek_t>;
}
```

---

## Common Interface

Each specialization provides the full `std::numeric_limits` interface:

### Static Member Functions

```cpp
static constexpr T min() noexcept;
static constexpr T lowest() noexcept;
static constexpr T max() noexcept;
static constexpr T epsilon() noexcept;        // always 0
static constexpr T round_error() noexcept;    // always 0
static constexpr T infinity() noexcept;       // always 0
static constexpr T quiet_NaN() noexcept;      // always 0
static constexpr T signaling_NaN() noexcept;  // always 0
static constexpr T denorm_min() noexcept;     // always 0
```

### Static Member Constants

| Member | uint128 | int128_tc | int128_ms | int128_ek |
|--------|:-------:|:---------:|:---------:|:---------:|
| `is_specialized` | true | true | true | true |
| `is_signed` | false | true | true | true |
| `is_integer` | true | true | true | true |
| `is_exact` | true | true | true | true |
| `has_infinity` | false | false | false | false |
| `has_quiet_NaN` | false | false | false | false |
| `is_bounded` | true | true | true | true |
| `is_modulo` | true | true | false | false |
| `digits` | 128 | 127 | 127 | 127* |
| `digits10` | 38 | 38 | 38 | 38* |
| `max_digits10` | 0 | 0 | 0 | 0 |
| `radix` | 2 | 2 | 2 | 2 |
| `min_exponent` | 0 | 0 | 0 | 0 |
| `max_exponent` | 0 | 0 | 0 | 0 |

*EK values depend on bias K = 2^126.

---

## Value Ranges

### `uint128_t`

| Function | Value |
|----------|-------|
| `min()` | 0 |
| `max()` | 2^128 - 1 = 340282366920938463463374607431768211455 |
| `lowest()` | 0 |

### `int128_tc_t` (Two's Complement)

| Function | Value |
|----------|-------|
| `min()` | -2^127 = -170141183460469231731687303715884105728 |
| `max()` | 2^127 - 1 = 170141183460469231731687303715884105727 |
| `lowest()` | same as `min()` |

### `int128_ms_t` (Magnitude-Sign)

| Function | Value |
|----------|-------|
| `min()` | -(2^127 - 1) |
| `max()` | 2^127 - 1 |
| `lowest()` | same as `min()` |

Note: MS has both +0 and -0.

### `int128_ek_t` (Excess-K, K = 2^126)

| Function | Value |
|----------|-------|
| `min()` | -2^126 |
| `max()` | 2^128 - 1 - 2^126 |
| `lowest()` | same as `min()` |

---

## Example

```cpp
#include "int128_param_limits.hpp"
using namespace nstd;

static_assert(std::numeric_limits<uint128_t>::is_specialized);
static_assert(std::numeric_limits<uint128_t>::digits == 128);
static_assert(!std::numeric_limits<uint128_t>::is_signed);
static_assert(std::numeric_limits<int128_tc_t>::is_signed);

constexpr auto mx = std::numeric_limits<uint128_t>::max();
// mx == 340282366920938463463374607431768211455
```
