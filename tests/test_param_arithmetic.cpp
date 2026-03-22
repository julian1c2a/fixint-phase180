// =============================================================================
// Test: Extended arithmetic API (widening_mul, mulhi, mullo)
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_arithmetic.hpp"
#include "test_sweep_framework.hpp"

#include <iostream>
#include <cstdint>

using namespace nstd;
using std::uint64_t;

int main()
{
    std::cout << "=== Extended Arithmetic API Tests ===" << std::endl;
    std::cout << std::endl;

    int total{0};
    int passed{0};

    // =========================================================================
    // Section 1: Known value tests
    // =========================================================================
    std::cout << "--- Section 1: Known Values ---" << std::endl;

    // Test 1: widening_mul identity (a * 1 == a)
    {
        const uint128_t a{0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};
        const uint128_t one{0, 1};
        const auto r{widening_mul(a, one)};
        const bool ok{r.low128() == a && r.high128() == uint128_t{0, 0}};
        std::cout << "[" << (ok ? "OK" : "FAIL") << "] widening_mul(a, 1) == a" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Test 2: widening_mul zero
    {
        const uint128_t a{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
        const uint128_t zero{0, 0};
        const auto r{widening_mul(a, zero)};
        const bool ok{r.low128() == uint128_t{0, 0} && r.high128() == uint128_t{0, 0}};
        std::cout << "[" << (ok ? "OK" : "FAIL") << "] widening_mul(MAX, 0) == 0" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Test 3: widening_mul simple product
    {
        const uint128_t a{0, 100};
        const uint128_t b{0, 200};
        const auto r{widening_mul(a, b)};
        const bool ok{r.low128() == uint128_t{0, 20000} && r.high128() == uint128_t{0, 0}};
        std::cout << "[" << (ok ? "OK" : "FAIL") << "] widening_mul(100, 200) == 20000" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Test 4: widening_mul overflow into high
    {
        const uint128_t a{0, 0xFFFFFFFFFFFFFFFFULL};
        const uint128_t b{0, 0xFFFFFFFFFFFFFFFFULL};
        const auto r{widening_mul(a, b)};
        // (2^64 - 1)^2 = 2^128 - 2^65 + 1
        // low128: high=0xFFFFFFFFFFFFFFFE, low=1
        // high128: 0 (product fits in 128 bits)
        const uint128_t expected_lo{0xFFFFFFFFFFFFFFFEULL, 1ULL};
        const uint128_t expected_hi{0, 0};
        const bool ok{r.low128() == expected_lo && r.high128() == expected_hi};
        std::cout << "[" << (ok ? "OK" : "FAIL") << "] widening_mul(2^64-1, 2^64-1)" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Test 5: mulhi for MAX * MAX
    {
        const uint128_t m{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
        const auto hi{mulhi(m, m)};
        // MAX² = (2^128-1)² = 2^256 - 2^129 + 1
        // Upper 128 bits = (2^256 - 2^129 + 1) >> 128 = 2^128 - 2 = MAX - 1
        const uint128_t expected{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL};
        const bool ok{hi == expected};
        std::cout << "[" << (ok ? "OK" : "FAIL") << "] mulhi(MAX, MAX) == MAX-1" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Test 6: mullo matches operator*
    {
        const uint128_t a{0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL};
        const uint128_t b{0x0BADCAFEDEADBEEFULL, 0x1111222233334444ULL};
        const auto lo{mullo(a, b)};
        const auto op{a * b};
        const bool ok{lo == op};
        std::cout << "[" << (ok ? "OK" : "FAIL") << "] mullo(a, b) == a * b" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Test 7: widening_mul consistency: lo == mullo, hi == mulhi
    {
        const uint128_t a{0xAAAABBBBCCCCDDDDULL, 0x1111222233334444ULL};
        const uint128_t b{0x5555666677778888ULL, 0x9999AAAABBBBCCCCULL};
        const auto full{widening_mul(a, b)};
        const auto hi{mulhi(a, b)};
        const auto lo{mullo(a, b)};
        const bool ok{full.low128() == lo && full.high128() == hi};
        std::cout << "[" << (ok ? "OK" : "FAIL") << "] widening_mul consistency" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 2: Commutativity sweep
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 2: Commutativity Sweep ---" << std::endl;

    {
        const bool ok{sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            {
                return widening_mul(a, b);
            },
            [](const uint128_t &a, const uint128_t &b)
            {
                return widening_mul(b, a);
            },
            "widening_mul_commutative")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 3: low128 matches operator* (sweep)
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 3: Low128 == operator* Sweep ---" << std::endl;

    {
        const bool ok{sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> uint128_t
            {
                return widening_mul(a, b).low128();
            },
            [](const uint128_t &a, const uint128_t &b) -> uint128_t
            {
                return a * b;
            },
            "widening_mul_low128_matches_operator")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 4: widening_mul low128 == operator* (redundant sweep)
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 4: widening_mul Low128 Redundant Check ---" << std::endl;

    {
        const bool ok{sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> uint128_t
            {
                return widening_mul(a, b).low128();
            },
            [](const uint128_t &a, const uint128_t &b) -> uint128_t
            {
                return mullo(a, b);
            },
            "widening_mul_lo_vs_mullo")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 5: mulhi(a, 0) == 0 and mulhi(a, 1) == 0
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 5: mulhi Boundary Values ---" << std::endl;

    {
        const bool ok{sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                return mulhi(a, uint128_t{0, 0});
            },
            [](const uint128_t &) -> uint128_t
            {
                return uint128_t{0, 0};
            },
            "mulhi_zero")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    {
        const bool ok{sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                return mulhi(a, uint128_t{0, 1});
            },
            [](const uint128_t &) -> uint128_t
            {
                return uint128_t{0, 0};
            },
            "mulhi_one")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Summary
    // =========================================================================
    std::cout << std::endl;
    print_sweep_summary(passed, total);

    return (passed == total) ? 0 : 1;
}
