// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_bits.hpp - Bit Manipulation Functions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_bits.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

// Test result macros
#define TEST_PASS() (++g_passed)
#define TEST_FAIL() (++g_failed)

int g_passed{0};
int g_failed{0};

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Bit Manipulation Tests (popcount, zeros, rotations)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] trailing_zeros() - Count trailing zero bits
    // ========================================================================
    {
        std::cout << "[Test 1] trailing_zeros():\n";

        const uint128_t val1{0, 8}; // 0x...0008 → 3 trailing zeros
        const uint128_t val2{1, 0}; // 0x1000...0000 → 64 trailing zeros
        const uint128_t val3{0, 1}; // 0x...0001 → 0 trailing zeros
        const uint128_t val4{0, 0}; // 0x0 → 128 (all zeros)

        if ((val1.trailing_zeros() == 3) &&
            (val2.trailing_zeros() == 64) &&
            (val3.trailing_zeros() == 0) &&
            (val4.trailing_zeros() == 128))
        {
            std::cout << "  [OK] trailing_zeros\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] trailing_zeros\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 2] leading_zeros() - Count leading zero bits
    // ========================================================================
    {
        std::cout << "[Test 2] leading_zeros():\n";

        const uint128_t val1{0, 1};          // 0x...0001 → 127 leading zeros
        const uint128_t val2{1, 0};          // 0x1000...0000 → 63 leading zeros
        const uint128_t val3{1ULL << 63, 0}; // 0x8000...0000 → 0 leading zeros
        const uint128_t val4{0, 0};          // 0x0 → 128 (all zeros)

        if ((val1.leading_zeros() == 127) &&
            (val2.leading_zeros() == 63) &&
            (val3.leading_zeros() == 0) &&
            (val4.leading_zeros() == 128))
        {
            std::cout << "  [OK] leading_zeros\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] leading_zeros\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 3] bit_width() - Position of highest set bit
    // ========================================================================
    {
        std::cout << "[Test 3] bit_width():\n";

        const uint128_t val1{0, 1};   // bit 0 → width = 1
        const uint128_t val2{0, 255}; // bit 7 → width = 8
        const uint128_t val3{1, 0};   // bit 64 → width = 65
        const uint128_t val4{0, 0};   // no bits → width = 0

        if ((val1.bit_width() == 1) &&
            (val2.bit_width() == 8) &&
            (val3.bit_width() == 65) &&
            (val4.bit_width() == 0))
        {
            std::cout << "  [OK] bit_width\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] bit_width\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 4] is_power_of_2() - Check if exactly one bit is set
    // ========================================================================
    {
        std::cout << "[Test 4] is_power_of_2():\n";

        const uint128_t pow1{0, 1};    // 2^0
        const uint128_t pow8{0, 256};  // 2^8
        const uint128_t pow64{1, 0};   // 2^64
        const uint128_t not_pow{0, 3}; // Not a power of 2
        const uint128_t zero{0, 0};    // Zero (not a power of 2)

        if (pow1.is_power_of_2() &&
            pow8.is_power_of_2() &&
            pow64.is_power_of_2() &&
            !not_pow.is_power_of_2() &&
            !zero.is_power_of_2())
        {
            std::cout << "  [OK] is_power_of_2\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] is_power_of_2\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 5] count_ones() / popcount() - Count bits set to 1
    // ========================================================================
    {
        std::cout << "[Test 5] count_ones() / popcount():\n";

        const uint128_t val1{0, 0};         // 0 ones
        const uint128_t val2{0, 1};         // 1 one
        const uint128_t val3{0, 0xFF};      // 8 ones
        const uint128_t val4{~0ULL, ~0ULL}; // 128 ones

        if ((val1.count_ones() == 0) &&
            (val2.count_ones() == 1) &&
            (val3.count_ones() == 8) &&
            (val4.count_ones() == 128) &&
            (val2.count_ones() == val2.popcount())) // Verify alias
        {
            std::cout << "  [OK] count_ones_popcount\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] count_ones_popcount\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 6] rotate_left() - Circular left shift
    // ========================================================================
    {
        std::cout << "[Test 6] rotate_left():\n";

        const uint128_t val{1ULL << 63, 0}; // MSB set
        const auto rotated1{val.rotate_left(1)};
        const auto rotated64{val.rotate_left(64)};
        const auto rotated128{val.rotate_left(128)};

        // After 1 left rotation, MSB moves to bit 0 of low part
        // After 64 rotations, MSB stays in same position (64-bit boundary)
        // After 128 rotations, returns to original position
        if ((rotated128 == val)) // Full rotation
        {
            std::cout << "  [OK] rotate_left\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] rotate_left\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 7] rotate_right() - Circular right shift
    // ========================================================================
    {
        std::cout << "[Test 7] rotate_right():\n";

        const uint128_t val{0, 1}; // LSB set
        const auto rotated1{val.rotate_right(1)};
        const auto rotated128{val.rotate_right(128)};

        // After 1 right rotation, LSB moves to MSB
        // After 128 rotations, returns to original position
        if ((rotated128 == val) &&
            (rotated1 == uint128_t{1ULL << 63, 0}))
        {
            std::cout << "  [OK] rotate_right\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] rotate_right\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 8] MS-specific: Magnitude operations (127-bit space)
    // ========================================================================
    {
        std::cout << "[Test 8] MS-specific operations:\n";

        const int128_ms_t ms_val1{0, 1};    // Magnitude = 1 → 126 leading zeros
        const int128_ms_t ms_val2{0, 0xFF}; // Magnitude = 0xFF → 119 leading zeros
        const int128_ms_t ms_val3{1, 0};    // Magnitude with bit 64 set → 62 leading zeros

        // MS operations work on 127-bit magnitude (sign bit excluded)
        if ((ms_val1.leading_zeros() == 126) && // 127-bit space: bit 0 set → 126 leading zeros
            (ms_val2.leading_zeros() == 119) && // bits 0-7 set → 119 leading zeros
            (ms_val3.leading_zeros() == 62))    // bit 64 set → 62 leading zeros
        {
            std::cout << "  [OK] ms_magnitude_operations\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] ms_magnitude_operations\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // RESULTS
    // ========================================================================
    std::cout << "====================================================================\n";
    std::cout << "RESULTS:\n";
    std::cout << "  Passed: " << g_passed << "\n";
    std::cout << "  Failed: " << g_failed << "\n";
    std::cout << "  Total:  " << (g_passed + g_failed) << "\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? 0 : 1;
}
