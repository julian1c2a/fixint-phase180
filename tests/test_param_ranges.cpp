// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
/**
 * @file test_param_ranges.cpp
 * @brief Tests for int128_param_ranges.hpp - Range auxiliary functions
 * Part of int128 Library - https://github.com/julian1c2a/int128-phase175
 * License: BSL-1.0
 */

#include "int128_param_ranges.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

using nstd::int128_ek_t;
using nstd::int128_ms_t;
using nstd::int128_tc_t;
using nstd::uint128_t;

// Test macros with ASCII output only
#define TEST_START(name) std::cout << "[Test " << test_num++ << "] " << name << ": "
#define ASSERT(cond)             \
    if (!(cond))                 \
    {                            \
        std::cout << "[FAIL]\n"; \
        ++failed;                \
        return;                  \
    }
#define TEST_END()         \
    std::cout << "[OK]\n"; \
    ++passed

int test_num = 1;
int passed = 0;
int failed = 0;

// ============================================================================
// Test 1: Arithmetic Sequence Generation
// ============================================================================
void test_arithmetic_sequence()
{
    TEST_START("Arithmetic sequence generation");

    // Test 1.1: Basic arithmetic sequence (unsigned)
    {
        std::vector<uint128_t> vec(5);
        nstd::int128_ranges::generate_arithmetic_sequence<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{10}, uint128_t{5});

        ASSERT(vec[0] == uint128_t{10});
        ASSERT(vec[1] == uint128_t{15});
        ASSERT(vec[2] == uint128_t{20});
        ASSERT(vec[3] == uint128_t{25});
        ASSERT(vec[4] == uint128_t{30});
    }

    // Test 1.2: Arithmetic sequence with signed TC
    {
        std::vector<int128_tc_t> vec(4);
        nstd::int128_ranges::generate_arithmetic_sequence<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end(), int128_tc_t{-10}, int128_tc_t{3});

        ASSERT(vec[0] == int128_tc_t{-10});
        ASSERT(vec[1] == int128_tc_t{-7});
        ASSERT(vec[2] == int128_tc_t{-4});
        ASSERT(vec[3] == int128_tc_t{-1});
    }

    TEST_END();
}

// ============================================================================
// Test 2: Iota (Unit Step Sequence)
// ============================================================================
void test_iota()
{
    TEST_START("Iota (unit step sequence)");

    // Test 2.1: Basic iota (unsigned)
    {
        std::vector<uint128_t> vec(5);
        nstd::int128_ranges::iota<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{100});

        ASSERT(vec[0] == uint128_t{100});
        ASSERT(vec[1] == uint128_t{101});
        ASSERT(vec[2] == uint128_t{102});
        ASSERT(vec[3] == uint128_t{103});
        ASSERT(vec[4] == uint128_t{104});
    }

    // Test 2.2: Iota with TC signed
    {
        std::vector<int128_tc_t> vec(3);
        nstd::int128_ranges::iota<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end(), int128_tc_t{-2});

        ASSERT(vec[0] == int128_tc_t{-2});
        ASSERT(vec[1] == int128_tc_t{-1});
        ASSERT(vec[2] == int128_tc_t{0});
    }

    TEST_END();
}

// ============================================================================
// Test 3: Geometric Sequence Generation
// ============================================================================
void test_geometric_sequence()
{
    TEST_START("Geometric sequence generation");

    // Test 3.1: Basic geometric sequence (unsigned)
    {
        std::vector<uint128_t> vec(5);
        nstd::int128_ranges::generate_geometric_sequence<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{2}, uint128_t{3});

        ASSERT(vec[0] == uint128_t{2});
        ASSERT(vec[1] == uint128_t{6});
        ASSERT(vec[2] == uint128_t{18});
        ASSERT(vec[3] == uint128_t{54});
        ASSERT(vec[4] == uint128_t{162});
    }

    // Test 3.2: Geometric with ratio 2 (TC)
    {
        std::vector<int128_tc_t> vec(4);
        nstd::int128_ranges::generate_geometric_sequence<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end(), int128_tc_t{3}, int128_tc_t{2});

        ASSERT(vec[0] == int128_tc_t{3});
        ASSERT(vec[1] == int128_tc_t{6});
        ASSERT(vec[2] == int128_tc_t{12});
        ASSERT(vec[3] == int128_tc_t{24});
    }

    TEST_END();
}

