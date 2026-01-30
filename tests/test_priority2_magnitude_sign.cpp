#include "representation.hpp"
#include "int128_parameterized.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <climits>
#include <limits>

// Macro definitions
#define TEST_CASE(name, func)                                                  \
    do                                                                         \
    {                                                                          \
        test_count++;                                                          \
        try                                                                    \
        {                                                                      \
            func();                                                            \
            pass_count++;                                                      \
            std::cout << "  PASS: " << name << std::endl;                      \
        }                                                                      \
        catch (const std::exception &e)                                        \
        {                                                                      \
            std::cout << "  FAIL: " << name << " - " << e.what() << std::endl; \
        }                                                                      \
    } while (false)
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_TRUE(a) assert((a) == true)
#define ASSERT_FALSE(a) assert((a) == false)

int test_count = 0, pass_count = 0;

// Test 1-5: is_negative() on various values
void test_is_negative_positive()
{
    nstd::int128_ms_t x(42LL);
    ASSERT_FALSE(x.is_negative());
}
void test_is_negative_negative()
{
    nstd::int128_ms_t x(-42LL);
    ASSERT_TRUE(x.is_negative());
}
void test_is_negative_zero()
{
    nstd::int128_ms_t x(0LL);
    ASSERT_FALSE(x.is_negative());
}
void test_is_negative_max()
{
    nstd::int128_ms_t x(std::numeric_limits<int64_t>::max());
    ASSERT_FALSE(x.is_negative());
}
void test_is_negative_min()
{
    nstd::int128_ms_t x(std::numeric_limits<int64_t>::min());
    ASSERT_TRUE(x.is_negative());
}

// Test 6-10: is_zero() and basic magnitude properties
void test_is_zero_positive()
{
    nstd::int128_ms_t x(100LL);
    ASSERT_FALSE(x.is_zero());
}
void test_is_zero_negative()
{
    nstd::int128_ms_t x(-100LL);
    ASSERT_FALSE(x.is_zero());
}
void test_is_zero_true()
{
    nstd::int128_ms_t x(0LL);
    ASSERT_TRUE(x.is_zero());
}
void test_low_high_access()
{
    nstd::int128_ms_t x(0xEF01, 0xABCD); // high, low
    ASSERT_EQ(x.low(), 0xABCD);
    ASSERT_EQ(x.high(), 0xEF01);
}
void test_low_high_setters()
{
    nstd::int128_ms_t x(0, 0);
    x.set_low(0x1234);
    x.set_high(0x5678);
    ASSERT_EQ(x.low(), 0x1234);
}

// Test 11-15: Two's complement representation for signed
void test_twos_complement_neg1()
{
    nstd::int128_ms_t x(-1LL);
    // Validar semántica: signo y magnitud
    ASSERT_TRUE(x.is_negative());
    auto mag = x.magnitude();
    ASSERT_EQ(mag, nstd::int128_ms_t(1LL));
}
void test_twos_complement_neg42()
{
    nstd::int128_ms_t x(-42LL);
    ASSERT_TRUE(x.is_negative());
}
void test_compare_pos_vs_zero()
{
    nstd::int128_ms_t pos(50LL), zero(0LL);
    ASSERT_TRUE(pos > zero);
}
void test_compare_zero_vs_neg()
{
    nstd::int128_ms_t zero(0LL), neg(-1LL);
    ASSERT_TRUE(zero > neg);
}
void test_compare_pos_vs_neg()
{
    nstd::int128_ms_t pos(100LL), neg(-100LL);
    ASSERT_TRUE(pos > neg);
}

// Test 16-20: Equality and inequality
void test_equal_same_value()
{
    nstd::int128_ms_t x(42LL), y(42LL);
    ASSERT_TRUE(x == y);
}
void test_unequal_diff_value()
{
    nstd::int128_ms_t x(42LL), y(-42LL);
    ASSERT_TRUE(x != y);
}
void test_equal_zero()
{
    nstd::int128_ms_t x(0LL), y(0LL);
    ASSERT_TRUE(x == y);
}
void test_copy_preserves_value()
{
    nstd::int128_ms_t x(99LL);
    nstd::int128_ms_t y = x;
    ASSERT_EQ(y, x);
}
void test_copy_preserves_sign()
{
    nstd::int128_ms_t x(-99LL);
    nstd::int128_ms_t y = x;
    ASSERT_TRUE(y.is_negative());
}

// Test 21-25: Unsigned type behavior
void test_uint_positive()
{
    nstd::uint128_ms_t u(100ULL);
    // Unsigned types don't have is_negative()
    ASSERT_FALSE(u.is_zero());
}
void test_uint_zero()
{
    nstd::uint128_ms_t u(0ULL);
    ASSERT_TRUE(u.is_zero());
}
void test_uint_max_representation()
{
    nstd::uint128_ms_t u(std::numeric_limits<uint64_t>::max());
    ASSERT_FALSE(u.is_zero());
}
void test_uint_low_high()
{
    nstd::uint128_ms_t u(0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL);
    // Constructor takes (high, low) in Western order
    ASSERT_EQ(u.high(), 0x1122334455667788ULL);
    ASSERT_EQ(u.low(), 0x99AABBCCDDEEFF00ULL);
}
void test_uint_compare()
{
    nstd::uint128_ms_t u1(100ULL), u2(50ULL);
    ASSERT_TRUE(u1 > u2);
}

