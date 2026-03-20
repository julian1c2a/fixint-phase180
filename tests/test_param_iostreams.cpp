// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_iostreams.hpp - Stream I/O
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================

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

        const uint128_t val{0, 42};
        std::ostringstream oss;
        oss << val;

        assert(oss.str() == "42");
        std::cout << "  OK: Basic output: 42\n";
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
        std::cout << "  OK: Negative output: -42\n";
    }

    // ========================================================================
    // TEST 3: HEXADECIMAL OUTPUT
    // ========================================================================
    {
        std::cout << "\nTest 3: Hexadecimal output\n";

        const uint128_t val{0, 255};
        std::ostringstream oss;
        oss << std::hex << val;

        assert(oss.str() == "ff");
        std::cout << "  OK: Hex output: ff\n";
    }

    // ========================================================================
    // TEST 4: HEX WITH SHOWBASE
    // ========================================================================
    {
        std::cout << "\nTest 4: Hex with showbase\n";

        const uint128_t val{0, 255};
        std::ostringstream oss;
        oss << std::hex << std::showbase << val;

        assert(oss.str() == "0xff");
        std::cout << "  OK: Hex with prefix: 0xff\n";
    }

    // ========================================================================
    // TEST 5: HEX UPPERCASE
    // ========================================================================
    {
        std::cout << "\nTest 5: Hex uppercase\n";

        const uint128_t val{0, 255};
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::showbase << val;

        assert(oss.str() == "0XFF");
        std::cout << "  OK: Hex uppercase: 0XFF\n";
    }

    // ========================================================================
    // TEST 6: OCTAL OUTPUT
    // ========================================================================
    {
        std::cout << "\nTest 6: Octal output\n";

        const uint128_t val{0, 64};
        std::ostringstream oss;
        oss << std::oct << std::showbase << val;

        assert(oss.str() == "0100");
        std::cout << "  OK: Octal output: 0100\n";
    }

    // ========================================================================
    // TEST 7: WIDTH AND FILL
    // ========================================================================
    {
        std::cout << "\nTest 7: Width and fill\n";

        const uint128_t val{0, 42};
        std::ostringstream oss;
        oss << std::setw(10) << std::setfill('0') << val;

        assert(oss.str() == "0000000042");
        std::cout << "  OK: Width 10, fill '0': 0000000042\n";
    }

    // ========================================================================
    // TEST 8: LEFT ALIGNMENT
    // ========================================================================
    {
        std::cout << "\nTest 8: Left alignment\n";

        const uint128_t val{0, 42};
        std::ostringstream oss;
        oss << std::left << std::setw(10) << std::setfill('*') << val;

        assert(oss.str() == "42********");
        std::cout << "  OK: Left align: 42********\n";
    }

    // ========================================================================
    // TEST 9: SHOWPOS (positive sign)
    // ========================================================================
    {
        std::cout << "\nTest 9: showpos flag\n";

        const uint128_t val{0, 42};
        std::ostringstream oss;
        oss << std::showpos << val;

        assert(oss.str() == "+42");
        std::cout << "  OK: Showpos: +42\n";
    }

    // ========================================================================
    // TEST 10: BASIC INPUT (operator>>)
    // ========================================================================
    {
        std::cout << "\nTest 10: Basic input (operator>>)\n";

        std::istringstream iss("42");
        uint128_t val{0, 0};
        iss >> val;

        assert((val == uint128_t{0, 42}));
        std::cout << "  OK: Basic input: 42\n";
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
        std::cout << "  OK: Negative input: -42\n";
    }

    // ========================================================================
    // TEST 12: HEX INPUT
    // ========================================================================
    {
        std::cout << "\nTest 12: Hex input\n";

        std::istringstream iss("0xff");
        uint128_t val{0, 0};
        iss >> val;

        assert((val == uint128_t{0, 255}));
        std::cout << "  OK: Hex input: 0xff = 255\n";
    }

    // ========================================================================
    // TEST 13: ROUND-TRIP (output then input)
    // ========================================================================
    {
        std::cout << "\nTest 13: Round-trip conversion\n";

        const uint128_t original{0, 123456789};
        std::ostringstream oss;
        oss << original;

        std::istringstream iss(oss.str());
        uint128_t result{0, 0};
        iss >> result;

        assert((result == original));
        std::cout << "  OK: Round-trip successful\n";
    }

    // ========================================================================
    // TEST 14: MULTIPLE VALUES
    // ========================================================================
    {
        std::cout << "\nTest 14: Multiple values in stream\n";

        std::istringstream iss("100 200 300");
        uint128_t a{0, 0}, b{0, 0}, c{0, 0};
        iss >> a >> b >> c;

        assert((a == uint128_t{0, 100}));
        assert((b == uint128_t{0, 200}));
        assert((c == uint128_t{0, 300}));

        std::cout << "  OK: Multiple values: 100, 200, 300\n";
    }

    // ========================================================================
    // TEST 15: CONVENIENCE FUNCTIONS - format()
    // ========================================================================
    {
        std::cout << "\nTest 15: Convenience function format()\n";

        const uint128_t val{0, 42};
        const auto str1 = iostreams::format(val, 10, 8, '0', false, true, false, false);

        assert(str1 == "00000+42");
        std::cout << "  OK: format(42, width=8, fill='0', showpos): " << str1 << "\n";
    }

    // ========================================================================
    // TEST 16: CONVENIENCE FUNCTIONS - hex()
    // ========================================================================
    {
        std::cout << "\nTest 16: Convenience function hex()\n";

        const uint128_t val{0, 255};
        const auto str = iostreams::hex(val, true, true);

        assert(str == "0XFF");
        std::cout << "  OK: hex(255): " << str << "\n";
    }

    // ========================================================================
    // TEST 17: CONVENIENCE FUNCTIONS - oct()
    // ========================================================================
    {
        std::cout << "\nTest 17: Convenience function oct()\n";

        const uint128_t val{0, 64};
        const auto str = iostreams::oct(val, true);

        assert(str == "0100");
        std::cout << "  OK: oct(64): " << str << "\n";
    }

    // ========================================================================
    // TEST 18: CONVENIENCE FUNCTIONS - dec()
    // ========================================================================
    {
        std::cout << "\nTest 18: Convenience function dec()\n";

        const uint128_t val{0, 42};
        const auto str = iostreams::dec(val, true);

        assert(str == "+42");
        std::cout << "  OK: dec(42, showpos): " << str << "\n";
    }

    // ========================================================================
    // TEST 19: CONVENIENCE FUNCTIONS - bin()
    // ========================================================================
    {
        std::cout << "\nTest 19: Convenience function bin()\n";

        const uint128_t val{0, 5};
        const auto str = iostreams::bin(val, true);

        assert(str == "0b101");
        std::cout << "  OK: bin(5): " << str << "\n";
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

        std::cout << "  OK: MS positive: " << oss1.str() << "\n";
        std::cout << "  OK: MS negative: " << oss2.str() << "\n";
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

        std::cout << "  OK: EK zero (real): " << oss1.str() << "\n";
        std::cout << "  OK: EK positive (real): " << oss2.str() << "\n";
    }

    // ========================================================================
    // TEST 22: MS-SPECIFIC ROUND-TRIP
    // ========================================================================
    {
        std::cout << "\nTest 22: MS-specific round-trip\n";

        const int128_ms_t ms_pos_orig{0, 98765};
        const int128_ms_t ms_neg_orig = -ms_pos_orig;

        std::ostringstream oss_pos, oss_neg;
        oss_pos << ms_pos_orig;
        oss_neg << ms_neg_orig;

        assert(oss_pos.str() == "98765");
        assert(oss_neg.str() == "-98765");

        int128_ms_t ms_pos_read, ms_neg_read;
        std::istringstream iss_pos(oss_pos.str());
        std::istringstream iss_neg(oss_neg.str());

        iss_pos >> ms_pos_read;
        iss_neg >> ms_neg_read;

        assert(ms_pos_read == ms_pos_orig);
        assert(ms_neg_read == ms_neg_orig);

        std::cout << "  OK: MS round-trip positive: 98765 -> " << oss_pos.str() << " -> " << ms_pos_read << "\n";
        std::cout << "  OK: MS round-trip negative: -98765 -> " << oss_neg.str() << " -> " << ms_neg_read << "\n";
    }

    // ========================================================================
    // TEST 23: EK-SPECIFIC ROUND-TRIP (and negative)
    // ========================================================================
    {
        std::cout << "\nTest 23: EK-specific round-trip\n";

        const int128_ek_t ek_zero_orig = int128_ek_t::from_string("0");
        const int128_ek_t ek_pos_orig = int128_ek_t::from_string("12345");
        const int128_ek_t ek_neg_orig = int128_ek_t::from_string("-54321");

        std::ostringstream oss_zero, oss_pos, oss_neg;
        oss_zero << ek_zero_orig;
        oss_pos << ek_pos_orig;
        oss_neg << ek_neg_orig;

        assert(oss_zero.str() == "0");
        assert(oss_pos.str() == "12345");
        assert(oss_neg.str() == "-54321");

        int128_ek_t ek_zero_read, ek_pos_read, ek_neg_read;
        std::istringstream iss_zero(oss_zero.str());
        std::istringstream iss_pos(oss_pos.str());
        std::istringstream iss_neg(oss_neg.str());

        iss_zero >> ek_zero_read;
        iss_pos >> ek_pos_read;
        iss_neg >> ek_neg_read;

        assert(ek_zero_read == ek_zero_orig);
        assert(ek_pos_read == ek_pos_orig);
        assert(ek_neg_read == ek_neg_orig);

        std::cout << "  OK: EK round-trip zero: 0 -> " << oss_zero.str() << " -> " << ek_zero_read << "\n";
        std::cout << "  OK: EK round-trip positive: 12345 -> " << oss_pos.str() << " -> " << ek_pos_read << "\n";
        std::cout << "  OK: EK round-trip negative: -54321 -> " << oss_neg.str() << " -> " << ek_neg_read << "\n";
    }

    std::cout << "\nOK: All stream I/O tests passed!\n";
    std::cout << "\n>> Summary:\n";
    std::cout << "   - operator<< works for all representations\n";
    std::cout << "   - operator>> works for all representations\n";
    std::cout << "   - Stream flags respected (hex, oct, dec, showbase, showpos, uppercase)\n";
    std::cout << "   - Width, fill, and alignment working\n";
    std::cout << "   - Round-trip conversion lossless for TC, MS, and EK\n";
    std::cout << "   - Convenience functions (format, hex, oct, dec, bin)\n";
    std::cout << "   - MS and EK output real values (not stored)\n";

    return 0;
}
