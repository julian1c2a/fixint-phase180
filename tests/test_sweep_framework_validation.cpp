// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: test_sweep_framework - Validates the sweep testing infrastructure
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "test_sweep_framework.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

int g_passed{0};
int g_failed{0};

#define TEST_PASS() (++g_passed)
#define TEST_FAIL() (++g_failed)

// ============================================================================
// Section 1: TestRegion generation tests
// ============================================================================

void test_first_region()
{
    std::cout << "  Testing first_region...\n";

    // first_region(0) should be 0
    const auto v0{TestRegion::first_region(0)};
    if (v0 == uint128_t{0ULL})
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: first_region(0) != 0\n";
        TEST_FAIL();
    }

    // first_region(1) should be 1
    const auto v1{TestRegion::first_region(1)};
    if (v1 == uint128_t{1ULL})
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: first_region(1) != 1\n";
        TEST_FAIL();
    }

    // first_region(REGION_SIZE - 1) should be REGION_SIZE - 1
    const auto vmax{TestRegion::first_region(TestRegion::REGION_SIZE - 1)};
    if (vmax == uint128_t{TestRegion::REGION_SIZE - 1})
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: first_region(REGION_SIZE-1) != REGION_SIZE-1\n";
        TEST_FAIL();
    }

    std::cout << "    first_region: done\n";
}

void test_last_region()
{
    std::cout << "  Testing last_region...\n";

    // last_region(REGION_SIZE - 1) should be MAX
    const auto vmax{TestRegion::last_region(TestRegion::REGION_SIZE - 1)};
    if (vmax == uint128_t::max())
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: last_region(REGION_SIZE-1) != MAX\n";
        TEST_FAIL();
    }

    // last_region(0) should be MAX - REGION_SIZE + 1
    const auto v0{TestRegion::last_region(0)};
    const auto expected{uint128_t::max() - uint128_t{TestRegion::REGION_SIZE - 1}};
    if (v0 == expected)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: last_region(0) != MAX - REGION_SIZE + 1\n";
        TEST_FAIL();
    }

    // Monotonic: last_region(i) < last_region(i+1)
    bool monotonic{true};
    for (std::uint64_t i{0}; i < 1000; ++i)
    {
        if (TestRegion::last_region(i) >= TestRegion::last_region(i + 1))
        {
            monotonic = false;
            break;
        }
    }
    if (monotonic)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: last_region not monotonically increasing\n";
        TEST_FAIL();
    }

    std::cout << "    last_region: done\n";
}

void test_random_region()
{
    std::cout << "  Testing random_region...\n";

    // Reproducibility: same index + seed => same result
    const auto a{TestRegion::random_region(42)};
    const auto b{TestRegion::random_region(42)};
    if (a == b)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: random_region not deterministic\n";
        TEST_FAIL();
    }

    // Different indices => different results (with overwhelming probability)
    const auto c{TestRegion::random_region(0)};
    const auto d{TestRegion::random_region(1)};
    if (c != d)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: random_region(0) == random_region(1)\n";
        TEST_FAIL();
    }

    // Different seeds => different results
    const auto e{TestRegion::random_region(0, 0xAAAAAAAABBBBBBBBULL)};
    const auto f{TestRegion::random_region(0, 0xCCCCCCCCDDDDDDDDULL)};
    if (e != f)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: different seeds produced same value\n";
        TEST_FAIL();
    }

    // Values should not be trivially zero
    bool all_nonzero{true};
    for (std::uint64_t i{0}; i < 100; ++i)
    {
        if (TestRegion::random_region(i) == uint128_t{0ULL})
        {
            all_nonzero = false;
            break;
        }
    }
    if (all_nonzero)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: random_region produced zero\n";
        TEST_FAIL();
    }

    std::cout << "    random_region: done\n";
}

// ============================================================================
// Section 2: SplitMix64 quality tests
// ============================================================================

void test_splitmix64()
{
    std::cout << "  Testing SplitMix64...\n";

    // Generate 1000 values, check no immediate repeats
    SplitMix64 rng{12345ULL};
    std::uint64_t prev{rng.next()};
    bool has_repeat{false};
    for (int i{0}; i < 999; ++i)
    {
        const std::uint64_t val{rng.next()};
        if (val == prev)
        {
            has_repeat = true;
        }
        prev = val;
    }
    if (!has_repeat)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: SplitMix64 produced consecutive duplicates\n";
        TEST_FAIL();
    }

    // Deterministic: two generators with same seed produce same sequence
    SplitMix64 g1{999ULL};
    SplitMix64 g2{999ULL};
    bool deterministic{true};
    for (int i{0}; i < 100; ++i)
    {
        if (g1.next() != g2.next())
        {
            deterministic = false;
        }
    }
    if (deterministic)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: SplitMix64 not deterministic\n";
        TEST_FAIL();
    }

    std::cout << "    SplitMix64: done\n";
}

