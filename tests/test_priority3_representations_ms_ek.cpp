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

using namespace std;

// ============================================================================
// MOCK: int128_t and uint128_t for testing (simplified versions)
// ============================================================================

struct int128_t {
    uint64_t low, high;
    
    int128_t() : low(0), high(0) {}
    int128_t(uint64_t l, uint64_t h = 0) : low(l), high(h) {}
    int128_t(int64_t val) : low(static_cast<uint64_t>(val)), high(val < 0 ? ~0ULL : 0) {}
    
    bool operator==(const int128_t& other) const { return low == other.low && high == other.high; }
    bool operator!=(const int128_t& other) const { return !(*this == other); }
    bool operator<(const int128_t& other) const {
        if (high != other.high) return (int64_t)high < (int64_t)other.high;
        return low < other.low;
    }
    bool operator<=(const int128_t& other) const { return *this < other || *this == other; }
    bool operator>(const int128_t& other) const { return !(*this <= other); }
    bool operator>=(const int128_t& other) const { return !(*this < other); }
    
    int128_t operator+(const int128_t& other) const {
        uint64_t new_low = low + other.low;
        uint64_t carry = (new_low < low) ? 1 : 0;
        uint64_t new_high = high + other.high + carry;
        return int128_t(new_low, new_high);
    }
    
    int128_t operator-(const int128_t& other) const {
        uint64_t new_low = low - other.low;
        uint64_t borrow = (new_low > low) ? 1 : 0;
        uint64_t new_high = high - other.high - borrow;
        return int128_t(new_low, new_high);
    }
    
    int128_t operator*(const int128_t& other) const {
        return int128_t(low * other.low, high * other.low + low * other.high);
    }
};

struct uint128_t {
    uint64_t low, high;
    
    uint128_t() : low(0), high(0) {}
    uint128_t(uint64_t l, uint64_t h = 0) : low(l), high(h) {}
    
    bool operator==(const uint128_t& other) const { return low == other.low && high == other.high; }
    bool operator!=(const uint128_t& other) const { return !(*this == other); }
    bool operator<(const uint128_t& other) const {
        if (high != other.high) return high < other.high;
        return low < other.low;
    }
    bool operator<=(const uint128_t& other) const { return *this < other || *this == other; }
    bool operator>(const uint128_t& other) const { return !(*this <= other); }
    bool operator>=(const uint128_t& other) const { return !(*this < other); }
    
    uint128_t operator+(const uint128_t& other) const {
        uint64_t new_low = low + other.low;
        uint64_t carry = (new_low < low) ? 1 : 0;
        uint64_t new_high = high + other.high + carry;
        return uint128_t(new_low, new_high);
    }
    
    uint128_t operator-(const uint128_t& other) const {
        uint64_t new_low = low - other.low;
        uint64_t borrow = (new_low > low) ? 1 : 0;
        uint64_t new_high = high - other.high - borrow;
        return uint128_t(new_low, new_high);
    }
    
    uint128_t operator*(const uint128_t& other) const {
        return uint128_t(low * other.low, high * other.low + low * other.high);
    }
};

// ============================================================================
// REPRESENTATION HELPER CLASS
// ============================================================================

class RepresentationHelper {
public:
    static uint128_t to_ms_representation(int128_t value) {
        if ((int64_t)value.high < 0) {
            uint128_t negated(~value.low + 1, ~value.high);
            return negated;
        }
        return uint128_t(value.low, value.high);
    }
    
    static int128_t from_ms_representation(uint128_t value) {
        if ((int64_t)value.high < 0) {
            int128_t negated;
            negated.low = ~value.low + 1;
            negated.high = ~value.high;
            return negated;
        }
        return int128_t(value.low, value.high);
    }
    
    static uint128_t to_excess_k(int128_t value) {
        constexpr uint64_t K_HIGH = 0x8000000000000000ULL;
        return uint128_t(value.low, value.high + K_HIGH);
    }
    
    static int128_t from_excess_k(uint128_t value) {
        constexpr uint64_t K_HIGH = 0x8000000000000000ULL;
        return int128_t(value.low, value.high - K_HIGH);
    }
    
    static uint128_t extract_magnitude(uint128_t ms_value) {
        return ms_value;
    }
    
    static int extract_sign(uint128_t ms_value) {
        return ((int64_t)ms_value.high < 0) ? -1 : 1;
    }
};

// ============================================================================
// TEST HELPERS
// ============================================================================

int g_tests_passed = 0;
int g_tests_failed = 0;
vector<string> g_failed_tests;

