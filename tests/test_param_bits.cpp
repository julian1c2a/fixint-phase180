// =============================================================================
// Test: int128_param_bits.hpp - Bit Manipulation Functions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "../include/int128_param_bits.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Testing int128_param_bits.hpp (Representation-Aware)...\n\n";

    // ========================================================================
    // Test 1: popcount (Population Count)
    // ========================================================================
    std::cout << "Test 1: popcount()\n";

    // TC: All 128 bits
    uint128_tc_t tc_val{0xFF00FF00FF00FF00ULL, 0x00FF00FF00FF00FFULL};
    const int tc_count = popcount(tc_val);
    assert(tc_count == 64); // 32 bits in each word
    std::cout << "  ✓ TC popcount: " << tc_count << " (expected 64)\n";

    // MS: Only 127 bits (excludes sign bit)
    int128_ms_t ms_val{0xFF00FF00FF00FF00ULL, 0x00FF00FF00FF00FFULL}; // Sign bit = 1
    const int ms_count = popcount(ms_val);
    assert(ms_count == 63); // 32 in low + 31 in high (excludes MSB)
    std::cout << "  ✓ MS popcount: " << ms_count << " (expected 63, excludes sign)\n";

    // EK: All 128 bits of stored value
    int128_ek_t ek_val{0xFF00FF00FF00FF00ULL, 0x00FF00FF00FF00FFULL};
    const int ek_count = popcount(ek_val);
    assert(ek_count == 64);
    std::cout << "  ✓ EK popcount: " << ek_count << " (stored value)\n";

    // ========================================================================
    // Test 2: countl_zero (Leading Zeros)
    // ========================================================================
    std::cout << "\nTest 2: countl_zero()\n";

    // TC: 128-bit space
    uint128_tc_t tc_lz{0, 0xFF}; // 64 zeros in high, 56 zeros in low
    assert(countl_zero(tc_lz) == 120);
    std::cout << "  ✓ TC countl_zero: 120\n";

    // MS: 127-bit magnitude space
    int128_ms_t ms_lz{0, 0xFF};        // 63 zeros in high mag, 56 zeros in low
    assert(countl_zero(ms_lz) == 119); // 63 + 56 = 119 (one less due to 127-bit)
    std::cout << "  ✓ MS countl_zero: 119 (127-bit magnitude)\n";

    // EK: 128-bit stored value
    int128_ek_t ek_lz{0, 0xFF};
    assert(countl_zero(ek_lz) == 120);
    std::cout << "  ✓ EK countl_zero: 120\n";

    // ========================================================================
    // Test 3: countr_zero (Trailing Zeros)
    // ========================================================================
    std::cout << "\nTest 3: countr_zero()\n";

    // TC: 128-bit space
    uint128_tc_t tc_tz{0xFF00, 0};    // 64 zeros in low, some in high
    assert(countr_zero(tc_tz) == 72); // 64 + 8
    std::cout << "  ✓ TC countr_zero: 72\n";

    // MS: 127-bit magnitude space
    int128_ms_t ms_tz{0xFF00, 0};
    assert(countr_zero(ms_tz) == 72);
    std::cout << "  ✓ MS countr_zero: 72\n";

    // EK: 128-bit stored value
    int128_ek_t ek_tz{0xFF00, 0};
    assert(countr_zero(ek_tz) == 72);
    std::cout << "  ✓ EK countr_zero: 72\n";

    // ========================================================================
    // Test 4: bit_width (Highest Bit Position)
    // ========================================================================
    std::cout << "\nTest 4: bit_width()\n";

    // TC: Value 255 (0xFF) has bit width 8
    uint128_tc_t tc_bw{0, 0xFF};
    assert(bit_width(tc_bw) == 8);
    std::cout << "  ✓ TC bit_width(0xFF): 8\n";

    // MS: Same value, 127-bit space
    int128_ms_t ms_bw{0, 0xFF};
    assert(bit_width(ms_bw) == 8);
    std::cout << "  ✓ MS bit_width(0xFF): 8\n";

    // EK: Stored value
    int128_ek_t ek_bw{0, 0xFF};
    assert(bit_width(ek_bw) == 8);
    std::cout << "  ✓ EK bit_width(0xFF): 8\n";

    // ========================================================================
    // Test 5: is_power_of_2
    // ========================================================================
    std::cout << "\nTest 5: is_power_of_2()\n";

    // TC: 256 (0x100) is a power of 2
    uint128_tc_t tc_pow2{0, 0x100};
    assert(is_power_of_2(tc_pow2));
    std::cout << "  ✓ TC is_power_of_2(256): true\n";

    // TC: 255 (0xFF) is not a power of 2
    uint128_tc_t tc_not_pow2{0, 0xFF};
    assert(!is_power_of_2(tc_not_pow2));
    std::cout << "  ✓ TC is_power_of_2(255): false\n";

    // MS: Checks magnitude
    int128_ms_t ms_pow2{0, 0x100};
    assert(is_power_of_2(ms_pow2));
    std::cout << "  ✓ MS is_power_of_2(256): true\n";

    // ========================================================================
    // Test 6: rotl (Rotate Left)
    // ========================================================================
    std::cout << "\nTest 6: rotl()\n";

    // TC: Rotate 0x0F by 4 positions
    uint128_tc_t tc_rot{0, 0x0F};
    const auto tc_rotated = rotl(tc_rot, 4);
    assert(tc_rotated.low() == 0xF0);
    std::cout << "  ✓ TC rotl(0x0F, 4): 0xF0\n";

    // MS: Rotate magnitude, preserve sign
    int128_ms_t ms_rot{0, 0x0F}; // Positive
    const auto ms_rotated = rotl(ms_rot, 4);
    assert(!ms_rotated.is_negative()); // Sign preserved
    std::cout << "  ✓ MS rotl preserves sign bit\n";

    // ========================================================================
    // Test 7: rotr (Rotate Right)
    // ========================================================================
    std::cout << "\nTest 7: rotr()\n";

    // TC: Rotate 0xF0 by 4 positions
    uint128_tc_t tc_rotr{0, 0xF0};
    const auto tc_rotr_result = rotr(tc_rotr, 4);
    assert(tc_rotr_result.low() == 0x0F);
    std::cout << "  ✓ TC rotr(0xF0, 4): 0x0F\n";

    // ========================================================================
    // Test 8: Zero values
    // ========================================================================
    std::cout << "\nTest 8: Zero values\n";

    uint128_tc_t zero_tc{0, 0};
    assert(countl_zero(zero_tc) == 128);
    assert(countr_zero(zero_tc) == 128);
    assert(bit_width(zero_tc) == 0);
    assert(!is_power_of_2(zero_tc));
    std::cout << "  ✓ TC zero: all tests pass\n";

    int128_ms_t zero_ms{0, 0};
    assert(countl_zero(zero_ms) == 127); // 127-bit magnitude space
    assert(countr_zero(zero_ms) == 127);
    assert(bit_width(zero_ms) == 0);
    std::cout << "  ✓ MS zero: all tests pass (127-bit space)\n";

    std::cout << "\n✅ All bit manipulation tests passed!\n";
    std::cout << "\n📝 Summary:\n";
    std::cout << "   - popcount: TC/EK=128 bits, MS=127 bits (excludes sign)\n";
    std::cout << "   - countl_zero/countr_zero: Representation-aware\n";
    std::cout << "   - bit_width: TC/EK=128-bit space, MS=127-bit space\n";
    std::cout << "   - is_power_of_2: MS checks magnitude only\n";
    std::cout << "   - rotl/rotr: MS preserves sign bit\n";

    return 0;
}
