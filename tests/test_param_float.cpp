// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Float/Double/Long Double — constructors and assignment operators
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
//
// Consolidates: test_float_constructors, test_float_assignment
// =============================================================================

#include "int128_parameterized.hpp"

#include <iostream>
#include <cmath>

using namespace nstd;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                              \
    do                                                \
    {                                                 \
        if (cond)                                     \
        {                                             \
            std::cout << "[OK] " << (name) << "\n";   \
            ++g_passed;                               \
        }                                             \
        else                                          \
        {                                             \
            std::cout << "[FAIL] " << (name) << "\n"; \
            ++g_failed;                               \
        }                                             \
    } while (false)

// =============================================================================
// Section 1: float — constructors (truncate toward zero)
// =============================================================================
static void test_float_constructors()
{
    std::cout << "\n--- Section 1: float constructors ---\n";

    // TC
    TEST("tc ctor  42.7f -> \"42\"", int128_tc_t{42.7f}.to_string() == "42");
    TEST("tc ctor -99.3f -> \"-99\"", int128_tc_t{-99.3f}.to_string() == "-99");

    // MS
    TEST("ms ctor  100.9f -> \"100\"", int128_ms_t{100.9f}.to_string() == "100");
    TEST("ms ctor  -50.1f -> \"-50\"", int128_ms_t{-50.1f}.to_string() == "-50");

    // EK
    TEST("ek ctor  10.5f -> \"10\"", int128_ek_t{10.5f}.to_string() == "10");
    TEST("ek ctor -20.8f -> \"-20\"", int128_ek_t{-20.8f}.to_string() == "-20");
}

// =============================================================================
// Section 2: float — assignment operators
// =============================================================================
static void test_float_assignment()
{
    std::cout << "\n--- Section 2: float assignment ---\n";

    int128_tc_t tc{0};
    tc = 42.7f;
    TEST("tc =  42.7f -> \"42\"", tc.to_string() == "42");
    tc = -99.3f;
    TEST("tc = -99.3f -> \"-99\"", tc.to_string() == "-99");

    int128_ms_t ms{0};
    ms = 100.9f;
    TEST("ms =  100.9f -> \"100\"", ms.to_string() == "100");
    ms = -50.1f;
    TEST("ms =  -50.1f -> \"-50\"", ms.to_string() == "-50");

    int128_ek_t ek{0};
    ek = 10.5f;
    TEST("ek =  10.5f -> \"10\"", ek.to_string() == "10");
    ek = -20.8f;
    TEST("ek = -20.8f -> \"-20\"", ek.to_string() == "-20");
}

// =============================================================================
// Section 3: double — constructors
// =============================================================================
static void test_double_constructors()
{
    std::cout << "\n--- Section 3: double constructors ---\n";

    TEST("tc ctor  123.456    -> \"123\"", int128_tc_t{123.456}.to_string() == "123");
    TEST("tc ctor  1234567890123.0 -> \"1234567890123\"",
         int128_tc_t{1234567890123.0}.to_string() == "1234567890123");
    TEST("ms ctor  999.999    -> \"999\"", int128_ms_t{999.999}.to_string() == "999");
    TEST("ms ctor -777.777    -> \"-777\"", int128_ms_t{-777.777}.to_string() == "-777");
    TEST("ek ctor   50.0      -> \"50\"", int128_ek_t{50.0}.to_string() == "50");
    TEST("ek ctor -100.0      -> \"-100\"", int128_ek_t{-100.0}.to_string() == "-100");
}

// =============================================================================
// Section 4: double — assignment operators
// =============================================================================
static void test_double_assignment()
{
    std::cout << "\n--- Section 4: double assignment ---\n";

    int128_tc_t tc{0};
    tc = 123.456;
    TEST("tc =  123.456         -> \"123\"", tc.to_string() == "123");
    tc = 1234567890123.0;
    TEST("tc =  1234567890123.0 -> \"1234567890123\"", tc.to_string() == "1234567890123");

    int128_ms_t ms{0};
    ms = 999.999;
    TEST("ms =  999.999  -> \"999\"", ms.to_string() == "999");
    ms = -777.777;
    TEST("ms = -777.777  -> \"-777\"", ms.to_string() == "-777");

    int128_ek_t ek{0};
    ek = 50.0;
    TEST("ek =  50.0  -> \"50\"", ek.to_string() == "50");
    ek = -100.0;
    TEST("ek = -100.0 -> \"-100\"", ek.to_string() == "-100");
}

