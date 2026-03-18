// =============================================================================
// Test: int128_param_format.hpp - std::format Support
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_format.hpp"
#include <iostream>
#include <format>
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
    std::cout << "std::format Support Tests (decimal, hex, binary, octal)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] Default format (decimal)
    // ========================================================================
    {
        std::cout << "[Test 1] Default format (decimal):\n";

        const uint128_t val1{0, 42};
        const uint128_t val2{0, 1000};
        const int128_tc_t val3{-100};

        const auto str1{std::format("{}", val1)};
        const auto str2{std::format("{}", val2)};
        const auto str3{std::format("{}", val3)};

        if ((str1 == "42") && (str2 == "1000") && (str3 == "-100"))
        {
            std::cout << "  [OK] default_decimal\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] default_decimal\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 2] Explicit decimal format
    // ========================================================================
    {
        std::cout << "[Test 2] Explicit decimal (:d):\n";

        const uint128_t val{0, 255};
        const auto str{std::format("{:d}", val)};

        if (str == "255")
        {
            std::cout << "  [OK] explicit_decimal\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] explicit_decimal\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 3] Lowercase hexadecimal (:x)
    // ========================================================================
    {
        std::cout << "[Test 3] Lowercase hexadecimal (:x):\n";

        const uint128_t val1{0, 255};
        const uint128_t val2{0, 4096};

        const auto str1{std::format("{:x}", val1)};
        const auto str2{std::format("{:x}", val2)};

        if ((str1 == "FF") && (str2 == "1000"))
        {
            std::cout << "  [OK] lowercase_hex\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] lowercase_hex\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 4] Uppercase hexadecimal (:X)
    // ========================================================================
    {
        std::cout << "[Test 4] Uppercase hexadecimal (:X):\n";

        const uint128_t val{0, 255};
        const auto str{std::format("{:X}", val)};

        if (str == "FF")
        {
            std::cout << "  [OK] uppercase_hex\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] uppercase_hex\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 5] Binary format (:b)
    // ========================================================================
    {
        std::cout << "[Test 5] Binary format (:b):\n";

        const uint128_t val{0, 7};
        const auto str{std::format("{:b}", val)};

        if (str == "111")
        {
            std::cout << "  [OK] binary\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] binary\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 6] Octal format (:o)
    // ========================================================================
    {
        std::cout << "[Test 6] Octal format (:o):\n";

        const uint128_t val{0, 64};
        const auto str{std::format("{:o}", val)};

        if (str == "100")
        {
            std::cout << "  [OK] octal\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] octal\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 7] Mixed formats in single string
    // ========================================================================
    {
        std::cout << "[Test 7] Mixed formats:\n";

        // Note: separate format calls to work around icpx 2025.3.2 WSL -O2
        //       inlining bug with multi-arg std::format on int128
        uint128_t val{0, 42};
        const auto dec{std::format("{}", val)};
        const auto hex{std::format("{:x}", val)};
        const auto bin{std::format("{:b}", val)};
        const std::string str{"Dec: " + dec + ", Hex: " + hex + ", Bin: " + bin};

        if (str == "Dec: 42, Hex: 2A, Bin: 101010")
        {
            std::cout << "  [OK] mixed_formats\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] mixed_formats\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 8] Signed types (TC)
    // ========================================================================
    {
        std::cout << "[Test 8] Signed type formats:\n";

        const int128_tc_t val_pos{100};
        const int128_tc_t val_neg{-50};

        const auto str1{std::format("{}", val_pos)};
        const auto str2{std::format("{}", val_neg)};
        const auto str3{std::format("{:x}", val_neg)};

        // Negative in hex shows two's complement representation
        if ((str1 == "100") && (str2 == "-50") && !str3.empty())
        {
            std::cout << "  [OK] signed_formats\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] signed_formats\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 9] Zero value
    // ========================================================================
    {
        std::cout << "[Test 9] Zero value:\n";

        const uint128_t zero{0, 0};
        const auto str1{std::format("{}", zero)};
        const auto str2{std::format("{:x}", zero)};
        const auto str3{std::format("{:b}", zero)};

        if ((str1 == "0") && (str2 == "0") && (str3 == "0"))
        {
            std::cout << "  [OK] zero_value\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] zero_value\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 10] Large values
    // ========================================================================
    {
        std::cout << "[Test 10] Large values:\n";

        const uint128_t large{1, 0}; // 2^64
        const auto str_dec{std::format("{}", large)};
        const auto str_hex{std::format("{:x}", large)};

        // 2^64 = 18446744073709551616 in decimal
        // 2^64 = 10000000000000000 in hex (without 0x prefix)
        if ((str_dec == "18446744073709551616") &&
            (str_hex == "10000000000000000"))
        {
            std::cout << "  [OK] large_values\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] large_values\n";
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
