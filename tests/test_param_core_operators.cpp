// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Core Operators - Arithmetic, Bitwise, Comparison, Unary, Construction
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
// Fills the critical gap: no dedicated tests existed for base operators.
// Tests uint128_t (binnat) and int128_tc_t (twos_complement) systematically.
// =============================================================================

#include "int128_parameterized.hpp"
#include <iostream>
#include <string>
#include <cstdint>

using namespace nstd;

using uint128_t = int128_param_t<signedness::unsigned_type, representation_form::binnat>;
using int128_tc_t = int128_param_t<signedness::signed_type, representation_form::twos_complement>;

int g_passed{0};
int g_failed{0};

#define TEST(name, condition)                           \
    do                                                  \
    {                                                   \
        if (condition)                                  \
        {                                               \
            ++g_passed;                                 \
            std::cout << "  [OK] " << (name) << "\n";   \
        }                                               \
        else                                            \
        {                                               \
            ++g_failed;                                 \
            std::cout << "  [FAIL] " << (name) << "\n"; \
        }                                               \
    } while (false)

// ============================================================================
//  GROUP 1: Construction
// ============================================================================
static void test_construction()
{
    std::cout << "\n[Group 1] Construction:\n";

    // From integer literal
    {
        const uint128_t a{42};
        TEST("uint128_from_int", a.low() == 42 && a.high() == 0);
    }
    // From zero
    {
        const uint128_t z{0};
        TEST("uint128_zero", z.low() == 0 && z.high() == 0);
    }
    // From (high, low) pair
    {
        const uint128_t big{1, 0};
        TEST("uint128_from_high_low", big.high() == 1 && big.low() == 0);
    }
    // Full max value
    {
        const uint128_t maxval{~std::uint64_t{0}, ~std::uint64_t{0}};
        TEST("uint128_max_value", maxval.high() == ~std::uint64_t{0} && maxval.low() == ~std::uint64_t{0});
    }
    // Signed from negative
    {
        const int128_tc_t neg{-1};
        TEST("tc_from_negative", neg.low() == ~std::uint64_t{0} && neg.high() == ~std::uint64_t{0});
    }
    // Signed from positive
    {
        const int128_tc_t pos{100};
        TEST("tc_from_positive", pos.low() == 100 && pos.high() == 0);
    }
    // Copy construction
    {
        const uint128_t orig{12345};
        const uint128_t copy{orig};
        TEST("copy_construction", copy.low() == 12345 && copy.high() == 0);
    }
    // from_string decimal
    {
        const auto val{uint128_t::from_string("100")};
        TEST("from_string_decimal", val.low() == 100 && val.high() == 0);
    }
    // from_string hex
    {
        const auto val{uint128_t::from_string("0xFF")};
        TEST("from_string_hex", val.low() == 255);
    }
    // from_string large
    {
        // 2^64 = 18446744073709551616
        const auto val{uint128_t::from_string("18446744073709551616")};
        TEST("from_string_2_64", val.high() == 1 && val.low() == 0);
    }
}

