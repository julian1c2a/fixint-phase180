# API Reference — int128_param_safe.hpp

> Overflow-checked, saturating, and optional-returning arithmetic operations.

## Synopsis

```cpp
#include "int128_param_safe.hpp"

namespace nstd {

template <signedness Sign, representation_form Form>
struct checked_result;

// Checked (returns {value, overflow_flag})
template <S, F> constexpr checked_result<S, F> checked_add(const T&, const T&) noexcept;
template <S, F> constexpr checked_result<S, F> checked_sub(const T&, const T&) noexcept;
template <S, F> constexpr checked_result<S, F> checked_mul(const T&, const T&) noexcept;
template <S, F> constexpr checked_result<S, F> checked_div(const T&, const T&) noexcept;

// Saturating (clamps to min/max on overflow)
template <S, F> constexpr T saturating_add(const T&, const T&) noexcept;
template <S, F> constexpr T saturating_sub(const T&, const T&) noexcept;
template <S, F> constexpr T saturating_mul(const T&, const T&) noexcept;

// Try (returns std::optional)
template <S, F> constexpr std::optional<T> try_add(const T&, const T&) noexcept;
template <S, F> constexpr std::optional<T> try_sub(const T&, const T&) noexcept;
template <S, F> constexpr std::optional<T> try_mul(const T&, const T&) noexcept;
template <S, F> constexpr std::optional<T> try_div(const T&, const T&) noexcept;

}
```

---

## `checked_result<Sign, Form>`

```cpp
template <signedness Sign, representation_form Form>
struct checked_result {
    int128_param_t<Sign, Form> value;
    bool overflow;

    explicit operator bool() const noexcept;  // true if NO overflow
};
```

---

## Checked Operations

### `checked_add`

```cpp
template <signedness Sign, representation_form Form>
constexpr checked_result<Sign, Form> checked_add(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
```

Returns `{a + b, overflow_flag}`. The result value is the wrapped result even on overflow.

### `checked_sub`

```cpp
template <signedness Sign, representation_form Form>
constexpr checked_result<Sign, Form> checked_sub(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
```

### `checked_mul`

```cpp
template <signedness Sign, representation_form Form>
constexpr checked_result<Sign, Form> checked_mul(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
```

### `checked_div`

```cpp
template <signedness Sign, representation_form Form>
constexpr checked_result<Sign, Form> checked_div(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
```

Sets `overflow = true` if `b == 0` or (signed only) if `a == min && b == -1`.

---

## Saturating Operations

### `saturating_add`

```cpp
template <signedness Sign, representation_form Form>
constexpr int128_param_t<Sign, Form> saturating_add(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
```

Returns `a + b`, clamped to `[min, max]` on overflow.

### `saturating_sub`

```cpp
template <signedness Sign, representation_form Form>
constexpr int128_param_t<Sign, Form> saturating_sub(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
```

### `saturating_mul`

```cpp
template <signedness Sign, representation_form Form>
constexpr int128_param_t<Sign, Form> saturating_mul(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
```

---

## Try Operations

Return `std::optional<T>` — `std::nullopt` on overflow.

### `try_add` / `try_sub` / `try_mul` / `try_div`

```cpp
template <signedness Sign, representation_form Form>
constexpr std::optional<int128_param_t<Sign, Form>> try_add(
    const int128_param_t<Sign, Form>& a,
    const int128_param_t<Sign, Form>& b) noexcept;
// ... same pattern for sub, mul, div
```

---

## Example

```cpp
#include "int128_param_safe.hpp"
using namespace nstd;

// Checked
const auto r1 = nstd::checked_add(uint128_t::max(), uint128_t{1});
assert(r1.overflow);  // true — would wrap

// Saturating
const auto r2 = nstd::saturating_add(uint128_t::max(), uint128_t{1});
assert(r2 == uint128_t::max());  // clamped

// Try
const auto r3 = nstd::try_add(uint128_t{100}, uint128_t{200});
assert(r3.has_value());
assert(*r3 == uint128_t{300});

const auto r4 = nstd::try_div(uint128_t{100}, uint128_t{0});
assert(!r4.has_value());  // division by zero
```
