// =============================================================================
// Test: Priority 11 - Array & Bitset Conversions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "../include/int128_parameterized.hpp"
#include <iostream>
#include <iomanip>
#include <array>
#include <bitset>
#include <vector>
#include <cstddef>

using namespace nstd;

// Test framework macros
#define TEST(name)                                                                        \
    static bool test_##name();                                                            \
    static bool test_##name##_registered = (tests.push_back({#name, test_##name}), true); \
    static bool test_##name()

struct TestCase
{
    const char *name;
    bool (*func)();
};

static std::vector<TestCase> tests;

// ========================================================================
// Test: std::array<std::byte, 16> Conversions
// ========================================================================

TEST(to_array_simple_tc)
{
    uint128_tc_t x{0x12, 0x34};
    const auto bytes{static_cast<std::array<std::byte, 16>>(x)};

    // Check low limb (bytes [0..7])
    bool ok{true};
    ok = ok && (bytes[0] == std::byte{0x12});
    for (int i{1}; i < 8; ++i)
    {
        ok = ok && (bytes[i] == std::byte{0x00});
    }

    // Check high limb (bytes [8..15])
    ok = ok && (bytes[8] == std::byte{0x34});
    for (int i{9}; i < 16; ++i)
    {
        ok = ok && (bytes[i] == std::byte{0x00});
    }

    return ok;
}

