// =============================================================================
// Test Sweep Framework: Systematic 3-Region Coverage for 128-bit Types
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Provides: TestRegion, SplitMix64, sweep_unary(), sweep_binary(),
//           edge case arrays, and unified reporting.
//
// Usage:
//   #include "test_sweep_framework.hpp"
//
// All tests SHOULD use this header for systematic coverage.
// See docs/PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md.
//
// =============================================================================

#ifndef TEST_SWEEP_FRAMEWORK_HPP
#define TEST_SWEEP_FRAMEWORK_HPP

#include "int128_parameterized.hpp"

#include <cstdint>
#include <array>
#include <iostream>
#include <iomanip>
#include <functional>

// ============================================================================
// Configuration
// ============================================================================

#ifndef SWEEP_REGION_BITS
#define SWEEP_REGION_BITS 21
#endif

static constexpr std::uint64_t SWEEP_REGION_SIZE{1ULL << SWEEP_REGION_BITS};
static constexpr std::uint64_t SWEEP_FIXED_SEED{0xDEADBEEF12345678ULL};

// ============================================================================
// Deterministic PRNG: SplitMix64
// ============================================================================
// Fast, high-quality 64-bit generator. Used for reproducible random regions.
// Reference: https://prng.di.unimi.it/splitmix64.c

class SplitMix64
{
    std::uint64_t state_;

public:
    explicit constexpr SplitMix64(std::uint64_t seed) noexcept : state_{seed} {}

    constexpr std::uint64_t next() noexcept
    {
        state_ += 0x9E3779B97F4A7C15ULL;
        std::uint64_t z{state_};
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};

// ============================================================================
// TestRegion: Generates values from the 3 mandatory regions
// ============================================================================

struct TestRegion
{
    static constexpr std::uint64_t REGION_SIZE{SWEEP_REGION_SIZE};

    // Region 1: First 2^21 values [0, 2^21 - 1]
    static constexpr nstd::uint128_t first_region(std::uint64_t index) noexcept
    {
        return nstd::uint128_t{index};
    }

    // Region 2: Last 2^21 values [MAX - 2^21 + 1, MAX]
    static constexpr nstd::uint128_t last_region(std::uint64_t index) noexcept
    {
        // MAX - (REGION_SIZE - 1 - index) = MAX - REGION_SIZE + 1 + index
        const nstd::uint128_t max_val{nstd::uint128_t::max()};
        const nstd::uint128_t offset{REGION_SIZE - 1ULL - index};
        return max_val - offset;
    }

