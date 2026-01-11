// =============================================================================
// Priority 3: Fixed-Width Integer Representations (128-bit)
// M&S (Magnitud y Signo) & Excess-k (EK)
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2024-2026 Julián Calderón Almendros
//
// Phase 1.75 - Priority 3 Test Suite
// Tests for alternative numeric representations (M&S and Excess-k)
// Preparing foundation for generalized floating-point types
//
// =============================================================================

#include <gtest/gtest.h>
#include <cstdint>
#include <utility>
#include <limits>
#include <cmath>

// Mock implementations for demonstration (will integrate with int128_base_tt.hpp)
class TestRepresentations
{
public:
    // M&S (Magnitud y Signo) Representation
    // Bit 127: sign (0=positive, 1=negative)
    // Bits [0-126]: magnitude (absolute value)

    static std::pair<bool, uint64_t> to_ms_high(int64_t value)
    {
        bool sign = (value < 0);
        uint64_t magnitude = sign ? static_cast<uint64_t>(-value) : static_cast<uint64_t>(value);
        return {sign, magnitude};
    }

    static int64_t from_ms_high(bool sign, uint64_t magnitude)
    {
        if (magnitude == 0)
            return 0;
        return sign ? -static_cast<int64_t>(magnitude) : static_cast<int64_t>(magnitude);
    }

    // Excess-k Representation
    // bias = 2^127 - 1 for 128-bit integers
    // value_ek = value_2c + bias

    static constexpr uint64_t EK_BIAS_HIGH = 0x7FFFFFFFFFFFFFFFull; // 2^63 - 1

    static std::pair<uint64_t, uint64_t> to_excess_k(int64_t value_low, int64_t value_high)
    {
        // Simplified for 128-bit: treat as value_high part
        uint64_t ek_value_high = static_cast<uint64_t>(value_high) + EK_BIAS_HIGH;
        uint64_t ek_value_low = static_cast<uint64_t>(value_low);
        return {ek_value_high, ek_value_low};
    }

    static std::pair<int64_t, int64_t> from_excess_k(uint64_t ek_high, uint64_t ek_low)
    {
        int64_t value_high = static_cast<int64_t>(ek_high - EK_BIAS_HIGH);
        int64_t value_low = static_cast<int64_t>(ek_low);
        return {value_low, value_high};
    }

    // M&S Operations
    static std::pair<int64_t, int64_t> ms_add(
        bool sign1, int64_t mag1,
        bool sign2, int64_t mag2)
    {
        // Simple signed addition using magnitudes
        if (sign1 == sign2)
        {
            // Same sign: add magnitudes, keep sign
            int64_t result = mag1 + mag2;
            return {sign1, result};
        }
        else
        {
            // Different signs: subtract magnitudes
            if (mag1 >= mag2)
            {
                return {sign1, mag1 - mag2};
            }
            else
            {
                return {sign2, mag2 - mag1};
            }
        }
    }

    static std::pair<bool, int64_t> ms_subtract(
        bool sign1, int64_t mag1,
        bool sign2, int64_t mag2)
    {
        // a - b = a + (-b), flip sign2 and add
        bool neg_sign2 = !sign2;
        return ms_add(sign1, mag1, neg_sign2, mag2);
    }

    static std::pair<bool, int64_t> ms_multiply(
        bool sign1, int64_t mag1,
        bool sign2, int64_t mag2)
    {
        // Result sign: XOR of signs
        // Result magnitude: product of magnitudes
        bool result_sign = sign1 ^ sign2;
        int64_t result_mag = mag1 * mag2;
        return {result_sign, result_mag};
    }

    // EK Operations
    static std::pair<uint64_t, uint64_t> ek_add(
        uint64_t ek1_high, uint64_t ek1_low,
        uint64_t ek2_high, uint64_t ek2_low)
    {
        // Simplified: convert to 2C, add, convert back
        auto [val1_low, val1_high] = from_excess_k(ek1_high, ek1_low);
        auto [val2_low, val2_high] = from_excess_k(ek2_high, ek2_low);

        // Simple addition (ignoring overflow for test)
        int64_t sum_high = val1_high + val2_high;
        uint64_t sum_low = static_cast<uint64_t>(val1_low) + static_cast<uint64_t>(val2_low);

        return to_excess_k(sum_high, sum_low);
    }

