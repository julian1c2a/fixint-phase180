// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Comparison - Systematic 3-region coverage for <, >, <=, >=, ==
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests mathematical properties of comparison operators over ~6.3M (unary)
// and ~12.6M (binary) values per sweep using the 3-region methodology.
//
// Migrated from: test_priority9_friends.cpp, test_param_core_operators.cpp
// New coverage:  ~12.6M pairs per property, 12+ edge cases

#include "test_sweep_framework.hpp"
#include <cstdlib>

using nstd::int128_t;
using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Comparison Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS << " = " << SWEEP_REGION_SIZE
              << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Reflexivity and self-comparison
    // ========================================================================
    std::cout << "[Section 1] Reflexivity\n";
    print_sweep_separator();

    // a == a (always true)
    ++total;
    if (sweep_unary([](const uint128_t &a) -> int { return (a == a) ? 1 : 0; },
                    [](const uint128_t &) -> int { return 1; }, "eq_reflexive"))
    {
        ++passed;
    }

    // !(a < a) (irreflexivity of strict order)
    ++total;
    if (sweep_unary([](const uint128_t &a) -> int { return (a < a) ? 1 : 0; },
                    [](const uint128_t &) -> int { return 0; }, "lt_irreflexive"))
    {
        ++passed;
    }

    // a <= a (always true)
    ++total;
    if (sweep_unary([](const uint128_t &a) -> int { return (a <= a) ? 1 : 0; },
                    [](const uint128_t &) -> int { return 1; }, "le_reflexive"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Complement relations
    // ========================================================================
    std::cout << "[Section 2] Complement relations\n";
    print_sweep_separator();

    // (a <= b) == !(a > b)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) -> int { return (a <= b) ? 1 : 0; },
                     [](const uint128_t &a, const uint128_t &b) -> int { return (a > b) ? 0 : 1; },
                     "le_eq_not_gt"))
    {
        ++passed;
    }

    // (a < b) == !(a >= b)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) -> int { return (a < b) ? 1 : 0; },
                     [](const uint128_t &a, const uint128_t &b) -> int { return (a >= b) ? 0 : 1; },
                     "lt_eq_not_ge"))
    {
        ++passed;
    }

    // (a == b) == (a <= b && b <= a)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) -> int { return (a == b) ? 1 : 0; },
                     [](const uint128_t &a, const uint128_t &b) -> int { return (a <= b && b <= a) ? 1 : 0; },
                     "eq_iff_le_both_ways"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: Trichotomy (exactly one of <, ==, > holds)
    // ========================================================================
    std::cout << "[Section 3] Trichotomy\n";
    print_sweep_separator();

    // Exactly one of: a < b, a == b, a > b
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> int
            {
                const int lt{(a < b) ? 1 : 0};
                const int eq{(a == b) ? 1 : 0};
                const int gt{(a > b) ? 1 : 0};
                return lt + eq + gt;
            },
            [](const uint128_t &, const uint128_t &) -> int { return 1; }, "trichotomy_exactly_one"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Consistency with arithmetic
    // ========================================================================
    std::cout << "[Section 4] Consistency with arithmetic\n";
    print_sweep_separator();

    // (a == b) == ((a - b).is_zero())
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) -> int { return (a == b) ? 1 : 0; },
                     [](const uint128_t &a, const uint128_t &b) -> int { return (a - b).is_zero() ? 1 : 0; },
                     "eq_iff_diff_zero"))
    {
        ++passed;
    }

    // For unsigned: (a < b) == (a - b > a) — wrapping subtraction
    // When a < b, a - b wraps to a large value > a (for unsigned)
    // When a >= b, a - b <= a
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) -> int { return (a < b) ? 1 : 0; },
                     [](const uint128_t &a, const uint128_t &b) -> int { return ((a - b) > a) ? 1 : 0; },
                     "lt_iff_wrap_subtraction"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: Antisymmetry
    // ========================================================================
    std::cout << "[Section 6] Antisymmetry\n";
    print_sweep_separator();

    // If a < b then !(b < a) — antisymmetry
    // Equivalent: (a < b) + (b < a) <= 1
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> int
            {
                const int ab{(a < b) ? 1 : 0};
                const int ba{(b < a) ? 1 : 0};
                return (ab + ba <= 1) ? 1 : 0;
            },
            [](const uint128_t &, const uint128_t &) -> int { return 1; }, "lt_antisymmetric"))
    {
        ++passed;
    }

    // If a <= b and b <= a then a == b
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> int
            {
                if (a <= b && b <= a)
                {
                    return (a == b) ? 1 : 0;
                }
                return 1; // vacuously true
            },
            [](const uint128_t &, const uint128_t &) -> int { return 1; }, "le_antisymmetric"))
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
