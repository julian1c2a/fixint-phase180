// =============================================================================
// Test: Excess-K Comparison Operators
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "../include/int128_parameterized.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Testing Excess-K Comparison Operators...\n\n";

    // Test values (stored values shown in comments)
    // Zero: stored = bias = 2^126
    int128_ek_t zero{(1ULL << 62), 0};

    // Positive values: stored > bias
    int128_ek_t pos_one{(1ULL << 62), 1};   // stored = bias + 1
    int128_ek_t pos_ten{(1ULL << 62), 10};  // stored = bias + 10
    int128_ek_t pos_hun{(1ULL << 62), 100}; // stored = bias + 100

    // Negative values: stored < bias
    // -1 = bias - 1 = (2^126 - 1) in 128-bit = (0x3FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
    int128_ek_t neg_one{(1ULL << 62) - 1, 0xFFFFFFFFFFFFFFFFULL};

    // -10 = bias - 10
    int128_ek_t neg_ten{(1ULL << 62) - 1, 0xFFFFFFFFFFFFFFF6ULL}; // -10 in low word

    // Test 1: Equality operator (==)
    std::cout << "Test 1: Equality (==)\n";
    int128_ek_t another_zero{(1ULL << 62), 0};
    assert(zero == another_zero);
    assert(pos_one == pos_one);
    assert(!(pos_one == pos_ten));
    std::cout << "  [OK] Equality works correctly\n";

    // Test 2: Inequality operator (!=)
    std::cout << "\nTest 2: Inequality (!=)\n";
    assert(zero != pos_one);
    assert(pos_one != pos_ten);
    assert(neg_one != zero);
    assert(!(zero != another_zero));
    std::cout << "  [OK] Inequality works correctly\n";

    // Test 3: Less than (<)
    std::cout << "\nTest 3: Less than (<)\n";
    assert(neg_one < zero);       // -1 < 0
    assert(zero < pos_one);       // 0 < 1
    assert(pos_one < pos_ten);    // 1 < 10
    assert(neg_ten < neg_one);    // -10 < -1
    assert(!(pos_one < pos_one)); // !(1 < 1)
    std::cout << "  [OK] Less than works correctly\n";

    // Test 4: Less than or equal (<=)
    std::cout << "\nTest 4: Less than or equal (<=)\n";
    assert(neg_one <= zero);
    assert(zero <= pos_one);
    assert(pos_one <= pos_one); // Equal case
    assert(pos_one <= pos_ten);
    assert(!(pos_ten <= pos_one));
    std::cout << "  [OK] Less than or equal works correctly\n";

    // Test 5: Greater than (>)
    std::cout << "\nTest 5: Greater than (>)\n";
    assert(pos_one > zero);       // 1 > 0
    assert(zero > neg_one);       // 0 > -1
    assert(pos_ten > pos_one);    // 10 > 1
    assert(neg_one > neg_ten);    // -1 > -10
    assert(!(pos_one > pos_one)); // !(1 > 1)
    std::cout << "  [OK] Greater than works correctly\n";

    // Test 6: Greater than or equal (>=)
    std::cout << "\nTest 6: Greater than or equal (>=)\n";
    assert(pos_one >= zero);
    assert(zero >= neg_one);
    assert(pos_one >= pos_one); // Equal case
    assert(pos_ten >= pos_one);
    assert(!(pos_one >= pos_ten));
    std::cout << "  [OK] Greater than or equal works correctly\n";

    // Test 7: Transitivity
    std::cout << "\nTest 7: Transitivity (a < b && b < c => a < c)\n";
    assert(neg_ten < neg_one);
    assert(neg_one < zero);
    assert(neg_ten < zero); // Transitivity

    assert(zero < pos_one);
    assert(pos_one < pos_ten);
    assert(zero < pos_ten); // Transitivity
    std::cout << "  [OK] Transitivity holds\n";

    // Test 8: Reflexivity (a == a)
    std::cout << "\nTest 8: Reflexivity (a == a)\n";
    assert(zero == zero);
    assert(pos_one == pos_one);
    assert(neg_one == neg_one);
    std::cout << "  [OK] Reflexivity holds\n";

    // Test 9: Antisymmetry (a <= b && b <= a => a == b)
    std::cout << "\nTest 9: Antisymmetry\n";
    int128_ek_t a{(1ULL << 62), 42};
    int128_ek_t b{(1ULL << 62), 42};
    assert(a <= b);
    assert(b <= a);
    assert(a == b);
    std::cout << "  [OK] Antisymmetry holds\n";

    // Test 10: Total ordering (a < b || a == b || a > b)
    std::cout << "\nTest 10: Total ordering\n";
    assert((pos_one < pos_ten) || (pos_one == pos_ten) || (pos_one > pos_ten));
    assert((zero < pos_one) || (zero == pos_one) || (zero > pos_one));
    assert((neg_one < zero) || (neg_one == zero) || (neg_one > zero));
    std::cout << "  [OK] Total ordering holds\n";

    std::cout << "\n[OK] All comparison operator tests passed!\n";
    std::cout << "\n📝 Note: Comparison operators work correctly for Excess-K\n";
    std::cout << "   because stored value ordering preserves real value ordering.\n";
    std::cout << "   (stored_a < stored_b) ⟺ (real_a < real_b)\n";

    return 0;
}