// =============================================================================
// Section 5: long double — constructors and assignment
// =============================================================================
static void test_long_double()
{
    std::cout << "\n--- Section 5: long double ---\n";

    // Constructors
    TEST("tc ctor  987654321.123L -> \"987654321\"", int128_tc_t{987654321.123L}.to_string() == "987654321");
    TEST("ms ctor  555555.555L    -> \"555555\"", int128_ms_t{555555.555L}.to_string() == "555555");
    TEST("ek ctor  300.0L         -> \"300\"", int128_ek_t{300.0L}.to_string() == "300");
    TEST("ek ctor -150.0L         -> \"-150\"", int128_ek_t{-150.0L}.to_string() == "-150");

    // Assignment
    int128_tc_t tc{0};
    tc = 987654321.123L;
    TEST("tc =  987654321.123L -> \"987654321\"", tc.to_string() == "987654321");
    int128_ms_t ms{0};
    ms = 555555.555L;
    TEST("ms =  555555.555L    -> \"555555\"", ms.to_string() == "555555");
    int128_ek_t ek{0};
    ek = 300.0L;
    TEST("ek =  300.0L -> \"300\"", ek.to_string() == "300");
    ek = -150.0L;
    TEST("ek = -150.0L -> \"-150\"", ek.to_string() == "-150");
}

// =============================================================================
// Section 6: Special values — NaN and overflow
// =============================================================================
static void test_special_values()
{
    std::cout << "\n--- Section 6: Special values ---\n";

    const double nan_val = std::nan("");

    // NaN -> 0 (constructor)
    TEST("tc ctor  NaN -> \"0\"", int128_tc_t{nan_val}.to_string() == "0");
    TEST("ek ctor  NaN -> \"0\"", int128_ek_t{nan_val}.to_string() == "0");

    // NaN -> 0 (assignment)
    {
        int128_tc_t tc{42};
        tc = nan_val;
        TEST("tc = NaN -> \"0\"", tc.to_string() == "0");
    }
    {
        int128_ek_t ek{42};
        ek = nan_val;
        TEST("ek = NaN -> \"0\"", ek.to_string() == "0");
    }

    // Overflow saturates (constructor)
    const double huge_val = 1e40; // >> 2^128
    {
        int128_tc_t tc{huge_val};
        TEST("tc ctor overflow saturates (high==~0)", tc.high() == ~0ULL && tc.low() == ~0ULL);
    }

    // Overflow saturates (assignment)
    {
        int128_tc_t tc{0};
        tc = huge_val;
        TEST("tc = overflow saturates (high==~0)", tc.high() == ~0ULL && tc.low() == ~0ULL);
    }
}

// =============================================================================
// Section 7: EK-specific — bias correctness after float construction
// =============================================================================
static void test_ek_specific()
{
    std::cout << "\n--- Section 7: EK-specific ---\n";

    // EK stores real_value + bias; bias = (1ULL<<62, 0)
    const int128_ek_t ek_zero{0.0};
    TEST("ek ctor 0.0: high==bias, low==0", ek_zero.high() == (1ULL << 62) && ek_zero.low() == 0ULL);

    const int128_ek_t ek_one{1.0};
    TEST("ek ctor 1.0: high==bias, low==1", ek_one.high() == (1ULL << 62) && ek_one.low() == 1ULL);

    const int128_ek_t ek_minus_one{-1.0};
    TEST("ek ctor -1.0: to_string==-1", ek_minus_one.to_string() == "-1");

    // Arithmetic after float construction must work correctly
    const int128_ek_t a{10.5}; // truncates to 10
    const int128_ek_t b{20.3}; // truncates to 20
    TEST("ek ctor: 10.5 + 20.3 => 10+20 = 30", (a + b).to_string() == "30");
}

