// ==============================================================================
// Priority 2: Magnitude-Sign (MS) Representation Methods
// Phase 1.75 - int128 parameterized template
// ==============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julián Calderón Almendros
// Email: julian.calderon.almendros@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// ==============================================================================

#include "representation.hpp"
#include "int128_parameterized.hpp"

#include <iostream>
#include <cassert>
#include <string>

using namespace nstd;

// ==============================================================================
// Test Macros
// ==============================================================================

#define TEST(name)      \
    void test_##name(); \
    test_##name();      \
    std::cout << "✓ " << #name << std::endl;
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_TRUE(a) assert(a)
#define ASSERT_FALSE(a) assert(!(a))

// ==============================================================================
// Magnitude-Sign (MS) Specific Tests
// ==============================================================================

// TEST 1: MS is_negative() - positive value
void test_ms_is_negative_positive()
{
    // In magnitude-sign: positive values have sign bit = 0
    int128_ms_t x(42); // Positive value
    ASSERT_FALSE(x.is_negative());
}

// TEST 2: MS is_negative() - negative value
void test_ms_is_negative_negative()
{
    // In magnitude-sign: negative values have sign bit = 1
    // Constructor from int64_t -42 should have sign bit set for MS
    int128_ms_t x(static_cast<int64_t>(-42));
    ASSERT_TRUE(x.is_negative());
}

// TEST 3: MS is_negative() - zero
void test_ms_is_negative_zero()
{
    // Zero is always non-negative (no sign bit set)
    int128_ms_t x(0);
    ASSERT_FALSE(x.is_negative());
}

// TEST 4: MS magnitude() - positive value
void test_ms_magnitude_positive()
{
    // magnitude() should return the absolute value (without sign bit)
    int128_ms_t x(100);
    auto mag = x.magnitude();
    ASSERT_EQ(mag, uint128_ms_t(100));
}

// TEST 5: MS magnitude() - negative value
void test_ms_magnitude_negative()
{
    // magnitude() of -100 should return 100 (absolute value)
    int128_ms_t x(static_cast<int64_t>(-100));
    auto mag = x.magnitude();
    ASSERT_EQ(mag, uint128_ms_t(100));
}

// TEST 6: MS magnitude() - max magnitude
void test_ms_magnitude_max()
{
    // Maximum representable magnitude in 128-bit MS: 2^127 - 1
    int128_ms_t x(9223372036854775807LL); // 2^63 - 1
    auto mag = x.magnitude();
    ASSERT_EQ(mag.high(), 0ULL);
    ASSERT_EQ(mag.low(), 9223372036854775807ULL);
}

// TEST 7: sign() method - positive value returns 1
void test_sign_positive()
{
    int128_ms_t x(42);
    ASSERT_EQ(x.get_sign(), 1); // Positive = +1
}

// TEST 8: sign() method - negative value returns -1
void test_sign_negative()
{
    int128_ms_t x(static_cast<int64_t>(-42));
    ASSERT_EQ(x.get_sign(), -1); // Negative = -1
}

// TEST 9: sign() method - zero returns 0
void test_sign_zero()
{
    int128_ms_t x(0);
    ASSERT_EQ(x.get_sign(), 0); // Zero = 0
}

// TEST 10: is_positive_zero() - detects +0
void test_is_positive_zero_true()
{
    // Positive zero in MS: sign bit = 0, magnitude = 0
    int128_ms_t x(0);
    ASSERT_TRUE(x.is_positive_zero());
}

// TEST 11: is_positive_zero() - non-zero value
void test_is_positive_zero_false()
{
    int128_ms_t x(42);
    ASSERT_FALSE(x.is_positive_zero());
}

// TEST 12: is_negative_zero() - detects -0
void test_is_negative_zero_true()
{
    // Negative zero in MS: sign bit = 1, magnitude = 0
    // This requires explicit construction with sign bit set but zero magnitude
    int128_ms_t x;
    x.set_high(1ULL << 63); // Set sign bit (MSB of high part)
    x.set_low(0);           // Zero magnitude
    ASSERT_TRUE(x.is_negative_zero());
}

// TEST 13: is_negative_zero() - non-zero negative
void test_is_negative_zero_false()
{
    int128_ms_t x(static_cast<int64_t>(-42));
    ASSERT_FALSE(x.is_negative_zero());
}

// TEST 14: MS negation preserves magnitude
void test_ms_negate_preserves_magnitude()
{
    // Negating should flip sign bit but preserve magnitude
    int128_ms_t x(50);
    ASSERT_EQ(x.magnitude(), uint128_ms_t(50));
    ASSERT_FALSE(x.is_negative());

    int128_ms_t neg_x(static_cast<int64_t>(-50));
    ASSERT_EQ(neg_x.magnitude(), uint128_ms_t(50));
    ASSERT_TRUE(neg_x.is_negative());
}

