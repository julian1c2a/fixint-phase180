// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test/Demo: Phase 5 - Increment, Decrement, Unary, incr(), decr()
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================

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

#define TEST_CASE(name, func)                                                    \
    do                                                                           \
    {                                                                            \
        test_count++;                                                            \
        try                                                                      \
        {                                                                        \
            func();                                                              \
            pass_count++;                                                        \
            std::cout << "  [OK] " << name << std::endl;                         \
        }                                                                        \
        catch (const std::exception &e)                                          \
        {                                                                        \
            std::cout << "  [ERROR] " << name << " - " << e.what() << std::endl; \
        }                                                                        \
    } while (false)

#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_TRUE(a) assert((a) == true)
#define ASSERT_FALSE(a) assert((a) == false)

// ============================================================================
// GROUP 1: Pre-increment (++x) - TC and unsigned
// ============================================================================

void test_preincrement_unsigned_basic()
{
    uint128_t x{42ULL};
    const auto &ref = ++x;
    ASSERT_EQ(x.low(), 43ULL);
    ASSERT_EQ(&ref, &x); // returns reference to self
}

void test_preincrement_tc_basic()
{
    int128_t x{99LL};
    ++x;
    ASSERT_EQ(x.low(), 100ULL);
}

void test_preincrement_unsigned_carry()
{
    // Low word overflow: 0xFFFFFFFFFFFFFFFF + 1 should carry to high
    uint128_t x{0xFFFFFFFFFFFFFFFFULL};
    ++x;
    ASSERT_EQ(x.low(), 0ULL);
    ASSERT_EQ(x.high(), 1ULL);
}

void test_preincrement_tc_negative()
{
    // -1 in TC: all bits set. ++(-1) == 0
    int128_t x{-1LL};
    ++x;
    ASSERT_TRUE(x.is_zero());
}

void test_preincrement_unsigned_zero()
{
    uint128_t x{0ULL};
    ++x;
    ASSERT_EQ(x.low(), 1ULL);
    ASSERT_EQ(x.high(), 0ULL);
}

// ============================================================================
// GROUP 2: Post-increment (x++) - TC and unsigned
// ============================================================================

void test_postincrement_unsigned_basic()
{
    uint128_t x{42ULL};
    const uint128_t old = x++;
    ASSERT_EQ(old.low(), 42ULL);
    ASSERT_EQ(x.low(), 43ULL);
}

void test_postincrement_tc_basic()
{
    int128_t x{99LL};
    const int128_t old = x++;
    ASSERT_EQ(old.low(), 99ULL);
    ASSERT_EQ(x.low(), 100ULL);
}

