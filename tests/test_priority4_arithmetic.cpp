// =============================================================================
// Priority 4 Tests: Arithmetic and ±0 Methods (Phase 1.75)
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0

#include "representation.hpp"
#include "int128_parameterized.hpp"

#include <iostream>
#include <cassert>
#include <string>

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
#define ASSERT_TRUE(a) assert((a) == true)
#define ASSERT_FALSE(a) assert((a) == false)

// ============================================================================
// ±0 Tests (MS Signed Only)
// ============================================================================

void test_positive_zero()
{
    int128_ms_t zero(0LL);
    ASSERT_TRUE(zero.is_zero());
    ASSERT_TRUE(zero.is_positive_zero());
    ASSERT_FALSE(zero.is_negative_zero());
}

void test_negative_zero()
{
    int128_ms_t neg_zero(0LL);
    neg_zero.set_high(0x8000000000000000ULL); // Set sign bit
    ASSERT_TRUE(neg_zero.is_zero());
    ASSERT_FALSE(neg_zero.is_positive_zero());
    ASSERT_TRUE(neg_zero.is_negative_zero());
}

void test_zero_distinction()
{
    int128_ms_t pos_zero(0LL);
    int128_ms_t neg_zero(0LL);
    neg_zero.set_high(0x8000000000000000ULL);

    ASSERT_TRUE(pos_zero == neg_zero); // Equal as values
    ASSERT_TRUE(pos_zero.is_positive_zero());
    ASSERT_TRUE(neg_zero.is_negative_zero());
}

void test_nonzero_not_positive_zero()
{
    int128_ms_t x(42LL);
    ASSERT_FALSE(x.is_positive_zero());
    ASSERT_FALSE(x.is_negative_zero());
}

void test_nonzero_not_negative_zero()
{
    int128_ms_t x(-42LL);
    ASSERT_FALSE(x.is_positive_zero());
    ASSERT_FALSE(x.is_negative_zero());
}

// ============================================================================
// Addition Tests
// ============================================================================

void test_add_positive()
{
    int128_t a(100LL);
    int128_t b(50LL);
    int128_t c = a + b;
    ASSERT_EQ(c.low(), 150ULL);
    ASSERT_EQ(c.high(), 0ULL);
}

void test_add_negative()
{
    int128_t a(-100LL);
    int128_t b(-50LL);
    int128_t c = a + b;
    // -100 - 50 = -150
    // In TC: data[0] = 0xFFFFFFFFFFFFFF6A, data[1] = 0xFFFFFFFFFFFFFFFF
    ASSERT_TRUE(c.is_negative());
}

void test_add_assign()
{
    int128_t x(42LL);
    x += int128_t(8LL);
    ASSERT_EQ(x.low(), 50ULL);
}

void test_add_zero()
{
    int128_t x(100LL);
    int128_t y(0LL);
    int128_t z = x + y;
    ASSERT_EQ(z.low(), 100ULL);
}

void test_add_unsigned()
{
    uint128_t a(100ULL);
    uint128_t b(50ULL);
    uint128_t c = a + b;
    ASSERT_EQ(c.low(), 150ULL);
}

// ============================================================================
// Subtraction Tests
// ============================================================================

void test_sub_positive()
{
    int128_t a(100LL);
    int128_t b(50LL);
    int128_t c = a - b;
    ASSERT_EQ(c.low(), 50ULL);
}

void test_sub_assign()
{
    int128_t x(100LL);
    x -= int128_t(50LL);
    ASSERT_EQ(x.low(), 50ULL);
}

void test_sub_zero()
{
    int128_t x(100LL);
    int128_t y(0LL);
    int128_t z = x - y;
    ASSERT_EQ(z.low(), 100ULL);
}

void test_sub_unsigned()
{
    uint128_t a(100ULL);
    uint128_t b(50ULL);
    uint128_t c = a - b;
    ASSERT_EQ(c.low(), 50ULL);
}

// ============================================================================
// Unary Negation Tests
// ============================================================================

void test_negate_positive()
{
    int128_t x(42LL);
    int128_t neg_x = -x;
    ASSERT_TRUE(neg_x.is_negative());
    ASSERT_EQ(neg_x + x, int128_t(0LL));
}

