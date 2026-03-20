// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_algorithm.hpp - Algorithm Functions
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_algorithm.hpp"
#include <iostream>
#include <vector>
#include <cassert>

using namespace nstd;

// Test result macros
#define TEST_PASS() (++g_passed)
#define TEST_FAIL() (++g_failed)

int g_passed{0};
int g_failed{0};

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Algorithm Functions Tests (fill, reverse, find, etc.)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] fill() - Fill range with value
    // ========================================================================
    {
        std::cout << "[Test 1] fill():\n";

        std::vector<uint128_t> vec(5);
        const uint128_t value{0, 42};

        nstd::fill<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), value);

        bool all_equal{true};
        for (const auto &elem : vec)
        {
            if (elem != value)
            {
                all_equal = false;
                break;
            }
        }

        if (all_equal && vec.size() == 5)
        {
            std::cout << "  [OK] fill\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] fill\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 2] fill_n() - Fill n elements
    // ========================================================================
    {
        std::cout << "[Test 2] fill_n():\n";

        std::vector<uint128_t> vec(10);
        const uint128_t value{0, 99};

        nstd::fill_n<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), 5, value);

        bool first_five_filled{true};
        bool rest_zero{true};

        for (size_t i = 0; i < 5; ++i)
        {
            if (vec[i] != value)
            {
                first_five_filled = false;
            }
        }

        for (size_t i = 5; i < 10; ++i)
        {
            if (vec[i] != uint128_t{0, 0})
            {
                rest_zero = false;
            }
        }

        if (first_five_filled && rest_zero)
        {
            std::cout << "  [OK] fill_n\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] fill_n\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 3] reverse() - Reverse range
    // ========================================================================
    {
        std::cout << "[Test 3] reverse():\n";

        std::vector<uint128_t> vec{
            uint128_t{0, 1},
            uint128_t{0, 2},
            uint128_t{0, 3},
            uint128_t{0, 4},
            uint128_t{0, 5}};

        nstd::reverse<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end());

        if ((vec[0] == uint128_t{0, 5}) &&
            (vec[1] == uint128_t{0, 4}) &&
            (vec[2] == uint128_t{0, 3}) &&
            (vec[3] == uint128_t{0, 2}) &&
            (vec[4] == uint128_t{0, 1}))
        {
            std::cout << "  [OK] reverse\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] reverse\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 4] find() - Find element in range
    // ========================================================================
    {
        std::cout << "[Test 4] find():\n";

        std::vector<uint128_t> vec{
            uint128_t{0, 10},
            uint128_t{0, 20},
            uint128_t{0, 30},
            uint128_t{0, 40}};

        const auto it_found{nstd::find<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{0, 30})};

        const auto it_not_found{nstd::find<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{0, 99})};

        if ((it_found != vec.end()) && (*it_found == uint128_t{0, 30}) &&
            (it_not_found == vec.end()))
        {
            std::cout << "  [OK] find\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] find\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 5] count() - Count occurrences
    // ========================================================================
    {
        std::cout << "[Test 5] count():\n";

        std::vector<uint128_t> vec{
            uint128_t{0, 5},
            uint128_t{0, 10},
            uint128_t{0, 5},
            uint128_t{0, 20},
            uint128_t{0, 5}};

        const auto count_5{nstd::count<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{0, 5})};

        const auto count_99{nstd::count<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{0, 99})};

        if ((count_5 == 3) && (count_99 == 0))
        {
            std::cout << "  [OK] count\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] count\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 6] all_of() / any_of() / none_of() - Predicate tests
    // ========================================================================
    {
        std::cout << "[Test 6] all_of / any_of / none_of:\n";

        std::vector<uint128_t> vec{
            uint128_t{0, 2},
            uint128_t{0, 4},
            uint128_t{0, 6},
            uint128_t{0, 8}};

        auto is_even = [](const uint128_t &x)
        { return (x.low() & 1) == 0; };
        auto is_large = [](const uint128_t &x)
        { return x.low() > 100; };

        const bool all_even{nstd::all_of<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), is_even)};

        const bool any_large{nstd::any_of<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), is_large)};

        const bool none_large{nstd::none_of<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), is_large)};

        if (all_even && !any_large && none_large)
        {
            std::cout << "  [OK] all_of_any_of_none_of\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] all_of_any_of_none_of\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 7] min_element() / max_element() - Find extremes
    // ========================================================================
    {
        std::cout << "[Test 7] min_element / max_element:\n";

        std::vector<uint128_t> vec{
            uint128_t{0, 50},
            uint128_t{0, 10},
            uint128_t{0, 100},
            uint128_t{0, 30}};

        const auto min_it{nstd::min_element<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end())};

        const auto max_it{nstd::max_element<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end())};

        if ((min_it != vec.end()) && (*min_it == uint128_t{0, 10}) &&
            (max_it != vec.end()) && (*max_it == uint128_t{0, 100}))
        {
            std::cout << "  [OK] min_element_max_element\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] min_element_max_element\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 8] accumulate() - Sum elements
    // ========================================================================
    {
        std::cout << "[Test 8] accumulate():\n";

        std::vector<uint128_t> vec{
            uint128_t{0, 1},
            uint128_t{0, 2},
            uint128_t{0, 3},
            uint128_t{0, 4},
            uint128_t{0, 5}};

        const auto sum{nstd::accumulate<signedness::unsigned_type, representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{0, 0})};

        const uint128_t expected{0, 15}; // 1+2+3+4+5 = 15

        if (sum == expected)
        {
            std::cout << "  [OK] accumulate\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] accumulate\n";
            TEST_FAIL();
        }
    }

    std::cout << "\n";

    // ========================================================================
    // [Test 9] Signed types (TC) - Test with int128_tc_t
    // ========================================================================
    {
        std::cout << "[Test 9] Signed type operations:\n";

        std::vector<int128_tc_t> vec{
            int128_tc_t{-5},
            int128_tc_t{10},
            int128_tc_t{-3},
            int128_tc_t{7}};

        const auto min_it{nstd::min_element<signedness::signed_type, representation_form::twos_complement>(
            vec.begin(), vec.end())};

        const auto max_it{nstd::max_element<signedness::signed_type, representation_form::twos_complement>(
            vec.begin(), vec.end())};

        if ((min_it != vec.end()) && (*min_it == int128_tc_t{-5}) &&
            (max_it != vec.end()) && (*max_it == int128_tc_t{10}))
        {
            std::cout << "  [OK] signed_operations\n";
            TEST_PASS();
        }
        else
        {
            std::cout << "  [FAIL] signed_operations\n";
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
