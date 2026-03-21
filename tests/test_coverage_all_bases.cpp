// =============================================================================
// Test: Coverage for to_string / from_string / to_cstr across all bases
// Part of int128 Library
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

using namespace nstd;
using std::uint64_t;

int total{0};
int passed{0};

static void check(bool cond, const char *label)
{
    ++total;
    if (cond)
    {
        ++passed;
        std::cout << "[OK] " << label << std::endl;
    }
    else
    {
        std::cout << "[FAIL] " << label << std::endl;
    }
}

static void check_str(const std::string &got, const std::string &expected, const char *label)
{
    ++total;
    if (got == expected)
    {
        ++passed;
    }
    else
    {
        std::cout << "[FAIL] " << label << ": expected=\"" << expected
                  << "\" got=\"" << got << "\"" << std::endl;
    }
}

// ============================================================================
// Section 1: to_string pow2 bases - boundary values
// ============================================================================
static void test_pow2_bases_boundary()
{
    std::cout << "\n--- Pow2 bases boundary values ---\n";

    const uint128_t zero{0};
    const uint128_t one{1};
    const uint128_t u64max{0xFFFFFFFFFFFFFFFFull};
    const uint128_t two_64{1ull, 0ull}; // 2^64
    const uint128_t max128{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};

    // Base 2
    check_str(zero.to_string(2), "0", "b2_zero");
    check_str(one.to_string(2), "1", "b2_one");
    check_str(uint128_t{0b1010}.to_string(2), "1010", "b2_ten");
    check_str(uint128_t{255}.to_string(2), "11111111", "b2_255");

    // Base 4
    check_str(zero.to_string(4), "0", "b4_zero");
    check_str(one.to_string(4), "1", "b4_one");
    check_str(uint128_t{255}.to_string(4), "3333", "b4_255");
    check_str(uint128_t{256}.to_string(4), "10000", "b4_256");

    // Base 8
    check_str(zero.to_string(8), "0", "b8_zero");
    check_str(one.to_string(8), "1", "b8_one");
    check_str(uint128_t{8}.to_string(8), "10", "b8_eight");
    check_str(uint128_t{63}.to_string(8), "77", "b8_63");
    check_str(uint128_t{64}.to_string(8), "100", "b8_64");
    check_str(uint128_t{511}.to_string(8), "777", "b8_511");

    // Base 16
    check_str(zero.to_string(16), "0", "b16_zero");
    check_str(one.to_string(16), "1", "b16_one");
    check_str(uint128_t{15}.to_string(16), "F", "b16_15");
    check_str(uint128_t{16}.to_string(16), "10", "b16_16");
    check_str(uint128_t{255}.to_string(16), "FF", "b16_255");
    check_str(uint128_t{0xDEADBEEF}.to_string(16), "DEADBEEF", "b16_deadbeef");

    // Base 32
    check_str(zero.to_string(32), "0", "b32_zero");
    check_str(one.to_string(32), "1", "b32_one");
    check_str(uint128_t{31}.to_string(32), "V", "b32_31");
    check_str(uint128_t{32}.to_string(32), "10", "b32_32");

    // MAX128 for pow2 bases (Python-verified)
    // bin: 128 ones
    {
        const std::string bin_max{max128.to_string(2)};
        check(bin_max.size() == 128, "b2_max_length");
        bool all_ones{true};
        for (const char c : bin_max)
        {
            if (c != '1')
            {
                all_ones = false;
                break;
            }
        }
        check(all_ones, "b2_max_all_ones");
    }
    // base4 MAX: 64 '3's
    {
        const std::string b4_max{max128.to_string(4)};
        check(b4_max.size() == 64, "b4_max_length");
        bool all_threes{true};
        for (const char c : b4_max)
        {
            if (c != '3')
            {
                all_threes = false;
                break;
            }
        }
        check(all_threes, "b4_max_all_threes");
    }
    // oct MAX: "3" + 42 '7's = 43 chars
    check_str(max128.to_string(8).substr(0, 1), "3", "b8_max_leading");
    check(max128.to_string(8).size() == 43, "b8_max_length");
    // hex MAX: 32 'F's
    {
        const std::string hex_max{max128.to_string(16)};
        check(hex_max.size() == 32, "b16_max_length");
        bool all_f{true};
        for (const char c : hex_max)
        {
            if (c != 'F')
            {
                all_f = false;
                break;
            }
        }
        check(all_f, "b16_max_all_F");
    }
    // b32 MAX: "7" + 24 'V's = 25 chars + leading '7' (verified with Python)
    {
        const std::string b32_max{max128.to_string(32)};
        check(b32_max.size() == 26, "b32_max_length");
        check_str(b32_max.substr(0, 1), "7", "b32_max_leading");
    }

    // 2^64 boundary (tests limb crossing)
    check_str(two_64.to_string(2), "10000000000000000000000000000000000000000000000000000000000000000", "b2_two64");
    check_str(two_64.to_string(16), "10000000000000000", "b16_two64");
    check_str(u64max.to_string(16), "FFFFFFFFFFFFFFFF", "b16_u64max");
}

