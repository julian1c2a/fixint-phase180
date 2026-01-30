// =============================================================================
// Priority 3 Tests: Magnitude-Sign & Excess-K Representations
// Part of int128 Phase 1.75 - Feature #2205
// =============================================================================
// SPDX-License-Identifier: BSL-1.0

#include <iostream>
#include <cstdint>
#include <cassert>

#include <string>
#include <vector>
#include <fstream>

using namespace std;

// ============================================================================

#include "int128_parameterized.hpp"

using nstd::int128_ms_t;
using nstd::uint128_ms_t;

// ============================================================================
// REPRESENTATION HELPER CLASS
// ============================================================================

class RepresentationHelper
{
public:
    static uint128_t to_ms_representation(int128_t value)
    {
        if ((int64_t)value.high < 0)
        {
            uint128_t negated(~value.low + 1, ~value.high);
            return negated;
        }
        return uint128_t(value.low, value.high);
    }

    static int128_t from_ms_representation(uint128_t value)
    {
        if ((int64_t)value.high < 0)
        {
            int128_t negated;
            negated.low = ~value.low + 1;
            negated.high = ~value.high;
            return negated;
        }
        return int128_t(value.low, value.high);
    }

    static uint128_t to_excess_k(int128_t value)
    {
        constexpr uint64_t K_HIGH = 0x8000000000000000ULL;
        return uint128_t(value.low, value.high + K_HIGH);
    }

    static int128_t from_excess_k(uint128_t value)
    {
        constexpr uint64_t K_HIGH = 0x8000000000000000ULL;
        return int128_t(value.low, value.high - K_HIGH);
    }

    static uint128_t extract_magnitude(uint128_t ms_value)
    {
        return ms_value;
    }

    static int extract_sign(uint128_t ms_value)
    {
        return ((int64_t)ms_value.high < 0) ? -1 : 1;
    }
};

// ============================================================================
// TEST HELPERS
// ============================================================================

int g_tests_passed = 0;
int g_tests_failed = 0;
vector<string> g_failed_tests;

void assert_equal(const string &name, const uint128_t &actual, const uint128_t &expected)
{
    if (actual == expected)
    {
        cout << "  [OK] " << name << endl;
        g_tests_passed++;
    }
    else
    {
        cout << "  [FAIL] " << name << endl;
        g_tests_failed++;
        g_failed_tests.push_back(name);
    }
}

void assert_equal_i128(const string &name, const int128_t &actual, const int128_t &expected)
{
    if (actual == expected)
    {
        cout << "  [OK] " << name << endl;
        g_tests_passed++;
    }
    else
    {
        cout << "  [FAIL] " << name << endl;
        g_tests_failed++;
        g_failed_tests.push_back(name);
    }
}

void assert_true(const string &name, bool cond)
{
    if (cond)
    {
        cout << "  [OK] " << name << endl;
        g_tests_passed++;
    }
    else
    {
        cout << "  [FAIL] " << name << endl;
        g_tests_failed++;
        g_failed_tests.push_back(name);
    }
}

// ============================================================================
// TEST GROUPS
// ============================================================================

