// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_format.hpp - std::format Support
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_format.hpp"

#if __has_include(<format>)
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

        if ((str1 == "ff") && (str2 == "1000"))
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

        if (str == "Dec: 42, Hex: 2a, Bin: 101010")
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
    // [Test 11] Width / right-align (default)
    // ========================================================================
    {
        std::cout << "[Test 11] Width / right-align:\n";

        const uint128_t val{0, 42};
        const auto str{std::format("{:10}", val)};

        if (str == "        42")
        {
            std::cout << "  [OK] width_right_align\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] width_right_align got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 12] Left-align
    // ========================================================================
    {
        std::cout << "[Test 12] Left-align:\n";

        const uint128_t val{0, 42};
        const auto str{std::format("{:<10}", val)};

        if (str == "42        ")
        {
            std::cout << "  [OK] left_align\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] left_align got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 13] Center-align
    // ========================================================================
    {
        std::cout << "[Test 13] Center-align:\n";

        const uint128_t val{0, 42};
        const auto str{std::format("{:^10}", val)};

        if (str == "    42    ")
        {
            std::cout << "  [OK] center_align\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] center_align got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 14] Fill character + alignment
    // ========================================================================
    {
        std::cout << "[Test 14] Fill character:\n";

        const uint128_t val{0, 42};
        const auto str{std::format("{:*>10}", val)};

        if (str == "********42")
        {
            std::cout << "  [OK] fill_char\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] fill_char got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 15] Zero-padding
    // ========================================================================
    {
        std::cout << "[Test 15] Zero-padding:\n";

        const uint128_t val{0, 42};
        const auto str{std::format("{:010}", val)};

        if (str == "0000000042")
        {
            std::cout << "  [OK] zero_pad\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] zero_pad got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 16] Sign: always show (+)
    // ========================================================================
    {
        std::cout << "[Test 16] Sign always (+):\n";

        const int128_tc_t pos{42};
        const int128_tc_t neg{-42};
        const auto str1{std::format("{:+}", pos)};
        const auto str2{std::format("{:+}", neg)};

        if (str1 == "+42" && str2 == "-42")
        {
            std::cout << "  [OK] sign_always\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] sign_always got=\"" << str1 << "\", \"" << str2 << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 17] Sign: space for positive
    // ========================================================================
    {
        std::cout << "[Test 17] Sign space:\n";

        const int128_tc_t pos{42};
        const int128_tc_t neg{-42};
        const auto str1{std::format("{: }", pos)};
        const auto str2{std::format("{: }", neg)};

        if (str1 == " 42" && str2 == "-42")
        {
            std::cout << "  [OK] sign_space\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] sign_space got=\"" << str1 << "\", \"" << str2 << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 18] Alt form: hex prefix
    // ========================================================================
    {
        std::cout << "[Test 18] Alt form hex (#x):\n";

        const uint128_t val{0, 255};
        const auto str1{std::format("{:#x}", val)};
        const auto str2{std::format("{:#X}", val)};

        if (str1 == "0xff" && str2 == "0XFF")
        {
            std::cout << "  [OK] alt_hex\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] alt_hex got=\"" << str1 << "\", \"" << str2 << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 19] Alt form: binary prefix
    // ========================================================================
    {
        std::cout << "[Test 19] Alt form binary (#b):\n";

        const uint128_t val{0, 7};
        const auto str{std::format("{:#b}", val)};

        if (str == "0b111")
        {
            std::cout << "  [OK] alt_binary\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] alt_binary got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 20] Alt form: octal prefix
    // ========================================================================
    {
        std::cout << "[Test 20] Alt form octal (#o):\n";

        const uint128_t val{0, 64};
        const auto str{std::format("{:#o}", val)};

        if (str == "0100")
        {
            std::cout << "  [OK] alt_octal\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] alt_octal got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 21] Combined: sign + zero-pad + width
    // ========================================================================
    {
        std::cout << "[Test 21] Combined sign+zero+width:\n";

        const int128_tc_t val{42};
        const auto str{std::format("{:+010}", val)};

        if (str == "+000000042")
        {
            std::cout << "  [OK] sign_zero_width\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] sign_zero_width got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 22] Combined: alt + zero-pad + hex
    // ========================================================================
    {
        std::cout << "[Test 22] Combined alt+zero+hex:\n";

        const uint128_t val{0, 255};
        const auto str{std::format("{:#010x}", val)};

        if (str == "0x000000ff")
        {
            std::cout << "  [OK] alt_zero_hex\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] alt_zero_hex got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 23] Width with no padding needed
    // ========================================================================
    {
        std::cout << "[Test 23] Width no padding:\n";

        const uint128_t val{0, 12345};
        const auto str{std::format("{:3}", val)};

        if (str == "12345")
        {
            std::cout << "  [OK] width_no_pad\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] width_no_pad got=\"" << str << "\"\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 24] Negative with zero-pad
    // ========================================================================
    {
        std::cout << "[Test 24] Negative zero-pad:\n";

        const int128_tc_t val{-42};
        const auto str{std::format("{:010}", val)};

        if (str == "-000000042")
        {
            std::cout << "  [OK] neg_zero_pad\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] neg_zero_pad got=\"" << str << "\"\n";
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

#else // !__has_include(<format>)

int main()
{
    return 0; // std::format not available in this compiler (requires GCC 13+)
}

#endif // __has_include(<format>)
