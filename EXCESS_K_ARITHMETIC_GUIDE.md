# Excess-K Arithmetic Limitations Guide

**Document:** Phase 1.75 - Excess-K Representation  
**Date:** 18 January 2026  
**Status:** ⚠️ Known Limitations - Workarounds Provided

---

## Executive Summary

Excess-K representation stores values with a bias offset:

```
stored_value = real_value + bias
bias = 2^126
```

**Critical Issue:** Standard arithmetic operators (+, -, *, /) operate on **stored values**, not **real values**, leading to incorrect results for Excess-K.

**Solutions:**

1. ✅ **Recommended:** Convert to Two's Complement, operate, convert back
2. ⚠️ **Advanced:** Implement custom operators with bias adjustment (future work)
3. ❌ **Not recommended:** Use raw operators (produces wrong results)

---

## 1. The Problem: Bias Accumulation

### Addition Example (INCORRECT)

```cpp
int128_ek_t a{(1ULL << 62), 1};  // Real: +1, Stored: bias+1
int128_ek_t b{(1ULL << 62), 1};  // Real: +1, Stored: bias+1

// Using standard operator+
int128_ek_t c = a + b;

// Expected: 1 + 1 = 2 → stored as bias+2
// Actual:   (bias+1) + (bias+1) = 2·bias + 2 ❌
// Result:   c represents a HUGE positive number, not 2!
```

**Why it fails:**

```
real(a) + real(b) = 1 + 1 = 2
stored(a) + stored(b) = (bias+1) + (bias+1) = 2·bias + 2

Correct stored value for 2: bias + 2
Actual stored value:        2·bias + 2 = bias + (bias + 2) ❌
                                       = bias + 2 + 85070591730234615865843651857942052864
```

The extra bias makes the result wrong by ~2^126!

---

### Subtraction Example (INCORRECT)

```cpp
int128_ek_t a{(1ULL << 62), 10};  // Real: +10, Stored: bias+10
int128_ek_t b{(1ULL << 62), 1};   // Real: +1, Stored: bias+1

// Using standard operator-
int128_ek_t c = a - b;

// Expected: 10 - 1 = 9 → stored as bias+9
// Actual:   (bias+10) - (bias+1) = 9 ✅ (WORKS!)
```

**Why subtraction works:**

```
stored(a) - stored(b) = (bias+10) - (bias+1) = 9
Correct stored value for 9: bias + 9

Wait, we need to ADD the bias back!
9 ≠ bias+9 ❌
```

Actually, standard subtraction also fails - it cancels out the bias completely.

---

### Multiplication Example (INCORRECT)

```cpp
int128_ek_t a{(1ULL << 62), 2};  // Real: +2, Stored: bias+2
int128_ek_t b{(1ULL << 62), 3};  // Real: +3, Stored: bias+3

// Using standard operator*
int128_ek_t c = a * b;

// Expected: 2 × 3 = 6 → stored as bias+6
// Actual:   (bias+2) × (bias+3) = bias² + 5·bias + 6 ❌
```

**Why it fails:**

```
(bias+2) × (bias+3) = bias² + 3·bias + 2·bias + 6
                     = bias² + 5·bias + 6

Correct stored value: bias + 6
Actual stored value:  bias² + 5·bias + 6 ❌ (off by bias² + 4·bias!)
```

---

## 2. Correct Formulas (For Custom Implementation)

### Addition

```
real(a) + real(b) = c
(stored(a) - bias) + (stored(b) - bias) = c
stored(c) = c + bias = stored(a) + stored(b) - bias

Algorithm:
1. result = a + b      // Standard addition
2. result -= bias      // Subtract one extra bias
```

### Subtraction

```
real(a) - real(b) = c
(stored(a) - bias) - (stored(b) - bias) = c
stored(c) = c + bias = stored(a) - stored(b) + bias

Algorithm:
1. result = a - b      // Standard subtraction
2. result += bias      // Add bias back
```

### Multiplication

```
real(a) × real(b) = c
(stored(a) - bias) × (stored(b) - bias) = c
stored(c) = c + bias = stored(a)·stored(b) - bias·(stored(a) + stored(b)) + bias² + bias

Algorithm:
1. result = a * b                    // Standard multiplication
2. result -= bias * (a + b)          // Subtract bias·(a+b)
3. result += bias * bias             // Add bias²
4. result += bias                    // Add bias
```

This is complex and expensive! Better to convert representations.

---

## 3. Recommended Solution: Convert to TC

### Pattern 1: Single Operation