void test_postincrement_returns_old()
{
    uint128_t x{0xFFFFFFFFFFFFFFFFULL};
    const uint128_t old = x++;
    ASSERT_EQ(old.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(old.high(), 0ULL);
    ASSERT_EQ(x.low(), 0ULL);
    ASSERT_EQ(x.high(), 1ULL);
}

// ============================================================================
// GROUP 3: Pre-decrement (--x) - TC and unsigned
// ============================================================================

void test_predecrement_unsigned_basic()
{
    uint128_t x{42ULL};
    const auto &ref = --x;
    ASSERT_EQ(x.low(), 41ULL);
    ASSERT_EQ(&ref, &x);
}

void test_predecrement_tc_basic()
{
    int128_t x{100LL};
    --x;
    ASSERT_EQ(x.low(), 99ULL);
}

void test_predecrement_unsigned_borrow()
{
    // low=0, high=1 => --x should give low=MAX, high=0
    uint128_t x{0ULL};
    x.set_high(1ULL);
    --x;
    ASSERT_EQ(x.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(x.high(), 0ULL);
}

void test_predecrement_tc_zero_to_minus1()
{
    int128_t x{0LL};
    --x;
    // -1 in TC: all bits set
    ASSERT_EQ(x.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(x.high(), 0xFFFFFFFFFFFFFFFFULL);
}

void test_predecrement_tc_one_to_zero()
{
    int128_t x{1LL};
    --x;
    ASSERT_TRUE(x.is_zero());
}

// ============================================================================
// GROUP 4: Post-decrement (x--) - TC and unsigned
// ============================================================================

void test_postdecrement_unsigned_basic()
{
    uint128_t x{42ULL};
    const uint128_t old = x--;
    ASSERT_EQ(old.low(), 42ULL);
    ASSERT_EQ(x.low(), 41ULL);
}

void test_postdecrement_tc_basic()
{
    int128_t x{100LL};
    const int128_t old = x--;
    ASSERT_EQ(old.low(), 100ULL);
    ASSERT_EQ(x.low(), 99ULL);
}

void test_postdecrement_returns_old()
{
    uint128_t x{0ULL};
    x.set_high(1ULL);
    const uint128_t old = x--;
    ASSERT_EQ(old.low(), 0ULL);
    ASSERT_EQ(old.high(), 1ULL);
    ASSERT_EQ(x.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(x.high(), 0ULL);
}

// ============================================================================
// GROUP 5: MS signed increment/decrement
// ============================================================================

void test_preincrement_ms_positive()
{
    int128_ms_t x{5LL};
    ++x;
    // MS: positive + 1 = increment magnitude
    ASSERT_EQ(x.low(), 6ULL);
    ASSERT_FALSE(x.is_negative());
}

void test_preincrement_ms_negative()
{
    int128_ms_t x{-5LL};
    ++x;
    // MS: negative + 1 = decrement magnitude (toward zero)
    ASSERT_EQ(x.low(), 4ULL);
    ASSERT_TRUE(x.is_negative());
}

void test_preincrement_ms_neg1_to_zero()
{
    int128_ms_t x{-1LL};
    ++x;
    // -1 + 1 = 0 (magnitude becomes 0, sign bit cleared)
    ASSERT_TRUE(x.is_zero());
    ASSERT_FALSE(x.is_negative());
}

void test_predecrement_ms_positive()
{
    int128_ms_t x{5LL};
    --x;
    ASSERT_EQ(x.low(), 4ULL);
    ASSERT_FALSE(x.is_negative());
}

void test_predecrement_ms_zero_to_neg1()
{
    int128_ms_t x{0LL};
    --x;
    // 0 - 1 = -1 (magnitude=1, sign=negative)
    ASSERT_EQ(x.low(), 1ULL);
    ASSERT_TRUE(x.is_negative());
}

void test_predecrement_ms_negative()
{
    int128_ms_t x{-3LL};
    --x;
    // -3 - 1 = -4 (increment magnitude)
    ASSERT_EQ(x.low(), 4ULL);
    ASSERT_TRUE(x.is_negative());
}

// ============================================================================
// GROUP 6: EK increment/decrement
// ============================================================================

void test_preincrement_ek_basic()
{
    int128_ek_t x{5LL};
    const auto before_low = x.low();
    ++x;
    // EK: stored value (x+K) + 1 = (x+1)+K
    ASSERT_EQ(x.low(), before_low + 1);
}

void test_predecrement_ek_basic()
{
    int128_ek_t x{5LL};
    const auto before_low = x.low();
    --x;
    ASSERT_EQ(x.low(), before_low - 1);
}

// ============================================================================
// GROUP 7: incr() - Pure function (by value)
// ============================================================================

void test_incr_unsigned_basic()
{
    const uint128_t x{42ULL};
    const uint128_t y = x.incr();
    ASSERT_EQ(y.low(), 43ULL);
    ASSERT_EQ(x.low(), 42ULL); // original unchanged
}

void test_incr_tc_basic()
{
    const int128_t x{99LL};
    const int128_t y = x.incr();
    ASSERT_EQ(y.low(), 100ULL);
    ASSERT_EQ(x.low(), 99ULL); // original unchanged
}

void test_incr_unsigned_carry()
{
    const uint128_t x{0xFFFFFFFFFFFFFFFFULL};
    const uint128_t y = x.incr();
    ASSERT_EQ(y.low(), 0ULL);
    ASSERT_EQ(y.high(), 1ULL);
    ASSERT_EQ(x.low(), 0xFFFFFFFFFFFFFFFFULL); // original unchanged
}

void test_incr_tc_negative()
{
    const int128_t x{-1LL};
    const int128_t y = x.incr();
    ASSERT_TRUE(y.is_zero());
    ASSERT_FALSE(x.is_zero()); // original unchanged
}

void test_incr_ms_positive()
{
    const int128_ms_t x{10LL};
    const int128_ms_t y = x.incr();
    ASSERT_EQ(y.low(), 11ULL);
    ASSERT_EQ(x.low(), 10ULL); // original unchanged
}

void test_incr_ms_neg1_to_zero()
{
    const int128_ms_t x{-1LL};
    const int128_ms_t y = x.incr();
    ASSERT_TRUE(y.is_zero());
    ASSERT_TRUE(x.is_negative()); // original unchanged
}

void test_incr_ek_basic()
{
    const int128_ek_t x{5LL};
    const int128_ek_t y = x.incr();
    ASSERT_EQ(y.low(), x.low() + 1);
}

void test_incr_constexpr()
{
    // Verify constexpr evaluation
    constexpr uint128_t x{100ULL};
    constexpr uint128_t y = x.incr();
    static_assert(y.low() == 101ULL, "incr() must be constexpr");
    ASSERT_EQ(y.low(), 101ULL);
}

// ============================================================================
// GROUP 8: decr() - Pure function (by value)
// ============================================================================

void test_decr_unsigned_basic()
{
    const uint128_t x{42ULL};
    const uint128_t y = x.decr();
    ASSERT_EQ(y.low(), 41ULL);
    ASSERT_EQ(x.low(), 42ULL); // original unchanged
}

void test_decr_tc_basic()
{
    const int128_t x{100LL};
    const int128_t y = x.decr();
    ASSERT_EQ(y.low(), 99ULL);
    ASSERT_EQ(x.low(), 100ULL); // original unchanged
}

void test_decr_unsigned_borrow()
{
    uint128_t x{0ULL};
    x.set_high(1ULL);
    const uint128_t y = x.decr();
    ASSERT_EQ(y.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(y.high(), 0ULL);
    ASSERT_EQ(x.high(), 1ULL); // original unchanged
}

void test_decr_tc_zero_to_minus1()
{
    const int128_t x{0LL};
    const int128_t y = x.decr();
    ASSERT_EQ(y.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(y.high(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_TRUE(x.is_zero()); // original unchanged
}

void test_decr_ms_positive()
{
    const int128_ms_t x{10LL};
    const int128_ms_t y = x.decr();
    ASSERT_EQ(y.low(), 9ULL);
    ASSERT_EQ(x.low(), 10ULL); // original unchanged
}

void test_decr_ms_zero_to_neg1()
{
    const int128_ms_t x{0LL};
    const int128_ms_t y = x.decr();
    ASSERT_EQ(y.low(), 1ULL);
    ASSERT_TRUE(y.is_negative());
    ASSERT_TRUE(x.is_zero()); // original unchanged
}

void test_decr_ek_basic()
{
    const int128_ek_t x{5LL};
    const int128_ek_t y = x.decr();
    ASSERT_EQ(y.low(), x.low() - 1);
}

void test_decr_constexpr()
{
    constexpr uint128_t x{100ULL};
    constexpr uint128_t y = x.decr();
    static_assert(y.low() == 99ULL, "decr() must be constexpr");
    ASSERT_EQ(y.low(), 99ULL);
}

// ============================================================================
// GROUP 9: Unary minus (operator-)
// ============================================================================

void test_unary_minus_tc_positive()
{
    const int128_t x{42LL};
    const int128_t neg = -x;
    ASSERT_TRUE(neg.is_negative());
    // -42 + 42 should be 0
    const int128_t sum = neg + x;
    ASSERT_TRUE(sum.is_zero());
}

void test_unary_minus_tc_negative()
{
    const int128_t x{-42LL};
    const int128_t pos = -x;
    ASSERT_FALSE(pos.is_negative());
    ASSERT_EQ(pos.low(), 42ULL);
}

void test_unary_minus_tc_zero()
{
    const int128_t x{0LL};
    const int128_t neg = -x;
    ASSERT_TRUE(neg.is_zero());
}

void test_unary_minus_unsigned()
{
    // Unsigned negation: wraps around (like builtin unsigned)
    const uint128_t x{1ULL};
    const uint128_t neg = -x;
    // -1 unsigned = all bits set
    ASSERT_EQ(neg.low(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(neg.high(), 0xFFFFFFFFFFFFFFFFULL);
}

void test_unary_minus_ms()
{
    const int128_ms_t x{42LL};
    const int128_ms_t neg = -x;
    ASSERT_TRUE(neg.is_negative());
    ASSERT_EQ(neg.low(), 42ULL); // Same magnitude
    // Double negate returns original
    const int128_ms_t pos = -neg;
    ASSERT_EQ(pos.low(), 42ULL);
    ASSERT_FALSE(pos.is_negative());
}

void test_unary_minus_ek()
{
    const int128_ek_t x{42LL};
    const int128_ek_t neg = -x;
    // For EK: -x = 2*bias - x
    // Verify round-trip: -(-x) == x
    const int128_ek_t back = -neg;
    ASSERT_EQ(back.low(), x.low());
    ASSERT_EQ(back.high(), x.high());
}

// ============================================================================
// GROUP 10: Unary plus (operator+)
// ============================================================================

void test_unary_plus_tc()
{
    const int128_t x{42LL};
    const int128_t y = +x;
    ASSERT_EQ(x, y);
}

void test_unary_plus_unsigned()
{
    const uint128_t x{42ULL};
    const uint128_t y = +x;
    ASSERT_EQ(x, y);
}

// ============================================================================
// GROUP 11: incr/decr chaining and identity properties
// ============================================================================

void test_incr_decr_identity_unsigned()
{
    // x.incr().decr() == x (round-trip)
    const uint128_t x{12345ULL};
    ASSERT_EQ(x.incr().decr(), x);
}

void test_decr_incr_identity_unsigned()
{
    const uint128_t x{12345ULL};
    ASSERT_EQ(x.decr().incr(), x);
}

void test_incr_decr_identity_tc()
{
    const int128_t x{-42LL};
    ASSERT_EQ(x.incr().decr(), x);
}

void test_incr_decr_identity_ms()
{
    const int128_ms_t x{-42LL};
    ASSERT_EQ(x.incr().decr(), x);
}

void test_double_negate_identity_tc()
{
    const int128_t x{42LL};
    ASSERT_EQ(-(-x), x);
}

void test_double_negate_identity_ms()
{
    const int128_ms_t x{42LL};
    ASSERT_EQ(-(-x), x);
}

void test_double_negate_identity_ek()
{
    const int128_ek_t x{42LL};
    ASSERT_EQ(-(-x), x);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main()
{
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "PHASE 5 TESTS: Increment, Decrement, Unary, incr(), decr()" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::cout << "\n[1-5] Pre-increment (++x) - TC & unsigned:" << std::endl;
    TEST_CASE("preincrement_unsigned_basic", test_preincrement_unsigned_basic);
    TEST_CASE("preincrement_tc_basic", test_preincrement_tc_basic);
    TEST_CASE("preincrement_unsigned_carry", test_preincrement_unsigned_carry);
    TEST_CASE("preincrement_tc_negative", test_preincrement_tc_negative);
    TEST_CASE("preincrement_unsigned_zero", test_preincrement_unsigned_zero);

    std::cout << "\n[6-8] Post-increment (x++) - TC & unsigned:" << std::endl;
    TEST_CASE("postincrement_unsigned_basic", test_postincrement_unsigned_basic);
    TEST_CASE("postincrement_tc_basic", test_postincrement_tc_basic);
    TEST_CASE("postincrement_returns_old", test_postincrement_returns_old);

    std::cout << "\n[9-13] Pre-decrement (--x) - TC & unsigned:" << std::endl;
    TEST_CASE("predecrement_unsigned_basic", test_predecrement_unsigned_basic);
    TEST_CASE("predecrement_tc_basic", test_predecrement_tc_basic);
    TEST_CASE("predecrement_unsigned_borrow", test_predecrement_unsigned_borrow);
    TEST_CASE("predecrement_tc_zero_to_minus1", test_predecrement_tc_zero_to_minus1);
    TEST_CASE("predecrement_tc_one_to_zero", test_predecrement_tc_one_to_zero);

    std::cout << "\n[14-16] Post-decrement (x--) - TC & unsigned:" << std::endl;
    TEST_CASE("postdecrement_unsigned_basic", test_postdecrement_unsigned_basic);
    TEST_CASE("postdecrement_tc_basic", test_postdecrement_tc_basic);
    TEST_CASE("postdecrement_returns_old", test_postdecrement_returns_old);

    std::cout << "\n[17-22] MS signed increment/decrement:" << std::endl;
    TEST_CASE("preincrement_ms_positive", test_preincrement_ms_positive);
    TEST_CASE("preincrement_ms_negative", test_preincrement_ms_negative);
    TEST_CASE("preincrement_ms_neg1_to_zero", test_preincrement_ms_neg1_to_zero);
    TEST_CASE("predecrement_ms_positive", test_predecrement_ms_positive);
    TEST_CASE("predecrement_ms_zero_to_neg1", test_predecrement_ms_zero_to_neg1);
    TEST_CASE("predecrement_ms_negative", test_predecrement_ms_negative);

    std::cout << "\n[23-24] EK increment/decrement:" << std::endl;
    TEST_CASE("preincrement_ek_basic", test_preincrement_ek_basic);
    TEST_CASE("predecrement_ek_basic", test_predecrement_ek_basic);

    std::cout << "\n[25-32] incr() - Pure function (by value):" << std::endl;
    TEST_CASE("incr_unsigned_basic", test_incr_unsigned_basic);
    TEST_CASE("incr_tc_basic", test_incr_tc_basic);
    TEST_CASE("incr_unsigned_carry", test_incr_unsigned_carry);
    TEST_CASE("incr_tc_negative", test_incr_tc_negative);
    TEST_CASE("incr_ms_positive", test_incr_ms_positive);
    TEST_CASE("incr_ms_neg1_to_zero", test_incr_ms_neg1_to_zero);
    TEST_CASE("incr_ek_basic", test_incr_ek_basic);
    TEST_CASE("incr_constexpr", test_incr_constexpr);

    std::cout << "\n[33-40] decr() - Pure function (by value):" << std::endl;
    TEST_CASE("decr_unsigned_basic", test_decr_unsigned_basic);
    TEST_CASE("decr_tc_basic", test_decr_tc_basic);
    TEST_CASE("decr_unsigned_borrow", test_decr_unsigned_borrow);
    TEST_CASE("decr_tc_zero_to_minus1", test_decr_tc_zero_to_minus1);
    TEST_CASE("decr_ms_positive", test_decr_ms_positive);
    TEST_CASE("decr_ms_zero_to_neg1", test_decr_ms_zero_to_neg1);
    TEST_CASE("decr_ek_basic", test_decr_ek_basic);
    TEST_CASE("decr_constexpr", test_decr_constexpr);

    std::cout << "\n[41-46] Unary minus (operator-):" << std::endl;
    TEST_CASE("unary_minus_tc_positive", test_unary_minus_tc_positive);
    TEST_CASE("unary_minus_tc_negative", test_unary_minus_tc_negative);
    TEST_CASE("unary_minus_tc_zero", test_unary_minus_tc_zero);
    TEST_CASE("unary_minus_unsigned", test_unary_minus_unsigned);
    TEST_CASE("unary_minus_ms", test_unary_minus_ms);
    TEST_CASE("unary_minus_ek", test_unary_minus_ek);

    std::cout << "\n[47-48] Unary plus (operator+):" << std::endl;
    TEST_CASE("unary_plus_tc", test_unary_plus_tc);
    TEST_CASE("unary_plus_unsigned", test_unary_plus_unsigned);

    std::cout << "\n[49-55] Identity properties (incr/decr/negate):" << std::endl;
    TEST_CASE("incr_decr_identity_unsigned", test_incr_decr_identity_unsigned);
    TEST_CASE("decr_incr_identity_unsigned", test_decr_incr_identity_unsigned);
    TEST_CASE("incr_decr_identity_tc", test_incr_decr_identity_tc);
    TEST_CASE("incr_decr_identity_ms", test_incr_decr_identity_ms);
    TEST_CASE("double_negate_identity_tc", test_double_negate_identity_tc);
    TEST_CASE("double_negate_identity_ms", test_double_negate_identity_ms);
    TEST_CASE("double_negate_identity_ek", test_double_negate_identity_ek);

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "RESULTS: " << pass_count << "/" << test_count << " PASSED" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
