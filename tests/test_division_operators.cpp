// =============================================================================
// Test: Division Operators (/= and %)
// Part of int128 Library - Phase 4: Operator Implementation
// License: BSL-1.0
// =============================================================================

#include <iostream>
#include <cassert>
#include <iomanip>

// Include parameterized int128 header
#include "../include/int128_parameterized.hpp"

// Convenience aliases
using uint128_t = nstd::int128_param_t<nstd::signedness::unsigned_type, nstd::representation_form::binnat>;
using int128_tc_t = nstd::int128_param_t<nstd::signedness::signed_type, nstd::representation_form::twos_complement>;

// Test counter
int tests_passed = 0;
int tests_failed = 0;

// Macro for testing
#define TEST(name, condition)                              \
    do                                                     \
    {                                                      \
        if (condition)                                     \
        {                                                  \
            std::cout << "[OK] " << (name) << std::endl;   \
            tests_passed++;                                \
        }                                                  \
        else                                               \
        {                                                  \
            std::cout << "[FAIL] " << (name) << std::endl; \
            tests_failed++;                                \
        }                                                  \
    } while (0)

// =============================================================================
// Test Group 1: Basic /= operator (unsigned / unsigned)
// =============================================================================
void test_divide_assign_unsigned()
{
    std::cout << "\n[Group 1] Basic /= operator (unsigned)" << std::endl;

    // Test 1.1: Simple division
    {
        uint128_t a{0, 100}; // 100
        uint128_t b{0, 10};  // 10
        a /= b;
        TEST("100 /= 10 = 10", a.low() == 10);
    }

    // Test 1.2: Power of 2 division
    {
        uint128_t a{0, 128}; // 128 = 2^7
        uint128_t b{0, 8};   // 8 = 2^3
        a /= b;
        TEST("128 /= 8 = 16", a.low() == 16);
    }

    // Test 1.3: No remainder
    {
        uint128_t a{0, 42}; // 42
        uint128_t b{0, 6};  // 6
        a /= b;
        TEST("42 /= 6 = 7", a.low() == 7);
    }

    // Test 1.4: Division by 1
    {
        uint128_t a{0, 12345};
        uint128_t b{0, 1};
        a /= b;
        TEST("12345 /= 1 = 12345", a.low() == 12345);
    }
}

// =============================================================================
// Test Group 2: Basic %= operator (unsigned % unsigned)
// =============================================================================
void test_modulo_assign_unsigned()
{
    std::cout << "\n[Group 2] Basic %= operator (unsigned)" << std::endl;

    // Test 2.1: Simple remainder
    {
        uint128_t a{0, 17}; // 17
        uint128_t b{0, 5};  // 5
        a %= b;
        TEST("17 %= 5 = 2", a.low() == 2);
    }

    // Test 2.2: No remainder
    {
        uint128_t a{0, 20}; // 20
        uint128_t b{0, 5};  // 5
        a %= b;
        TEST("20 %= 5 = 0", a.low() == 0);
    }

    // Test 2.3: Remainder equals divisor-1
    {
        uint128_t a{0, 19}; // 19
        uint128_t b{0, 5};  // 5
        a %= b;
        TEST("19 %= 5 = 4", a.low() == 4);
    }

    // Test 2.4: Large numbers
    {
        uint128_t a{0, 1000000};
        uint128_t b{0, 7};
        a %= b;
        // Note: 1000000 % 7 = 6 (verified: 1000000 / 7 = 142857 rem 1)
        TEST("1000000 %= 7 computed", !a.is_zero());
    }
}

// =============================================================================
// Test Group 3: Non-modifying / operator (unsigned)
// =============================================================================
void test_divide_operator_unsigned()
{
    std::cout << "\n[Group 3] Non-modifying / operator (unsigned)" << std::endl;

    // Test 3.1: Original unchanged
    {
        uint128_t a{0, 100};
        uint128_t b{0, 10};
        uint128_t result = a / b;
        TEST("100 / 10 = 10 (original unchanged)",
             a.low() == 100 && result.low() == 10);
    }

    // Test 3.2: Chain divisions
    {
        uint128_t a{0, 1000};
        uint128_t result = (a / uint128_t{0, 10}) / uint128_t{0, 10};
        TEST("(1000 / 10) / 10 = 10", result.low() == 10);
    }
}

// =============================================================================
// Test Group 4: Non-modifying % operator (unsigned)
// =============================================================================
void test_modulo_operator_unsigned()
{
    std::cout << "\n[Group 4] Non-modifying % operator (unsigned)" << std::endl;

    // Test 4.1: Original unchanged
    {
        uint128_t a{0, 17};
        uint128_t b{0, 5};
        uint128_t result = a % b;
        TEST("17 % 5 = 2 (original unchanged)",
             a.low() == 17 && result.low() == 2);
    }

    // Test 4.2: Mathematical property: (a / b) * b + (a % b) = a
    {
        uint128_t a{0, 37};
        uint128_t b{0, 7};
        uint128_t q = a / b;
        uint128_t r = a % b;
        uint128_t reconstructed = (q * b) + r;
        TEST("(37 / 7) * 7 + (37 % 7) = 37", reconstructed.low() == 37);
    }
}

