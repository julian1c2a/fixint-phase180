// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Knuth Algorithm D correctness verification
// Part of int128 Library - Phase 3: Knuth Algorithm D
// License: BSL-1.0
// =============================================================================

#include <iostream>
#include <cassert>
#include <cstdint>

#include "../include/int128_parameterized.hpp"

using uint128_t = nstd::int128_param_t<nstd::signedness::unsigned_type,
                                       nstd::representation_form::binnat>;

int tests_passed = 0;
int tests_failed = 0;

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
// Group 1: 64/64 division (both fit in 64 bits)
// =============================================================================
void test_64_64()
{
    std::cout << "\n[Group 1] 64/64 division" << std::endl;

    {
        uint128_t a{0ull, 100ull};
        uint128_t b{0ull, 7ull};
        const auto [q, r] = a.divmod(b);
        TEST("100/7 = 14 rem 2", q.low() == 14 && r.low() == 2);
    }
    {
        uint128_t a{0ull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{0ull, 0xFFFFFFFFull};
        const auto [q, r] = a.divmod(b);
        TEST("(2^64-1)/(2^32-1) = 2^32+1 rem 0",
             q.low() == 0x100000001ull && r.low() == 0);
    }
    {
        uint128_t a{0ull, 1000000007ull};
        uint128_t b{0ull, 1000000007ull};
        const auto [q, r] = a.divmod(b);
        TEST("x/x = 1 rem 0", q.low() == 1 && r.low() == 0);
    }
}

// =============================================================================
// Group 2: 128/64 division (optimized Knuth with div128_64_composed)
// =============================================================================
void test_128_64()
{
    std::cout << "\n[Group 2] 128/64 division" << std::endl;

    // 2^64 / 3 = 6148914691236517205 rem 1
    {
        uint128_t a{1ull, 0ull}; // 2^64
        uint128_t b{0ull, 3ull};
        const auto [q, r] = a.divmod(b);
        TEST("2^64 / 3", q.low() == 6148914691236517205ull && q.high() == 0 && r.low() == 1);
    }

    // 2^127 / 7
    {
        uint128_t a{0x8000000000000000ull, 0ull}; // 2^127
        uint128_t b{0ull, 7ull};
        const auto [q, r] = a.divmod(b);
        // Verify: q * 7 + r == 2^127
        const auto check = q * b + r;
        TEST("2^127 / 7 (reconstruction)", check.low() == a.low() && check.high() == a.high());
    }

    // (2^128-1) / 2 = 2^127-1 rem 1
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{0ull, 2ull};
        const auto [q, r] = a.divmod(b);
        TEST("(2^128-1) / 2",
             q.high() == 0x7FFFFFFFFFFFFFFFull &&
                 q.low() == 0xFFFFFFFFFFFFFFFFull &&
                 r.low() == 1);
    }

    // (2^128-1) / 10
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{0ull, 10ull};
        const auto [q, r] = a.divmod(b);
        const auto check = q * b + r;
        TEST("(2^128-1) / 10 (reconstruction)",
             check.low() == a.low() && check.high() == a.high());
    }

    // Large dividend, large 64-bit divisor
    {
        uint128_t a{0xABCDEF0123456789ull, 0x1234567890ABCDEFull};
        uint128_t b{0ull, 0xFEDCBA9876543210ull};
        const auto [q, r] = a.divmod(b);
        const auto check = q * b + r;
        TEST("large 128/64 (reconstruction)",
             check.low() == a.low() && check.high() == a.high());
    }
}

