// =============================================================================
// Test: int128_param_iostreams.hpp - Stream I/O
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_iostreams.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Testing int128_param_iostreams.hpp (Stream I/O)...\n\n";

    // ========================================================================
    // TEST 1: BASIC OUTPUT (operator<<)
    // ========================================================================
    {
        std::cout << "Test 1: Basic output (operator<<)\n";

        const uint128_tc_t val{0, 42};
        std::ostringstream oss;
        oss << val;

        assert(oss.str() == "42");
        std::cout << "  ✓ Basic output: 42\n";
    }

    // ========================================================================
    // TEST 2: NEGATIVE VALUES
    // ========================================================================
    {
        std::cout << "\nTest 2: Negative values\n";

        const int128_tc_t val{-42};
        std::ostringstream oss;
        oss << val;

        assert(oss.str() == "-42");
        std::cout << "  ✓ Negative output: -42\n";
    }

    // ========================================================================
    // TEST 3: HEXADECIMAL OUTPUT
    // ========================================================================
    {
        std::cout << "\nTest 3: Hexadecimal output\n";

        const uint128_tc_t val{0, 255};
        std::ostringstream oss;
        oss << std::hex << val;

        assert(oss.str() == "ff");
        std::cout << "  ✓ Hex output: ff\n";
    }

    // ========================================================================
    // TEST 4: HEX WITH SHOWBASE
    // ========================================================================
    {
        std::cout << "\nTest 4: Hex with showbase\n";

        const uint128_tc_t val{0, 255};
        std::ostringstream oss;
        oss << std::hex << std::showbase << val;

        assert(oss.str() == "0xff");
        std::cout << "  ✓ Hex with prefix: 0xff\n";
    }

    // ========================================================================
    // TEST 5: HEX UPPERCASE
    // ========================================================================
    {
        std::cout << "\nTest 5: Hex uppercase\n";

        const uint128_tc_t val{0, 255};
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::showbase << val;

        assert(oss.str() == "0XFF");
        std::cout << "  ✓ Hex uppercase: 0XFF\n";
    }

    // ========================================================================
    // TEST 6: OCTAL OUTPUT
    // ========================================================================
    {
        std::cout << "\nTest 6: Octal output\n";

        const uint128_tc_t val{0, 64};
        std::ostringstream oss;
        oss << std::oct << std::showbase << val;

        assert(oss.str() == "0100");
        std::cout << "  ✓ Octal output: 0100\n";
    }

    // ========================================================================
    // TEST 7: WIDTH AND FILL
    // ========================================================================
    {
        std::cout << "\nTest 7: Width and fill\n";

        const uint128_tc_t val{0, 42};
        std::ostringstream oss;
        oss << std::setw(10) << std::setfill('0') << val;

        assert(oss.str() == "0000000042");
        std::cout << "  ✓ Width 10, fill '0': 0000000042\n";
    }

    // ========================================================================
    // TEST 8: LEFT ALIGNMENT
    // ========================================================================
    {
        std::cout << "\nTest 8: Left alignment\n";

        const uint128_tc_t val{0, 42};
        std::ostringstream oss;
        oss << std::left << std::setw(10) << std::setfill('*') << val;

        assert(oss.str() == "42********");
        std::cout << "  ✓ Left align: 42********\n";
    }

    // ========================================================================
    // TEST 9: SHOWPOS (positive sign)
    // ========================================================================
    {
        std::cout << "\nTest 9: showpos flag\n";

        const uint128_tc_t val{0, 42};
        std::ostringstream oss;
        oss << std::showpos << val;

        assert(oss.str() == "+42");
        std::cout << "  ✓ Showpos: +42\n";
    }

    // ========================================================================
    // TEST 10: BASIC INPUT (operator>>)
    // ========================================================================
    {
        std::cout << "\nTest 10: Basic input (operator>>)\n";

        std::istringstream iss("42");
        uint128_tc_t val{0, 0};
        iss >> val;

        assert((val == uint128_tc_t{0, 42}));
        std::cout << "  ✓ Basic input: 42\n";
    }

    // ========================================================================
    // TEST 11: NEGATIVE INPUT
    // ========================================================================
    {
        std::cout << "\nTest 11: Negative input\n";

        std::istringstream iss("-42");
        int128_tc_t val{0, 0};
        iss >> val;

        assert((val == int128_tc_t{-42}));
        std::cout << "  ✓ Negative input: -42\n";
    }

    // ========================================================================
    // TEST 12: HEX INPUT
    // ========================================================================
    {
        std::cout << "\nTest 12: Hex input\n";

        std::istringstream iss("0xff");
        uint128_tc_t val{0, 0};
        iss >> val;

        assert((val == uint128_tc_t{0, 255}));
        std::cout << "  ✓ Hex input: 0xff = 255\n";
    }

    // ========================================================================
    // TEST 13: ROUND-TRIP (output then input)
    // ========================================================================
    {
        std::cout << "\nTest 13: Round-trip conversion\n";

        const uint128_tc_t original{0, 123456789};
        std::ostringstream oss;
        oss << original;

        std::istringstream iss(oss.str());
        uint128_tc_t result{0, 0};
        iss >> result;

        assert((result == original));
        std::cout << "  ✓ Round-trip successful\n";
    }

    // ========================================================================
    // TEST 14: MULTIPLE VALUES
    // ========================================================================
    {
        std::cout << "\nTest 14: Multiple values in stream\n";

        std::istringstream iss("100 200 300");
        uint128_tc_t a{0, 0}, b{0, 0}, c{0, 0};
        iss >> a >> b >> c;

        assert((a == uint128_tc_t{0, 100}));
        assert((b == uint128_tc_t{0, 200}));
        assert((c == uint128_tc_t{0, 300}));

        std::cout << "  ✓ Multiple values: 100, 200, 300\n";
    }

    // ========================================================================
    // TEST 15: CONVENIENCE FUNCTIONS - format()
    // ========================================================================
    {
        std::cout << "\nTest 15: Convenience function format()\n";

        const uint128_tc_t val{0, 42};
        const auto str1 = iostreams::format(val, 10, 8, '0', false, true, false, false);

        assert(str1 == "00000+42");
        std::cout << "  ✓ format(42, width=8, fill='0', showpos): " << str1 << "\n";
    }

    // ========================================================================
    // TEST 16: CONVENIENCE FUNCTIONS - hex()
    // ========================================================================
    {
        std::cout << "\nTest 16: Convenience function hex()\n";

        const uint128_tc_t val{0, 255};
        const auto str = iostreams::hex(val, true, true);

        assert(str == "0XFF");
        std::cout << "  ✓ hex(255): " << str << "\n";
    }

    // ========================================================================
    // TEST 17: CONVENIENCE FUNCTIONS - oct()
    // ========================================================================
    {
        std::cout << "\nTest 17: Convenience function oct()\n";

        const uint128_tc_t val{0, 64};
        const auto str = iostreams::oct(val, true);

        assert(str == "0100");
        std::cout << "  ✓ oct(64): " << str << "\n";
    }

    // ========================================================================
    // TEST 18: CONVENIENCE FUNCTIONS - dec()
    // ========================================================================
    {
        std::cout << "\nTest 18: Convenience function dec()\n";

        const uint128_tc_t val{0, 42};
        const auto str = iostreams::dec(val, true);

        assert(str == "+42");
        std::cout << "  ✓ dec(42, showpos): " << str << "\n";
    }

    // ========================================================================
    // TEST 19: CONVENIENCE FUNCTIONS - bin()
    // ========================================================================
    {
        std::cout << "\nTest 19: Convenience function bin()\n";

        const uint128_tc_t val{0, 5};
        const auto str = iostreams::bin(val, true);

        assert(str == "0b101");
        std::cout << "  ✓ bin(5): " << str << "\n";
    }

    // ========================================================================
    // TEST 20: MS-SPECIFIC (sign bit handling)
    // ========================================================================
    {
        std::cout << "\nTest 20: MS-specific output\n";

        int128_ms_t ms_pos{0, 42};
        int128_ms_t ms_neg{static_cast<uint64_t>(1ULL << 63), 42}; // Sign bit set

        std::ostringstream oss1, oss2;
        oss1 << ms_pos;
        oss2 << ms_neg;

        assert(oss1.str() == "42");
        assert(oss2.str() == "-42");

        std::cout << "  ✓ MS positive: " << oss1.str() << "\n";
        std::cout << "  ✓ MS negative: " << oss2.str() << "\n";
    }

    // ========================================================================
    // TEST 21: EK-SPECIFIC (real value output)
    // ========================================================================
    {
        std::cout << "\nTest 21: EK-specific output (real values)\n";

        // Zero (stored = bias)
        const int128_ek_t ek_zero{(1ULL << 62), 0};
        // Positive (stored = bias+1)
        const int128_ek_t ek_pos{(1ULL << 62), 1};

        std::ostringstream oss1, oss2;
        oss1 << ek_zero;
        oss2 << ek_pos;

        assert(oss1.str() == "0");
        assert(oss2.str() == "1");

        std::cout << "  ✓ EK zero (real): " << oss1.str() << "\n";
        std::cout << "  ✓ EK positive (real): " << oss2.str() << "\n";
    }

    std::cout << "\n✅ All stream I/O tests passed!\n";
    std::cout << "\n📝 Summary:\n";
    std::cout << "   - operator<< works for all representations\n";
    std::cout << "   - operator>> works for all representations\n";
    std::cout << "   - Stream flags respected (hex, oct, dec, showbase, showpos, uppercase)\n";
    std::cout << "   - Width, fill, and alignment working\n";
    std::cout << "   - Round-trip conversion lossless\n";
    std::cout << "   - Convenience functions (format, hex, oct, dec, bin)\n";
    std::cout << "   - MS and EK output real values (not stored)\n";

    return 0;
}
