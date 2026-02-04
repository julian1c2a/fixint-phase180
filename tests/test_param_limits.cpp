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

        if (Limits::is_specialized && !Limits::is_signed && Limits::is_integer &&
            Limits::is_exact && Limits::is_bounded && Limits::is_modulo &&
            Limits::digits == 128 && Limits::digits10 == 38 && Limits::radix == 2)
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

        if ((Limits::epsilon() == uint128_t{0, 0}) &&
            (Limits::infinity() == uint128_t{0, 0}) &&
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

        if (Limits::is_specialized && Limits::is_signed && Limits::is_integer &&
            Limits::is_exact && Limits::is_bounded && !Limits::is_modulo &&
            Limits::digits == 127 && Limits::digits10 == 38)
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
        if ((min_val == int128_tc_t{1ULL << 63, 0}) &&
            (max_val == int128_tc_t{(1ULL << 63) - 1, ~0ULL}) &&
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

        if ((Limits::epsilon() == int128_tc_t{0, 0}) &&
            (Limits::infinity() == int128_tc_t{0, 0}) &&
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

        if (Limits::is_specialized && Limits::is_signed && Limits::is_integer &&
            Limits::is_exact && Limits::is_bounded && !Limits::is_modulo &&
            Limits::digits == 127 && Limits::digits10 == 38)
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

        if ((min_val == expected_min) && (max_val == expected_max) &&
            (Limits::lowest() == min_val))
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

        if ((Limits::epsilon() == int128_ms_t{0, 0}) &&
            (Limits::infinity() == int128_ms_t{0, 0}) &&
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

        if (Limits::is_specialized && Limits::is_signed && Limits::is_integer &&
            Limits::is_exact && Limits::is_bounded && !Limits::is_modulo &&
            Limits::digits == 127 && Limits::digits10 == 38)
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
        if ((min_val == int128_ek_t{0, 0}) &&
            (max_val == int128_ek_t{(1ULL << 63) - 1, ~0ULL}) &&
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

        if ((Limits::epsilon() == int128_ek_t{0, 0}) &&
            (Limits::infinity() == int128_ek_t{0, 0}) &&
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
