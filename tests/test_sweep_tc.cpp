// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep TC — Systematic 3-region coverage for signed TC binary arithmetic
// Part of int128 Library - Phase 1.80
// License: BSL-1.0
// =============================================================================
//
// TC (two's complement) modular arithmetic: all roundtrip properties hold exactly.
// Covers the signed dimension that the unsigned sweep_arithmetic misses.
//
// Binary: 6 region combos × 2^21 = 12,582,912 verifications/test
// Unary:  3 regions × 2^21 = 6,291,456 verifications/test
// Total:  7 binary + 3 unary = 10 tests, ~105M verifications
//
// TC regions (signed):
//   Region 1: tc(0) .. tc(2^21 - 1)       — small non-negative
//   Region 2: tc(-2^21) .. tc(-1)          — small negative
//   Region 3: large values (random 128-bit) — full cross-word range

#include "test_sweep_framework.hpp"

using tc_t = nstd::int128_tc_t;

// =============================================================================
// TC-specific region generators
// =============================================================================

static constexpr uint64_t TC_SEED_Y{SWEEP_FIXED_SEED + 0xABCDEF0123456789ULL};

static tc_t tc_first_region(uint64_t i) noexcept { return tc_t{static_cast<int64_t>(i)}; }

static tc_t tc_last_region(uint64_t i) noexcept { return tc_t{-static_cast<int64_t>(SWEEP_REGION_SIZE - i)}; }

static tc_t tc_random_region(uint64_t i, uint64_t seed = SWEEP_FIXED_SEED) noexcept
{
    SplitMix64 rng{seed + i * 2};
    const uint64_t lo{rng.next()};
    const uint64_t hi{rng.next()};
    return tc_t{hi, lo};
}

// =============================================================================
// Sweep runners
// =============================================================================

template <typename F, typename Oracle>
static bool sweep_unary_tc(F &&func, Oracle &&oracle, const char *name)
{
    uint64_t pass{0};
    uint64_t fail{0};

    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const tc_t x{tc_first_region(i)};
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
        const tc_t x{tc_last_region(i)};
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
        const tc_t x{tc_random_region(i)};
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }

    const SweepResult result{name, pass, fail, pass + fail};
    print_sweep_result(result);
    return fail == 0;
}

template <typename F, typename Oracle>
static bool sweep_binary_tc(F &&func, Oracle &&oracle, const char *name)
{
    uint64_t pass{0};
    uint64_t fail{0};

    // 6 region combinations
    auto run = [&](auto gen_a, auto gen_b)
    {
        for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
        {
            const tc_t a{gen_a(i)};
            const tc_t b{gen_b(i)};
            if (func(a, b) != oracle(a, b))
            {
                ++fail;
            }
            else
            {
                ++pass;
            }
        }
    };

    run(tc_first_region, tc_first_region);
    run(tc_first_region, tc_last_region);
    run(tc_last_region, tc_first_region);
    run(tc_last_region, tc_last_region);
    run([](uint64_t i) { return tc_random_region(i); },
        [](uint64_t i) { return tc_random_region(i, TC_SEED_Y); });
    run([](uint64_t i) { return tc_random_region(i, TC_SEED_Y); },
        [](uint64_t i) { return tc_random_region(i); });

    const SweepResult result{name, pass, fail, pass + fail};
    print_sweep_result(result);
    return fail == 0;
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep TC Tests (3-region systematic coverage, signed TC arithmetic)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS << " = " << SWEEP_REGION_SIZE
              << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Addition commutativity
    // ========================================================================
    std::cout << "[Section 1] Addition\n";
    print_sweep_separator();

    ++total;
    if (sweep_binary_tc([](const tc_t &a, const tc_t &b) { return a + b; },
                        [](const tc_t &a, const tc_t &b) { return b + a; }, "add_commutativity"))
        ++passed;

    ++total;
    if (sweep_binary_tc([](const tc_t &a, const tc_t &b) { return (a + b) - b; },
                        [](const tc_t &a, const tc_t &) { return a; }, "add_sub_roundtrip"))
        ++passed;

    // ========================================================================
    // Section 2: Subtraction
    // ========================================================================
    std::cout << "\n[Section 2] Subtraction\n";
    print_sweep_separator();

    ++total;
    if (sweep_binary_tc([](const tc_t &a, const tc_t &b) { return (a - b) + b; },
                        [](const tc_t &a, const tc_t &) { return a; }, "sub_add_roundtrip"))
        ++passed;

    // (a - b) + (b - a) == 0  (antisymmetry — holds for TC via modular wrap)
    ++total;
    if (sweep_binary_tc([](const tc_t &a, const tc_t &b) { return (a - b) + (b - a); },
                        [](const tc_t &, const tc_t &) { return tc_t{0LL}; }, "sub_antisymmetry"))
        ++passed;

    // ========================================================================
    // Section 3: Additive inverse
    // ========================================================================
    std::cout << "\n[Section 3] Additive inverse\n";
    print_sweep_separator();

    // a + (-a) == 0
    ++total;
    if (sweep_unary_tc([](const tc_t &a) { return a + (-a); }, [](const tc_t &) { return tc_t{0LL}; },
                       "add_neg_inverse"))
        ++passed;

    // ========================================================================
    // Section 4: Multiplication
    // ========================================================================
    std::cout << "\n[Section 4] Multiplication\n";
    print_sweep_separator();

    ++total;
    if (sweep_binary_tc([](const tc_t &a, const tc_t &b) { return a * b; },
                        [](const tc_t &a, const tc_t &b) { return b * a; }, "mul_commutativity"))
        ++passed;

    // a * 1 == a
    ++total;
    if (sweep_unary_tc([](const tc_t &a) { return a * tc_t{1LL}; }, [](const tc_t &a) { return a; },
                       "mul_one_identity"))
        ++passed;

    // a * 0 == 0
    ++total;
    if (sweep_unary_tc([](const tc_t &a) { return a * tc_t{0LL}; }, [](const tc_t &) { return tc_t{0LL}; },
                       "mul_zero"))
        ++passed;

    // ========================================================================
    // Section 5: Increment / Decrement roundtrip
    // ========================================================================
    std::cout << "\n[Section 5] Increment/Decrement\n";
    print_sweep_separator();

    // (++a then --a) == original
    ++total;
    if (sweep_unary_tc(
            [](const tc_t &a)
            {
                tc_t v{a};
                ++v;
                --v;
                return v;
            },
            [](const tc_t &a) { return a; }, "inc_dec_roundtrip"))
        ++passed;

    // ========================================================================
    // Summary
    // ========================================================================
    std::cout << "\n====================================================================\n";
    std::cout << "  RESULTS: " << passed << "/" << total << " tests passed\n";
    if (passed == total)
        std::cout << "  ALL PASS\n";
    else
        std::cout << "  FAILED: " << (total - passed) << " test(s)\n";
    std::cout << "====================================================================\n";

    return (passed == total) ? 0 : 1;
}
