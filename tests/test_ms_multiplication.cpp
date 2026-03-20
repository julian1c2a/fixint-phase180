// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "representation.hpp"
#include "int128_parameterized.hpp"
#include <iostream>
#include <cassert>
#include <limits>

// Macro definitions (same style as test_priority2_magnitude_sign.cpp)
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
#define ASSERT_TRUE(a)  assert((a) == true)
#define ASSERT_FALSE(a) assert((a) == false)

int test_count = 0, pass_count = 0;

// =============================================================================
// Group 1: Sign rules (small 64-bit values, no overflow)
// =============================================================================

// 3 × 4 = 12  (positive × positive = positive)
void test_sign_pos_pos()
{
    nstd::int128_ms_t a(3LL);
    nstd::int128_ms_t b(4LL);
    nstd::int128_ms_t expected(12LL);
    a *= b;
    ASSERT_EQ(a, expected);
    ASSERT_FALSE(a.is_negative());
}

// 3 × (−4) = −12  (positive × negative = negative)
void test_sign_pos_neg()
{
    nstd::int128_ms_t a(3LL);
    nstd::int128_ms_t b(-4LL);
    nstd::int128_ms_t expected(-12LL);
    a *= b;
    ASSERT_EQ(a, expected);
    ASSERT_TRUE(a.is_negative());
}

// (−3) × 4 = −12  (negative × positive = negative)
void test_sign_neg_pos()
{
    nstd::int128_ms_t a(-3LL);
    nstd::int128_ms_t b(4LL);
    nstd::int128_ms_t expected(-12LL);
    a *= b;
    ASSERT_EQ(a, expected);
    ASSERT_TRUE(a.is_negative());
}

// (−3) × (−4) = 12  (negative × negative = positive)
void test_sign_neg_neg()
{
    nstd::int128_ms_t a(-3LL);
    nstd::int128_ms_t b(-4LL);
    nstd::int128_ms_t expected(12LL);
    a *= b;
    ASSERT_EQ(a, expected);
    ASSERT_FALSE(a.is_negative());
}

// =============================================================================
// Group 2: Zero and identity
// =============================================================================

// x × 0 = 0
void test_zero_rhs()
{
    nstd::int128_ms_t a(999LL);
    nstd::int128_ms_t b(0LL);
    a *= b;
    ASSERT_TRUE(a.is_zero());
    ASSERT_FALSE(a.is_negative());
}

// 0 × x = 0
void test_zero_lhs()
{
    nstd::int128_ms_t a(0LL);
    nstd::int128_ms_t b(999LL);
    a *= b;
    ASSERT_TRUE(a.is_zero());
}

// x × 1 = x
void test_identity_rhs()
{
    nstd::int128_ms_t a(42LL);
    nstd::int128_ms_t b(1LL);
    nstd::int128_ms_t expected(42LL);
    a *= b;
    ASSERT_EQ(a, expected);
}

// (−x) × 1 = −x
void test_identity_neg_rhs()
{
    nstd::int128_ms_t a(-42LL);
    nstd::int128_ms_t b(1LL);
    nstd::int128_ms_t expected(-42LL);
    a *= b;
    ASSERT_EQ(a, expected);
    ASSERT_TRUE(a.is_negative());
}

// =============================================================================
// Group 3: Carry from low product into high word
//
// The portable fallback (used in constexpr contexts) computes the high 64 bits
// of a_low × b_low via 32-bit schoolbook multiplication.  These tests exercise
// cases where a_low × b_low ≥ 2^64.
//
// (a) (2^32 + 1)^2 = 2^64 + 2·2^32 + 1
//     stored as: data[1]=1, data[0]=0x200000001
//
// (b) (2^64 − 1) × 2 = 2^65 − 2
//     stored as: data[1]=1, data[0]=0xFFFFFFFFFFFFFFFE
// =============================================================================

// (2^32+1) × (2^32+1) — carry produces data[1]=1
void test_carry_low_product()
{
    // Construct via (high, low) constructor: data[1]=0 (positive), data[0]=2^32+1
    nstd::int128_ms_t a(0ULL, 0x100000001ULL);
    nstd::int128_ms_t b(0ULL, 0x100000001ULL);
    // Expected: (2^32+1)^2 = 2^64 + 2*2^32 + 1 = data[1]=1, data[0]=0x200000001
    nstd::int128_ms_t expected(1ULL, 0x200000001ULL);
    a *= b;
    ASSERT_EQ(a.low(),  expected.low());
    ASSERT_EQ(a.high(), expected.high());
    ASSERT_FALSE(a.is_negative());
}

// (2^64−1) × 2 — carry produces data[1]=1, data[0] = 0xFFFFFFFFFFFFFFFE
void test_carry_maxlow_times_two()
{
    nstd::int128_ms_t a(0ULL, 0xFFFFFFFFFFFFFFFFULL);
    nstd::int128_ms_t b(0ULL, 2ULL);
    // Expected: 2*(2^64-1) = 2^65-2 → data[1]=1, data[0]=0xFFFFFFFFFFFFFFFE
    nstd::int128_ms_t expected(1ULL, 0xFFFFFFFFFFFFFFFEULL);
    a *= b;
    ASSERT_EQ(a.low(),  expected.low());
    ASSERT_EQ(a.high(), expected.high());
    ASSERT_FALSE(a.is_negative());
}