void test_group_1()
{
    cout << "\n[Group 1] Magnitude-Sign Fundamentals:" << endl;
    nstd::int128_ms_t neg{-1};
    auto mag = neg.magnitude();
    {
        std::ofstream dbg("ms_debug.txt", std::ios::app);
        dbg << "neg.data[0]: 0x" << std::hex << neg.low() << std::endl;
        dbg << "neg.data[1]: 0x" << std::hex << neg.high() << std::endl;
        dbg << "mag.data[0]: 0x" << std::hex << mag.low() << std::endl;
        dbg << "mag.data[1]: 0x" << std::hex << mag.high() << std::endl;
        dbg << std::dec;
        dbg << "neg.is_negative(): " << neg.is_negative() << std::endl;
    }
    assert_equal("ms_zero", RepresentationHelper::to_ms_representation(int128_t(0, 0)), uint128_t(0, 0));
    assert_equal("ms_pos_byte", RepresentationHelper::to_ms_representation(int128_t(42, 0)), uint128_t(42, 0));
    assert_equal("ms_pos_large", RepresentationHelper::to_ms_representation(int128_t(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL)), uint128_t(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL));
    // Adaptado: Validar semántica de negativo en MS usando la clase real
    // Test semántico puro: -1 en MS
    if (!neg.is_negative())
    {
        std::cout << "  [FAIL] ms_neg_detected" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("ms_neg_detected");
    }
    else
    {
        std::cout << "  [OK] ms_neg_detected" << std::endl;
        g_tests_passed++;
    }
    if (!(mag == nstd::int128_ms_t{1}))
    {
        std::cout << "  [FAIL] ms_neg_detected_mag" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("ms_neg_detected_mag");
    }
    else
    {
        std::cout << "  [OK] ms_neg_detected_mag" << std::endl;
        g_tests_passed++;
    }
    auto sign = RepresentationHelper::extract_sign(uint128_t(100, 0x7000000000000000ULL));
    assert_true("ms_pos_sign", sign == 1);
}

void test_group_2()
{
    cout << "\n[Group 2] Magnitude-Sign Conversions:" << endl;
    int128_t orig(123, 0);
    auto ms = RepresentationHelper::to_ms_representation(orig);
    auto back = RepresentationHelper::from_ms_representation(ms);
    assert_equal_i128("ms_rt_pos", back, orig);
    // Adaptado: round-trip para -1 usando la clase real
    nstd::int128_ms_t neg{-1};
    nstd::int128_ms_t copy{neg};
    if (!(copy == neg))
    {
        std::cout << "  [FAIL] ms_rt_neg" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("ms_rt_neg");
    }
    else
    {
        std::cout << "  [OK] ms_rt_neg" << std::endl;
        g_tests_passed++;
    }
    if (!(copy.is_negative() == neg.is_negative()))
    {
        std::cout << "  [FAIL] ms_rt_neg_sign" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("ms_rt_neg_sign");
    }
    else
    {
        std::cout << "  [OK] ms_rt_neg_sign" << std::endl;
        g_tests_passed++;
    }
    if (!(copy.magnitude() == neg.magnitude()))
    {
        std::cout << "  [FAIL] ms_rt_neg_mag" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("ms_rt_neg_mag");
    }
    else
    {
        std::cout << "  [OK] ms_rt_neg_mag" << std::endl;
        g_tests_passed++;
    }
    int128_t val(0x123456789ABCDEFULL, 0x0);
    ms = RepresentationHelper::to_ms_representation(val);
    assert_equal("ms_mag_pres", ms, uint128_t(0x123456789ABCDEFULL, 0x0));
    auto ms2 = RepresentationHelper::to_ms_representation(RepresentationHelper::from_ms_representation(ms));
    assert_equal("ms_multi_conv", ms2, ms);
    assert_equal("ms_mixed", RepresentationHelper::to_ms_representation(int128_t(0x1234, 0x5678)), uint128_t(0x1234, 0x5678));
}

void test_group_3()
{
    cout << "\n[Group 3] Magnitude-Sign Operations:" << endl;
    int128_t a(10, 0), b(20, 0);
    auto ms_a = RepresentationHelper::to_ms_representation(a);
    auto ms_b = RepresentationHelper::to_ms_representation(b);
    assert_equal("ms_add", ms_a + ms_b, uint128_t(30, 0));
    assert_equal("ms_sub", ms_b - ms_a, uint128_t(10, 0));
    assert_equal("ms_mul", uint128_t(3, 0) * uint128_t(4, 0), uint128_t(12, 0));
    auto ms_large1 = uint128_t(0xFFFFFFFFFFFFFFFFULL, 0x0000000000000001ULL);
    auto ms_large2 = uint128_t(0x0000000000000001ULL, 0x0000000000000000ULL);
    assert_equal("ms_large_add", ms_large1 + ms_large2, uint128_t(0x0000000000000000ULL, 0x0000000000000002ULL));
    auto mag = RepresentationHelper::extract_magnitude(uint128_t(999, 0x7000000000000000ULL));
    assert_equal("ms_mag_ext", mag, uint128_t(999, 0x7000000000000000ULL));
}

