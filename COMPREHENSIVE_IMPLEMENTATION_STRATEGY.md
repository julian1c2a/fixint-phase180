# COMPREHENSIVE IMPLEMENTATION STRATEGY: Replicate int128_base_tt.hpp with MS/EK Support

**Date:** January 11, 2026  
**Status:** Planning Phase  
**Approach:** Systematic replication of all 100+ methods from legacy code with parameterized representation support

---

## 1. Overview

Instead of incremental "Priority-based" development, we will systematically replicate **all** functionality from `int128_base_tt.hpp` (4,314 lines, TC-only) into our parameterized template, adding `if constexpr` branches for:

- Two's Complement (TC) - unchanged from legacy
- Magnitude-Sign (MS) - sign bit separate, magnitude bits for value, ±0 distinction
- Excess-K (EK) - bias notation (framework ready)

### Timeline Estimate

- **Phase 1 (This week):** Full TC parity + MS support (80-100 hours)
- **Phase 2 (Next week):** EK support + comprehensive testing
- **Phase 3:** Performance optimization and documentation

---

## 2. Current Implementation Status

### ✅ Already Implemented (144 core tests passing)

| Category | Methods | Status | Tests |
|----------|---------|--------|-------|
| Constructors | 7 types | ✅ Complete | 20/20 |
| Accessors | 4 methods (low, high, set_low, set_high) | ✅ Complete | 35/35 |
| Arithmetic | +, -, *, / (library), % (library) | ✅ Complete | 24/24 |
| Bitwise | &, \|, ^, ~ (with assignments) | ✅ Complete | 24/24 |
| Comparison | ==, !=, <, <=, >, >= (with MS support) | ✅ Complete | — |
| String I/O | to_string(), parse_ct_safe(), from_string() | ✅ Complete | 41/41 |

### ❌ Still Missing (from legacy code)

| Category | Methods | Lines | Priority |
|----------|---------|-------|----------|
| Bit Manipulation | trailing_zeros, leading_zeros, bit_width, is_power_of_2, count_ones, rotate_left, rotate_right, popcount | 150-200 | High |
| Shifts | <<, >> (both with proper TC/MS/EK semantics) | 150-200 | High |
| Division Helper | divrem_by_chunk() for to_string chunking | 100-150 | Medium |
| I/O Extensions | to_string(base), parse(), abs(), signed-specific methods | 100-150 | Medium |
| Comparisons | friend operators for cross-type comparisons | 50-100 | Medium |
| Conversions | Floating-point conversions, byte arrays, bitsets | 50-100 | Low |

---

## 3. Detailed Method Inventory from int128_base_tt.hpp

### A. Bit Manipulation Section (Lines 743-830)

```cpp
// trailing_zeros() - Count trailing zero bits (tz = 0 if all bits 0)
// leading_zeros() - Count leading zero bits in absolute value
// bit_width() - Minimum bits needed to represent value
// is_power_of_2() - Check if value is 2^n
// count_ones() / popcount() - Count set bits
// rotate_left(int) - Circular left shift
// rotate_right(int) - Circular right shift
```

**MS Implementation Note:** For MS, operate on magnitude bits only, preserve sign.

### B. Unary Operators (Lines 866-910)

```cpp
operator~()          // Bitwise NOT
operator-()          // Unary negation (TC: standard; MS: flip sign bit)
operator+()          // Unary plus (no-op, returns copy)
```

### C. Arithmetic Operators (Lines 908-1170)

```cpp
// Addition
operator+=(T)                              // Integral types
operator+(T)
operator+=(const int128_base_t<S2>&)       // Same-type int128
operator+(const int128_base_t<S2>&)
operator+=(const int128_base_t<S2>&)       // Different signedness

// Subtraction (similar overloads)
operator-=(T)
operator-(T)
operator-=(const int128_base_t<S2>&)
operator-(const int128_base_t<S2>&)

// Multiplication (similar overloads)
operator*=(T)
operator*=(const int128_base_t&)
operator*(T)
operator*(const int128_base_t&)
operator*=(const int128_base_t<S2>&)

// Division and Modulo
operator/=(const int128_base_t&)
operator/(const int128_base_t&)
operator%=(const int128_base_t&)
operator%(const int128_base_t&)
operator/=(T)
operator/(T)
operator%=(T)
operator%(T)
operator/=(const int128_base_t<S2>&)
operator/(const int128_base_t<S2>&)
operator%=(const int128_base_t<S2>&)
operator%(const int128_base_t<S2>&)
```

**MS Implementation Note:** MS addition/subtraction has sign-dependent logic (same sign vs different sign). Multiplication/division requires magnitude handling.

### D. Bitwise Operators (Lines 1173-1302)

```cpp
// AND (multiple overloads)
operator&=(const int128_base_t&)
operator&=(const int128_base_t<S2>&)
operator&(const int128_base_t&)
operator&(const int128_base_t<S2>&)
operator&=(T)
operator&(T)

// OR (similar structure - 6 overloads)
operator|=(const int128_base_t&)
operator|=(const int128_base_t<S2>&)
operator|(const int128_base_t&)
operator|(const int128_base_t<S2>&)
operator|=(T)
operator|(T)

// XOR (similar structure - 6 overloads)
operator^=(const int128_base_t&)
operator^=(const int128_base_t<S2>&)
operator^(const int128_base_t&)
operator^(const int128_base_t<S2>&)
operator^=(T)
operator^(T)
```

