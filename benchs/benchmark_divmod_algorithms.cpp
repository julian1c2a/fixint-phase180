// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
/**
 * @file benchmark_divmod_algorithms.cpp
 * @brief Comprehensive benchmarking of division algorithms using RDTSC
 *
 * Compares performance of big_bin_divrem vs D_knuth_divrem
 * Measures throughput in CPU cycles/operation (RDTSC methodology).
 *
 * @author Julián Calderón Almendros
 * @date 5 February 2026 (migrated to RDTSC: 20 March 2026)
 * @version 2.0.0
 */

#include "int128_parameterized.hpp"
#include "bench_common.hpp"

#include <array>
#include <vector>
#include <cmath>

// ====================================================================
// BENCHMARK CONFIGURATION
// ====================================================================

// Divmod-specific iteration counts (overridable via -D)
#ifndef DIVMOD_ITERATIONS
#define DIVMOD_ITERATIONS 5000000
#endif

#ifndef DIVMOD_WARMUP
#define DIVMOD_WARMUP 10000
#endif

static constexpr std::size_t DIV_ITERS{DIVMOD_ITERATIONS};
static constexpr std::size_t DIV_WARMUP{DIVMOD_WARMUP};

// ====================================================================
// TEST CASE DEFINITIONS
// ====================================================================

struct TestCase
{
    const char *name;
    uint64_t dividend_high;
    uint64_t dividend_low;
    uint64_t divisor_high;
    uint64_t divisor_low;
    const char *description;
};

// 9 Test cases covering different scenarios
static constexpr std::array<TestCase, 9> test_cases{
    TestCase{
        "Power-of-2",
        0x8000000000000000ULL, 0x0ULL, // 2^127
        0x0ULL, 0x2ULL,                // 2
        "Level 1: Power-of-2 divisor (1 shift operation)"},
    TestCase{
        "64-bit values",
        0x0ULL, 100ULL, // 100
        0x0ULL, 7ULL,   // 7
        "Level 3: Both fit in 64 bits (native CPU division)"},
    TestCase{
        "128/64 hybrid",
        0x0ULL, 0x0000000100000000ULL, // 2^32
        0x0ULL, 0x0100ULL,             // 2^8
        "Level 4: 128-bit / 64-bit (hybrid algorithm)"},
    TestCase{
        "Large 128/128",
        0x8000000000000000ULL, 0x0ULL, // 2^127
        0x0ULL, 0x2ULL,                // 2
        "Level 6: Full 128/128 binary long division"},
    TestCase{
        "Small divisor",
        0x0ULL, 42ULL, // 42
        0x0ULL, 3ULL,  // 3
        "Level 2: Small specific divisor"},
    TestCase{
        "Remainder test",
        0x0ULL, 17ULL, // 17
        0x0ULL, 5ULL,  // 5
        "Level 3: 64-bit division with remainder"},
    TestCase{
        "Equal values",
        0x0ULL, 42ULL, // 42
        0x0ULL, 42ULL, // 42
        "Level: Divisor == Dividend (fast path)"},
    TestCase{
        "Divide by one",
        0x0ULL, 12345ULL, // 12345
        0x0ULL, 1ULL,     // 1
        "Level: Divisor == 1 (ultra-fast path)"},
    TestCase{
        "Large quotient",
        0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, // 2^128 - 1
        0x0ULL, 2ULL,                                 // 2
        "Level 3: Large quotient (64-bit division)"}};

// ====================================================================
// BENCHMARK UTILITIES
// ====================================================================

// ====================================================================
// DIVMOD BENCHMARK RESULT
// ====================================================================

struct DivmodResult
{
    const char *algorithm_name;
    const char *test_name;
    double cycles_per_op;
};

// ====================================================================
// BENCHMARK RUNNER (RDTSC-based)
// ====================================================================

using uint128_t = nstd::int128_param_t<
    nstd::signedness::signed_type,
    nstd::representation_form::twos_complement>;

static DivmodResult run_divmod_bench(
    const char *algorithm_name,
    const char *test_name,
    uint64_t dividend_high,
    uint64_t dividend_low,
    uint64_t divisor_high,
    uint64_t divisor_low,
    bool use_big_bin)
{
    uint128_t dividend{dividend_high, dividend_low};
    uint128_t divisor{divisor_high, divisor_low};

    // Warmup
    for (std::size_t i{0}; i < DIV_WARMUP; ++i)
    {
        auto result{use_big_bin ? dividend.big_bin_divrem(divisor)
                                : dividend.D_knuth_divrem(divisor)};
        doNotOptimize(result);
    }

    // Measure
    CycleTimer timer;
    for (std::size_t i{0}; i < DIV_ITERS; ++i)
    {
        auto result{use_big_bin ? dividend.big_bin_divrem(divisor)
                                : dividend.D_knuth_divrem(divisor)};
        doNotOptimize(result);
    }
    const double cycles{static_cast<double>(timer.elapsed_cycles())};

    return {algorithm_name, test_name, cycles / DIV_ITERS};
}

// ====================================================================
// REPORTING (cycles/op + ratio)
// ====================================================================

static void print_divmod_header()
{
    std::cout << "+----------------------+-----------------+--------------+--------------+-----------+\n";
    std::cout << "| Test Case            | Algorithm       |  cyc/op      | vs big_bin   | winner    |\n";
    std::cout << "+----------------------+-----------------+--------------+--------------+-----------+\n";
}

