// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Tutorial 02: Signed Types and Representations
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Demonstrates: signed int128, different representation forms
// (Two's Complement, Magnitude-Sign, Excess-K), and their behavior.
//

#include "int128_parameterized.hpp"
#include <iostream>

using nstd::int128_ek_t; // Excess-K
using nstd::int128_ms_t; // Magnitude-Sign
using nstd::int128_t;    // Two's Complement (default signed)
using nstd::int128_tc_t; // Two's Complement (explicit alias)
using nstd::uint128_t;

int main()
{
    std::cout << "=== Tutorial 02: Signed Types & Representations ===" << std::endl;
    std::cout << std::endl;

    // --- Two's Complement (TC) ---
    std::cout << "--- Two's Complement ---" << std::endl;

    const int128_tc_t tc_pos(42LL);
    const int128_tc_t tc_neg(-42LL);
    const int128_tc_t tc_zero(0LL);

    std::cout << " 42 : to_string = " << tc_pos.to_string()
              << ", is_negative = " << (tc_pos.is_negative() ? "true" : "false") << std::endl;
    std::cout << "-42 : to_string = " << tc_neg.to_string()
              << ", is_negative = " << (tc_neg.is_negative() ? "true" : "false") << std::endl;
    std::cout << "  0 : to_string = " << tc_zero.to_string()
              << ", is_zero = " << (tc_zero.is_zero() ? "true" : "false") << std::endl;

    // Negation
    const int128_tc_t tc_negated = -tc_pos;
    std::cout << "-(42) = " << tc_negated.to_string() << std::endl;
    std::cout << std::endl;

    // --- Magnitude-Sign (MS) ---
    std::cout << "--- Magnitude-Sign ---" << std::endl;

    const int128_ms_t ms_pos(42LL);
    const int128_ms_t ms_neg(-42LL);

    std::cout << " 42 : high = 0x" << std::hex << ms_pos.high() << ", low = 0x" << ms_pos.low() << std::dec
              << std::endl;
    std::cout << "-42 : high = 0x" << std::hex << ms_neg.high() << ", low = 0x" << ms_neg.low() << std::dec
              << std::endl;
    std::cout << " 42 : is_negative = " << (ms_pos.is_negative() ? "true" : "false") << std::endl;
    std::cout << "-42 : is_negative = " << (ms_neg.is_negative() ? "true" : "false") << std::endl;

    // MS has +0 and -0
    const int128_ms_t ms_pzero(0LL);
    std::cout << "+0 : is_zero = " << (ms_pzero.is_zero() ? "true" : "false")
              << ", is_positive_zero = " << (ms_pzero.is_positive_zero() ? "true" : "false") << std::endl;
    std::cout << std::endl;

    // --- Excess-K (EK) ---
    std::cout << "--- Excess-K ---" << std::endl;

    const int128_ek_t ek_pos(42LL);
    const int128_ek_t ek_neg(-42LL);
    const int128_ek_t ek_zero(0LL);

    std::cout << " 42 : is_negative = " << (ek_pos.is_negative() ? "true" : "false") << std::endl;
    std::cout << "-42 : is_negative = " << (ek_neg.is_negative() ? "true" : "false") << std::endl;
    std::cout << "  0 : is_zero = " << (ek_zero.is_zero() ? "true" : "false") << std::endl;
    std::cout << std::endl;

    // --- Type properties ---
    std::cout << "--- Type Properties ---" << std::endl;

    std::cout << "uint128_t : is_signed = " << (uint128_t::is_signed ? "true" : "false") << std::endl;
    std::cout << "int128_tc_t : is_signed = " << (int128_tc_t::is_signed ? "true" : "false") << std::endl;
    std::cout << "int128_ms_t : is_signed = " << (int128_ms_t::is_signed ? "true" : "false") << std::endl;
    std::cout << "int128_ek_t : is_signed = " << (int128_ek_t::is_signed ? "true" : "false") << std::endl;
    std::cout << std::endl;

    std::cout << "[OK] Tutorial 02 complete." << std::endl;
    return 0;
}
