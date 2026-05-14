// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: std::array<std::byte,16> and std::bitset<128> conversions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
//
// Consolidates: test_priority11_array
// =============================================================================

#include "int128_parameterized.hpp"

#include <iostream>
#include <array>
#include <bitset>
#include <cstddef>

using namespace nstd;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                                                    \
    do {                                                                    \
        if (cond) { std::cout << "[OK] " << (name) << "\n"; ++g_passed; }  \
        else      { std::cout << "[FAIL] " << (name) << "\n"; ++g_failed; }\
    } while (false)

// =============================================================================
// Section 1: to array<byte,16>
// =============================================================================
static void test_to_array()
{
    std::cout << "\n--- Section 1: to array<byte,16> ---\n";

    // Small value: low[0]==0x12, high[8]==0x34
    { uint128_t x{0x34, 0x12};
      const auto b{static_cast<std::array<std::byte, 16>>(x)};
      bool ok = (b[0] == std::byte{0x12}) && (b[8] == std::byte{0x34});
      for (int i = 1; i < 8  && ok; ++i) ok = (b[i] == std::byte{0x00});
      for (int i = 9; i < 16 && ok; ++i) ok = (b[i] == std::byte{0x00});
      TEST("to_array low=0x12 high=0x34", ok); }

    // All 0xFF bytes
    { uint128_t x{~0ULL, ~0ULL};
      const auto b{static_cast<std::array<std::byte, 16>>(x)};
      bool ok = true;
      for (int i = 0; i < 16; ++i) ok = ok && (b[i] == std::byte{0xFF});
      TEST("to_array all 0xFF", ok); }

    // MS negative: low byte 42, high byte 15 has sign bit set
    { int128_ms_t x{0, 0}; x.set_high(1ULL << 63); x.set_low(42);
      const auto b{static_cast<std::array<std::byte, 16>>(x)};
      bool ok = (b[0] == std::byte{42}) && ((b[15] & std::byte{0x80}) == std::byte{0x80});
      TEST("to_array ms_negative sign+mag42", ok); }
}

// =============================================================================
// Section 2: from array<byte,16>
// =============================================================================
static void test_from_array()
{
    std::cout << "\n--- Section 2: from array<byte,16> ---\n";

    // bytes[0]=0xAB bytes[1]=0xCD → low=0xCDAB; bytes[8]=0xEF bytes[9]=0x12 → high=0x12EF
    { std::array<std::byte, 16> b{};
      b[0] = std::byte{0xAB}; b[1] = std::byte{0xCD};
      b[8] = std::byte{0xEF}; b[9] = std::byte{0x12};
      const uint128_t x{b};
      TEST("from_array low=0xCDAB high=0x12EF",
           x.low() == 0xCDABULL && x.high() == 0x12EFULL); }

    // All 0xFF → low=high=~0
    { std::array<std::byte, 16> b{};
      for (int i = 0; i < 16; ++i) b[i] = std::byte{0xFF};
      const uint128_t x{b};
      TEST("from_array all 0xFF",
           x.low() == ~0ULL && x.high() == ~0ULL); }
}

// =============================================================================
// Section 3: array roundtrip
// =============================================================================
static void test_array_roundtrip()
{
    std::cout << "\n--- Section 3: array roundtrip ---\n";

    { uint128_t orig{0x123456789ABCDEFULL, 0xFEDCBA987654321ULL};
      const auto b{static_cast<std::array<std::byte, 16>>(orig)};
      const uint128_t back{b};
      TEST("array roundtrip arbitrary",
           back.low() == orig.low() && back.high() == orig.high()); }

    // Zero roundtrip
    { uint128_t orig{0, 0};
      const auto b{static_cast<std::array<std::byte, 16>>(orig)};
      TEST("array zero all bytes zero",
           [&]{ for (int i=0;i<16;++i) if (b[i]!=std::byte{0}) return false; return true; }());
      const uint128_t back{b};
      TEST("array zero roundtrip", back.is_zero()); }
}

