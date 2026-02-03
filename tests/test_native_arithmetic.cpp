// =============================================================================
// Native Arithmetic Tests: EK and MS Representations
// Tests for operator++, operator--, operator+=, operator-=
// =============================================================================
// SPDX-License-Identifier: BSL-1.0

#include <iostream>
#include <cassert>
#include <string>

#include "int128_parameterized.hpp"

using nstd::int128_ek_t;
using nstd::int128_ms_t;
using nstd::int128_t;
using nstd::uint128_t;

// Test counters
int g_tests_passed = 0;
int g_tests_failed = 0;

#define TEST(name, condition)                              \
    do                                                     \
    {                                                      \
        if (condition)                                     \
        {                                                  \
            std::cout << "  [OK] " << name << std::endl;   \
            g_tests_passed++;                              \
        }                                                  \
        else                                               \
        {                                                  \
            std::cout << "  [FAIL] " << name << std::endl; \
            g_tests_failed++;                              \
        }                                                  \
    } while (0)

// ============================================================================
// GROUP 1: Excess-K Increment/Decrement
// ============================================================================

void test_group_1_ek_increment_decrement()
{
    std::cout << "\n[Group 1] Excess-K Increment/Decrement:" << std::endl;

    // Test 1: Pre-increment from zero
    int128_ek_t a{0};
    ++a;
    TEST("ek_preinc_zero", a.to_string() == "1");

    // Test 2: Post-increment
    int128_ek_t b{5};
    int128_ek_t b_old = b++;
    TEST("ek_postinc_old", b_old.to_string() == "5");
    TEST("ek_postinc_new", b.to_string() == "6");

    // Test 3: Pre-decrement
    int128_ek_t c{10};
    --c;
    TEST("ek_predec", c.to_string() == "9");

    // Test 4: Post-decrement
    int128_ek_t d{3};
    int128_ek_t d_old = d--;
    TEST("ek_postdec_old", d_old.to_string() == "3");
    TEST("ek_postdec_new", d.to_string() == "2");

    // Test 5: Increment negative
    int128_ek_t e{-5};
    ++e;
    TEST("ek_inc_negative", e.to_string() == "-4");

    // Test 6: Decrement through zero
    int128_ek_t f{1};
    --f;
    TEST("ek_dec_to_zero", f.to_string() == "0");
    --f;
    TEST("ek_dec_below_zero", f.to_string() == "-1");
}

// ============================================================================
// GROUP 2: Excess-K Addition/Subtraction
// ============================================================================

void test_group_2_ek_addition_subtraction()
{
    std::cout << "\n[Group 2] Excess-K Addition/Subtraction:" << std::endl;

    // Test 1: Simple addition
    int128_ek_t a{10};
    int128_ek_t b{20};
    auto sum = a + b;
    TEST("ek_add_simple", sum.to_string() == "30");

    // Test 2: Addition with assignment
    int128_ek_t c{15};
    c += int128_ek_t{25};
    TEST("ek_add_assign", c.to_string() == "40");

    // Test 3: Subtraction
    int128_ek_t d{50};
    int128_ek_t e{30};
    auto diff = d - e;
    TEST("ek_sub_simple", diff.to_string() == "20");

    // Test 4: Subtraction with assignment
    int128_ek_t f{100};
    f -= int128_ek_t{35};
    TEST("ek_sub_assign", f.to_string() == "65");

    // Test 5: Add negative
    int128_ek_t g{10};
    int128_ek_t h{-5};
    auto sum2 = g + h;
    TEST("ek_add_negative", sum2.to_string() == "5");

    // Test 6: Subtract resulting in negative
    int128_ek_t i{10};
    int128_ek_t j{15};
    auto diff2 = i - j;
    TEST("ek_sub_negative_result", diff2.to_string() == "-5");
}

// ============================================================================
// GROUP 3: Magnitude-Sign Increment/Decrement
// ============================================================================

void test_group_3_ms_increment_decrement()
{
    std::cout << "\n[Group 3] Magnitude-Sign Increment/Decrement:" << std::endl;

    // Test 1: Increment positive
    int128_ms_t a{5};
    ++a;
    TEST("ms_inc_positive", a.to_string() == "6");

    // Test 2: Decrement positive
    int128_ms_t b{10};
    --b;
    TEST("ms_dec_positive", b.to_string() == "9");

    // Test 3: Increment negative (moves toward zero)
    int128_ms_t c{-5};
    ++c;
    TEST("ms_inc_negative", c.to_string() == "-4");

    // Test 4: Decrement negative (moves away from zero)
    int128_ms_t d{-3};
    --d;
    TEST("ms_dec_negative", d.to_string() == "-4");

    // Test 5: Increment negative to zero
    int128_ms_t e{-1};
    ++e;
    TEST("ms_inc_to_zero", e.to_string() == "0");

    // Test 6: Decrement from zero
    int128_ms_t f{0};
    --f;
    TEST("ms_dec_from_zero", f.to_string() == "-1");

    // Test 7: Decrement positive to zero
    int128_ms_t g{1};
    --g;
    TEST("ms_dec_positive_to_zero", g.to_string() == "0");

    // Test 8: Post-increment
    int128_ms_t h{7};
    auto h_old = h++;
    TEST("ms_postinc_old", h_old.to_string() == "7");
    TEST("ms_postinc_new", h.to_string() == "8");
}