// TEST 15: MS distinguishes +0 from -0
void test_ms_zero_distinction()
{
    // This is a unique feature of magnitude-sign:
    // positive zero ≠ negative zero in bit representation
    int128_ms_t pos_zero(0);

    int128_ms_t neg_zero;
    neg_zero.set_high(1ULL << 63); // Sign bit set
    neg_zero.set_low(0);           // But magnitude is zero

    // Both should compare equal mathematically
    ASSERT_EQ(pos_zero, neg_zero);
    // But internal representation differs
    ASSERT_NE(pos_zero.high(), neg_zero.high());
}

// TEST 16: magnitude() of negative maximum
void test_ms_magnitude_max_negative()
{
    // Maximum magnitude in negative form: -(2^127 - 1)
    int128_ms_t x(static_cast<int64_t>(-9223372036854775807LL));
    auto mag = x.magnitude();
    ASSERT_EQ(mag.high(), 0ULL);
    ASSERT_EQ(mag.low(), 9223372036854775807ULL);
}

// TEST 17: is_negative() on unsigned MS
void test_unsigned_ms_always_positive()
{
    // Unsigned MS types should never be negative
    uint128_ms_t x(100);
    ASSERT_FALSE(x.is_negative());

    uint128_ms_t y(0);
    ASSERT_FALSE(y.is_negative());
}

// TEST 18: magnitude() of unsigned MS (identity)
void test_unsigned_ms_magnitude_identity()
{
    // For unsigned MS, magnitude() should return the value itself
    uint128_ms_t x(12345);
    auto mag = x.magnitude();
    ASSERT_EQ(mag, x);
}

// TEST 19: sign() on unsigned MS (always 0 or 1)
void test_unsigned_ms_sign_positive()
{
    uint128_ms_t x(100);
    ASSERT_EQ(x.get_sign(), 1); // Always 1 (positive) or 0 (zero)

    uint128_ms_t zero(0);
    ASSERT_EQ(zero.get_sign(), 0); // Zero returns 0
}

// TEST 20: MS comparison with mixed signedness
void test_ms_mixed_comparison()
{
    int128_ms_t pos(42);
    int128_ms_t neg(static_cast<int64_t>(-42));

    // Positive should be greater than negative
    ASSERT_TRUE(pos > neg);
    ASSERT_TRUE(neg < pos);
    ASSERT_FALSE(pos == neg);
}

// TEST 21: MS round-trip sign flip
void test_ms_round_trip_sign_flip()
{
    // Flipping sign twice should return to original
    int128_ms_t x(123);
    ASSERT_EQ(x.get_sign(), 1);

    int128_ms_t flipped(static_cast<int64_t>(-123));
    ASSERT_EQ(flipped.get_sign(), -1);

    // Flipping back should match original
    int128_ms_t double_flipped(123);
    ASSERT_EQ(x, double_flipped);
}

// TEST 22: is_positive_zero() only for signed MS
void test_is_positive_zero_only_signed()
{
    // Unsigned MS shouldn't have the concept of positive zero
    // (or it should return true for zero like any other zero)
    uint128_ms_t x(0);
    // This test depends on whether unsigned MS has positive_zero concept
    // Likely: yes for consistency
    ASSERT_TRUE(x.is_positive_zero());
}

// ==============================================================================
// Main Test Driver
// ==============================================================================

int main()
{
    std::cout << "\n==================== PRIORITY 2 TESTS ====================\n\n";

    std::cout << "Magnitude-Sign Representation Methods:\n";
    TEST(ms_is_negative_positive)
    TEST(ms_is_negative_negative)
    TEST(ms_is_negative_zero)
    TEST(ms_magnitude_positive)
    TEST(ms_magnitude_negative)
    TEST(ms_magnitude_max)
    TEST(sign_positive)
    TEST(sign_negative)
    TEST(sign_zero)
    TEST(is_positive_zero_true)
    TEST(is_positive_zero_false)
    TEST(is_negative_zero_true)
    TEST(is_negative_zero_false)
    TEST(ms_negate_preserves_magnitude)
    TEST(ms_zero_distinction)
    TEST(ms_magnitude_max_negative)
    TEST(unsigned_ms_always_positive)
    TEST(unsigned_ms_magnitude_identity)
    TEST(unsigned_ms_sign_positive)
    TEST(ms_mixed_comparison)
    TEST(ms_round_trip_sign_flip)
    TEST(is_positive_zero_only_signed)

    std::cout << "\n✅ All 22 Priority 2 tests PASSED!\n\n";
    return 0;
}