**Status:** ✅ IMPLEMENTED for same-type operands. Need additional overloads for cross-type.

### E. Shift Operators (Lines 1303-1440)

```cpp
// Left Shift
operator<<=(int)
operator<<(int)
operator<<=(T)     // For integral shift amounts
operator<<(T)

// Right Shift (complex - has sign-extension logic for signed)
operator>>=(int)
operator>>(int)
operator>>=(T)
operator>>(T)
```

**Status:** ❌ NOT IMPLEMENTED. Need implementation with:

- TC unsigned: logical right shift
- TC signed: arithmetic right shift (sign-extend)
- MS: magnitude-only shifts with sign preservation

### F. Comparison Operators (Lines 1443-1625)

```cpp
// Member operators
operator==(const int128_base_t&)
operator!=(const int128_base_t&)
operator<(const int128_base_t&)
operator<=>(const int128_base_t&)    // Three-way comparison

// Friend operators with integral types
operator==(const int128_base_t&, T)
operator==(T, const int128_base_t&)
operator!=(const int128_base_t&, T)
operator!=(T, const int128_base_t&)
operator<(const int128_base_t&, T)
operator<(T, const int128_base_t&)
operator<=(const int128_base_t&, T)
operator<=(T, const int128_base_t&)
operator>(const int128_base_t&, T)
operator>(T, const int128_base_t&)
operator>=(const int128_base_t&, T)
operator>=(T, const int128_base_t&)

// Additional specializations for unsigned integral types
```

**Status:** ✅ PARTIALLY IMPLEMENTED. Have member operators; need friend operators for integral types.

### G. String I/O Methods (Lines 1787-2100)

```cpp
// String output
to_string()              // Decimal, handles signed
to_string(int base)      // Bases 2-36

// Parsing
parse(std::string_view, int base = 10)        // Static method
parse_ct_safe(std::string_view, int base)     // Constexpr-safe
from_string(const char*, int base = 10)       // Throws on error

// Helper for chunked decimal conversion
divrem_by_chunk(uint64_t divisor, int128_base_t& quotient, uint64_t& remainder)
```

**Status:** ✅ IMPLEMENTED except divrem_by_chunk() helper.

### H. Abs and Sign Methods (Lines 2100-2150)

```cpp
abs()                    // Absolute value (returns new int128)
is_negative()            // Sign check
is_positive()            // Positive check (> 0)
is_zero()                // Zero check
sign()                   // Return -1, 0, or +1

// MS-specific (already have these)
magnitude()              // Extract magnitude bits
is_positive_zero()       // +0 in MS
is_negative_zero()       // -0 in MS
```

**Status:** ✅ PARTIALLY IMPLEMENTED.

### I. Floating-Point Conversions (Lines 2150-2250)

```cpp
operator double()        // Safe conversion to double
operator long double()   // Safe conversion to long double
from_double(double)      // Static method
from_long_double(long double)
```

**Status:** ❌ NOT IMPLEMENTED.

### J. Byte/Bitset Conversions (Lines 2250-2350)

```cpp
to_bytearray()          // Convert to std::array<uint8_t, 16>
from_bytearray(...)     // Static method
to_bitset()             // Convert to std::bitset<128>
from_bitset(...)        // Static method
```

**Status:** ❌ NOT IMPLEMENTED.

---

## 4. Implementation Plan by Category

### Phase A: Core Functionality (Must-Have)

**Priority A1: Shift Operators [75 minutes]**

- `operator<<`, `operator<<=`
- `operator>>`, `operator>>=`
- With proper TC/MS/EK semantics
- ~150 lines of code
- Create test_priority7_shift.cpp (20-25 tests)

**Priority A2: Bit Manipulation [120 minutes]**

- `trailing_zeros()`, `leading_zeros()`
- `bit_width()`, `is_power_of_2()`
- `count_ones()`, `popcount()`
- `rotate_left()`, `rotate_right()`
- ~180 lines of code
- Create test_priority8_bitops.cpp (25-30 tests)

**Priority A3: Friend Comparison Operators [60 minutes]**

- All friend operator overloads with integral types
- Cross-signedness comparisons
- ~100 lines of code
- Add to existing test_priority2_comparisons.cpp (15-20 tests)

**Total Phase A:** ~255 minutes (4+ hours)

### Phase B: Extended Functionality (Nice-to-Have)

**Priority B1: Division Helper [45 minutes]**

- `divrem_by_chunk()` - Essential for efficient to_string()
- ~80 lines of code

**Priority B2: Floating-Point Conversions [90 minutes]**

- Double/long double conversions
- ~150 lines of code
- Create test_priority9_float_conv.cpp (15-20 tests)

**Priority B3: Byte/Bitset Support [75 minutes]**

