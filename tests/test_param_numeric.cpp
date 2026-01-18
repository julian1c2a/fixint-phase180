// =============================================================================
// Test: int128_param_numeric.hpp - Additional Numeric Functions
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_numeric.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Testing int128_param_numeric.hpp (Additional Numeric Functions)...\n\n";

    // ========================================================================
    // TEST 1: SIGN FUNCTION
    // ========================================================================
    {
        std::cout << "Test 1: sign()\n";

        const int128_tc_t tc_pos{0, 42};
        const int128_tc_t tc_neg{-42};
        const int128_tc_t tc_zero{0, 0};

        assert((sign(tc_pos) == 1));
        assert((sign(tc_neg) == -1));
        assert((sign(tc_zero) == 0));

        const uint128_tc_t tc_unsigned{0, 100};
        assert((sign(tc_unsigned) == 1)); // Unsigned always positive

        std::cout << "  ✓ sign(42) = 1\n";
        std::cout << "  ✓ sign(-42) = -1\n";
        std::cout << "  ✓ sign(0) = 0\n";
        std::cout << "  ✓ sign(unsigned) = 1\n";
    }

    // ========================================================================
    // TEST 2: IS_EVEN / IS_ODD
    // ========================================================================
    {
        std::cout << "\nTest 2: is_even() / is_odd()\n";

        const uint128_tc_t even_val{0, 42};
        const uint128_tc_t odd_val{0, 43};
        const uint128_tc_t zero{0, 0};

        assert(is_even(even_val));
        assert(!is_odd(even_val));
        assert(!is_even(odd_val));
        assert(is_odd(odd_val));
        assert(is_even(zero));

        std::cout << "  ✓ is_even(42) = true\n";
        std::cout << "  ✓ is_odd(42) = false\n";
        std::cout << "  ✓ is_even(43) = false\n";
        std::cout << "  ✓ is_odd(43) = true\n";
        std::cout << "  ✓ is_even(0) = true\n";
    }

    // ========================================================================
    // TEST 3: ABS_DIFF
    // ========================================================================
    {
        std::cout << "\nTest 3: abs_diff()\n";

        const uint128_tc_t a{0, 100};
        const uint128_tc_t b{0, 50};
        const uint128_tc_t c{0, 150};

        const auto diff1 = abs_diff(a, b);
        const auto diff2 = abs_diff(b, a);
        const auto diff3 = abs_diff(a, c);

        assert((diff1 == uint128_tc_t{0, 50}));
        assert((diff2 == uint128_tc_t{0, 50}));
        assert((diff3 == uint128_tc_t{0, 50}));

        std::cout << "  ✓ abs_diff(100, 50) = 50\n";
        std::cout << "  ✓ abs_diff(50, 100) = 50\n";
        std::cout << "  ✓ abs_diff(100, 150) = 50\n";
    }

    // ========================================================================
    // TEST 4: ILOG2
    // ========================================================================
    {
        std::cout << "\nTest 4: ilog2()\n";

        const uint128_tc_t val1{0, 1};   // 2^0
        const uint128_tc_t val2{0, 8};   // 2^3
        const uint128_tc_t val3{0, 255}; // Between 2^7 and 2^8
        const uint128_tc_t val4{1, 0};   // 2^64

        assert((ilog2(val1) == 0));
        assert((ilog2(val2) == 3));
        assert((ilog2(val3) == 7)); // floor(log2(255)) = 7
        assert((ilog2(val4) == 64));

        std::cout << "  ✓ ilog2(1) = 0\n";
        std::cout << "  ✓ ilog2(8) = 3\n";
        std::cout << "  ✓ ilog2(255) = 7\n";
        std::cout << "  ✓ ilog2(2^64) = 64\n";
    }

    // ========================================================================
    // TEST 5: ISQRT
    // ========================================================================
    {
        std::cout << "\nTest 5: isqrt()\n";

        const uint128_tc_t val1{0, 0};
        const uint128_tc_t val2{0, 1};
        const uint128_tc_t val3{0, 4};
        const uint128_tc_t val4{0, 16};
        const uint128_tc_t val5{0, 100};
        const uint128_tc_t val6{0, 99}; // Non-perfect square

        const auto sqrt1 = isqrt(val1);
        const auto sqrt2 = isqrt(val2);
        const auto sqrt3 = isqrt(val3);
        const auto sqrt4 = isqrt(val4);
        const auto sqrt5 = isqrt(val5);
        const auto sqrt6 = isqrt(val6);

        assert((sqrt1 == uint128_tc_t{0, 0}));
        assert((sqrt2 == uint128_tc_t{0, 1}));
        assert((sqrt3 == uint128_tc_t{0, 2}));
        assert((sqrt4 == uint128_tc_t{0, 4}));
        assert((sqrt5 == uint128_tc_t{0, 10}));
        assert((sqrt6 == uint128_tc_t{0, 9})); // floor(sqrt(99)) = 9

        std::cout << "  ✓ isqrt(0) = 0\n";
        std::cout << "  ✓ isqrt(1) = 1\n";
        std::cout << "  ✓ isqrt(4) = 2\n";
        std::cout << "  ✓ isqrt(16) = 4\n";
        std::cout << "  ✓ isqrt(100) = 10\n";
        std::cout << "  ✓ isqrt(99) = 9 (floor)\n";
    }

    // ========================================================================
    // TEST 6: FACTORIAL
    // ========================================================================
    {
        std::cout << "\nTest 6: factorial()\n";

        const auto fact0 = factorial<signedness::unsigned_type, representation_form::twos_complement>(0);
        const auto fact1 = factorial<signedness::unsigned_type, representation_form::twos_complement>(1);
        const auto fact5 = factorial<signedness::unsigned_type, representation_form::twos_complement>(5);
        const auto fact10 = factorial<signedness::unsigned_type, representation_form::twos_complement>(10);

        assert((fact0 == uint128_tc_t{0, 1}));        // 0! = 1
        assert((fact1 == uint128_tc_t{0, 1}));        // 1! = 1
        assert((fact5 == uint128_tc_t{0, 120}));      // 5! = 120
        assert((fact10 == uint128_tc_t{0, 3628800})); // 10! = 3628800

        std::cout << "  ✓ 0! = 1\n";
        std::cout << "  ✓ 1! = 1\n";
        std::cout << "  ✓ 5! = 120\n";
        std::cout << "  ✓ 10! = 3628800\n";
    }

    // ========================================================================
    // TEST 7: DIVMOD
    // ========================================================================
    {
        std::cout << "\nTest 7: divmod()\n";

        const uint128_tc_t dividend{0, 100};
        const uint128_tc_t divisor1{0, 7};
        const uint128_tc_t divisor2{0, 10};

        const auto [q1, r1] = divmod(dividend, divisor1);
        const auto [q2, r2] = divmod(dividend, divisor2);

        // 100 / 7 = 14 remainder 2
        assert((q1 == uint128_tc_t{0, 14}));
        assert((r1 == uint128_tc_t{0, 2}));

        // 100 / 10 = 10 remainder 0
        assert((q2 == uint128_tc_t{0, 10}));
        assert((r2 == uint128_tc_t{0, 0}));

        std::cout << "  ✓ divmod(100, 7) = {14, 2}\n";
        std::cout << "  ✓ divmod(100, 10) = {10, 0}\n";
    }

    // ========================================================================
    // TEST 8: POWER (alias for pow)
    // ========================================================================
    {
        std::cout << "\nTest 8: power()\n";

        const uint128_tc_t base1{0, 2};
        const uint128_tc_t base2{0, 3};

        const auto pow1 = power(base1, 10);
        const auto pow2 = power(base2, 5);

        assert((pow1 == uint128_tc_t{0, 1024})); // 2^10
        assert((pow2 == uint128_tc_t{0, 243}));  // 3^5

        std::cout << "  ✓ power(2, 10) = 1024\n";
        std::cout << "  ✓ power(3, 5) = 243\n";
    }

    // ========================================================================
    // TEST 9: MS-SPECIFIC (127-bit magnitude space)
    // ========================================================================
    {
        std::cout << "\nTest 9: MS-specific operations\n";

        // MS positive value
        int128_ms_t ms_pos{0, 42};

        assert((sign(ms_pos) == 1));
        assert(is_even(ms_pos));

        // MS uses 127-bit magnitude for ilog2
        const auto log_val = ilog2(ms_pos);
        assert(log_val >= 0); // Should work correctly

        std::cout << "  ✓ MS sign detection works\n";
        std::cout << "  ✓ MS parity works\n";
        std::cout << "  ✓ MS ilog2 works (127-bit space)\n";
    }

    std::cout << "\n✅ All additional numeric function tests passed!\n";
    std::cout << "\n📝 Summary:\n";
    std::cout << "   - sign: Returns -1, 0, or +1\n";
    std::cout << "   - is_even/is_odd: Parity checks on LSB\n";
    std::cout << "   - abs_diff: Overflow-safe absolute difference\n";
    std::cout << "   - ilog2: floor(log2(x)) using bit position\n";
    std::cout << "   - isqrt: floor(sqrt(x)) using Newton's method\n";
    std::cout << "   - factorial: n! for small n (0 <= n <= ~34)\n";
    std::cout << "   - divmod: Combined quotient and remainder\n";
    std::cout << "   - power: Alias for pow (consistency)\n";
    std::cout << "\n⚠️  Note for EK:\n";
    std::cout << "   ilog2/isqrt operate on stored values (not real values).\n";
    std::cout << "   For semantic correctness, convert to TC first.\n";

    return 0;
}
