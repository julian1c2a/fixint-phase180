// =============================================================================
// Test: to_string() with Granlund-Montgomery fast path verification
// Part of int128 Library
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "test_sweep_framework.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

using namespace nstd;
using std::uint64_t;

int main()
{
    std::cout << "=== to_string() Fast Path Tests ===" << std::endl;
    std::cout << std::endl;

    int total{0};
    int passed{0};

    // =========================================================================
    // Section 1: Known decimal values
    // =========================================================================
    std::cout << "--- Section 1: Known Decimal Values ---" << std::endl;

    auto check = [&](const uint128_t &val, const std::string &expected, const char *label)
    {
        ++total;
        const std::string got{val.to_string()};
        if (got == expected)
        {
            std::cout << "[OK] " << label << ": " << got << std::endl;
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] " << label << ": expected=" << expected << " got=" << got << std::endl;
        }
    };

    check(uint128_t{0}, "0", "zero");
    check(uint128_t{1}, "1", "one");
    check(uint128_t{9}, "9", "nine");
    check(uint128_t{10}, "10", "ten");
    check(uint128_t{42}, "42", "forty-two");
    check(uint128_t{100}, "100", "hundred");
    check(uint128_t{255}, "255", "uint8_max");
    check(uint128_t{65535}, "65535", "uint16_max");
    check(uint128_t{0xFFFFFFFFull}, "4294967295", "uint32_max");
    check(uint128_t{0xFFFFFFFFFFFFFFFFull}, "18446744073709551615", "uint64_max");

    // Large 128-bit values
    check(uint128_t{1ull, 0ull}, "18446744073709551616", "2^64");
    check(uint128_t{10ull, 0ull}, "184467440737095516160", "10*2^64");
    check(uint128_t{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull}, "340282366920938463463374607431768211455",
          "MAX128");

    // =========================================================================
    // Section 2: Signed values (twos_complement)
    // =========================================================================
    std::cout << std::endl << "--- Section 2: Signed Values (TC) ---" << std::endl;

    {
        ++total;
        const int128_t neg{-1};
        const std::string s{neg.to_string()};
        if (s == "-1")
        {
            std::cout << "[OK] int128 -1: " << s << std::endl;
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] int128 -1: got=" << s << std::endl;
        }
    }
    {
        ++total;
        const int128_t neg{-42};
        const std::string s{neg.to_string()};
        if (s == "-42")
        {
            std::cout << "[OK] int128 -42: " << s << std::endl;
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] int128 -42: got=" << s << std::endl;
        }
    }
    {
        ++total;
        const int128_t pos{12345};
        const std::string s{pos.to_string()};
        if (s == "12345")
        {
            std::cout << "[OK] int128 12345: " << s << std::endl;
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] int128 12345: got=" << s << std::endl;
        }
    }

    // =========================================================================
    // Section 3: Non-decimal bases still work (generic path)
    // =========================================================================
    std::cout << std::endl << "--- Section 3: Non-Decimal Bases ---" << std::endl;

    {
        ++total;
        const uint128_t val{255};
        const std::string hex{val.to_string(16)};
        if (hex == "FF")
        {
            std::cout << "[OK] 255 base16: " << hex << std::endl;
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] 255 base16: got=" << hex << std::endl;
        }
    }
    {
        ++total;
        const uint128_t val{8};
        const std::string oct{val.to_string(8)};
        if (oct == "10")
        {
            std::cout << "[OK] 8 base8: " << oct << std::endl;
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] 8 base8: got=" << oct << std::endl;
        }
    }
    {
        ++total;
        const uint128_t val{10};
        const std::string bin{val.to_string(2)};
        if (bin == "1010")
        {
            std::cout << "[OK] 10 base2: " << bin << std::endl;
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] 10 base2: got=" << bin << std::endl;
        }
    }

    // =========================================================================
    // Section 4: Sweep - to_string round-trip consistency
    // =========================================================================
    std::cout << std::endl << "--- Section 4: Sweep Round-Trip ---" << std::endl;

    {
        // Verify to_string(10) produces correct digits by reconstructing
        // the value from the decimal string manually (avoiding from_string)
        const bool ok{sweep_unary(
            [](const uint128_t &n) -> uint128_t
            {
                const std::string s{n.to_string()};
                uint128_t result{0};
                for (const char c : s)
                {
                    result = result * uint128_t{10} + uint128_t{static_cast<uint64_t>(c - '0')};
                }
                return result;
            },
            [](const uint128_t &n) -> uint128_t
            {
                return n; // identity
            },
            "to_string_roundtrip")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    // =========================================================================
    // Section 5: Powers of 10
    // =========================================================================
    std::cout << std::endl << "--- Section 5: Powers of 10 ---" << std::endl;

    {
        ++total;
        bool all_ok{true};
        uint128_t power{1};
        for (int exp{0}; exp <= 38; ++exp)
        {
            const std::string s{power.to_string()};
            // Verify: string starts with "1" followed by exp zeros
            std::string expected{"1"};
            for (int i{0}; i < exp; ++i)
            {
                expected += "0";
            }
            if (s != expected)
            {
                std::cout << "[FAIL] 10^" << exp << ": expected=" << expected << " got=" << s << std::endl;
                all_ok = false;
            }
            if (exp < 38)
            {
                power = power * uint128_t{10};
            }
        }
        if (all_ok)
        {
            std::cout << "[OK] Powers 10^0..10^38 all correct" << std::endl;
            ++passed;
        }
    }

    // =========================================================================
    std::cout << std::endl;
    print_sweep_summary(passed, total);

    return (passed == total) ? 0 : 1;
}