// ============================================================================
// Section 3: sweep_unary test (using identity / bit complement)
// ============================================================================

void test_sweep_unary_identity()
{
    std::cout << "  Testing sweep_unary (identity)...\n";

    // Identity function: f(x) = x, oracle(x) = x => should pass 100%
    const bool ok{sweep_unary(
        [](const uint128_t &x) -> uint128_t
        { return x; },
        [](const uint128_t &x) -> uint128_t
        { return x; },
        "identity")};

    if (ok)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: sweep_unary(identity) did not pass\n";
        TEST_FAIL();
    }
}

void test_sweep_unary_complement()
{
    std::cout << "  Testing sweep_unary (bitwise complement)...\n";

    // ~(~x) == x
    const bool ok{sweep_unary(
        [](const uint128_t &x) -> uint128_t
        { return ~(~x); },
        [](const uint128_t &x) -> uint128_t
        { return x; },
        "double complement")};

    if (ok)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: sweep_unary(double complement) did not pass\n";
        TEST_FAIL();
    }
}

// ============================================================================
// Section 4: sweep_binary test (using addition commutativity)
// ============================================================================

void test_sweep_binary_add_commutative()
{
    std::cout << "  Testing sweep_binary (addition commutativity)...\n";

    // a + b == b + a (commutativity)
    const bool ok{sweep_binary(
        [](const uint128_t &x, const uint128_t &y) -> uint128_t
        { return x + y; },
        [](const uint128_t &x, const uint128_t &y) -> uint128_t
        { return y + x; },
        "add commutative")};

    if (ok)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: sweep_binary(add commutative) did not pass\n";
        TEST_FAIL();
    }
}

void test_sweep_binary_xor_self()
{
    std::cout << "  Testing sweep_binary (x ^ x == 0)...\n";

    // x ^ x == 0 (self-xor)
    // Use same generator for both x and y so they are identical
    const bool ok{sweep_binary(
        [](const uint128_t &x, const uint128_t &) -> uint128_t
        { return x ^ x; },
        [](const uint128_t &, const uint128_t &) -> uint128_t
        { return uint128_t{0ULL}; },
        "xor self == 0")};

    if (ok)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: sweep_binary(xor self) did not pass\n";
        TEST_FAIL();
    }
}

// ============================================================================
// Section 5: Edge cases array validation
// ============================================================================

void test_edge_cases()
{
    std::cout << "  Testing edge case arrays...\n";

    // UNARY_EDGE_CASES: first should be 0
    if (UNARY_EDGE_CASES[0] == uint128_t{0ULL})
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: UNARY_EDGE_CASES[0] != 0\n";
        TEST_FAIL();
    }

    // UNARY_EDGE_CASES: second should be 1
    if (UNARY_EDGE_CASES[1] == uint128_t{1ULL})
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: UNARY_EDGE_CASES[1] != 1\n";
        TEST_FAIL();
    }

    // UNARY_EDGE_CASES: third should be max
    if (UNARY_EDGE_CASES[2] == uint128_t::max())
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: UNARY_EDGE_CASES[2] != max\n";
        TEST_FAIL();
    }

    // BINARY_EDGE_CASES: size should be 12
    if (BINARY_EDGE_CASES.size() == 12)
    {
        TEST_PASS();
    }
    else
    {
        std::cout << "    FAIL: BINARY_EDGE_CASES size != 12\n";
        TEST_FAIL();
    }

    std::cout << "    edge cases: done\n";
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << " Test Sweep Framework Validation\n";
    std::cout << " Region size: " << TestRegion::REGION_SIZE << " values\n";
    std::cout << "====================================================================\n\n";

    // Section 1: Region generators
    std::cout << "--- Region Generators ---\n";
    test_first_region();
    test_last_region();
    test_random_region();
    std::cout << "\n";

    // Section 2: PRNG
    std::cout << "--- PRNG Quality ---\n";
    test_splitmix64();
    std::cout << "\n";

    // Section 3: Unary sweeps
    std::cout << "--- Unary Sweep Tests ---\n";
    test_sweep_unary_identity();
    test_sweep_unary_complement();
    std::cout << "\n";

    // Section 4: Binary sweeps
    std::cout << "--- Binary Sweep Tests ---\n";
    test_sweep_binary_add_commutative();
    test_sweep_binary_xor_self();
    std::cout << "\n";

    // Section 5: Edge cases
    std::cout << "--- Edge Case Validation ---\n";
    test_edge_cases();
    std::cout << "\n";

    // Summary
    const int total{g_passed + g_failed};
    std::cout << "====================================================================\n";
    std::cout << " RESULTS: " << g_passed << "/" << total << " passed";
    if (g_failed == 0)
    {
        std::cout << " [ALL PASS]\n";
    }
    else
    {
        std::cout << " [" << g_failed << " FAILED]\n";
    }
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? 0 : 1;
}