// ============================================================================
//  GROUP 2: Arithmetic Operators (unsigned)
// ============================================================================
static void test_arithmetic_unsigned()
{
    std::cout << "\n[Group 2] Arithmetic (unsigned):\n";

    // Addition
    {
        const uint128_t a{100};
        const uint128_t b{200};
        const uint128_t c{a + b};
        TEST("u_add_simple", c.low() == 300 && c.high() == 0);
    }
    // Addition with carry
    {
        const uint128_t a{0, ~std::uint64_t{0}}; // low = max64
        const uint128_t b{1};
        const uint128_t c{a + b};
        TEST("u_add_carry", c.high() == 1 && c.low() == 0);
    }
    // Addition: identity
    {
        const uint128_t a{42};
        const uint128_t zero{0};
        TEST("u_add_identity", (a + zero) == a);
    }
    // Subtraction
    {
        const uint128_t a{300};
        const uint128_t b{100};
        const uint128_t c{a - b};
        TEST("u_sub_simple", c.low() == 200 && c.high() == 0);
    }
    // Subtraction with borrow
    {
        const uint128_t a{1, 0}; // 2^64
        const uint128_t b{1};
        const uint128_t c{a - b};
        TEST("u_sub_borrow", c.high() == 0 && c.low() == ~std::uint64_t{0});
    }
    // Multiplication
    {
        const uint128_t a{1000};
        const uint128_t b{1000};
        const uint128_t c{a * b};
        TEST("u_mul_simple", c.low() == 1000000 && c.high() == 0);
    }
    // Multiplication: overflow into high
    {
        const uint128_t a{0, std::uint64_t{1} << 32}; // 2^32
        const uint128_t b{0, std::uint64_t{1} << 32}; // 2^32
        const uint128_t c{a * b};                     // 2^64
        TEST("u_mul_overflow_high", c.high() == 1 && c.low() == 0);
    }
    // Multiplication: by zero
    {
        const uint128_t a{12345};
        const uint128_t zero{0};
        TEST("u_mul_zero", (a * zero) == zero);
    }
    // Multiplication: by one
    {
        const uint128_t a{12345};
        const uint128_t one{1};
        TEST("u_mul_one", (a * one) == a);
    }
    // Division
    {
        const uint128_t a{100};
        const uint128_t b{10};
        const uint128_t c{a / b};
        TEST("u_div_simple", c.low() == 10 && c.high() == 0);
    }
    // Division: large
    {
        const uint128_t a{1, 0}; // 2^64
        const uint128_t b{2};
        const uint128_t c{a / b};
        TEST("u_div_large", c.high() == 0 && c.low() == (std::uint64_t{1} << 63));
    }
    // Division: by self
    {
        const uint128_t a{999999};
        TEST("u_div_self", (a / a) == uint128_t{1});
    }
    // Modulo
    {
        const uint128_t a{17};
        const uint128_t b{5};
        const uint128_t c{a % b};
        TEST("u_mod_simple", c.low() == 2 && c.high() == 0);
    }
    // Modulo: zero remainder
    {
        const uint128_t a{100};
        const uint128_t b{10};
        TEST("u_mod_zero_rem", (a % b) == uint128_t{0});
    }
    // Compound assignment += -= *= /= %=
    {
        uint128_t a{100};
        a += uint128_t{50};
        TEST("u_plus_eq", a == uint128_t{150});
    }
    {
        uint128_t a{100};
        a -= uint128_t{30};
        TEST("u_minus_eq", a == uint128_t{70});
    }
    {
        uint128_t a{10};
        a *= uint128_t{20};
        TEST("u_mul_eq", a == uint128_t{200});
    }
    {
        uint128_t a{100};
        a /= uint128_t{4};
        TEST("u_div_eq", a == uint128_t{25});
    }
    {
        uint128_t a{17};
        a %= uint128_t{5};
        TEST("u_mod_eq", a == uint128_t{2});
    }
    // Mixed: T + uint128_t and uint128_t + T
    {
        const uint128_t a{100};
        const uint128_t c{a + 50};
        TEST("u_add_int_rhs", c == uint128_t{150});
    }
    {
        const uint128_t b{100};
        const uint128_t c{50 + b};
        TEST("u_add_int_lhs", c == uint128_t{150});
    }
}

// ============================================================================
//  GROUP 3: Arithmetic Operators (signed TC)
// ============================================================================
static void test_arithmetic_signed()
{
    std::cout << "\n[Group 3] Arithmetic (signed TC):\n";

    // Addition: pos + pos
    {
        const int128_tc_t a{50};
        const int128_tc_t b{75};
        TEST("tc_add_pos_pos", (a + b) == int128_tc_t{125});
    }
    // Addition: pos + neg
    {
        const int128_tc_t a{100};
        const int128_tc_t b{-30};
        TEST("tc_add_pos_neg", (a + b) == int128_tc_t{70});
    }
    // Addition: neg + neg
    {
        const int128_tc_t a{-50};
        const int128_tc_t b{-30};
        TEST("tc_add_neg_neg", (a + b) == int128_tc_t{-80});
    }
    // Subtraction: crossing zero
    {
        const int128_tc_t a{10};
        const int128_tc_t b{20};
        const int128_tc_t c{a - b};
        TEST("tc_sub_cross_zero", c == int128_tc_t{-10});
    }
    // Multiplication: pos * neg
    {
        const int128_tc_t a{10};
        const int128_tc_t b{-5};
        TEST("tc_mul_pos_neg", (a * b) == int128_tc_t{-50});
    }
    // Multiplication: neg * neg
    {
        const int128_tc_t a{-7};
        const int128_tc_t b{-8};
        TEST("tc_mul_neg_neg", (a * b) == int128_tc_t{56});
    }
    // Division: signed
    {
        const int128_tc_t a{-100};
        const int128_tc_t b{10};
        TEST("tc_div_neg_pos", (a / b) == int128_tc_t{-10});
    }
    // Modulo: signed
    {
        const int128_tc_t a{-17};
        const int128_tc_t b{5};
        // C++ truncated division: -17 % 5 == -2
        TEST("tc_mod_neg", (a % b) == int128_tc_t{-2});
    }
}

