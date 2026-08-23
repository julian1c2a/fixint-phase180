// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Sweep String Roundtrip - Systematic 3-region coverage for to/from_string
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests roundtrip consistency: from_string(to_string(x)) == x
// across ~6.3M values per sweep using the 3-region methodology.
//
// Migrated from: test_priority5_string_io.cpp, test_tostring_fast.cpp (Section 4)
// New coverage:  ~6.3M values for decimal, hex, octal, binary roundtrips

#include "test_sweep_framework.hpp"
#include <cstdlib>
#include <string>

using nstd::uint128_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Sweep String Roundtrip Tests (3-region systematic coverage)\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS << " = " << SWEEP_REGION_SIZE
              << " values per region\n";
    std::cout << "====================================================================\n\n";

    int passed{0};
    int total{0};

    // ========================================================================
    // Section 1: Decimal roundtrip: from_string(to_string(x, 10)) == x
    // ========================================================================
    std::cout << "[Section 1] Decimal roundtrip\n";
    print_sweep_separator();

    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                const std::string s{a.to_string(10)};
                return uint128_t::from_string(s.c_str());
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "decimal_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 2: Hexadecimal roundtrip: from_string("0x" + to_string(x, 16)) == x
    // ========================================================================
    std::cout << "[Section 2] Hexadecimal roundtrip\n";
    print_sweep_separator();

    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                const std::string hex{a.to_string(16)};
                const std::string prefixed{"0x" + hex};
                return uint128_t::from_string(prefixed.c_str());
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "hex_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 3: Octal roundtrip: from_string("0" + to_string(x, 8)) == x
    // ========================================================================
    std::cout << "[Section 3] Octal roundtrip\n";
    print_sweep_separator();

    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                const std::string oct{a.to_string(8)};
                const std::string prefixed{"0" + oct};
                return uint128_t::from_string(prefixed.c_str());
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "octal_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 4: Binary roundtrip: from_string("0b" + to_string(x, 2)) == x
    // ========================================================================
    std::cout << "[Section 4] Binary roundtrip\n";
    print_sweep_separator();

    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> uint128_t
            {
                const std::string bin{a.to_string(2)};
                const std::string prefixed{"0b" + bin};
                return uint128_t::from_string(prefixed.c_str());
            },
            [](const uint128_t &a) -> uint128_t { return a; }, "binary_roundtrip"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 5: to_string properties
    // ========================================================================
    std::cout << "[Section 5] to_string properties\n";
    print_sweep_separator();

    // Zero always produces "0"
    ++total;
    {
        const std::string s{uint128_t{0ULL}.to_string()};
        const bool ok{s == "0"};
        std::cout << "  " << std::left << std::setw(30) << "zero_to_string" << (ok ? " [OK]" : " [FAIL]")
                  << "\n";
        if (ok)
        {
            ++passed;
        }
    }

    // Decimal string: no leading zeros (for non-zero values)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            {
                if (a.is_zero())
                {
                    return 1; // "0" is fine
                }
                const std::string s{a.to_string(10)};
                return (s[0] != '0') ? 1 : 0;
            },
            [](const uint128_t &) -> int { return 1; }, "no_leading_zeros_decimal"))
    {
        ++passed;
    }

    // Hex string: no leading zeros (for non-zero values)
    ++total;
    if (sweep_unary(
            [](const uint128_t &a) -> int
            {
                if (a.is_zero())
                {
                    return 1; // "0" is fine
                }
                const std::string s{a.to_string(16)};
                return (s[0] != '0') ? 1 : 0;
            },
            [](const uint128_t &) -> int { return 1; }, "no_leading_zeros_hex"))
    {
        ++passed;
    }

    // Decimal string length: <= 39 chars (max uint128 = 39 digits)
    ++total;
    if (sweep_unary([](const uint128_t &a) -> int { return (a.to_string(10).length() <= 39) ? 1 : 0; },
                    [](const uint128_t &) -> int { return 1; }, "decimal_length_le_39"))
    {
        ++passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Summary
    // ========================================================================
    print_sweep_summary(passed, total);

    return (passed == total) ? EXIT_SUCCESS : EXIT_FAILURE;
}
