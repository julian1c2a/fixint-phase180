# Priority 8-11 Implementation Roadmap

**For Continuation Tomorrow**

---

## Quick Navigation

| Priority | Feature | Tests | Est. Time | Status |
|----------|---------|-------|-----------|--------|
| **P8** | Bit Manipulation | 25-30 | 2.5h | 🎯 NEXT |
| P9 | Friend Operators | 15-20 | 2.5h | ⏳ After P8 |
| P10 | Float Conversions | 8-10 | 2h | ⏳ After P9 |
| P11 | Array/Bitset Conv | 6-8 | 1.5h | ⏳ Final |

---

## Priority 8: Bit Manipulation Functions (NEXT - START HERE)

### Overview

Add 8 core bit manipulation methods with comprehensive test coverage for all representation forms (TC/MS/EK).

### Methods to Implement

#### 1. `trailing_zeros()` - Count trailing zero bits

```cpp
// Example: 0b1000 → 3 (three zeros at right)
// MS: Same as unsigned (operates on magnitude)
constexpr int trailing_zeros() const noexcept;
```

**Source Pattern:** Legacy int128_base_tt.hpp lines 743-755

#### 2. `leading_zeros()` - Count leading zero bits

```cpp
// Example: 0b00001000...0000 → 124 (rest are zeros)
// MS: Count in magnitude (127 bits, MSB is sign)
constexpr int leading_zeros() const noexcept;
```

**Source Pattern:** Legacy code lines 756-768

#### 3. `bit_width()` - Position of highest set bit

```cpp
// Example: 0b1000 → 4 (highest bit at position 4)
// MS: Count in magnitude only
constexpr int bit_width() const noexcept;
```

#### 4. `is_power_of_2()` - Check if exactly one bit set

```cpp
// Example: 8 (0b1000) → true
// Example: 6 (0b0110) → false
// MS: Check magnitude for power of 2
constexpr bool is_power_of_2() const noexcept;
```

#### 5. `count_ones()` / `popcount()` - Count set bits

```cpp
// Example: 0b1101 → 3
// MS: Count in magnitude only
constexpr int count_ones() const noexcept;
```

**Compiler Support:** Use `__builtin_popcountll` for uint64_t chunks

#### 6. `rotate_left(int shift)` - Circular left shift

```cpp
// Example: rotate_left(1) with 0x8000...0001 → 0x0000...0003
// MS: Rotate magnitude, preserve sign
constexpr int128_param_t rotate_left(int shift) const noexcept;
```

#### 7. `rotate_right(int shift)` - Circular right shift

```cpp
// Example: rotate_right(1) with 0x8000...0001 → 0x0000...0000
constexpr int128_param_t rotate_right(int shift) const noexcept;
```

#### 8. `reverse_bits()` - Reverse bit order (optional)

```cpp
// Example: 0b1000 → 0b0001 (in 128-bit context)
// May defer to P12+
constexpr int128_param_t reverse_bits() const noexcept;
```

### Implementation Strategy

**TC/Unsigned Path (Simplest):**

```cpp
constexpr int trailing_zeros() const noexcept
{
    if (data[0] != 0) {
        return __builtin_ctzll(data[0]);  // Count from low
    }
    if (data[1] != 0) {
        return 64 + __builtin_ctzll(data[1]);
    }
    return 128;  // All zeros
}
```

**MS Signed Path (Magnitude Only):**

```cpp
if constexpr (is_magnitude_sign && is_signed) {
    uint64_t mag_high = data[1] & ~(1ULL << 63);  // Strip sign bit
    // Operate on magnitude only
    // Result is same as unsigned magnitude
}
```

### Test File Structure: `tests/test_priority8_bitops.cpp`