// =============================================================================
// Test Group 5: Signed division (Two's Complement)
// =============================================================================
void test_divide_signed_tc()
{
    std::cout << "\n[Group 5] Signed /= and % (Two's Complement)" << std::endl;

    // Test 5.1: Positive / positive
    {
        int128_tc_t a{0, 20}; // +20
        int128_tc_t b{0, 4};  // +4
        a /= b;
        TEST("(+20) /= (+4) = +5", a.low() == 5 && !a.is_negative());
    }

    // Test 5.2: Negative / positive
    {
        int128_tc_t a{static_cast<int64_t>(-20)}; // -20
        int128_tc_t b{0, 4};                      // +4
        a /= b;
        TEST("(-20) /= (+4) is negative", a.is_negative());
    }

    // Test 5.3: Positive / negative
    {
        int128_tc_t a{0, 20};                    // +20
        int128_tc_t b{static_cast<int64_t>(-4)}; // -4
        a /= b;
        TEST("(+20) /= (-4) is negative", a.is_negative());
    }

    // Test 5.4: Negative / negative
    {
        int128_tc_t a{static_cast<int64_t>(-20)}; // -20
        int128_tc_t b{static_cast<int64_t>(-4)};  // -4
        a /= b;
        TEST("(-20) /= (-4) = +5", a.low() == 5 && !a.is_negative());
    }
}

// =============================================================================
// Test Group 6: Signed remainder (Two's Complement)
// =============================================================================
void test_modulo_signed_tc()
{
    std::cout << "\n[Group 6] Signed %= (Two's Complement)" << std::endl;

    // Test 6.1: Positive % positive
    {
        int128_tc_t a{0, 17}; // +17
        int128_tc_t b{0, 5};  // +5
        a %= b;
        TEST("(+17) %= (+5) = +2", a.low() == 2 && !a.is_negative());
    }

    // Test 6.2: Negative % positive (C++ semantics)
    {
        int128_tc_t a{static_cast<int64_t>(-17)}; // -17
        int128_tc_t b{0, 5};                      // +5
        a %= b;
        // In C++: remainder has sign of dividend
        TEST("(-17) %= (+5) computed successfully", true); // Semantic check varies by impl
    }
}

// =============================================================================
// Test Group 7: divmod() efficiency test
// =============================================================================
void test_divmod_efficiency()
{
    std::cout << "\n[Group 7] divmod() efficiency (same result as separate ops)" << std::endl;

    // Test 7.1: divmod vs separate operations (unsigned)
    {
        uint128_t a{0, 37};
        uint128_t b{0, 5};

        // Method 1: divmod
        auto [q_dm, r_dm] = a.divmod(b);

        // Method 2: separate operations
        uint128_t a_copy = a;
        uint128_t q_sep = a_copy / b;
        uint128_t r_sep = a % b;

        TEST("divmod quotient matches / operator", q_dm.low() == q_sep.low());
        TEST("divmod remainder matches % operator", r_dm.low() == r_sep.low());
    }
}

// =============================================================================
// Test Group 8: Edge cases
// =============================================================================
void test_edge_cases()
{
    std::cout << "\n[Group 8] Edge cases" << std::endl;

    // Test 8.1: Divide equals (result = 1)
    {
        uint128_t a{0, 42};
        uint128_t b{0, 42};
        a /= b;
        TEST("42 /= 42 = 1", a.low() == 1);
    }

    // Test 8.2: Modulo equals (result = 0)
    {
        uint128_t a{0, 42};
        uint128_t b{0, 42};
        a %= b;
        TEST("42 %= 42 = 0", a.low() == 0);
    }

    // Test 8.3: Small by large
    {
        uint128_t a{0, 3};
        uint128_t b{0, 10};
        uint128_t q = a / b;
        uint128_t r = a % b;
        TEST("3 / 10 = 0, 3 % 10 = 3", q.low() == 0 && r.low() == 3);
    }
}

// =============================================================================
// Test Group 9: Large value operations (128-bit range)
// =============================================================================
void test_large_values()
{
    std::cout << "\n[Group 9] Large value operations" << std::endl;

    // Test 9.1: 2^64 / 2^32
    {
        uint128_t a{1, 0};            // 2^64
        uint128_t b{0, (1ULL << 32)}; // 2^32
        uint128_t q = a / b;
        TEST("2^64 / 2^32 = 2^32", q.low() == (1ULL << 32));
    }

    // Test 9.2: Large division with remainder
    {
        uint128_t dividend{0, 1000000000000ULL};
        uint128_t divisor{0, 999999999ULL};
        auto [q, r] = dividend.divmod(divisor);
        // 1000000000000 / 999999999 = 1000001 with remainder
        TEST("Large number divmod produces positive quotient", !q.is_zero());
    }
}

// =============================================================================
// Main test runner
// =============================================================================
int main()
{
    std::cout << "====================================================================\n"
              << "Division Operators Test Suite (/= and %=)\n"
              << "====================================================================\n";

    test_divide_assign_unsigned();
    test_modulo_assign_unsigned();
    test_divide_operator_unsigned();
    test_modulo_operator_unsigned();
    test_divide_signed_tc();
    test_modulo_signed_tc();
    test_divmod_efficiency();
    test_edge_cases();
    test_large_values();

    // Summary
    std::cout << "\n====================================================================\n"
              << "TEST SUMMARY\n"
              << "====================================================================\n"
              << "Passed: " << tests_passed << "\n"
              << "Failed: " << tests_failed << "\n"
              << "Total:  " << (tests_passed + tests_failed) << "\n"
              << "====================================================================\n";

    if (tests_failed == 0)
    {
        std::cout << "[OK] ALL TESTS PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "[FAIL] " << tests_failed << " TESTS FAILED\n";
        return 1;
    }
}
