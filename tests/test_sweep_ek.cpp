// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep EK — Systematic 3-region coverage for +, -, ++, --
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Binary operations (+, -): 6 region combinations × 2^21 values = ~12.6M/test
// Unary operations (++, --): 3 regions × 2^21 values = ~6.3M/test
// Total: 3 binary + 8 unary = 11 sweep tests, ~88M verifications
//
// EK regions (signed):
//   Region 1: ek(0) .. ek(2^21 - 1)       — small non-negative
//   Region 2: ek(-2^21) .. ek(-1)          — small negative
//   Region 3: large values (random TC->EK) — intermediate/large magnitude

#include "test_sweep_framework.hpp"

using ek_t = nstd::int128_ek_t;
using tc_t = nstd::int128_tc_t;

// =============================================================================
// EK-specific region generators
// =============================================================================

static constexpr uint64_t EK_SEED_Y{SWEEP_FIXED_SEED + 0x0123456789ABCDEFULL};

static ek_t ek_first_region(uint64_t i) noexcept
{
    return ek_t{static_cast<int64_t>(i)};
}

static ek_t ek_last_region(uint64_t i) noexcept
{
    return ek_t{-static_cast<int64_t>(SWEEP_REGION_SIZE - i)};
}

static ek_t ek_random_region(uint64_t i, uint64_t seed = SWEEP_FIXED_SEED) noexcept
{
    SplitMix64 rng{seed + i * 2};
    const uint64_t lo{rng.next()};
    const uint64_t hi{rng.next()};
    return ek_t{tc_t{hi, lo}};
}

// =============================================================================
// sweep_unary_ek: 3 regions, each 2^21 values
// =============================================================================

template <typename F, typename Oracle>
static bool sweep_unary_ek(F &&func, Oracle &&oracle, const char *name)
{
    uint64_t pass{0};
    uint64_t fail{0};

    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const ek_t x{ek_first_region(i)};
        if (func(x) != oracle(x)) { ++fail; } else { ++pass; }
    }
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const ek_t x{ek_last_region(i)};
        if (func(x) != oracle(x)) { ++fail; } else { ++pass; }
    }
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const ek_t x{ek_random_region(i)};
        if (func(x) != oracle(x)) { ++fail; } else { ++pass; }
    }

    print_sweep_result({name, pass, fail, pass + fail});
    return fail == 0;
}

// =============================================================================
// sweep_binary_ek: 6 region combinations, each 2^21 values
// =============================================================================

