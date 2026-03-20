// =============================================================================
// Test Suite: Priority 9 - Friend Operators & Helper Methods
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include <cassert>
#include <iostream>
#include <utility>

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
// Helper Methods Tests (5 tests)
// =============================================================================

TEST(divmod_simple_tc)
{
    const uint128_t dividend{0, 100};
    const uint128_t divisor{0, 7};
    const auto [quot, rem] = dividend.divmod(divisor);

    ASSERT_EQ(quot.low(), 14);
    ASSERT_EQ(rem.low(), 2);
}

TEST(divmod_large_tc)
{
    const uint128_t dividend{0, 0xFFFFFFFFFFFFFFFFULL};
    const uint128_t divisor{0, 1000};
    const auto [quot, rem] = dividend.divmod(divisor);

    // 2^64 - 1 = 18446744073709551615
    // Quotient: 18446744073709551615 / 1000 = 18446744073709551
    // Remainder: 18446744073709551615 % 1000 = 615
    ASSERT_EQ(quot.low(), 18446744073709551ULL);
    ASSERT_EQ(rem.low(), 615);
}

TEST(abs_positive_tc)
{
    const int128_tc_t x{0, 42};
    const auto result = x.abs();
    ASSERT_EQ(result.low(), 42);
    ASSERT_EQ(result.high(), 0);
}

TEST(abs_negative_tc)
{
    const int128_tc_t x{-1}; // Signed -1
    const auto result = x.abs();
    ASSERT_EQ(result.low(), 1);
    ASSERT_EQ(result.high(), 0);
}

TEST(swap_simple)
{
    uint128_t a{0x1234, 0x5678};
    uint128_t b{0xABCD, 0xEF01};

    a.swap(b);

    ASSERT_EQ(a.high(), 0xABCD);
    ASSERT_EQ(a.low(), 0xEF01);
    ASSERT_EQ(b.high(), 0x1234);
    ASSERT_EQ(b.low(), 0x5678);
}

// =============================================================================
// Friend Addition Tests (3 tests)
// =============================================================================

TEST(friend_add_int128_plus_int)
{
    const uint128_t x{0, 100};
    const auto result = x + 50; // int128 + int
    ASSERT_EQ(result.low(), 150);
}

TEST(friend_add_int_plus_int128)
{
    const uint128_t x{0, 100};
    const auto result = 50 + x; // int + int128 (symmetric)
    ASSERT_EQ(result.low(), 150);
}

TEST(friend_add_uint64_plus_int128)
{
    const uint128_t x{0, 0xFFFFFFFFFFFFFF00ULL};
    const auto result = 0x100ULL + x;
    ASSERT_EQ(result.low(), 0);
    ASSERT_EQ(result.high(), 1); // Overflow to high limb
}

// =============================================================================
// Friend Subtraction Tests (2 tests)
// =============================================================================

TEST(friend_sub_int128_minus_int)
{
    const uint128_t x{0, 100};
    const auto result = x - 30;
    ASSERT_EQ(result.low(), 70);
}

TEST(friend_sub_int_minus_int128)
{
    const uint128_t x{0, 30};
    const auto result = 100 - x; // int - int128
    ASSERT_EQ(result.low(), 70);
}

// =============================================================================
// Friend Multiplication Tests (2 tests)
// =============================================================================

TEST(friend_mul_int128_times_int)
{
    const uint128_t x{0, 10};
    const auto result = x * 5;
    ASSERT_EQ(result.low(), 50);
}

TEST(friend_mul_int_times_int128)
{
    const uint128_t x{0, 10};
    const auto result = 5 * x; // int * int128
    ASSERT_EQ(result.low(), 50);
}

// =============================================================================
// Friend Comparison Tests (6 tests)
// =============================================================================

TEST(friend_eq_int128_eq_int)
{
    const uint128_t x{0, 42};
    ASSERT_TRUE(x == 42);
}

TEST(friend_eq_int_eq_int128)
{
    const uint128_t x{0, 42};
    ASSERT_TRUE(42 == x);
}

TEST(friend_lt_int128_lt_int)
{
    const uint128_t x{0, 10};
    ASSERT_TRUE(x < 20);
    ASSERT_FALSE(x < 5);
}

TEST(friend_lt_int_lt_int128)
{
    const uint128_t x{0, 10};
    ASSERT_TRUE(5 < x);
    ASSERT_FALSE(20 < x);
}

TEST(friend_gt_int128_gt_int)
{
    const uint128_t x{0, 20};
    ASSERT_TRUE(x > 10);
    ASSERT_FALSE(x > 30);
}

TEST(friend_gt_int_gt_int128)
{
    const uint128_t x{0, 10};
    ASSERT_TRUE(20 > x);
    ASSERT_FALSE(5 > x);
}

// =============================================================================
// Friend Bitwise Tests (3 tests)
// =============================================================================

TEST(friend_and_int128_and_int)
{
    const uint128_t x{0, 0xFF};
    const auto result = x & 0x0F;
    ASSERT_EQ(result.low(), 0x0F);
}

TEST(friend_or_int_or_int128)
{
    const uint128_t x{0, 0xF0};
    const auto result = 0x0F | x;
    ASSERT_EQ(result.low(), 0xFF);
}

TEST(friend_xor_int128_xor_int)
{
    const uint128_t x{0, 0xFF};
    const auto result = x ^ 0x0F;
    ASSERT_EQ(result.low(), 0xF0);
}

// =============================================================================
// ADL Swap Test (1 test)
// =============================================================================

TEST(adl_swap)
{
    uint128_t a{0x1234, 0x5678};
    uint128_t b{0xABCD, 0xEF01};

    using std::swap; // Enable ADL
    swap(a, b);

    ASSERT_EQ(a.high(), 0xABCD);
    ASSERT_EQ(a.low(), 0xEF01);
    ASSERT_EQ(b.high(), 0x1234);
    ASSERT_EQ(b.low(), 0x5678);
}

// =============================================================================
// Magnitude-Sign Specific Tests (3 tests)
// =============================================================================

TEST(abs_ms_negative)
{
    int128_ms_t x{0, 0};
    x.set_high(1ULL << 63); // Sign bit
    x.set_low(42);          // Magnitude 42

    const auto result = x.abs();
    ASSERT_EQ(result.low(), 42);
    ASSERT_EQ(result.high(), 0); // Sign bit cleared
}

TEST(divmod_ms_signed)
{
    const int128_ms_t dividend{0, 100};
    const int128_ms_t divisor{0, 7};
    const auto [quot, rem] = dividend.divmod(divisor);

    ASSERT_EQ(quot.low(), 14);
    ASSERT_EQ(rem.low(), 2);
}

TEST(friend_comparison_ms)
{
    const int128_ms_t x{0, 42};
    ASSERT_TRUE(x == 42);
    ASSERT_TRUE(42 == x);
    ASSERT_FALSE(x < 10);
    ASSERT_TRUE(x > 10);
}

// =============================================================================
// Main Function
// =============================================================================

int main()
{
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Priority 9: Friend Operators Tests\n";
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