    static std::pair<uint64_t, uint64_t> ek_subtract(
        uint64_t ek1_high, uint64_t ek1_low,
        uint64_t ek2_high, uint64_t ek2_low)
    {
        // a - b = a + (-b)
        auto [val2_low, val2_high] = from_excess_k(ek2_high, ek2_low);
        int64_t neg_high = -val2_high;
        uint64_t neg_low = static_cast<uint64_t>(-static_cast<int64_t>(val2_low));

        auto [neg_ek_high, neg_ek_low] = to_excess_k(neg_high, neg_low);
        return ek_add(ek1_high, ek1_low, neg_ek_high, neg_ek_low);
    }

    static std::pair<uint64_t, uint64_t> ek_multiply(
        uint64_t ek1_high, uint64_t ek1_low,
        uint64_t ek2_high, uint64_t ek2_low)
    {
        // Convert to 2C, multiply, convert back
        auto [val1_low, val1_high] = from_excess_k(ek1_high, ek1_low);
        auto [val2_low, val2_high] = from_excess_k(ek2_high, ek2_low);

        int64_t prod_high = val1_high * val2_high;
        uint64_t prod_low = static_cast<uint64_t>(val1_low) * static_cast<uint64_t>(val2_low);

        return to_excess_k(prod_high, prod_low);
    }

    static bool ek_less_than(
        uint64_t ek1_high, uint64_t ek1_low,
        uint64_t ek2_high, uint64_t ek2_low)
    {
        // Direct comparison in EK: preserves order
        if (ek1_high != ek2_high)
            return ek1_high < ek2_high;
        return ek1_low < ek2_low;
    }
};

// ============================================================================
// GROUP 1: M&S Fundamentals (5 tests)
// ============================================================================

class MSFundamentalsTest : public ::testing::Test
{
};

TEST_F(MSFundamentalsTest, sign_bit_representation)
{
    // Test sign bit extraction and magnitude
    auto [sign_pos, mag_pos] = TestRepresentations::to_ms_high(42);
    EXPECT_FALSE(sign_pos); // positive = 0
    EXPECT_EQ(mag_pos, 42);

    auto [sign_neg, mag_neg] = TestRepresentations::to_ms_high(-42);
    EXPECT_TRUE(sign_neg); // negative = 1
    EXPECT_EQ(mag_neg, 42);
}

TEST_F(MSFundamentalsTest, positive_values)
{
    for (int64_t val : {1, 42, 1000, 9999999})
    {
        auto [sign, mag] = TestRepresentations::to_ms_high(val);
        EXPECT_FALSE(sign) << "Positive should have sign=0";
        EXPECT_EQ(mag, val) << "Magnitude should equal value";
    }
}

TEST_F(MSFundamentalsTest, negative_values)
{
    for (int64_t val : {-1, -42, -1000, -9999999})
    {
        auto [sign, mag] = TestRepresentations::to_ms_high(val);
        EXPECT_TRUE(sign) << "Negative should have sign=1";
        EXPECT_EQ(mag, -val) << "Magnitude should be absolute value";
    }
}

TEST_F(MSFundamentalsTest, zero_representation)
{
    // +0 and -0 both represent zero
    auto [sign_pos, mag_pos] = TestRepresentations::to_ms_high(0);
    EXPECT_FALSE(sign_pos);
    EXPECT_EQ(mag_pos, 0);

    // In M&S, -0 would be represented as (1, 0) but converts to 0
    auto from_neg_zero = TestRepresentations::from_ms_high(true, 0);
    EXPECT_EQ(from_neg_zero, 0);
}

TEST_F(MSFundamentalsTest, magnitude_extraction)
{
    int64_t test_values[] = {-999, -1, 0, 1, 999};
    for (int64_t val : test_values)
    {
        auto [sign, mag] = TestRepresentations::to_ms_high(val);
        EXPECT_EQ(mag, std::abs(val)) << "Magnitude should be absolute value";
    }
}

