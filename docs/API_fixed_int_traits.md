# API Reference — fixed_int traits, concepts, and limits

> Type traits, `std::common_type`, `std::numeric_limits`, and C++20 concepts for
> `nstd::fixed_int_t<N, Sign, Form>`. Companion to [API_fixed_int.md](API_fixed_int.md).

## Synopsis

```cpp
#include "fixed_int_traits_specializations.hpp"   // traits + std::common_type
#include "fixed_int_concepts.hpp"                  // concepts
#include "fixed_int_limits.hpp"                    // std::numeric_limits

namespace nstd {

// nstd:: trait templates (specialized for fixed_int_t)
template <typename T> struct is_integral;
template <typename T> struct is_arithmetic;
template <typename T> struct is_signed;
template <typename T> struct is_unsigned;
template <typename T> struct make_signed   { using type = /* ... */; };
template <typename T> struct make_unsigned { using type = /* ... */; };

template <typename T> inline constexpr bool is_integral_v;
template <typename T> inline constexpr bool is_arithmetic_v;
template <typename T> inline constexpr bool is_signed_v;
template <typename T> inline constexpr bool is_unsigned_v;
template <typename T> using make_signed_t   = typename make_signed<T>::type;
template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;

// Concepts
template <typename T> concept fixed_int_type;          // detection
template <typename T> concept signed_fixed_int_type;
template <typename T> concept unsigned_fixed_int_type;
template <typename T> concept integral;                // built-in ∪ fixed_int_t (excludes bool)
template <typename T> concept signed_integral;
template <typename T> concept unsigned_integral;

}  // namespace nstd

namespace std {
// Specializations allowed by the standard:
template <size_t N, ::nstd::signedness Sign, ::nstd::representation_form Form>
class numeric_limits<::nstd::fixed_int_t<N, Sign, Form>>;

template <size_t N, size_t M>
struct common_type<::nstd::int_fixed_t<N>,  ::nstd::int_fixed_t<M>>;
template <size_t N, size_t M>
struct common_type<::nstd::uint_fixed_t<N>, ::nstd::uint_fixed_t<M>>;
template <size_t N, size_t M>
struct common_type<::nstd::int_fixed_t<N>,  ::nstd::uint_fixed_t<M>>;
template <size_t N, size_t M>
struct common_type<::nstd::uint_fixed_t<N>, ::nstd::int_fixed_t<M>>;
template <size_t N, ::nstd::signedness S, ::nstd::representation_form F, class T>
struct common_type<::nstd::fixed_int_t<N, S, F>, T>;
template <class T, size_t N, ::nstd::signedness S, ::nstd::representation_form F>
struct common_type<T, ::nstd::fixed_int_t<N, S, F>>;
}
```

---

## Why `nstd::` for traits, `std::` for `common_type` / `numeric_limits`?

The C++ standard explicitly *prohibits* user specializations of:

- `std::is_integral`, `std::is_arithmetic`, `std::is_signed`, `std::is_unsigned`
- `std::make_signed`, `std::make_unsigned` (behavior is unspecified for non-integral T)

It explicitly *allows* user specializations of:

- `std::common_type` (§22.10.7.6)
- `std::numeric_limits` (§17.6.4.2.1)
- `std::hash`

We follow this exactly. `nstd::is_*` and `nstd::make_*` are user-namespace twins of the std:: traits that delegate to `std::` for built-in types and add fixed_int_t support; `std::common_type` and `std::numeric_limits` use the standard's permitted extension points.

---

## `nstd::` Type Traits

### `is_integral` / `is_arithmetic` / `is_signed` / `is_unsigned`

```cpp
template <typename T> struct is_integral    : std::is_integral<T>   {};   // primary
template <typename T> struct is_arithmetic  : std::is_arithmetic<T> {};
template <typename T> struct is_signed      : std::is_signed<T>     {};
template <typename T> struct is_unsigned    : std::is_unsigned<T>   {};
```

Plus partial specializations for `fixed_int_t<N, Sign, Form>`:

