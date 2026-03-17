// =============================================================================
// Test: Intrinsics and Byte Operations (byteswap, endianness, carry, borrow)
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace nstd;

int g_passed{0};
int g_failed{0};

#define TEST_PASS() (++g_passed)
#define TEST_FAIL() (++g_failed)

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Intrinsics & Byte Operations Tests\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // Section 1: Low-level intrinsics (addcarry, subborrow, umul128)
    // ========================================================================

    // [Test 1] addcarry_u64 basic
    {
        std::cout << "[Test 1] addcarry_u64 basic:\n";
        const uint128_t a{0, 42};
        const uint128_t b{0, 58};
        const uint128_t sum{a + b};
        if (sum == uint128_t{0, 100})
        {
            std::cout << "  [OK] 42 + 58 = 100\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] 42 + 58 != 100\n";
            TEST_FAIL();
        }
    }

    // [Test 2] addcarry_u64 with carry propagation
    {
        std::cout << "[Test 2] addcarry_u64 carry propagation:\n";
        const uint128_t a{0, 0xFFFFFFFFFFFFFFFFull};
        const uint128_t b{0, 1};
        const uint128_t sum{a + b};
        // low overflows to 0, carry propagates to high
        if (sum == uint128_t{1, 0})
        {
            std::cout << "  [OK] carry propagation to high\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] carry propagation failed\n";
            TEST_FAIL();
        }
    }

    // [Test 3] subborrow_u64 basic
    {
        std::cout << "[Test 3] subborrow_u64 basic:\n";
        const uint128_t a{0, 100};
        const uint128_t b{0, 42};
        const uint128_t diff{a - b};
        if (diff == uint128_t{0, 58})
        {
            std::cout << "  [OK] 100 - 42 = 58\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] 100 - 42 != 58\n";
            TEST_FAIL();
        }
    }

    // [Test 4] subborrow_u64 with borrow propagation
    {
        std::cout << "[Test 4] subborrow_u64 borrow propagation:\n";
        const uint128_t a{1, 0}; // 2^64
        const uint128_t b{0, 1};
        const uint128_t diff{a - b};
        if (diff == uint128_t{0, 0xFFFFFFFFFFFFFFFFull})
        {
            std::cout << "  [OK] borrow propagation from high\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] borrow propagation failed\n";
            TEST_FAIL();
        }
    }

    // [Test 5] umul128 basic multiplication
    {
        std::cout << "[Test 5] umul128 basic multiplication:\n";
        const uint128_t a{0, 1000000};
        const uint128_t b{0, 1000000};
        const uint128_t prod{a * b};
        // 10^6 * 10^6 = 10^12 = 0xE8D4A51000
        if (prod == uint128_t{0, 1000000000000ull})
        {
            std::cout << "  [OK] 10^6 * 10^6 = 10^12\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] multiplication result wrong\n";
            TEST_FAIL();
        }
    }

    // [Test 6] umul128 overflow into high limb
    {
        std::cout << "[Test 6] umul128 overflow into high:\n";
        const uint128_t a{0, 0xFFFFFFFFull};
        const uint128_t b{0, 0xFFFFFFFFull};
        const uint128_t prod{a * b};
        // 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE00000001
        if (prod == uint128_t{0, 0xFFFFFFFE00000001ull})
        {
            std::cout << "  [OK] 0xFFFFFFFF^2 correct\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] 0xFFFFFFFF^2 wrong\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 2: Bit intrinsics (clz, ctz, popcount)
    // ========================================================================

    // [Test 7] ctz64 via trailing_zeros
    {
        std::cout << "[Test 7] ctz64 via trailing_zeros:\n";
        const uint128_t val{0, 0x80}; // bit 7 set
        bool ok{val.trailing_zeros() == 7};

        const uint128_t val2{0, 1}; // bit 0 set
        ok = ok && (val2.trailing_zeros() == 0);

        const uint128_t val3{1, 0}; // bit 64 set
        ok = ok && (val3.trailing_zeros() == 64);

        if (ok)
        {
            std::cout << "  [OK] trailing_zeros uses ctz64 correctly\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] trailing_zeros incorrect\n";
            TEST_FAIL();
        }
    }

    // [Test 8] clz64 via leading_zeros
    {
        std::cout << "[Test 8] clz64 via leading_zeros:\n";
        const uint128_t val{0, 1}; // only bit 0 set -> 127 leading zeros
        bool ok{val.leading_zeros() == 127};

        const uint128_t val2{0x8000000000000000ull, 0}; // MSB set -> 0 leading zeros
        ok = ok && (val2.leading_zeros() == 0);

        const uint128_t val3{0, 0x100}; // bit 8 set -> 119 leading zeros
        ok = ok && (val3.leading_zeros() == 119);

        if (ok)
        {
            std::cout << "  [OK] leading_zeros uses clz64 correctly\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] leading_zeros incorrect\n";
            TEST_FAIL();
        }
    }

    // [Test 9] popcount64 via popcount
    {
        std::cout << "[Test 9] popcount64 via popcount:\n";
        const uint128_t val{0, 0xFF}; // 8 bits set
        bool ok{val.popcount() == 8};

        const uint128_t val2{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull}; // all 128 bits
        ok = ok && (val2.popcount() == 128);

        const uint128_t val3{0, 0}; // zero
        ok = ok && (val3.popcount() == 0);

        const uint128_t val4{0xAAAAAAAAAAAAAAAAull, 0x5555555555555555ull}; // alternating
        ok = ok && (val4.popcount() == 64);

        if (ok)
        {
            std::cout << "  [OK] popcount uses popcount64 correctly\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] popcount incorrect\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 3: get_byte / set_byte
    // ========================================================================

    // [Test 10] get_byte basic
    {
        std::cout << "[Test 10] get_byte basic:\n";
        const uint128_t val{0, 0x0102030405060708ull};
        bool ok{true};
        ok = ok && (val.get_byte(0) == std::byte{0x08}); // LSB
        ok = ok && (val.get_byte(1) == std::byte{0x07});
        ok = ok && (val.get_byte(7) == std::byte{0x01});
        ok = ok && (val.get_byte(8) == std::byte{0x00}); // high limb is 0
        ok = ok && (val.get_byte(15) == std::byte{0x00});

        if (ok)
        {
            std::cout << "  [OK] get_byte extracts correct bytes\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] get_byte wrong\n";
            TEST_FAIL();
        }
    }

    // [Test 11] set_byte basic
    {
        std::cout << "[Test 11] set_byte basic:\n";
        uint128_t val{0, 0};
        val.set_byte(0, std::byte{0xAB});
        val.set_byte(15, std::byte{0xCD});

        bool ok{val.get_byte(0) == std::byte{0xAB}};
        ok = ok && (val.get_byte(15) == std::byte{0xCD});
        ok = ok && (val.get_byte(1) == std::byte{0x00}); // unchanged

        if (ok)
        {
            std::cout << "  [OK] set_byte modifies correct bytes\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] set_byte wrong\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 4: byteswap()
    // ========================================================================

    // [Test 12] byteswap identity (double byteswap = original)
    {
        std::cout << "[Test 12] byteswap double-swap identity:\n";
        const uint128_t val{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const uint128_t swapped{val.byteswap()};
        const uint128_t restored{swapped.byteswap()};

        if (restored == val)
        {
            std::cout << "  [OK] byteswap(byteswap(x)) == x\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] double byteswap not identity\n";
            TEST_FAIL();
        }
    }

    // [Test 13] byteswap known value
    {
        std::cout << "[Test 13] byteswap known value:\n";
        // val = high:0x0000000000000001, low:0x0000000000000000
        // In memory (LE): 00 00 00 00 00 00 00 00 | 01 00 00 00 00 00 00 00
        // After byteswap: byte[0] should become byte[15], etc.
        const uint128_t val{1, 0};
        const uint128_t swapped{val.byteswap()};

        // After swap: the 0x01 byte (at position 8) goes to position 7
        // high limb gets swapped low limb = bswap64(0) = 0
        // low limb gets swapped high limb = bswap64(1) = 0x0100000000000000
        if (swapped == uint128_t{0, 0x0100000000000000ull})
        {
            std::cout << "  [OK] byteswap({1,0}) correct\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] byteswap({1,0}) wrong\n";
            std::cout << "    high=0x" << std::hex << swapped.high() << " low=0x" << swapped.low() << std::dec << "\n";
            TEST_FAIL();
        }
    }

    // [Test 14] byteswap of zero
    {
        std::cout << "[Test 14] byteswap of zero:\n";
        const uint128_t zero{0, 0};
        if (zero.byteswap() == zero)
        {
            std::cout << "  [OK] byteswap(0) == 0\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] byteswap(0) != 0\n";
            TEST_FAIL();
        }
    }

    // [Test 15] byteswap of max
    {
        std::cout << "[Test 15] byteswap of max:\n";
        const uint128_t max_val{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        if (max_val.byteswap() == max_val)
        {
            std::cout << "  [OK] byteswap(MAX) == MAX\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] byteswap(MAX) != MAX\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 5: to/from big-endian
    // ========================================================================

    // [Test 16] to_big_endian / from_big_endian roundtrip
    {
        std::cout << "[Test 16] big-endian roundtrip:\n";
        const uint128_t val{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const auto be_bytes{val.to_big_endian()};
        const uint128_t restored{uint128_t::from_big_endian(be_bytes)};

        if (restored == val)
        {
            std::cout << "  [OK] from_big_endian(to_big_endian(x)) == x\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] big-endian roundtrip failed\n";
            TEST_FAIL();
        }
    }

    // [Test 17] to_big_endian byte order (MSB first)
    {
        std::cout << "[Test 17] big-endian byte order:\n";
        // val = 1 (in low limb)
        // big-endian: MSB first -> first 15 bytes are 0x00, last byte is 0x01
        const uint128_t val{0, 1};
        const auto be_bytes{val.to_big_endian()};

        bool ok{be_bytes[15] == std::byte{0x01}};
        for (int i{0}; i < 15; ++i)
        {
            ok = ok && (be_bytes[i] == std::byte{0x00});
        }

        if (ok)
        {
            std::cout << "  [OK] value=1 has 0x01 at byte[15] in BE\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] BE byte order wrong for value=1\n";
            TEST_FAIL();
        }
    }

    // [Test 18] to_big_endian with high value
    {
        std::cout << "[Test 18] big-endian high value:\n";
        // val with high=0xFF, low=0 -> BE byte[0] should be 0x00...00FF at pos 7
        const uint128_t val{0xFF, 0};
        const auto be_bytes{val.to_big_endian()};

        // high=0xFF -> in BE, byte[7] = 0xFF (after 7 zero bytes from high limb)
        if (be_bytes[7] == std::byte{0xFF} && be_bytes[0] == std::byte{0x00})
        {
            std::cout << "  [OK] high byte correct in BE\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] high byte wrong in BE\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 6: to/from little-endian
    // ========================================================================

    // [Test 19] to_little_endian / from_little_endian roundtrip
    {
        std::cout << "[Test 19] little-endian roundtrip:\n";
        const uint128_t val{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const auto le_bytes{val.to_little_endian()};
        const uint128_t restored{uint128_t::from_little_endian(le_bytes)};

        if (restored == val)
        {
            std::cout << "  [OK] from_little_endian(to_little_endian(x)) == x\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] little-endian roundtrip failed\n";
            TEST_FAIL();
        }
    }

    // [Test 20] to_little_endian byte order (LSB first)
    {
        std::cout << "[Test 20] little-endian byte order:\n";
        // val = 1 -> LE: byte[0] = 0x01, rest = 0x00
        const uint128_t val{0, 1};
        const auto le_bytes{val.to_little_endian()};

        bool ok{le_bytes[0] == std::byte{0x01}};
        for (int i{1}; i < 16; ++i)
        {
            ok = ok && (le_bytes[i] == std::byte{0x00});
        }

        if (ok)
        {
            std::cout << "  [OK] value=1 has 0x01 at byte[0] in LE\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] LE byte order wrong for value=1\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 7: BE/LE consistency
    // ========================================================================

    // [Test 21] BE and LE are reverse of each other
    {
        std::cout << "[Test 21] BE and LE are reverses:\n";
        const uint128_t val{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const auto be_bytes{val.to_big_endian()};
        const auto le_bytes{val.to_little_endian()};

        bool ok{true};
        for (int i{0}; i < 16; ++i)
        {
            ok = ok && (be_bytes[i] == le_bytes[15 - i]);
        }

        if (ok)
        {
            std::cout << "  [OK] BE[i] == LE[15-i] for all i\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] BE and LE not reverses\n";
            TEST_FAIL();
        }
    }

    // [Test 22] Cross-endian: from_big_endian(to_little_endian(x)) == byteswap(x)
    {
        std::cout << "[Test 22] cross-endian consistency:\n";
        const uint128_t val{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const auto le_bytes{val.to_little_endian()};
        const uint128_t from_be{uint128_t::from_big_endian(le_bytes)};
        const uint128_t swapped{val.byteswap()};

        if (from_be == swapped)
        {
            std::cout << "  [OK] from_BE(to_LE(x)) == byteswap(x)\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] cross-endian consistency broken\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 8: Signed types (TC) intrinsics
    // ========================================================================

    // [Test 23] TC addition via intrinsics
    {
        std::cout << "[Test 23] TC addition (intrinsics path):\n";
        const int128_t a{0, 100};
        const int128_t b{0, 200};
        const int128_t sum{a + b};
        if (sum == int128_t{0, 300})
        {
            std::cout << "  [OK] TC 100 + 200 = 300\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] TC addition wrong\n";
            TEST_FAIL();
        }
    }

    // [Test 24] TC subtraction with borrow
    {
        std::cout << "[Test 24] TC subtraction with borrow:\n";
        const int128_t a{1, 0}; // 2^64
        const int128_t b{0, 1};
        const int128_t diff{a - b};
        if (diff == int128_t{0, 0xFFFFFFFFFFFFFFFFull})
        {
            std::cout << "  [OK] TC borrow propagation\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] TC subtraction borrow wrong\n";
            TEST_FAIL();
        }
    }

    // [Test 25] TC byteswap roundtrip
    {
        std::cout << "[Test 25] TC byteswap roundtrip:\n";
        const int128_t val{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const int128_t restored{val.byteswap().byteswap()};
        if (restored == val)
        {
            std::cout << "  [OK] TC byteswap(byteswap(x)) == x\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] TC double byteswap failed\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Section 9: Edge cases
    // ========================================================================

    // [Test 26] Large multiplication (cross-limb)
    {
        std::cout << "[Test 26] large multiplication cross-limb:\n";
        const uint128_t a{0, 0x100000000ull}; // 2^32
        const uint128_t b{0, 0x100000000ull}; // 2^32
        const uint128_t prod{a * b};
        // 2^32 * 2^32 = 2^64 -> high=1, low=0
        if (prod == uint128_t{1, 0})
        {
            std::cout << "  [OK] 2^32 * 2^32 = 2^64\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] cross-limb multiplication wrong\n";
            TEST_FAIL();
        }
    }

    // [Test 27] Addition near overflow
    {
        std::cout << "[Test 27] addition near overflow:\n";
        const uint128_t max_val{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        const uint128_t one{0, 1};
        const uint128_t wrapped{max_val + one};
        // Should wrap to 0
        if (wrapped == uint128_t{0, 0})
        {
            std::cout << "  [OK] MAX + 1 wraps to 0\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] overflow wrap incorrect\n";
            TEST_FAIL();
        }
    }

    // [Test 28] byteswap preserves get_byte relationship
    {
        std::cout << "[Test 28] byteswap vs get_byte:\n";
        const uint128_t val{0x0123456789ABCDEFull, 0xFEDCBA9876543210ull};
        const uint128_t swapped{val.byteswap()};

        bool ok{true};
        for (int i{0}; i < 16; ++i)
        {
            ok = ok && (val.get_byte(i) == swapped.get_byte(15 - i));
        }

        if (ok)
        {
            std::cout << "  [OK] val.get_byte(i) == swapped.get_byte(15-i)\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] byteswap/get_byte inconsistency\n";
            TEST_FAIL();
        }
    }

    // ========================================================================
    // Summary
    // ========================================================================

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed"
              << " (total: " << (g_passed + g_failed) << ")\n";
    std::cout << "====================================================================\n";

    if (g_failed > 0)
    {
        std::cout << "[FAIL] Some tests failed!\n";
        return 1;
    }

    std::cout << "All tests passed.\n";
    return 0;
}