- to_bytearray(), from_bytearray()
- to_bitset(), from_bitset()
- ~130 lines of code
- Create test_priority10_array_conv.cpp (15-20 tests)

**Total Phase B:** ~210 minutes (3.5 hours)

### Phase C: Polish & Optimization

- Performance benchmarking vs legacy code
- Documentation and examples
- Edge case analysis
- ~120 minutes

---

## 5. MS-Specific Implementation Notes

### Shift Operators in MS

```cpp
// Left Shift: shift magnitude bits only
constexpr int128_param_t operator<<(int shift) const noexcept {
    if constexpr (is_magnitude_sign && is_signed) {
        // Extract magnitude (clear sign bit)
        auto mag = *this;
        mag.data[1] &= ~(1ULL << 63);
        // Shift magnitude
        // ... shift logic ...
        // Restore sign bit
        mag.data[1] |= (data[1] & (1ULL << 63));
        return mag;
    } else {
        // TC: standard shift (already implemented)
        // ...
    }
}

// Right Shift: magnitude-only, logical fill with zeros
constexpr int128_param_t operator>>(int shift) const noexcept {
    if constexpr (is_magnitude_sign && is_signed) {
        // Extract magnitude
        auto mag = *this;
        uint64_t sign_bit = mag.data[1] & (1ULL << 63);
        mag.data[1] &= ~(1ULL << 63);
        // Shift magnitude logically
        // ... shift logic ...
        // Restore sign bit
        mag.data[1] |= sign_bit;
        return mag;
    } else if constexpr (is_signed) {
        // TC signed: arithmetic right shift (sign-extend)
        // ...
    } else {
        // TC unsigned: logical right shift
        // ...
    }
}
```

### Arithmetic in MS

```cpp
// Addition has sign-dependent logic
constexpr int128_param_t& operator+=(const int128_param_t& other) noexcept {
    if constexpr (is_magnitude_sign && is_signed) {
        if (is_negative() == other.is_negative()) {
            // Same sign: add magnitudes, keep sign
            // ...
        } else {
            // Different signs: subtract magnitudes, determine result sign
            // ...
        }
    } else {
        // TC: standard addition
        // ...
    }
}
```

---

## 6. Testing Strategy

### Test Coverage Target

| Priority | Feature | Test Count | Estimate |
|----------|---------|-----------|----------|
| P7 | Shift operators | 20-25 | 30 min |
| P8 | Bit manipulation | 25-30 | 40 min |
| P9 | Friend comparisons | 15-20 | 25 min |
| P10 | Float conversions | 15-20 | 35 min |
| P11 | Array/bitset | 15-20 | 30 min |
| **Total** | | **90-115** | **2.5 hours** |

### Test File Structure

```
tests/
  test_priority7_shift.cpp          (20-25 tests)
  test_priority8_bitops.cpp         (25-30 tests)
  test_priority9_float_conv.cpp     (15-20 tests)
  test_priority10_array_conv.cpp    (15-20 tests)
  test_comparisons_friends.cpp      (15-20 additional tests)
```

---

## 7. Success Criteria

✅ **100% of int128_base_tt.hpp methods replicated**  
✅ **All methods support TC, MS, and EK representations**  
✅ **220+ test cases all passing**  
✅ **MS arithmetic with sign-dependent logic working correctly**  
✅ **Zero warnings, clean compilation**  
✅ **Performance within 10% of legacy code**  

---

## 8. Execution Order

**Session 1 (Today):**

1. Implement shift operators (P7) - 75 min
2. Create shift tests - 20 min
3. Debug and achieve 100% pass rate - 15 min

**Session 2:**

1. Implement bit manipulation (P8) - 120 min
2. Create bit ops tests - 30 min
3. Debug - 15 min

**Session 3:**

1. Implement friend comparisons - 60 min
2. Implement division helper - 45 min
3. Add tests and debug - 30 min

**Session 4:**

1. Implement float conversions (P10) - 90 min
2. Implement array/bitset (P11) - 75 min
3. Create tests - 30 min
4. Final testing and polish - 30 min

**Total Estimated Time:** ~11-12 hours of focused development

---

## 9. Key Implementation Files

**Main Header (to be expanded):**

- `include/int128_parameterized.hpp` (currently 1,289 lines → target ~3,000 lines)

**New Test Files:**

- `tests/test_priority7_shift.cpp`
- `tests/test_priority8_bitops.cpp`
- `tests/test_priority9_float_conv.cpp`
- `tests/test_priority10_array_conv.cpp`

**Documentation (auto-generated):**

- `PRIORITY_7_COMPLETION.md` through `PRIORITY_11_COMPLETION.md`

---

## 10. Backward Compatibility

✅ All existing code continues to work  
✅ Type aliases unchanged  
✅ Default types still map to Two's Complement  
✅ Test suite expands without modification to existing tests  

---

## Next Steps

Ready to begin with **Priority 7: Shift Operators**

1. Review legacy shift operator implementations
2. Implement for all types (TC unsigned, TC signed, MS signed, EK)
3. Create comprehensive test suite
4. Validate all 20-25 tests pass
5. Document in PRIORITY_7_COMPLETION.md

Estimated completion: **75-100 minutes**