// ============================================================================
// Section 2: to_string non-pow2 bases
// ============================================================================
static void test_nonpow2_bases()
{
    std::cout << "\n--- Non-pow2 bases ---\n";

    const uint128_t val{100};

    // base 3: 100 decimal = 10201 (1*81 + 0*27 + 2*9 + 0*3 + 1)
    check_str(val.to_string(3), "10201", "b3_100");
    // base 5: 100 = 400 (4*25 + 0*5 + 0)
    check_str(val.to_string(5), "400", "b5_100");
    // base 6: 100 = 244 (2*36 + 4*6 + 4)
    check_str(val.to_string(6), "244", "b6_100");
    // base 7: 100 = 202 (2*49 + 0*7 + 2)
    check_str(val.to_string(7), "202", "b7_100");
    // base 9: 100 = 121 (1*81 + 2*9 + 1)
    check_str(val.to_string(9), "121", "b9_100");
    // base 36: 100 = 2S (2*36 + 28)
    check_str(val.to_string(36), "2S", "b36_100");

    // Zero and one for non-pow2
    check_str(uint128_t{0}.to_string(3), "0", "b3_zero");
    check_str(uint128_t{0}.to_string(7), "0", "b7_zero");
    check_str(uint128_t{1}.to_string(3), "1", "b3_one");
    check_str(uint128_t{1}.to_string(36), "1", "b36_one");
    check_str(uint128_t{35}.to_string(36), "Z", "b36_35");
    check_str(uint128_t{36}.to_string(36), "10", "b36_36");
}

// ============================================================================
// Section 3: from_string round-trip (bases with prefix support)
// ============================================================================
static void test_roundtrip_fromstring()
{
    std::cout << "\n--- from_string round-trip ---\n";

    // Decimal round-trip
    {
        const uint128_t vals[] = {
            uint128_t{0}, uint128_t{1}, uint128_t{42}, uint128_t{255},
            uint128_t{0xFFFFFFFFFFFFFFFFull},
            uint128_t{1ull, 0ull},                                  // 2^64
            uint128_t{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull} // MAX
        };
        const char *labels[] = {
            "rt_dec_0", "rt_dec_1", "rt_dec_42", "rt_dec_255",
            "rt_dec_u64max", "rt_dec_2_64", "rt_dec_max128"};
        for (int i{0}; i < 7; ++i)
        {
            const std::string s{vals[i].to_string()};
            const auto parsed{uint128_t::parse_ct_safe(s.c_str())};
            check(parsed.success() && parsed.value == vals[i], labels[i]);
        }
    }

    // Hex round-trip
    {
        const uint128_t vals[] = {
            uint128_t{0}, uint128_t{1}, uint128_t{0xFF},
            uint128_t{0xDEADBEEF},
            uint128_t{0xFFFFFFFFFFFFFFFFull},
            uint128_t{0xABCDEF0123456789ull, 0xFEDCBA9876543210ull}};
        const char *labels[] = {
            "rt_hex_0", "rt_hex_1", "rt_hex_FF",
            "rt_hex_DEADBEEF", "rt_hex_u64max", "rt_hex_large"};
        for (int i{0}; i < 6; ++i)
        {
            const std::string hex_str{"0x" + vals[i].to_string(16)};
            const auto parsed{uint128_t::parse_ct_safe(hex_str.c_str())};
            check(parsed.success() && parsed.value == vals[i], labels[i]);
        }
    }

    // Binary round-trip
    {
        const uint128_t vals[] = {
            uint128_t{0}, uint128_t{1}, uint128_t{0b10101010},
            uint128_t{0xFFFF},
            uint128_t{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull}};
        const char *labels[] = {
            "rt_bin_0", "rt_bin_1", "rt_bin_AA", "rt_bin_FFFF", "rt_bin_max128"};
        for (int i{0}; i < 5; ++i)
        {
            const std::string bin_str{"0b" + vals[i].to_string(2)};
            const auto parsed{uint128_t::parse_ct_safe(bin_str.c_str())};
            check(parsed.success() && parsed.value == vals[i], labels[i]);
        }
    }

    // Octal round-trip
    {
        const uint128_t vals[] = {
            uint128_t{0}, uint128_t{1}, uint128_t{7}, uint128_t{8},
            uint128_t{0777},
            uint128_t{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull}};
        const char *labels[] = {
            "rt_oct_0", "rt_oct_1", "rt_oct_7", "rt_oct_8",
            "rt_oct_777", "rt_oct_max128"};
        for (int i{0}; i < 6; ++i)
        {
            const std::string oct_str{"0" + vals[i].to_string(8)};
            const auto parsed{uint128_t::parse_ct_safe(oct_str.c_str())};
            check(parsed.success() && parsed.value == vals[i], labels[i]);
        }
    }
}