void test_negate_negative()
{
    int128_t x(-42LL);
    int128_t neg_x = -x;
    ASSERT_FALSE(neg_x.is_negative());
    ASSERT_EQ(neg_x.low(), 42ULL);
}

void test_negate_zero()
{
    int128_t zero(0LL);
    int128_t neg_zero = -zero;
    ASSERT_TRUE(neg_zero.is_zero());
}

void test_negate_ms_trivial()
{
    int128_ms_t x(42LL);
    int128_ms_t neg_x = -x;
    ASSERT_TRUE(neg_x.is_negative());
    ASSERT_FALSE(x.is_negative());
    // Sign bit should be flipped
    ASSERT_EQ(neg_x.get_sign(), -1);
}

void test_unary_plus()
{
    int128_t x(42LL);
    int128_t y = +x;
    ASSERT_EQ(x, y);
}

// ============================================================================
// Multiplication Tests
// ============================================================================

void test_mult_small()
{
    int128_t a(10LL);
    int128_t b(5LL);
    int128_t c = a * b;
    ASSERT_EQ(c.low(), 50ULL);
}

void test_mult_zero()
{
    int128_t a(42LL);
    int128_t zero(0LL);
    int128_t c = a * zero;
    ASSERT_TRUE(c.is_zero());
}

void test_mult_one()
{
    int128_t a(42LL);
    int128_t one(1LL);
    int128_t c = a * one;
    ASSERT_EQ(c.low(), 42ULL);
}

void test_mult_assign()
{
    int128_t x(10LL);
    x *= int128_t(5LL);
    ASSERT_EQ(x.low(), 50ULL);
}

void test_mult_unsigned()
{
    uint128_t a(10ULL);
    uint128_t b(5ULL);
    uint128_t c = a * b;
    ASSERT_EQ(c.low(), 50ULL);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main()
{
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "PRIORITY 4 TESTS: Arithmetic & ±0 Methods" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::cout << "\n[1-5] ±0 Distinction (MS Signed):" << std::endl;
    TEST_CASE("test_positive_zero", test_positive_zero);
    TEST_CASE("test_negative_zero", test_negative_zero);
    TEST_CASE("test_zero_distinction", test_zero_distinction);
    TEST_CASE("test_nonzero_not_positive_zero", test_nonzero_not_positive_zero);
    TEST_CASE("test_nonzero_not_negative_zero", test_nonzero_not_negative_zero);

    std::cout << "\n[6-10] Addition:" << std::endl;
    TEST_CASE("test_add_positive", test_add_positive);
    TEST_CASE("test_add_negative", test_add_negative);
    TEST_CASE("test_add_assign", test_add_assign);
    TEST_CASE("test_add_zero", test_add_zero);
    TEST_CASE("test_add_unsigned", test_add_unsigned);

    std::cout << "\n[11-14] Subtraction:" << std::endl;
    TEST_CASE("test_sub_positive", test_sub_positive);
    TEST_CASE("test_sub_assign", test_sub_assign);
    TEST_CASE("test_sub_zero", test_sub_zero);
    TEST_CASE("test_sub_unsigned", test_sub_unsigned);

    std::cout << "\n[15-19] Unary Negation:" << std::endl;
    TEST_CASE("test_negate_positive", test_negate_positive);
    TEST_CASE("test_negate_negative", test_negate_negative);
    TEST_CASE("test_negate_zero", test_negate_zero);
    TEST_CASE("test_negate_ms_trivial", test_negate_ms_trivial);
    TEST_CASE("test_unary_plus", test_unary_plus);

    std::cout << "\n[20-24] Multiplication:" << std::endl;
    TEST_CASE("test_mult_small", test_mult_small);
    TEST_CASE("test_mult_zero", test_mult_zero);
    TEST_CASE("test_mult_one", test_mult_one);
    TEST_CASE("test_mult_assign", test_mult_assign);
    TEST_CASE("test_mult_unsigned", test_mult_unsigned);

    std::cout << "\n"
              << std::string(70, '=') << std::endl;
    std::cout << "RESULTS: " << pass_count << "/" << test_count << " PASSED" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
