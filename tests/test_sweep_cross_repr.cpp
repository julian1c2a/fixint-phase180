// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Cross-Repr — Round-trip conversion between TC, MS, and EK
// Part of int128 Library - Phase 1.80
// License: BSL-1.0
// =============================================================================
//
// Verifies that converting A → B → A yields the original value for all
// combinations of the three signed representations (TC, MS, EK).
//
// The MS generators do not produce MS(-0), so MS → X → MS round-trips are
// safe: canonical MS values are always recovered correctly.
//
// Unary: 3 regions x 2^21 = 6,291,456 verifications/test
// Total: 6 tests, ~37.7M verifications

#include "test_sweep_framework.hpp"
#include <cstdlib>

using tc_t = nstd::int128_tc_t;
using ms_t = nstd::int128_ms_t;
using ek_t = nstd::int128_ek_t;

// =============================================================================
// Region generators (self-contained, not importing from other sweep files)
// =============================================================================

static constexpr uint64_t CROSS_SEED{SWEEP_FIXED_SEED + 0x9E3779B97F4A7C15ULL};

// --- TC ---
static tc_t tc_first(uint64_t i) noexcept { return tc_t{static_cast<int64_t>(i)}; }
static tc_t tc_last(uint64_t i) noexcept { return tc_t{-static_cast<int64_t>(SWEEP_REGION_SIZE - i)}; }
static tc_t tc_rand(uint64_t i) noexcept
{
    SplitMix64 rng{CROSS_SEED + i * 2};
    return tc_t{rng.next(), rng.next()};
}

// --- MS ---
static ms_t ms_first(uint64_t i) noexcept { return ms_t{static_cast<int64_t>(i)}; }
static ms_t ms_last(uint64_t i) noexcept { return ms_t{-static_cast<int64_t>(SWEEP_REGION_SIZE - i)}; }
static ms_t ms_rand(uint64_t i) noexcept
{
    SplitMix64 rng{CROSS_SEED + i * 2};
    return ms_t{tc_t{rng.next(), rng.next()}}; // via TC to avoid MS(-0)
}

// --- EK ---
static ek_t ek_first(uint64_t i) noexcept { return ek_t{static_cast<int64_t>(i)}; }
static ek_t ek_last(uint64_t i) noexcept { return ek_t{-static_cast<int64_t>(SWEEP_REGION_SIZE - i)}; }
static ek_t ek_rand(uint64_t i) noexcept
{
    SplitMix64 rng{CROSS_SEED + i * 2};
    return ek_t{tc_t{rng.next(), rng.next()}};
}

// =============================================================================
// Generic type-parameterised unary sweep over 3 regions
// =============================================================================

template <typename T, typename GenFirst, typename GenLast, typename GenRand,
          typename Func, typename Oracle>
static bool sweep_unary_repr(GenFirst &&gf, GenLast &&gl, GenRand &&gr,
                             Func &&func, Oracle &&oracle, const char *name)
{
    uint64_t pass{0};
    uint64_t fail{0};

    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const T x{gf(i)};
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const T x{gl(i)};
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const T x{gr(i)};
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }

    print_sweep_result({name, pass, fail, pass + fail});
    return fail == 0;
}

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Cross-Representation Round-Trip Tests\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS
              << " = " << SWEEP_REGION_SIZE << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: TC <-> MS round-trips
    // ========================================================================
    std::cout << "[Section 1] TC <-> MS round-trips\n";
    print_sweep_separator();

    // TC -> MS -> TC
    ++total;
    if (sweep_unary_repr<tc_t>(
            tc_first, tc_last, tc_rand,
            [](const tc_t &x)
            { return static_cast<tc_t>(static_cast<ms_t>(x)); },
            [](const tc_t &x)
            { return x; },
            "tc_ms_tc_roundtrip"))
    {
        ++passed;
    }

    // MS -> TC -> MS
    ++total;
    if (sweep_unary_repr<ms_t>(
            ms_first, ms_last, ms_rand,
            [](const ms_t &x)
            { return static_cast<ms_t>(static_cast<tc_t>(x)); },
            [](const ms_t &x)
            { return x; },
            "ms_tc_ms_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: TC <-> EK round-trips
    // ========================================================================
    std::cout << "[Section 2] TC <-> EK round-trips\n";
    print_sweep_separator();

    // TC -> EK -> TC
    ++total;
    if (sweep_unary_repr<tc_t>(
            tc_first, tc_last, tc_rand,
            [](const tc_t &x)
            { return static_cast<tc_t>(static_cast<ek_t>(x)); },
            [](const tc_t &x)
            { return x; },
            "tc_ek_tc_roundtrip"))
    {
        ++passed;
    }

    // EK -> TC -> EK
    ++total;
    if (sweep_unary_repr<ek_t>(
            ek_first, ek_last, ek_rand,
            [](const ek_t &x)
            { return static_cast<ek_t>(static_cast<tc_t>(x)); },
            [](const ek_t &x)
            { return x; },
            "ek_tc_ek_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: MS <-> EK round-trips
    // ========================================================================
    std::cout << "[Section 3] MS <-> EK round-trips\n";
    print_sweep_separator();

    // MS -> EK -> MS
    ++total;
    if (sweep_unary_repr<ms_t>(
            ms_first, ms_last, ms_rand,
            [](const ms_t &x)
            { return static_cast<ms_t>(static_cast<ek_t>(x)); },
            [](const ms_t &x)
            { return x; },
            "ms_ek_ms_roundtrip"))
    {
        ++passed;
    }

    // EK -> MS -> EK
    ++total;
    if (sweep_unary_repr<ek_t>(
            ek_first, ek_last, ek_rand,
            [](const ek_t &x)
            { return static_cast<ek_t>(static_cast<ms_t>(x)); },
            [](const ek_t &x)
            { return x; },
            "ek_ms_ek_roundtrip"))
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
