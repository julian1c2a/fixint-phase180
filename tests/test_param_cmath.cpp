// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_cmath.hpp - Mathematical Functions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_cmath.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

// Test result macros
#define TEST_PASS() (++g_passed)
#define TEST_FAIL() (++g_failed)

int g_passed{0};
int g_failed{0};

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Mathematical Functions Tests (abs, gcd, lcm, pow, etc.)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] abs() - Absolute value
    // ========================================================================
    {
        std::cout << "[Test 1] abs():\n";

        // TC: Standard negation
        const int128_tc_t tc_neg{-42};
        const auto tc_abs_result{abs(tc_neg)};
        const int128_tc_t tc_expected{42};

        // MS: Clear sign bit
        int128_ms_t ms_neg{1ULL << 63, 42}; // Negative with magnitude 42
        const auto ms_abs_result{abs(ms_neg)};

        if ((tc_abs_result == tc_expected) && !ms_abs_result.is_negative())
        {
            std::cout << "  [OK] abs\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] abs\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 2] min() / max() - Minimum and maximum
    // ========================================================================
    {
        std::cout << "[Test 2] min() / max():\n";

        const int128_tc_t tc_a{0, 100};
        const int128_tc_t tc_b{0, 200};

        const int128_ms_t ms_a{0, 50};
        const int128_ms_t ms_b{0, 75};

        if ((min(tc_a, tc_b) == tc_a) &&
            (max(tc_a, tc_b) == tc_b) &&
            (min(ms_a, ms_b) == ms_a) &&
            (max(ms_a, ms_b) == ms_b))
        {
            std::cout << "  [OK] min_max\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] min_max\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 3] clamp() - Clamp to range
    // ========================================================================
    {
        std::cout << "[Test 3] clamp():\n";

        const uint128_t lo{0, 100};
        const uint128_t hi{0, 200};

        const uint128_t val_in{0, 150};   // Within range
        const uint128_t val_low{0, 50};   // Below range
        const uint128_t val_high{0, 250}; // Above range

        if ((clamp(val_in, lo, hi) == val_in) &&
            (clamp(val_low, lo, hi) == lo) &&
            (clamp(val_high, lo, hi) == hi))
        {
            std::cout << "  [OK] clamp\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] clamp\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 4] gcd() - Greatest Common Divisor
    // ========================================================================
    {
        std::cout << "[Test 4] gcd():\n";

        const uint128_t a1{0, 48};
        const uint128_t b1{0, 18};
        const uint128_t expected1{0, 6};

        const uint128_t a2{0, 100};
        const uint128_t b2{0, 50};
        const uint128_t expected2{0, 50};

        const uint128_t a3{0, 17};
        const uint128_t b3{0, 19};
        const uint128_t expected3{0, 1}; // Coprime

        const uint128_t a4{0, 0};
        const uint128_t b4{0, 42};
        const uint128_t expected4{0, 42}; // gcd(0, n) = n

        if ((gcd(a1, b1) == expected1) &&
            (gcd(a2, b2) == expected2) &&
            (gcd(a3, b3) == expected3) &&
            (gcd(a4, b4) == expected4))
        {
            std::cout << "  [OK] gcd\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] gcd\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 5] lcm() - Least Common Multiple
    // ========================================================================
    {
        std::cout << "[Test 5] lcm():\n";

        const uint128_t a1{0, 4};
        const uint128_t b1{0, 6};
        const uint128_t expected1{0, 12};

        const uint128_t a2{0, 5};
        const uint128_t b2{0, 7};
        const uint128_t expected2{0, 35};

        const uint128_t a3{0, 0};
        const uint128_t b3{0, 42};
        const uint128_t expected3{0, 0}; // lcm(0, n) = 0

        if ((lcm(a1, b1) == expected1) &&
            (lcm(a2, b2) == expected2) &&
            (lcm(a3, b3) == expected3))
        {
            std::cout << "  [OK] lcm\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] lcm\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 6] midpoint() - Overflow-safe midpoint
    // ========================================================================
    {
        std::cout << "[Test 6] midpoint():\n";

        const uint128_t a1{0, 100};
        const uint128_t b1{0, 200};
        const uint128_t expected1{0, 150};

        const uint128_t a2{0, 0};
        const uint128_t b2{0, 10};
        const uint128_t expected2{0, 5};

        const uint128_t a3{0, 42};
        const uint128_t b3{0, 42};
        const uint128_t expected3{0, 42};

        if ((midpoint(a1, b1) == expected1) &&
            (midpoint(a2, b2) == expected2) &&
            (midpoint(a3, b3) == expected3))
        {
            std::cout << "  [OK] midpoint\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] midpoint\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 7] pow() - Integer exponentiation
    // ========================================================================
    {
        std::cout << "[Test 7] pow():\n";

        const uint128_t base1{0, 2};
        const auto result1{pow(base1, 10)};
        const uint128_t expected1{0, 1024};

        const uint128_t base2{0, 3};
        const auto result2{pow(base2, 4)};
        const uint128_t expected2{0, 81};

        const uint128_t base3{0, 999};
        const auto result3{pow(base3, 0)};
        const uint128_t expected3{0, 1}; // x^0 = 1

        const uint128_t base4{0, 42};
        const auto result4{pow(base4, 1)};
        const uint128_t expected4{0, 42}; // x^1 = x

        const uint128_t base5{0, 10};
        const auto result5{pow(base5, 3)};
        const uint128_t expected5{0, 1000};

        if ((result1 == expected1) &&
            (result2 == expected2) &&
            (result3 == expected3) &&
            (result4 == expected4) &&
            (result5 == expected5))
        {
            std::cout << "  [OK] pow\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] pow\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 8] Mixed-type operations (int128 + builtin int)
    // ========================================================================
    {
        std::cout << "[Test 8] Mixed-type operations:\n";

        const uint128_t big{0, 100};
        const auto gcd_result{gcd(big, 50)};
        const auto lcm_result{lcm(big, 4)};

        if ((gcd_result == uint128_t{0, 50}) &&
            (lcm_result == uint128_t{0, 100}))
        {
            std::cout << "  [OK] mixed_type_ops\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] mixed_type_ops\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // RESULTS
    // ========================================================================
    std::cout << "====================================================================\n";
    std::cout << "RESULTS:\n";
    std::cout << "  Passed: " << g_passed << "\n";
    std::cout << "  Failed: " << g_failed << "\n";
    std::cout << "  Total:  " << (g_passed + g_failed) << "\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? 0 : 1;
}
