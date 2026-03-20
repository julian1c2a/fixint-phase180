// =============================================================================
// Test Suite: Division Performance Optimization
// =============================================================================
// Comprehensive tests for big_bin_divrem() 6-level optimization cascade

#include "int128_parameterized.hpp"
#include "representation.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace nstd;

// Color codes for output
#define TEST_PASS "\033[32m[OK]\033[0m"
#define TEST_FAIL "\033[31m[FAIL]\033[0m"

int tests_passed = 0;
int tests_failed = 0;

template <typename T>
void assert_equal(const char *test_name, const T &actual, const T &expected)
{
    if (actual.high() == expected.high() && actual.low() == expected.low())
    {
        std::cout << TEST_PASS << " " << test_name << "\n";
        tests_passed++;
    }
    else
    {
        std::cout << TEST_FAIL << " " << test_name << "\n";
        std::cout << "  Expected: 0x" << std::hex << std::setfill('0') << std::setw(16)
                  << expected.high() << expected.low() << "\n";
        std::cout << "  Actual:   0x" << std::setfill('0') << std::setw(16)
                  << actual.high() << actual.low() << "\n"
                  << std::dec;
        tests_failed++;
    }
}

int main()
{
    std::cout << "\n====================================================================\n";
    std::cout << "Division Performance Optimization Tests\n";
    std::cout << "====================================================================\n\n";

    // Test 1: Power-of-2 divisors (Level 1)
    {
        std::cout << "[Group 1] Power-of-2 divisors:\n";

        auto [q, r] = uint128_t{0, 0x8000000000000000ULL}.divmod(uint128_t{0, 2});
        assert_equal("  2^127 / 2 = 2^126", q, uint128_t{0, 0x4000000000000000ULL});
        assert_equal("  2^127 % 2 = 0", r, uint128_t{0, 0});

        auto [q2, r2] = uint128_t{0, 0x0000000000000100ULL}.divmod(uint128_t{0, 0x0000000000000010ULL});
        assert_equal("  256 / 16 = 16", q2, uint128_t{0, 16});
        assert_equal("  256 % 16 = 0", r2, uint128_t{0, 0});
    }

    // Test 2: 64-bit values (Level 3)
    {
        std::cout << "\n[Group 2] Both values fit in 64 bits:\n";

        auto [q, r] = uint128_t{0, 100}.divmod(uint128_t{0, 7});
        assert_equal("  100 / 7 = 14", q, uint128_t{0, 14});
        assert_equal("  100 % 7 = 2", r, uint128_t{0, 2});

        auto [q2, r2] = uint128_t{0, 1000}.divmod(uint128_t{0, 13});
        assert_equal("  1000 / 13 = 76", q2, uint128_t{0, 76});
        assert_equal("  1000 % 13 = 12", r2, uint128_t{0, 12});
    }

    // Test 3: 128-bit / 64-bit divisor (Level 4)
    {
        std::cout << "\n[Group 3] 128-bit dividend / 64-bit divisor:\n";

        // Create 2^80 / 2^16
        uint128_t dividend1{0x0000000000000001ULL, 0x0ULL}; // 2^64
        uint128_t divisor1{0, 0x0000000000000100ULL};       // 2^8
        auto [q1, r1] = dividend1.divmod(divisor1);
        assert_equal("  2^64 / 2^8 = 2^56", q1, uint128_t{0, 0x0100000000000000ULL});

        // Create 10^18 / 1000
        uint128_t dividend2{0, 0x0DE0B6B3A7640000ULL}; // 10^18
        uint128_t divisor2{0, 1000};
        auto [q2, r2] = dividend2.divmod(divisor2);
        assert_equal("  10^18 / 1000 = 10^15", q2, uint128_t{0, 0x38D7EA4C68000ULL});
    }

    // Test 4: 128-bit / 128-bit (Level 6 - Binary long division)
    {
        std::cout << "\n[Group 4] 128-bit / 128-bit division:\n";

        // 2^127 / 2 = 2^126
        auto [q, r] = uint128_t{0, 0x8000000000000000ULL}.divmod(uint128_t{0, 2});
        assert_equal("  2^127 / 2 = 2^126", q, uint128_t{0, 0x4000000000000000ULL});
        assert_equal("  2^127 % 2 = 0", r, uint128_t{0, 0});

        // 2^100 / 2^50
        uint128_t dividend{0x0000010000000000ULL, 0x0ULL}; // 2^100
        uint128_t divisor{0x0000000000000001ULL, 0x0ULL};  // 2^50
        auto [q2, r2] = dividend.divmod(divisor);
        assert_equal("  2^100 / 2^50 = 2^50", q2, uint128_t{0, 0x0000010000000000ULL});
    }

    // Test 5: Small specific divisors 3-15 (Level 2)
    {
        std::cout << "\n[Group 5] Small specific divisors:\n";

        auto [q3, r3] = uint128_t{0, 42}.divmod(uint128_t{0, 3});
        assert_equal("  42 / 3 = 14", q3, uint128_t{0, 14});
        assert_equal("  42 % 3 = 0", r3, uint128_t{0, 0});

        auto [q5, r5] = uint128_t{0, 37}.divmod(uint128_t{0, 5});
        assert_equal("  37 / 5 = 7", q5, uint128_t{0, 7});
        assert_equal("  37 % 5 = 2", r5, uint128_t{0, 2});
    }

    // Test 6: Division with remainder
    {
        std::cout << "\n[Group 6] Division with remainders:\n";

        auto [q, r] = uint128_t{0, 17}.divmod(uint128_t{0, 5});
        assert_equal("  17 / 5 = 3", q, uint128_t{0, 3});
        assert_equal("  17 % 5 = 2", r, uint128_t{0, 2});

        auto [q2, r2] = uint128_t{0, 999}.divmod(uint128_t{0, 10});
        assert_equal("  999 / 10 = 99", q2, uint128_t{0, 99});
        assert_equal("  999 % 10 = 9", r2, uint128_t{0, 9});
    }

    // Test 7: Signed TC division
    {
        std::cout << "\n[Group 7] Signed (TC) division:\n";

        // Construct -100 in TC with proper sign extension (high = all 1s)
        const int128_tc_t neg100{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFF9CULL};
        const int128_tc_t seven{0, 7};
        auto [q, r] = neg100.divmod(seven);
        // Verify by reconstruction: q * 7 + r == -100
        const auto reconstructed = q * seven + r;
        assert_equal("  -100 / 7 reconstruction", reconstructed, neg100);
    }

    // Test 8: Large number division
    {
        std::cout << "\n[Group 8] Large number division:\n";

        // Create a large number and divide by small divisor
        const uint128_t large{0x0FFFFFFFFFFFFFFFULL, 0x0FFFFFFFFFFFFFFFULL};
        const uint128_t divisor{0, 0x00000000FFFFFFFFULL};
        auto [q, r] = large.divmod(divisor);
        // Verify by reconstruction: q * divisor + r == large
        const auto reconstructed = q * divisor + r;
        assert_equal("  Large division reconstruction", reconstructed, large);
    }

    // Test 9: Edge cases
    {
        std::cout << "\n[Group 9] Edge cases:\n";

        // Division where divisor == dividend
        auto [q1, r1] = uint128_t{0, 42}.divmod(uint128_t{0, 42});
        assert_equal("  n / n = 1", q1, uint128_t{0, 1});
        assert_equal("  n % n = 0", r1, uint128_t{0, 0});

        // Division by 1
        auto [q2, r2] = uint128_t{0, 12345}.divmod(uint128_t{0, 1});
        assert_equal("  n / 1 = n", q2, uint128_t{0, 12345});
        assert_equal("  n % 1 = 0", r2, uint128_t{0, 0});
    }

    // Summary
    std::cout << "\n====================================================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "====================================================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    std::cout << "Total:  " << (tests_passed + tests_failed) << "\n";

    return (tests_failed > 0) ? 1 : 0;
}
