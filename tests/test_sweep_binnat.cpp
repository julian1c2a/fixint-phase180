// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Binnat — Algebraic properties specific to unsigned 128-bit integers
// Part of int128 Library - Phase 1.80
// License: BSL-1.0
// =============================================================================
//
// Binnat (unsigned natural binary) properties not covered by other sweep files:
//   - Ring axioms: add/mul associativity, mul distributive over add (mod 2^128)
//   - Complement arithmetic: a+~a==MAX, ~a==MAX-a, unsigned negation
//   - Bitwise monotonicity: (a|b)>=a, (a&b)<=a  (unsigned order preservation)
//   - Binary adder identity: a+b == (a^b)+((a&b)<<1)  (mod 2^128)
//   - Boolean decomposition: a|b == (a^b)|(a&b)
//
// Not duplicated: commutativity (test_sweep_arithmetic), division identities
// (test_sweep_division), De Morgan (test_sweep_bitwise), comparison properties
// (test_sweep_comparison).
//
// Binary: 6 region combos x 2^21 = 12,582,912 verifications/test
// Unary:  3 regions       x 2^21 =  6,291,456 verifications/test
// Total:  8 binary + 3 unary = 11 tests, ~120M verifications

#include "test_sweep_framework.hpp"
#include <cstdlib>

using nstd::uint128_t;

// Fixed third operand for ternary-like associativity/distributivity tests.
// Chosen to exercise carries across the 64-bit limb boundary.
static const uint128_t C_TERNARY{UINT64_C(0xFEDCBA9876543210), UINT64_C(0x0123456789ABCDEF)};

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Binnat Tests (unsigned algebraic properties)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS
              << " = " << SWEEP_REGION_SIZE << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Additive ring axioms
    // ========================================================================
    std::cout << "[Section 1] Additive ring axioms\n";
    print_sweep_separator();

    // (a + b) + C == a + (b + C)  — associativity (mod 2^128)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return (a + b) + C_TERNARY; },
            [](const uint128_t &a, const uint128_t &b)
            { return a + (b + C_TERNARY); },
            "add_associativity"))
    {
        ++passed;
    }

    // a - b == a + (~b + 1)  — subtraction via unsigned complement (mod 2^128)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return a - b; },
            [](const uint128_t &a, const uint128_t &b)
            { return a + (~b + uint128_t{1ULL}); },
            "sub_via_complement"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Multiplicative ring axioms
    // ========================================================================
    std::cout << "[Section 2] Multiplicative ring axioms\n";
    print_sweep_separator();

    // (a * b) * C == a * (b * C)  — mul associativity (mod 2^128)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return (a * b) * C_TERNARY; },
            [](const uint128_t &a, const uint128_t &b)
            { return a * (b * C_TERNARY); },
            "mul_associativity"))
    {
        ++passed;
    }

    // a * (b + C) == a*b + a*C  — distributivity (mod 2^128)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return a * (b + C_TERNARY); },
            [](const uint128_t &a, const uint128_t &b)
            { return a * b + a * C_TERNARY; },
            "mul_distributive_add"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: Complement arithmetic (binnat-specific)
    // ========================================================================
    std::cout << "[Section 3] Complement arithmetic\n";
    print_sweep_separator();

    // a + ~a == MAX  — complement sum is all-ones
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a + ~a; },
            [](const uint128_t &)
            { return uint128_t::max(); },
            "complement_sum_is_max"))
    {
        ++passed;
    }

    // ~a == MAX - a  — complement equals MAX minus value
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return ~a; },
            [](const uint128_t &a)
            { return uint128_t::max() - a; },
            "complement_is_max_minus"))
    {
        ++passed;
    }

    // a + (~a + 1) == 0  — unsigned negation wraps to zero (mod 2^128)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a)
            { return a + (~a + uint128_t{1ULL}); },
            [](const uint128_t &)
            { return uint128_t{0ULL}; },
            "negate_via_complement"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Bitwise monotonicity (unsigned order preservation)
    // ========================================================================
    std::cout << "[Section 4] Bitwise monotonicity\n";
    print_sweep_separator();

    // (a | b) >= a  — OR is >= both operands (unsigned)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> int
            { return ((a | b) >= a) ? 1 : 0; },
            [](const uint128_t &, const uint128_t &) -> int
            { return 1; },
            "or_geq_left_operand"))
    {
        ++passed;
    }

    // (a & b) <= a  — AND is <= both operands (unsigned)
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b) -> int
            { return ((a & b) <= a) ? 1 : 0; },
            [](const uint128_t &, const uint128_t &) -> int
            { return 1; },
            "and_leq_left_operand"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: Fundamental binary identities
    // ========================================================================
    std::cout << "[Section 5] Fundamental binary identities\n";
    print_sweep_separator();

    // a + b == (a ^ b) + ((a & b) << 1)  — binary adder identity (mod 2^128)
    // Proof: for each bit position i, a[i]+b[i] = (a[i]^b[i]) + 2*(a[i]&b[i]);
    // summing over all positions gives the full identity modulo 2^128.
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return a + b; },
            [](const uint128_t &a, const uint128_t &b)
            { return (a ^ b) + ((a & b) << 1); },
            "add_via_xor_carry"))
    {
        ++passed;
    }

    // a | b == (a ^ b) | (a & b)  — OR via XOR and AND
    // Proof per bit: 0|0=0, 0^0|0&0=0; 0|1=1, 0^1|0&1=1; 1|1=1, 1^1|1&1=0|1=1.
    ++total;
    if (sweep_binary(
            [](const uint128_t &a, const uint128_t &b)
            { return a | b; },
            [](const uint128_t &a, const uint128_t &b)
            { return (a ^ b) | (a & b); },
            "or_via_xor_and"))
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
