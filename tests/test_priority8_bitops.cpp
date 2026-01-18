// =============================================================================
// Test Suite: Priority 8 - Bit Manipulation Functions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include <cassert>
#include <iostream>
#include <iomanip>

using namespace nstd;

// Test counter
int g_test_count = 0;
int g_passed = 0;
int g_failed = 0;

#define TEST(name)                                               \
    void test_##name();                                          \
    namespace                                                    \
    {                                                            \
        struct register_##name                                   \
        {                                                        \
            register_##name()                                    \
            {                                                    \
                std::cout << "Running test: " << #name << "..."; \
                ++g_test_count;                                  \
                try                                              \
                {                                                \
                    test_##name();                               \
                    std::cout << " [OK]\n";                      \
                    ++g_passed;                                  \
                }                                                \
                catch (const std::exception &e)                  \
                {                                                \
                    std::cout << " [FAIL] " << e.what() << "\n"; \
                    ++g_failed;                                  \
                }                                                \
            }                                                    \
        } register_##name##_instance;                            \
    }                                                            \
    void test_##name()

#define ASSERT_EQ(expr, expected)                                                     \
    do                                                                                \
    {                                                                                 \
        auto val = (expr);                                                            \
        auto exp = (expected);                                                        \
        if (val != exp)                                                               \
        {                                                                             \
            throw std::runtime_error(std::string("Expected ") + std::to_string(exp) + \
                                     " but got " + std::to_string(val));              \
        }                                                                             \
    } while (0)

#define ASSERT_TRUE(expr)                                            \
    do                                                               \
    {                                                                \
        if (!(expr))                                                 \
        {                                                            \
            throw std::runtime_error("Expected true but got false"); \
        }                                                            \
    } while (0)

#define ASSERT_FALSE(expr)                                           \
    do                                                               \
    {                                                                \
        if ((expr))                                                  \
        {                                                            \
            throw std::runtime_error("Expected false but got true"); \
        }                                                            \
    } while (0)

// =============================================================================
// Trailing Zeros Tests (4 tests)
// =============================================================================

TEST(trailing_zeros_simple_tc)
{
    const uint128_tc_t x{0, 8}; // 0b1000
    ASSERT_EQ(x.trailing_zeros(), 3);
}

TEST(trailing_zeros_high_tc)
{
    const uint128_tc_t x{0x8000000000000000ULL, 0}; // Bit 127 set
    ASSERT_EQ(x.trailing_zeros(), 64);              // All 64 low bits are zero
}

TEST(trailing_zeros_all_zeros_tc)
{
    const uint128_tc_t x{0, 0};
    ASSERT_EQ(x.trailing_zeros(), 128);
}

TEST(trailing_zeros_ms_negative)
{
    const int128_ms_t x{-1}; // MS: magnitude 1, sign bit set
    // Magnitude is 1, which is 0b00...0001
    ASSERT_EQ(x.trailing_zeros(), 0); // No trailing zeros in magnitude
}

TEST(trailing_zeros_ms_magnitude)
{
    // MS with magnitude 8 (0b1000)
    int128_ms_t x{0, 0};
    x.set_low(8);
    x.set_high(0); // Positive
    ASSERT_EQ(x.trailing_zeros(), 3);
}

// =============================================================================
// Leading Zeros Tests (4 tests)
// =============================================================================

TEST(leading_zeros_simple_tc)
{
    const uint128_tc_t x{0, 1}; // Only LSB set
    ASSERT_EQ(x.leading_zeros(), 127);
}

TEST(leading_zeros_high_bit_tc)
{
    const uint128_tc_t x{0x8000000000000000ULL, 0}; // MSB set
    ASSERT_EQ(x.leading_zeros(), 0);
}

TEST(leading_zeros_all_zeros_tc)
{
    const uint128_tc_t x{0, 0};
    ASSERT_EQ(x.leading_zeros(), 128);
}

TEST(leading_zeros_ms_signed)
{
    // MS signed: leading zeros counts in magnitude (127 bits)
    const int128_ms_t x{0, 1};         // Positive, magnitude 1
    ASSERT_EQ(x.leading_zeros(), 127); // 126 leading zeros in magnitude
}

TEST(leading_zeros_mid_value)
{
    const uint128_tc_t x{0, 0xFF};     // 8 bits set in low
    ASSERT_EQ(x.leading_zeros(), 120); // 128 - 8
}