// ============================================================================
//  GROUP 4: Increment / Decrement
// ============================================================================
static void test_inc_dec()
{
    std::cout << "\n[Group 4] Increment/Decrement:\n";

    // Pre-increment
    {
        uint128_t a{41};
        ++a;
        TEST("u_pre_inc", a == uint128_t{42});
    }
    // Post-increment
    {
        uint128_t a{41};
        const uint128_t old{a++};
        TEST("u_post_inc_old", old == uint128_t{41});
        TEST("u_post_inc_new", a == uint128_t{42});
    }
    // Pre-decrement
    {
        uint128_t a{43};
        --a;
        TEST("u_pre_dec", a == uint128_t{42});
    }
    // Post-decrement
    {
        uint128_t a{43};
        const uint128_t old{a--};
        TEST("u_post_dec_old", old == uint128_t{43});
        TEST("u_post_dec_new", a == uint128_t{42});
    }
    // Increment with carry (low overflows to high)
    {
        uint128_t a{0, ~std::uint64_t{0}}; // low = MAX
        ++a;
        TEST("u_inc_carry", a.high() == 1 && a.low() == 0);
    }
    // Decrement with borrow (high borrows to low)
    {
        uint128_t a{1, 0}; // high=1, low=0
        --a;
        TEST("u_dec_borrow", a.high() == 0 && a.low() == ~std::uint64_t{0});
    }
    // Signed: ++/-- around zero
    {
        int128_tc_t a{0};
        --a;
        TEST("tc_dec_zero", a == int128_tc_t{-1});
    }
    {
        int128_tc_t a{-1};
        ++a;
        TEST("tc_inc_neg_one", a == int128_tc_t{0});
    }
}

// ============================================================================
//  GROUP 5: Comparison Operators
// ============================================================================
static void test_comparisons()
{
    std::cout << "\n[Group 5] Comparison Operators:\n";

    const uint128_t zero{0};
    const uint128_t one{1};
    const uint128_t max_u{~std::uint64_t{0}, ~std::uint64_t{0}};
    const uint128_t mid{0, 1000};
    const uint128_t high_only{1, 0};

    // Equality
    TEST("u_eq_same", uint128_t{42} == uint128_t{42});
    TEST("u_neq_diff", uint128_t{42} != uint128_t{43});

    // Less/Greater
    TEST("u_lt_true", zero < one);
    TEST("u_lt_false", !(one < zero));
    TEST("u_gt_true", one > zero);
    TEST("u_gt_false", !(zero > one));

    // Less-or-equal / Greater-or-equal
    TEST("u_le_lt", zero <= one);
    TEST("u_le_eq", one <= one);
    TEST("u_ge_gt", one >= zero);
    TEST("u_ge_eq", one >= one);

    // High word matters
    TEST("u_lt_high_word", mid < high_only);
    TEST("u_gt_high_word", high_only > mid);

    // Max comparisons
    TEST("u_lt_max", mid < max_u);
    TEST("u_max_ge_all", max_u >= high_only);

    // Mixed type: uint128 vs int literal
    TEST("u_eq_int", uint128_t{100} == 100);
    TEST("u_lt_int", uint128_t{50} < 100);
    TEST("u_gt_int", 200 > uint128_t{100});

    // Signed comparisons
    std::cout << "\n  -- Signed TC comparisons --\n";
    const int128_tc_t neg{-100};
    const int128_tc_t pos{100};
    const int128_tc_t tc_zero{0};

    TEST("tc_neg_lt_pos", neg < pos);
    TEST("tc_neg_lt_zero", neg < tc_zero);
    TEST("tc_pos_gt_zero", pos > tc_zero);
    TEST("tc_neg_eq", int128_tc_t{-42} == int128_tc_t{-42});
    TEST("tc_neg_ne_pos", neg != pos);
    TEST("tc_le_neg", int128_tc_t{-100} <= int128_tc_t{-100});
    TEST("tc_ge_neg", int128_tc_t{-50} >= int128_tc_t{-100});
}

