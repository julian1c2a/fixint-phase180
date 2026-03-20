// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Priority 6 Tests: Bitwise Operators (Phase 1.75)
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0

#include "representation.hpp"
#include "int128_parameterized.hpp"

#include <iostream>
#include <cassert>

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
            std::cout << "  [OK] " << name << std::endl;                      \
        }                                                                  \
        catch (const std::exception &e)                                    \
        {                                                                  \
            std::cout << "  [ERROR] " << name << " - " << e.what() << std::endl; \
        }                                                                  \
    } while (false)

#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_TRUE(a) assert((a) == true)
#define ASSERT_FALSE(a) assert((a) == false)

// ============================================================================
// Bitwise AND Tests
// ============================================================================

void test_and_zero()
{
    int128_t x(15LL);
    int128_t y(0LL);
    ASSERT_EQ(x & y, int128_t(0LL));
}

void test_and_self()
{
    int128_t x(42LL);
    ASSERT_EQ(x & x, x);
}

void test_and_all_bits()
{
    int128_t x(0x0FLL);
    int128_t y(0xFFLL);
    ASSERT_EQ(x & y, int128_t(0x0FLL));
}

void test_and_unsigned()
{
    uint128_t x(0xAAAAAAAAULL);
    uint128_t y(0x55555555ULL);
    ASSERT_EQ(x & y, uint128_t(0ULL));
}

void test_and_assignment()
{
    int128_t x(0xFF);
    x &= int128_t(0x0F);
    ASSERT_EQ(x, int128_t(0x0F));
}

// ============================================================================
// Bitwise OR Tests
// ============================================================================

void test_or_zero()
{
    int128_t x(15LL);
    int128_t y(0LL);
    ASSERT_EQ(x | y, x);
}

void test_or_self()
{
    int128_t x(42LL);
    ASSERT_EQ(x | x, x);
}

void test_or_combine()
{
    int128_t x(0x0FLL);
    int128_t y(0xF0LL);
    ASSERT_EQ(x | y, int128_t(0xFFLL));
}

void test_or_unsigned()
{
    uint128_t x(0x0F0F0F0FUL);
    uint128_t y(0xF0F0F0F0UL);
    ASSERT_EQ(x | y, uint128_t(0xFFFFFFFFUL));
}

void test_or_assignment()
{
    int128_t x(0x0F);
    x |= int128_t(0xF0);
    ASSERT_EQ(x, int128_t(0xFF));
}

// ============================================================================
// Bitwise XOR Tests
// ============================================================================

void test_xor_zero()
{
    int128_t x(15LL);
    ASSERT_EQ(x ^ int128_t(0LL), x);
}

void test_xor_self()
{
    int128_t x(42LL);
    ASSERT_EQ(x ^ x, int128_t(0LL));
}

void test_xor_invert()
{
    int128_t x(0x0FLL);
    int128_t y(0xFFLL);
    ASSERT_EQ(x ^ y, int128_t(0xF0LL));
}

void test_xor_unsigned()
{
    uint128_t x(0xAAAAAAAAULL);
    uint128_t y(0x55555555ULL);
    ASSERT_EQ(x ^ y, uint128_t(0xFFFFFFFFULL));
}

void test_xor_assignment()
{
    int128_t x(0xFF);
    x ^= int128_t(0xAA);
    ASSERT_EQ(x, int128_t(0x55));
}

// ============================================================================
// Bitwise NOT Tests
// ============================================================================

void test_not_zero()
{
    int128_t x(0LL);
    // ~0 is all 1s
    int128_t result = ~x;
    ASSERT_TRUE(result != int128_t(0LL));
}

void test_not_inverts()
{
    int128_t x(0x0FLL);
    int128_t result = ~x;
    // ~0x0F = ...11111111...11110000 (bitwise complement)
    ASSERT_TRUE(result != x);
}

void test_not_unsigned()
{
    uint128_t x(0LL);
    uint128_t result = ~x;
    // For unsigned, ~0 should be all 1s
    ASSERT_TRUE(result != uint128_t(0ULL));
}

void test_not_double_invert()
{
    int128_t x(42LL);
    int128_t result = ~(~x);
    ASSERT_EQ(result, x);
}