| Trait | `uint_fixed_t<N>` | `int_fixed_t<N>` | built-in `unsigned` | built-in `int` |
|---|---|---|---|---|
| `nstd::is_integral_v` | true | true | true | true |
| `nstd::is_arithmetic_v` | true | true | true | true |
| `nstd::is_signed_v` | false | true | false | true |
| `nstd::is_unsigned_v` | true | false | true | false |

### `make_signed_t<T>` / `make_unsigned_t<T>`

| Input | `make_signed_t` | `make_unsigned_t` |
|---|---|---|
| `uint_fixed_t<N>` | `int_fixed_t<N>` | `uint_fixed_t<N>` (identity) |
| `int_fixed_t<N>` | `int_fixed_t<N>` (identity) | `uint_fixed_t<N>` |
| `unsigned` | `int` | `unsigned` (identity) |
| `int` | `int` (identity) | `unsigned` |

The choice of `int_fixed_t<N>` (rather than some MS or EK variant) reflects the canonical signed alias; it matches the default `Form` selection for `Sign = signed_type`.

---

## `nstd::` Concepts

### Detection concepts (always available, collision-free)

```cpp
template <typename T> concept fixed_int_type           = is_fixed_int_v<T>;
template <typename T> concept signed_fixed_int_type    = is_signed_fixed_int_v<T>;
template <typename T> concept unsigned_fixed_int_type  = is_unsigned_fixed_int_v<T>;
```

### Aglutinating concepts (built-in ∪ `fixed_int_t`)

```cpp
template <typename T>
concept integral = (std::integral<T> && !std::is_same_v<std::remove_cv_t<T>, bool>)
                || fixed_int_type<T>;

template <typename T>
concept signed_integral = std::signed_integral<T> || signed_fixed_int_type<T>;

template <typename T>
concept unsigned_integral = (std::unsigned_integral<T> && !std::is_same_v<std::remove_cv_t<T>, bool>)
                         || unsigned_fixed_int_type<T>;
```

Bool is excluded by design from `integral` and `unsigned_integral` (matches the convention of typical arithmetic concepts).

### Why not specialize `std::is_integral`?

The standard prohibits it. So `std::integral<fixed_int_t<2>>` is **false** and there is no way to change that without UB. Code that needs to accept both built-ins and `fixed_int_t` must use `nstd::integral` instead of `std::integral`.

### Conflict with `int128_param_concepts.hpp`

Both `int128_param_concepts.hpp` and `fixed_int_concepts.hpp` define `nstd::integral`, with different bodies. Including both in the same TU would be an ODR violation. The fixed-int header is guarded:

```cpp
#ifndef INT128_PARAM_CONCEPTS_HPP
// our nstd::integral / signed_integral / unsigned_integral
#endif
```

If `int128_param_concepts.hpp` is included first, **the int128 version wins** — that one covers built-ins + `int128_param_t` but *not* `fixed_int_t`. In that mixed-usage scenario, use `nstd::fixed_int_type` (the detection concept) directly in generic code.

---

## `std::common_type`

```cpp
std::common_type_t<int_fixed_t<N>,  int_fixed_t<M>>   = int_fixed_t<max(N, M)>
std::common_type_t<uint_fixed_t<N>, uint_fixed_t<M>>  = uint_fixed_t<max(N, M)>
std::common_type_t<int_fixed_t<N>,  uint_fixed_t<M>>  = mixed_iu_t<N, M>
std::common_type_t<uint_fixed_t<N>, int_fixed_t<M>>   = mixed_iu_t<M, N>
std::common_type_t<fixed_int_t<N,S,F>, T>             = fixed_int_t<N,S,F>   // T = built-in integral
std::common_type_t<T, fixed_int_t<N,S,F>>             = fixed_int_t<N,S,F>
```

The fixed_int_t × built-in cases keep the fixed_int_t side because it is always wider; this matches built-in `common_type_t<long long, int>` style behavior.

Built-in `bool` is rejected via `static_assert` in the specialization body.

---