// =============================================================================
// Bit Width Tests (3 tests)
// =============================================================================

TEST(bit_width_simple_tc)
{
    const uint128_tc_t x{0, 8}; // 0b1000
    ASSERT_EQ(x.bit_width(), 4);
}

TEST(bit_width_zero_tc)
{
    const uint128_tc_t x{0, 0};
    ASSERT_EQ(x.bit_width(), 0);
}

TEST(bit_width_high_bit_tc)
{
    const uint128_tc_t x{0x8000000000000000ULL, 0};
    ASSERT_EQ(x.bit_width(), 128);
}

TEST(bit_width_ms_magnitude)
{
    int128_ms_t x{0, 0};
    x.set_low(16); // 0b10000, magnitude
    ASSERT_EQ(x.bit_width(), 5);
}

// =============================================================================
// Is Power of 2 Tests (4 tests)
// =============================================================================

TEST(is_power_of_2_true_simple)
{
    const uint128_tc_t x{0, 8}; // 2^3
    ASSERT_TRUE(x.is_power_of_2());
}

TEST(is_power_of_2_false_simple)
{
    const uint128_tc_t x{0, 6}; // 0b110, not power of 2
    ASSERT_FALSE(x.is_power_of_2());
}

TEST(is_power_of_2_zero)
{
    const uint128_tc_t x{0, 0};
    ASSERT_FALSE(x.is_power_of_2());
}

TEST(is_power_of_2_high_bit)
{
    const uint128_tc_t x{0x8000000000000000ULL, 0};
    ASSERT_TRUE(x.is_power_of_2());
}

TEST(is_power_of_2_one)
{
    const uint128_tc_t x{0, 1};
    ASSERT_TRUE(x.is_power_of_2());
}

TEST(is_power_of_2_ms_magnitude)
{
    int128_ms_t x{0, 0};
    x.set_low(32); // 2^5
    ASSERT_TRUE(x.is_power_of_2());
}

// =============================================================================
// Count Ones / Popcount Tests (4 tests)
// =============================================================================

TEST(count_ones_simple_tc)
{
    const uint128_tc_t x{0, 0b1101}; // 3 bits set
    ASSERT_EQ(x.count_ones(), 3);
}

TEST(count_ones_all_ones_tc)
{
    const uint128_tc_t x{~0ULL, ~0ULL}; // All 128 bits set
    ASSERT_EQ(x.count_ones(), 128);
}

TEST(count_ones_zero_tc)
{
    const uint128_tc_t x{0, 0};
    ASSERT_EQ(x.count_ones(), 0);
}

TEST(popcount_alias_tc)
{
    const uint128_tc_t x{0, 0xFF}; // 8 bits set
    ASSERT_EQ(x.popcount(), 8);
    ASSERT_EQ(x.count_ones(), x.popcount()); // Verify alias works
}

TEST(count_ones_ms_magnitude)
{
    int128_ms_t x{0, 0};
    x.set_low(0xFFFFFFFFFFFFFFFFULL);  // 64 bits in low
    x.set_high(0x0000000000000000ULL); // No sign bit
    ASSERT_EQ(x.count_ones(), 64);
}

// =============================================================================
// Rotate Left Tests (3 tests)
// =============================================================================

TEST(rotate_left_simple_tc)
{
    const uint128_tc_t x{0, 1}; // 0b1
    const auto result{x.rotate_left(1)};
    ASSERT_EQ(result.high(), 0);
    ASSERT_EQ(result.low(), 2); // 0b10
}

TEST(rotate_left_wraparound_tc)
{
    const uint128_tc_t x{0x8000000000000000ULL, 0}; // MSB set
    const auto result{x.rotate_left(1)};
    ASSERT_EQ(result.high(), 0);
    ASSERT_EQ(result.low(), 1); // Bit wraps to LSB
}

TEST(rotate_left_zero_shift_tc)
{
    const uint128_tc_t x{0x1234, 0x5678};
    const auto result{x.rotate_left(0)};
    ASSERT_EQ(result.high(), 0x1234);
    ASSERT_EQ(result.low(), 0x5678);
}

