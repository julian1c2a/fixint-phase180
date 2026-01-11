# Magnitude-Sign Implementation Guide

> **Representation:** Magnitude-Sign (MS)  
> **Purpose:** Alternative signed representation for Phase 1.75 research  
> **Phase:** 1.75.1  
> **Status:** 🔬 Research & Implementation Guide

---

## 📚 Overview

### What is Magnitude-Sign?

Magnitude-Sign representation separates the value into two independent components:

```
Encoding: [Sign Bit (MSB)] [Magnitude (127 bits, unsigned)]

Example for 16-bit MS (abbreviated):
Value    Binary (MS)      Interpretation
------- ────────────────  ──────────────────
+0      0 0000000000000   S=0 (positive), M=0 (zero)
-0      1 0000000000000   S=1 (negative), M=0 (zero)  ← Distinct from +0!
+5      0 0000000000101   S=0 (positive), M=5
-5      1 0000000000101   S=1 (negative), M=5
+127    0 0000001111111   S=0 (positive), M=127
-127    1 0000001111111   S=1 (negative), M=127
```

### Key Differences from Two's Complement

| Aspect | Two's Complement | Magnitude-Sign |
|--------|-----------------|-----------------|
| **Sign encoding** | Implicit (MSB value contributes to number) | Explicit (MSB only indicates sign) |
| **Range** | [-2^127, 2^127-1] | [-2^127+1, 2^127-1] |
| **Zero** | One representation: 0x0...0 | Two representations: +0, -0 |
| **Negation** | Invert all bits + 1 | Flip sign bit only |
| **Magnitude extraction** | Negate if negative, else identity | Mask sign bit |
| **Hardware support** | Excellent (native CPU support) | None (software only) |

---

## 🔧 Implementation Architecture

### Storage (128 bits total)

```cpp
std::uint64_t data[2];
// data[0] = low 64 bits  (LSB)
// data[1] = high 64 bits (MSB)

// Magnitude-Sign interpretation:
// data[1] MSB (bit 63) = sign bit (0=positive, 1=negative)
// data[1] bits 62-0 + data[0] all 64 bits = magnitude (unsigned 127-bit value)

// Visual layout:
// data[1] [S|M_high_63bits] [M_low_64bits_data[0]]
//         └─ Sign bit      └──── Magnitude (127 bits unsigned) ────┘
```

### Representation-Specific Methods

#### 1. **is_negative() - Sign Bit Check**

```cpp
constexpr bool is_negative() const requires(is_magnitude_sign && is_signed) {
    // Check MSB of data[1] (bit 63)
    return (data[1] & 0x8000000000000000ULL) != 0;
}
```

**Two's Complement version (for comparison):**

```cpp
constexpr bool is_negative() const requires(is_twos_complement && is_signed) {
    return (data[1] & 0x8000000000000000ULL) != 0;
}
```

**⚠️ They look the same!** But interpretation differs:

- TC: MSB value is part of number (negative = value < 0)
- MS: MSB is just sign flag

---

#### 2. **magnitude() - Extract Absolute Value**

**Magnitude-Sign version:**

```cpp
constexpr int128_param_t magnitude() const requires(is_magnitude_sign && is_signed) {
    // Create MS value with sign bit cleared (force positive)
    int128_param_t result = *this;
    result.data[1] &= 0x7FFFFFFFFFFFFFFULL;  // Clear sign bit
    // Result: |value| in MS representation (always positive)
    return result;
}
```

**Two's Complement version (for comparison):**

```cpp
constexpr int128_param_t magnitude() const requires(is_twos_complement && is_signed) {
    if (is_negative()) {
        return -(*this);  // Two's complement negation
    }
    return *this;
}
```

**Key difference:**

- TC: Full negation operation (invert + 1)
- MS: Single bit flip (very fast!)

---

#### 3. **sign() - Extract Sign (-1, 0, +1)**

```cpp
constexpr int sign() const requires(is_signed) {
    // Works identically for MS and TC (both check MSB for sign)
    if (is_negative()) {
        return -1;
    } else if (is_zero()) {
        return 0;
    } else {
        return 1;
    }
}
```

---

### Arithmetic Operations (Magnitude-Sign Specific)

#### **Addition Example: a + b in MS**

**Challenge:** Addition in magnitude-sign requires different handling:

```
Case 1: Both positive or both negative
  (+a) + (+b) = +(a+b)     [Simple add, keep sign]
  (-a) + (-b) = -(a+b)     [Simple add, flip result sign if needed]

Case 2: Different signs (assume a > b for simplicity)
  (+a) + (-b) = +(a-b)     [Subtract, result inherits positive sign]
  (-a) + (+b) = -(a-b)     [Subtract, result inherits negative sign]

Key operations needed:
1. Compare magnitudes
2. Perform magnitude arithmetic (+ or -)
3. Set appropriate sign bit
```

**Implementation pattern:**

```cpp
constexpr int128_param_t& operator+=(const int128_param_t& other) 
    requires(is_magnitude_sign) 
{
    // Extract signs and magnitudes
    bool this_sign = is_negative();
    bool other_sign = other.is_negative();
    
    // Extract magnitudes (clear sign bit)
    int128_param_t this_mag = magnitude();
    int128_param_t other_mag = other.magnitude();
    
    if (this_sign == other_sign) {
        // Same sign: add magnitudes, preserve sign
        // result = +(magnitude_add(this_mag, other_mag)) or 
        //        = -(magnitude_add(this_mag, other_mag))
        magnitude_addition(this_mag, other_mag);
        if (this_sign) set_sign_bit();  // Set if negative
    } else {
        // Different signs: subtract magnitudes
        if (this_mag >= other_mag) {
            // Result inherits this_sign
            magnitude_subtract(this_mag, other_mag);
            if (this_sign) set_sign_bit();
        } else {
            // Result inherits other_sign  
            magnitude_subtract(other_mag, this_mag);
            if (other_sign) set_sign_bit();
        }
    }
    return *this;
}
```

---

#### **Negation: unary -x in MS**

**Key insight:** Negation is trivial in MS (just flip sign bit!)

```cpp
constexpr int128_param_t operator-() const requires(is_magnitude_sign) {
    int128_param_t result = *this;
    result.data[1] ^= 0x8000000000000000ULL;  // Toggle sign bit
    return result;
}
```

**Two's Complement version (for comparison):**

```cpp
constexpr int128_param_t operator-() const requires(is_twos_complement) {
    int128_param_t result;
    result = ~(*this);      // Invert all bits
    ++result;               // Add 1
    return result;
}
```

**Performance difference:**

- MS: 1 bit toggle = ~1 cycle
- TC: Full inversion + increment = ~2-3 cycles

---

### Special Case: Zero and ±0 Handling

**Magnitude-Sign allows two representations of zero:**

```cpp
// Check for both +0 and -0
constexpr bool is_zero() const {
    return (data[0] == 0) && ((data[1] & 0x7FFFFFFFFFFFFFFULL) == 0);
    // Note: Sign bit doesn't matter, both +0 and -0 are zero
}

// Distinguish between +0 and -0
constexpr bool is_positive_zero() const {
    return is_zero() && !is_negative();  // data[1] MSB = 0
}

constexpr bool is_negative_zero() const {
    return is_zero() && is_negative();   // data[1] MSB = 1
}
```

---

## 🔄 Conversion Between Representations

### Two's Complement → Magnitude-Sign

```cpp
constexpr int128_param_t<signed_type, magnitude_sign> 
from_twos_complement(const int128_param_t<signed_type, twos_complement>& tc_value) {
    int128_param_t<signed_type, magnitude_sign> result;
    
    if (tc_value.is_negative()) {
        // TC: negative value
        // MS: magnitude = -tc_value, sign = 1
        result = -tc_value;                                    // Negate in TC
        result.data[1] |= 0x8000000000000000ULL;              // Set sign bit
    } else {
        // TC: non-negative
        // MS: magnitude = tc_value, sign = 0
        result = tc_value;
        result.data[1] &= 0x7FFFFFFFFFFFFFFULL;  // Ensure sign bit clear
    }
    
    return result;
}
```

### Magnitude-Sign → Two's Complement

```cpp
constexpr int128_param_t<signed_type, twos_complement> 
to_twos_complement(const int128_param_t<signed_type, magnitude_sign>& ms_value) {
    int128_param_t<signed_type, twos_complement> result;
    
    // Extract magnitude (clear sign bit)
    int128_param_t temp = ms_value;
    temp.data[1] &= 0x7FFFFFFFFFFFFFFULL;
    
    if (ms_value.is_negative()) {
        // MS: negative
        // TC: two's complement negation
        result = -temp;
    } else {
        // MS: positive
        // TC: identity
        result = temp;
    }
    
    return result;
}
```

---

## 🧪 Test Cases (What to Implement)

### Basic Construction & Accessors

```cpp
✓ Constructor from int64_t (positive)
✓ Constructor from int64_t (negative)
✓ is_negative() returns correct sign
✓ magnitude() returns correct absolute value
✓ sign() returns -1, 0, or +1
✓ is_positive_zero() vs is_negative_zero()
✓ Zero special cases (+0 and -0)
```