// ============================================================================
//  GROUP 6: Unary Operators
// ============================================================================
static void test_unary()
{
    std::cout << "\n[Group 6] Unary Operators:\n";

    // Unary plus (identity)
    {
        const uint128_t a{42};
        TEST("u_unary_plus", +a == a);
    }
    // Unary minus (signed)
    {
        const int128_tc_t a{42};
        const int128_tc_t b{-a};
        TEST("tc_unary_neg", b == int128_tc_t{-42});
    }
    // Double negation
    {
        const int128_tc_t a{42};
        TEST("tc_double_neg", -(-a) == a);
    }
    // Negation of zero
    {
        const int128_tc_t z{0};
        TEST("tc_neg_zero", -z == z);
    }
    // is_zero
    {
        TEST("u_is_zero_true", uint128_t{0}.is_zero());
        TEST("u_is_zero_false", !uint128_t{1}.is_zero());
    }
    // is_negative (signed)
    {
        TEST("tc_is_neg_true", int128_tc_t{-1}.is_negative());
        TEST("tc_is_neg_false", !int128_tc_t{1}.is_negative());
        TEST("tc_is_neg_zero", !int128_tc_t{0}.is_negative());
    }
}

// ============================================================================
//  GROUP 7: Bitwise Operators
// ============================================================================
static void test_bitwise()
{
    std::cout << "\n[Group 7] Bitwise Operators:\n";

    const uint128_t all_ones{~std::uint64_t{0}, ~std::uint64_t{0}};
    const uint128_t zero{0};
    const uint128_t a{0, 0xFF00FF00};
    const uint128_t b{0, 0x0FF00FF0};

    // AND
    {
        const uint128_t c{a & b};
        TEST("u_and", c.low() == (0xFF00FF00ull & 0x0FF00FF0ull) && c.high() == 0);
    }
    // AND with all ones = identity
    TEST("u_and_ones", (a & all_ones) == a);
    // AND with zero = zero
    TEST("u_and_zero", (a & zero) == zero);

    // OR
    {
        const uint128_t c{a | b};
        TEST("u_or", c.low() == (0xFF00FF00ull | 0x0FF00FF0ull) && c.high() == 0);
    }
    // OR with zero = identity
    TEST("u_or_zero", (a | zero) == a);

    // XOR
    {
        const uint128_t c{a ^ b};
        TEST("u_xor", c.low() == (0xFF00FF00ull ^ 0x0FF00FF0ull) && c.high() == 0);
    }
    // XOR with self = zero
    TEST("u_xor_self", (a ^ a) == zero);

    // NOT
    {
        const uint128_t c{~zero};
        TEST("u_not_zero", c == all_ones);
    }
    {
        const uint128_t c{~all_ones};
        TEST("u_not_ones", c == zero);
    }

    // Left shift
    {
        const uint128_t x{1};
        TEST("u_shl_1", (x << 1) == uint128_t{2});
        TEST("u_shl_63", (x << 63).low() == (std::uint64_t{1} << 63) && (x << 63).high() == 0);
        TEST("u_shl_64", (x << 64).high() == 1 && (x << 64).low() == 0);
        TEST("u_shl_127", (x << 127).high() == (std::uint64_t{1} << 63) && (x << 127).low() == 0);
    }

    // Right shift
    {
        const uint128_t x{std::uint64_t{1} << 63, 0}; // high = 2^63
        TEST("u_shr_63", (x >> 63).high() == 1 && (x >> 63).low() == 0);
        // 2^127 >> 64 = 2^63 in low
        TEST("u_shr_64", (x >> 64).high() == 0 && (x >> 64).low() == (std::uint64_t{1} << 63));
    }
    // Shift by 0 = identity
    {
        const uint128_t x{0xDEAD, 0xBEEF};
        TEST("u_shl_0", (x << 0) == x);
        TEST("u_shr_0", (x >> 0) == x);
    }

    // Compound bitwise assignments
    {
        uint128_t x{0, 0xFF};
        const uint128_t mask{0, 0x0F};
        x &= mask;
        const uint128_t expected_and{0, 0x0F};
        TEST("u_and_eq", x == expected_and);
    }
    {
        uint128_t x{0, 0xF0};
        const uint128_t mask{0, 0x0F};
        x |= mask;
        const uint128_t expected_or{0, 0xFF};
        TEST("u_or_eq", x == expected_or);
    }
    {
        uint128_t x{0, 0xFF};
        const uint128_t mask{0, 0x0F};
        x ^= mask;
        const uint128_t expected_xor{0, 0xF0};
        TEST("u_xor_eq", x == expected_xor);
    }
    {
        uint128_t x{1};
        x <<= 64;
        TEST("u_shl_eq", x.high() == 1 && x.low() == 0);
    }
    {
        uint128_t x{1, 0};
        x >>= 64;
        TEST("u_shr_eq", x.high() == 0 && x.low() == 1);
    }

    // High word bitwise ops
    {
        const uint128_t h1{0xAAAAAAAAAAAAAAAAull, 0};
        const uint128_t h2{0x5555555555555555ull, 0};
        const uint128_t all_high{~std::uint64_t{0}, 0};
        TEST("u_or_high", (h1 | h2) == all_high);
        TEST("u_and_high", (h1 & h2) == zero);
        TEST("u_xor_high", (h1 ^ h2) == all_high);
    }
}