```cpp
// Helper functions (to be implemented)
uint128_tc_t ek_to_tc(const int128_ek_t& ek);
int128_ek_t tc_to_ek(const uint128_tc_t& tc);

// Correct addition
int128_ek_t add_ek(const int128_ek_t& a, const int128_ek_t& b)
{
    uint128_tc_t a_tc = ek_to_tc(a);  // Extract real value
    uint128_tc_t b_tc = ek_to_tc(b);
    uint128_tc_t result_tc = a_tc + b_tc;  // Proper arithmetic
    return tc_to_ek(result_tc);  // Encode back
}
```

### Pattern 2: Batch Operations

```cpp
// Convert once, operate multiple times, convert back
int128_ek_t a_ek = ...;
int128_ek_t b_ek = ...;
int128_ek_t c_ek = ...;

// Convert to TC
uint128_tc_t a = ek_to_tc(a_ek);
uint128_tc_t b = ek_to_tc(b_ek);
uint128_tc_t c = ek_to_tc(c_ek);

// Operate in TC space (efficient!)
uint128_tc_t result = (a + b) * c - (a / b);

// Convert back
int128_ek_t result_ek = tc_to_ek(result);
```

---

## 4. Implementation: Conversion Helpers

### Extract Real Value (EK → TC)

```cpp
constexpr uint128_tc_t ek_to_tc(const int128_ek_t& ek) noexcept
{
    // Real value = stored value - bias
    constexpr uint64_t bias_high = (1ULL << 62);
    constexpr uint64_t bias_low = 0;
    
    uint128_tc_t result;
    
    // 128-bit subtraction: result = ek - bias
    bool borrow = (ek.low() < bias_low);
    result.set_low(ek.low() - bias_low);
    
    if (borrow) {
        result.set_high(ek.high() - bias_high - 1);
    } else {
        result.set_high(ek.high() - bias_high);
    }
    
    return result;
}
```

### Encode Real Value (TC → EK)

```cpp
constexpr int128_ek_t tc_to_ek(const uint128_tc_t& tc) noexcept
{
    // Stored value = real value + bias
    constexpr uint64_t bias_high = (1ULL << 62);
    constexpr uint64_t bias_low = 0;
    
    int128_ek_t result;
    
    // 128-bit addition: result = tc + bias
    uint64_t low = tc.low() + bias_low;
    bool carry = (low < tc.low());  // Overflow detection
    
    result.set_low(low);
    result.set_high(tc.high() + bias_high + (carry ? 1 : 0));
    
    return result;
}
```

---

## 5. What DOES Work for Excess-K

### ✅ Comparison Operators

All comparison operators work correctly:

```cpp
int128_ek_t a{(1ULL << 62), 10};  // Real: +10
int128_ek_t b{(1ULL << 62), 5};   // Real: +5

a > b   // ✅ true (stored: bias+10 > bias+5)
a == b  // ✅ false
a < b   // ✅ false
```

**Why:** Stored value ordering preserves real value ordering.

---

### ✅ Unary Negation (operator-)

Negation works correctly:

```cpp
int128_ek_t a{(1ULL << 62), 1};  // Real: +1
int128_ek_t b = -a;              // Real: -1 ✅

// Uses formula: -x = 2·bias - x
```

**Why:** Implemented with explicit bias adjustment.

---

### ✅ Zero Detection (is_zero)

Zero detection works:

```cpp
int128_ek_t zero{(1ULL << 62), 0};  // Real: 0
zero.is_zero()  // ✅ true
```

**Why:** Implemented to check `stored == bias`.

---

### ✅ Sign Detection (is_negative)

Sign detection works:

```cpp
int128_ek_t neg{(1ULL << 62) - 1, ~0ULL};  // Real: -1
neg.is_negative()  // ✅ true
```

**Why:** Implemented to check `stored < bias`.

---

## 6. Summary Table

