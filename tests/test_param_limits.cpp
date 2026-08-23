// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_limits.hpp - Numeric Limits (4 valid types)
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_limits.hpp"
#include <iostream>
#include <limits>
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
    std::cout << "Numeric Limits Tests (4 valid representation forms)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Group 1] BINNAT (unsigned) - 3 tests
    // ========================================================================
    std::cout << "[Group 1] BINNAT (unsigned binary natural):\n";

    // Test 1.1: binnat traits
    {
        using Limits = std::numeric_limits<uint128_t>;

        if (Limits::is_specialized && !Limits::is_signed && Limits::is_integer && Limits::is_exact &&
            Limits::is_bounded && Limits::is_modulo && Limits::digits == 128 && Limits::digits10 == 38 &&
            Limits::radix == 2)
        {
            std::cout << "  [OK] binnat_traits\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] binnat_traits\n";
            TEST_FAIL();
        }
    }

    // Test 1.2: binnat min/max
    {
        using Limits = std::numeric_limits<uint128_t>;

        const auto min_val{Limits::min()};
        const auto max_val{Limits::max()};

        if ((min_val == uint128_t{0, 0}) && (max_val == uint128_t{~0ULL, ~0ULL}) &&
            (Limits::lowest() == min_val))
        {
            std::cout << "  [OK] binnat_min_max\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] binnat_min_max\n";
            TEST_FAIL();
        }
    }

    // Test 1.3: binnat special values (all zero for integers)
    {
        using Limits = std::numeric_limits<uint128_t>;

        if ((Limits::epsilon() == uint128_t{0, 0}) && (Limits::infinity() == uint128_t{0, 0}) &&
            (Limits::quiet_NaN() == uint128_t{0, 0}))
        {
            std::cout << "  [OK] binnat_special_values\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] binnat_special_values\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 2] TWO'S COMPLEMENT (signed) - 3 tests
    // ========================================================================
    std::cout << "[Group 2] Two's Complement (signed):\n";

    // Test 2.1: TC traits
    {
        using Limits = std::numeric_limits<int128_tc_t>;

        if (Limits::is_specialized && Limits::is_signed && Limits::is_integer && Limits::is_exact &&
            Limits::is_bounded && !Limits::is_modulo && Limits::digits == 127 && Limits::digits10 == 38)
        {
            std::cout << "  [OK] tc_traits\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] tc_traits\n";
            TEST_FAIL();
        }
    }

    // Test 2.2: TC min/max
    {
        using Limits = std::numeric_limits<int128_tc_t>;

        const auto min_val{Limits::min()};
        const auto max_val{Limits::max()};

        // TC: min = -2^127 = 0x8000...0000, max = 2^127-1 = 0x7FFF...FFFF
        if ((min_val == int128_tc_t{1ULL << 63, 0}) && (max_val == int128_tc_t{(1ULL << 63) - 1, ~0ULL}) &&
            (Limits::lowest() == min_val))
        {
            std::cout << "  [OK] tc_min_max\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] tc_min_max\n";
            TEST_FAIL();
        }
    }

    // Test 2.3: TC special values
    {
        using Limits = std::numeric_limits<int128_tc_t>;

        if ((Limits::epsilon() == int128_tc_t{0, 0}) && (Limits::infinity() == int128_tc_t{0, 0}) &&
            (Limits::quiet_NaN() == int128_tc_t{0, 0}))
        {
            std::cout << "  [OK] tc_special_values\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] tc_special_values\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 3] MAGNITUDE-SIGN (signed) - 3 tests
    // ========================================================================
    std::cout << "[Group 3] Magnitude-Sign (signed):\n";

    // Test 3.1: MS traits
    {
        using Limits = std::numeric_limits<int128_ms_t>;

        if (Limits::is_specialized && Limits::is_signed && Limits::is_integer && Limits::is_exact &&
            Limits::is_bounded && !Limits::is_modulo && Limits::digits == 127 && Limits::digits10 == 38)
        {
            std::cout << "  [OK] ms_traits\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ms_traits\n";
            TEST_FAIL();
        }
    }

    // Test 3.2: MS min/max
    {
        using Limits = std::numeric_limits<int128_ms_t>;

        const auto min_val{Limits::min()};
        const auto max_val{Limits::max()};

        // MS: min = -(2^127-1) with sign bit set
        // MS: max = +(2^127-1) with sign bit clear
        const int128_ms_t expected_min{(1ULL << 63) | ((1ULL << 63) - 1), ~0ULL};
        const int128_ms_t expected_max{(1ULL << 63) - 1, ~0ULL};

        if ((min_val == expected_min) && (max_val == expected_max) && (Limits::lowest() == min_val))
        {
            std::cout << "  [OK] ms_min_max\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ms_min_max\n";
            TEST_FAIL();
        }
    }

    // Test 3.3: MS special values
    {
        using Limits = std::numeric_limits<int128_ms_t>;

        if ((Limits::epsilon() == int128_ms_t{0, 0}) && (Limits::infinity() == int128_ms_t{0, 0}) &&
            (Limits::quiet_NaN() == int128_ms_t{0, 0}))
        {
            std::cout << "  [OK] ms_special_values\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ms_special_values\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 4] EXCESS-K (signed) - 3 tests
    // ========================================================================
    std::cout << "[Group 4] Excess-K (signed):\n";

    // Test 4.1: EK traits
    {
        using Limits = std::numeric_limits<int128_ek_t>;

        if (Limits::is_specialized && Limits::is_signed && Limits::is_integer && Limits::is_exact &&
            Limits::is_bounded && !Limits::is_modulo && Limits::digits == 126 && Limits::digits10 == 37)
        {
            std::cout << "  [OK] ek_traits\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ek_traits\n";
            TEST_FAIL();
        }
    }

    // Test 4.2: EK min/max
    {
        using Limits = std::numeric_limits<int128_ek_t>;

        const auto min_val{Limits::min()};
        const auto max_val{Limits::max()};

        // EK: min = -2^126 (stored as 0), max = 2^126-1 (stored as 2^127-1)
        if ((min_val == int128_ek_t{0, 0}) && (max_val == int128_ek_t{(1ULL << 63) - 1, ~0ULL}) &&
            (Limits::lowest() == min_val))
        {
            std::cout << "  [OK] ek_min_max\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ek_min_max\n";
            TEST_FAIL();
        }
    }

    // Test 4.3: EK special values
    {
        using Limits = std::numeric_limits<int128_ek_t>;

        if ((Limits::epsilon() == int128_ek_t{0, 0}) && (Limits::infinity() == int128_ek_t{0, 0}) &&
            (Limits::quiet_NaN() == int128_ek_t{0, 0}))
        {
            std::cout << "  [OK] ek_special_values\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ek_special_values\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 5] Compile-time static_assert verification - 4 tests
    // ========================================================================
    std::cout << "[Group 5] Compile-time static_assert:\n";

    // Test 5.1: binnat constexpr
    {
        static_assert(std::numeric_limits<uint128_t>::is_specialized);
        static_assert(!std::numeric_limits<uint128_t>::is_signed);
        static_assert(std::numeric_limits<uint128_t>::is_integer);
        static_assert(std::numeric_limits<uint128_t>::digits == 128);
        static_assert(std::numeric_limits<uint128_t>::is_modulo);
        std::cout << "  [OK] binnat_constexpr_traits\n";
        TEST_PASS();
    }

    // Test 5.2: TC constexpr
    {
        static_assert(std::numeric_limits<int128_tc_t>::is_specialized);
        static_assert(std::numeric_limits<int128_tc_t>::is_signed);
        static_assert(std::numeric_limits<int128_tc_t>::digits == 127);
        static_assert(!std::numeric_limits<int128_tc_t>::is_modulo);
        std::cout << "  [OK] tc_constexpr_traits\n";
        TEST_PASS();
    }

    // Test 5.3: MS constexpr
    {
        static_assert(std::numeric_limits<int128_ms_t>::is_specialized);
        static_assert(std::numeric_limits<int128_ms_t>::is_signed);
        static_assert(std::numeric_limits<int128_ms_t>::digits == 127);
        static_assert(!std::numeric_limits<int128_ms_t>::is_modulo);
        std::cout << "  [OK] ms_constexpr_traits\n";
        TEST_PASS();
    }

    // Test 5.4: EK constexpr
    {
        static_assert(std::numeric_limits<int128_ek_t>::is_specialized);
        static_assert(std::numeric_limits<int128_ek_t>::is_signed);
        static_assert(!std::numeric_limits<int128_ek_t>::is_modulo);
        std::cout << "  [OK] ek_constexpr_traits\n";
        TEST_PASS();
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 6] Semantic min/max ordering - 5 tests
    // ========================================================================
    std::cout << "[Group 6] Semantic min/max ordering:\n";

    // Test 6.1: binnat ordering (min == 0, max is maximum)
    {
        const auto bn_min{std::numeric_limits<uint128_t>::min()};
        const auto bn_max{std::numeric_limits<uint128_t>::max()};
        if ((bn_min == uint128_t{0}) && (bn_min < bn_max) &&
            (std::numeric_limits<uint128_t>::lowest() == bn_min))
        {
            std::cout << "  [OK] binnat_ordering\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] binnat_ordering\n";
            TEST_FAIL();
        }
    }

    // Test 6.2: TC ordering (min < 0 < max)
    {
        const auto tc_min{std::numeric_limits<int128_tc_t>::min()};
        const auto tc_max{std::numeric_limits<int128_tc_t>::max()};
        const int128_tc_t zero{0};
        if ((tc_min < zero) && (zero < tc_max) && (tc_min < tc_max))
        {
            std::cout << "  [OK] tc_ordering\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] tc_ordering\n";
            TEST_FAIL();
        }
    }

    // Test 6.3: MS ordering (min < 0 < max)
    {
        const auto ms_min{std::numeric_limits<int128_ms_t>::min()};
        const auto ms_max{std::numeric_limits<int128_ms_t>::max()};
        const int128_ms_t zero{0};
        if ((ms_min < zero) && (zero < ms_max) && (ms_min < ms_max))
        {
            std::cout << "  [OK] ms_ordering\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ms_ordering\n";
            TEST_FAIL();
        }
    }

    // Test 6.4: lowest() == min() for all integer types
    {
        const bool bn_ok{std::numeric_limits<uint128_t>::lowest() == std::numeric_limits<uint128_t>::min()};
        const bool tc_ok{std::numeric_limits<int128_tc_t>::lowest() ==
                         std::numeric_limits<int128_tc_t>::min()};
        const bool ms_ok{std::numeric_limits<int128_ms_t>::lowest() ==
                         std::numeric_limits<int128_ms_t>::min()};
        const bool ek_ok{std::numeric_limits<int128_ek_t>::lowest() ==
                         std::numeric_limits<int128_ek_t>::min()};
        if (bn_ok && tc_ok && ms_ok && ek_ok)
        {
            std::cout << "  [OK] lowest_equals_min_all_types\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] lowest_equals_min_all_types\n";
            TEST_FAIL();
        }
    }

    // Test 6.5: MS symmetric range (|min| == max)
    {
        // MS: min = -(2^127-1), max = +(2^127-1) => symmetric
        const auto ms_min{std::numeric_limits<int128_ms_t>::min()};
        const auto ms_max{std::numeric_limits<int128_ms_t>::max()};
        // Negate min to get |min|, verify it equals max
        const auto neg_min{-ms_min};
        if (neg_min == ms_max)
        {
            std::cout << "  [OK] ms_symmetric_range\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ms_symmetric_range\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 7] Cross-type properties - 4 tests
    // ========================================================================
    std::cout << "[Group 7] Cross-type properties:\n";

    // Test 7.1: unsigned digits > signed digits
    {
        if (std::numeric_limits<uint128_t>::digits > std::numeric_limits<int128_tc_t>::digits)
        {
            std::cout << "  [OK] unsigned_more_digits_than_signed\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] unsigned_more_digits_than_signed\n";
            TEST_FAIL();
        }
    }

    // Test 7.2: all types have same radix (2)
    {
        if (std::numeric_limits<uint128_t>::radix == 2 && std::numeric_limits<int128_tc_t>::radix == 2 &&
            std::numeric_limits<int128_ms_t>::radix == 2 && std::numeric_limits<int128_ek_t>::radix == 2)
        {
            std::cout << "  [OK] all_radix_2\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] all_radix_2\n";
            TEST_FAIL();
        }
    }

    // Test 7.3: all types have digits10 >= 37
    {
        if (std::numeric_limits<uint128_t>::digits10 >= 37 &&
            std::numeric_limits<int128_tc_t>::digits10 >= 37 &&
            std::numeric_limits<int128_ms_t>::digits10 >= 37 &&
            std::numeric_limits<int128_ek_t>::digits10 >= 37)
        {
            std::cout << "  [OK] all_digits10_at_least_37\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] all_digits10_at_least_37\n";
            TEST_FAIL();
        }
    }

    // Test 7.4: TC and MS have same digits and digits10
    {
        if (std::numeric_limits<int128_tc_t>::digits == std::numeric_limits<int128_ms_t>::digits &&
            std::numeric_limits<int128_tc_t>::digits10 == std::numeric_limits<int128_ms_t>::digits10)
        {
            std::cout << "  [OK] tc_ms_same_digits\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] tc_ms_same_digits\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 8] Non-float properties consistency - 3 tests
    // ========================================================================
    std::cout << "[Group 8] Non-float properties:\n";

    // Test 8.1: no infinity/NaN for any type
    {
        const bool ok{!std::numeric_limits<uint128_t>::has_infinity &&
                      !std::numeric_limits<uint128_t>::has_quiet_NaN &&
                      !std::numeric_limits<uint128_t>::has_signaling_NaN &&
                      !std::numeric_limits<int128_tc_t>::has_infinity &&
                      !std::numeric_limits<int128_tc_t>::has_quiet_NaN &&
                      !std::numeric_limits<int128_tc_t>::has_signaling_NaN &&
                      !std::numeric_limits<int128_ms_t>::has_infinity &&
                      !std::numeric_limits<int128_ms_t>::has_quiet_NaN &&
                      !std::numeric_limits<int128_ms_t>::has_signaling_NaN &&
                      !std::numeric_limits<int128_ek_t>::has_infinity &&
                      !std::numeric_limits<int128_ek_t>::has_quiet_NaN &&
                      !std::numeric_limits<int128_ek_t>::has_signaling_NaN};
        if (ok)
        {
            std::cout << "  [OK] no_infinity_nan_all_types\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] no_infinity_nan_all_types\n";
            TEST_FAIL();
        }
    }

    // Test 8.2: no denorm for any type
    {
        const bool ok{std::numeric_limits<uint128_t>::has_denorm == std::denorm_absent &&
                      std::numeric_limits<int128_tc_t>::has_denorm == std::denorm_absent &&
                      std::numeric_limits<int128_ms_t>::has_denorm == std::denorm_absent &&
                      std::numeric_limits<int128_ek_t>::has_denorm == std::denorm_absent};
        if (ok)
        {
            std::cout << "  [OK] denorm_absent_all_types\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] denorm_absent_all_types\n";
            TEST_FAIL();
        }
    }

    // Test 8.3: not IEC559 for any type
    {
        const bool ok{
            !std::numeric_limits<uint128_t>::is_iec559 && !std::numeric_limits<int128_tc_t>::is_iec559 &&
            !std::numeric_limits<int128_ms_t>::is_iec559 && !std::numeric_limits<int128_ek_t>::is_iec559};
        if (ok)
        {
            std::cout << "  [OK] not_iec559_all_types\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] not_iec559_all_types\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 9] Arithmetic near limits - 4 tests
    // ========================================================================
    std::cout << "[Group 9] Arithmetic near limits:\n";

    // Test 9.1: binnat max - 1 < max
    {
        const auto bn_max{std::numeric_limits<uint128_t>::max()};
        const auto bn_max_minus_1{bn_max - uint128_t{1}};
        if (bn_max_minus_1 < bn_max)
        {
            std::cout << "  [OK] binnat_max_minus_1\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] binnat_max_minus_1\n";
            TEST_FAIL();
        }
    }

    // Test 9.2: binnat max + 1 wraps to 0 (is_modulo)
    {
        const auto bn_max{std::numeric_limits<uint128_t>::max()};
        const auto wrapped{bn_max + uint128_t{1}};
        if (wrapped == uint128_t{0})
        {
            std::cout << "  [OK] binnat_max_wraps\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] binnat_max_wraps\n";
            TEST_FAIL();
        }
    }

    // Test 9.3: TC max - 1 < max
    {
        const auto tc_max{std::numeric_limits<int128_tc_t>::max()};
        const auto tc_max_minus_1{tc_max - int128_tc_t{1}};
        if (tc_max_minus_1 < tc_max)
        {
            std::cout << "  [OK] tc_max_minus_1\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] tc_max_minus_1\n";
            TEST_FAIL();
        }
    }

    // Test 9.4: TC min + 1 > min
    {
        const auto tc_min{std::numeric_limits<int128_tc_t>::min()};
        const auto tc_min_plus_1{tc_min + int128_tc_t{1}};
        if (tc_min < tc_min_plus_1)
        {
            std::cout << "  [OK] tc_min_plus_1\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] tc_min_plus_1\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Group 10] Exponent and max_digits10 (all zero for integers) - 2 tests
    // ========================================================================
    std::cout << "[Group 10] Exponent/decimal properties:\n";

    // Test 10.1: all exponent values zero
    {
        const bool ok{std::numeric_limits<uint128_t>::min_exponent == 0 &&
                      std::numeric_limits<uint128_t>::max_exponent == 0 &&
                      std::numeric_limits<uint128_t>::min_exponent10 == 0 &&
                      std::numeric_limits<uint128_t>::max_exponent10 == 0 &&
                      std::numeric_limits<int128_tc_t>::min_exponent == 0 &&
                      std::numeric_limits<int128_tc_t>::max_exponent == 0 &&
                      std::numeric_limits<int128_ms_t>::min_exponent == 0 &&
                      std::numeric_limits<int128_ms_t>::max_exponent == 0 &&
                      std::numeric_limits<int128_ek_t>::min_exponent == 0 &&
                      std::numeric_limits<int128_ek_t>::max_exponent == 0};
        if (ok)
        {
            std::cout << "  [OK] all_exponents_zero\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] all_exponents_zero\n";
            TEST_FAIL();
        }
    }

    // Test 10.2: max_digits10 == 0 for all integer types
    {
        const bool ok{std::numeric_limits<uint128_t>::max_digits10 == 0 &&
                      std::numeric_limits<int128_tc_t>::max_digits10 == 0 &&
                      std::numeric_limits<int128_ms_t>::max_digits10 == 0 &&
                      std::numeric_limits<int128_ek_t>::max_digits10 == 0};
        if (ok)
        {
            std::cout << "  [OK] max_digits10_zero\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] max_digits10_zero\n";
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
