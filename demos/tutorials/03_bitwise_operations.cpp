// =============================================================================
// Tutorial 03: Bitwise Operations
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Demonstrates: bit operations, shifts, rotations, popcount, leading/trailing
// zeros, and bitwise logical operators.
//

#include "int128_parameterized.hpp"
#include <iostream>

using nstd::uint128_t;

int main()
{
    std::cout << "=== Tutorial 03: Bitwise Operations ===" << std::endl;
    std::cout << std::endl;

    // --- Bitwise logic ---
    std::cout << "--- Bitwise Logic ---" << std::endl;

    const uint128_t a(0xF0F0F0F0ULL);
    const uint128_t b(0xFF00FF00ULL);

    std::cout << "a         = 0x" << a.to_string(16) << std::endl;
    std::cout << "b         = 0x" << b.to_string(16) << std::endl;
    std::cout << "a & b     = 0x" << (a & b).to_string(16) << std::endl;
    std::cout << "a | b     = 0x" << (a | b).to_string(16) << std::endl;
    std::cout << "a ^ b     = 0x" << (a ^ b).to_string(16) << std::endl;
    std::cout << "~a (low)  = 0x" << std::hex << (~a).low() << std::dec << std::endl;
    std::cout << std::endl;

    // --- Shifts ---
    std::cout << "--- Shifts ---" << std::endl;

    const uint128_t one(1ULL);
    const uint128_t shifted_left = one << 64;           // 2^64
    const uint128_t shifted_right = shifted_left >> 32; // 2^32

    std::cout << "1 << 64  = " << shifted_left.to_string() << std::endl;
    std::cout << "  high   = " << shifted_left.high() << ", low = " << shifted_left.low() << std::endl;
    std::cout << "(1<<64) >> 32 = " << shifted_right.to_string() << std::endl;
    std::cout << std::endl;

    // --- Bit counting ---
    std::cout << "--- Bit Counting ---" << std::endl;

    const uint128_t val(0b11110000ULL);
    std::cout << "val = " << val.to_string(2) << " (binary)" << std::endl;
    std::cout << "popcount       = " << val.popcount() << std::endl;
    std::cout << "trailing_zeros = " << val.trailing_zeros() << std::endl;
    std::cout << "leading_zeros  = " << val.leading_zeros() << std::endl;
    std::cout << "bit_width      = " << val.bit_width() << std::endl;
    std::cout << "is_power_of_2  = " << (val.is_power_of_2() ? "true" : "false") << std::endl;
    std::cout << std::endl;

    // Power of 2 check
    const uint128_t pow2(256ULL); // 2^8
    std::cout << "256 is_power_of_2 = " << (pow2.is_power_of_2() ? "true" : "false") << std::endl;
    std::cout << "256 bit_width     = " << pow2.bit_width() << std::endl;
    std::cout << std::endl;

    // --- Rotation ---
    std::cout << "--- Rotation ---" << std::endl;

    const uint128_t rot_val(0xABCD'0000'0000'0000ULL);
    const uint128_t rotated = rot_val.rotate_left(8);

    std::cout << "Original (hex) = " << rot_val.to_string(16) << std::endl;
    std::cout << "rotl(8)  (hex) = " << rotated.to_string(16) << std::endl;
    std::cout << std::endl;

    std::cout << "[OK] Tutorial 03 complete." << std::endl;
    return 0;
}
