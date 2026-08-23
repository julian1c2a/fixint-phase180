// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Showcase: All Representation Forms
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Demonstrates the parameterized nature of the library: how the same logical
// value is stored differently in each representation form (TC, MS, EK, binnat).
//

#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>

using nstd::int128_ek_t;
using nstd::int128_ms_t;
using nstd::int128_tc_t;
using nstd::uint128_t;

void print_raw_bits(const std::string &label, uint64_t high, uint64_t low)
{
    std::cout << "  " << std::left << std::setw(18) << label << " high = 0x" << std::hex << std::setfill('0')
              << std::setw(16) << high << "  low = 0x" << std::setw(16) << low << std::dec
              << std::setfill(' ') << std::endl;
}

int main()
{
    std::cout << "=== Showcase: Representation Forms ===" << std::endl;
    std::cout << std::endl;

    // --- Positive value: 42 ---
    std::cout << "--- Value: +42 ---" << std::endl;

    const int128_tc_t tc_42(42LL);
    const int128_ms_t ms_42(42LL);
    const int128_ek_t ek_42(42LL);
    const uint128_t bn_42(42ULL);

    print_raw_bits("Two's Complement:", tc_42.high(), tc_42.low());
    print_raw_bits("Magnitude-Sign:", ms_42.high(), ms_42.low());
    print_raw_bits("Excess-K:", ek_42.high(), ek_42.low());
    print_raw_bits("Binary Natural:", bn_42.high(), bn_42.low());
    std::cout << std::endl;

    // --- Negative value: -42 ---
    std::cout << "--- Value: -42 ---" << std::endl;

    const int128_tc_t tc_neg(-42LL);
    const int128_ms_t ms_neg(-42LL);
    const int128_ek_t ek_neg(-42LL);
    // uint128_t cannot represent -42 (unsigned)

    print_raw_bits("Two's Complement:", tc_neg.high(), tc_neg.low());
    print_raw_bits("Magnitude-Sign:", ms_neg.high(), ms_neg.low());
    print_raw_bits("Excess-K:", ek_neg.high(), ek_neg.low());
    std::cout << "  Binary Natural:  (unsigned - cannot represent -42)" << std::endl;
    std::cout << std::endl;

    // --- Explain the differences ---
    std::cout << "--- How each form stores -42 ---" << std::endl;
    std::cout << std::endl;

    std::cout << "  Two's Complement : Bitwise NOT of 42, plus 1" << std::endl;
    std::cout << "    All F's in high = sign extension of negative" << std::endl;
    std::cout << std::endl;

    std::cout << "  Magnitude-Sign   : Sign bit in MSB of high word" << std::endl;
    std::cout << "    high[63] = 1 means negative, magnitude = 42" << std::endl;
    std::cout << std::endl;

    std::cout << "  Excess-K         : Value stored as value + K (bias)" << std::endl;
    std::cout << "    K = 2^127, so -42 stored as 2^127 - 42" << std::endl;
    std::cout << std::endl;

    // --- Zero handling ---
    std::cout << "--- Zero in each form ---" << std::endl;

    const int128_tc_t tc_zero(0LL);
    const int128_ms_t ms_zero(0LL);
    const int128_ek_t ek_zero(0LL);
    const uint128_t bn_zero(0ULL);

    print_raw_bits("TC zero:", tc_zero.high(), tc_zero.low());
    print_raw_bits("MS zero:", ms_zero.high(), ms_zero.low());
    print_raw_bits("EK zero:", ek_zero.high(), ek_zero.low());
    print_raw_bits("BN zero:", bn_zero.high(), bn_zero.low());
    std::cout << std::endl;

    std::cout << "Note: EK stores 0 with bias (high = 0x8000000000000000)" << std::endl;
    std::cout << "      MS has both +0 and -0 (distinct bit patterns)" << std::endl;
    std::cout << std::endl;

    // --- Properties ---
    std::cout << "--- Type Properties ---" << std::endl;
    std::cout << "  TC is_twos_complement:  " << (int128_tc_t::is_twos_complement ? "true" : "false")
              << std::endl;
    std::cout << "  MS is_magnitude_sign:   " << (int128_ms_t::is_magnitude_sign ? "true" : "false")
              << std::endl;
    std::cout << "  EK is_excess_k:         " << (int128_ek_t::is_excess_k ? "true" : "false") << std::endl;
    std::cout << "  BN is_binnat:           " << (uint128_t::is_binnat ? "true" : "false") << std::endl;
    std::cout << std::endl;

    std::cout << "[OK] Showcase complete." << std::endl;
    return 0;
}