TEST(rotate_left_64_shift_tc)
{
    const uint128_tc_t x{0x1234, 0x5678};
    const auto result{x.rotate_left(64)};
    // After 64-bit rotation, high and low swap
    ASSERT_EQ(result.high(), 0x5678);
    ASSERT_EQ(result.low(), 0x1234);
}

// =============================================================================
// Rotate Right Tests (3 tests)
// =============================================================================

TEST(rotate_right_simple_tc)
{
    const uint128_tc_t x{0, 2}; // 0b10
    const auto result{x.rotate_right(1)};
    ASSERT_EQ(result.high(), 0);
    ASSERT_EQ(result.low(), 1); // 0b1
}

TEST(rotate_right_wraparound_tc)
{
    const uint128_tc_t x{0, 1}; // LSB set
    const auto result{x.rotate_right(1)};
    ASSERT_EQ(result.high(), 0x8000000000000000ULL); // Bit wraps to MSB
    ASSERT_EQ(result.low(), 0);
}

TEST(rotate_right_zero_shift_tc)
{
    const uint128_tc_t x{0x1234, 0x5678};
    const auto result{x.rotate_right(0)};
    ASSERT_EQ(result.high(), 0x1234);
    ASSERT_EQ(result.low(), 0x5678);
}

TEST(rotate_right_64_shift_tc)
{
    const uint128_tc_t x{0x1234, 0x5678};
    const auto result{x.rotate_right(64)};
    // After 64-bit rotation, high and low swap
    ASSERT_EQ(result.high(), 0x5678);
    ASSERT_EQ(result.low(), 0x1234);
}

// =============================================================================
// Cross-Type Tests (MS-specific, 4 tests)
// =============================================================================

TEST(ms_trailing_zeros_sign_ignored)
{
    // MS: sign bit should be ignored in trailing zeros count
    int128_ms_t neg{0, 0};
    neg.set_high(1ULL << 63); // Sign bit only
    neg.set_low(8);           // Magnitude 8 (0b1000)
    ASSERT_EQ(neg.trailing_zeros(), 3);
}

TEST(ms_leading_zeros_magnitude_only)
{
    // MS: leading zeros counts in 127-bit magnitude
    int128_ms_t x{0, 0};
    x.set_low(0xFFFFFFFFFFFFFFFFULL);
    x.set_high(0x7FFFFFFFFFFFFFFFULL); // All magnitude bits set
    // No leading zeros in magnitude
    ASSERT_EQ(x.leading_zeros(), 0);
}

TEST(ms_popcount_ignores_sign)
{
    int128_ms_t neg{0, 0};
    neg.set_high(1ULL << 63); // Sign bit set
    neg.set_low(0xFF);        // 8 bits in magnitude
    // Should count 8 bits (sign bit ignored)
    ASSERT_EQ(neg.count_ones(), 8);
}

TEST(ms_is_power_of_2_magnitude)
{
    int128_ms_t x{0, 0};
    x.set_low(16); // Magnitude 16 = 2^4
    ASSERT_TRUE(x.is_power_of_2());

    x.set_low(17); // Magnitude 17 (not power of 2)
    ASSERT_FALSE(x.is_power_of_2());
}

// =============================================================================
// Edge Case Tests (2 tests)
// =============================================================================

TEST(edge_large_rotation)
{
    const uint128_tc_t x{0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL};
    const auto result{x.rotate_left(127)};
    // 127-bit rotation is same as 1-bit right rotation
    const auto expected{x.rotate_right(1)};
    ASSERT_EQ(result.high(), expected.high());
    ASSERT_EQ(result.low(), expected.low());
}

TEST(edge_trailing_zeros_one)
{
    const uint128_tc_t x{0, 1};
    ASSERT_EQ(x.trailing_zeros(), 0);
}

// =============================================================================
// Main Function
// =============================================================================

int main()
{
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Priority 8: Bit Manipulation Tests\n";
    std::cout << "========================================\n\n";

    std::cout << "\nTest Summary:\n";
    std::cout << "  Total:  " << g_test_count << "\n";
    std::cout << "  Passed: " << g_passed << " [OK]\n";

    if (g_failed > 0)
    {
        std::cout << "  Failed: " << g_failed << " [FAIL]\n";
        return 1;
    }
    else
    {
        std::cout << "  Failed: 0\n";
        std::cout << "\nAll tests passed!\n";
        return 0;
    }
}