// ============================================================================
//  GROUP 8: to_string and conversions
// ============================================================================
static void test_conversions()
{
    std::cout << "\n[Group 8] Conversions:\n";

    // to_string decimal
    {
        const uint128_t a{42};
        TEST("u_to_string_42", a.to_string() == "42");
    }
    // to_string zero
    {
        const uint128_t z{0};
        TEST("u_to_string_zero", z.to_string() == "0");
    }
    // to_string large (2^64)
    {
        const uint128_t big{1, 0};
        TEST("u_to_string_2_64", big.to_string() == "18446744073709551616");
    }
    // to_string signed negative
    {
        const int128_tc_t neg{-42};
        TEST("tc_to_string_neg", neg.to_string() == "-42");
    }
    // to_string hex
    {
        const uint128_t a{255};
        TEST("u_to_string_hex", a.to_string(16) == "FF");
    }
    // Round-trip: to_string -> from_string
    {
        const uint128_t original{0xDEADBEEF, 0x12345678ABCD0000ull};
        const auto str{original.to_string()};
        const auto roundtrip{uint128_t::from_string(str.c_str())};
        TEST("u_roundtrip_str", roundtrip == original);
    }
    // high() / low() accessors
    {
        const uint128_t a{0xABCD, 0x1234};
        TEST("u_high_accessor", a.high() == 0xABCD);
        TEST("u_low_accessor", a.low() == 0x1234);
    }
}

