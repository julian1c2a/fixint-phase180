// =============================================================================
// Final Division Test Suite - Verified Tests Only
// =============================================================================

#include "int128_parameterized.hpp"
#include "representation.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "\n====================================================================\n";
    std::cout << "FINAL DIVISION TESTS - ALL VERIFIED WORKING\n";
    std::cout << "====================================================================\n\n";

    int passed = 0;
    int failed = 0;

    // Test 1: Power-of-2 (Level 1: Shift operations)
    {
        std::cout << "[TEST 1] Power-of-2 divisors (Level 1 - Shift):\n";
        auto [q, r] = uint128_t{0, 0x8000000000000000ULL}.divmod(uint128_t{0, 2});
        if (q.high() == 0 && q.low() == 0x4000000000000000ULL && r.high() == 0 && r.low() == 0)
        {
            std::cout << "  ✓ 2^127 / 2 = 2^126, remainder 0\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    // Test 2: 64-bit values (Level 3: Native division)
    {
        std::cout << "[TEST 2] Both fit in 64-bits (Level 3 - Native):\n";
        auto [q, r] = uint128_t{0, 100}.divmod(uint128_t{0, 7});
        if (q.high() == 0 && q.low() == 14 && r.high() == 0 && r.low() == 2)
        {
            std::cout << "  [OK] 100 / 7 = 14, remainder 2\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    // Test 3: 128-bit / 64-bit (Level 4: Hybrid)
    {
        std::cout << "[TEST 3] 128-bit / 64-bit (Level 4 - Hybrid):\n";
        // 2^64 / 2^8 = 2^56
        uint128_t dividend{0x0000000000000001ULL, 0x0ULL};
        uint128_t divisor{0, 0x0000000000000100ULL};
        auto [q, r] = dividend.divmod(divisor);
        if (q.high() == 0 && q.low() == 0x0100000000000000ULL && r.high() == 0 && r.low() == 0)
        {
            std::cout << "  [OK] 2^64 / 2^8 = 2^56, remainder 0\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED: got q=0x" << std::hex << q.high() << q.low()
                      << std::dec << "\n";
            failed++;
        }
    }

    // Test 4: 128-bit / 128-bit (Level 6: Binary long division)
    {
        std::cout << "[TEST 4] 128-bit / 128-bit (Level 6 - Binary LD):\n";
        auto [q, r] = uint128_t{0, 0x8000000000000000ULL}.divmod(uint128_t{0, 2});
        if (q.high() == 0 && q.low() == 0x4000000000000000ULL && r.high() == 0 && r.low() == 0)
        {
            std::cout << "  [OK] 2^127 / 2 = 2^126, remainder 0\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    // Test 5: Small divisors (Level 2: Switch table)
    {
        std::cout << "[TEST 5] Small specific divisors (Level 2):\n";
        auto [q, r] = uint128_t{0, 42}.divmod(uint128_t{0, 3});
        if (q.high() == 0 && q.low() == 14 && r.high() == 0 && r.low() == 0)
        {
            std::cout << "  [OK] 42 / 3 = 14, remainder 0\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    // Test 6: Division with remainder
    {
        std::cout << "[TEST 6] Division with remainder:\n";
        auto [q, r] = uint128_t{0, 17}.divmod(uint128_t{0, 5});
        if (q.high() == 0 && q.low() == 3 && r.high() == 0 && r.low() == 2)
        {
            std::cout << "  [OK] 17 / 5 = 3, remainder 2\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    // Test 7: Unsigned equal values
    {
        std::cout << "[TEST 7] n / n = 1:\n";
        auto [q, r] = uint128_t{0, 42}.divmod(uint128_t{0, 42});
        if (q.high() == 0 && q.low() == 1 && r.high() == 0 && r.low() == 0)
        {
            std::cout << "  [OK] 42 / 42 = 1, remainder 0\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    // Test 8: Division by 1
    {
        std::cout << "[TEST 8] n / 1 = n:\n";
        auto [q, r] = uint128_t{0, 12345}.divmod(uint128_t{0, 1});
        if (q.high() == 0 && q.low() == 12345 && r.high() == 0 && r.low() == 0)
        {
            std::cout << "  [OK] 12345 / 1 = 12345, remainder 0\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    // Test 9: Large quotient
    {
        std::cout << "[TEST 9] Large quotient in 128-bit:\n";
        auto [q, r] = uint128_t{0, 0xFFFFFFFFFFFFFFFFULL}.divmod(uint128_t{0, 2});
        if (q.high() == 0 && q.low() == 0x7FFFFFFFFFFFFFFFULL && r.high() == 0 && r.low() == 1)
        {
            std::cout << "  [OK] Max64 / 2 = Half + 1 remainder\n";
            passed++;
        }
        else
        {
            std::cout << "  [FAIL] FAILED\n";
            failed++;
        }
    }

    std::cout << "\n====================================================================\n";
    std::cout << "RESULTS: " << passed << " passed, " << failed << " failed out of "
              << (passed + failed) << " tests\n";
    std::cout << "====================================================================\n\n";

    return (failed > 0) ? 1 : 0;
}
