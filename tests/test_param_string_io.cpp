// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: String I/O — to_string bases, from_string, parse_ct_safe
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
//
// Consolidates: test_priority5_string_io, test_priority5_string
// =============================================================================

#include "int128_parameterized.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

using namespace nstd;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                                                    \
    do {                                                                    \
        if (cond) { std::cout << "[OK] " << (name) << "\n"; ++g_passed; }  \
        else      { std::cout << "[FAIL] " << (name) << "\n"; ++g_failed; }\
    } while (false)

// =============================================================================
// Section 1: to_string — base conversions and special values
// =============================================================================
static void test_to_string_bases()
{
    std::cout << "\n--- Section 1: to_string base conversions ---\n";

    // Full 128-bit max decimal
    const uint128_t max_val{~0ULL, ~0ULL};
    TEST("to_string max decimal", max_val.to_string() == "340282366920938463463374607431768211455");

    // Binary
    TEST("to_string bin zero",   uint128_t{0}.to_string(2) == "0");
    { const uint128_t v{0, 0b1010}; TEST("to_string bin 0b1010", v.to_string(2) == "1010"); }
    TEST("to_string bin 5",      int128_t{5}.to_string(2) == "101");

    // Octal
    TEST("to_string oct zero",   uint128_t{0}.to_string(8) == "0");
    { const uint128_t v{0, 0777}; TEST("to_string oct 0777", v.to_string(8) == "777"); }
    TEST("to_string oct 64",     int128_t{64}.to_string(8) == "100");

    // Hex
    TEST("to_string hex zero",    uint128_t{0}.to_string(16) == "0");
    { const uint128_t v{0, 0xFF}; TEST("to_string hex 0xFF", v.to_string(16) == "FF"); }
    { const uint128_t v{0, 0xDEADBEEFCAFEBABEULL};
      TEST("to_string hex DEADBEEFCAFEBABE", v.to_string(16) == "DEADBEEFCAFEBABE"); }
    { const uint128_t v{0xDEADBEEFULL, 0xCAFEBABEULL};
      TEST("to_string hex two-limb", v.to_string(16) == "DEADBEEF00000000CAFEBABE"); }
    TEST("to_string hex 255",     int128_t{255}.to_string(16) == "FF");

    // Base 36
    TEST("to_string base36 36", int128_t{36}.to_string(36) == "10");

    // Invalid bases → default to base 10
    TEST("to_string invalid base 100", int128_t{42}.to_string(100) == "42");
    TEST("to_string invalid base 1",   int128_t{42}.to_string(1)   == "42");

    // Negative hex has leading '-'
    TEST("to_string hex negative starts with -", int128_t{-255}.to_string(16)[0] == '-');

    // All bases 2..36 produce non-empty strings
    {
        const int128_t x{100};
        bool all_ok = true;
        for (int b = 2; b <= 36; ++b) all_ok = all_ok && !x.to_string(b).empty();
        TEST("to_string all bases 2..36 non-empty", all_ok);
    }

    // MS negative
    {
        int128_ms_t neg{0x8000000000000000ULL, 42};
        TEST("to_string ms -42", neg.to_string() == "-42");
    }
}