// ============================================================================
//  GROUP 9: Edge Cases and Boundary Values
// ============================================================================
static void test_edge_cases()
{
    std::cout << "\n[Group 9] Edge Cases:\n";

    // Max value: max + 1 wraps to 0 (unsigned)
    {
        const uint128_t max_val{~std::uint64_t{0}, ~std::uint64_t{0}};
        const uint128_t wrapped{max_val + uint128_t{1}};
        TEST("u_overflow_wrap", wrapped == uint128_t{0});
    }
    // 0 - 1 wraps to max (unsigned)
    {
        uint128_t z{0};
        --z;
        const uint128_t max_val_expected{~std::uint64_t{0}, ~std::uint64_t{0}};
        TEST("u_underflow_wrap", z == max_val_expected);
    }
    // Shift by 128 or more
    {
        const uint128_t x{1};
        // Behavior for shift >= 128 is implementation-defined, but test it doesn't crash
        const uint128_t s128{x << 128};
        TEST("u_shl_128_no_crash", true); // Just verifying no crash
        (void)s128;
    }
    // Division: exact powers of 2
    {
        const uint128_t big{1, 0};                               // 2^64
        const uint128_t d{uint128_t{0, std::uint64_t{1} << 32}}; // 2^32
        const uint128_t q{big / d};
        const uint128_t expected_q{0, std::uint64_t{1} << 32};
        TEST("u_div_power_2", q == expected_q);
    }
    // Multiplication commutativity
    {
        const uint128_t a{12345};
        const uint128_t b{67890};
        TEST("u_mul_commutative", (a * b) == (b * a));
    }
    // Addition commutativity
    {
        const uint128_t a{0xDEAD, 0xBEEF};
        const uint128_t b{0xCAFE, 0xBABE};
        TEST("u_add_commutative", (a + b) == (b + a));
    }
    // Distributivity: a * (b + c) == a*b + a*c
    {
        const uint128_t a{7};
        const uint128_t b{100};
        const uint128_t c{200};
        TEST("u_distributive", (a * (b + c)) == (a * b + a * c));
    }
    // Signed: min value negation is min (two's complement overflow)
    {
        const int128_tc_t min_val{int128_tc_t::min()};
        const int128_tc_t neg_min{-min_val};
        // In TC: -MIN wraps back to MIN (no positive representation)
        TEST("tc_neg_min_wraps", neg_min == min_val);
    }
    // Division: quotient * divisor + remainder == dividend
    {
        const uint128_t dividend{0x123, 0x456789AB};
        const uint128_t divisor{17};
        const uint128_t quotient{dividend / divisor};
        const uint128_t remainder{dividend % divisor};
        TEST("u_div_mod_identity", (quotient * divisor + remainder) == dividend);
    }
    // Large multiplication
    {
        const uint128_t a{0, 0xFFFFFFFFFFFFFFFFull};
        const uint128_t b{0, 2};
        const uint128_t c{a * b};
        // 0xFFFFFFFFFFFFFFFF * 2 = 0x1FFFFFFFFFFFFFFFE
        TEST("u_mul_large", c.high() == 1 && c.low() == 0xFFFFFFFFFFFFFFFEull);
    }
}

// ============================================================================
//  GROUP 10: Signed arithmetic edge cases
// ============================================================================
static void test_signed_edge_cases()
{
    std::cout << "\n[Group 10] Signed Edge Cases:\n";

    // -1 * -1 = 1
    {
        const int128_tc_t a{-1};
        const int128_tc_t b{-1};
        TEST("tc_neg1_mul_neg1", (a * b) == int128_tc_t{1});
    }
    // Large negative
    {
        const int128_tc_t big_neg{-1000000};
        const int128_tc_t big_pos{1000000};
        TEST("tc_add_cancel", (big_neg + big_pos) == int128_tc_t{0});
    }
    // Division truncation toward zero
    {
        const int128_tc_t a{7};
        const int128_tc_t b{-2};
        // 7 / -2 = -3 (truncated toward zero)
        TEST("tc_div_trunc_neg", (a / b) == int128_tc_t{-3});
    }
    {
        const int128_tc_t a{-7};
        const int128_tc_t b{2};
        // -7 / 2 = -3 (truncated toward zero)
        TEST("tc_div_trunc_pos", (a / b) == int128_tc_t{-3});
    }
    // Modulo sign follows dividend (C++ standard)
    {
        const int128_tc_t a{7};
        const int128_tc_t b{-3};
        // 7 % -3 = 1 (sign follows dividend)
        TEST("tc_mod_sign_pos", (a % b) == int128_tc_t{1});
    }
    {
        const int128_tc_t a{-7};
        const int128_tc_t b{3};
        // -7 % 3 = -1 (sign follows dividend)
        TEST("tc_mod_sign_neg", (a % b) == int128_tc_t{-1});
    }
    // Shift right for signed (arithmetic shift)
    {
        const int128_tc_t neg{-1};
        // Arithmetic right shift of -1 should stay -1
        TEST("tc_shr_neg1", (neg >> 1) == int128_tc_t{-1});
    }
    {
        const int128_tc_t neg{-2};
        // -2 >> 1 = -1 (arithmetic shift)
        TEST("tc_shr_neg2", (neg >> 1) == int128_tc_t{-1});
    }
    {
        const int128_tc_t neg{-4};
        // -4 >> 1 = -2
        TEST("tc_shr_neg4", (neg >> 1) == int128_tc_t{-2});
    }
}