// ============================================================================
// Test 4: Powers of 2 Generation
// ============================================================================
void test_powers_of_2()
{
    TEST_START("Powers of 2 generation");

    // Test 4.1: Powers of 2 starting from 2^0
    {
        std::vector<uint128_t> vec(5);
        nstd::int128_ranges::generate_powers_of_2<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), 0);

        ASSERT(vec[0] == uint128_t{1});
        ASSERT(vec[1] == uint128_t{2});
        ASSERT(vec[2] == uint128_t{4});
        ASSERT(vec[3] == uint128_t{8});
        ASSERT(vec[4] == uint128_t{16});
    }

    // Test 4.2: Powers of 2 starting from 2^3
    {
        std::vector<uint128_t> vec(4);
        nstd::int128_ranges::generate_powers_of_2<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), 3);

        ASSERT(vec[0] == uint128_t{8});
        ASSERT(vec[1] == uint128_t{16});
        ASSERT(vec[2] == uint128_t{32});
        ASSERT(vec[3] == uint128_t{64});
    }

    TEST_END();
}

// ============================================================================
// Test 5: Range Statistics
// ============================================================================
void test_range_stats()
{
    TEST_START("Range statistics calculation");

    // Test 5.1: Basic stats (unsigned)
    {
        std::vector<uint128_t> vec{uint128_t{5}, uint128_t{10}, uint128_t{3}, uint128_t{8}, uint128_t{2}};

        auto stats = nstd::int128_ranges::calculate_stats<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end());

        ASSERT(stats.valid == true);
        ASSERT(stats.count == 5);
        ASSERT(stats.sum == uint128_t{28});
        ASSERT(stats.min_val == uint128_t{2});
        ASSERT(stats.max_val == uint128_t{10});
        ASSERT(stats.average() == uint128_t{5}); // 28/5 = 5 (truncated)
        ASSERT(stats.range() == uint128_t{8});   // 10 - 2 = 8
    }

    // Test 5.2: Stats with signed values (TC)
    {
        std::vector<int128_tc_t> vec{int128_tc_t{-5}, int128_tc_t{10}, int128_tc_t{-3}, int128_tc_t{7}};

        auto stats = nstd::int128_ranges::calculate_stats<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end());

        ASSERT(stats.valid == true);
        ASSERT(stats.count == 4);
        ASSERT(stats.sum == int128_tc_t{9}); // -5 + 10 - 3 + 7 = 9
        ASSERT(stats.min_val == int128_tc_t{-5});
        ASSERT(stats.max_val == int128_tc_t{10});
        ASSERT(stats.average() == int128_tc_t{2}); // 9/4 = 2 (truncated)
        ASSERT(stats.range() == int128_tc_t{15});  // 10 - (-5) = 15
    }

    // Test 5.3: Empty range
    {
        std::vector<uint128_t> vec;

        auto stats = nstd::int128_ranges::calculate_stats<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end());

        ASSERT(stats.valid == false);
        ASSERT(stats.count == 0);
    }

    TEST_END();
}

// ============================================================================
// Test 6: Find First If
// ============================================================================
void test_find_first_if()
{
    TEST_START("Find first element matching predicate");

    // Test 6.1: Find first even number
    {
        std::vector<uint128_t> vec{uint128_t{1}, uint128_t{3}, uint128_t{6}, uint128_t{8}};

        auto result = nstd::int128_ranges::find_first_if<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), [](const uint128_t &x)
            {
                return (x.low() & 1) == 0; // even
            });

        ASSERT(result.has_value());
        ASSERT(*result == uint128_t{6});
    }

    // Test 6.2: No match
    {
        std::vector<uint128_t> vec{uint128_t{1}, uint128_t{3}, uint128_t{5}};

        auto result = nstd::int128_ranges::find_first_if<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), [](const uint128_t &x)
            {
                return (x.low() & 1) == 0; // even
            });

        ASSERT(!result.has_value());
    }

    TEST_END();
}