// ============================================================================
// GROUP 4: Two's Complement Increment/Decrement (baseline)
// ============================================================================

void test_group_4_tc_increment_decrement()
{
    std::cout << "\n[Group 4] Two's Complement Increment/Decrement:" << std::endl;

    // Test 1: Pre-increment
    int128_t a{100};
    ++a;
    TEST("tc_preinc", a.to_string() == "101");

    // Test 2: Pre-decrement
    int128_t b{50};
    --b;
    TEST("tc_predec", b.to_string() == "49");

    // Test 3: Post-increment
    int128_t c{25};
    auto c_old = c++;
    TEST("tc_postinc_old", c_old.to_string() == "25");
    TEST("tc_postinc_new", c.to_string() == "26");

    // Test 4: Increment negative
    int128_t d{-10};
    ++d;
    TEST("tc_inc_negative", d.to_string() == "-9");

    // Test 5: Decrement through zero
    int128_t e{1};
    --e;
    TEST("tc_dec_to_zero", e.to_string() == "0");
    --e;
    TEST("tc_dec_below_zero", e.to_string() == "-1");
}

// ============================================================================
// GROUP 5: Comparison Operators (all representations)
// ============================================================================

void test_group_5_comparisons()
{
    std::cout << "\n[Group 5] Comparison Operators:" << std::endl;

    // Excess-K comparisons
    int128_ek_t ek1{10};
    int128_ek_t ek2{20};
    int128_ek_t ek3{10};
    TEST("ek_less_than", ek1 < ek2);
    TEST("ek_greater_than", ek2 > ek1);
    TEST("ek_equal", ek1 == ek3);
    TEST("ek_not_equal", ek1 != ek2);
    TEST("ek_less_equal", ek1 <= ek3);
    TEST("ek_greater_equal", ek2 >= ek1);

    // Magnitude-Sign comparisons
    int128_ms_t ms1{-5};
    int128_ms_t ms2{10};
    int128_ms_t ms3{-5};
    TEST("ms_less_than", ms1 < ms2);
    TEST("ms_greater_than", ms2 > ms1);
    TEST("ms_equal", ms1 == ms3);
    TEST("ms_not_equal", ms1 != ms2);

    // Two's Complement comparisons
    int128_t tc1{-100};
    int128_t tc2{50};
    TEST("tc_less_than", tc1 < tc2);
    TEST("tc_greater_than", tc2 > tc1);
}

// ============================================================================
// GROUP 6: Edge Cases and Overflow
// ============================================================================

void test_group_6_edge_cases()
{
    std::cout << "\n[Group 6] Edge Cases:" << std::endl;

    // Test 1: Multiple increments
    int128_ek_t a{0};
    for (int i = 0; i < 100; ++i)
    {
        ++a;
    }
    TEST("ek_multiple_increments", a.to_string() == "100");

    // Test 2: Increment then decrement (should return to original)
    int128_ms_t b{42};
    ++b;
    --b;
    TEST("ms_inc_dec_roundtrip", b.to_string() == "42");

    // Test 3: Decrement then increment
    int128_t c{-7};
    --c;
    ++c;
    TEST("tc_dec_inc_roundtrip", c.to_string() == "-7");

    // Test 4: Chain operations
    int128_ek_t d{10};
    d = d + int128_ek_t{5} - int128_ek_t{3};
    TEST("ek_chain_operations", d.to_string() == "12");
}

// ============================================================================
// MAIN
// ============================================================================

int main()
{
    std::cout << "=====================================================================" << std::endl;
    std::cout << "Native Arithmetic Tests: EK, MS, TC Representations" << std::endl;
    std::cout << "=====================================================================" << std::endl;

    test_group_1_ek_increment_decrement();
    test_group_2_ek_addition_subtraction();
    test_group_3_ms_increment_decrement();
    test_group_4_tc_increment_decrement();
    test_group_5_comparisons();
    test_group_6_edge_cases();

    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "RESULTS:" << std::endl;
    std::cout << "  Passed: " << g_tests_passed << std::endl;
    std::cout << "  Failed: " << g_tests_failed << std::endl;
    std::cout << "  Total:  " << (g_tests_passed + g_tests_failed) << std::endl;
    std::cout << "=====================================================================" << std::endl;

    return (g_tests_failed > 0) ? 1 : 0;
}
