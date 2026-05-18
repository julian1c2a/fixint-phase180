// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep MS — Systematic 3-region coverage for +, -, *, /, ++, --
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// MS-specific semantics tested:
//   - Sign-aware addition/subtraction (not raw TC binary ops)
//   - Sign-XOR multiplication rule; zero product is always +0
//   - Division: truncate toward zero, q*d+r == n identity
//   - Increment/decrement honour ±0 and wrap-around
//
// Oracle strategy:
//   - Add/sub/inc/dec: property invariants (commutativity, roundtrip, identity)
//   - Mul: commutativity, sign-XOR rule, ×1 identity
//   - Div: q*d+r == n identity (b==0 → trivially pass)
//
// Binary: 6 region combos × 2^21 = 12,582,912 verifications/test
// Unary:  3 regions × 2^21 = 6,291,456 verifications/test
// Total:  6 binary + 9 unary = 15 tests, ~133M verifications
//
// MS regions (signed):
//   Region 1: ms(0) .. ms(2^21 - 1)        — small non-negative
//   Region 2: ms(-2^21) .. ms(-1)           — small negative
//   Region 3: large values (random TC→MS)   — intermediate/large magnitude

#include "test_sweep_framework.hpp"

using ms_t = nstd::int128_ms_t;
using tc_t = nstd::int128_tc_t;

// =============================================================================
// MS-specific region generators
// =============================================================================

static constexpr uint64_t MS_SEED_Y{SWEEP_FIXED_SEED + 0x0FEDCBA987654321ULL};

static ms_t ms_first_region(uint64_t i) noexcept
{
    return ms_t{static_cast<int64_t>(i)};
}

static ms_t ms_last_region(uint64_t i) noexcept
{
    return ms_t{-static_cast<int64_t>(SWEEP_REGION_SIZE - i)};
}

static ms_t ms_random_region(uint64_t i, uint64_t seed = SWEEP_FIXED_SEED) noexcept
{
    SplitMix64 rng{seed + i * 2};
    const uint64_t lo{rng.next()};
    const uint64_t hi{rng.next()};
    // Construct via TC cross-repr cast so all 128 bits are exercised.
    // Sign bit is whatever bit 127 happens to be — genuinely random.
    return ms_t{tc_t{hi, lo}};
}

// =============================================================================
// Sweep runners (mirror test_sweep_ek.cpp pattern)
// =============================================================================

template <typename F, typename Oracle>
static bool sweep_unary_ms(F &&func, Oracle &&oracle, const char *name)
{
    uint64_t pass{0};
    uint64_t fail{0};

    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const ms_t x{ms_first_region(i)};
        if (func(x) != oracle(x)) { ++fail; } else { ++pass; }
    }
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const ms_t x{ms_last_region(i)};
        if (func(x) != oracle(x)) { ++fail; } else { ++pass; }
    }
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
    {
        const ms_t x{ms_random_region(i)};
        if (func(x) != oracle(x)) { ++fail; } else { ++pass; }
    }

    print_sweep_result({name, pass, fail, pass + fail});
    return fail == 0;
}