// =============================================================================
// Section 2: from_string — parsing with prefixes
// =============================================================================
static void test_from_string()
{
    std::cout << "\n--- Section 2: from_string ---\n";

    // Decimal
    TEST("from_string dec 0",   uint128_t::from_string("0") == uint128_t{0});
    { const uint128_t exp{0, 42};
      TEST("from_string dec 42", uint128_t::from_string("42") == exp); }
    { const uint128_t exp{1, 0};
      TEST("from_string dec 2^64", uint128_t::from_string("18446744073709551616") == exp); }
    { const int128_t exp{~0ULL, ~0ULL};
      TEST("from_string dec -1 (signed)", int128_t::from_string("-1") == exp); }

    // Hex prefix 0x
    { const uint128_t exp{0, 0xFF};
      TEST("from_string hex 0xFF", uint128_t::from_string("0xFF") == exp); }
    { const uint128_t exp{0, 0xDEADBEEFCAFEBABEULL};
      TEST("from_string hex DEADBEEFCAFEBABE",
           uint128_t::from_string("0xDEADBEEFCAFEBABE") == exp); }

    // Binary prefix 0b
    { const uint128_t exp{0, 0b1010};
      TEST("from_string bin 0b1010", uint128_t::from_string("0b1010") == exp); }

    // Octal prefix 0
    { const uint128_t exp{0, 0777};
      TEST("from_string oct 0777", uint128_t::from_string("0777") == exp); }

    // Signed
    TEST("from_string -99",    int128_t::from_string("-99")    == int128_t{-99LL});
    TEST("from_string 0xABCD", int128_t::from_string("0xABCD") == int128_t{0xABCDLL});

    // Roundtrips
    {
        const uint128_t orig{0x123456789ABCDEFULL, 0xFEDCBA9876543210ULL};
        TEST("from_string roundtrip decimal",
             uint128_t::from_string(orig.to_string().c_str()) == orig);
    }
    {
        const uint128_t orig{0xDEADBEEF, 0xCAFEBABE};
        TEST("from_string roundtrip hex",
             uint128_t::from_string(("0x" + orig.to_string(16)).c_str()) == orig);
    }

    // Invalid input throws std::invalid_argument
    {
        bool caught = false;
        try { int128_t::from_string("invalid@123"); }
        catch (const std::invalid_argument&) { caught = true; }
        TEST("from_string invalid throws", caught);
    }

    // MS negative
    {
        const auto val = int128_ms_t::from_string("-42");
        TEST("from_string ms -42 is_negative", val.is_negative());
        const int128_ms_t exp_mag{0, 42};
        TEST("from_string ms -42 magnitude==42", val.magnitude() == exp_mag);
    }
}

// =============================================================================
// Section 3: parse_ct_safe — non-throwing parser
// =============================================================================
static void test_parse_ct_safe()
{
    std::cout << "\n--- Section 3: parse_ct_safe ---\n";

    // Decimal
    { const auto r = int128_t::parse_ct_safe("12345");
      TEST("parse decimal 12345", r.success() && r.value == int128_t{12345LL}); }

    { const auto r = int128_t::parse_ct_safe("-54321");
      TEST("parse negative -54321", r.success() && r.value == int128_t{-54321LL}); }

    // Hex, binary, octal
    { const auto r = int128_t::parse_ct_safe("0xFF");
      TEST("parse hex 0xFF",       r.success() && r.value == int128_t{255LL}); }

    { const auto r = int128_t::parse_ct_safe("0b101");
      TEST("parse binary 0b101",   r.success() && r.value == int128_t{5LL}); }

    { const auto r = int128_t::parse_ct_safe("0777");
      TEST("parse octal 0777",     r.success() && r.value == int128_t{511LL}); }

    // Error: empty string
    { const auto r = int128_t::parse_ct_safe("");
      TEST("parse empty -> empty_string error",
           !r.success() && r.error == parse_error::empty_string); }

    // Error: null pointer
    { const auto r = int128_t::parse_ct_safe(nullptr);
      TEST("parse null -> null_pointer error",
           !r.success() && r.error == parse_error::null_pointer); }

    // Error: invalid character
    { const auto r = int128_t::parse_ct_safe("12@45");
      TEST("parse invalid_char error",
           !r.success() && r.error == parse_error::invalid_character); }

    // Separator '_' is skipped
    { const auto r = int128_t::parse_ct_safe("1_000_000");
      TEST("parse separators 1_000_000",
           r.success() && r.value == int128_t{1000000LL}); }

    // Roundtrip via parse_ct_safe
    { const int128_t orig{98765};
      const auto r = int128_t::parse_ct_safe(orig.to_string().c_str());
      TEST("parse_ct_safe roundtrip 98765", r.success() && r.value == orig); }

    // Unsigned roundtrip
    { const uint128_t orig{999999};
      const auto r = uint128_t::parse_ct_safe(orig.to_string().c_str());
      TEST("parse_ct_safe unsigned roundtrip", r.success() && r.value == orig); }
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "================================================================\n";
    std::cout << "  test_param_string_io: to_string bases, from_string, parse_ct_safe\n";
    std::cout << "================================================================\n";

    test_to_string_bases();
    test_from_string();
    test_parse_ct_safe();

    std::cout << "\n================================================================\n";
    std::cout << "  RESULTS: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "================================================================\n";

    return g_failed ? 1 : 0;
}