// Sign not corrupted by carry: negative × negative with carry
// (−(2^32+1)) × (−(2^32+1)) = +(2^64+2^33+1) — result positive
void test_carry_neg_neg_sign()
{
    // Build negative (2^32+1): set sign bit in data[1]
    nstd::int128_ms_t a(0x8000000000000000ULL, 0x100000001ULL); // negative
    nstd::int128_ms_t b(0x8000000000000000ULL, 0x100000001ULL); // negative
    nstd::int128_ms_t expected(1ULL, 0x200000001ULL);            // positive
    a *= b;
    ASSERT_FALSE(a.is_negative());
    ASSERT_EQ(a.low(),  expected.low());
    ASSERT_EQ(a.high(), expected.high());
}

// =============================================================================
// Group 4: operator* (non-mutating) and constexpr
// =============================================================================

// operator* returns correct value without modifying lhs
void test_operator_star_value()
{
    const nstd::int128_ms_t a(7LL);
    const nstd::int128_ms_t b(6LL);
    nstd::int128_ms_t result = a * b;
    ASSERT_EQ(result, nstd::int128_ms_t(42LL));
    // lhs unchanged
    ASSERT_EQ(a, nstd::int128_ms_t(7LL));
}

// operator* sign: positive × negative
void test_operator_star_sign()
{
    const nstd::int128_ms_t a(10LL);
    const nstd::int128_ms_t b(-5LL);
    nstd::int128_ms_t result = a * b;
    ASSERT_EQ(result, nstd::int128_ms_t(-50LL));
    ASSERT_TRUE(result.is_negative());
}

// constexpr multiplication (uses portable schoolbook path)
void test_constexpr_mul()
{
    constexpr nstd::int128_ms_t result = nstd::int128_ms_t(9LL) * nstd::int128_ms_t(9LL);
    static_assert(result == nstd::int128_ms_t(81LL), "constexpr: 9*9 must equal 81");
    ASSERT_EQ(result, nstd::int128_ms_t(81LL));
}

// constexpr sign: negative × positive
void test_constexpr_mul_sign()
{
    constexpr nstd::int128_ms_t result = nstd::int128_ms_t(-5LL) * nstd::int128_ms_t(3LL);
    static_assert(result == nstd::int128_ms_t(-15LL), "constexpr: (-5)*3 must equal -15");
    ASSERT_EQ(result, nstd::int128_ms_t(-15LL));
}

// =============================================================================
// Group 5: Chained multiplication
// =============================================================================

// 2 × 3 × 4 = 24  (left-to-right)
void test_chained_mul()
{
    nstd::int128_ms_t a(2LL);
    a *= nstd::int128_ms_t(3LL);
    a *= nstd::int128_ms_t(4LL);
    ASSERT_EQ(a, nstd::int128_ms_t(24LL));
}

// (−1) × (−1) × (−1) = −1  (three negatives)
void test_chained_neg_neg_neg()
{
    nstd::int128_ms_t a(-1LL);
    a *= nstd::int128_ms_t(-1LL);  // → +1
    a *= nstd::int128_ms_t(-1LL);  // → −1
    ASSERT_EQ(a, nstd::int128_ms_t(-1LL));
    ASSERT_TRUE(a.is_negative());
}

int main()
{
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "MS MULTIPLICATION TESTS: Magnitude-Sign operator*=" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::cout << "\n[1-4] Sign Rules:" << std::endl;
    TEST_CASE("pos_x_pos",  test_sign_pos_pos);
    TEST_CASE("pos_x_neg",  test_sign_pos_neg);
    TEST_CASE("neg_x_pos",  test_sign_neg_pos);
    TEST_CASE("neg_x_neg",  test_sign_neg_neg);

    std::cout << "\n[5-8] Zero and Identity:" << std::endl;
    TEST_CASE("zero_rhs",        test_zero_rhs);
    TEST_CASE("zero_lhs",        test_zero_lhs);
    TEST_CASE("identity_rhs",    test_identity_rhs);
    TEST_CASE("identity_neg",    test_identity_neg_rhs);

    std::cout << "\n[9-11] Carry from Low Product into High Word:" << std::endl;
    TEST_CASE("carry_low_product",      test_carry_low_product);
    TEST_CASE("carry_maxlow_x_2",       test_carry_maxlow_times_two);
    TEST_CASE("carry_neg_neg_sign",     test_carry_neg_neg_sign);

    std::cout << "\n[12-15] operator* and constexpr:" << std::endl;
    TEST_CASE("operator_star_value",    test_operator_star_value);
    TEST_CASE("operator_star_sign",     test_operator_star_sign);
    TEST_CASE("constexpr_mul",          test_constexpr_mul);
    TEST_CASE("constexpr_mul_sign",     test_constexpr_mul_sign);

    std::cout << "\n[16-17] Chained Multiplication:" << std::endl;
    TEST_CASE("chained_mul",            test_chained_mul);
    TEST_CASE("chained_neg_neg_neg",    test_chained_neg_neg_neg);

    std::cout << std::string(70, '=') << std::endl;
    std::cout << "RESULTS: " << pass_count << "/" << test_count << " PASSED" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
