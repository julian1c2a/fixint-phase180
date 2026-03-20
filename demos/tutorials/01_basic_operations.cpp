// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Tutorial 01: Basic Operations with int128
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Demonstrates: construction, arithmetic, comparison, and basic I/O
// with uint128_t and int128_t types.
//

#include "int128_parameterized.hpp"
#include <iostream>
#include <string>

using nstd::int128_t;
using nstd::uint128_t;

int main()
{
    std::cout << "=== Tutorial 01: Basic Operations ===" << std::endl;
    std::cout << std::endl;

    // --- Construction ---
    std::cout << "--- Construction ---" << std::endl;

    const uint128_t zero;                      // Default: 0
    const uint128_t from_int(42ULL);           // From integer
    const uint128_t from_pair(0x1ULL, 0x0ULL); // high=1, low=0 => 2^64
    const int128_t negative(-1LL);             // Signed, negative

    std::cout << "zero       = " << zero.to_string() << std::endl;
    std::cout << "from_int   = " << from_int.to_string() << std::endl;
    std::cout << "from_pair  = " << from_pair.to_string() << std::endl;
    std::cout << "negative   = " << negative.to_string() << std::endl;
    std::cout << std::endl;

    // --- Arithmetic ---
    std::cout << "--- Arithmetic ---" << std::endl;

    const uint128_t a(100ULL);
    const uint128_t b(50ULL);

    const uint128_t sum = a + b;
    const uint128_t diff = a - b;
    const uint128_t prod = a * b;
    const uint128_t quot = a / b;
    const uint128_t rem = a % b;

    std::cout << "a = " << a.to_string() << std::endl;
    std::cout << "b = " << b.to_string() << std::endl;
    std::cout << "a + b = " << sum.to_string() << std::endl;
    std::cout << "a - b = " << diff.to_string() << std::endl;
    std::cout << "a * b = " << prod.to_string() << std::endl;
    std::cout << "a / b = " << quot.to_string() << std::endl;
    std::cout << "a % b = " << rem.to_string() << std::endl;
    std::cout << std::endl;

    // --- Large values (beyond 64-bit) ---
    std::cout << "--- Large Values ---" << std::endl;

    // 2^64 = 18446744073709551616
    const uint128_t two_pow_64(0x1ULL, 0x0ULL);
    const uint128_t large_sum = two_pow_64 + uint128_t(1ULL);

    std::cout << "2^64     = " << two_pow_64.to_string() << std::endl;
    std::cout << "2^64 + 1 = " << large_sum.to_string() << std::endl;
    std::cout << "high     = 0x" << std::hex << two_pow_64.high() << std::endl;
    std::cout << "low      = 0x" << two_pow_64.low() << std::dec << std::endl;
    std::cout << std::endl;

    // --- Comparison ---
    std::cout << "--- Comparison ---" << std::endl;

    std::cout << "a == b : " << (a == b ? "true" : "false") << std::endl;
    std::cout << "a != b : " << (a != b ? "true" : "false") << std::endl;
    std::cout << "a >  b : " << (a > b ? "true" : "false") << std::endl;
    std::cout << "a <  b : " << (a < b ? "true" : "false") << std::endl;
    std::cout << "a >= b : " << (a >= b ? "true" : "false") << std::endl;
    std::cout << "a <= b : " << (a <= b ? "true" : "false") << std::endl;
    std::cout << std::endl;

    // --- String bases ---
    std::cout << "--- String Bases ---" << std::endl;

    const uint128_t val(255ULL);
    std::cout << "255 in decimal : " << val.to_string(10) << std::endl;
    std::cout << "255 in hex     : " << val.to_string(16) << std::endl;
    std::cout << "255 in binary  : " << val.to_string(2) << std::endl;
    std::cout << "255 in octal   : " << val.to_string(8) << std::endl;
    std::cout << std::endl;

    std::cout << "[OK] Tutorial 01 complete." << std::endl;
    return 0;
}