// ============================================================================
//  GROUP 11: Setters & Type Traits
// ============================================================================
static void test_setters_and_type_traits()
{
    std::cout << "\n[Group 11] Setters & Type Traits:\n";

    // Default constructor
    {
        const uint128_t x{};
        TEST("default_ctor_is_zero", x.is_zero() && x.high() == 0 && x.low() == 0);
    }
    // set_high()
    {
        uint128_t x{1, 2};
        x.set_high(0xFF);
        TEST("set_high_preserves_low", x.high() == 0xFF && x.low() == 2);
    }
    // set_low()
    {
        uint128_t x{1, 2};
        x.set_low(0xAA);
        TEST("set_low_preserves_high", x.low() == 0xAA && x.high() == 1);
    }
    // Type traits: sign
    TEST("uint128_sign_unsigned", uint128_t::sign == signedness::unsigned_type);
    TEST("int128_sign_signed", int128_tc_t::sign == signedness::signed_type);
    // Type traits: form
    TEST("uint128_form_binnat", uint128_t::form == representation_form::binnat);
    TEST("int128_form_twos_complement", int128_tc_t::form == representation_form::twos_complement);
    TEST("int128_ek_form_excess_k", int128_ek_t::form == representation_form::excess_k);
    // Type traits: is_signed
    TEST("uint128_not_is_signed", !uint128_t::is_signed);
    TEST("int128_is_signed", int128_tc_t::is_signed);
    TEST("int128_ek_is_signed", int128_ek_t::is_signed);
    // is_excess_k trait
    TEST("int128_ek_is_excess_k", int128_ek_t::is_excess_k);
}

// ============================================================================
//  GROUP 12: Extended Bitwise/Shift (DeMorgan, shift >= 128)
// ============================================================================
static void test_extended_shift_bitwise()
{
    std::cout << "\n[Group 12] Extended Bitwise/Shift:\n";

    // DeMorgan: ~(a & b) == ~a | ~b
    {
        const uint128_t a{0, 0x0F0F0F0FULL};
        const uint128_t b{0, 0xF0F0F0F0ULL};
        TEST("demorgan_and_or", ~(a & b) == (~a | ~b));
        TEST("demorgan_or_and", ~(a | b) == (~a & ~b));
    }
    // Double NOT: ~~x == x
    {
        const uint128_t x{0xDEAD, 0xBEEF};
        TEST("double_not_identity", ~~x == x);
    }
    // Left shift >= 128 yields zero (unsigned)
    {
        const uint128_t x{1};
        TEST("u_shl_128_zero", (x << 128) == uint128_t{0});
        TEST("u_shl_200_zero", (x << 200) == uint128_t{0});
    }
    // Right shift >= 128 yields zero (unsigned)
    {
        const uint128_t x{~0ULL, ~0ULL};
        TEST("u_shr_128_zero", (x >> 128) == uint128_t{0});
    }
    // Right shift >= 128 for signed TC: result is 0 or -1 (sign-fill)
    {
        const int128_tc_t sx{-1};
        const int128_tc_t r = sx >> 128;
        TEST("tc_shr_128_zero_or_neg1", r == int128_tc_t{0} || r == int128_tc_t{-1});
    }
    // Shift by short and int types
    {
        const int128_tc_t x{8};
        TEST("shift_by_short", (x >> static_cast<short>(1)) == int128_tc_t{4});
        TEST("shift_by_int", (x << static_cast<int>(1)) == int128_tc_t{16});
    }
}

// ============================================================================
//  MAIN
// ============================================================================
int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Core Operator Tests (Construction, Arithmetic, Bitwise, Comparison)\n";
    std::cout << "====================================================================\n";

    test_construction();
    test_arithmetic_unsigned();
    test_arithmetic_signed();
    test_inc_dec();
    test_comparisons();
    test_unary();
    test_bitwise();
    test_conversions();
    test_edge_cases();
    test_signed_edge_cases();
    test_setters_and_type_traits();
    test_extended_shift_bitwise();

    std::cout << "\n====================================================================\n";
    std::cout << "Results: Passed: " << g_passed << ", Failed: " << g_failed
              << ", Total: " << (g_passed + g_failed) << "\n";
    std::cout << "====================================================================\n";

    return (g_failed > 0) ? 1 : 0;
}
