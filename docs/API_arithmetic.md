# API Reference — int128_param_arithmetic.hpp

> Extended arithmetic operations: widening multiply, mulhi, and 256-bit results.

## Synopsis

```cpp
#include "int128_param_arithmetic.hpp"

namespace nstd {

// 256-bit result type (4 little-endian 64-bit limbs)
using uint256_t = algorithms::uint256_result;

// Full 128×128→256 multiplication (Karatsuba algorithm, 3 multiplications)
inline uint256_t widening_mul(const uint128_t& a, const uint128_t& b) noexcept;

// Upper 128 bits of 128×128 product: (a * b) >> 128
inline uint128_t mulhi(const uint128_t& a, const uint128_t& b) noexcept;

// Lower 128 bits of 128×128 product: (a * b) & ((1<<128)-1)
inline uint128_t mullo(const uint128_t& a, const uint128_t& b) noexcept;

}
```

---

## Types

### `uint256_t`

A 256-bit unsigned integer stored as 4 little-endian 64-bit limbs.

```cpp
struct uint256_result {
    uint64_t limbs[4];  // [0]=least significant, [3]=most significant

    uint128_t low128() const noexcept;   // limbs[0..1]
    uint128_t high128() const noexcept;  // limbs[2..3]
};

using uint256_t = uint256_result;
```

---

## Functions

### `widening_mul`

```cpp
inline uint256_t widening_mul(const uint128_t& a, const uint128_t& b) noexcept;
```

**Brief:** Compute the full 256-bit product of two 128-bit unsigned values.

**Algorithm:** Karatsuba sub-quadratic multiplication (3 multiplications instead of schoolbook 4).

**Parameters:**

- `a` — First 128-bit operand
- `b` — Second 128-bit operand

**Return value:** Complete 256-bit product.

**Example:**

```cpp
const uint128_t a{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // MAX
const uint128_t b{uint64_t{2}, uint64_t{0}};
const auto result{nstd::widening_mul(a, b)};
// result.low128()  == MAX - 1
// result.high128() == 1
```

---

### `mulhi`

```cpp
inline uint128_t mulhi(const uint128_t& a, const uint128_t& b) noexcept;
```

**Brief:** Compute the upper 128 bits of a 128×128 multiplication.

**Equivalent to:** `(a * b) >> 128` with 256-bit precision.

**Use cases:**

- Granlund-Montgomery division by constants
- Fixed-point arithmetic
- Overflow detection

**Example:**

```cpp
const uint128_t a{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // MAX
const uint128_t b{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // MAX
const auto hi{nstd::mulhi(a, b)};
// hi == MAX - 1  (upper half of MAX * MAX)
```

---

### `mullo`

```cpp
inline uint128_t mullo(const uint128_t& a, const uint128_t& b) noexcept;
```

**Brief:** Compute the lower 128 bits of a 128×128 multiplication.

**Equivalent to:** `a * b` (same as `operator*`, provided for API symmetry with `mulhi`).

---

## Performance

Uses the Karatsuba algorithm internally, which requires 3 base multiplications (64×64→128) instead of the schoolbook 4. This is also used internally by `div_by_const.hpp` for the Granlund-Montgomery division-by-constant optimization.

**Benchmark (GCC -O2):** `mulhi` ~3.87 cycles/op, matching `__int128` native multiplication performance.

---

## Related

- `int128_parameterized.hpp` — Core `uint128_t` type and `operator*`
- `algorithms/karatsuba.hpp` — Internal Karatsuba implementation
- `algorithms/div_by_const.hpp` — Uses `mulhi` for division by constants
