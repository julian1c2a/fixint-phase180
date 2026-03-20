// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Float/Double/Long Double Assignment Operators
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
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
    std::cout << "Float/Double/Long Double Assignment Operator Tests" << std::endl;
    std::cout << "====================================================================" << std::endl;

    // ========================================================================
    // Group 1: Float Assignment (TC, MS, EK)
    // ========================================================================
    std::cout << "\n[Group 1] Float Assignment:" << std::endl;

    // TC from float
    int128_tc_t tc_val{0};
    tc_val = 42.7f;
    TEST("tc_assign_float_truncate", tc_val.to_string() == "42");

    tc_val = -99.3f;
    TEST("tc_assign_neg_float", tc_val.to_string() == "-99");

    // MS from float
    int128_ms_t ms_val{0};
    ms_val = 100.9f;
    TEST("ms_assign_float", ms_val.to_string() == "100");

    ms_val = -50.1f;
    TEST("ms_assign_neg_float", ms_val.to_string() == "-50");

    // EK from float
    int128_ek_t ek_val{0};
    ek_val = 10.5f;
    TEST("ek_assign_float", ek_val.to_string() == "10");

    ek_val = -20.8f;
    TEST("ek_assign_neg_float", ek_val.to_string() == "-20");

    // ========================================================================
    // Group 2: Double Assignment (TC, MS, EK)
    // ========================================================================
    std::cout << "\n[Group 2] Double Assignment:" << std::endl;

    // TC from double
    tc_val = 123.456;
    TEST("tc_assign_double", tc_val.to_string() == "123");

    tc_val = 1234567890123.0;
    TEST("tc_assign_large_double", tc_val.to_string() == "1234567890123");

    // MS from double
    ms_val = 999.999;
    TEST("ms_assign_double", ms_val.to_string() == "999");

    ms_val = -777.777;
    TEST("ms_assign_neg_double", ms_val.to_string() == "-777");

    // EK from double
    ek_val = 50.0;
    TEST("ek_assign_double", ek_val.to_string() == "50");

    ek_val = -100.0;
    TEST("ek_assign_neg_double", ek_val.to_string() == "-100");

    // ========================================================================
    // Group 3: Long Double Assignment (TC, MS, EK)
    // ========================================================================
    std::cout << "\n[Group 3] Long Double Assignment:" << std::endl;

    // TC from long double
    tc_val = 987654321.123L;
    TEST("tc_assign_ldouble", tc_val.to_string() == "987654321");

    // MS from long double
    ms_val = 555555.555L;
    TEST("ms_assign_ldouble", ms_val.to_string() == "555555");

    // EK from long double
    ek_val = 300.0L;
    TEST("ek_assign_ldouble", ek_val.to_string() == "300");

    ek_val = -150.0L;
    TEST("ek_assign_neg_ldouble", ek_val.to_string() == "-150");

    // ========================================================================
    // Group 4: Special Values (NaN, Overflow)
    // ========================================================================
    std::cout << "\n[Group 4] Special Values:" << std::endl;

    // NaN handling
    double nan_val = std::nan("");
    tc_val = nan_val;
    TEST("tc_assign_nan_is_zero", tc_val.to_string() == "0");

    ek_val = nan_val;
    TEST("ek_assign_nan_is_zero", ek_val.to_string() == "0");

    // Overflow handling (values too large)
    double huge_val = 1e40; // Much larger than 2^128
    tc_val = huge_val;
    TEST("tc_assign_overflow_saturates", tc_val.high() == ~0ULL && tc_val.low() == ~0ULL);

    // ========================================================================
    // Group 5: Multiple Assignments (Chaining)
    // ========================================================================
    std::cout << "\n[Group 5] Multiple Assignments:" << std::endl;

    // Test that assignment returns reference
    int128_tc_t tc_a{0}, tc_b{0}, tc_c{0};
    tc_a = tc_b = tc_c = 42.0;
    TEST("tc_chain_assign", tc_a.to_string() == "42" && tc_b.to_string() == "42" && tc_c.to_string() == "42");

    // Assign different types in sequence
    tc_val = 100;
    tc_val = 50.5f;
    tc_val = 75.75;
    TEST("tc_sequential_assign", tc_val.to_string() == "75");

    // EK chain assignment
    int128_ek_t ek_a{0}, ek_b{0};
    ek_a = ek_b = 33.3;
    TEST("ek_chain_assign", ek_a.to_string() == "33" && ek_b.to_string() == "33");

    // ========================================================================
    // Group 6: Assignment After Construction
    // ========================================================================
    std::cout << "\n[Group 6] Assignment After Construction:" << std::endl;

    // Construct with one value, assign another
    int128_tc_t tc_init{100};
    tc_init = 200.5;
    TEST("tc_assign_after_construct", tc_init.to_string() == "200");

    int128_ms_t ms_init{-50};
    ms_init = 75.9f;
    TEST("ms_assign_after_construct", ms_init.to_string() == "75");

    int128_ek_t ek_init{10};
    ek_init = -20.0L;
    TEST("ek_assign_after_construct", ek_init.to_string() == "-20");

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
