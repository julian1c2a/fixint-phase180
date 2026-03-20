# API Reference — int128_parameterized.hpp

> Core template class for 128-bit integers with configurable signedness and representation.

## Synopsis

```cpp
#include "int128_parameterized.hpp"

namespace nstd {

template <signedness Sign, representation_form Form>
class int128_param_t;

// Type aliases
using uint128_t    = int128_param_t<unsigned_type, binnat>;
using int128_t     = int128_param_t<signed_type, twos_complement>;
using int128_tc_t  = int128_param_t<signed_type, twos_complement>;
using int128_ms_t  = int128_param_t<signed_type, magnitude_sign>;
using int128_ek_t  = int128_param_t<signed_type, excess_k>;
using uint128_bn_t = uint128_t;

}  // namespace nstd
```

---

## Class Template `int128_param_t<Sign, Form>`

### Template Parameters

| Parameter | Type | Values |
|-----------|------|--------|
| `Sign` | `signedness` | `unsigned_type`, `signed_type` |
| `Form` | `representation_form` | `binnat`, `twos_complement`, `magnitude_sign`, `excess_k` |

### Valid Instantiations

| Alias | Sign | Form | Range |
|-------|------|------|-------|
| `uint128_t` | unsigned | binnat | [0, 2^128-1] |
| `int128_tc_t` | signed | twos_complement | [-2^127, 2^127-1] |
| `int128_ms_t` | signed | magnitude_sign | [-(2^127-1), 2^127-1] |
| `int128_ek_t` | signed | excess_k (K=2^126) | [-2^126, 2^127-1] |

### Static Constants

```cpp
static constexpr signedness sign;
static constexpr representation_form form;
static constexpr bool is_signed;
static constexpr bool is_binnat;
static constexpr bool is_twos_complement;
static constexpr bool is_magnitude_sign;
static constexpr bool is_excess_k;
static constexpr int BITS  = 128;
static constexpr int BYTES = 16;
```

---

## Static Methods

### `max()`

```cpp
static constexpr int128_param_t max() noexcept;
```

Returns the maximum representable value for the type.

### `min()`

```cpp
static constexpr int128_param_t min() noexcept;
```

Returns the minimum representable value for the type.

### `from_string()`

```cpp
static constexpr int128_param_t from_string(const char* str);
```

Parses a string into an int128 value. Supports decimal, hex (`0x`), octal (`0`), binary (`0b`).

**Throws:** `std::invalid_argument` if parsing fails, `std::out_of_range` if value overflows.

### `parse_ct_safe()`

```cpp
static constexpr parse_result<int128_param_t> parse_ct_safe(const char* str) noexcept;
```

Compile-time safe parsing — returns a `parse_result` (no exceptions).

```cpp
constexpr auto result = uint128_t::parse_ct_safe("123456789");
if (result.success()) {
    // use result.value
}
```

### `from_big_endian()` / `from_little_endian()`

```cpp
static constexpr int128_param_t from_big_endian(const std::array<std::byte, 16>& bytes) noexcept;
static constexpr int128_param_t from_little_endian(const std::array<std::byte, 16>& bytes) noexcept;
```

Construct from raw bytes in specified endianness.

---

## Constructors

### Default

```cpp
constexpr int128_param_t() noexcept;
```

Initializes to zero.

### Copy / Move

```cpp
constexpr int128_param_t(const int128_param_t&) noexcept = default;
constexpr int128_param_t(int128_param_t&&) noexcept = default;
```

### Cross-Representation

```cpp
template <signedness S2, representation_form F2>
explicit constexpr int128_param_t(const int128_param_t<S2, F2>& other) noexcept;

template <signedness S2, representation_form F2>
explicit constexpr int128_param_t(int128_param_t<S2, F2>&& other) noexcept;
```

Converts between any two int128 representations.

```cpp
const int128_tc_t tc{42, 0};
const int128_ms_t ms{tc};       // explicit cross-repr conversion
const int128_ek_t ek{tc};       // TC -> EK conversion
```

### From High/Low Pair

```cpp
template <typename T1, typename T2>
explicit constexpr int128_param_t(T1 high, T2 low) noexcept;
```

**Note:** The order is `(high, low)` — `high` is the most significant 64 bits.

```cpp
const uint128_t x{0x0, 0x2};           // value = 2
const uint128_t y{0x1, 0x0};           // value = 2^64
const int128_tc_t z{0x8000000000000000ULL, 0x0};  // min value
```

### From Integral

```cpp
template <typename T>
explicit constexpr int128_param_t(T value) noexcept;
```

Constructs from any integral type. Sign-extends for signed T into signed representations.

### From Floating-Point

```cpp
explicit constexpr int128_param_t(float value) noexcept;
explicit constexpr int128_param_t(double value) noexcept;
explicit constexpr int128_param_t(long double value) noexcept;
```

### From Bytes / Bitset