// =============================================================================
// Section 4: to bitset<128>
// =============================================================================
static void test_to_bitset()
{
    std::cout << "\n--- Section 4: to bitset<128> ---\n";

    // low=0xFF: bits 0..7 set, rest clear
    { uint128_t x{0x00, 0xFF};
      const auto bits{static_cast<std::bitset<128>>(x)};
      bool ok = true;
      for (int i =  0; i <   8; ++i) ok = ok &&  bits.test(i);
      for (int i =  8; i < 128; ++i) ok = ok && !bits.test(i);
      TEST("to_bitset low=0xFF bits 0..7", ok); }

    // high=0xFF: bits 64..71 set, low bits clear
    { uint128_t x{0xFF, 0x00};
      const auto bits{static_cast<std::bitset<128>>(x)};
      bool ok = true;
      for (int i =  0; i <  64; ++i) ok = ok && !bits.test(i);
      for (int i = 64; i <  72; ++i) ok = ok &&  bits.test(i);
      for (int i = 72; i < 128; ++i) ok = ok && !bits.test(i);
      TEST("to_bitset high=0xFF bits 64..71", ok); }

    // MS negative: bit 0 (mag LSB) and bit 127 (sign) set
    { int128_ms_t x{0, 0}; x.set_high(1ULL << 63); x.set_low(1);
      const auto bits{static_cast<std::bitset<128>>(x)};
      TEST("to_bitset ms_negative bit0 and bit127", bits.test(0) && bits.test(127)); }
}

// =============================================================================
// Section 5: from bitset<128>
// =============================================================================
static void test_from_bitset()
{
    std::cout << "\n--- Section 5: from bitset<128> ---\n";

    // Set bits 0, 63, 64, 127
    { std::bitset<128> bits{};
      bits.set(0); bits.set(63); bits.set(64); bits.set(127);
      const uint128_t x{bits};
      const uint64_t exp_low  = 1ULL | (1ULL << 63);
      const uint64_t exp_high = 1ULL | (1ULL << 63);
      TEST("from_bitset bits 0/63/64/127",
           x.low() == exp_low && x.high() == exp_high); }

    // All bits set
    { std::bitset<128> bits{};
      bits.set();
      const uint128_t x{bits};
      TEST("from_bitset all ones", x.low() == ~0ULL && x.high() == ~0ULL); }
}

// =============================================================================
// Section 6: bitset roundtrip
// =============================================================================
static void test_bitset_roundtrip()
{
    std::cout << "\n--- Section 6: bitset roundtrip ---\n";

    { uint128_t orig{0x0F0F0F0F0F0F0F0FULL, 0xF0F0F0F0F0F0F0F0ULL};
      const auto bits{static_cast<std::bitset<128>>(orig)};
      const uint128_t back{bits};
      TEST("bitset roundtrip alternating",
           back.low() == orig.low() && back.high() == orig.high()); }
}

// =============================================================================
// Section 7: Mixed array ↔ bitset consistency and boundary values
// =============================================================================
static void test_mixed_and_boundaries()
{
    std::cout << "\n--- Section 7: mixed array<->bitset and boundaries ---\n";

    // array→uint128→bitset must equal direct uint128→bitset
    { uint128_t x{0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL};
      const auto bytes{static_cast<std::array<std::byte, 16>>(x)};
      const uint128_t from_bytes{bytes};
      const auto bits1{static_cast<std::bitset<128>>(from_bytes)};
      const auto bits2{static_cast<std::bitset<128>>(x)};
      TEST("array->bitset == direct bitset", bits1 == bits2); }

    // Zero value: all array bytes zero, bitset.none()
    { uint128_t zero{0, 0};
      const auto bytes{static_cast<std::array<std::byte, 16>>(zero)};
      bool ok = true;
      for (int i = 0; i < 16; ++i) ok = ok && (bytes[i] == std::byte{0x00});
      const auto bits{static_cast<std::bitset<128>>(zero)};
      TEST("zero value array all zero",  ok);
      TEST("zero value bitset none()",   bits.none()); }

    // Max value: all array bytes 0xFF, bitset.all()
    { uint128_t max{~0ULL, ~0ULL};
      const auto bytes{static_cast<std::array<std::byte, 16>>(max)};
      bool ok = true;
      for (int i = 0; i < 16; ++i) ok = ok && (bytes[i] == std::byte{0xFF});
      const auto bits{static_cast<std::bitset<128>>(max)};
      TEST("max value array all 0xFF",   ok);
      TEST("max value bitset all()",     bits.all()); }
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "================================================================\n";
    std::cout << "  test_param_array: array<byte,16> and bitset<128> conversions\n";
    std::cout << "================================================================\n";

    test_to_array();
    test_from_array();
    test_array_roundtrip();
    test_to_bitset();
    test_from_bitset();
    test_bitset_roundtrip();
    test_mixed_and_boundaries();

    std::cout << "\n================================================================\n";
    std::cout << "  RESULTS: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "================================================================\n";

    return g_failed ? 1 : 0;
}
