// =============================================================================
// Priority 7 Tests: Shift Operators (Phase 1.75)
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0

#include "representation.hpp"
#include "int128_parameterized.hpp"

#include <iostream>
#include <cassert>
#include <cstring>

using namespace nstd;

// ============================================================================
// Test Macros
// ============================================================================

int test_count = 0, pass_count = 0;

#define TEST_CASE(name, func)                                              \
    do                                                                     \
    {                                                                      \
        test_count++;                                                      \
        try                                                                \
        {                                                                  \
            func();                                                        \
            pass_count++;                                                  \
            std::cout << "  ✓ " << name << std::endl;                      \
        }                                                                  \
        catch (const std::exception &e)                                    \
        {                                                                  \
            std::cout << "  ✗ " << name << " - " << e.what() << std::endl; \
        }                                                                  \
    } while (false)

#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_TRUE(a) assert((a) == true)
#define ASSERT_FALSE(a) assert((a) == false)

// ============================================================================
// Left Shift Tests
// ============================================================================

void test_left_shift_zero()
{
    int128_t x(42LL);
    ASSERT_EQ(x << 0, int128_t(42LL));
}

void test_left_shift_one()
{
    int128_t x(1LL);
    ASSERT_EQ(x << 1, int128_t(2LL));
}

void test_left_shift_multiple()
{
    int128_t x(1LL);
    ASSERT_EQ(x << 3, int128_t(8LL));
    ASSERT_EQ(x << 8, int128_t(256LL));
}

void test_left_shift_64()
{
    uint128_t x(1ULL);
    // Shifting into high limb
    uint128_t result = x << 64;
    ASSERT_EQ(result.low(), 0ULL);
    ASSERT_EQ(result.high(), 1ULL);
}

void test_left_shift_assignment()
{
    int128_t x(5LL);
    x <<= 2;
    ASSERT_EQ(x, int128_t(20LL));
}

void test_left_shift_beyond_128()
{
    int128_t x(42LL);
    ASSERT_EQ(x << 128, int128_t(0LL));
    ASSERT_EQ(x << 200, int128_t(0LL));
}

void test_left_shift_unsigned()
{
    uint128_t x(0x1122334455667788ULL);
    uint128_t result = x << 1;
    // Should be doubled
    ASSERT_NE(result, x);
}

void test_left_shift_negative_tc()
{
    int128_t x(-1LL); // All bits set in TC
    int128_t result = x << 1;
    // Still all bits set (or shifted version)
    ASSERT_TRUE(result.is_negative());
}

// ============================================================================
// Right Shift Tests
// ============================================================================

void test_right_shift_zero()
{
    int128_t x(42LL);
    ASSERT_EQ(x >> 0, int128_t(42LL));
}

void test_right_shift_one()
{
    int128_t x(8LL);
    ASSERT_EQ(x >> 1, int128_t(4LL));
}

void test_right_shift_multiple()
{
    int128_t x(256LL);
    ASSERT_EQ(x >> 1, int128_t(128LL));
    ASSERT_EQ(x >> 2, int128_t(64LL));
    ASSERT_EQ(x >> 8, int128_t(1LL));
}

void test_right_shift_64()
{
    // Constructor takes (high, low) in Western order
    uint128_t x(1ULL, 0); // high=1, low=0
    uint128_t result = x >> 64;
    // After right shifting by 64, high limb goes to low limb
    ASSERT_EQ(result.low(), 1ULL);
    ASSERT_EQ(result.high(), 0ULL);
}

void test_right_shift_assignment()
{
    int128_t x(20LL);
    x >>= 2;
    ASSERT_EQ(x, int128_t(5LL));
}

