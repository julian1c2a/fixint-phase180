// =============================================================================
// Priority 5: String I/O Tests
// =============================================================================
//
// Tests for string conversion methods (to_string, from_string, parsing)
//
// =============================================================================

#include "int128_parameterized.hpp"
#include <iostream>
#include <string>
#include <cassert>

using namespace nstd;

// Test macro
#define TEST(name) \
    static void test_##name(); \
    static struct test_##name##_runner { \
        test_##name##_runner() { \
            std::cout << "[TEST] " #name "... "; \
            test_##name(); \
            std::cout << "[OK]\n"; \
        } \
    } test_##name##_instance; \
    static void test_##name()

// =============================================================================
// Group 1: to_string() - Decimal (base 10)
// =============================================================================

TEST(to_string_decimal_zero)
{
    uint128_t zero{0};
    assert(zero.to_string() == "0");
    assert(zero.to_string(10) == "0");
}

TEST(to_string_decimal_small)
{
    uint128_t val{0, 42};
    assert(val.to_string() == "42");
    assert(val.to_string(10) == "42");
}

TEST(to_string_decimal_large)
{
    uint128_t val{0x1, 0x0};  // 2^64
    assert(val.to_string() == "18446744073709551616");
}

TEST(to_string_decimal_max)
{
    uint128_t max{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
    assert(max.to_string() == "340282366920938463463374607431768211455");
}

TEST(to_string_signed_negative)
{
    int128_tc_t neg{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // -1
    assert(neg.to_string() == "-1");
}

// =============================================================================
// Group 2: to_string() - Hexadecimal (base 16)
// =============================================================================

TEST(to_string_hex_zero)
{
    uint128_t zero{0};
    assert(zero.to_string(16) == "0");
}

TEST(to_string_hex_small)
{
    uint128_t val{0, 0xFF};
    assert(val.to_string(16) == "FF");
}

TEST(to_string_hex_large)
{
    uint128_t val{0xDEADBEEF, 0xCAFEBABE};
    assert(val.to_string(16) == "DEADBEEFCAFEBABE");
}

// =============================================================================
// Group 3: to_string() - Binary (base 2)
// =============================================================================

TEST(to_string_binary_zero)
{
    uint128_t zero{0};
    assert(zero.to_string(2) == "0");
}

TEST(to_string_binary_small)
{
    uint128_t val{0, 0b1010};
    assert(val.to_string(2) == "1010");
}

// =============================================================================
// Group 4: to_string() - Octal (base 8)
// =============================================================================

TEST(to_string_octal_zero)
{
    uint128_t zero{0};
    assert(zero.to_string(8) == "0");
}

TEST(to_string_octal_small)
{
    uint128_t val{0, 0777};
    assert(val.to_string(8) == "777");
}

// =============================================================================
// Group 5: from_string() - Decimal parsing
// =============================================================================

TEST(from_string_decimal_zero)
{
    auto val = uint128_t::from_string("0");
    assert(val == uint128_t{0});
}

TEST(from_string_decimal_small)
{
    auto val = uint128_t::from_string("42");
    assert(val == uint128_t{0, 42});
}

TEST(from_string_decimal_large)
{
    auto val = uint128_t::from_string("18446744073709551616");
    assert(val == uint128_t{0x1, 0x0});
}

TEST(from_string_signed_negative)
{
    auto val = int128_tc_t::from_string("-1");
    assert(val == int128_tc_t{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL});
}

// =============================================================================
// Group 6: from_string() - Hexadecimal parsing (0x prefix)
// =============================================================================

TEST(from_string_hex_prefix)
{
    auto val = uint128_t::from_string("0xFF");
    assert(val == uint128_t{0, 0xFF});
}

TEST(from_string_hex_large)
{
    auto val = uint128_t::from_string("0xDEADBEEFCAFEBABE");
    assert(val == uint128_t{0xDEADBEEF, 0xCAFEBABE});
}

// =============================================================================
// Group 7: from_string() - Binary parsing (0b prefix)
// =============================================================================

TEST(from_string_binary_prefix)
{
    auto val = uint128_t::from_string("0b1010");
    assert(val == uint128_t{0, 0b1010});
}

// =============================================================================
// Group 8: from_string() - Octal parsing (0 prefix)
// =============================================================================

TEST(from_string_octal_prefix)
{
    auto val = uint128_t::from_string("0777");
    assert(val == uint128_t{0, 0777});
}

// =============================================================================
// Group 9: Round-trip conversions
// =============================================================================

TEST(roundtrip_decimal)
{
    uint128_t original{0x123456789ABCDEFULL, 0xFEDCBA9876543210ULL};
    std::string str = original.to_string();
    auto parsed = uint128_t::from_string(str.c_str());
    assert(parsed == original);
}

TEST(roundtrip_hex)
{
    uint128_t original{0xDEADBEEF, 0xCAFEBABE};
    std::string str = original.to_string(16);
    auto parsed = uint128_t::from_string(("0x" + str).c_str());
    assert(parsed == original);
}

// =============================================================================
// Group 10: MS representation
// =============================================================================

TEST(to_string_ms_negative)
{
    int128_ms_t neg{0x8000000000000000ULL, 42};  // -42 in MS
    assert(neg.to_string() == "-42");
}

TEST(from_string_ms_negative)
{
    auto val = int128_ms_t::from_string("-42");
    assert(val.is_negative());
    assert(val.magnitude() == int128_ms_t{0, 42});
}

// =============================================================================
// Main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Priority 5: String I/O Tests\n";
    std::cout << "====================================================================\n\n";
    
    std::cout << "All tests completed successfully!\n";
    return 0;
}