```cpp
explicit constexpr int128_param_t(const std::array<std::byte, 16>& bytes) noexcept;
explicit constexpr int128_param_t(const std::bitset<128>& bits) noexcept;
```

---

## Assignment Operators

### Default

```cpp
constexpr int128_param_t& operator=(const int128_param_t&) noexcept = default;
constexpr int128_param_t& operator=(int128_param_t&&) noexcept = default;
```

### Cross-Representation

```cpp
template <signedness S2, representation_form F2>
constexpr int128_param_t& operator=(const int128_param_t<S2, F2>& other) noexcept;

template <signedness S2, representation_form F2>
constexpr int128_param_t& operator=(int128_param_t<S2, F2>&& other) noexcept;
```

### From Integral / Floating-Point

```cpp
template <typename T>
constexpr int128_param_t& operator=(T value) noexcept;  // integral

int128_param_t& operator=(float value) noexcept;
int128_param_t& operator=(double value) noexcept;
int128_param_t& operator=(long double value) noexcept;
```

---

## Accessors

```cpp
constexpr uint64_t high() const noexcept;  // Most significant 64 bits
constexpr uint64_t low() const noexcept;   // Least significant 64 bits

template <typename T>
constexpr void set_high(T value) noexcept;

template <typename T>
constexpr void set_low(T value) noexcept;
```

---

## Conversion Operators

### To bool

```cpp
[[nodiscard]] explicit constexpr operator bool() const noexcept;
```

Returns `true` if the value is non-zero.

### To Integral

```cpp
template <typename T>
[[nodiscard]] explicit constexpr operator T() const noexcept;
```

Truncates to `T`. For signed representations, extracts the real value first.

### To Floating-Point

```cpp
[[nodiscard]] explicit constexpr operator double() const noexcept;
[[nodiscard]] explicit constexpr operator long double() const noexcept;
```

### Cross-Representation Cast

```cpp
template <signedness S2, representation_form F2>
[[nodiscard]] explicit constexpr operator int128_param_t<S2, F2>() const noexcept;
```

### To Bytes / Bitset

```cpp
explicit constexpr operator std::array<std::byte, 16>() const noexcept;
explicit constexpr operator std::bitset<128>() const noexcept;
```

---

## Utility Methods

```cpp
[[nodiscard]] constexpr bool is_negative() const noexcept;
[[nodiscard]] constexpr bool is_zero() const noexcept;
[[nodiscard]] constexpr bool is_positive_zero() const noexcept;
[[nodiscard]] constexpr bool is_negative_zero() const noexcept;  // MS only
[[nodiscard]] constexpr int get_sign() const noexcept;           // -1, 0, +1
[[nodiscard]] constexpr int128_param_t magnitude() const noexcept;
constexpr int128_param_t abs() const noexcept;

std::string to_string(int base = 10) const noexcept;

constexpr void swap(int128_param_t& other) noexcept;
friend constexpr void swap(int128_param_t& a, int128_param_t& b) noexcept;

[[nodiscard]] constexpr int128_param_t incr() const noexcept;  // pure +1
[[nodiscard]] constexpr int128_param_t decr() const noexcept;  // pure -1
```

---

## Comparison Operators

```cpp
constexpr bool operator==(const int128_param_t& other) const noexcept;
constexpr bool operator!=(const int128_param_t& other) const noexcept;
constexpr bool operator< (const int128_param_t& other) const noexcept;
constexpr bool operator> (const int128_param_t& other) const noexcept;
constexpr bool operator<=(const int128_param_t& other) const noexcept;
constexpr bool operator>=(const int128_param_t& other) const noexcept;
```

### Mixed Comparison with Integrals

```cpp
// Both directions: int128 op T and T op int128
friend constexpr bool operator==(const int128_param_t&, T) noexcept;
friend constexpr bool operator==(T, const int128_param_t&) noexcept;
// ... same for !=, <, <=, >, >=
```

---

## Arithmetic Operators

### Unary

```cpp
constexpr int128_param_t operator+() const noexcept;   // identity
constexpr int128_param_t operator-() const noexcept;   // negation

constexpr int128_param_t& operator++() noexcept;       // pre-increment
constexpr int128_param_t  operator++(int) noexcept;    // post-increment
constexpr int128_param_t& operator--() noexcept;       // pre-decrement
constexpr int128_param_t  operator--(int) noexcept;    // post-decrement
```

### Binary (same type)

```cpp
constexpr int128_param_t  operator+(const int128_param_t&) const noexcept;
constexpr int128_param_t& operator+=(const int128_param_t&) noexcept;

constexpr int128_param_t  operator-(const int128_param_t&) const noexcept;
constexpr int128_param_t& operator-=(const int128_param_t&) noexcept;

// *, /, % — deleted for excess_k
constexpr int128_param_t  operator*(const int128_param_t&) const noexcept requires(!is_excess_k);
constexpr int128_param_t& operator*=(const int128_param_t&) noexcept requires(!is_excess_k);

constexpr int128_param_t  operator/(const int128_param_t&) const noexcept requires(!is_excess_k);
constexpr int128_param_t& operator/=(const int128_param_t&) noexcept requires(!is_excess_k);

constexpr int128_param_t  operator%(const int128_param_t&) const noexcept requires(!is_excess_k);
constexpr int128_param_t& operator%=(const int128_param_t&) noexcept requires(!is_excess_k);
```