TEST(to_array_full_bytes_tc)
{
    uint128_tc_t x{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
    const auto bytes{static_cast<std::array<std::byte, 16>>(x)};

    // All bytes should be 0xFF
    bool ok{true};
    for (int i{0}; i < 16; ++i)
    {
        ok = ok && (bytes[i] == std::byte{0xFF});
    }

    return ok;
}

TEST(from_array_simple_tc)
{
    std::array<std::byte, 16> bytes{};
    bytes[0] = std::byte{0xAB};
    bytes[1] = std::byte{0xCD};
    bytes[8] = std::byte{0xEF};
    bytes[9] = std::byte{0x12};

    const uint128_tc_t x{bytes};

    // Expected: low = 0xCDAB, high = 0x12EF
    return (x.low() == 0xCDABULL) && (x.high() == 0x12EFULL);
}

TEST(from_array_full_bytes_tc)
{
    std::array<std::byte, 16> bytes{};
    for (int i{0}; i < 16; ++i)
    {
        bytes[i] = std::byte{0xFF};
    }

    const uint128_tc_t x{bytes};

    return (x.low() == 0xFFFFFFFFFFFFFFFFULL) &&
           (x.high() == 0xFFFFFFFFFFFFFFFFULL);
}

TEST(roundtrip_array_tc)
{
    uint128_tc_t original{0x123456789ABCDEFULL, 0xFEDCBA987654321ULL};

    // Convert to array and back
    const auto bytes{static_cast<std::array<std::byte, 16>>(original)};
    const uint128_tc_t reconstructed{bytes};

    return (reconstructed.low() == original.low()) &&
           (reconstructed.high() == original.high());
}

TEST(to_array_ms_negative)
{
    int128_ms_t x{0, 0};
    x.set_high(1ULL << 63); // Sign bit set
    x.set_low(42);          // Magnitude 42

    const auto bytes{static_cast<std::array<std::byte, 16>>(x)};

    // Check low limb has 42
    bool ok{bytes[0] == std::byte{42}};

    // Check high limb has sign bit (byte[15] MSB = 1)
    ok = ok && ((bytes[15] & std::byte{0x80}) == std::byte{0x80});

    return ok;
}

// ========================================================================
// Test: std::bitset<128> Conversions
// ========================================================================

TEST(to_bitset_simple_tc)
{
    uint128_tc_t x{0xFF, 0x00}; // Low = 0xFF, High = 0
    const auto bits{static_cast<std::bitset<128>>(x)};

    // First 8 bits should be set
    bool ok{true};
    for (int i{0}; i < 8; ++i)
    {
        ok = ok && bits.test(i);
    }

    // Remaining bits should be clear
    for (int i{8}; i < 128; ++i)
    {
        ok = ok && !bits.test(i);
    }

    return ok;
}

TEST(to_bitset_high_limb_tc)
{
    uint128_tc_t x{0x00, 0xFF}; // Low = 0, High = 0xFF
    const auto bits{static_cast<std::bitset<128>>(x)};

    // First 64 bits should be clear
    bool ok{true};
    for (int i{0}; i < 64; ++i)
    {
        ok = ok && !bits.test(i);
    }

    // Bits 64..71 should be set
    for (int i{64}; i < 72; ++i)
    {
        ok = ok && bits.test(i);
    }

    // Remaining high bits should be clear
    for (int i{72}; i < 128; ++i)
    {
        ok = ok && !bits.test(i);
    }

    return ok;
}

TEST(from_bitset_simple_tc)
{
    std::bitset<128> bits{};
    bits.set(0);   // LSB
    bits.set(63);  // MSB of low limb
    bits.set(64);  // LSB of high limb
    bits.set(127); // MSB of high limb

    const uint128_tc_t x{bits};

    const uint64_t expected_low{1ULL | (1ULL << 63)};
    const uint64_t expected_high{1ULL | (1ULL << 63)};

    return (x.low() == expected_low) && (x.high() == expected_high);
}

TEST(from_bitset_all_ones_tc)
{
    std::bitset<128> bits{};
    bits.set(); // Set all bits to 1

    const uint128_tc_t x{bits};

    return (x.low() == 0xFFFFFFFFFFFFFFFFULL) &&
           (x.high() == 0xFFFFFFFFFFFFFFFFULL);
}

TEST(roundtrip_bitset_tc)
{
    uint128_tc_t original{0x0F0F0F0F0F0F0F0FULL, 0xF0F0F0F0F0F0F0F0ULL};

    // Convert to bitset and back
    const auto bits{static_cast<std::bitset<128>>(original)};
    const uint128_tc_t reconstructed{bits};

    return (reconstructed.low() == original.low()) &&
           (reconstructed.high() == original.high());
}

TEST(to_bitset_ms_negative)
{
    int128_ms_t x{0, 0};
    x.set_high(1ULL << 63); // Sign bit set
    x.set_low(1);           // Magnitude 1

    const auto bits{static_cast<std::bitset<128>>(x)};

    // Bit 0 should be set (magnitude LSB)
    // Bit 127 should be set (sign bit)
    return bits.test(0) && bits.test(127);
}

// ========================================================================
// Test: Mixed Conversions (Array ↔ Bitset)
// ========================================================================

TEST(array_to_bitset_consistency)
{
    uint128_tc_t x{0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL};

    // Convert to array then to bitset
    const auto bytes{static_cast<std::array<std::byte, 16>>(x)};
    const uint128_tc_t from_bytes{bytes};
    const auto bits1{static_cast<std::bitset<128>>(from_bytes)};

    // Convert directly to bitset
    const auto bits2{static_cast<std::bitset<128>>(x)};

    // Both bitsets should be identical
    return bits1 == bits2;
}

TEST(zero_value_conversions)
{
    uint128_tc_t zero{0, 0};

    // To array
    const auto bytes{static_cast<std::array<std::byte, 16>>(zero)};
    bool ok{true};
    for (int i{0}; i < 16; ++i)
    {
        ok = ok && (bytes[i] == std::byte{0x00});
    }

    // To bitset
    const auto bits{static_cast<std::bitset<128>>(zero)};
    ok = ok && (bits.none()); // All bits should be 0

    return ok;
}

TEST(max_value_conversions)
{
    uint128_tc_t max{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

    // To array
    const auto bytes{static_cast<std::array<std::byte, 16>>(max)};
    bool ok{true};
    for (int i{0}; i < 16; ++i)
    {
        ok = ok && (bytes[i] == std::byte{0xFF});
    }

    // To bitset
    const auto bits{static_cast<std::bitset<128>>(max)};
    ok = ok && (bits.all()); // All bits should be 1

    return ok;
}

// ========================================================================
// Main Test Runner
// ========================================================================

int main()
{
    std::cout << "\nPriority 11: Array & Bitset Conversion Tests\n";
    std::cout << "========================================\n\n";

    int passed{0};
    int failed{0};

    for (const auto &test : tests)
    {
        std::cout << "Running: " << std::setw(40) << std::left << test.name << " ... ";
        std::cout.flush();

        try
        {
            if (test.func())
            {
                std::cout << "[OK]\n";
                ++passed;
            }
            else
            {
                std::cout << "[FAIL]\n";
                ++failed;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "[EXCEPTION: " << e.what() << "]\n";
            ++failed;
        }
    }

    std::cout << "\nTest Summary:\n";
    std::cout << "  Total:  " << (passed + failed) << "\n";
    std::cout << "  Passed: " << passed << " [OK]\n";
    std::cout << "  Failed: " << failed << "\n";

    if (failed == 0)
    {
        std::cout << "\nAll tests passed!\n";
        return 0;
    }
    else
    {
        std::cout << "\nSome tests failed.\n";
        return 1;
    }
}
