// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Float/Double/Long Double Constructors with EK support
// =============================================================================

#include "representation.hpp"
#include "int128_parameterized.hpp"
#include <iostream>
#include <cmath>

using namespace nstd;

int test_count = 0, pass_count = 0;

#define TEST(name, condition)                              \
    do                                                     \
    {                                                      \
        test_count++;                                      \
        if (condition)                                     \
        {                                                  \
            pass_count++;                                  \
            std::cout << "  [OK] " << name << std::endl;   \
        }                                                  \
        else                                               \
        {                                                  \
            std::cout << "  [FAIL] " << name << std::endl; \
        }                                                  \
    } while (false)

int main()
{
    std::cout << "====================================================================" << std::endl;
    std::cout << "Float/Double/Long Double Constructor Tests" << std::endl;
    std::cout << "====================================================================" << std::endl;

    // ========================================================================
    // Group 1: Float Constructor (TC, MS, EK)
    // ========================================================================
    std::cout << "\n[Group 1] Float Constructor:" << std::endl;

    // TC from float
    int128_tc_t tc_from_float{42.7f};
    TEST("tc_from_float_truncate", tc_from_float.to_string() == "42");

    int128_tc_t tc_neg_float{-99.3f};
    TEST("tc_from_neg_float", tc_neg_float.to_string() == "-99");

    // MS from float
    int128_ms_t ms_from_float{100.9f};
    TEST("ms_from_float", ms_from_float.to_string() == "100");

    int128_ms_t ms_neg_float{-50.1f};
    TEST("ms_from_neg_float", ms_neg_float.to_string() == "-50");

    // EK from float
    int128_ek_t ek_from_float{10.5f};
    TEST("ek_from_float", ek_from_float.to_string() == "10");

    int128_ek_t ek_neg_float{-20.8f};
    TEST("ek_from_neg_float", ek_neg_float.to_string() == "-20");

    // ========================================================================
    // Group 2: Double Constructor (TC, MS, EK)
    // ========================================================================
    std::cout << "\n[Group 2] Double Constructor:" << std::endl;

    // TC from double
    int128_tc_t tc_from_double{123.456};
    TEST("tc_from_double", tc_from_double.to_string() == "123");

    int128_tc_t tc_large_double{1234567890123.0};
    TEST("tc_from_large_double", tc_large_double.to_string() == "1234567890123");

    // MS from double
    int128_ms_t ms_from_double{999.999};
    TEST("ms_from_double", ms_from_double.to_string() == "999");

    int128_ms_t ms_neg_double{-777.777};
    TEST("ms_from_neg_double", ms_neg_double.to_string() == "-777");

    // EK from double
    int128_ek_t ek_from_double{50.0};
    TEST("ek_from_double", ek_from_double.to_string() == "50");

    int128_ek_t ek_neg_double{-100.0};
    TEST("ek_from_neg_double", ek_neg_double.to_string() == "-100");

    // ========================================================================
    // Group 3: Long Double Constructor (TC, MS, EK)
    // ========================================================================
    std::cout << "\n[Group 3] Long Double Constructor:" << std::endl;

    // TC from long double
    int128_tc_t tc_from_ldouble{987654321.123L};
    TEST("tc_from_ldouble", tc_from_ldouble.to_string() == "987654321");

    // MS from long double
    int128_ms_t ms_from_ldouble{555555.555L};
    TEST("ms_from_ldouble", ms_from_ldouble.to_string() == "555555");

    // EK from long double
    int128_ek_t ek_from_ldouble{300.0L};
    TEST("ek_from_ldouble", ek_from_ldouble.to_string() == "300");

    int128_ek_t ek_neg_ldouble{-150.0L};
    TEST("ek_from_neg_ldouble", ek_neg_ldouble.to_string() == "-150");

    // ========================================================================
    // Group 4: Special Values (NaN, Overflow)
    // ========================================================================
    std::cout << "\n[Group 4] Special Values:" << std::endl;

    // NaN handling
    double nan_val = std::nan("");
    int128_tc_t tc_from_nan{nan_val};
    TEST("tc_from_nan_is_zero", tc_from_nan.to_string() == "0");

    int128_ek_t ek_from_nan{nan_val};
    TEST("ek_from_nan_is_zero", ek_from_nan.to_string() == "0");

    // Overflow handling (values too large)
    double huge_val = 1e40; // Much larger than 2^128
    int128_tc_t tc_overflow{huge_val};
    // Should saturate to max value
    TEST("tc_overflow_saturates", tc_overflow.high() == ~0ULL && tc_overflow.low() == ~0ULL);

    // ========================================================================
    // Group 5: EK-Specific Validation
    // ========================================================================
    std::cout << "\n[Group 5] EK-Specific Validation:" << std::endl;

    // Verify bias is correctly applied
    int128_ek_t ek_zero{0.0};
    TEST("ek_zero_has_bias", ek_zero.high() == (1ULL << 62) && ek_zero.low() == 0ULL);

    int128_ek_t ek_one{1.0};
    TEST("ek_one_has_bias_plus_one", ek_one.high() == (1ULL << 62) && ek_one.low() == 1ULL);

    int128_ek_t ek_minus_one{-1.0};
    // -1 in EK: bias - 1 = (1<<62) - 1 in high, with borrow
    TEST("ek_minus_one_correct", ek_minus_one.to_string() == "-1");

    // Arithmetic should work correctly
    int128_ek_t ek_a{10.5}; // Truncates to 10
    int128_ek_t ek_b{20.3}; // Truncates to 20
    auto ek_sum = ek_a + ek_b;
    TEST("ek_arithmetic_after_float_construction", ek_sum.to_string() == "30");

    // ========================================================================
    // Group 6: Zero and Fractional Truncation
    // ========================================================================
    std::cout << "\n[Group 6] Zero and Fractional Truncation:" << std::endl;

    int128_tc_t tc_zero_f{0.0f};
    TEST("tc_zero_from_float", tc_zero_f.to_string() == "0");

    int128_ms_t ms_zero_d{0.0};
    TEST("ms_zero_from_double", ms_zero_d.to_string() == "0");

    int128_tc_t tc_frac{123.999};
    TEST("tc_truncate_fraction", tc_frac.to_string() == "123");

    int128_tc_t tc_neg_frac{-456.001};
    TEST("tc_truncate_neg_fraction", tc_neg_frac.to_string() == "-456");

    // ========================================================================
    // Results
    // ========================================================================
    std::cout << "\n====================================================================" << std::endl;
    std::cout << "RESULTS:" << std::endl;
    std::cout << "  Passed: " << pass_count << std::endl;
    std::cout << "  Failed: " << (test_count - pass_count) << std::endl;
    std::cout << "  Total:  " << test_count << std::endl;
    std::cout << "====================================================================" << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