// ============================================================================
// GROUP 2: M&S Conversions (5 tests)
// ============================================================================

class MSConversionsTest : public ::testing::Test
{
};

TEST_F(MSConversionsTest, from_2c_to_ms)
{
    // Test conversion from 2C to M&S
    int64_t original = 42;
    auto [sign, mag] = TestRepresentations::to_ms_high(original);
    EXPECT_FALSE(sign);
    EXPECT_EQ(mag, 42);
}

TEST_F(MSConversionsTest, from_ms_to_2c)
{
    // Test conversion from M&S back to 2C
    int64_t reconstructed = TestRepresentations::from_ms_high(false, 42);
    EXPECT_EQ(reconstructed, 42);

    reconstructed = TestRepresentations::from_ms_high(true, 42);
    EXPECT_EQ(reconstructed, -42);
}

TEST_F(MSConversionsTest, extremes_conversion)
{
    // Test with extreme values
    int64_t max_val = INT64_MAX / 2; // Avoid overflow in operations
    int64_t min_val = INT64_MIN / 2;

    auto [sign_max, mag_max] = TestRepresentations::to_ms_high(max_val);
    EXPECT_FALSE(sign_max);
    EXPECT_EQ(mag_max, max_val);

    auto [sign_min, mag_min] = TestRepresentations::to_ms_high(min_val);
    EXPECT_TRUE(sign_min);
    EXPECT_EQ(mag_min, -min_val);
}

TEST_F(MSConversionsTest, bidirectional_roundtrip)
{
    // Test 2C → M&S → 2C roundtrip
    int64_t original[] = {-9999, -1, 0, 1, 9999};
    for (int64_t val : original)
    {
        auto [sign, mag] = TestRepresentations::to_ms_high(val);
        int64_t reconstructed = TestRepresentations::from_ms_high(sign, mag);
        EXPECT_EQ(reconstructed, val) << "Roundtrip should preserve value";
    }
}

TEST_F(MSConversionsTest, mantissa_alignment)
{
    // Test that magnitude bits [0-126] correctly represent mantissa
    int64_t val = 0x123456789ABCDEFll;
    auto [sign, mag] = TestRepresentations::to_ms_high(val);
    EXPECT_EQ(mag, val);
    // In actual implementation, would verify bit positions [0-126]
}

// ============================================================================
// GROUP 3: M&S Operations (5 tests)
// ============================================================================

class MSOperationsTest : public ::testing::Test
{
};

TEST_F(MSOperationsTest, addition)
{
    // 5 + 3 = 8
    auto [sign, mag] = TestRepresentations::ms_add(false, 5, false, 3);
    EXPECT_FALSE(sign);
    EXPECT_EQ(mag, 8);

    // -5 - 3 = -8 (both negative)
    auto [sign2, mag2] = TestRepresentations::ms_add(true, 5, true, 3);
    EXPECT_TRUE(sign2);
    EXPECT_EQ(mag2, 8);

    // 5 + (-3) = 2
    auto [sign3, mag3] = TestRepresentations::ms_add(false, 5, true, 3);
    EXPECT_FALSE(sign3);
    EXPECT_EQ(mag3, 2);
}

TEST_F(MSOperationsTest, subtraction)
{
    // 5 - 3 = 2
    auto [sign, mag] = TestRepresentations::ms_subtract(false, 5, false, 3);
    EXPECT_FALSE(sign);
    EXPECT_EQ(mag, 2);

    // -5 - (-3) = -2
    auto [sign2, mag2] = TestRepresentations::ms_subtract(true, 5, true, 3);
    EXPECT_TRUE(sign2);
    EXPECT_EQ(mag2, 2);
}

TEST_F(MSOperationsTest, multiplication)
{
    // 5 * 3 = 15 (both positive)
    auto [sign, mag] = TestRepresentations::ms_multiply(false, 5, false, 3);
    EXPECT_FALSE(sign);
    EXPECT_EQ(mag, 15);

    // -5 * 3 = -15 (different signs)
    auto [sign2, mag2] = TestRepresentations::ms_multiply(true, 5, false, 3);
    EXPECT_TRUE(sign2);
    EXPECT_EQ(mag2, 15);

    // -5 * -3 = 15 (both negative)
    auto [sign3, mag3] = TestRepresentations::ms_multiply(true, 5, true, 3);
    EXPECT_FALSE(sign3); // XOR(true, true) = false
    EXPECT_EQ(mag3, 15);
}

