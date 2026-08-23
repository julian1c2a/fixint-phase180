// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Unary Ops - Systematic 3-region coverage for ++, --, unary -
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests mathematical properties of unary increment, decrement and negation
// over ~6.3M values per sweep using the 3-region methodology.
//
// Migrated from: test_phase5_operators.cpp, test_priority4_arithmetic.cpp
// New coverage:  ~6.3M values per property, 20 edge cases

#include "test_sweep_framework.hpp"
#include <cstdlib>

using nstd::int128_t;
using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Unary Ops Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS << " = " << SWEEP_REGION_SIZE
              << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Pre-increment / Pre-decrement roundtrip
    // ========================================================================
    std::cout << "[Section 1] Increment / Decrement roundtrip\n";
    print_sweep_separator();

    // ++x then --x == x (roundtrip)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                auto copy{a};
                ++copy;
                --copy;
                return copy;
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "inc_dec_roundtrip"))
    {
        ++passed;
    }

    // --x then ++x == x (roundtrip)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                auto copy{a};
                --copy;
                ++copy;
                return copy;
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "dec_inc_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Increment/Decrement vs addition/subtraction
    // ========================================================================
    std::cout << "[Section 2] Increment/Decrement vs arithmetic\n";
    print_sweep_separator();

    // ++x == x + 1
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                auto copy{a};
                ++copy;
                return copy;
            },
            [](const uint128_t &a) -> uint128_t { return a + uint128_t{1ULL}; }, "preinc_eq_add_one"))
    {
        ++passed;
    }

    // --x == x - 1
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                auto copy{a};
                --copy;
                return copy;
            },
            [](const uint128_t &a) -> uint128_t { return a - uint128_t{1ULL}; }, "predec_eq_sub_one"))
    {
        ++passed;
    }

    // post-increment x++ returns old value
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                auto copy{a};
                const auto old{copy++};
                return old;
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "postinc_returns_old"))
    {
        ++passed;
    }

    // post-decrement x-- returns old value
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                auto copy{a};
                const auto old{copy--};
                return old;
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "postdec_returns_old"))
    {
        ++passed;
    }

    // post-increment: after x++, value == x + 1
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                auto copy{a};
                copy++;
                return copy;
            },
            [](const uint128_t &a) -> uint128_t { return a + uint128_t{1ULL}; }, "postinc_increments"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: Double complement identity (~(~x) == x)
    // ========================================================================
    std::cout << "[Section 3] Bitwise complement identity\n";
    print_sweep_separator();

    // ~(~x) == x (already in sweep_bitwise, but good cross-check)
    ++total;
    if (sweep_unary([](const uint128_t &a) -> uint128_t { return ~(~a); },
                    [](const uint128_t &a) -> uint128_t { return a; }, "double_complement"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Unsigned negation properties (-(-x) == x for two's complement)
    // ========================================================================
    std::cout << "[Section 4] Unsigned negation\n";
    print_sweep_separator();

    // For unsigned: -x == ~x + 1 (two's complement definition)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                const uint128_t zero{0ULL};
                return zero - a;
            },
            [](const uint128_t &a) -> uint128_t { return ~a + uint128_t{1ULL}; },
            "neg_eq_complement_plus_one"))
    {
        ++passed;
    }

    // -(-x) == x (double negation)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                const uint128_t zero{0ULL};
                const uint128_t neg{zero - a};
                return zero - neg;
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "double_negation"))
    {
        ++passed;
    }

    // x + (-x) == 0 (additive inverse)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                const uint128_t zero{0ULL};
                const uint128_t neg{zero - a};
                return a + neg;
            },
            [](const uint128_t &) -> uint128_t { return uint128_t{0ULL}; }, "additive_inverse"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: Boolean conversion consistency
    // ========================================================================
    std::cout << "[Section 5] Boolean conversion\n";
    print_sweep_separator();

    // bool(x) == !x.is_zero()
    ++total;
    if (sweep_unary([](const uint128_t &a) -> int { return static_cast<bool>(a) ? 1 : 0; },
                    [](const uint128_t &a) -> int { return a.is_zero() ? 0 : 1; }, "bool_eq_not_is_zero"))
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
