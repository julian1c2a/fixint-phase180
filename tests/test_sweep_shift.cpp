// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep Shift - Systematic 3-region coverage for <<, >> operators
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests mathematical properties of shift operations over ~6.3M (unary)
// values per sweep using the 3-region methodology.
//
// Migrated from: test_priority7_shift.cpp (ad-hoc ~20 tests)
// New coverage:  ~6.3M values per property, 20+ edge cases

#include "test_sweep_framework.hpp"
#include "int128_param_bits.hpp"
#include <cstdlib>

using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep Shift Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS << " = " << SWEEP_REGION_SIZE
              << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Shift by zero (identity)
    // ========================================================================
    std::cout << "[Section 1] Shift by zero (identity)\n";
    print_sweep_separator();

    // x << 0 == x
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a << 0; }, [](const uint128_t &a) { return a; },
                    "shl_zero_identity"))
    {
        ++passed;
    }

    // x >> 0 == x
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a >> 0; }, [](const uint128_t &a) { return a; },
                    "shr_zero_identity"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Shift-left equivalences with arithmetic
    // ========================================================================
    std::cout << "[Section 2] Shift-left vs arithmetic\n";
    print_sweep_separator();

    // x << 1 == x + x (multiply by 2, wrapping)
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a << 1; }, [](const uint128_t &a) { return a + a; },
                    "shl1_eq_add_self"))
    {
        ++passed;
    }

    // x << 1 == x * 2
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a << 1; },
                    [](const uint128_t &a) { return a * uint128_t{2ULL}; }, "shl1_eq_mul2"))
    {
        ++passed;
    }

    // x << 2 == x * 4
    ++total;
    if (sweep_unary([](const uint128_t &a) { return a << 2; },
                    [](const uint128_t &a) { return a * uint128_t{4ULL}; }, "shl2_eq_mul4"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: Shift roundtrip (shr undoes shl for masked value)
    // ========================================================================
    std::cout << "[Section 3] Shift roundtrip\n";
    print_sweep_separator();

    // (x >> n) << n == x & (~uint128_t{0} << n) — zeros bottom n bits
    // Test for n = 1
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a >> 1) << 1; },
                    [](const uint128_t &a) { return a & (uint128_t::max() << 1); }, "shr1_shl1_clears_bit0"))
    {
        ++passed;
    }

    // Test for n = 8
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a >> 8) << 8; },
                    [](const uint128_t &a) { return a & (uint128_t::max() << 8); }, "shr8_shl8_clears_low8"))
    {
        ++passed;
    }

    // Test for n = 64 (cross-limb boundary)
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a >> 64) << 64; }, [](const uint128_t &a)
                    { return a & (uint128_t::max() << 64); }, "shr64_shl64_clears_low_limb"))
    {
        ++passed;
    }

    // (x << n) >> n == x & (~uint128_t{0} >> n) — zeros top n bits
    // Test for n = 1
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a << 1) >> 1; },
                    [](const uint128_t &a) { return a & (uint128_t::max() >> 1); }, "shl1_shr1_clears_msb"))
    {
        ++passed;
    }

    // Test for n = 64
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a << 64) >> 64; }, [](const uint128_t &a)
                    { return a & (uint128_t::max() >> 64); }, "shl64_shr64_clears_high_limb"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Shift by 128 (full width)
    // ========================================================================
    std::cout << "[Section 4] Full-width and large shifts\n";
    print_sweep_separator();

    // x >> 127 extracts MSB (0 or 1)
    ++total;
    if (sweep_unary([](const uint128_t &a) -> int { return static_cast<int>((a >> 127).low()); },
                    [](const uint128_t &a) -> int { return (a.high() >> 63) & 1; }, "shr127_extracts_msb"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: Shift composition
    // ========================================================================
    std::cout << "[Section 5] Shift composition\n";
    print_sweep_separator();

    // (x << a) << b == x << (a + b) when a + b < 128
    // Test: (x << 3) << 5 == x << 8
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a << 3) << 5; },
                    [](const uint128_t &a) { return a << 8; }, "shl_compose_3_5_eq_8"))
    {
        ++passed;
    }

    // (x >> 3) >> 5 == x >> 8
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a >> 3) >> 5; },
                    [](const uint128_t &a) { return a >> 8; }, "shr_compose_3_5_eq_8"))
    {
        ++passed;
    }

    // (x << 32) << 32 == x << 64
    ++total;
    if (sweep_unary([](const uint128_t &a) { return (a << 32) << 32; },
                    [](const uint128_t &a) { return a << 64; }, "shl_compose_32_32_eq_64"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 6: Shift distributes over bitwise OR
    // ========================================================================
    std::cout << "[Section 6] Distributivity over bitwise OR\n";
    print_sweep_separator();

    // (a | b) << n == (a << n) | (b << n)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return (a | b) << 7; },
                     [](const uint128_t &a, const uint128_t &b) { return (a << 7) | (b << 7); },
                     "shl7_distributes_over_or"))
    {
        ++passed;
    }

    // (a | b) >> n == (a >> n) | (b >> n) (for unsigned/logical shift)
    ++total;
    if (sweep_binary([](const uint128_t &a, const uint128_t &b) { return (a | b) >> 7; },
                     [](const uint128_t &a, const uint128_t &b) { return (a >> 7) | (b >> 7); },
                     "shr7_distributes_over_or"))
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
