// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Division - Systematic 3-region coverage for /, %, divmod
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests mathematical properties of division/modulo operators over ~12.6M
// pairs per sweep using the 3-region methodology.
//
// Migrated from: test_divmod_suite.cpp, test_knuth_vs_binary.cpp,
//                test_division_operators.cpp
// New coverage:  ~12.6M pairs per property, 12+ edge cases
//
// NOTE: Uses (y | 1) as divisor to guarantee non-zero divisor for all inputs.

#include "test_sweep_framework.hpp"
#include <cstdlib>

using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Division Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS << " = " << SWEEP_REGION_SIZE
              << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Fundamental division identity: q * d + r == n
    // ========================================================================
    std::cout << "[Section 1] Division identity (q*d + r == n)\n";
    print_sweep_separator();

    // For (n, y): d = y | 1 (guarantees d >= 1)
    // q = n / d, r = n % d => q * d + r == n
    ++total;
    if (sweep_binary(
            [](const uint128_t &n, const uint128_t &y) -> uint128_t
            {
                const uint128_t d{y | uint128_t{1ULL}};
                const uint128_t q{n / d};
                const uint128_t r{n % d};
                return q * d + r;
            },
            [](const uint128_t &n, const uint128_t &) -> uint128_t { return n; }, "div_identity_q_d_plus_r"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Remainder bound: r < d (for unsigned division)
    // ========================================================================
    std::cout << "[Section 2] Remainder bound (r < d)\n";
    print_sweep_separator();

    ++total;
    if (sweep_binary(
            [](const uint128_t &n, const uint128_t &y) -> int
            {
                const uint128_t d{y | uint128_t{1ULL}};
                const uint128_t r{n % d};
                return (r < d) ? 1 : 0;
            },
            [](const uint128_t &, const uint128_t &) -> int { return 1; }, "remainder_less_than_divisor"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: Division by 1 identity: n / 1 == n
    // ========================================================================
    std::cout << "[Section 3] Division by 1\n";
    print_sweep_separator();

    ++total;
    if (sweep_unary([](const uint128_t &n) -> uint128_t { return n / uint128_t{1ULL}; },
                    [](const uint128_t &n) -> uint128_t { return n; }, "div_by_one_identity"))
    {
        ++passed;
    }

    ++total;
    if (sweep_unary([](const uint128_t &n) -> uint128_t { return n % uint128_t{1ULL}; },
                    [](const uint128_t &) -> uint128_t { return uint128_t{0ULL}; }, "mod_by_one_is_zero"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Self-division: n / n == 1, n % n == 0 (for n != 0)
    // ========================================================================
    std::cout << "[Section 4] Self-division\n";
    print_sweep_separator();

    // n / n == 1 (skip n == 0)
    ++total;
    if (sweep_unary(
            [](const uint128_t &n) -> uint128_t
            {
                if (n.is_zero())
                {
                    return uint128_t{1ULL}; // vacuously return expected
                }
                return n / n;
            },
            [](const uint128_t &) -> uint128_t { return uint128_t{1ULL}; }, "self_div_is_one"))
    {
        ++passed;
    }

    // n % n == 0 (skip n == 0)
    ++total;
    if (sweep_unary(
            [](const uint128_t &n) -> uint128_t
            {
                if (n.is_zero())
                {
                    return uint128_t{0ULL}; // vacuously return expected
                }
                return n % n;
            },
            [](const uint128_t &) -> uint128_t { return uint128_t{0ULL}; }, "self_mod_is_zero"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: Zero dividend: 0 / d == 0, 0 % d == 0
    // ========================================================================
    std::cout << "[Section 5] Zero dividend\n";
    print_sweep_separator();

    ++total;
    if (sweep_unary(
            [](const uint128_t &d) -> uint128_t
            {
                if (d.is_zero())
                {
                    return uint128_t{0ULL}; // vacuously return expected
                }
                return uint128_t{0ULL} / d;
            },
            [](const uint128_t &) -> uint128_t { return uint128_t{0ULL}; }, "zero_div_d_is_zero"))
    {
        ++passed;
    }

    ++total;
    if (sweep_unary(
            [](const uint128_t &d) -> uint128_t
            {
                if (d.is_zero())
                {
                    return uint128_t{0ULL}; // vacuously return expected
                }
                return uint128_t{0ULL} % d;
            },
            [](const uint128_t &) -> uint128_t { return uint128_t{0ULL}; }, "zero_mod_d_is_zero"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 6: Division by powers of 2 vs right shift
    // ========================================================================
    std::cout << "[Section 6] Division by powers of 2\n";
    print_sweep_separator();

    // n / 2 == n >> 1
    ++total;
    if (sweep_unary([](const uint128_t &n) -> uint128_t { return n / uint128_t{2ULL}; },
                    [](const uint128_t &n) -> uint128_t { return n >> 1; }, "div_by_2_eq_shr1"))
    {
        ++passed;
    }

    // n / 4 == n >> 2
    ++total;
    if (sweep_unary([](const uint128_t &n) -> uint128_t { return n / uint128_t{4ULL}; },
                    [](const uint128_t &n) -> uint128_t { return n >> 2; }, "div_by_4_eq_shr2"))
    {
        ++passed;
    }

    // n % 2 == n & 1
    ++total;
    if (sweep_unary([](const uint128_t &n) -> uint128_t { return n % uint128_t{2ULL}; },
                    [](const uint128_t &n) -> uint128_t { return n & uint128_t{1ULL}; }, "mod_2_eq_and_1"))
    {
        ++passed;
    }

    // n % 4 == n & 3
    ++total;
    if (sweep_unary([](const uint128_t &n) -> uint128_t { return n % uint128_t{4ULL}; },
                    [](const uint128_t &n) -> uint128_t { return n & uint128_t{3ULL}; }, "mod_4_eq_and_3"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 7: Quotient bound: q <= n (when d >= 1)
    // ========================================================================
    std::cout << "[Section 7] Quotient bound\n";
    print_sweep_separator();

    ++total;
    if (sweep_binary(
            [](const uint128_t &n, const uint128_t &y) -> int
            {
                const uint128_t d{y | uint128_t{1ULL}};
                const uint128_t q{n / d};
                return (q <= n) ? 1 : 0;
            },
            [](const uint128_t &, const uint128_t &) -> int { return 1; }, "quotient_le_dividend"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Summary
    // ========================================================================
    print_sweep_summary(passed, total);

    return (passed == total) ? EXIT_SUCCESS : EXIT_FAILURE;
}
