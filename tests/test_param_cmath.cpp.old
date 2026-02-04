// =============================================================================
// Test: int128_param_cmath.hpp - Mathematical Functions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "../include/int128_param_cmath.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Testing int128_param_cmath.hpp (Representation-Aware)...\n\n";

    // ========================================================================
    // Test 1: abs (Absolute Value)
    // ========================================================================
    std::cout << "Test 1: abs()\n";

    // TC: Standard negation
    int128_tc_t tc_neg{-42}; // Constructor handles sign extension
    const auto tc_abs_result = abs(tc_neg);
    const int128_tc_t tc_expected{42};
    assert((tc_abs_result == tc_expected));
    std::cout << "  ✓ TC abs(-42) = 42\n";

    // MS: Clear sign bit
    int128_ms_t ms_neg{1ULL << 63, 42}; // Negative with magnitude 42
    const auto ms_abs_result = abs(ms_neg);
    assert(!ms_abs_result.is_negative());
    std::cout << "  ✓ MS abs(negative) clears sign bit\n";

    // ========================================================================
    // Test 2: min/max
    // ========================================================================
    std::cout << "\nTest 2: min/max()\n";

    int128_tc_t tc_a{0, 100};
    int128_tc_t tc_b{0, 200};

    assert(min(tc_a, tc_b) == tc_a);
    assert(max(tc_a, tc_b) == tc_b);
    std::cout << "  ✓ TC min(100, 200) = 100\n";
    std::cout << "  ✓ TC max(100, 200) = 200\n";

    // MS: Uses comparison operators (representation-aware)
    int128_ms_t ms_a{0, 50};
    int128_ms_t ms_b{0, 75};
    assert(min(ms_a, ms_b) == ms_a);
    assert(max(ms_a, ms_b) == ms_b);
    std::cout << "  ✓ MS min/max work correctly\n";

    // ========================================================================
    // Test 3: clamp
    // ========================================================================
    std::cout << "\nTest 3: clamp()\n";

    int128_tc_t value{0, 150};
    int128_tc_t lo{0, 100};
    int128_tc_t hi{0, 200};

    assert(clamp(value, lo, hi) == value); // Within range

    int128_tc_t too_low{0, 50};
    assert(clamp(too_low, lo, hi) == lo);

    int128_tc_t too_high{0, 250};
    assert(clamp(too_high, lo, hi) == hi);

    std::cout << "  ✓ clamp(150, 100, 200) = 150\n";
    std::cout << "  ✓ clamp(50, 100, 200) = 100\n";
    std::cout << "  ✓ clamp(250, 100, 200) = 200\n";

    // ========================================================================
    // Test 4: gcd (Greatest Common Divisor)
    // ========================================================================
    std::cout << "\nTest 4: gcd()\n";

    // Test: gcd(48, 18) = 6
    int128_tc_t gcd_a{0, 48};
    int128_tc_t gcd_b{0, 18};
    const auto gcd_result = gcd(gcd_a, gcd_b);
    assert((gcd_result == int128_tc_t{0, 6}));
    std::cout << "  ✓ gcd(48, 18) = 6\n";

    // Test: gcd(100, 50) = 50
    int128_tc_t gcd_c{0, 100};
    int128_tc_t gcd_d{0, 50};
    const auto gcd_result2 = gcd(gcd_c, gcd_d);
    assert((gcd_result2 == int128_tc_t{0, 50}));
    std::cout << "  ✓ gcd(100, 50) = 50\n";

    // Test: gcd(17, 19) = 1 (coprime)
    int128_tc_t gcd_e{0, 17};
    int128_tc_t gcd_f{0, 19};
    const auto gcd_result3 = gcd(gcd_e, gcd_f);
    assert((gcd_result3 == int128_tc_t{0, 1}));
    std::cout << "  ✓ gcd(17, 19) = 1 (coprime)\n";

    // Test: gcd with zero
    int128_tc_t zero{0, 0};
    int128_tc_t nonzero{0, 42};
    assert(gcd(zero, nonzero) == nonzero);
    assert(gcd(nonzero, zero) == nonzero);
    std::cout << "  ✓ gcd(0, 42) = 42\n";

    // ========================================================================
    // Test 5: lcm (Least Common Multiple)
    // ========================================================================
    std::cout << "\nTest 5: lcm()\n";

    // Test: lcm(4, 6) = 12
    int128_tc_t lcm_a{0, 4};
    int128_tc_t lcm_b{0, 6};
    const auto lcm_result = lcm(lcm_a, lcm_b);
    assert((lcm_result == int128_tc_t{0, 12}));
    std::cout << "  ✓ lcm(4, 6) = 12\n";

    // Test: lcm(5, 7) = 35 (coprime)
    int128_tc_t lcm_c{0, 5};
    int128_tc_t lcm_d{0, 7};
    const auto lcm_result2 = lcm(lcm_c, lcm_d);
    assert((lcm_result2 == int128_tc_t{0, 35}));
    std::cout << "  ✓ lcm(5, 7) = 35\n";

    // Test: lcm with zero
    assert(lcm(zero, nonzero).is_zero());
    assert(lcm(nonzero, zero).is_zero());
    std::cout << "  ✓ lcm(0, 42) = 0\n";

    // ========================================================================
    // Test 6: midpoint (Overflow-Safe)
    // ========================================================================
    std::cout << "\nTest 6: midpoint()\n";

    // Test: midpoint(100, 200) = 150
    int128_tc_t mid_a{0, 100};
    int128_tc_t mid_b{0, 200};
    const auto mid_result = midpoint(mid_a, mid_b);
    assert((mid_result == int128_tc_t{0, 150}));
    std::cout << "  ✓ midpoint(100, 200) = 150\n";

    // Test: midpoint(0, 10) = 5
    int128_tc_t mid_c{0, 0};
    int128_tc_t mid_d{0, 10};
    const auto mid_result2 = midpoint(mid_c, mid_d);
    assert((mid_result2 == int128_tc_t{0, 5}));
    std::cout << "  ✓ midpoint(0, 10) = 5\n";

    // Test: midpoint with same values
    int128_tc_t mid_e{0, 42};
    const auto mid_result3 = midpoint(mid_e, mid_e);
    assert(mid_result3 == mid_e);
    std::cout << "  ✓ midpoint(42, 42) = 42\n";

    // ========================================================================
    // Test 7: pow (Integer Exponentiation)
    // ========================================================================
    std::cout << "\nTest 7: pow()\n";

    // Test: 2^10 = 1024
    int128_tc_t pow_base{0, 2};
    const auto pow_result = pow(pow_base, 10);
    assert((pow_result == int128_tc_t{0, 1024}));
    std::cout << "  ✓ pow(2, 10) = 1024\n";

    // Test: 3^4 = 81
    int128_tc_t pow_base2{0, 3};
    const auto pow_result2 = pow(pow_base2, 4);
    assert((pow_result2 == int128_tc_t{0, 81}));
    std::cout << "  ✓ pow(3, 4) = 81\n";

    // Test: x^0 = 1
    int128_tc_t pow_base3{0, 999};
    const auto pow_result3 = pow(pow_base3, 0);
    assert((pow_result3 == int128_tc_t{0, 1}));
    std::cout << "  ✓ pow(999, 0) = 1\n";

    // Test: x^1 = x
    int128_tc_t pow_base4{0, 42};
    const auto pow_result4 = pow(pow_base4, 1);
    assert(pow_result4 == pow_base4);
    std::cout << "  ✓ pow(42, 1) = 42\n";

    // Test: 10^3 = 1000
    int128_tc_t pow_base5{0, 10};
    const auto pow_result5 = pow(pow_base5, 3);
    assert((pow_result5 == int128_tc_t{0, 1000}));
    std::cout << "  ✓ pow(10, 3) = 1000\n";

    // ========================================================================
    // Test 8: Mixed-type operations
    // ========================================================================
    std::cout << "\nTest 8: Mixed-type operations\n";

    // gcd with builtin int
    const auto gcd_mixed = gcd(gcd_a, 18);
    assert((gcd_mixed == int128_tc_t{0, 6}));
    std::cout << "  ✓ gcd(int128, int) works\n";

    // lcm with builtin int
    const auto lcm_mixed = lcm(lcm_a, 6);
    assert((lcm_mixed == int128_tc_t{0, 12}));
    std::cout << "  ✓ lcm(int128, int) works\n";

    std::cout << "\n✅ All mathematical function tests passed!\n";
    std::cout << "\n📝 Summary:\n";
    std::cout << "   - abs: Works for all representations (TC/MS/EK)\n";
    std::cout << "   - min/max/clamp: Use comparison operators (representation-aware)\n";
    std::cout << "   - gcd/lcm: Binary algorithm, works with absolute values\n";
    std::cout << "   - midpoint: Overflow-safe algorithm\n";
    std::cout << "   - pow: Exponentiation by squaring (O(log n))\n";
    std::cout << "\n⚠️  Note for Excess-K:\n";
    std::cout << "   gcd/lcm/pow operate on stored values (not real values).\n";
    std::cout << "   For semantic correctness, convert to TC first.\n";

    return 0;
}
