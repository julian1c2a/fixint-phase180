// =============================================================================
// Test: Karatsuba sub-quadratic multiplication 128x128->256
// Part of int128 Library
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "algorithms/karatsuba.hpp"
#include "test_sweep_framework.hpp"

#include <iostream>
#include <cstdint>

using namespace nstd;
using namespace nstd::algorithms;

using std::uint64_t;

int main()
{
    std::cout << "=== Karatsuba Multiplication Tests ===" << std::endl;
    std::cout << std::endl;

    int total{0};
    int passed{0};

    // =========================================================================
    // Section 1: Karatsuba vs Schoolbook Agreement
    // =========================================================================
    std::cout << "--- Section 1: Karatsuba vs Schoolbook Agreement ---" << std::endl;

    // Sweep 1: karatsuba matches schoolbook for all binary pairs
    {
        const bool ok{sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            {
                return karatsuba_full_mul(a, b);
            },
            [](const uint128_t &a, const uint128_t &b)
            {
                return schoolbook_full_mul(a, b);
            },
            "karatsuba_matches_schoolbook")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 2: Algebraic Properties
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 2: Algebraic Properties ---" << std::endl;

    // Sweep 2: commutativity
    {
        const bool ok{sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            {
                return karatsuba_full_mul(a, b);
            },
            [](const uint128_t &a, const uint128_t &b)
            {
                return karatsuba_full_mul(b, a);
            },
            "karatsuba_commutativity")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Sweep 3: low 128 bits match truncated multiplication (a * b)
    {
        const bool ok{sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            {
                return karatsuba_full_mul(a, b).low128();
            },
            [](const uint128_t &a, const uint128_t &b)
            {
                return a * b;
            },
            "full_mul_low128_matches_truncated")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 3: Identity and Zero Properties
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 3: Identity and Zero Properties ---" << std::endl;

    // Sweep 4: multiply by zero
    {
        const auto zero{uint128_t{0}};
        const bool ok{sweep_unary(
            [&](const uint128_t &a)
            {
                return karatsuba_full_mul(a, zero).is_zero();
            },
            [](const uint128_t &)
            { return true; },
            "karatsuba_mul_by_zero")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Sweep 5: multiply by one produces identity
    {
        const auto one{uint128_t{1}};
        const auto zero128{uint128_t{0}};
        const bool ok{sweep_unary(
            [&](const uint128_t &a)
            {
                const auto r{karatsuba_full_mul(a, one)};
                return r.low128() == a && r.high128() == zero128;
            },
            [](const uint128_t &)
            { return true; },
            "karatsuba_mul_by_one")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Sweep 6: schoolbook multiply by zero (verify reference too)
    {
        const auto zero{uint128_t{0}};
        const bool ok{sweep_unary(
            [&](const uint128_t &a)
            {
                return schoolbook_full_mul(a, zero).is_zero();
            },
            [](const uint128_t &)
            { return true; },
            "schoolbook_mul_by_zero")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Sweep 7: schoolbook multiply by one
    {
        const auto one{uint128_t{1}};
        const auto zero128{uint128_t{0}};
        const bool ok{sweep_unary(
            [&](const uint128_t &a)
            {
                const auto r{schoolbook_full_mul(a, one)};
                return r.low128() == a && r.high128() == zero128;
            },
            [](const uint128_t &)
            { return true; },
            "schoolbook_mul_by_one")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 4: Known Values
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 4: Known Values ---" << std::endl;

    constexpr uint64_t MAX64{0xFFFFFFFFFFFFFFFFull};

    // Case 1: MAX128 * MAX128 = 2^256 - 2^129 + 1
    // Expected limbs: [1, 0, 0xFFFFFFFFFFFFFFFE, 0xFFFFFFFFFFFFFFFF]
    {
        const auto max128{uint128_t{MAX64, MAX64}};
        const auto k{karatsuba_full_mul(max128, max128)};
        const auto s{schoolbook_full_mul(max128, max128)};

        const bool ok{k == s && k.limbs[0] == 1 && k.limbs[1] == 0 && k.limbs[2] == 0xFFFFFFFFFFFFFFFEull && k.limbs[3] == 0xFFFFFFFFFFFFFFFFull};

        std::cout << (ok ? "[OK]" : "[FAIL]") << " MAX128 * MAX128" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Case 2: 2^64 * 2^64 = 2^128
    // Expected limbs: [0, 0, 1, 0]
    {
        const auto pow64{uint128_t{1, 0}}; // high=1, low=0
        const auto k{karatsuba_full_mul(pow64, pow64)};

        const bool ok{k.limbs[0] == 0 && k.limbs[1] == 0 && k.limbs[2] == 1 && k.limbs[3] == 0};

        std::cout << (ok ? "[OK]" : "[FAIL]") << " 2^64 * 2^64 = 2^128" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Case 3: (2^127)^2 = 2^254
    // Expected: limbs[3] = 2^62 = 0x4000000000000000
    {
        const auto pow127{uint128_t{uint64_t{1} << 63, 0}};
        const auto k{karatsuba_full_mul(pow127, pow127)};

        const bool ok{k.limbs[0] == 0 && k.limbs[1] == 0 && k.limbs[2] == 0 && k.limbs[3] == (uint64_t{1} << 62)};

        std::cout << (ok ? "[OK]" : "[FAIL]") << " 2^127 * 2^127 = 2^254" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Case 4: small values: 3 * 7 = 21
    {
        const auto three{uint128_t{3}};
        const auto seven{uint128_t{7}};
        const auto k{karatsuba_full_mul(three, seven)};

        const bool ok{k.limbs[0] == 21 && k.limbs[1] == 0 && k.limbs[2] == 0 && k.limbs[3] == 0};

        std::cout << (ok ? "[OK]" : "[FAIL]") << " 3 * 7 = 21" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // Case 5: MAX64 * MAX64 (cross-limb boundary)
    // (2^64-1)^2 = 2^128 - 2^65 + 1
    // limbs: [1, 0xFFFFFFFFFFFFFFFE, 0, 0]
    {
        const auto max64_128{uint128_t{MAX64}};
        const auto k{karatsuba_full_mul(max64_128, max64_128)};

        const bool ok{k.limbs[0] == 1 && k.limbs[1] == 0xFFFFFFFFFFFFFFFEull && k.limbs[2] == 0 && k.limbs[3] == 0};

        std::cout << (ok ? "[OK]" : "[FAIL]") << " MAX64 * MAX64" << std::endl;
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Summary
    // =========================================================================
    print_sweep_summary(passed, total);

    return (passed == total) ? 0 : 1;
}
