# Priority 6: Bitwise Operators - Implementation Plan

**Target:** `&` (AND), `|` (OR), `^` (XOR), `~` (NOT)

## Semantic Requirements

### Two's Complement & Unsigned

- Standard bitwise operations (no special handling)
- Result type: same as operands

### Magnitude-Sign Signed

- **AND/OR/XOR:** Apply to **magnitude bits only**, preserve sign separately
- **NOT:** Invert magnitude bits, preserve sign bit
- Example:
  - MS(-5) = sign:1, mag:101
  - ~MS(-5) = sign:1, mag:010 = MS(-2)

### Excess-K (Not yet supported)

- Reserve for future implementation

## Test Plan

### For Each Operator

1. Basic cases (0, 1, all-bits)
2. Self operations (x & x = x, x ^ x = 0)
3. Identity operations (x | 0 = x, x & all-bits = x)
4. Unsigned variants
5. Signed TC variants
6. Signed MS variants (with magnitude-awareness)

### Total Tests Expected: ~20-24

## Implementation Approach

Add to `int128_parameterized.hpp`:

```cpp
// Bitwise operators
constexpr int128_param_t operator&(const int128_param_t &other) const noexcept;
constexpr int128_param_t operator|(const int128_param_t &other) const noexcept;
constexpr int128_param_t operator^(const int128_param_t &other) const noexcept;
constexpr int128_param_t operator~() const noexcept;

// Assignment variants
constexpr int128_param_t &operator&=(const int128_param_t &other) noexcept;
constexpr int128_param_t &operator|=(const int128_param_t &other) noexcept;
constexpr int128_param_t &operator^=(const int128_param_t &other) noexcept;
```

## Magnitude-Sign Implementation Detail

For MS signed types:

```cpp
if constexpr (is_magnitude_sign && is_signed) {
    // Extract magnitude (clear sign bit)
    std::uint64_t this_mag_low = data[0];
    std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
    
    // Apply bitwise op to magnitudes
    std::uint64_t result_mag_low = this_mag_low & other_mag_low;
    std::uint64_t result_mag_high = this_mag_high & other_mag_high;
    
    // Reconstruct with original sign bit
    result.data[0] = result_mag_low;
    result.data[1] = result_mag_high | (data[1] & (1ULL << 63));
}
```

## Timeline

1. Implement operators (30 min)
2. Create test file (20 min)
3. Run tests & debug (15 min)

**Estimated Total: 60-75 minutes**