TEST_F(MSOperationsTest, negation)
{
    // Negation flips the sign bit
    int64_t val = 42;
    auto [sign, mag] = TestRepresentations::to_ms_high(val);
    bool negated_sign = !sign; // Flip sign bit
    EXPECT_EQ(TestRepresentations::from_ms_high(negated_sign, mag), -val);
}

TEST_F(MSOperationsTest, absolute_value)
{
    // ABS in M&S is just the magnitude part with sign=0
    int64_t values[] = {-999, -1, 0, 1, 999};
    for (int64_t val : values)
    {
        auto [sign, mag] = TestRepresentations::to_ms_high(val);
        int64_t abs_result = TestRepresentations::from_ms_high(false, mag);
        EXPECT_EQ(abs_result, std::abs(val));
    }
}

// ============================================================================
// GROUP 4: Excess-k Fundamentals (5 tests)
// ============================================================================

class ExcessKFundamentalsTest : public ::testing::Test
{
};

TEST_F(ExcessKFundamentalsTest, bias_calculation)
{
    // For 128-bit: bias = 2^127 - 1
    // For 64-bit high part: bias = 2^63 - 1
    uint64_t expected_bias = 0x7FFFFFFFFFFFFFFFull; // 2^63 - 1
    EXPECT_EQ(TestRepresentations::EK_BIAS_HIGH, expected_bias);
}

TEST_F(ExcessKFundamentalsTest, standard_bias_for_128bit)
{
    // In actual 128-bit EK, bias = 2^127 - 1
    // This test uses simplified 64-bit high part
    uint64_t bias = TestRepresentations::EK_BIAS_HIGH;
    EXPECT_EQ(bias, 9223372036854775807ULL); // 2^63 - 1
}

TEST_F(ExcessKFundamentalsTest, representation)
{
    // value_ek = value_2c + bias
    int64_t value_2c = 0;
    auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, value_2c);
    // EK representation of 0 should be bias value
    EXPECT_EQ(ek_high, TestRepresentations::EK_BIAS_HIGH);
}

TEST_F(ExcessKFundamentalsTest, range_mapping)
{
    // Test range mapping: [INT64_MIN, INT64_MAX] → [0, 2^64)
    int64_t min_val = INT64_MIN / 2;
    int64_t max_val = INT64_MAX / 2;

    auto [ek_min_high, ek_min_low] = TestRepresentations::to_excess_k(0, min_val);
    auto [ek_max_high, ek_max_low] = TestRepresentations::to_excess_k(0, max_val);

    // Min should map to a low EK value
    // Max should map to a high EK value
    EXPECT_LT(ek_min_high, ek_max_high);
}

TEST_F(ExcessKFundamentalsTest, zero_representation_ek)
{
    // Zero in EK is represented as bias
    auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, 0);
    EXPECT_EQ(ek_high, TestRepresentations::EK_BIAS_HIGH);
    EXPECT_EQ(ek_low, 0);
}

// ============================================================================
// GROUP 5: Excess-k Conversions (5 tests)
// ============================================================================

class ExcessKConversionsTest : public ::testing::Test
{
};

TEST_F(ExcessKConversionsTest, from_2c_to_ek)
{
    // Test conversion 2C → EK (adds bias)
    int64_t value_2c = 42;
    auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, value_2c);

    // Verify: ek_value = 2c_value + bias
    EXPECT_EQ(ek_high, TestRepresentations::EK_BIAS_HIGH + value_2c);
}

TEST_F(ExcessKConversionsTest, from_ek_to_2c)
{
    // Test conversion EK → 2C (subtracts bias)
    uint64_t ek_high = TestRepresentations::EK_BIAS_HIGH + 42;
    uint64_t ek_low = 0;

    auto [val_low, val_high] = TestRepresentations::from_excess_k(ek_high, ek_low);
    EXPECT_EQ(val_high, 42);
    EXPECT_EQ(val_low, 0);
}