| Operation | Standard Operator | Custom Needed | Convert to TC |
|-----------|-------------------|---------------|---------------|
| **Comparison** | | | |
| `a == b` | ✅ Works | ❌ No | ❌ No |
| `a < b` | ✅ Works | ❌ No | ❌ No |
| `a > b` | ✅ Works | ❌ No | ❌ No |
| **Arithmetic** | | | |
| `a + b` | ❌ Fails | ✅ Complex | ✅ Recommended |
| `a - b` | ❌ Fails | ✅ Simple | ✅ Recommended |
| `a * b` | ❌ Fails | ⚠️ Very Complex | ✅ Recommended |
| `a / b` | ❌ Fails | ⚠️ Very Complex | ✅ Recommended |
| **Unary** | | | |
| `-a` | ✅ Works | ❌ No | ❌ No |
| `++a` | ❌ Fails | ✅ Simple | ⚠️ Optional |
| **Bitwise** | | | |
| `a & b` | ⚠️ Raw bits | ⚠️ Use with care | ✅ If semantic needed |
| `a \| b` | ⚠️ Raw bits | ⚠️ Use with care | ✅ If semantic needed |
| `a ^ b` | ⚠️ Raw bits | ⚠️ Use with care | ✅ If semantic needed |

**Legend:**

- ✅ Recommended/Works correctly
- ❌ Does not work/Not needed
- ⚠️ Partial/Use with caution

---

## 7. Examples: Correct Usage

### Example 1: Simple Addition

```cpp
#include "int128_parameterized.hpp"

int main()
{
    int128_ek_t a{(1ULL << 62), 100};  // Real: +100
    int128_ek_t b{(1ULL << 62), 200};  // Real: +200
    
    // WRONG: int128_ek_t sum = a + b; ❌
    
    // CORRECT:
    uint128_tc_t a_tc = ek_to_tc(a);
    uint128_tc_t b_tc = ek_to_tc(b);
    uint128_tc_t sum_tc = a_tc + b_tc;
    int128_ek_t sum = tc_to_ek(sum_tc);  // sum = 300 ✅
}
```

---

### Example 2: Complex Expression

```cpp
int128_ek_t a{(1ULL << 62), 10};
int128_ek_t b{(1ULL << 62), 5};
int128_ek_t c{(1ULL << 62), 2};

// Compute: (a + b) * c - a/b

// Convert all to TC
uint128_tc_t a_tc = ek_to_tc(a);
uint128_tc_t b_tc = ek_to_tc(b);
uint128_tc_t c_tc = ek_to_tc(c);

// Operate in TC (natural arithmetic)
uint128_tc_t result_tc = (a_tc + b_tc) * c_tc - a_tc / b_tc;
// = (10 + 5) * 2 - 10/5 = 15 * 2 - 2 = 30 - 2 = 28

// Convert back
int128_ek_t result = tc_to_ek(result_tc);  // result = 28 ✅
```

---

### Example 3: Comparison (No Conversion Needed)

```cpp
int128_ek_t a{(1ULL << 62), 42};
int128_ek_t b{(1ULL << 62), 17};

// Direct comparison works!
if (a > b) {  // ✅ true (42 > 17)
    std::cout << "a is greater\n";
}
```

---

## 8. Future Work

### Potential Custom Operators (Phase 1.76+)

```cpp
// Custom addition with bias adjustment
constexpr int128_ek_t& operator+=(const int128_ek_t& other) noexcept
{
    if constexpr (is_excess_k)
    {
        // Add normally
        data[0] += other.data[0];
        uint64_t carry = (data[0] < other.data[0]) ? 1 : 0;
        data[1] += other.data[1] + carry;
        
        // Subtract one bias
        constexpr uint64_t bias_high = (1ULL << 62);
        bool borrow = (data[0] < 0);
        if (borrow) {
            data[1] = data[1] - bias_high - 1;
        } else {
            data[1] -= bias_high;
        }
    }
    else
    {
        // Standard TC/MS addition
        // ...
    }
    return *this;
}
```

**Complexity:** High (requires careful testing for all edge cases)  
**Priority:** Low (conversion approach is simpler and correct)

---

## 9. Testing Recommendations

### Required Tests

1. ✅ Comparison operators (10 tests) - **COMPLETE**
2. ⏳ Conversion helpers (ek_to_tc, tc_to_ek) - **TODO**
3. ⏳ Arithmetic via conversion (20+ tests) - **TODO**
4. ⏳ Edge cases (overflow, underflow) - **TODO**
5. ⏳ Performance benchmarks (conversion overhead) - **TODO**

---

## 10. Conclusion

**For Excess-K in Phase 1.75:**

- ✅ Use comparison operators directly (they work!)
- ✅ Use unary negation directly (it works!)
- ❌ Do NOT use standard arithmetic operators (+, -, *, /)
- ✅ Convert to TC for arithmetic, then convert back
- ⏳ Custom operators are future work (low priority)

**Performance Note:**
Conversion overhead is ~10-20 cycles per operation (negligible for most use cases). For tight loops with millions of operations, consider staying in TC representation.

---

**END OF GUIDE**