// =============================================================================
// Group 3: 128/128 division (Knuth D general case)
// =============================================================================
void test_128_128()
{
    std::cout << "\n[Group 3] 128/128 division (Knuth D general case)" << std::endl;

    // (2^128-1) / (2^64+1)
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{1ull, 1ull}; // 2^64+1
        const auto [q, r] = a.divmod(b);
        const auto check = q * b + r;
        TEST("(2^128-1) / (2^64+1) (reconstruction)",
             check.low() == a.low() && check.high() == a.high());
    }

    // (2^128-1) / (2^127)
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{0x8000000000000000ull, 0ull};
        const auto [q, r] = a.divmod(b);
        TEST("(2^128-1) / 2^127 = 1 rem (2^127-1)",
             q.low() == 1 && q.high() == 0 &&
                 r.high() == 0x7FFFFFFFFFFFFFFFull && r.low() == 0xFFFFFFFFFFFFFFFFull);
    }

    // Two large 128-bit values
    {
        uint128_t a{0xABCDEF0123456789ull, 0x1234567890ABCDEFull};
        uint128_t b{0x1234567890ABCDEFull, 0xABCDEF0123456789ull};
        const auto [q, r] = a.divmod(b);
        const auto check = q * b + r;
        TEST("large 128/128 (reconstruction)",
             check.low() == a.low() && check.high() == a.high());
    }

    // Near-equal values (quotient = 1, small remainder)
    {
        uint128_t a{0x8000000000000000ull, 0x0000000000000005ull};
        uint128_t b{0x8000000000000000ull, 0x0000000000000001ull};
        const auto [q, r] = a.divmod(b);
        TEST("near-equal 128/128: q=1, r=4",
             q.low() == 1 && q.high() == 0 && r.low() == 4 && r.high() == 0);
    }

    // Divisor slightly smaller than dividend
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull}; // 2^128-1
        uint128_t b{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFEull}; // 2^128-2
        const auto [q, r] = a.divmod(b);
        TEST("(2^128-1)/(2^128-2) = 1 rem 1",
             q.low() == 1 && q.high() == 0 && r.low() == 1 && r.high() == 0);
    }

    // Divisor is half the dividend
    {
        uint128_t a{0x8000000000000000ull, 0ull};
        uint128_t b{0x4000000000000000ull, 0ull};
        const auto [q, r] = a.divmod(b);
        TEST("2^127 / 2^126 = 2 rem 0",
             q.low() == 2 && q.high() == 0 && r.low() == 0 && r.high() == 0);
    }
}

// =============================================================================
// Group 4: Power-of-2 fast path
// =============================================================================
void test_power_of_2()
{
    std::cout << "\n[Group 4] Power-of-2 fast path" << std::endl;

    {
        uint128_t a{0x8000000000000000ull, 0ull};
        uint128_t b{0ull, 4ull};
        const auto [q, r] = a.divmod(b);
        TEST("2^127 / 4 = 2^125",
             q.high() == 0x2000000000000000ull && q.low() == 0 && r.low() == 0);
    }
    {
        uint128_t a{0xABCDEF0123456789ull, 0x1234567890ABCDEFull};
        uint128_t b{0ull, 256ull};
        const auto [q, r] = a.divmod(b);
        const auto check = q * b + r;
        TEST("large / 256 (power-of-2, reconstruction)",
             check.low() == a.low() && check.high() == a.high());
    }
}

// =============================================================================
// Group 5: Edge cases
// =============================================================================
void test_edge_cases()
{
    std::cout << "\n[Group 5] Edge cases" << std::endl;

    // Division by zero
    {
        uint128_t a{0ull, 42ull};
        uint128_t b{0ull, 0ull};
        const auto [q, r] = a.divmod(b);
        TEST("42 / 0 = 0 rem 0 (defined behavior)", q.low() == 0 && r.low() == 0);
    }

    // 0 / anything
    {
        uint128_t a{0ull, 0ull};
        uint128_t b{0ull, 42ull};
        const auto [q, r] = a.divmod(b);
        TEST("0 / 42 = 0 rem 0", q.low() == 0 && r.low() == 0);
    }

    // divisor > dividend
    {
        uint128_t a{0ull, 5ull};
        uint128_t b{0ull, 100ull};
        const auto [q, r] = a.divmod(b);
        TEST("5 / 100 = 0 rem 5", q.low() == 0 && r.low() == 5);
    }

    // 1 / 1
    {
        uint128_t a{0ull, 1ull};
        uint128_t b{0ull, 1ull};
        const auto [q, r] = a.divmod(b);
        TEST("1 / 1 = 1 rem 0", q.low() == 1 && r.low() == 0);
    }
}

