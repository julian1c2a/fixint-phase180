// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_concepts.hpp - C++20 Concepts Validation
// =============================================================================

#include "int128_param_concepts.hpp"
#include <iostream>
#include <vector>

// Import type aliases
using nstd::int128_ek_t;
using nstd::int128_ms_t;
using nstd::int128_tc_t;
using nstd::uint128_t;

// Test counters
static int g_passed{0};
static int g_failed{0};

#define TEST_PASS() ++g_passed
#define TEST_FAIL() ++g_failed

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "C++20 Concepts Tests (compile-time validation)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] Type trait checks
    // ========================================================================
    {
        std::cout << "[Test 1] Type traits:\n";

        // Check is_128bit_type_v
        constexpr bool check1{nstd::is_128bit_type_v<uint128_t>};
        constexpr bool check2{nstd::is_128bit_type_v<int128_tc_t>};
        constexpr bool check3{nstd::is_128bit_type_v<int128_ms_t>};
        constexpr bool check4{nstd::is_128bit_type_v<int128_ek_t>};
        constexpr bool check5{!nstd::is_128bit_type_v<int>};

        if (check1 && check2 && check3 && check4 && check5)
        {
            std::cout << "  [OK] is_128bit_type_v\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] is_128bit_type_v\n";
            TEST_FAIL();
        }

        // Check is_uint128_v
        constexpr bool check6{nstd::is_uint128_v<uint128_t>};
        constexpr bool check7{!nstd::is_uint128_v<int128_tc_t>};

        if (check6 && check7)
        {
            std::cout << "  [OK] is_uint128_v\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] is_uint128_v\n";
            TEST_FAIL();
        }

        // Check is_signed_int128_v
        constexpr bool check8{nstd::is_signed_int128_v<int128_tc_t>};
        constexpr bool check9{nstd::is_signed_int128_v<int128_ms_t>};
        constexpr bool check10{!nstd::is_signed_int128_v<uint128_t>};

        if (check8 && check9 && check10)
        {
            std::cout << "  [OK] is_signed_int128_v\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] is_signed_int128_v\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 2] Basic concepts
    // ========================================================================
    {
        std::cout << "[Test 2] Basic concepts:\n";

        // Check int128_type concept
        constexpr bool check1{nstd::int128_type<uint128_t>};
        constexpr bool check2{nstd::int128_type<int128_tc_t>};
        constexpr bool check3{!nstd::int128_type<int>};

        if (check1 && check2 && check3)
        {
            std::cout << "  [OK] int128_type concept\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] int128_type concept\n";
            TEST_FAIL();
        }

        // Check uint128_type concept
        constexpr bool check4{nstd::uint128_type<uint128_t>};
        constexpr bool check5{!nstd::uint128_type<int128_tc_t>};

        if (check4 && check5)
        {
            std::cout << "  [OK] uint128_type concept\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] uint128_type concept\n";
            TEST_FAIL();
        }

        // Check signed_int128_type concept
        constexpr bool check6{nstd::signed_int128_type<int128_tc_t>};
        constexpr bool check7{nstd::signed_int128_type<int128_ms_t>};
        constexpr bool check8{!nstd::signed_int128_type<uint128_t>};

        if (check6 && check7 && check8)
        {
            std::cout << "  [OK] signed_int128_type concept\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] signed_int128_type concept\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 3] Conversion concepts
    // ========================================================================
    {
        std::cout << "[Test 3] Conversion concepts:\n";

        // Check int128_convertible
        constexpr bool check1{nstd::int128_convertible<int>};
        constexpr bool check2{nstd::int128_convertible<uint128_t>};
        constexpr bool check3{!nstd::int128_convertible<double>};

        if (check1 && check2 && check3)
        {
            std::cout << "  [OK] int128_convertible\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] int128_convertible\n";
            TEST_FAIL();
        }

        // Check int128_compatible
        constexpr bool check4{nstd::int128_compatible<int>};
        constexpr bool check5{nstd::int128_compatible<double>};
        constexpr bool check6{nstd::int128_compatible<uint128_t>};

        if (check4 && check5 && check6)
        {
            std::cout << "  [OK] int128_compatible\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] int128_compatible\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 4] Representation-specific concepts
    // ========================================================================
    {
        std::cout << "[Test 4] Representation concepts:\n";

        // Check int128_tc_type
        constexpr bool check1{nstd::int128_tc_type<int128_tc_t>};
        constexpr bool check2{!nstd::int128_tc_type<int128_ms_t>};

        if (check1 && check2)
        {
            std::cout << "  [OK] int128_tc_type\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] int128_tc_type\n";
            TEST_FAIL();
        }

        // Check int128_ms_type
        constexpr bool check3{nstd::int128_ms_type<int128_ms_t>};
        constexpr bool check4{!nstd::int128_ms_type<int128_tc_t>};

        if (check3 && check4)
        {
            std::cout << "  [OK] int128_ms_type\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] int128_ms_type\n";
            TEST_FAIL();
        }

        // Check int128_ek_type
        constexpr bool check5{nstd::int128_ek_type<int128_ek_t>};
        constexpr bool check6{!nstd::int128_ek_type<int128_tc_t>};

        if (check5 && check6)
        {
            std::cout << "  [OK] int128_ek_type\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] int128_ek_type\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 5] Function templates using concepts
    // ========================================================================
    {
        std::cout << "[Test 5] Template functions with concepts:\n";

        // Function using int128_type concept
        auto test_int128_type = []<nstd::int128_type T>(T value)
        {
            return value.is_zero();
        };

        const bool result1{test_int128_type(uint128_t{0})};
        const bool result2{test_int128_type(int128_tc_t{0})};

        if (result1 && result2)
        {
            std::cout << "  [OK] Function with int128_type concept\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] Function with int128_type concept\n";
            TEST_FAIL();
        }

        // Function using uint128_type concept
        auto test_uint128 = []<nstd::uint128_type T>(T value)
        {
            return value == T{42};
        };

        const bool result3{test_uint128(uint128_t{42})};

        if (result3)
        {
            std::cout << "  [OK] Function with uint128_type concept\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] Function with uint128_type concept\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // RESULTS
    // ========================================================================
    std::cout << "====================================================================\n";
    std::cout << "RESULTS:\n";
    std::cout << "  Passed: " << g_passed << "\n";
    std::cout << "  Failed: " << g_failed << "\n";
    std::cout << "  Total:  " << (g_passed + g_failed) << "\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? 0 : 1;
}