```cpp
// Structure template:
#include <cassert>
#include "int128_parameterized.hpp"

TEST_CASE(trailing_zeros_simple)
{
    uint128_tc_t x(0, 8);  // 0x0000...0008
    ASSERT_EQ(x.trailing_zeros(), 3);
}

TEST_CASE(leading_zeros_simple)
{
    uint128_tc_t x(0, 1);  // 0x0000...0001
    ASSERT_EQ(x.leading_zeros(), 127);
}

TEST_CASE(popcount_all_ones)
{
    uint128_tc_t x(~0ULL, ~0ULL);  // All 1s
    ASSERT_EQ(x.count_ones(), 128);
}

TEST_CASE(ms_trailing_zeros_negative)
{
    int128_ms_t x(-1);  // -1 in MS = magnitude 1, sign bit set
    ASSERT_EQ(x.trailing_zeros(), 0);  // Magnitude 1 has 0 trailing zeros
}

TEST_CASE(rotate_left_simple)
{
    uint128_tc_t x(0, 1);
    auto result = x.rotate_left(1);
    ASSERT_EQ(result.high(), 2);
    ASSERT_EQ(result.low(), 0);
}
```

### Target Test Coverage (25-30 tests)

- **Trailing zeros:** 4 tests (simple, boundary, all zeros, negative)
- **Leading zeros:** 4 tests
- **Bit width:** 3 tests
- **Is power of 2:** 4 tests (true cases, false cases, edge cases)
- **Count ones:** 4 tests
- **Rotate left/right:** 6 tests (various shifts, edge cases)
- **MS-specific:** 4 tests (magnitude preservation with negative numbers)
- **Cross-type:** 2 tests (different representation forms)

### Estimated Time

- Implementation: 60 min
- Testing: 45 min
- Documentation: 15 min
- **Total: 2.5 hours**

---

## Priority 9: Friend Operators & Helper Methods

### Overview

Implement cross-type comparison operators and helper methods for efficient arithmetic.

### Methods

#### 1. Friend Comparison Operators

```cpp
// Allow comparisons: int128 vs long, long vs int128, etc.
template <typename T>
friend constexpr bool operator<(const int128_param_t& a, T b) noexcept;

template <typename T>
friend constexpr bool operator==(const int128_param_t& a, T b) noexcept;
```

#### 2. `divmod(divisor)` / `divrem_by_chunk()`

```cpp
// Efficient division by separating into chunks
constexpr std::pair<int128_param_t, int128_param_t> divmod(
    const int128_param_t& divisor) const noexcept;

// Helper for decimal conversion
static constexpr int128_param_t divrem_by_chunk(
    int128_param_t dividend, uint64_t divisor, uint64_t& remainder) noexcept;
```

### Source Pattern

Legacy code lines 1492-1606 (friend definitions and helpers)

### Test Count: 15-20 tests

### Estimated Time: 2.5 hours

---

## Priority 10: Float Conversions

### Overview

Support double/long double conversions with proper precision handling.

### Methods

#### 1. `operator double()`

```cpp
constexpr operator double() const noexcept;
```

#### 2. `operator long double()`

```cpp
constexpr operator long double() const noexcept;
```

#### 3. Constructor from double

```cpp
constexpr int128_param_t(double value) noexcept;
constexpr int128_param_t(long double value) noexcept;
```

### Considerations

- Precision loss (double = 52-bit mantissa vs 128-bit integer)
- Overflow detection (double range < int128 range)
- Negative number handling (TC vs MS semantics)
- Zero handling (MS has ±0 distinction)

### Test Count: 8-10 tests

### Estimated Time: 2 hours

---

## Priority 11: Array & Bitset Conversions

### Overview

Serialize/deserialize to/from byte arrays and bitsets.

### Methods

#### 1. `operator std::array<uint8_t, 16>()`

```cpp
constexpr operator std::array<uint8_t, 16>() const noexcept;
```

#### 2. `operator std::bitset<128>()`

```cpp
constexpr operator std::bitset<128>() const noexcept;
```

#### 3. Constructor from std::array

```cpp
constexpr int128_param_t(const std::array<uint8_t, 16>& bytes) noexcept;
constexpr int128_param_t(const std::bitset<128>& bits) noexcept;
```

### Endianness Handling

- Decide: Big-endian or little-endian?
- Document clearly for interoperability

### Test Count: 6-8 tests

### Estimated Time: 1.5 hours

---

## Implementation Checklist for Tomorrow

### Session Start (First 30 min)