template <typename F, typename Oracle>
static bool sweep_binary_ek(F &&func, Oracle &&oracle, const char *name)
{
    uint64_t pass{0};
    uint64_t fail{0};

    const auto check = [&](const ek_t &a, const ek_t &b)
    {
        if (func(a, b) != oracle(a, b)) { ++fail; } else { ++pass; }
    };

    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)   // (first, first)
        check(ek_first_region(i), ek_first_region(i));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)   // (first, last)
        check(ek_first_region(i), ek_last_region(i));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)   // (first, random)
        check(ek_first_region(i), ek_random_region(i, EK_SEED_Y));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)   // (last, last)
        check(ek_last_region(i), ek_last_region(i));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)   // (last, random)
        check(ek_last_region(i), ek_random_region(i, EK_SEED_Y));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)   // (random, random)
        check(ek_random_region(i), ek_random_region(i, EK_SEED_Y));

    print_sweep_result({name, pass, fail, pass + fail});
    return fail == 0;
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep EK Tests (3-region signed coverage: +, -, ++, --)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS
              << " = " << SWEEP_REGION_SIZE << " values per region\n";
    std::cout << "  Binary: 6 combos x " << SWEEP_REGION_SIZE << " = "
              << 6 * SWEEP_REGION_SIZE << " verifications/test\n";
    std::cout << "  Unary:  3 regions x " << SWEEP_REGION_SIZE << " = "
              << 3 * SWEEP_REGION_SIZE << " verifications/test\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Addition — binary, 6 region combos
    // ========================================================================
    std::cout << "[Section 1] Addition (binary, 6 region combos)\n";
    print_sweep_separator();

    // a + b == b + a
    ++total;
    if (sweep_binary_ek(
            [](const ek_t &a, const ek_t &b) { return a + b; },
            [](const ek_t &a, const ek_t &b) { return b + a; },
            "add_commutativity"))
        ++passed;

    // (a + b) - b == a  (round-trip in Z/2^128Z)
    ++total;
    if (sweep_binary_ek(
            [](const ek_t &a, const ek_t &b) { return (a + b) - b; },
            [](const ek_t &a, const ek_t &)  { return a; },
            "add_sub_roundtrip"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 2: Addition identity & inverse — unary, 3 regions
    // ========================================================================
    std::cout << "[Section 2] Addition identity & inverse (unary, 3 regions)\n";
    print_sweep_separator();

    // a + ek(0) == a
    ++total;
    if (sweep_unary_ek(
            [](const ek_t &a) { return a + ek_t{0LL}; },
            [](const ek_t &a) { return a; },
            "add_zero_identity"))
        ++passed;

    // a + (-a) == ek(0)
    ++total;
    if (sweep_unary_ek(
            [](const ek_t &a) { return a + (-a); },
            [](const ek_t &)  { return ek_t{0LL}; },
            "add_neg_inverse"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 3: Subtraction — binary, 6 region combos
    // ========================================================================
    std::cout << "[Section 3] Subtraction (binary, 6 region combos)\n";
    print_sweep_separator();

    // (a - b) + b == a
    ++total;
    if (sweep_binary_ek(
            [](const ek_t &a, const ek_t &b) { return (a - b) + b; },
            [](const ek_t &a, const ek_t &)  { return a; },
            "sub_add_roundtrip"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 4: Subtraction identity — unary, 3 regions
    // ========================================================================
    std::cout << "[Section 4] Subtraction identity (unary, 3 regions)\n";
    print_sweep_separator();

    // a - ek(0) == a
    ++total;
    if (sweep_unary_ek(
            [](const ek_t &a) { return a - ek_t{0LL}; },
            [](const ek_t &a) { return a; },
            "sub_zero_identity"))
        ++passed;

    // a - a == ek(0)
    ++total;
    if (sweep_unary_ek(
            [](const ek_t &a) { return a - a; },
            [](const ek_t &)  { return ek_t{0LL}; },
            "sub_self_zero"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 5: Increment — unary, 3 regions
    // ========================================================================
    std::cout << "[Section 5] Increment (unary, 3 regions)\n";
    print_sweep_separator();

    // pre-increment equivalent to +1
    ++total;
    if (sweep_unary_ek(
            [](ek_t a) { ++a; return a; },
            [](const ek_t &a) { return a + ek_t{1LL}; },
            "pre_inc_eq_add1"))
        ++passed;

    // round-trip: ++a then --a restores original (incl. wrap-around at MAX)
    ++total;
    if (sweep_unary_ek(
            [](ek_t a) { ++a; --a; return a; },
            [](const ek_t &a) { return a; },
            "inc_dec_roundtrip"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 6: Decrement — unary, 3 regions
    // ========================================================================
    std::cout << "[Section 6] Decrement (unary, 3 regions)\n";
    print_sweep_separator();

    // pre-decrement equivalent to -1
    ++total;
    if (sweep_unary_ek(
            [](ek_t a) { --a; return a; },
            [](const ek_t &a) { return a - ek_t{1LL}; },
            "pre_dec_eq_sub1"))
        ++passed;

    // round-trip: --a then ++a restores original (incl. wrap-around at MIN)
    ++total;
    if (sweep_unary_ek(
            [](ek_t a) { --a; ++a; return a; },
            [](const ek_t &a) { return a; },
            "dec_inc_roundtrip"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Summary
    // ========================================================================
    print_sweep_summary(passed, total);
    return (passed == total) ? 0 : 1;
}
