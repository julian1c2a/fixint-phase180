// =============================================================================
// Test Suite: Priority 10 - Float/Double Conversions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <limits>

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

#define ASSERT_NEAR(expr, expected, epsilon)                                          \
    do                                                                                \
    {                                                                                 \
        auto val = (expr);                                                            \
        auto exp = (expected);                                                        \
        if (std::abs(val - exp) > epsilon)                                            \
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

// =============================================================================
// To Double Tests (4 tests)
// =============================================================================

TEST(to_double_simple_tc)
{
    const uint128_tc_t x{0, 100};
    const double d{static_cast<double>(x)};
    ASSERT_NEAR(d, 100.0, 0.001);
}

TEST(to_double_large_tc)
{
    const uint128_tc_t x{0, 0xFFFFFFFFFFFFFFFFULL}; // 2^64 - 1
    const double d{static_cast<double>(x)};
    ASSERT_NEAR(d, 18446744073709551615.0, 1.0);
}

TEST(to_double_high_limb_tc)
{
    const uint128_tc_t x{1, 0}; // 2^64
    const double d{static_cast<double>(x)};
    ASSERT_NEAR(d, 18446744073709551616.0, 1.0);
}

TEST(to_double_negative_tc)
{
    const int128_tc_t x{-100};
    const double d{static_cast<double>(x)};
    ASSERT_NEAR(d, -100.0, 0.001);
}

// =============================================================================
// To Long Double Tests (2 tests)
// =============================================================================

TEST(to_long_double_simple_tc)
{
    const uint128_tc_t x{0, 1000};
    const long double ld{static_cast<long double>(x)};
    ASSERT_NEAR(static_cast<double>(ld), 1000.0, 0.001);
}

TEST(to_long_double_negative_tc)
{
    const int128_tc_t x{-1000};
    const long double ld{static_cast<long double>(x)};
    ASSERT_NEAR(static_cast<double>(ld), -1000.0, 0.001);
}

// =============================================================================
// From Double Tests (4 tests)
// =============================================================================

TEST(from_double_simple_tc)
{
    const uint128_tc_t x{100.0};
    ASSERT_EQ(x.low(), 100);
    ASSERT_EQ(x.high(), 0);
}

TEST(from_double_truncate_tc)
{
    const uint128_tc_t x{123.456}; // Should truncate to 123
    ASSERT_EQ(x.low(), 123);
}

TEST(from_double_negative_tc)
{
    const int128_tc_t x{-100.0};
    const double back{static_cast<double>(x)};
    ASSERT_NEAR(back, -100.0, 0.001);
}

TEST(from_double_large_tc)
{
    const uint128_tc_t x{18446744073709551616.0}; // 2^64
    ASSERT_EQ(x.high(), 1);
    ASSERT_EQ(x.low(), 0);
}

// =============================================================================
// From Long Double Tests (2 tests)
// =============================================================================

TEST(from_long_double_simple_tc)
{
    const uint128_tc_t x{1000.0L};
    ASSERT_EQ(x.low(), 1000);
    ASSERT_EQ(x.high(), 0);
}

TEST(from_long_double_negative_tc)
{
    const int128_tc_t x{-500.0L};
    const long double back{static_cast<long double>(x)};
    ASSERT_NEAR(static_cast<double>(back), -500.0, 0.001);
}

// =============================================================================
// Round-Trip Tests (2 tests)
// =============================================================================

TEST(roundtrip_double_tc)
{
    const uint128_tc_t original{0, 12345};
    const double d{static_cast<double>(original)};
    const uint128_tc_t restored{d};

    ASSERT_EQ(restored.low(), original.low());
    ASSERT_EQ(restored.high(), original.high());
}

TEST(roundtrip_negative_double_tc)
{
    const int128_tc_t original{-12345};
    const double d{static_cast<double>(original)};
    const int128_tc_t restored{d};

    // Convert back to double for comparison
    const double original_d{static_cast<double>(original)};
    const double restored_d{static_cast<double>(restored)};
    ASSERT_NEAR(restored_d, original_d, 0.001);
}

// =============================================================================
// MS-Specific Tests (2 tests)
// =============================================================================

TEST(to_double_ms_negative)
{
    int128_ms_t x{0, 0};
    x.set_high(1ULL << 63); // Sign bit
    x.set_low(42);          // Magnitude 42

    const double d{static_cast<double>(x)};
    ASSERT_NEAR(d, -42.0, 0.001);
}

TEST(from_double_ms_negative)
{
    const int128_ms_t x{-100.0};

    // Check sign bit is set
    ASSERT_TRUE((x.high() & (1ULL << 63)) != 0);

    // Check magnitude (clear sign bit and verify)
    const uint64_t mag_high{x.high() & ~(1ULL << 63)};
    ASSERT_EQ(mag_high, 0);
    ASSERT_EQ(x.low(), 100);
}

// =============================================================================
// Edge Case Tests (2 tests)
// =============================================================================

TEST(from_double_zero)
{
    const uint128_tc_t x{0.0};
    ASSERT_EQ(x.low(), 0);
    ASSERT_EQ(x.high(), 0);
}

TEST(from_double_nan_becomes_zero)
{
    const double nan_val{std::numeric_limits<double>::quiet_NaN()};
    const uint128_tc_t x{nan_val};
    ASSERT_EQ(x.low(), 0);
    ASSERT_EQ(x.high(), 0);
}

// =============================================================================
// Main Function
// =============================================================================

int main()
{
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Priority 10: Float Conversion Tests\n";
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