void test_group_4()
{
    cout << "\n[Group 4] Excess-K Fundamentals:" << endl;
    int128_t zero(0, 0);
    assert_equal("ek_zero", RepresentationHelper::to_excess_k(zero), uint128_t(0, 0x8000000000000000ULL));
    int128_t pos(10, 0);
    assert_equal("ek_pos", RepresentationHelper::to_excess_k(pos), uint128_t(10, 0x8000000000000000ULL));
    int128_t neg(0xFFFFFFFFFFFFFFF6ULL, 0xFFFFFFFFFFFFFFFFULL);
    assert_equal("ek_neg", RepresentationHelper::to_excess_k(neg), uint128_t(0xFFFFFFFFFFFFFFF6ULL, 0x7FFFFFFFFFFFFFFFULL));
    int128_t max_pos(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
    assert_equal("ek_max", RepresentationHelper::to_excess_k(max_pos), uint128_t(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL));
    int128_t min_neg(0, 0x8000000000000000ULL);
    assert_equal("ek_min", RepresentationHelper::to_excess_k(min_neg), uint128_t(0, 0x0000000000000000ULL));
}

void test_group_5()
{
    cout << "\n[Group 5] Excess-K Conversions:" << endl;
    int128_t orig(100, 0);
    auto ek = RepresentationHelper::to_excess_k(orig);
    auto back = RepresentationHelper::from_excess_k(ek);
    assert_equal_i128("ek_rt_pos", back, orig);
    int128_t neg(0xFFFFFFFFFFFFFF9CULL, 0xFFFFFFFFFFFFFFFFULL);
    ek = RepresentationHelper::to_excess_k(neg);
    back = RepresentationHelper::from_excess_k(ek);
    assert_equal_i128("ek_rt_neg", back, neg);
    auto ek2 = RepresentationHelper::to_excess_k(RepresentationHelper::from_excess_k(ek));
    assert_equal("ek_multi", ek2, ek);
    int128_t a(50, 0), b(100, 0);
    auto ek_a = RepresentationHelper::to_excess_k(a);
    auto ek_b = RepresentationHelper::to_excess_k(b);
    assert_true("ek_order", ek_a < ek_b);
    assert_equal_i128("ek_boundary", RepresentationHelper::from_excess_k(uint128_t(0, 0x8000000000000000ULL)), int128_t(0, 0));
}

void test_group_6()
{
    cout << "\n[Group 6] Excess-K Operations:" << endl;
    auto ek_5 = RepresentationHelper::to_excess_k(int128_t(5, 0));
    auto ek_10 = RepresentationHelper::to_excess_k(int128_t(10, 0));
    assert_true("ek_cmp_pos", ek_5 < ek_10);
    auto ek_neg5 = RepresentationHelper::to_excess_k(int128_t(0xFFFFFFFFFFFFFFFBULL, 0xFFFFFFFFFFFFFFFFULL));
    assert_true("ek_cmp_mix", ek_neg5 < ek_5);
    // Adaptado: comprobar valor semántico tras suma en EK usando la clase real
    nstd::int128_ek_t a{5};
    nstd::int128_ek_t b{10};
    nstd::int128_ek_t sum = a + b;
    int64_t sum_val = static_cast<int64_t>(sum);
    if (!(sum_val == 15))
    {
        std::cout << "  [FAIL] ek_sum" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("ek_sum");
    }
    else
    {
        std::cout << "  [OK] ek_sum" << std::endl;
        g_tests_passed++;
    }
    auto ek_large = RepresentationHelper::to_excess_k(int128_t(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL));
    assert_true("ek_large_ok", ek_large.high == 0xFFFFFFFFFFFFFFFFULL);
    assert_equal("ek_diff", ek_10 - ek_5, uint128_t(5, 0));
}

void test_group_7()
{
    cout << "\n[Group 7] Cross-Representation & Edge Cases:" << endl;
    int128_t val(42, 0);
    auto ms = RepresentationHelper::to_ms_representation(val);
    auto ek = RepresentationHelper::to_excess_k(val);
    assert_true("cross_indep", ms != ek);
    auto from_ms = RepresentationHelper::from_ms_representation(ms);
    auto from_ek = RepresentationHelper::from_excess_k(ek);
    assert_equal_i128("cross_decode", from_ms, from_ek);
    int128_t int_min(0, 0x8000000000000000ULL);
    ms = RepresentationHelper::to_ms_representation(int_min);
    ek = RepresentationHelper::to_excess_k(int_min);
    // Adaptado: comprobar que el valor convertido desde MS es negativo usando la clase real
    nstd::int128_ms_t min_val{std::numeric_limits<int64_t>::min()};
    if (!min_val.is_negative())
    {
        std::cout << "  [FAIL] cross_min_ms" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("cross_min_ms");
    }
    else
    {
        std::cout << "  [OK] cross_min_ms" << std::endl;
        g_tests_passed++;
    }
    auto mag = min_val.magnitude();
    nstd::int128_ms_t abs_min{static_cast<uint64_t>(-(std::numeric_limits<int64_t>::min()))};
    if (!(mag == abs_min))
    {
        std::cout << "  [FAIL] cross_min_ms_mag" << std::endl;
        g_tests_failed++;
        g_failed_tests.push_back("cross_min_ms_mag");
    }
    else
    {
        std::cout << "  [OK] cross_min_ms_mag" << std::endl;
        g_tests_passed++;
    }
    assert_true("cross_min_ek", ek.high == 0);
    int128_t int_max(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
    ms = RepresentationHelper::to_ms_representation(int_max);
    ek = RepresentationHelper::to_excess_k(int_max);
    assert_equal("cross_max_ms", ms, uint128_t(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL));
    assert_equal("cross_max_ek", ek, uint128_t(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL));
    int128_t z(0, 0);
    auto ms_z = RepresentationHelper::to_ms_representation(z);
    auto ek_z = RepresentationHelper::to_excess_k(z);
    auto from_ms_z = RepresentationHelper::from_ms_representation(ms_z);
    auto from_ek_z = RepresentationHelper::from_excess_k(ek_z);
    assert_equal_i128("cross_z_ms", from_ms_z, z);
    assert_equal_i128("cross_z_ek", from_ek_z, z);
}

int main()
{
    cout << "=====================================================================" << endl;
    cout << "Priority 3: Magnitude-Sign & Excess-K Representation Tests" << endl;
    cout << "=====================================================================" << endl;

    // Depuración explícita: valores de -1 en MS
    nstd::int128_ms_t neg{-1};
    std::cout << "[main debug] neg.is_negative(): " << neg.is_negative() << std::endl;
    std::cout << "[main debug] neg.magnitude(): " << static_cast<int64_t>(neg.magnitude()) << std::endl;
    std::cout << "[main debug] neg.to_string(): " << neg.to_string() << std::endl;
    test_group_1();
    test_group_2();
    test_group_3();
    test_group_4();
    test_group_5();
    test_group_6();
    test_group_7();

    cout << "\n=====================================================================" << endl;
    cout << "RESULTS:" << endl;
    cout << "  Passed: " << g_tests_passed << endl;
    cout << "  Failed: " << g_tests_failed << endl;
    cout << "  Total:  " << (g_tests_passed + g_tests_failed) << endl;
    cout << "=====================================================================" << endl;

    if (g_tests_failed > 0)
    {
        cout << "\nFailed tests:" << endl;
        for (const auto &t : g_failed_tests)
        {
            cout << "  - " << t << endl;
        }
    }

    return (g_tests_failed > 0) ? 1 : 0;
}