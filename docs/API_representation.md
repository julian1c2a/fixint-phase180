# API Reference — representation.hpp

> Enumerations and conversion functions for 128-bit integer representation forms.

## Synopsis

```cpp
#include "representation.hpp"

namespace nstd {

enum class signedness : bool;
enum class representation_form : std::uint8_t;

template <representation_form Form>
struct representation_traits;

// 64-bit conversions
constexpr std::uint64_t ms_to_twos_complement(std::uint64_t) noexcept;
constexpr std::uint64_t twos_complement_to_ms(std::uint64_t) noexcept;

// 128-bit conversions (6 functions)
constexpr void ms128_to_twos_complement(uint64_t, uint64_t, uint64_t&, uint64_t&) noexcept;
constexpr void twos_complement128_to_ms(uint64_t, uint64_t, uint64_t&, uint64_t&) noexcept;
constexpr void twos_complement128_to_excess_k(uint64_t, uint64_t, uint64_t&, uint64_t&) noexcept;
constexpr void excess_k128_to_twos_complement(uint64_t, uint64_t, uint64_t&, uint64_t&) noexcept;
constexpr void ms128_to_excess_k(uint64_t, uint64_t, uint64_t&, uint64_t&) noexcept;
constexpr void excess_k128_to_ms(uint64_t, uint64_t, uint64_t&, uint64_t&) noexcept;

}
```

---

## Enumerations

### `signedness`

```cpp
enum class signedness : bool {
    unsigned_type = false,
    signed_type   = true
};
```

Controls whether the 128-bit type is signed or unsigned.

### `representation_form`

```cpp
enum class representation_form : std::uint8_t {
    binnat           = 0,  // Binary natural (unsigned only)
    twos_complement  = 1,  // Two's complement (signed)
    magnitude_sign   = 2,  // Sign-magnitude (signed)
    excess_k         = 3   // Excess-K / biased (signed, K=2^126)
};
```

| Form | Description | Range | Notes |
|------|-------------|-------|-------|
| `binnat` | Unsigned binary | [0, 2^128-1] | Standard unsigned |
| `twos_complement` | Two's complement | [-2^127, 2^127-1] | Hardware-native signed |
| `magnitude_sign` | Sign + magnitude | [-(2^127-1), 2^127-1] | Has ±0 |
| `excess_k` | Biased by K=2^126 | [-2^126, 2^127-1] | Asymmetric range |

---

## `representation_traits<Form>`

Compile-time metadata about each representation.

```cpp
template <representation_form Form>
struct representation_traits;
```

### Static Members

| Member | Type | Description |
|--------|------|-------------|
| `name` | `const char*` | Human-readable name |
| `has_implicit_sign_bit` | `bool` | True for TC |
| `uses_inversion` | `bool` | True for TC (negation = invert + 1) |
| `hardware_optimized` | `bool` | True for TC |
| `min_i64` / `max_i64` | `int64_t` | 64-bit subrange limits |
| `has_two_zeros` | `bool` | True for MS (+0 and -0) |
| `uses_bias` | `bool` | True for EK |
| `default_bias_high` | `uint64_t` | EK: `1ULL << 62` |
| `default_bias_low` | `uint64_t` | EK: `0` |

---

## Conversion Functions

### 64-bit

```cpp
inline constexpr std::uint64_t ms_to_twos_complement(std::uint64_t ms_val) noexcept;
inline constexpr std::uint64_t twos_complement_to_ms(std::uint64_t tc_val) noexcept;
```

### 128-bit

All take `(high_in, low_in, high_out&, low_out&)`:

| Function | From | To |
|----------|------|----|
| `ms128_to_twos_complement` | MS | TC |
| `twos_complement128_to_ms` | TC | MS |
| `twos_complement128_to_excess_k` | TC | EK |
| `excess_k128_to_twos_complement` | EK | TC |
| `ms128_to_excess_k` | MS | EK |
| `excess_k128_to_ms` | EK | MS |

```cpp
uint64_t out_hi{}, out_lo{};
nstd::twos_complement128_to_excess_k(in_hi, in_lo, out_hi, out_lo);
```