TEST_F(ExcessKConversionsTest, negative_values_ek)
{
    // Negative values in 2C should still convert correctly to EK
    int64_t value_2c = -42;
    auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, value_2c);

    // EK of -42: should be less than EK of 0
    auto [ek_zero_high, ek_zero_low] = TestRepresentations::to_excess_k(0, 0);
    EXPECT_LT(ek_high, ek_zero_high);
}

TEST_F(ExcessKConversionsTest, bidirectional_ek)
{
    // Test 2C → EK → 2C roundtrip
    int64_t values[] = {-999, -1, 0, 1, 999};
    for (int64_t val : values)
    {
        auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, val);
        auto [val_low, val_high] = TestRepresentations::from_excess_k(ek_high, ek_low);
        EXPECT_EQ(val_high, val) << "Roundtrip should preserve value";
    }
}

TEST_F(ExcessKConversionsTest, comparison_preservation)
{
    // Order must be preserved: if a < b then ek(a) < ek(b)
    int64_t a = -100;
    int64_t b = 50;

    auto [ek_a_high, ek_a_low] = TestRepresentations::to_excess_k(0, a);
    auto [ek_b_high, ek_b_low] = TestRepresentations::to_excess_k(0, b);

    EXPECT_TRUE(TestRepresentations::ek_less_than(ek_a_high, ek_a_low, ek_b_high, ek_b_low));
    EXPECT_EQ(a < b, TestRepresentations::ek_less_than(ek_a_high, ek_a_low, ek_b_high, ek_b_low));
}

// ============================================================================
// GROUP 6: EK Operations (5 tests)
// ============================================================================

class ExcessKOperationsTest : public ::testing::Test
{
};

TEST_F(ExcessKOperationsTest, ek_addition)
{
    // Convert 2C values to EK, perform EK addition, convert back
    int64_t val1 = 10, val2 = 20;
    auto [ek1_high, ek1_low] = TestRepresentations::to_excess_k(0, val1);
    auto [ek2_high, ek2_low] = TestRepresentations::to_excess_k(0, val2);

    auto [ek_sum_high, ek_sum_low] = TestRepresentations::ek_add(ek1_high, ek1_low, ek2_high, ek2_low);
    auto [result_low, result_high] = TestRepresentations::from_excess_k(ek_sum_high, ek_sum_low);

    EXPECT_EQ(result_high, 30); // 10 + 20 = 30
}

TEST_F(ExcessKOperationsTest, ek_subtraction)
{
    // EK subtraction
    int64_t val1 = 50, val2 = 30;
    auto [ek1_high, ek1_low] = TestRepresentations::to_excess_k(0, val1);
    auto [ek2_high, ek2_low] = TestRepresentations::to_excess_k(0, val2);

    auto [ek_diff_high, ek_diff_low] = TestRepresentations::ek_subtract(ek1_high, ek1_low, ek2_high, ek2_low);
    auto [result_low, result_high] = TestRepresentations::from_excess_k(ek_diff_high, ek_diff_low);

    EXPECT_EQ(result_high, 20); // 50 - 30 = 20
}

TEST_F(ExcessKOperationsTest, ek_multiplication)
{
    // EK multiplication (via 2C conversion)
    int64_t val1 = 5, val2 = 7;
    auto [ek1_high, ek1_low] = TestRepresentations::to_excess_k(0, val1);
    auto [ek2_high, ek2_low] = TestRepresentations::to_excess_k(0, val2);

    auto [ek_prod_high, ek_prod_low] = TestRepresentations::ek_multiply(ek1_high, ek1_low, ek2_high, ek2_low);
    auto [result_low, result_high] = TestRepresentations::from_excess_k(ek_prod_high, ek_prod_low);

    EXPECT_EQ(result_high, 35); // 5 * 7 = 35
}