// =============================================================================
// Section 8: Fractional truncation, zero, and chained assignment
// =============================================================================
static void test_truncation_and_chain()
{
    std::cout << "\n--- Section 8: Truncation, zero, chained assignment ---\n";

    // Zero from float/double
    TEST("tc ctor  0.0f -> \"0\"", int128_tc_t{0.0f}.to_string() == "0");
    TEST("ms ctor  0.0  -> \"0\"", int128_ms_t{0.0}.to_string() == "0");

    // Fractional parts are truncated (toward zero)
    TEST("tc ctor  123.999 -> \"123\"", int128_tc_t{123.999}.to_string() == "123");
    TEST("tc ctor -456.001 -> \"-456\"", int128_tc_t{-456.001}.to_string() == "-456");

    // Sequential assignment overwrites correctly
    {
        int128_tc_t tc{0};
        tc = 100;
        tc = 50.5f;
        tc = 75.75;
        TEST("tc sequential assign ends at \"75\"", tc.to_string() == "75");
    }

    // Chained assignment: tc_a = tc_b = tc_c = 42.0
    {
        int128_tc_t a{0}, b{0}, c{0};
        a = b = c = 42.0;
        bool ok = (a.to_string() == "42") && (b.to_string() == "42") && (c.to_string() == "42");
        TEST("tc chain assign a=b=c=42.0 all==\"42\"", ok);
    }

    // Chained EK assignment
    {
        int128_ek_t a{0}, b{0};
        a = b = 33.3;
        bool ok = (a.to_string() == "33") && (b.to_string() == "33");
        TEST("ek chain assign a=b=33.3 both==\"33\"", ok);
    }

    // Assign after construction with a different value
    {
        int128_tc_t tc{100};
        tc = 200.5;
        TEST("tc assign after ctor: 100 -> 200.5 -> \"200\"", tc.to_string() == "200");
    }
    {
        int128_ms_t ms{-50};
        ms = 75.9f;
        TEST("ms assign after ctor: -50 -> 75.9f -> \"75\"", ms.to_string() == "75");
    }
    {
        int128_ek_t ek{10};
        ek = -20.0L;
        TEST("ek assign after ctor: 10 -> -20.0L -> \"-20\"", ek.to_string() == "-20");
    }
}

// =============================================================================
// Section 9: int128 → float direction (static_cast to double/long double)
// =============================================================================
static void test_to_float()
{
    std::cout << "\n--- Section 9: to_double / to_long_double ---\n";

    // to_double
    {
        const uint128_t x{0, 100};
        TEST("to_double 100", std::abs(static_cast<double>(x) - 100.0) < 0.001);
    }

    {
        const uint128_t x{0, ~0ULL};
        TEST("to_double 2^64-1 approx", static_cast<double>(x) > 1.8e19);
    }

    {
        const uint128_t x{1, 0};
        TEST("to_double 2^64 approx", std::abs(static_cast<double>(x) - 18446744073709551616.0) < 1.0);
    }

    {
        const int128_tc_t x{-100};
        TEST("to_double -100", std::abs(static_cast<double>(x) - (-100.0)) < 0.001);
    }

    // to_long_double
    {
        const uint128_t x{0, 1000};
        TEST("to_long_double 1000",
             std::abs(static_cast<double>(static_cast<long double>(x)) - 1000.0) < 0.001);
    }

    {
        const int128_tc_t x{-1000};
        TEST("to_long_double -1000",
             std::abs(static_cast<double>(static_cast<long double>(x)) - (-1000.0)) < 0.001);
    }

    // Round-trips: int128 → double → int128
    {
        const uint128_t orig{0, 12345};
        const uint128_t back{static_cast<double>(orig)};
        TEST("roundtrip to_double 12345", back == orig);
    }

    {
        const int128_tc_t orig{-12345};
        const double d{static_cast<double>(orig)};
        const int128_tc_t back{d};
        const double back_d{static_cast<double>(back)};
        TEST("roundtrip to_double -12345", std::abs(back_d - d) < 0.001);
    }

    // MS → double preserves sign
    {
        int128_ms_t x{0, 0};
        x.set_high(1ULL << 63);
        x.set_low(42);
        TEST("to_double ms_negative == -42", std::abs(static_cast<double>(x) - (-42.0)) < 0.001);
    }

    // from_double MS: sign bit set, magnitude correct
    {
        const int128_ms_t x{-100.0};
        TEST("from_double ms -100 sign bit", (x.high() & (1ULL << 63)) != 0);
        TEST("from_double ms -100 low==100", x.low() == 100);
    }
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "================================================================\n";
    std::cout << "  test_param_float: float/double/long double constructors & =\n";
    std::cout << "================================================================\n";

    test_float_constructors();
    test_float_assignment();
    test_double_constructors();
    test_double_assignment();
    test_long_double();
    test_special_values();
    test_ek_specific();
    test_truncation_and_chain();
    test_to_float();

    std::cout << "\n================================================================\n";
    std::cout << "  RESULTS: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "================================================================\n";

    return g_failed ? 1 : 0;
}