// ============================================================================
// Test 7: Count If
// ============================================================================
void test_count_if()
{
    TEST_START("Count elements matching predicate");

    // Test 7.1: Count even numbers
    {
        std::vector<uint128_t> vec{uint128_t{1}, uint128_t{2}, uint128_t{3}, uint128_t{4}, uint128_t{5}, uint128_t{6}};

        auto count = nstd::int128_ranges::count_if<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), [](const uint128_t &x)
            {
                return (x.low() & 1) == 0; // even
            });

        ASSERT(count == 3); // 2, 4, 6
    }

    // Test 7.2: Count negative numbers (TC)
    {
        std::vector<int128_tc_t> vec{int128_tc_t{-5}, int128_tc_t{2}, int128_tc_t{-3}, int128_tc_t{7}, int128_tc_t{-1}};

        auto count = nstd::int128_ranges::count_if<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end(), [](const int128_tc_t &x)
            { return x.is_negative(); });

        ASSERT(count == 3); // -5, -3, -1
    }

    TEST_END();
}

// ============================================================================
// Test 8: Transform
// ============================================================================
void test_transform()
{
    TEST_START("Transform (map operation)");

    // Test 8.1: Double all values
    {
        std::vector<uint128_t> input{uint128_t{1}, uint128_t{2}, uint128_t{3}};
        std::vector<uint128_t> output(3);

        nstd::int128_ranges::transform<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            input.begin(), input.end(), output.begin(),
            [](const uint128_t &x)
            { return x * uint128_t{2}; });

        ASSERT(output[0] == uint128_t{2});
        ASSERT(output[1] == uint128_t{4});
        ASSERT(output[2] == uint128_t{6});
    }

    // Test 8.2: Negate all values (TC)
    {
        std::vector<int128_tc_t> input{int128_tc_t{5}, int128_tc_t{-3}, int128_tc_t{7}};
        std::vector<int128_tc_t> output(3);

        nstd::int128_ranges::transform<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            input.begin(), input.end(), output.begin(),
            [](const int128_tc_t &x)
            { return -x; });

        ASSERT(output[0] == int128_tc_t{-5});
        ASSERT(output[1] == int128_tc_t{3});
        ASSERT(output[2] == int128_tc_t{-7});
    }

    TEST_END();
}

// ============================================================================
// Test 9: Copy If (Filter)
// ============================================================================
void test_copy_if()
{
    TEST_START("Copy if (filter operation)");

    // Test 9.1: Filter even numbers
    {
        std::vector<uint128_t> input{uint128_t{1}, uint128_t{2}, uint128_t{3}, uint128_t{4}, uint128_t{5}, uint128_t{6}};
        std::vector<uint128_t> output(6); // oversize

        auto end = nstd::int128_ranges::copy_if<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            input.begin(), input.end(), output.begin(),
            [](const uint128_t &x)
            { return (x.low() & 1) == 0; });

        std::size_t result_size = std::distance(output.begin(), end);
        ASSERT(result_size == 3);
        ASSERT(output[0] == uint128_t{2});
        ASSERT(output[1] == uint128_t{4});
        ASSERT(output[2] == uint128_t{6});
    }

    // Test 9.2: Filter positive numbers (TC)
    {
        std::vector<int128_tc_t> input{int128_tc_t{-5}, int128_tc_t{2}, int128_tc_t{-3}, int128_tc_t{7}};
        std::vector<int128_tc_t> output(4);

        auto end = nstd::int128_ranges::copy_if<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            input.begin(), input.end(), output.begin(),
            [](const int128_tc_t &x)
            { return !x.is_negative() && !x.is_zero(); });

        std::size_t result_size = std::distance(output.begin(), end);
        ASSERT(result_size == 2);
        ASSERT(output[0] == int128_tc_t{2});
        ASSERT(output[1] == int128_tc_t{7});
    }

    TEST_END();
}

// ============================================================================
// Test 10: Reduce
// ============================================================================
void test_reduce()
{
    TEST_START("Reduce with custom operation");

    // Test 10.1: Sum using reduce (unsigned)
    {
        std::vector<uint128_t> vec{uint128_t{10}, uint128_t{20}, uint128_t{30}};

        auto result = nstd::int128_ranges::reduce<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end(), uint128_t{0},
            [](const uint128_t &a, const uint128_t &b)
            { return a + b; });

        ASSERT(result == uint128_t{60});
    }

    // Test 10.2: Max using reduce (TC)
    {
        std::vector<int128_tc_t> vec{int128_tc_t{-5}, int128_tc_t{12}, int128_tc_t{7}, int128_tc_t{-3}};

        auto result = nstd::int128_ranges::reduce<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end(), vec[0],
            [](const int128_tc_t &a, const int128_tc_t &b)
            { return (a > b) ? a : b; });

        ASSERT(result == int128_tc_t{12});
    }

    TEST_END();
}

