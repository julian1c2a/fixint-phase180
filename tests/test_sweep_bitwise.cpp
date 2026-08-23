// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Bitwise - Systematic 3-region coverage for &, |, ^, ~, popcount
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests mathematical properties of bitwise operations and bit-counting
// functions over ~6.3M (unary) and ~12.6M (binary) values per sweep.

#include "test_sweep_framework.hpp"
#include "int128_param_bits.hpp"
#include <cstdlib>

using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Bitwise Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS << " = " << SWEEP_REGION_SIZE
              << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Commutativity
    // ========================================================================
    std::cout << "[Section 1] Commutativity\n";
    print_sweep_separator();

    // a & b == b & a
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return a & b; },
                     [](const uint128_t &a, const uint128_t &b) { return b & a; }, "and_commutativity"))
    {
        ++passed;
    }

    // a | b == b | a
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return a | b; },
                     [](const uint128_t &a, const uint128_t &b) { return b | a; }, "or_commutativity"))
    {
        ++passed;
    }

    // a ^ b == b ^ a
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return a ^ b; },
                     [](const uint128_t &a, const uint128_t &b) { return b ^ a; }, "xor_commutativity"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Identity / Self operations
    // ========================================================================
    std::cout << "[Section 2] Identity and Self\n";
    print_sweep_separator();

    // ~~a == a (complement involution)
    ++total;
    if (sweep_unary([](const uint128_t &a) { return ~(~a); }, [](const uint128_t &a) { return a; },
                    "not_involution"))
    {
        ++passed;
    }

    // a ^ a == 0
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a ^ a; },
                    [](const uint128_t &) { return uint128_t{0ULL}; }, "xor_self_zero"))
    {
        ++passed;
    }

    // a & a == a (idempotent)
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a & a; }, [](const uint128_t &a) { return a; },
                    "and_self_idempotent"))
    {
        ++passed;
    }

    // a | a == a (idempotent)
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a | a; }, [](const uint128_t &a) { return a; },
                    "or_self_idempotent"))
    {
        ++passed;
    }

    // a & MAX == a
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a & uint128_t::max(); },
                    [](const uint128_t &a) { return a; }, "and_max_identity"))
    {
        ++passed;
    }

    // a | 0 == a
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a | uint128_t{0ULL}; },
                    [](const uint128_t &a) { return a; }, "or_zero_identity"))
    {
        ++passed;
    }

    // a ^ 0 == a
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a ^ uint128_t{0ULL}; },
                    [](const uint128_t &a) { return a; }, "xor_zero_identity"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: De Morgan's Laws
    // ========================================================================
    std::cout << "[Section 3] De Morgan's Laws\n";
    print_sweep_separator();

    // ~(a & b) == (~a) | (~b)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return ~(a & b); },
                     [](const uint128_t &a, const uint128_t &b) { return (~a) | (~b); }, "de_morgan_and"))
    {
        ++passed;
    }

    // ~(a | b) == (~a) & (~b)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return ~(a | b); },
                     [](const uint128_t &a, const uint128_t &b) { return (~a) & (~b); }, "de_morgan_or"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: XOR algebraic identities
    // ========================================================================
    std::cout << "[Section 4] XOR Algebra\n";
    print_sweep_separator();

    // a ^ b ^ b == a (XOR cancel)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return a ^ b ^ b; },
                     [](const uint128_t &a, const uint128_t &b)
                     {
                         (void)b;
                         return a;
                     },
                     "xor_cancel"))
    {
        ++passed;
    }

    // a ^ b == (a | b) & ~(a & b) (XOR definition)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return a ^ b; },
                     [](const uint128_t &a, const uint128_t &b) { return (a | b) & ~(a & b); },
                     "xor_definition"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: Bit counting
    // ========================================================================
    std::cout << "[Section 5] Bit Counting\n";
    print_sweep_separator();

    // popcount(a) + popcount(~a) == 128
    ++total;
    if (sweep_unary([](const uint128_t &a) -> int { return a.count_ones() + (~a).count_ones(); },
                    [](const uint128_t &) -> int { return 128; }, "popcount_complement_sum"))
    {
        ++passed;
    }

    // popcount(a & b) + popcount(a | b) == popcount(a) + popcount(b)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) -> int
                     { return (a & b).count_ones() + (a | b).count_ones(); },
                     [](const uint128_t &a, const uint128_t &b) -> int
                     { return a.count_ones() + b.count_ones(); }, "popcount_and_or_sum"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Summary
    // ========================================================================
    print_sweep_summary(passed, total);

    return (passed == total) ? 0 : 1;
}