// =============================================================================
// Group 6: Consistency with big_bin_divrem
// =============================================================================
void test_consistency()
{
    std::cout << "\n[Group 6] Consistency D_knuth_divrem vs big_bin_divrem" << std::endl;

    struct TestCase
    {
        uint64_t ah, al, bh, bl;
        const char *name;
    };

    const TestCase cases[] = {
        {0, 100, 0, 7, "100/7"},
        {1, 0, 0, 3, "2^64/3"},
        {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 0, 10, "(2^128-1)/10"},
        {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 1, 1, "(2^128-1)/(2^64+1)"},
        {0x8000000000000000ull, 0, 0x4000000000000000ull, 0, "2^127/2^126"},
        {0xABCDEF0123456789ull, 0x1234567890ABCDEFull, 0x1234567890ABCDEFull, 0xABCDEF0123456789ull, "large/large"},
        {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFEull, "max/(max-1)"},
        {0x123ull, 0x456ull, 0, 0x789ull, "small 128/64"},
        {0x8000000000000000ull, 0x0000000000000005ull, 0x8000000000000000ull, 0x0000000000000001ull, "near-equal"},
        {0ull, 0xFFFFFFFFFFFFFFFFull, 0ull, 0xFFFFFFFFull, "(2^64-1)/(2^32-1)"},
    };

    int match_count = 0;
    for (const auto &tc : cases)
    {
        uint128_t a{tc.ah, tc.al};
        uint128_t b{tc.bh, tc.bl};

        const auto [qk, rk] = a.D_knuth_divrem(b);
        const auto [qb, rb] = a.big_bin_divrem(b);

        const bool match = (qk.low() == qb.low() && qk.high() == qb.high() &&
                            rk.low() == rb.low() && rk.high() == rb.high());
        if (match)
        {
            match_count++;
        }
        else
        {
            std::cout << "[FAIL] Mismatch on " << tc.name << std::endl;
            std::cout << "  Knuth:  q=(" << qk.high() << "," << qk.low()
                      << ") r=(" << rk.high() << "," << rk.low() << ")" << std::endl;
            std::cout << "  Binary: q=(" << qb.high() << "," << qb.low()
                      << ") r=(" << rb.high() << "," << rb.low() << ")" << std::endl;
        }
    }

    TEST("All 10 cases match big_bin_divrem", match_count == 10);
    if (match_count == 10)
    {
        tests_passed += 9; // Count 10 total but 1 already counted by TEST
    }
    else
    {
        tests_failed += (10 - match_count);
    }
}

// =============================================================================
// MAIN
// =============================================================================
int main()
{
    std::cout << "====================================================================\n"
              << "Knuth Algorithm D Correctness Test Suite\n"
              << "====================================================================" << std::endl;

    test_64_64();
    test_128_64();
    test_128_128();
    test_power_of_2();
    test_edge_cases();
    test_consistency();

    std::cout << "\n====================================================================\n"
              << "TEST SUMMARY\n"
              << "====================================================================\n"
              << "Passed: " << tests_passed << "\n"
              << "Failed: " << tests_failed << "\n"
              << "Total:  " << (tests_passed + tests_failed) << "\n"
              << "====================================================================" << std::endl;

    if (tests_failed == 0)
    {
        std::cout << "[OK] ALL TESTS PASSED" << std::endl;
    }
    else
    {
        std::cout << "[FAIL] SOME TESTS FAILED" << std::endl;
    }

    return tests_failed;
}