TEST_F(ExcessKOperationsTest, ek_comparisons)
{
    // EK comparisons preserve order
    int64_t values[] = {-50, -1, 0, 1, 100};
    for (int i = 0; i < 4; i++)
    {
        auto [ek_i_high, ek_i_low] = TestRepresentations::to_excess_k(0, values[i]);
        auto [ek_j_high, ek_j_low] = TestRepresentations::to_excess_k(0, values[i + 1]);

        bool ek_less = TestRepresentations::ek_less_than(ek_i_high, ek_i_low, ek_j_high, ek_j_low);
        EXPECT_TRUE(ek_less) << values[i] << " should be less than " << values[i + 1] << " in EK";
    }
}

TEST_F(ExcessKOperationsTest, ek_equality)
{
    // EK equality: same value should have same EK representation
    int64_t val = 42;
    auto [ek1_high, ek1_low] = TestRepresentations::to_excess_k(0, val);
    auto [ek2_high, ek2_low] = TestRepresentations::to_excess_k(0, val);

    EXPECT_EQ(ek1_high, ek2_high);
    EXPECT_EQ(ek1_low, ek2_low);
}

// ============================================================================
// GROUP 7: Cross-representation Conversions & Edge Cases (5 tests)
// ============================================================================

class CrossRepresentationTest : public ::testing::Test
{
};

TEST_F(CrossRepresentationTest, all_representations_zero)
{
    // Zero should be consistent across all representations

    // 2C: 0
    int64_t val_2c = 0;

    // M&S: (0, 0)
    auto [ms_sign, ms_mag] = TestRepresentations::to_ms_high(val_2c);
    EXPECT_FALSE(ms_sign);
    EXPECT_EQ(ms_mag, 0);

    // EK: bias value
    auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, val_2c);
    EXPECT_EQ(ek_high, TestRepresentations::EK_BIAS_HIGH);
    EXPECT_EQ(ek_low, 0);
}

TEST_F(CrossRepresentationTest, all_representations_one)
{
    // +1 should be consistent across all representations
    int64_t val_2c = 1;

    // M&S: (0, 1)
    auto [ms_sign, ms_mag] = TestRepresentations::to_ms_high(val_2c);
    EXPECT_FALSE(ms_sign);
    EXPECT_EQ(ms_mag, 1);

    // EK: bias + 1
    auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, val_2c);
    EXPECT_EQ(ek_high, TestRepresentations::EK_BIAS_HIGH + 1);
}

TEST_F(CrossRepresentationTest, all_representations_minus_one)
{
    // -1 should be consistent across all representations
    int64_t val_2c = -1;

    // M&S: (1, 1)
    auto [ms_sign, ms_mag] = TestRepresentations::to_ms_high(val_2c);
    EXPECT_TRUE(ms_sign);
    EXPECT_EQ(ms_mag, 1);

    // EK: bias - 1
    auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, val_2c);
    EXPECT_EQ(ek_high, TestRepresentations::EK_BIAS_HIGH - 1);
}

TEST_F(CrossRepresentationTest, mantissa_magnitude_relationship)
{
    // M&S magnitude is the mantissa component for FP types
    // Should be extractable cleanly (bits [0-126])
    int64_t values[] = {1, 42, 999, 1000000};
    for (int64_t val : values)
    {
        auto [sign, mag] = TestRepresentations::to_ms_high(val);
        // In actual 128-bit: mag occupies bits [0-126]
        // Verify magnitude is correct
        EXPECT_EQ(mag, std::abs(val));
    }
}

TEST_F(CrossRepresentationTest, representation_boundaries)
{
    // Test boundaries and extreme values
    int64_t half_max = INT64_MAX / 2;
    int64_t half_min = INT64_MIN / 2;

    for (int64_t val : {half_min, -1000, -1, 0, 1, 1000, half_max})
    {
        // M&S conversion
        auto [ms_sign, ms_mag] = TestRepresentations::to_ms_high(val);
        int64_t from_ms = TestRepresentations::from_ms_high(ms_sign, ms_mag);
        EXPECT_EQ(from_ms, val);

        // EK conversion
        auto [ek_high, ek_low] = TestRepresentations::to_excess_k(0, val);
        auto [val_low, val_high] = TestRepresentations::from_excess_k(ek_high, ek_low);
        EXPECT_EQ(val_high, val);
    }
}

// ============================================================================
// Main entry point
// ============================================================================

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
