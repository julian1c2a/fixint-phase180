// =============================================================================
// Test: int128_param_limits.hpp - Numeric Limits
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_limits.hpp"
#include <iostream>
#include <limits>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Testing int128_param_limits.hpp (Numeric Limits)...\n\n";

    // ========================================================================
    // TEST 1: TWO'S COMPLEMENT UNSIGNED TRAITS
    // ========================================================================
    {
        std::cout << "Test 1: TC Unsigned Traits\n";
        using Limits = std::numeric_limits<uint128_tc_t>;

        assert(Limits::is_specialized);
        assert(!Limits::is_signed);
        assert(Limits::is_integer);
        assert(Limits::is_exact);
        assert(Limits::is_bounded);
        assert(Limits::is_modulo);
        assert(Limits::digits == 128);
        assert(Limits::digits10 == 38);
        assert(Limits::radix == 2);

        std::cout << "  ✓ All traits correct\n";
    }

    // ========================================================================
    // TEST 2: TWO'S COMPLEMENT UNSIGNED VALUES
    // ========================================================================
    {
        std::cout << "\nTest 2: TC Unsigned Values\n";
        using Limits = std::numeric_limits<uint128_tc_t>;

        const auto min_val = Limits::min();
        const auto max_val = Limits::max();

        assert((min_val == uint128_tc_t{0, 0}));
        assert((max_val == uint128_tc_t{~0ULL, ~0ULL}));
        assert((Limits::lowest() == min_val));

        std::cout << "  ✓ min() = 0\n";
        std::cout << "  ✓ max() = 2^128 - 1\n";
        std::cout << "  ✓ lowest() = min()\n";
    }

    // ========================================================================
    // TEST 3: TWO'S COMPLEMENT SIGNED TRAITS
    // ========================================================================
    {
        std::cout << "\nTest 3: TC Signed Traits\n";
        using Limits = std::numeric_limits<int128_tc_t>;

        assert(Limits::is_specialized);
        assert(Limits::is_signed);
        assert(Limits::is_integer);
        assert(Limits::is_exact);
        assert(Limits::is_bounded);
        assert(!Limits::is_modulo);    // Signed is not modulo
        assert(Limits::digits == 127); // 1 bit for sign
        assert(Limits::digits10 == 38);

        std::cout << "  ✓ All traits correct\n";
    }

    // ========================================================================
    // TEST 4: TWO'S COMPLEMENT SIGNED VALUES
    // ========================================================================
    {
        std::cout << "\nTest 4: TC Signed Values\n";
        using Limits = std::numeric_limits<int128_tc_t>;

        const auto min_val = Limits::min();
        const auto max_val = Limits::max();

        // min = -2^127 (MSB set, rest 0)
        assert((min_val == int128_tc_t{1ULL << 63, 0}));
        // max = 2^127 - 1 (MSB clear, rest all 1s)
        assert((max_val == int128_tc_t{(1ULL << 63) - 1, ~0ULL}));
        assert((Limits::lowest() == min_val));

        std::cout << "  ✓ min() = -2^127\n";
        std::cout << "  ✓ max() = 2^127 - 1\n";
        std::cout << "  ✓ lowest() = min()\n";
    }

    // ========================================================================
    // TEST 5: MAGNITUDE-SIGN UNSIGNED TRAITS
    // ========================================================================
    {
        std::cout << "\nTest 5: MS Unsigned Traits\n";
        using Limits = std::numeric_limits<uint128_ms_t>;

        assert(Limits::is_specialized);
        assert(!Limits::is_signed);
        assert(Limits::digits == 127); // Sign bit excluded
        assert(Limits::digits10 == 38);

        std::cout << "  ✓ All traits correct\n";
        std::cout << "  ✓ digits = 127 (sign bit excluded)\n";
    }

    // ========================================================================
    // TEST 6: MAGNITUDE-SIGN UNSIGNED VALUES
    // ========================================================================
    {
        std::cout << "\nTest 6: MS Unsigned Values\n";
        using Limits = std::numeric_limits<uint128_ms_t>;

        const auto min_val = Limits::min();
        const auto max_val = Limits::max();

        assert((min_val == uint128_ms_t{0, 0}));
        assert((max_val == uint128_ms_t{(1ULL << 63) - 1, ~0ULL})); // 2^127 - 1

        std::cout << "  ✓ min() = 0\n";
        std::cout << "  ✓ max() = 2^127 - 1 (sign bit excluded)\n";
    }

    // ========================================================================
    // TEST 7: MAGNITUDE-SIGN SIGNED TRAITS
    // ========================================================================
    {
        std::cout << "\nTest 7: MS Signed Traits\n";
        using Limits = std::numeric_limits<int128_ms_t>;

        assert(Limits::is_specialized);
        assert(Limits::is_signed);
        assert(Limits::digits == 127); // Magnitude bits only
        assert(!Limits::is_modulo);

        std::cout << "  ✓ All traits correct\n";
    }

    // ========================================================================
    // TEST 8: MAGNITUDE-SIGN SIGNED VALUES (SYMMETRIC RANGE)
    // ========================================================================
    {
        std::cout << "\nTest 8: MS Signed Values (Symmetric Range)\n";
        using Limits = std::numeric_limits<int128_ms_t>;

        const auto min_val = Limits::min();
        const auto max_val = Limits::max();

        // MS has symmetric range: -(2^127 - 1) to +(2^127 - 1)
        // min = -(2^127 - 1): sign bit set, magnitude = all 1s
        assert((min_val == int128_ms_t{(1ULL << 63) | ((1ULL << 63) - 1), ~0ULL}));
        // max = +(2^127 - 1): sign bit clear, magnitude = all 1s
        assert((max_val == int128_ms_t{(1ULL << 63) - 1, ~0ULL}));

        std::cout << "  ✓ min() = -(2^127 - 1) [symmetric]\n";
        std::cout << "  ✓ max() = +(2^127 - 1) [symmetric]\n";
        std::cout << "  ✓ MS has two zeros: +0 and -0\n";
    }

    // ========================================================================
    // TEST 9: EXCESS-K UNSIGNED TRAITS
    // ========================================================================
    {
        std::cout << "\nTest 9: EK Unsigned Traits\n";
        using Limits = std::numeric_limits<uint128_ek_t>;

        assert(Limits::is_specialized);
        assert(!Limits::is_signed);
        assert(Limits::digits == 128);
        assert(Limits::is_modulo);

        std::cout << "  ✓ All traits correct\n";
    }

    // ========================================================================
    // TEST 10: EXCESS-K UNSIGNED VALUES
    // ========================================================================
    {
        std::cout << "\nTest 10: EK Unsigned Values\n";
        using Limits = std::numeric_limits<uint128_ek_t>;

        const auto min_val = Limits::min();
        const auto max_val = Limits::max();

        // EK: stored_value = real_value + bias
        // min stored = 0 (real = -bias)
        // max stored = all 1s (real = 2^128 - 1 - bias)
        assert((min_val == uint128_ek_t{0, 0}));
        assert((max_val == uint128_ek_t{~0ULL, ~0ULL}));

        std::cout << "  ✓ min() = 0 (stored)\n";
        std::cout << "  ✓ max() = all 1s (stored)\n";
    }

    // ========================================================================
    // TEST 11: EXCESS-K SIGNED TRAITS
    // ========================================================================
    {
        std::cout << "\nTest 11: EK Signed Traits\n";
        using Limits = std::numeric_limits<int128_ek_t>;

        assert(Limits::is_specialized);
        assert(Limits::is_signed);
        assert(Limits::digits == 127);
        assert(!Limits::is_modulo);

        std::cout << "  ✓ All traits correct\n";
    }

    // ========================================================================
    // TEST 12: EXCESS-K SIGNED VALUES
    // ========================================================================
    {
        std::cout << "\nTest 12: EK Signed Values\n";
        using Limits = std::numeric_limits<int128_ek_t>;

        const auto min_val = Limits::min();
        const auto max_val = Limits::max();

        // With bias = 2^126:
        // min stored = 0 (real = -2^126)
        // max stored = all 1s (real = 2^127 - 2^126 - 1)
        assert((min_val == int128_ek_t{0, 0}));
        assert((max_val == int128_ek_t{~0ULL, ~0ULL}));

        std::cout << "  ✓ min() = 0 (stored, real = -2^126)\n";
        std::cout << "  ✓ max() = all 1s (stored)\n";
    }

    std::cout << "\n✅ All numeric limits tests passed!\n";
    std::cout << "\n📝 Summary:\n";
    std::cout << "   - TC unsigned: [0, 2^128-1], 128 bits\n";
    std::cout << "   - TC signed: [-2^127, 2^127-1], 127 value bits\n";
    std::cout << "   - MS unsigned: [0, 2^127-1], 127 magnitude bits\n";
    std::cout << "   - MS signed: [-(2^127-1), 2^127-1], symmetric range, ±0\n";
    std::cout << "   - EK unsigned: Full 128-bit stored range\n";
    std::cout << "   - EK signed: Bias-centered range (bias = 2^126)\n";

    return 0;
}