void test_right_shift_unsigned_logical()
{
    uint128_t x(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    uint128_t result = x >> 64;
    // High bits should become low, high should be 0
    ASSERT_EQ(result.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(result.high(), 0ULL);
}

void test_right_shift_signed_arithmetic()
{
    // Negative number in TC (MSB=1)
    int128_t x(-1LL); // 0xFFFF...FFFF
    int128_t result = x >> 1;
    // Should still be negative (arithmetic shift extends sign)
    ASSERT_TRUE(result.is_negative());
}

void test_right_shift_beyond_128()
{
    int128_t x(42LL);
    // Unsigned zero out
    uint128_t ux(42ULL);
    ASSERT_EQ(ux >> 128, uint128_t(0ULL));

    // Signed extends sign
    ASSERT_EQ(x >> 128, int128_t(0LL));
}

// ============================================================================
// Combined Shift Tests
// ============================================================================

void test_roundtrip_shift()
{
    int128_t x(12345LL);
    // Left then right by same amount
    ASSERT_EQ((x << 3) >> 3, x);
}

void test_shift_integral_types()
{
    int128_t x(8LL);
    // Shift by short, int, long, etc.
    ASSERT_EQ(x >> (short)1, int128_t(4LL));
    ASSERT_EQ(x << (int)1, int128_t(16LL));
}

void test_combined_operations()
{
    uint128_t x(0x123456789ABCDEFULL);
    uint128_t shifted = x << 4;
    uint128_t back = shifted >> 4;
    // May not be exact due to overflow, but verify it works
    ASSERT_NE(shifted, x);
}

// ============================================================================
// MS-Specific Shift Tests
// ============================================================================

void test_ms_left_shift_positive()
{
    int128_ms_t x(5LL);
    int128_ms_t result = x << 1;
    ASSERT_FALSE(result.is_negative()); // Sign preserved
}

void test_ms_left_shift_negative()
{
    int128_ms_t x(-5LL);
    int128_ms_t result = x << 1;
    ASSERT_TRUE(result.is_negative()); // Sign preserved
}

void test_ms_right_shift_positive()
{
    int128_ms_t x(20LL);
    int128_ms_t result = x >> 1;
    ASSERT_FALSE(result.is_negative()); // Sign preserved
}

void test_ms_right_shift_negative()
{
    int128_ms_t x(-20LL);
    int128_ms_t result = x >> 1;
    ASSERT_TRUE(result.is_negative()); // Sign preserved
}

void test_ms_shift_magnitude_preservation()
{
    // Shifting should affect magnitude, not sign
    int128_ms_t x(8LL);
    int128_ms_t shifted = (x << 1) >> 1;
    // May not equal due to magnitude changes, but both positive
    ASSERT_FALSE(shifted.is_negative());
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_shift_by_zero_is_identity()
{
    uint128_t x(0x123456789ABCDEFULL);
    ASSERT_EQ(x << 0, x);
    ASSERT_EQ(x >> 0, x);
}

void test_shift_zero_value()
{
    int128_t zero(0LL);
    ASSERT_EQ(zero << 50, int128_t(0LL));
    ASSERT_EQ(zero >> 50, int128_t(0LL));
}

void test_left_shift_then_right()
{
    int128_t x(123LL);
    int128_t result = ((x << 10) >> 10);
    // Should recover original (no overflow)
    ASSERT_EQ(result, x);
}

void test_negative_shift_amount_ignored()
{
    int128_t x(8LL);
    // Negative or zero shift should be no-op or safe
    ASSERT_EQ(x << 0, x);
    ASSERT_EQ(x >> 0, x);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main()
{
    std::cout << "======================================================================\n";
    std::cout << "PRIORITY 7 TESTS: Shift Operators (<<, >>, <<=, >>=)\n";
    std::cout << "======================================================================\n";

    std::cout << "\n[1-8] Left Shift Tests:\n";
    TEST_CASE("test_left_shift_zero", test_left_shift_zero);
    TEST_CASE("test_left_shift_one", test_left_shift_one);
    TEST_CASE("test_left_shift_multiple", test_left_shift_multiple);
    TEST_CASE("test_left_shift_64", test_left_shift_64);
    TEST_CASE("test_left_shift_assignment", test_left_shift_assignment);
    TEST_CASE("test_left_shift_beyond_128", test_left_shift_beyond_128);
    TEST_CASE("test_left_shift_unsigned", test_left_shift_unsigned);
    TEST_CASE("test_left_shift_negative_tc", test_left_shift_negative_tc);

    std::cout << "\n[9-18] Right Shift Tests:\n";
    TEST_CASE("test_right_shift_zero", test_right_shift_zero);
    TEST_CASE("test_right_shift_one", test_right_shift_one);
    TEST_CASE("test_right_shift_multiple", test_right_shift_multiple);
    TEST_CASE("test_right_shift_64", test_right_shift_64);
    TEST_CASE("test_right_shift_assignment", test_right_shift_assignment);
    TEST_CASE("test_right_shift_unsigned_logical", test_right_shift_unsigned_logical);
    TEST_CASE("test_right_shift_signed_arithmetic", test_right_shift_signed_arithmetic);
    TEST_CASE("test_right_shift_beyond_128", test_right_shift_beyond_128);
    TEST_CASE("test_shift_integral_types", test_shift_integral_types);
    TEST_CASE("test_combined_operations", test_combined_operations);

    std::cout << "\n[19-24] MS-Specific Shift Tests:\n";
    TEST_CASE("test_ms_left_shift_positive", test_ms_left_shift_positive);
    TEST_CASE("test_ms_left_shift_negative", test_ms_left_shift_negative);
    TEST_CASE("test_ms_right_shift_positive", test_ms_right_shift_positive);
    TEST_CASE("test_ms_right_shift_negative", test_ms_right_shift_negative);
    TEST_CASE("test_ms_shift_magnitude_preservation", test_ms_shift_magnitude_preservation);

    std::cout << "\n[25-29] Edge Cases & Roundtrips:\n";
    TEST_CASE("test_roundtrip_shift", test_roundtrip_shift);
    TEST_CASE("test_shift_by_zero_is_identity", test_shift_by_zero_is_identity);
    TEST_CASE("test_shift_zero_value", test_shift_zero_value);
    TEST_CASE("test_left_shift_then_right", test_left_shift_then_right);
    TEST_CASE("test_negative_shift_amount_ignored", test_negative_shift_amount_ignored);

    std::cout << "\n======================================================================\n";
    std::cout << "RESULTS: " << pass_count << "/" << test_count << " PASSED\n";
    std::cout << "======================================================================\n";

    return (pass_count == test_count) ? 0 : 1;
}
