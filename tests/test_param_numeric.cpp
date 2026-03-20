// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_numeric.hpp - Additional Numeric Functions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_numeric.hpp"
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
    std::cout << "Numeric Functions Tests (additional algorithms)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] sign() - Returns -1, 0, or +1
    // ========================================================================
    {
        std::cout << "[Test 1] sign():\n";

        const int128_tc_t tc_pos{0, 42};
        const int128_tc_t tc_neg{-42};
        const int128_tc_t tc_zero{0, 0};

        if ((sign(tc_pos) == 1) && (sign(tc_neg) == -1) && (sign(tc_zero) == 0))
        {
            std::cout << "  [OK] sign_tc\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] sign_tc\n";
            TEST_FAIL();
        }

        const uint128_t unsigned_val{0, 100};
        if (sign(unsigned_val) == 1) // Unsigned always positive
        {
            std::cout << "  [OK] sign_unsigned\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] sign_unsigned\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 2] is_even() / is_odd() - Parity checks
    // ========================================================================
    {
        std::cout << "[Test 2] is_even() / is_odd():\n";

        const uint128_t even_val{0, 42};
        const uint128_t odd_val{0, 43};
        const uint128_t zero{0, 0};

        if (is_even(even_val) && !is_odd(even_val) &&
            !is_even(odd_val) && is_odd(odd_val) &&
            is_even(zero))
        {
            std::cout << "  [OK] parity_checks\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] parity_checks\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 3] abs_diff() - Absolute difference
    // ========================================================================
    {
        std::cout << "[Test 3] abs_diff():\n";

        const uint128_t a{0, 100};
        const uint128_t b{0, 50};
        const uint128_t c{0, 200};

        const auto diff1{abs_diff(a, b)};
        const auto diff2{abs_diff(b, a)};
        const auto diff3{abs_diff(a, c)};

        if ((diff1 == uint128_t{0, 50}) &&
            (diff2 == uint128_t{0, 50}) &&
            (diff3 == uint128_t{0, 100}))
        {
            std::cout << "  [OK] abs_diff\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] abs_diff\n";
            TEST_FAIL();
        }

        const int128_tc_t tc_a{0, 100};
        const int128_tc_t tc_b{-50};
        const auto diff4{abs_diff(tc_a, tc_b)};

        if (diff4 == int128_tc_t{0, 150})
        {
            std::cout << "  [OK] abs_diff_signed\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] abs_diff_signed\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 4] ilog2() - Integer log base 2
    // ========================================================================
    {
        std::cout << "[Test 4] ilog2():\n";

        const uint128_t val1{0, 1};   // 2^0
        const uint128_t val2{0, 8};   // 2^3
        const uint128_t val3{0, 255}; // 2^8 - 1
        const uint128_t val4{1, 0};   // 2^64

        if ((ilog2(val1) == 0) &&
            (ilog2(val2) == 3) &&
            (ilog2(val3) == 7) && // floor(log2(255)) = 7
            (ilog2(val4) == 64))
        {
            std::cout << "  [OK] ilog2\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ilog2\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 5] isqrt() - Integer square root
    // ========================================================================
    {
        std::cout << "[Test 5] isqrt():\n";

        const uint128_t val1{0, 0};   // sqrt(0) = 0
        const uint128_t val2{0, 1};   // sqrt(1) = 1
        const uint128_t val3{0, 4};   // sqrt(4) = 2
        const uint128_t val4{0, 100}; // sqrt(100) = 10
        const uint128_t val5{0, 255}; // sqrt(255) = 15

        if ((isqrt(val1) == uint128_t{0, 0}) &&
            (isqrt(val2) == uint128_t{0, 1}) &&
            (isqrt(val3) == uint128_t{0, 2}) &&
            (isqrt(val4) == uint128_t{0, 10}) &&
            (isqrt(val5) == uint128_t{0, 15}))
        {
            std::cout << "  [OK] isqrt\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] isqrt\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 6] factorial() - Factorial function
    // ========================================================================
    {
        std::cout << "[Test 6] factorial():\n";

        const auto fact0{factorial<signedness::unsigned_type, representation_form::binnat>(0)};
        const auto fact1{factorial<signedness::unsigned_type, representation_form::binnat>(1)};
        const auto fact5{factorial<signedness::unsigned_type, representation_form::binnat>(5)};
        const auto fact10{factorial<signedness::unsigned_type, representation_form::binnat>(10)};

        if ((fact0 == uint128_t{0, 1}) &&      // 0! = 1
            (fact1 == uint128_t{0, 1}) &&      // 1! = 1
            (fact5 == uint128_t{0, 120}) &&    // 5! = 120
            (fact10 == uint128_t{0, 3628800})) // 10! = 3628800
        {
            std::cout << "  [OK] factorial\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] factorial\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 7] divmod() - Combined division and modulo
    // ========================================================================
    {
        std::cout << "[Test 7] divmod():\n";

        const uint128_t dividend{0, 100};
        const uint128_t divisor{0, 7};

        const auto [quot, rem]{divmod(dividend, divisor)};

        if ((quot == uint128_t{0, 14}) && (rem == uint128_t{0, 2}))
        {
            std::cout << "  [OK] divmod_unsigned\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] divmod_unsigned\n";
            TEST_FAIL();
        }

        const int128_tc_t tc_dividend{-100};
        const int128_tc_t tc_divisor{7};
        const auto [tc_quot, tc_rem]{divmod(tc_dividend, tc_divisor)};

        // TC divmod: quotient = -14, remainder = -2
        if ((tc_quot == int128_tc_t{-14}) && (tc_rem == int128_tc_t{-2}))
        {
            std::cout << "  [OK] divmod_signed\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] divmod_signed\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 8] power() - Integer exponentiation
    // ========================================================================
    {
        std::cout << "[Test 8] power():\n";

        const uint128_t base{0, 2};
        const auto pow0{power(base, 0)};
        const auto pow1{power(base, 1)};
        const auto pow8{power(base, 8)};
        const auto pow10{power(base, 10)};

        if ((pow0 == uint128_t{0, 1}) &&   // 2^0 = 1
            (pow1 == uint128_t{0, 2}) &&   // 2^1 = 2
            (pow8 == uint128_t{0, 256}) && // 2^8 = 256
            (pow10 == uint128_t{0, 1024})) // 2^10 = 1024
        {
            std::cout << "  [OK] power\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] power\n";
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