static void print_divmod_row(
    const DivmodResult &r,
    double baseline_cyc)
{
    const double ratio{(baseline_cyc > 0.0) ? r.cycles_per_op / baseline_cyc : 0.0};
    const char *winner{""};
    if (baseline_cyc > 0.0 && ratio < 0.95)
    {
        winner = "[FASTER]";
    }
    else if (baseline_cyc > 0.0 && ratio > 1.05)
    {
        winner = "[SLOWER]";
    }
    else
    {
        winner = "[~SAME]";
    }

    std::cout << "| " << std::left << std::setw(20) << r.test_name
              << " | " << std::setw(15) << r.algorithm_name
              << " | " << std::right << std::fixed << std::setprecision(2) << std::setw(12) << r.cycles_per_op
              << " | " << std::fixed << std::setprecision(2) << std::setw(12) << ratio << " | "
              << std::left << std::setw(9) << winner << " |\n";
}

static void print_divmod_separator()
{
    std::cout << "+----------------------+-----------------+--------------+--------------+-----------+\n";
}

// ====================================================================
// MAIN BENCHMARKING ROUTINE
// ====================================================================

int main()
{
    std::cout << "====================================================================\n"
              << "DIVISION ALGORITHM BENCHMARKING: big_bin_divrem vs D_knuth_divrem\n"
              << "  Methodology: RDTSC (CPU cycles/op) -- bench_common.hpp\n"
              << "====================================================================\n\n";

    std::cout << "Configuration:\n"
              << "  Iterations per test: " << DIV_ITERS << "\n"
              << "  Warmup iterations:   " << DIV_WARMUP << "\n"
              << "  Test cases:          " << test_cases.size() << "\n"
              << "  Measurement:         RDTSC (cycles/op)\n\n";

    // ====================================================================
    // PHASE 1: Per-test comparison
    // ====================================================================

    std::cout << "====================================================================\n"
              << "PHASE 1: Per-Test Algorithm Comparison (cycles/op)\n"
              << "====================================================================\n\n";

    print_divmod_header();

    std::vector<DivmodResult> binary_results;
    std::vector<DivmodResult> knuth_results;

    for (const auto &test : test_cases)
    {
        // big_bin_divrem (baseline for ratio)
        const auto bin_r{run_divmod_bench(
            "big_bin_divrem", test.name,
            test.dividend_high, test.dividend_low,
            test.divisor_high, test.divisor_low,
            true)};
        binary_results.push_back(bin_r);

        // D_knuth_divrem
        const auto knuth_r{run_divmod_bench(
            "D_knuth_divrem", test.name,
            test.dividend_high, test.dividend_low,
            test.divisor_high, test.divisor_low,
            false)};
        knuth_results.push_back(knuth_r);

        // Print both rows with big_bin as baseline
        print_divmod_row(bin_r, bin_r.cycles_per_op);
        print_divmod_row(knuth_r, bin_r.cycles_per_op);
        print_divmod_separator();
    }

    // ====================================================================
    // PHASE 2: Summary Statistics
    // ====================================================================

    std::cout << "\n====================================================================\n"
              << "PHASE 2: Summary Statistics\n"
              << "====================================================================\n\n";

    double avg_binary_cyc{0.0};
    double avg_knuth_cyc{0.0};

    for (std::size_t i{0}; i < binary_results.size(); ++i)
    {
        avg_binary_cyc += binary_results[i].cycles_per_op;
        avg_knuth_cyc += knuth_results[i].cycles_per_op;
    }

    avg_binary_cyc /= static_cast<double>(binary_results.size());
    avg_knuth_cyc /= static_cast<double>(knuth_results.size());

    const double avg_speedup{avg_binary_cyc / avg_knuth_cyc};
    const double avg_ratio{avg_speedup > 1.0 ? avg_speedup : 1.0 / avg_speedup};

    std::cout << "Average Performance Across All " << test_cases.size() << " Tests:\n"
              << "  big_bin_divrem:  " << std::fixed << std::setprecision(2) << avg_binary_cyc << " cyc/op\n"
              << "  D_knuth_divrem:  " << std::fixed << std::setprecision(2) << avg_knuth_cyc << " cyc/op\n"
              << "  Average speedup: "
              << (avg_speedup > 1.0 ? "Knuth D is " : "Binary is ")
              << std::fixed << std::setprecision(2) << avg_ratio << "x faster\n\n";

    // ====================================================================
    // IMPLEMENTATION STATUS
    // ====================================================================

    std::cout << "====================================================================\n"
              << "IMPLEMENTATION STATUS\n"
              << "====================================================================\n\n"
              << "D_knuth_divrem: Implemented with optimized fast paths:\n"
              << "  - Power-of-2 divisor: single shift operation\n"
              << "  - 64/64 division: native CPU div instruction\n"
              << "  - 128/64 division: intrinsics::div128_64_composed()\n"
              << "  - 128/128 division: __uint128_t native (GCC/Clang)\n"
              << "  - MSVC fallback: delegates to big_bin_divrem()\n\n"
              << "big_bin_divrem: Binary long division O(128) baseline\n\n"
              << "====================================================================\n"
              << "BENCHMARK COMPLETE\n"
              << "====================================================================\n";

    return 0;
}