### Compound Assignment with Integrals

```cpp
template <typename T> constexpr int128_param_t& operator+=(T rhs) noexcept;
template <typename T> constexpr int128_param_t& operator-=(T rhs) noexcept;
template <typename T> constexpr int128_param_t& operator*=(T rhs) noexcept requires(!is_excess_k);
template <typename T> constexpr int128_param_t& operator/=(T rhs) noexcept requires(!is_excess_k);
template <typename T> constexpr int128_param_t& operator%=(T rhs) noexcept requires(!is_excess_k);
```

### Friend Mixed Arithmetic (int128 op T, T op int128)

```cpp
friend constexpr int128_param_t operator+(const int128_param_t&, T) noexcept;
friend constexpr int128_param_t operator+(T, const int128_param_t&) noexcept;
// ... same pattern for -, *, /, %
// *, /, % have requires(!is_excess_k)
```

### Division with Remainder

```cpp
[[nodiscard]] constexpr std::pair<int128_param_t, int128_param_t>
    divmod(const int128_param_t& divisor) const noexcept requires(!is_excess_k);
```

Returns `{quotient, remainder}`.

---

## Bitwise Operators

```cpp
constexpr int128_param_t  operator&(const int128_param_t&) const noexcept;
constexpr int128_param_t& operator&=(const int128_param_t&) noexcept;
constexpr int128_param_t  operator|(const int128_param_t&) const noexcept;
constexpr int128_param_t& operator|=(const int128_param_t&) noexcept;
constexpr int128_param_t  operator^(const int128_param_t&) const noexcept;
constexpr int128_param_t& operator^=(const int128_param_t&) noexcept;
constexpr int128_param_t  operator~() const noexcept;
```

### Friend Mixed Bitwise

```cpp
friend constexpr int128_param_t operator&(const int128_param_t&, T) noexcept;
friend constexpr int128_param_t operator&(T, const int128_param_t&) noexcept;
// ... same for |, ^
```

---

## Shift Operators

```cpp
constexpr int128_param_t  operator<<(int shift) const noexcept;
constexpr int128_param_t& operator<<=(int shift) noexcept;
constexpr int128_param_t  operator>>(int shift) const noexcept;  // arithmetic for signed TC, logical otherwise
constexpr int128_param_t& operator>>=(int shift) noexcept;

// Template versions for other integral shift types
template <typename T> constexpr int128_param_t operator<<(T shift) const noexcept;
template <typename T> constexpr int128_param_t operator>>(T shift) const noexcept;
```

---

## Bit Manipulation Methods

```cpp
constexpr int trailing_zeros() const noexcept;
constexpr int leading_zeros() const noexcept;
constexpr int count_ones() const noexcept;
constexpr int popcount() const noexcept;          // alias for count_ones
constexpr bool is_power_of_2() const noexcept;
constexpr int bit_width() const noexcept;
constexpr int128_param_t rotate_left(int s) const noexcept;
constexpr int128_param_t rotate_right(int s) const noexcept;
constexpr int128_param_t reverse_bits() const noexcept;
```

---

## Byte Operations and Endianness

```cpp
constexpr std::byte get_byte(size_t index) const;       // throws std::out_of_range
constexpr void set_byte(size_t index, std::byte value);  // throws std::out_of_range
constexpr int128_param_t byteswap() const noexcept;
constexpr std::array<std::byte, 16> to_big_endian() const noexcept;
constexpr std::array<std::byte, 16> to_little_endian() const noexcept;
```

---

## Parsing Types

### `parse_error`

```cpp
enum class parse_error : uint8_t {
    success, null_pointer, empty_string, invalid_base, invalid_base_value,
    invalid_character, digit_out_of_range, no_digits, overflow,
    separator_at_boundaries, unknown_error
};
```

### `parse_result<T>`

```cpp
template <typename T>
struct parse_result {
    parse_error error;
    T value;
    size_t error_index;
    constexpr bool success() const noexcept;
};
```

---

## Example

```cpp
#include "int128_parameterized.hpp"
using namespace nstd;

constexpr uint128_t a{100};
constexpr int128_tc_t b{-42};
constexpr int128_ms_t c{b};  // cross-repr conversion

static_assert(a + uint128_t{1} == uint128_t{101});
static_assert(b.is_negative());
static_assert(!a.is_zero());

const auto [q, r] = a.divmod(uint128_t{7});
// q == 14, r == 2

const std::string s = a.to_string();  // "100"
```