void assert_equal(const string& name, const uint128_t& actual, const uint128_t& expected) {
    if (actual == expected) {
        cout << "  [OK] " << name << endl;
        g_tests_passed++;
    } else {
        cout << "  [FAIL] " << name << endl;
        g_tests_failed++;
        g_failed_tests.push_back(name);
    }
}

void assert_equal_i128(const string& name, const int128_t& actual, const int128_t& expected) {
    if (actual == expected) {
        cout << "  [OK] " << name << endl;
        g_tests_passed++;
    } else {
        cout << "  [FAIL] " << name << endl;
        g_tests_failed++;
        g_failed_tests.push_back(name);
    }
}

void assert_true(const string& name, bool cond) {
    if (cond) {
        cout << "  [OK] " << name << endl;
        g_tests_passed++;
    } else {
        cout << "  [FAIL] " << name << endl;
        g_tests_failed++;
        g_failed_tests.push_back(name);
    }
}

// ============================================================================
// TEST GROUPS
// ============================================================================

void test_group_1() {
    cout << "\n[Group 1] Magnitude-Sign Fundamentals:" << endl;
    assert_equal("ms_zero", RepresentationHelper::to_ms_representation(int128_t(0, 0)), uint128_t(0, 0));
    assert_equal("ms_pos_byte", RepresentationHelper::to_ms_representation(int128_t(42, 0)), uint128_t(42, 0));
    assert_equal("ms_pos_large", RepresentationHelper::to_ms_representation(int128_t(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL)), uint128_t(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL));
    int128_t neg = int128_t(0, 0) - int128_t(1, 0);
    uint128_t ms_neg = RepresentationHelper::to_ms_representation(neg);
    assert_true("ms_neg_detected", (int64_t)ms_neg.high < 0);
    auto sign = RepresentationHelper::extract_sign(uint128_t(100, 0x7000000000000000ULL));
    assert_true("ms_pos_sign", sign == 1);
}

void test_group_2() {
    cout << "\n[Group 2] Magnitude-Sign Conversions:" << endl;
    int128_t orig(123, 0);
    auto ms = RepresentationHelper::to_ms_representation(orig);
    auto back = RepresentationHelper::from_ms_representation(ms);
    assert_equal_i128("ms_rt_pos", back, orig);
    int128_t neg(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    ms = RepresentationHelper::to_ms_representation(neg);
    back = RepresentationHelper::from_ms_representation(ms);
    assert_equal_i128("ms_rt_neg", back, neg);
    int128_t val(0x123456789ABCDEFULL, 0x0);
    ms = RepresentationHelper::to_ms_representation(val);
    assert_equal("ms_mag_pres", ms, uint128_t(0x123456789ABCDEFULL, 0x0));
    auto ms2 = RepresentationHelper::to_ms_representation(RepresentationHelper::from_ms_representation(ms));
    assert_equal("ms_multi_conv", ms2, ms);
    assert_equal("ms_mixed", RepresentationHelper::to_ms_representation(int128_t(0x1234, 0x5678)), uint128_t(0x1234, 0x5678));
}

void test_group_3() {
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

void test_group_4() {
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

void test_group_5() {
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

void test_group_6() {
    cout << "\n[Group 6] Excess-K Operations:" << endl;
    auto ek_5 = RepresentationHelper::to_excess_k(int128_t(5, 0));
    auto ek_10 = RepresentationHelper::to_excess_k(int128_t(10, 0));
    assert_true("ek_cmp_pos", ek_5 < ek_10);
    auto ek_neg5 = RepresentationHelper::to_excess_k(int128_t(0xFFFFFFFFFFFFFFFBULL, 0xFFFFFFFFFFFFFFFFULL));
    assert_true("ek_cmp_mix", ek_neg5 < ek_5);
    assert_equal("ek_sum", ek_5 + ek_10, uint128_t(15, 0x8000000000000001ULL));
    auto ek_large = RepresentationHelper::to_excess_k(int128_t(0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL));
    assert_true("ek_large_ok", ek_large.high == 0xFFFFFFFFFFFFFFFFULL);
    assert_equal("ek_diff", ek_10 - ek_5, uint128_t(5, 0));
}

void test_group_7() {
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
    assert_true("cross_min_ms", (int64_t)ms.high < 0);
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

int main() {
    cout << "=====================================================================" << endl;
    cout << "Priority 3: Magnitude-Sign & Excess-K Representation Tests" << endl;
    cout << "=====================================================================" << endl;
    
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
    
    if (g_tests_failed > 0) {
        cout << "\nFailed tests:" << endl;
        for (const auto& t : g_failed_tests) {
            cout << "  - " << t << endl;
        }
    }
    
    return (g_tests_failed > 0) ? 1 : 0;
}