// Test 26-30: Mixed comparisons and value properties
void test_compare_with_literal()
{
    nstd::int128_ms_t x(42LL);
    nstd::int128_ms_t zero(0LL);
    ASSERT_TRUE(x > zero);
}
void test_compare_negative_with_zero()
{
    nstd::int128_ms_t x(-1LL);
    nstd::int128_ms_t zero(0LL);
    ASSERT_TRUE(x < zero);
}
void test_compare_eq_with_literal()
{
    nstd::int128_ms_t x(100LL);
    nstd::int128_ms_t y(100LL);
    ASSERT_TRUE(x == y);
}
void test_value_diff_properties()
{
    nstd::int128_ms_t x(50LL), y(100LL);
    ASSERT_NE(x, y);
}
void test_neg_to_pos_inequality()
{
    nstd::int128_ms_t neg(-1LL), pos(1LL);
    ASSERT_NE(neg, pos);
}

// Test 31-35: Edge cases and boundary conditions
void test_one()
{
    nstd::int128_ms_t x(1LL);
    ASSERT_FALSE(x.is_negative());
    ASSERT_FALSE(x.is_zero());
}
void test_minus_one()
{
    nstd::int128_ms_t x(-1LL);
    ASSERT_TRUE(x.is_negative());
    ASSERT_FALSE(x.is_zero());
}
void test_boundary_positive()
{
    nstd::int128_ms_t x(std::numeric_limits<int64_t>::max());
    ASSERT_FALSE(x.is_negative());
}
void test_boundary_negative()
{
    nstd::int128_ms_t x(std::numeric_limits<int64_t>::min());
    ASSERT_TRUE(x.is_negative());
}
void test_copy_preserves_all()
{
    nstd::int128_ms_t x(std::numeric_limits<int64_t>::min());
    nstd::int128_ms_t y = x;
    ASSERT_EQ(y.low(), x.low());
    ASSERT_EQ(y.high(), x.high());
}

int main()
{
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "PRIORITY 2 TESTS: MS Representation Methods" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // Test group 1: is_negative() on signed
    std::cout << "\n[1-5] is_negative() Tests:" << std::endl;
    TEST_CASE("test_is_negative_positive", test_is_negative_positive);
    TEST_CASE("test_is_negative_negative", test_is_negative_negative);
    TEST_CASE("test_is_negative_zero", test_is_negative_zero);
    TEST_CASE("test_is_negative_max", test_is_negative_max);
    TEST_CASE("test_is_negative_min", test_is_negative_min);

    // Test group 2: is_zero() and accessors
    std::cout << "\n[6-10] is_zero() and Accessor Tests:" << std::endl;
    TEST_CASE("test_is_zero_positive", test_is_zero_positive);
    TEST_CASE("test_is_zero_negative", test_is_zero_negative);
    TEST_CASE("test_is_zero_true", test_is_zero_true);
    TEST_CASE("test_low_high_access", test_low_high_access);
    TEST_CASE("test_low_high_setters", test_low_high_setters);

    // Test group 3: Two's complement representation
    std::cout << "\n[11-15] Two's Complement Representation:" << std::endl;
    TEST_CASE("test_twos_complement_neg1", test_twos_complement_neg1);
    TEST_CASE("test_twos_complement_neg42", test_twos_complement_neg42);
    TEST_CASE("test_compare_pos_vs_zero", test_compare_pos_vs_zero);
    TEST_CASE("test_compare_zero_vs_neg", test_compare_zero_vs_neg);
    TEST_CASE("test_compare_pos_vs_neg", test_compare_pos_vs_neg);

    // Test group 4: Equality and copying
    std::cout << "\n[16-20] Equality and Copy Semantics:" << std::endl;
    TEST_CASE("test_equal_same_value", test_equal_same_value);
    TEST_CASE("test_unequal_diff_value", test_unequal_diff_value);
    TEST_CASE("test_equal_zero", test_equal_zero);
    TEST_CASE("test_copy_preserves_value", test_copy_preserves_value);
    TEST_CASE("test_copy_preserves_sign", test_copy_preserves_sign);

    // Test group 5: Unsigned type behavior
    std::cout << "\n[21-25] Unsigned Type Behavior:" << std::endl;
    TEST_CASE("test_uint_positive", test_uint_positive);
    TEST_CASE("test_uint_zero", test_uint_zero);
    TEST_CASE("test_uint_max_representation", test_uint_max_representation);
    TEST_CASE("test_uint_low_high", test_uint_low_high);
    TEST_CASE("test_uint_compare", test_uint_compare);

    // Test group 6: Mixed comparisons
    std::cout << "\n[26-30] Mixed Comparisons:" << std::endl;
    TEST_CASE("test_compare_with_literal", test_compare_with_literal);
    TEST_CASE("test_compare_negative_with_zero", test_compare_negative_with_zero);
    TEST_CASE("test_compare_eq_with_literal", test_compare_eq_with_literal);
    TEST_CASE("test_value_diff_properties", test_value_diff_properties);
    TEST_CASE("test_neg_to_pos_inequality", test_neg_to_pos_inequality);

    // Test group 7: Edge cases
    std::cout << "\n[31-35] Edge Cases & Boundaries:" << std::endl;
    TEST_CASE("test_one", test_one);
    TEST_CASE("test_minus_one", test_minus_one);
    TEST_CASE("test_boundary_positive", test_boundary_positive);
    TEST_CASE("test_boundary_negative", test_boundary_negative);
    TEST_CASE("test_copy_preserves_all", test_copy_preserves_all);

    std::cout << std::string(70, '=') << std::endl;
    std::cout << "RESULTS: " << pass_count << "/" << test_count << " PASSED" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