    // Region 3: Random intermediate values (deterministic PRNG)
    static nstd::uint128_t random_region(std::uint64_t index, std::uint64_t seed = SWEEP_FIXED_SEED) noexcept
    {
        // Each index produces a unique 128-bit value via 2 PRNG calls
        SplitMix64 rng{seed + index * 2};
        const std::uint64_t lo{rng.next()};
        const std::uint64_t hi{rng.next()};
        // Ensure value is in the intermediate range (not in first/last regions)
        // by OR-ing bit 21 in low limb and clearing top bit in high limb
        const std::uint64_t adj_lo{lo | (1ULL << SWEEP_REGION_BITS)};
        const std::uint64_t adj_hi{hi & 0x7FFFFFFFFFFFFFFFULL};
        return nstd::uint128_t{adj_hi, adj_lo};
    }
};

// ============================================================================
// Edge Cases
// ============================================================================

static const std::array<nstd::uint128_t, 20> UNARY_EDGE_CASES{{
    nstd::uint128_t{0ULL},                                         // zero
    nstd::uint128_t{1ULL},                                         // one
    nstd::uint128_t::max(),                                        // MAX
    nstd::uint128_t::max() - nstd::uint128_t{1ULL},                // MAX-1
    nstd::uint128_t{0xFFFFFFFFFFFFFFFFULL},                        // 2^64 - 1 (max low limb)
    nstd::uint128_t{1ULL, 0ULL},                                   // 2^64 (first high limb)
    nstd::uint128_t{0x8000000000000000ULL, 0ULL},                  // 2^127 (MSB)
    nstd::uint128_t{1ULL << 0},                                    // 2^0
    nstd::uint128_t{1ULL << 7},                                    // 2^7
    nstd::uint128_t{1ULL << 8},                                    // 2^8
    nstd::uint128_t{1ULL << 15},                                   // 2^15
    nstd::uint128_t{1ULL << 16},                                   // 2^16
    nstd::uint128_t{1ULL << 31},                                   // 2^31
    nstd::uint128_t{1ULL << 32},                                   // 2^32
    nstd::uint128_t{1ULL << 63},                                   // 2^63
    nstd::uint128_t{1ULL, 0ULL},                                   // 2^64
    nstd::uint128_t{0x5555555555555555ULL, 0x5555555555555555ULL}, // alternating 01
    nstd::uint128_t{0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL}, // alternating 10
    nstd::uint128_t{0xFF00FF00FF00FF00ULL, 0xFF00FF00FF00FF00ULL}, // byte pattern
    nstd::uint128_t{0x00FF00FF00FF00FFULL, 0x00FF00FF00FF00FFULL}, // inverse byte
}};

struct BinaryEdgeCase
{
    nstd::uint128_t x;
    nstd::uint128_t y;
};

static const std::array<BinaryEdgeCase, 12> BINARY_EDGE_CASES{{
    {nstd::uint128_t{0ULL}, nstd::uint128_t{0ULL}},                                   // (0, 0)
    {nstd::uint128_t{0ULL}, nstd::uint128_t::max()},                                  // (0, MAX)
    {nstd::uint128_t::max(), nstd::uint128_t::max()},                                 // (MAX, MAX)
    {nstd::uint128_t{1ULL}, nstd::uint128_t::max()},                                  // (1, MAX)
    {nstd::uint128_t::max(), nstd::uint128_t{1ULL}},                                  // (MAX, 1)
    {nstd::uint128_t{0xFFFFFFFFFFFFFFFFULL}, nstd::uint128_t{1ULL}},                  // (2^64-1, 1)
    {nstd::uint128_t{1ULL, 0ULL}, nstd::uint128_t{1ULL, 0ULL}},                       // (2^64, 2^64)
    {nstd::uint128_t{1ULL}, nstd::uint128_t{1ULL}},                                   // (1, 1)
    {nstd::uint128_t{0xFFFFFFFFFFFFFFFFULL}, nstd::uint128_t{0xFFFFFFFFFFFFFFFFULL}}, // (2^64-1, 2^64-1)
    {nstd::uint128_t{0x8000000000000000ULL, 0ULL}, nstd::uint128_t{1ULL}},            // (2^127, 1)
    {nstd::uint128_t{42ULL}, nstd::uint128_t{0ULL}}, // (42, 0) - neutral/absorbing
    {nstd::uint128_t{0ULL}, nstd::uint128_t{42ULL}}, // (0, 42) - neutral/absorbing
}};

// ============================================================================
// Sweep Reporting
// ============================================================================

struct SweepResult
{
    const char *name;
    std::uint64_t pass;
    std::uint64_t fail;
    std::uint64_t total;
};

static void print_sweep_result(const SweepResult &r)
{
    const char *status{(r.fail == 0) ? "[OK]" : "[FAIL]"};
    std::cout << "  " << std::left << std::setw(30) << r.name << " " << status << "  pass=" << r.pass
              << "  fail=" << r.fail << "  total=" << r.total << "\n";
}

static void print_sweep_separator() { std::cout << "  " << std::string(70, '-') << "\n"; }

// ============================================================================
// sweep_unary: Test a unary function f(x) across all 3 regions + edge cases
// ============================================================================
//
// F:      callable(uint128_t) -> R
// Oracle: callable(uint128_t) -> R  (reference implementation)
// Returns: true if all tests pass

template <typename F, typename Oracle>
bool sweep_unary(F &&func, Oracle &&oracle, const char *name)
{
    constexpr std::uint64_t N{TestRegion::REGION_SIZE};
    std::uint64_t pass{0};
    std::uint64_t fail{0};

    // Region 1: first values
    for (std::uint64_t i{0}; i < N; ++i)
    {
        const auto x{TestRegion::first_region(i)};
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }

    // Region 2: last values
    for (std::uint64_t i{0}; i < N; ++i)
    {
        const auto x{TestRegion::last_region(i)};
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }

    // Region 3: random intermediate
    for (std::uint64_t i{0}; i < N; ++i)
    {
        const auto x{TestRegion::random_region(i)};
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }

    // Edge cases
    for (const auto &x : UNARY_EDGE_CASES)
    {
        if (func(x) != oracle(x))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }

    const std::uint64_t total{pass + fail};
    const SweepResult result{name, pass, fail, total};
    print_sweep_result(result);
    return fail == 0;
}

// ============================================================================
// sweep_binary: Test a binary function g(x,y) across 6 region combos + edges
// ============================================================================
//
// G:      callable(uint128_t, uint128_t) -> R
// Oracle: callable(uint128_t, uint128_t) -> R
// Returns: true if all tests pass

template <typename G, typename Oracle>
bool sweep_binary(G &&func, Oracle &&oracle, const char *name)
{
    constexpr std::uint64_t N{TestRegion::REGION_SIZE};
    std::uint64_t pass{0};
    std::uint64_t fail{0};

    // Helper lambda for a region pair
    const auto sweep_pair = [&](auto gen_x, auto gen_y, std::uint64_t count)
    {
        for (std::uint64_t i{0}; i < count; ++i)
        {
            const auto x{gen_x(i)};
            const auto y{gen_y(i)};
            if (func(x, y) != oracle(x, y))
            {
                ++fail;
            }
            else
            {
                ++pass;
            }
        }
    };

    // Use a different seed offset for y-generation so x != y in random regions
    constexpr std::uint64_t SEED_Y_OFFSET{0x0123456789ABCDEFULL};

    const auto rand_x = [](std::uint64_t i) { return TestRegion::random_region(i); };
    const auto rand_y = [](std::uint64_t i)
    { return TestRegion::random_region(i, SWEEP_FIXED_SEED + 0x0123456789ABCDEFULL); };
    const auto first = [](std::uint64_t i) { return TestRegion::first_region(i); };
    const auto last = [](std::uint64_t i) { return TestRegion::last_region(i); };

    // 6 region combinations
    sweep_pair(first, first, N);   // (first, first)
    sweep_pair(first, last, N);    // (first, last)
    sweep_pair(first, rand_y, N);  // (first, random)
    sweep_pair(last, last, N);     // (last, last)
    sweep_pair(last, rand_y, N);   // (last, random)
    sweep_pair(rand_x, rand_y, N); // (random, random)

    // Binary edge cases
    for (const auto &ec : BINARY_EDGE_CASES)
    {
        if (func(ec.x, ec.y) != oracle(ec.x, ec.y))
        {
            ++fail;
        }
        else
        {
            ++pass;
        }
    }

    const std::uint64_t total{pass + fail};
    const SweepResult result{name, pass, fail, total};
    print_sweep_result(result);
    return fail == 0;
}

// ============================================================================
// Convenience: sweep summary printer
// ============================================================================

static void print_sweep_summary(int passed, int total)
{
    std::cout << "\n========================================\n";
    std::cout << "  SWEEP SUMMARY: " << passed << "/" << total;
    if (passed == total)
    {
        std::cout << " [ALL PASS]\n";
    }
    else
    {
        std::cout << " [" << (total - passed) << " FAILED]\n";
    }
    std::cout << "========================================\n";
}

#endif // TEST_SWEEP_FRAMEWORK_HPP