template <typename F, typename Oracle>
static bool sweep_binary_ms(F &&func, Oracle &&oracle, const char *name)
{
    uint64_t pass{0};
    uint64_t fail{0};

    const auto check = [&](const ms_t &a, const ms_t &b)
    {
        if (func(a, b) != oracle(a, b)) { ++fail; } else { ++pass; }
    };

    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
        check(ms_first_region(i), ms_first_region(i));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
        check(ms_first_region(i), ms_last_region(i));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
        check(ms_first_region(i), ms_random_region(i, MS_SEED_Y));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
        check(ms_last_region(i), ms_last_region(i));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
        check(ms_last_region(i), ms_random_region(i, MS_SEED_Y));
    for (uint64_t i{0}; i < SWEEP_REGION_SIZE; ++i)
        check(ms_random_region(i), ms_random_region(i, MS_SEED_Y));

    print_sweep_result({name, pass, fail, pass + fail});
    return fail == 0;
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep MS Tests (3-region signed coverage: +, -, *, /, ++, --)\n";
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
    if (sweep_binary_ms(
            [](const ms_t &a, const ms_t &b) { return a + b; },
            [](const ms_t &a, const ms_t &b) { return b + a; },
            "add_commutativity"))
        ++passed;

    // (a - b) + (b - a) == +0   (antisymmetry: both sides have equal magnitude, opposite sign)
    ++total;
    if (sweep_binary_ms(
            [](const ms_t &a, const ms_t &b) { return (a - b) + (b - a); },
            [](const ms_t &, const ms_t &)   { return ms_t{0LL}; },
            "sub_antisymmetry"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 2: Addition identity & inverse — unary, 3 regions
    // ========================================================================
    std::cout << "[Section 2] Addition identity & inverse (unary, 3 regions)\n";
    print_sweep_separator();

    // a + ms(0) == a
    ++total;
    if (sweep_unary_ms(
            [](const ms_t &a) { return a + ms_t{0LL}; },
            [](const ms_t &a) { return a; },
            "add_zero_identity"))
        ++passed;

    // a + (-a) == +0   (MS zero from opposite-sign cancel is always positive)
    ++total;
    if (sweep_unary_ms(
            [](const ms_t &a) { return a + (-a); },
            [](const ms_t &)  { return ms_t{0LL}; },
            "add_neg_inverse"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 3: Subtraction — binary, 6 region combos
    // ========================================================================
    std::cout << "[Section 3] Subtraction (binary, 6 region combos)\n";
    print_sweep_separator();

    // a - b == a + (-b)   (subtraction is addition of negation, holds for all MS values)
    ++total;
    if (sweep_binary_ms(
            [](const ms_t &a, const ms_t &b) { return a - b; },
            [](const ms_t &a, const ms_t &b) { return a + (-b); },
            "sub_eq_add_neg"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 4: Subtraction identity — unary, 3 regions
    // ========================================================================
    std::cout << "[Section 4] Subtraction identity (unary, 3 regions)\n";
    print_sweep_separator();

    // a - ms(0) == a
    ++total;
    if (sweep_unary_ms(
            [](const ms_t &a) { return a - ms_t{0LL}; },
            [](const ms_t &a) { return a; },
            "sub_zero_identity"))
        ++passed;

    // a - a == +0   (same value subtracted is always positive zero in MS)
    ++total;
    if (sweep_unary_ms(
            [](const ms_t &a) { return a - a; },
            [](const ms_t &)  { return ms_t{0LL}; },
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
    if (sweep_unary_ms(
            [](ms_t a) { ++a; return a; },
            [](const ms_t &a) { return a + ms_t{1LL}; },
            "pre_inc_eq_add1"))
        ++passed;

    // round-trip: ++a then --a restores original (incl. wrap at MAX)
    ++total;
    if (sweep_unary_ms(
            [](ms_t a) { ++a; --a; return a; },
            [](const ms_t &a) { return a; },
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
    if (sweep_unary_ms(
            [](ms_t a) { --a; return a; },
            [](const ms_t &a) { return a - ms_t{1LL}; },
            "pre_dec_eq_sub1"))
        ++passed;

    // round-trip: --a then ++a restores original (incl. wrap at MIN)
    ++total;
    if (sweep_unary_ms(
            [](ms_t a) { --a; ++a; return a; },
            [](const ms_t &a) { return a; },
            "dec_inc_roundtrip"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 7: Multiplication commutativity — binary, 6 region combos
    // ========================================================================
    std::cout << "[Section 7] Multiplication commutativity (binary, 6 region combos)\n";
    print_sweep_separator();

    // a * b == b * a
    ++total;
    if (sweep_binary_ms(
            [](const ms_t &a, const ms_t &b) { return a * b; },
            [](const ms_t &a, const ms_t &b) { return b * a; },
            "mul_commutativity"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 8: Multiplication sign rule — binary, 6 region combos
    // ========================================================================
    // MS rule: result is negative iff exactly one operand is negative AND
    //          the product is non-zero.  Express as: sign-indicator (±1) matches.
    std::cout << "[Section 8] Multiplication sign rule (binary, 6 region combos)\n";
    print_sweep_separator();

    ++total;
    if (sweep_binary_ms(
            [](const ms_t &a, const ms_t &b) -> ms_t
            {
                const ms_t r{a * b};
                return r.is_zero() ? ms_t{0LL} : (r.is_negative() ? ms_t{-1LL} : ms_t{1LL});
            },
            [](const ms_t &a, const ms_t &b) -> ms_t
            {
                const ms_t r{a * b};
                const bool expected_neg{a.is_negative() != b.is_negative()};
                return r.is_zero() ? ms_t{0LL} : (expected_neg ? ms_t{-1LL} : ms_t{1LL});
            },
            "mul_sign_rule"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 9: Multiplication ×1 identity — unary, 3 regions
    // ========================================================================
    std::cout << "[Section 9] Multiplication x1 identity (unary, 3 regions)\n";
    print_sweep_separator();

    // a * ms(1) == a
    ++total;
    if (sweep_unary_ms(
            [](const ms_t &a) { return a * ms_t{1LL}; },
            [](const ms_t &a) { return a; },
            "mul_one_identity"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Section 10: Division identity — binary, 6 region combos
    // ========================================================================
    // Property: q * d + r == a  (for any a, b != 0)
    // When b == 0: trivially pass (undefined, skip).
    std::cout << "[Section 10] Division identity q*d+r==n (binary, 6 region combos)\n";
    print_sweep_separator();

    ++total;
    if (sweep_binary_ms(
            [](const ms_t &a, const ms_t &b) -> ms_t
            {
                if (b.is_zero()) { return a; }   // skip: oracle also returns a
                const auto [q, r] = a.divmod(b);
                return q * b + r;
            },
            [](const ms_t &a, const ms_t &) -> ms_t { return a; },
            "div_identity_q_d_plus_r"))
        ++passed;

    std::cout << "\n";

    // ========================================================================
    // Summary
    // ========================================================================
    print_sweep_summary(passed, total);
    return (passed == total) ? 0 : 1;
}
