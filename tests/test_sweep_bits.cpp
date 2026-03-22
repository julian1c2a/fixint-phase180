// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Bits - Systematic 3-region coverage for bit query/rotation ops
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests mathematical properties of leading_zeros, trailing_zeros, bit_width,
// is_power_of_2, rotate_left, and rotate_right over ~6.3M (unary) and
// ~12.6M (binary) values per sweep using the 3-region methodology.
//
// Complements test_sweep_bitwise.cpp which covers &, |, ^, ~, popcount.

#include "test_sweep_framework.hpp"
#include "int128_param_bits.hpp"
#include <cstdlib>

using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Bit-Query & Rotation Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS
              << " = " << SWEEP_REGION_SIZE << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: leading_zeros + bit_width identity
    // ========================================================================
    std::cout << "[Section 1] leading_zeros + bit_width\n";
    print_sweep_separator();

    // leading_zeros(x) + bit_width(x) == 128  (always, including x=0)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return a.leading_zeros() + a.bit_width(); },
            [](const uint128_t &) -> int
            { return 128; },
            "lz_plus_bw_eq_128"))
    {
        ++passed;
    }

    // leading_zeros(x) range: 0 <= lz(x) <= 128
    // Verify: lz(x | MSB) == 0  (setting MSB forces lz=0)
    {
        const uint128_t msb{0x8000000000000000ULL, 0ULL};
        ++total;
        if (sweep_unary(
                [msb](const uint128_t &a) -> int
                { return (a | msb).leading_zeros(); },
                [](const uint128_t &) -> int
                { return 0; },
                "lz_with_msb_set_is_zero"))
        {
            ++passed;
        }
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: trailing_zeros properties
    // ========================================================================
    std::cout << "[Section 2] trailing_zeros\n";
    print_sweep_separator();

    // trailing_zeros(x | 1) == 0  (setting bit 0 forces tz=0)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return (a | uint128_t{1ULL}).trailing_zeros(); },
            [](const uint128_t &) -> int
            { return 0; },
            "tz_with_bit0_set_is_zero"))
    {
        ++passed;
    }

    // For x != 0: trailing_zeros(x) + leading_zeros(x) <= 127
    // Equivalently: for all x, tz(x|1) + lz(x|MSB) <= 127
    // (This is trivially true since both are 0 with those ops, but test below
    //  verifies the original: for x != 0, tz + lz < 128)
    // We test: trailing_zeros(x) <= bit_width(x) - 1 for x != 0
    // Better: verify trailing_zeros matches countr_zero free function
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return a.trailing_zeros(); },
            [](const uint128_t &a) -> int
            { return nstd::countr_zero(a); },
            "tz_matches_countr_zero"))
    {
        ++passed;
    }

    // trailing_zeros(x) == trailing_zeros(x & -x) for x != 0
    // where -x is two's complement negation; x & -x isolates lowest set bit
    // For x == 0, both return 128
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return a.trailing_zeros(); },
            [](const uint128_t &a) -> int
            {
                if (a.is_zero())
                {
                    return 128;
                }
                // -a for unsigned is ~a + 1 (wrapping)
                const uint128_t neg_a{~a + uint128_t{1ULL}};
                return (a & neg_a).trailing_zeros();
            },
            "tz_equals_tz_of_lowest_bit"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: is_power_of_2 vs popcount identity
    // ========================================================================
    std::cout << "[Section 3] is_power_of_2\n";
    print_sweep_separator();

    // is_power_of_2(x) == (x != 0 && popcount(x) == 1)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return a.is_power_of_2() ? 1 : 0; },
            [](const uint128_t &a) -> int
            { return (!a.is_zero() && a.count_ones() == 1) ? 1 : 0; },
            "pow2_iff_popcount_eq_1"))
    {
        ++passed;
    }

    // is_power_of_2(x) == (x != 0 && (x & (x-1)) == 0)  [bit trick]
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return a.is_power_of_2() ? 1 : 0; },
            [](const uint128_t &a) -> int
            {
                if (a.is_zero())
                {
                    return 0;
                }
                return ((a & (a - uint128_t{1ULL})).is_zero()) ? 1 : 0;
            },
            "pow2_bit_trick_identity"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Rotation roundtrip properties
    // ========================================================================
    std::cout << "[Section 4] Rotation roundtrips\n";
    print_sweep_separator();

    // rotate_left(rotate_right(x, 37), 37) == x
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a.rotate_right(37).rotate_left(37); },
            [](const uint128_t &a)
            { return a; },
            "rotl_rotr_roundtrip_37"))
    {
        ++passed;
    }

    // rotate_right(rotate_left(x, 91), 91) == x
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a.rotate_left(91).rotate_right(91); },
            [](const uint128_t &a)
            { return a; },
            "rotr_rotl_roundtrip_91"))
    {
        ++passed;
    }

    // rotate_left(x, 64) == rotate_right(x, 64)  (half rotation)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a.rotate_left(64); },
            [](const uint128_t &a)
            { return a.rotate_right(64); },
            "rot64_left_eq_right"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: Rotation preserves popcount
    // ========================================================================
    std::cout << "[Section 5] Rotation preserves popcount\n";
    print_sweep_separator();

    // popcount(rotate_left(x, 37)) == popcount(x)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return a.rotate_left(37).count_ones(); },
            [](const uint128_t &a) -> int
            { return a.count_ones(); },
            "rotl37_preserves_popcount"))
    {
        ++passed;
    }

    // popcount(rotate_right(x, 91)) == popcount(x)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            { return a.rotate_right(91).count_ones(); },
            [](const uint128_t &a) -> int
            { return a.count_ones(); },
            "rotr91_preserves_popcount"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 6: Rotation composition
    // ========================================================================
    std::cout << "[Section 6] Rotation composition\n";
    print_sweep_separator();

    // rotate_left(x, a) then rotate_left(x, b) == rotate_left(x, a+b)
    // rotl(rotl(x, 17), 29) == rotl(x, 46)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a.rotate_left(17).rotate_left(29); },
            [](const uint128_t &a)
            { return a.rotate_left(46); },
            "rotl_composition_17_29"))
    {
        ++passed;
    }

    // rotr(rotr(x, 50), 13) == rotr(x, 63)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a.rotate_right(50).rotate_right(13); },
            [](const uint128_t &a)
            { return a.rotate_right(63); },
            "rotr_composition_50_13"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 7: Cross-property consistency (binary)
    // ========================================================================
    std::cout << "[Section 7] Cross-property binary\n";
    print_sweep_separator();

    // popcount(a & b) <= min(popcount(a), popcount(b))
    // We test: popcount(a & b) <= popcount(a) (always true)
    // Expressed as equality check: min(popcount(a), popcount(a&b)) == popcount(a&b)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> int
            { return (a & b).count_ones(); },
            [](const uint128_t &a, const uint128_t &b) -> int
            {
                const int pab{(a & b).count_ones()};
                const int pa{a.count_ones()};
                // If pab <= pa, return pab (match); otherwise return -1 (mismatch)
                return (pab <= pa) ? pab : -1;
            },
            "popcount_and_leq_popcount"))
    {
        ++passed;
    }

    // bit_width(a | b) >= max(bit_width(a), bit_width(b))
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> int
            { return (a | b).bit_width(); },
            [](const uint128_t &a, const uint128_t &b) -> int
            {
                const int bw_or{(a | b).bit_width()};
                const int bw_a{a.bit_width()};
                const int bw_b{b.bit_width()};
                const int mx{(bw_a > bw_b) ? bw_a : bw_b};
                return (bw_or >= mx) ? bw_or : -1;
            },
            "bw_or_geq_max_bw"))
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