### Arithmetic (Add/Subtract/Multiply/Divide)

```cpp
✓ (+) + (+) = (+)
✓ (-) + (-) = (-)
✓ (+) + (-) with various magnitudes
✓ (-) + (+) with various magnitudes
✓ Unary negation (flip sign bit)
✓ Subtraction as -(+b) + a
✓ Multiplication with various sign combinations
✓ Division with various sign combinations
✓ Overflow behavior
✓ Zero edge cases
```

### Comparisons

```cpp
✓ Positive > Negative (always)
✓ Negative < Positive (always)
✓ Same sign comparison (magnitude order)
✓ +0 == -0 (mathematical equality)
✓ +0 and -0 distinct in representation
```

### Conversions

```cpp
✓ Two's Complement ↔ Magnitude-Sign round-trip
✓ Preservation of value through conversion
✓ Zero conversions (+0 and -0)
✓ Edge cases (INT128_MIN, INT128_MAX)
```

---

## 📊 Performance Implications

### Operations Fast in Magnitude-Sign

| Operation | MS | TC | Ratio |
|-----------|----|----|-------|
| Negation | 1 bit flip | 64 ops | **MS ~64x faster** |
| Sign extraction | 1 bit check | 1 bit check | Same |
| Magnitude | 1 bit clear | Negate+check | **MS 2-3x faster** |
| Absolute value | Direct | Conditional negate | **MS faster** |

### Operations Slow in Magnitude-Sign

| Operation | MS | TC | Ratio |
|-----------|----|----|-------|
| Addition | Compare+add/sub | Simple add | **TC faster** |
| Multiplication | Compare+mul+handle | Simple mul | **TC faster** |
| Division | Complex branching | Division | **TC faster** |
| Overall computation | Software intensive | Native CPU | **TC 2-5x faster** |

**Conclusion:** MS excels at sign manipulation, sacrifices arithmetic speed

---

## 🎯 Implementation Roadmap

### Phase 1: Basics (Days 1-3)

- [ ] Constructors (int64_t, pairs, strings)
- [ ] Accessors (high, low, sign, magnitude)
- [ ] is_negative(), is_zero() variations
- [ ] ✅ Test suite: 20+ basic tests

### Phase 2: Arithmetic (Days 4-7)

- [ ] operator+= (add/subtract with sign logic)
- [ ] operator-= (subtraction)
- [ ] unary operator- (negation)
- [ ] operator* (multiplication)
- [ ] operator/ (division)
- [ ] ✅ Test suite: 30+ arithmetic tests

### Phase 3: Conversions & I/O (Days 8-10)

- [ ] Conversion from/to Two's Complement
- [ ] String parsing
- [ ] String formatting
- [ ] Stream I/O operators
- [ ] ✅ Test suite: 15+ conversion tests

### Phase 4: Documentation & Benchmarking (Days 11-14)

- [ ] Complete API documentation
- [ ] Tutorial demos
- [ ] Performance benchmarks (MS vs TC)
- [ ] Comparison analysis

---

## 📝 Code Template

```cpp
// In int128_parameterized.hpp, add specialization:

template <>
class int128_param_t<signedness::signed_type, representation_form::magnitude_sign> {
    static constexpr bool is_signed = true;
    static constexpr bool is_magnitude_sign = true;
    static constexpr int BITS = 128;
    
private:
    std::uint64_t data[2];
    
    // Helper: Set/clear sign bit
    constexpr void set_sign_bit() noexcept {
        data[1] |= 0x8000000000000000ULL;
    }
    
    constexpr void clear_sign_bit() noexcept {
        data[1] &= 0x7FFFFFFFFFFFFFFULL;
    }
    
public:
    // Constructors, operators, etc.
    
    constexpr bool is_negative() const noexcept {
        return (data[1] & 0x8000000000000000ULL) != 0;
    }
    
    constexpr int128_param_t magnitude() const noexcept {
        int128_param_t result = *this;
        result.clear_sign_bit();
        return result;
    }
    
    // ... implement remaining methods
};
```

---

## 🔗 Related Documentation

- [representation.hpp](include/representation.hpp) - Representation system
- [int128_parameterized.hpp](include/int128_parameterized.hpp) - Template class
- [README.md](README.md) - Project overview
- [Phase 1.66 API Reference](../int128-phase166/API_INT128_BASE_TT.md) - For comparison

---

**Last Updated:** 11 January 2026  
**Status:** 🔬 Implementation Guide - Ready for Development
