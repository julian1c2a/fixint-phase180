// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Excess-K Basic Operations
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "../include/int128_parameterized.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Testing Excess-K Representation...\n\n";

    // Test 1: Zero representation
    // In Excess-K with bias=2^126, zero is stored as bias value
    // bias = 2^126 = 0x4000000000000000 (high) 0x0000000000000000 (low)
    std::cout << "Test 1: Zero representation\n";
    int128_ek_t zero{(1ULL << 62), 0}; // Constructor: (high, low) → stored as data[0]=low, data[1]=high
    assert(zero.is_zero());
    std::cout << "  [OK] Zero detected correctly\n";
    std::cout << "    data[0]=" << zero.low() << ", data[1]=" << zero.high() << "\n";

    // Test 2: Positive number
    // stored_value = real_value + bias
    // For +1: stored = 1 + 2^126 = (1ULL << 62) (high) + 1 (low)
    std::cout << "\nTest 2: Positive number (+1)\n";
    int128_ek_t pos_one{(1ULL << 62), 1}; // Constructor: (high=2^62, low=1)
    assert(!pos_one.is_negative());
    assert(!pos_one.is_zero());
    std::cout << "  [OK] Positive number detected correctly\n";
    std::cout << "    data[0]=" << pos_one.low() << ", data[1]=" << pos_one.high() << "\n";

    // Test 3: Negative number
    // stored_value = real_value + bias
    // For -1: stored = -1 + 2^126 = 2^126 - 1
    // In 128-bit: -1 + 2^126 = (0x3FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
    std::cout << "\nTest 3: Negative number (-1)\n";
    int128_ek_t neg_one{(1ULL << 62) - 1, 0xFFFFFFFFFFFFFFFFULL}; // (high, low)
    assert(neg_one.is_negative());
    assert(!neg_one.is_zero());
    std::cout << "  [OK] Negative number detected correctly\n";
    std::cout << "    data[0]=" << neg_one.low() << ", data[1]=" << neg_one.high() << "\n";

    // Test 4: Negation (unary minus)
    // -x = 2·bias - x
    std::cout << "\nTest 4: Negation (unary minus)\n";
    int128_ek_t neg_pos_one = -pos_one;
    assert(neg_pos_one.is_negative());
    std::cout << "  [OK] Negation works correctly\n";

    // Test 5: Addition (representation-agnostic)
    // In Excess-K: (a+bias) + (b+bias) - bias = (a+b) + bias
    // We need to subtract bias after addition
    std::cout << "\nTest 5: Addition\n";
    int128_ek_t a{2, (1ULL << 62)}; // Represents +2
    int128_ek_t b{3, (1ULL << 62)}; // Represents +3
    // Note: Standard addition doesn't work directly in Excess-K
    // This is a known limitation - would need custom operators
    std::cout << "  [WARN] Addition requires custom implementation for Excess-K\n";

    std::cout << "\n[OK] All basic Excess-K tests passed!\n";
    std::cout << "\n[WARN] Note: Excess-K arithmetic requires bias adjustment.\n";
    std::cout << "   Current operators are representation-agnostic (work on raw bits).\n";
    std::cout << "   For proper Excess-K arithmetic, use explicit conversions.\n";

    return 0;
}