- [ ] Review COMPREHENSIVE_IMPLEMENTATION_STRATEGY.md
- [ ] Open `tests/test_priority8_bitops.cpp` for editing
- [ ] Verify build system: `cmake --build build`

### P8 Main Implementation (Next 2 hours)

- [ ] Implement `trailing_zeros()` (TC path + MS path)
- [ ] Implement `leading_zeros()`
- [ ] Implement `bit_width()`, `is_power_of_2()`, `count_ones()`
- [ ] Implement `rotate_left()`, `rotate_right()`
- [ ] Create comprehensive test suite (25-30 tests)
- [ ] Run tests: `./build/tests/test_priority8_bitops.exe`

### Documentation (30 min)

- [ ] Create PRIORITY_8_COMPLETION.md
- [ ] Update PROJECT_STATUS.md (173/173 → 198/198 tests)
- [ ] Update next priority pointer

### Before Closeout

- [ ] Verify all P1-P8 tests PASSING
- [ ] Commit: "P8: Bit Manipulation Functions - 26 tests added"
- [ ] Create P9 stub file for continuation

---

## Code Template (Copy-Paste Start)

### Header Addition: `include/int128_parameterized.hpp`

```cpp
        // ========================================================================
        // Bit Manipulation Functions
        // ========================================================================

        /// @brief Count trailing zero bits
        constexpr int trailing_zeros() const noexcept
        {
            if (data[0] != 0) {
                return __builtin_ctzll(data[0]);
            }
            if (data[1] != 0) {
                return 64 + __builtin_ctzll(data[1]);
            }
            return 128;
        }

        /// @brief Count leading zero bits
        constexpr int leading_zeros() const noexcept
        {
            if (data[1] != 0) {
                if constexpr (is_magnitude_sign && is_signed) {
                    uint64_t mag = data[1] & ~(1ULL << 63);
                    return __builtin_clzll(mag) + 1;
                } else {
                    return __builtin_clzll(data[1]);
                }
            }
            return 64 + __builtin_clzll(data[0]);
        }

        // ... more methods
```

### Test Template: `tests/test_priority8_bitops.cpp`

```cpp
#include <cassert>
#include <iostream>
#include "int128_parameterized.hpp"

void test_trailing_zeros_simple()
{
    uint128_tc_t x(0, 8);
    assert(x.trailing_zeros() == 3);
    std::cout << "✓ test_trailing_zeros_simple\n";
}

int main()
{
    test_trailing_zeros_simple();
    // ... more tests
    std::cout << "\nRESULTS: X/Y PASSED\n";
    return 0;
}
```

---

## Quick Reference: Build & Test

```bash
# After modifications
cd /path/to/int128-phase175
cmake --build build              # Compile
./build/tests/test_priority8_bitops.exe  # Run P8 tests
./build/tests/test_priority7_shift.exe   # Verify P7 still works
cmake --build build --target check       # All tests
```

---

## Resources Available

1. ✅ **Legacy Code:** `legacy-code/int128-phase166/include/int128_base_tt.hpp`
   - Lines 743-830: Bit manipulation implementations
   - Use as reference for algorithms

2. ✅ **Build System:** Proven and tested
   - Auto-detects test_priority8_bitops.cpp
   - No CMakeLists.txt changes needed

3. ✅ **Test Framework:** Established pattern
   - See test_priority7_shift.cpp for structure
   - Same assertion style

4. ✅ **Type System:** All 6 variants ready
   - uint128_tc_t, int128_tc_t
   - uint128_ms_t, int128_ms_t
   - uint128_ek_t, int128_ek_t

---

## Success Criteria for P8

✅ 25-30 tests passing  
✅ All 4 representation forms tested (TC unsigned, TC signed, MS unsigned, MS signed)  
✅ Edge cases covered (0, ~0, power-of-2, etc.)  
✅ MS magnitude operations correctly isolated from sign bit  
✅ Zero compilation warnings  
✅ Documentation complete (PRIORITY_8_COMPLETION.md)  

---

**Ready for Tomorrow! 🚀**

Start with P8 - estimated 2.5 hours to complete 25-30 new tests.

Next milestone: **200/200 total tests passing** (P1-P8)