// ============================================================================
// Test 11: Sum
// ============================================================================
void test_sum()
{
    TEST_START("Sum of all elements");

    // Test 11.1: Basic sum (unsigned)
    {
        std::vector<uint128_t> vec{uint128_t{5}, uint128_t{10}, uint128_t{15}};

        auto result = nstd::int128_ranges::sum<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end());

        ASSERT(result == uint128_t{30});
    }

    // Test 11.2: Sum with mixed signs (TC)
    {
        std::vector<int128_tc_t> vec{int128_tc_t{-10}, int128_tc_t{25}, int128_tc_t{-5}, int128_tc_t{15}};

        auto result = nstd::int128_ranges::sum<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end());

        ASSERT(result == int128_tc_t{25}); // -10 + 25 - 5 + 15 = 25
    }

    TEST_END();
}

// ============================================================================
// Test 12: Product
// ============================================================================
void test_product()
{
    TEST_START("Product of all elements");

    // Test 12.1: Basic product (unsigned)
    {
        std::vector<uint128_t> vec{uint128_t{2}, uint128_t{3}, uint128_t{4}};

        auto result = nstd::int128_ranges::product<nstd::signedness::unsigned_type, nstd::representation_form::binnat>(
            vec.begin(), vec.end());

        ASSERT(result == uint128_t{24}); // 2 * 3 * 4 = 24
    }

    // Test 12.2: Product with signed values (TC)
    {
        std::vector<int128_tc_t> vec{int128_tc_t{2}, int128_tc_t{3}, int128_tc_t{5}};

        auto result = nstd::int128_ranges::product<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end());

        ASSERT(result == int128_tc_t{30}); // 2 * 3 * 5 = 30
    }

    TEST_END();
}

// ============================================================================
// Test 13: MS Representation Support
// ============================================================================
void test_ms_representation()
{
    TEST_START("TC mixed operations (unsigned + signed)");

    // Test 13.1: Powers of 2 with signed TC
    {
        std::vector<int128_tc_t> vec(4);
        nstd::int128_ranges::generate_powers_of_2<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end(), 2);

        ASSERT(vec[0] == int128_tc_t{4});
        ASSERT(vec[1] == int128_tc_t{8});
        ASSERT(vec[2] == int128_tc_t{16});
        ASSERT(vec[3] == int128_tc_t{32});
    }

    // Test 13.2: Stats with TC signed
    {
        std::vector<int128_tc_t> vec{int128_tc_t{-10}, int128_tc_t{5}, int128_tc_t{-3}};

        auto stats = nstd::int128_ranges::calculate_stats<nstd::signedness::signed_type, nstd::representation_form::twos_complement>(
            vec.begin(), vec.end());

        ASSERT(stats.valid == true);
        ASSERT(stats.count == 3);
        ASSERT(stats.min_val == int128_tc_t{-10});
        ASSERT(stats.max_val == int128_tc_t{5});
    }

    TEST_END();
}

// ============================================================================
// Main
// ============================================================================
int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Range Auxiliary Functions Tests (int128_param_ranges.hpp)\n";
    std::cout << "====================================================================\n\n";

    test_arithmetic_sequence(); // Test 1
    test_iota();                // Test 2
    test_geometric_sequence();  // Test 3
    test_powers_of_2();         // Test 4
    test_range_stats();         // Test 5
    test_find_first_if();       // Test 6
    test_count_if();            // Test 7
    test_transform();           // Test 8
    test_copy_if();             // Test 9
    test_reduce();              // Test 10
    test_sum();                 // Test 11
    test_product();             // Test 12
    test_ms_representation();   // Test 13

    std::cout << "\n====================================================================\n";
    std::cout << "RESULTS:\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n";
    std::cout << "  Total:  " << (passed + failed) << "\n";
    std::cout << "====================================================================\n";

    return (failed > 0) ? 1 : 0;
}
