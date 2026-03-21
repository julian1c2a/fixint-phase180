// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Arithmetic - Systematic 3-region coverage for +, -, *
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests mathematical properties of arithmetic operations over ~6.3M (unary)
// and ~12.6M (binary) values per sweep using the 3-region methodology.

#include "test_sweep_framework.hpp"
#include <cstdlib>

using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Arithmetic Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS
              << " = " << SWEEP_REGION_SIZE << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Addition properties
    // ========================================================================
    std::cout << "[Section 1] Addition\n";
    print_sweep_separator();

    // a + b == b + a
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return a + b; },
            [](const uint128_t &a, const uint128_t &b)
            { return b + a; },
            "add_commutativity"))
    {
        ++passed;
    }

    // a + 0 == a
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a + uint128_t{0ULL}; },
            [](const uint128_t &a)
            { return a; },
            "add_zero_identity"))
    {
        ++passed;
    }

    // (a + b) - b == a (add/sub inverse)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return (a + b) - b; },
            [](const uint128_t &a, const uint128_t &b)
            { (void)b; return a; },
            "add_sub_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Subtraction properties
    // ========================================================================
    std::cout << "[Section 2] Subtraction\n";
    print_sweep_separator();

    // a - 0 == a
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a - uint128_t{0ULL}; },
            [](const uint128_t &a)
            { return a; },
            "sub_zero_identity"))
    {
        ++passed;
    }

    // a - a == 0
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a - a; },
            [](const uint128_t &)
            { return uint128_t{0ULL}; },
            "sub_self_zero"))
    {
        ++passed;
    }

    // (a - b) + b == a (sub/add inverse)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return (a - b) + b; },
            [](const uint128_t &a, const uint128_t &b)
            { (void)b; return a; },
            "sub_add_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: Multiplication properties
    // ========================================================================
    std::cout << "[Section 3] Multiplication\n";
    print_sweep_separator();

    // a * b == b * a
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return a * b; },
            [](const uint128_t &a, const uint128_t &b)
            { return b * a; },
            "mul_commutativity"))
    {
        ++passed;
    }

    // a * 1 == a
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a * uint128_t{1ULL}; },
            [](const uint128_t &a)
            { return a; },
            "mul_one_identity"))
    {
        ++passed;
    }

    // a * 0 == 0
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a * uint128_t{0ULL}; },
            [](const uint128_t &)
            { return uint128_t{0ULL}; },
            "mul_zero_absorb"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Increment/Decrement consistency
    // ========================================================================
    std::cout << "[Section 4] Increment/Decrement\n";
    print_sweep_separator();

    // (a + 1) - 1 == a (works for all values including MAX due to wrap-around)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return (a + uint128_t{1ULL}) - uint128_t{1ULL}; },
            [](const uint128_t &a)
            { return a; },
            "incr_decr_roundtrip"))
    {
        ++passed;
    }

    // (a - 1) + 1 == a (works for all values including 0 due to wrap-around)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return (a - uint128_t{1ULL}) + uint128_t{1ULL}; },
            [](const uint128_t &a)
            { return a; },
            "decr_incr_roundtrip"))
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
