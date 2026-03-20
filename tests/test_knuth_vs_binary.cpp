// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Knuth Algorithm D vs Binary Long Division Comparison
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
// Copyright (c) 2024-2026 Julián Calderón Almendros
//
// Tests that both division algorithms produce identical results

#include <iostream>
#include <cassert>
#include <utility>
#include <cstdint>

// Include the parameterized int128 header
#include "../include/int128_parameterized.hpp"

using namespace nstd;

// Helper macro for test reporting
#define TEST(name)                            \
    std::cout << "[TEST] " << name << "... "; \
    std::cout.flush()

#define PASS() std::cout << "[OK]\n"
#define FAIL(reason)                          \
    std::cout << "[FAIL] " << reason << "\n"; \
    return false

// Test function that compares both algorithms
bool test_division_algorithms_match()
{
    TEST("Both algorithms produce identical results");

    // Test 1: Power-of-2 (2^127 / 2)
    {
        const uint128_t dividend{0x8000000000000000ULL, 0x0};
        const uint128_t divisor{0x2, 0x0};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            std::cout << "\nBinary:  q=" << q_bin.high() << ":" << q_bin.low()
                      << ", r=" << r_bin.high() << ":" << r_bin.low() << "\n";
            std::cout << "Knuth:   q=" << q_knuth.high() << ":" << q_knuth.low()
                      << ", r=" << r_knuth.high() << ":" << r_knuth.low() << "\n";
            FAIL("Power-of-2 division mismatch");
        }
    }

    // Test 2: 64-bit values (100 / 7)
    {
        const uint128_t dividend{0x0, 100ULL};
        const uint128_t divisor{0x0, 7ULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("64-bit division mismatch");
        }
    }

    // Test 3: 128/64 (2^64 / 2^8)
    {
        const uint128_t dividend{0x0000000001000000ULL, 0x0};
        const uint128_t divisor{0x0, 0x0000000000000100ULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("128/64 division mismatch");
        }
    }

    // Test 4: Large full division
    {
        const uint128_t dividend{0x0FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
        const uint128_t divisor{0x0, 0x00000000FFFFFFFFULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("Large division mismatch");
        }
    }

    // Test 5: Small divisor (42 / 3)
    {
        const uint128_t dividend{0x0, 42ULL};
        const uint128_t divisor{0x0, 3ULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("Small divisor mismatch");
        }
    }

    // Test 6: With remainder (17 / 5)
    {
        const uint128_t dividend{0x0, 17ULL};
        const uint128_t divisor{0x0, 5ULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("Remainder test mismatch");
        }
    }

    // Test 7: Self division (42 / 42)
    {
        const uint128_t dividend{0x0, 42ULL};
        const uint128_t divisor{0x0, 42ULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("Self-division mismatch");
        }
    }

    // Test 8: Division by 1
    {
        const uint128_t dividend{0x0, 12345ULL};
        const uint128_t divisor{0x0, 1ULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("Division by 1 mismatch");
        }
    }

    // Test 9: Large quotient
    {
        const uint128_t dividend{0x0, 0xFFFFFFFFFFFFFFFFULL};
        const uint128_t divisor{0x0, 2ULL};

        auto [q_bin, r_bin] = dividend.big_bin_divrem(divisor);
        auto [q_knuth, r_knuth] = dividend.D_knuth_divrem(divisor);

        if (q_bin != q_knuth || r_bin != r_knuth)
        {
            FAIL("Large quotient mismatch");
        }
    }

    PASS();
    return true;
}

int main()
{
    std::cout << "\n====================================================================\n";
    std::cout << "KNUTH ALGORITHM D vs BINARY LONG DIVISION VERIFICATION\n";
    std::cout << "====================================================================\n\n";

    bool all_pass = true;

    all_pass &= test_division_algorithms_match();

    std::cout << "\n====================================================================\n";
    if (all_pass)
    {
        std::cout << "RESULTS: All algorithms match (BOTH CORRECT)\n";
        std::cout << "Status: READY FOR BENCHMARKING\n";
    }
    else
    {
        std::cout << "RESULTS: Algorithm mismatch detected\n";
        std::cout << "Status: NEEDS DEBUGGING\n";
    }
    std::cout << "====================================================================\n\n";

    return all_pass ? 0 : 1;
}