## `std::numeric_limits<fixed_int_t<N, Sign, Form>>`

One partial specialization covers **all** combinations of (N, Sign, Form) — the body branches on `is_signed` via `if constexpr`.

### Static Member Constants

| Member | uint (any N) | int (any N) |
|---|---|---|
| `is_specialized` | true | true |
| `is_signed` | false | true |
| `is_integer` | true | true |
| `is_exact` | true | true |
| `is_bounded` | true | true |
| `is_modulo` | true (wraps) | false (overflow UB) |
| `has_infinity` | false | false |
| `has_quiet_NaN` | false | false |
| `has_signaling_NaN` | false | false |
| `has_denorm` | denorm_absent | denorm_absent |
| `radix` | 2 | 2 |
| `digits` | `64 * N` | `64 * N - 1` (excludes sign bit) |
| `digits10` | `(digits * 30103) / 100000` | `(digits * 30103) / 100000` |
| `max_digits10` | 0 | 0 |
| `traps` | false | false |

### `digits10` values

| N | `uint_fixed_t<N>::digits10` | `int_fixed_t<N>::digits10` |
|---|---|---|
| 1 | 19 (= `uint64_t`) | 18 (= `int64_t`) |
| 2 | 38 | 38 |
| 4 | 77 | 76 |
| 8 | 154 | 153 |

### Static Member Functions

```cpp
static constexpr T min() noexcept;   // 0 if unsigned, -2^(64N-1) if signed
static constexpr T max() noexcept;   // 2^(64N) - 1 if unsigned, 2^(64N-1) - 1 if signed
static constexpr T lowest() noexcept;          // == min()
static constexpr T epsilon() noexcept;         // = 0
static constexpr T round_error() noexcept;     // = 0
static constexpr T infinity() noexcept;        // = 0
static constexpr T quiet_NaN() noexcept;       // = 0
static constexpr T signaling_NaN() noexcept;   // = 0
static constexpr T denorm_min() noexcept;      // = 0
```

`min()`/`max()` delegate to `fixed_int_t<N, Sign, Form>::min()` / `::max()` static members.

### Future Forms (MS, EK)

When `fixed_int_t` gains `magnitude_sign` or `excess_k` Forms, the partial specialization above already covers them — no new specialization is required *unless* the bit layout of `min()` / `max()` for the new Form differs from the static methods on the class. In that case, add a Form-specific partial specialization.

---

## Usage Examples

### Generic function accepting built-in or fixed_int_t

```cpp
#include "fixed_int_concepts.hpp"

template <nstd::integral T>
constexpr T add_one(T x) { return x + T{1}; }

add_one(41);                          // → int 42
add_one(nstd::uint_fixed_t<2>{41});   // → uint_fixed_t<2> 42
add_one(nstd::int_fixed_t<4>{-1});    // → int_fixed_t<4> 0
```

### Cross-type promotion via common_type

```cpp
#include "fixed_int_traits_specializations.hpp"

template <typename A, typename B>
constexpr auto add(A a, B b) -> std::common_type_t<A, B>
{
    using R = std::common_type_t<A, B>;
    return R{a} + R{b};
}

add(nstd::int_fixed_t<2>{5}, nstd::uint_fixed_t<4>{3});  // returns uint_fixed_t<4>{8}
```

### Numeric limits

```cpp
#include "fixed_int_limits.hpp"

using u4 = nstd::uint_fixed_t<4>;
constexpr auto max256 = std::numeric_limits<u4>::max();  // 2^256 - 1
constexpr bool has_inf = std::numeric_limits<u4>::has_infinity;  // false
static_assert(std::numeric_limits<u4>::digits == 256);
```

---

## See Also

- [API_fixed_int.md](API_fixed_int.md) — the class, operators, and cross-sign semantics
- [API_concepts.md](API_concepts.md) — int128_param concepts (mutually exclusive header)
- [API_traits.md](API_traits.md) — int128_param trait specializations
- [API_limits.md](API_limits.md) — int128_param numeric_limits
