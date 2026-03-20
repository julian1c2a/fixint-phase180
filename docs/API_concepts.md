# API Reference — int128_param_concepts.hpp

> C++20 concepts and type classification traits for 128-bit integer types.

## Synopsis

```cpp
#include "int128_param_concepts.hpp"

namespace nstd {

// Type classification
template <typename T> concept int128_type;
template <typename T> concept uint128_type;
template <typename T> concept signed_int128_type;
template <typename T> concept int128_tc_type;
template <typename T> concept int128_ms_type;
template <typename T> concept int128_ek_type;

// Interoperability
template <typename T> concept int128_convertible;
template <typename T> concept int128_compatible;
template <typename T> concept integral;

// Operation-specific
template <typename T> concept shift_operand;
template <typename T> concept arithmetic_operand;
template <typename T> concept comparable_with_int128;
template <typename T> concept accumulative_type;
template <typename T> concept rotatable;

// Predicate/Iterator/Range
template <typename Pred, typename T> concept int128_predicate;
template <typename Iter>             concept int128_iterator;
template <typename Range>            concept int128_range;
template <typename Container>        concept int128_container;

}
```

---

## Type Classification Concepts

### `int128_type`

```cpp
template <typename T>
concept int128_type = /* T is any int128_param_t instantiation */;
```

Matches any of the four int128 types (`uint128_t`, `int128_tc_t`, `int128_ms_t`, `int128_ek_t`).

### `uint128_type`

```cpp
template <typename T>
concept uint128_type = /* T is int128_param_t<unsigned_type, binnat> */;
```

### `signed_int128_type`

```cpp
template <typename T>
concept signed_int128_type = /* T is any signed int128_param_t */;
```

Matches `int128_tc_t`, `int128_ms_t`, or `int128_ek_t`.

### `int128_tc_type` / `int128_ms_type` / `int128_ek_type`

```cpp
template <typename T> concept int128_tc_type = /* T is twos_complement */;
template <typename T> concept int128_ms_type = /* T is magnitude_sign */;
template <typename T> concept int128_ek_type = /* T is excess_k */;
```

---

## Interoperability Concepts

### `int128_convertible`

```cpp
template <typename T>
concept int128_convertible = int128_type<T> || std::integral<T>;
```

Any type that can be converted to/from int128.

### `int128_compatible`

```cpp
template <typename T>
concept int128_compatible = int128_type<T> || std::integral<T>;
```

Types that can participate in mixed int128 arithmetic.

### `integral`

```cpp
template <typename T>
concept integral = std::integral<T> || int128_type<T>;
```

Extended integral concept that includes 128-bit types.

### `int128_bitwise_compatible`

```cpp
template <typename T>
concept int128_bitwise_compatible = int128_type<T> || std::unsigned_integral<T>;
```

### `int128_signed_compatible` / `int128_unsigned_compatible`

```cpp
template <typename T>
concept int128_signed_compatible = signed_int128_type<T> || std::signed_integral<T>;

template <typename T>
concept int128_unsigned_compatible = uint128_type<T> || std::unsigned_integral<T>;
```

---

## Operation-Specific Concepts

### `shift_operand`

```cpp
template <typename T>
concept shift_operand = std::integral<T>;
```

Valid right-hand operand for shift operators.

### `arithmetic_operand`

```cpp
template <typename T>
concept arithmetic_operand = std::integral<T> || int128_type<T>;
```

### `comparable_with_int128`

```cpp
template <typename T>
concept comparable_with_int128 = std::integral<T> || int128_type<T>;
```

### `accumulative_type`

```cpp
template <typename T>
concept accumulative_type = int128_type<T>;
```

Types that can serve as accumulator in reduction operations.

### `rotatable`

```cpp
template <typename T>
concept rotatable = int128_type<T>;
```

---

## Iterator/Range/Container Concepts

### `int128_predicate`

```cpp
template <typename Pred, typename T>
concept int128_predicate = std::predicate<Pred, T> && int128_type<T>;
```

### `int128_iterator`

```cpp
template <typename Iter>
concept int128_iterator = std::input_iterator<Iter> && int128_type<std::iter_value_t<Iter>>;
```

### `int128_range`

```cpp
template <typename Range>
concept int128_range = std::ranges::range<Range> && int128_type<std::ranges::range_value_t<Range>>;
```

### `int128_container`

```cpp
template <typename Container>
concept int128_container = requires(Container c) { /* begin, end, size, value_type */ };
```

---

## Variable Templates (Type Traits)

```cpp
template <typename T> inline constexpr bool is_128bit_type_v;
template <typename T> inline constexpr bool is_uint128_v;
template <typename T> inline constexpr bool is_int128_tc_v;
template <typename T> inline constexpr bool is_int128_ms_v;
template <typename T> inline constexpr bool is_int128_ek_v;
template <typename T> inline constexpr bool is_signed_int128_v;
```

### Usage

```cpp
static_assert(nstd::int128_type<nstd::uint128_t>);
static_assert(nstd::signed_int128_type<nstd::int128_tc_t>);
static_assert(!nstd::signed_int128_type<nstd::uint128_t>);
static_assert(nstd::integral<nstd::uint128_t>);
static_assert(nstd::integral<int>);
static_assert(nstd::is_uint128_v<nstd::uint128_t>);
```
