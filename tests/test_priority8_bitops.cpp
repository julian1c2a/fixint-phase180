#include <cassert>
#include <iostream>
#include "int128_parameterized.hpp"

// Define a simple testing framework
#define TEST_CASE(name) void test_##name()
#define RUN_TEST(name) test_##name(); std::cout << "✓ " #name "\n"
#define ASSERT_EQ(a, b) assert((a) == (b))

// Forward declarations for all tests
TEST_CASE(trailing_zeros_simple);
TEST_CASE(leading_zeros_simple);
TEST_CASE(popcount_all_ones);
TEST_CASE(ms_trailing_zeros_negative);
// Add more test forward declarations here...

// ========================================================================
// Test Implementations
// ========================================================================

TEST_CASE(trailing_zeros_simple)
{
    uint128_tc_t x(0, 8);  // 0x0000...0008
    ASSERT_EQ(x.trailing_zeros(), 3);
}

TEST_CASE(leading_zeros_simple)
{
    uint128_tc_t x(0, 1);  // This is wrong, should be high part
    x.set_high(1); // Corrected
    x.set_low(0);
    // This now represents 1 << 64
    // leading_zeros should be 63 for the high part, but clzll(1) is 63.
    // The total leading zeros should be 63. Let's trace.
    // data[1] = 1, data[0] = 0.
    // __builtin_clzll(1) = 63.
    // So this is correct.
    ASSERT_EQ(x.leading_zeros(), 63);

    uint128_tc_t y(0, 1); // low part is 1
    // data[1] = 0, data[0] = 1
    // leading_zeros should go into the second branch.
    // 64 + __builtin_clzll(1) = 64 + 63 = 127
    ASSERT_EQ(y.leading_zeros(), 127);
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


// ========================================================================
// Main function to run all tests
// ========================================================================

int main()
{
    std::cout << "Running Priority 8: Bit Operations Tests...\n\n";

    RUN_TEST(trailing_zeros_simple);
    RUN_TEST(leading_zeros_simple);
    RUN_TEST(popcount_all_ones);
    RUN_TEST(ms_trailing_zeros_negative);
    // Add more RUN_TEST calls here...


    std::cout << "\nPRIORITY 8 TESTS PASSED!\n";
    return 0;
}