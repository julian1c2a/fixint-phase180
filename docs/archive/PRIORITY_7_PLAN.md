# PRIORITY 7 PLAN: Shift Operators (Phase 1.75)

**Target:** Implement left shift (`<<`, `<<=`) and right shift (`>>`, `>>=`) operators with full representation awareness

**Estimated Duration:** 60-75 minutes total

## Objectives

1. Implement `operator<<` (left shift) for all type combinations
2. Implement `operator<<=` (left shift assignment)
3. Implement `operator>>` (right shift) for all type combinations  
4. Implement `operator>>=` (right shift assignment)
5. Handle MS representation-specific semantics
6. Create comprehensive test suite (20-25 tests)
7. Achieve 100% test pass rate

## Implementation Strategy

### Left Shift (`<<` and `<<=`)

**Two's Complement Behavior:**

```cpp
constexpr int128_t operator<<(int shift) const noexcept {
    // Shift both limbs left
    // Handle overflow into higher limb
    // Discard bits shifted out beyond bit 127
}
```

**Magnitude-Sign Behavior:**

- Apply left shift to magnitude bits only
- Preserve sign bit (MSB) separately
- Handle magnitude overflow properly

**Edge Cases:**

- `shift == 0`: Return unchanged value
- `shift >= 128`: Result is 0 (or wrap for unsigned)
- `shift < 0`: Undefined or implementation-defined (use constrained implementation)

### Right Shift (`>>` and `>>=`)

**Two's Complement Behavior:**

- **Unsigned:** Logical right shift (fill with 0s)
- **Signed:** Arithmetic right shift (sign-extend with MSB)

**Magnitude-Sign Behavior:**

- Apply right shift to magnitude bits only
- Preserve sign bit (MSB)
- Sign-extend magnitude zeros (all positive magnitudes)

**Edge Cases:**

- `shift == 0`: Return unchanged value
- `shift >= 128`: Result is 0 or max value (depending on sign)
- `shift < 0`: Undefined or implementation-defined

## Code Structure

### File Location

`include/int128_parameterized.hpp` - Add after bitwise operators section

### Pattern to Follow

```cpp
// ============================================================================
// Shift Operators
// ============================================================================

/// Left Shift
constexpr int128_param_t operator<<(int shift) const noexcept {
    if constexpr (is_magnitude_sign && is_signed) {
        // MS-specific: shift magnitude, preserve sign
    } else {
        // TC/unsigned: standard bitwise shift
    }
}

constexpr int128_param_t& operator<<=(int shift) noexcept {
    *this = *this << shift;
    return *this;
}

/// Right Shift
constexpr int128_param_t operator>>(int shift) const noexcept {
    if constexpr (is_magnitude_sign && is_signed) {
        // MS-specific: shift magnitude, preserve sign, fill with 0s
    } else if constexpr (is_signed) {
        // TC signed: arithmetic right shift (sign-extend)
    } else {
        // Unsigned: logical right shift
    }
}

constexpr int128_param_t& operator>>=(int shift) noexcept {
    *this = *this >> shift;
    return *this;
}
```

## Test Plan

### Test File

`tests/test_priority7_shift.cpp` - New file, ~450 lines, 20-25 test cases

### Test Categories

**Left Shift Tests (5-6 tests)**

- `test_left_shift_zero`: Shifting by 0
- `test_left_shift_one`: Shifting by 1
- `test_left_shift_full_range`: Shifts from 0-127
- `test_left_shift_unsigned`: uint128 variant
- `test_left_shift_assignment`: <<= operator
- `test_left_shift_double_shift`: Consecutive shifts

**Right Shift Tests (5-6 tests)**

- `test_right_shift_zero`: Shifting by 0
- `test_right_shift_one`: Shifting by 1
- `test_right_shift_logical_unsigned`: uint128 (always logical)
- `test_right_shift_arithmetic_signed_tc`: int128_tc_t sign-extends
- `test_right_shift_assignment`: >>= operator
- `test_right_shift_fill_zeros`: Large shifts

**Combined Shift Tests (3 tests)**

- `test_shift_left_then_right`: << then >> recovers original (shifted bits)
- `test_shift_right_then_left`: >> then << may lose information
- `test_shift_by_128_and_beyond`: Edge case handling

**MS-Specific Shift Tests (4-5 tests)**

- `test_ms_left_shift_magnitude`: MS magnitude left shift
- `test_ms_right_shift_magnitude`: MS magnitude right shift  
- `test_ms_sign_preservation_left`: Sign bit preserved after <<
- `test_ms_sign_preservation_right`: Sign bit preserved after >>
- `test_ms_shift_negative_number`: Shifting negative (high magnitude)

**Unsigned TC Tests (2-3 tests)**

- `test_unsigned_left_shift`: uint128_t <<
- `test_unsigned_right_shift`: uint128_t >> (logical)
- `test_unsigned_shift_overflow`: Discarded bits

### Test Validation Strategy

1. Test individual shift amounts (0, 1, 2, 63, 64, 65, 127, 128)
2. Verify sign preservation in TC arithmetic right shift
3. Verify magnitude preservation in MS shifts
4. Test roundtrip: `(x << n) >> n` relationships
5. Test overflow behavior for left shifts
6. Test zero-fill for unsigned right shifts
7. Test sign-extension for signed right shifts (TC)

## Semantic Requirements

### Left Shift

- Bits shifted out on the left (beyond bit 127) are discarded
- Zeros fill in on the right
- Can cause unsigned overflow (modular semantics)
- Can cause signed overflow (undefined in strict C++, but we wrap)
- Non-commutative and non-associative with other operations

### Right Shift (Unsigned)

- Bits shifted out on the right are discarded
- Zeros fill in on the left (logical shift)
- Never overflows
- Effectively divides by 2^n (with truncation)

### Right Shift (Signed TC)

- Bits shifted out on the right are discarded
- Signed bit (MSB) extends into left positions (arithmetic shift)
- Never overflows
- Effectively divides by 2^n with proper sign handling

### Right Shift (Signed MS)

- Bits shifted out on the right are discarded
- Magnitude bits are shifted logically (fill with 0s)
- Sign bit (MSB) is preserved independently
- Magnitude is effectively divided by 2^n

## Implementation Notes

1. **Shift Count Validation:** No range validation (may shift > 128)
   - Shifts >= 128 result in 0 or max value
   - Similar to standard C++ behavior

2. **MS Magnitude Extraction:**

   ```cpp
   uint64_t mag_low = data[0];
   uint64_t mag_high = data[1] & ~(1ULL << 63);  // Mask off sign
   ```

3. **MS Sign Preservation:**

   ```cpp
   uint64_t sign_bit = data[1] & (1ULL << 63);
   ```

4. **128-bit Shift Implementation:**
   - Shifts 0-63: Operate within single limb
   - Shifts 64-127: Move data between limbs
   - Shifts >= 128: Complete annihilation or sign extension

## Success Criteria

✅ All 20-25 tests pass  
✅ No compiler warnings  
✅ Consistent with legacy code patterns  
✅ Zero runtime overhead (via `if constexpr`)  
✅ Documentation complete  
✅ TC and MS semantics correct  

## Timeline

| Phase | Duration | Task |
|-------|----------|------|
| Analysis | 5 min | Review shift semantics for all types |
| Implementation | 40 min | Code ~150 lines of shift operators |
| Test Creation | 15 min | Write 20-25 test cases |
| Testing & Debug | 10 min | Run tests, fix issues |
| Documentation | 5 min | Update completion markdown |
| **Total** | **75 min** | Complete Priority 7 |