// ============================================================================
// Section 4: Signed values across bases
// ============================================================================
static void test_signed_bases()
{
    std::cout << "\n--- Signed values across bases ---\n";

    const int128_tc_t neg{-42};
    const int128_tc_t pos{42};
    const int128_tc_t neg_one{-1};

    // Decimal
    check_str(neg.to_string(), "-42", "tc_dec_neg42");
    check_str(pos.to_string(), "42", "tc_dec_pos42");
    check_str(neg_one.to_string(), "-1", "tc_dec_neg1");

    // Hex
    check_str(neg.to_string(16), "-2A", "tc_hex_neg42");
    check_str(pos.to_string(16), "2A", "tc_hex_pos42");

    // Binary
    check_str(neg.to_string(2), "-101010", "tc_bin_neg42");
    check_str(pos.to_string(2), "101010", "tc_bin_pos42");

    // Octal
    check_str(neg.to_string(8), "-52", "tc_oct_neg42");
    check_str(pos.to_string(8), "52", "tc_oct_pos42");

    // Base 36
    check_str(neg.to_string(36), "-16", "tc_b36_neg42");
    check_str(pos.to_string(36), "16", "tc_b36_pos42");

    // Signed from_string round-trip
    {
        const std::string s{neg.to_string()};
        const auto parsed{int128_tc_t::parse_ct_safe(s.c_str())};
        check(parsed.success() && parsed.value == neg, "tc_rt_dec_neg42");
    }
    {
        const std::string s{pos.to_string()};
        const auto parsed{int128_tc_t::parse_ct_safe(s.c_str())};
        check(parsed.success() && parsed.value == pos, "tc_rt_dec_pos42");
    }
}

// ============================================================================
// Section 5: to_cstr consistency with to_string
// ============================================================================
static void test_cstr_consistency()
{
    std::cout << "\n--- to_cstr consistency ---\n";

    const uint128_t vals[] = {
        uint128_t{0}, uint128_t{1}, uint128_t{42}, uint128_t{255},
        uint128_t{0xDEADBEEF}, uint128_t{0xFFFFFFFFFFFFFFFFull},
        uint128_t{1ull, 0ull},
        uint128_t{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull}};
    const int bases[] = {2, 4, 8, 10, 16, 32};

    for (const auto &v : vals)
    {
        for (const int b : bases)
        {
            const std::string str{v.to_string(b)};
            const char *cstr{v.to_cstr(b)};
            ++total;
            if (str == cstr)
            {
                ++passed;
            }
            else
            {
                std::cout << "[FAIL] cstr_consistency val=" << v.to_string()
                          << " base=" << b << " to_string=\"" << str
                          << "\" to_cstr=\"" << cstr << "\"" << std::endl;
            }
        }
    }
    std::cout << "[INFO] to_cstr consistency: tested " << 8 * 6 << " combinations" << std::endl;
}

// ============================================================================
// Section 6: Invalid base defaults to 10
// ============================================================================
static void test_invalid_base()
{
    std::cout << "\n--- Invalid base handling ---\n";

    const uint128_t val{42};
    check_str(val.to_string(0), "42", "invalid_base_0");
    check_str(val.to_string(1), "42", "invalid_base_1");
    check_str(val.to_string(-1), "42", "invalid_base_neg");
    check_str(val.to_string(37), "42", "invalid_base_37");
    check_str(val.to_string(100), "42", "invalid_base_100");
}

// ============================================================================
// Section 7: All bases 2-36 produce non-empty output for nonzero
// ============================================================================
static void test_all_bases_nonempty()
{
    std::cout << "\n--- All bases 2-36 produce output ---\n";

    const uint128_t val{12345678};
    bool all_ok{true};
    for (int b{2}; b <= 36; ++b)
    {
        const std::string s{val.to_string(b)};
        if (s.empty())
        {
            std::cout << "[FAIL] base " << b << " produced empty string" << std::endl;
            all_ok = false;
        }
    }
    ++total;
    if (all_ok)
    {
        ++passed;
        std::cout << "[OK] All bases 2-36 produce non-empty output" << std::endl;
    }
}

// ============================================================================
// Main
// ============================================================================
int main()
{
    std::cout << "================================================================\n";
    std::cout << "Coverage Tests: to_string / from_string / to_cstr all bases\n";
    std::cout << "================================================================\n";

    test_pow2_bases_boundary();
    test_nonpow2_bases();
    test_roundtrip_fromstring();
    test_signed_bases();
    test_cstr_consistency();
    test_invalid_base();
    test_all_bases_nonempty();

    std::cout << "\n================================================================\n";
    std::cout << "Results: " << passed << "/" << total << " passed\n";
    std::cout << "================================================================\n";

    if (passed == total)
    {
        std::cout << "[OK] ALL TESTS PASSED\n";
    }
    else
    {
        std::cout << "[FAIL] " << (total - passed) << " tests FAILED\n";
    }

    return (passed == total) ? 0 : 1;
}