// ============================================================================
// Combined Bitwise Operations
// ============================================================================

void test_and_or_commutative()
{
    int128_t x(0x0FLL);
    int128_t y(0xF0LL);
    ASSERT_EQ((x & y) | y, y);
    ASSERT_EQ((x | y) & y, y);
}

void test_demorgan_laws()
{
    int128_t x(0x0F);
    int128_t y(0xF0);
    // ~(x & y) should be ~x | ~y
    int128_t result1 = ~(x & y);
    int128_t result2 = (~x) | (~y);
    ASSERT_EQ(result1, result2);
}

// ============================================================================
// Magnitude-Sign Specific Tests
// ============================================================================

void test_ms_and_magnitude()
{
    int128_ms_t x(15LL); // Stored as TC: 0x0F
    int128_ms_t y(7LL);  // Stored as TC: 0x07
    int128_ms_t result = x & y;
    // AND of magnitudes should preserve behavior
    ASSERT_FALSE(result.is_negative());
}

void test_ms_or_magnitude()
{
    int128_ms_t x(8LL); // Stored as TC: 0x08
    int128_ms_t y(4LL); // Stored as TC: 0x04
    int128_ms_t result = x | y;
    ASSERT_FALSE(result.is_negative());
}

void test_ms_not_inverts_magnitude()
{
    // ~(positive small) in MS magnitude-aware mode creates a large value
    // because we invert the magnitude bits independently
    int128_ms_t x(5LL); // Magnitude 5, positive
    int128_ms_t result = ~x;
    // After inverting magnitude: large number
    // The result could be negative depending on bit patterns
    ASSERT_NE(result, x);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main()
{
    std::cout << "======================================================================\n";
    std::cout << "PRIORITY 6 TESTS: Bitwise Operators (&, |, ^, ~)\n";
    std::cout << "======================================================================\n";

    std::cout << "\n[1-5] Bitwise AND:\n";
    TEST_CASE("test_and_zero", test_and_zero);
    TEST_CASE("test_and_self", test_and_self);
    TEST_CASE("test_and_all_bits", test_and_all_bits);
    TEST_CASE("test_and_unsigned", test_and_unsigned);
    TEST_CASE("test_and_assignment", test_and_assignment);

    std::cout << "\n[6-10] Bitwise OR:\n";
    TEST_CASE("test_or_zero", test_or_zero);
    TEST_CASE("test_or_self", test_or_self);
    TEST_CASE("test_or_combine", test_or_combine);
    TEST_CASE("test_or_unsigned", test_or_unsigned);
    TEST_CASE("test_or_assignment", test_or_assignment);

    std::cout << "\n[11-15] Bitwise XOR:\n";
    TEST_CASE("test_xor_zero", test_xor_zero);
    TEST_CASE("test_xor_self", test_xor_self);
    TEST_CASE("test_xor_invert", test_xor_invert);
    TEST_CASE("test_xor_unsigned", test_xor_unsigned);
    TEST_CASE("test_xor_assignment", test_xor_assignment);

    std::cout << "\n[16-19] Bitwise NOT:\n";
    TEST_CASE("test_not_zero", test_not_zero);
    TEST_CASE("test_not_inverts", test_not_inverts);
    TEST_CASE("test_not_unsigned", test_not_unsigned);
    TEST_CASE("test_not_double_invert", test_not_double_invert);

    std::cout << "\n[20-22] Combined Operations:\n";
    TEST_CASE("test_and_or_commutative", test_and_or_commutative);
    TEST_CASE("test_demorgan_laws", test_demorgan_laws);
    TEST_CASE("test_ms_and_magnitude", test_ms_and_magnitude);

    std::cout << "\n[23-25] Magnitude-Sign Specific:\n";
    TEST_CASE("test_ms_or_magnitude", test_ms_or_magnitude);
    TEST_CASE("test_ms_not_inverts_magnitude", test_ms_not_inverts_magnitude);

    std::cout << "\n======================================================================\n";
    std::cout << "RESULTS: " << pass_count << "/" << test_count << " PASSED\n";
    std::cout << "======================================================================\n";

    return (pass_count == test_count) ? 0 : 1;
}
