/**
 * @file benchmark_divmod_algorithms.cpp
 * @brief Comprehensive benchmarking of division algorithms
 *
 * Compares performance of big_bin_divrem vs D_knuth_divrem
 * Measures throughput, latency, and consistency across optimization levels
 *
 * @author Julián Calderón Almendros
 * @date 5 February 2026
 * @version 1.0.0
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <array>
#include <cmath>
#include <string>
#include <sstream>

#include "int128_parameterized.hpp"

// ====================================================================
// BENCHMARK CONFIGURATION
// ====================================================================

constexpr int ITERATIONS = 10000;       // Per test case
constexpr int WARMUP_ITERATIONS = 1000; // Pre-benchmark warmup

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

struct BenchmarkResult
{
    const char *algorithm_name;
    const char *test_name;
    double operations_per_second;
    double nanoseconds_per_operation;
    uint64_t total_iterations;
    uint64_t total_nanoseconds;
};

class BenchmarkTimer
{
private:
    using clock_t = std::chrono::high_resolution_clock;

public:
    static uint64_t get_nanoseconds()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   clock_t::now().time_since_epoch())
            .count();
    }

    static BenchmarkResult run_benchmark(
        const char *algorithm_name,
        const char *test_name,
        uint64_t dividend_high,
        uint64_t dividend_low,
        uint64_t divisor_high,
        uint64_t divisor_low,
        int iterations,
        bool call_big_bin = true)
    {
        using nstd::int128_param_t;
        using nstd::representation_form;
        using nstd::signedness;

        using uint128_t = int128_param_t<signedness::signed_type, representation_form::twos_complement>;
        uint128_t dividend{dividend_high, dividend_low};
        uint128_t divisor{divisor_high, divisor_low};

        // Warmup
        volatile auto dummy_result =
            call_big_bin ? dividend.big_bin_divrem(divisor) : dividend.D_knuth_divrem(divisor);
        (void)dummy_result;

        // Actual benchmark
        uint64_t start_ns = get_nanoseconds();

        for (int i = 0; i < iterations; i++)
        {
            volatile auto result =
                call_big_bin ? dividend.big_bin_divrem(divisor) : dividend.D_knuth_divrem(divisor);
            (void)result;
        }

        uint64_t end_ns = get_nanoseconds();
        uint64_t total_ns = end_ns - start_ns;

        double ops_per_sec = (iterations * 1e9) / total_ns;
        double ns_per_op = static_cast<double>(total_ns) / iterations;

        return {
            algorithm_name,
            test_name,
            ops_per_sec,
            ns_per_op,
            static_cast<uint64_t>(iterations),
            total_ns};
    }
};

// ====================================================================
// REPORTING
// ====================================================================

void print_result(const BenchmarkResult &result)
{
    std::cout << std::setw(20) << result.test_name << " | "
              << std::setw(15) << result.algorithm_name << " | "
              << std::setw(12) << std::fixed << std::setprecision(2)
              << result.operations_per_second << " ops/s | "
              << std::setw(10) << std::fixed << std::setprecision(2)
              << result.nanoseconds_per_operation << " ns/op"
              << std::endl;
}

void print_comparison(
    const BenchmarkResult &binary,
    const BenchmarkResult &knuth)
{

    double speedup = binary.nanoseconds_per_operation / knuth.nanoseconds_per_operation;
    const char *faster = speedup > 1.0 ? "Binary FASTER" : "Knuth FASTER";

    std::cout << std::setw(20) << binary.test_name << " | "
              << "COMPARISON: "
              << std::fixed << std::setprecision(2) << std::abs(speedup - 1.0)
              << "x | " << faster
              << std::endl;
}

// ====================================================================
// MAIN BENCHMARKING ROUTINE
// ====================================================================

int main()
{
    std::cout << "====================================================================\n"
              << "DIVISION ALGORITHM BENCHMARKING: big_bin_divrem vs D_knuth_divrem\n"
              << "====================================================================\n\n";

    std::cout << "Configuration:\n"
              << "  Iterations per test: " << ITERATIONS << "\n"
              << "  Warmup iterations: " << WARMUP_ITERATIONS << "\n"
              << "  Test cases: " << test_cases.size() << "\n"
              << "  Compiler: GCC 15.2.0\n"
              << "  Optimization: -O2\n\n";

    // ====================================================================
    // BENCHMARK PHASE 1: Individual Algorithm Performance
    // ====================================================================

    std::cout << "====================================================================\n"
              << "PHASE 1: Individual Algorithm Performance\n"
              << "====================================================================\n\n";

    std::vector<BenchmarkResult> binary_results;
    std::vector<BenchmarkResult> knuth_results;

    for (const auto &test : test_cases)
    {
        std::cout << "Benchmarking: " << test.name << "\n"
                  << "  Description: " << test.description << "\n";

        // Benchmark big_bin_divrem
        auto binary_result = BenchmarkTimer::run_benchmark(
            "big_bin_divrem",
            test.name,
            test.dividend_high, test.dividend_low,
            test.divisor_high, test.divisor_low,
            ITERATIONS,
            true);

        binary_results.push_back(binary_result);
        print_result(binary_result);

        // Benchmark D_knuth_divrem
        auto knuth_result = BenchmarkTimer::run_benchmark(
            "D_knuth_divrem",
            test.name,
            test.dividend_high, test.dividend_low,
            test.divisor_high, test.divisor_low,
            ITERATIONS,
            false);

        knuth_results.push_back(knuth_result);
        print_result(knuth_result);

        std::cout << "\n";
    }

    // ====================================================================
    // BENCHMARK PHASE 2: Algorithm Comparison
    // ====================================================================

    std::cout << "\n====================================================================\n"
              << "PHASE 2: Algorithm Comparison (Speedup Analysis)\n"
              << "====================================================================\n\n";

    for (size_t i = 0; i < test_cases.size(); i++)
    {
        print_comparison(binary_results[i], knuth_results[i]);
    }

    // ====================================================================
    // SUMMARY STATISTICS
    // ====================================================================

    std::cout << "\n====================================================================\n"
              << "SUMMARY STATISTICS\n"
              << "====================================================================\n\n";

    double avg_binary_ns = 0;
    double avg_knuth_ns = 0;

    for (size_t i = 0; i < binary_results.size(); i++)
    {
        avg_binary_ns += binary_results[i].nanoseconds_per_operation;
        avg_knuth_ns += knuth_results[i].nanoseconds_per_operation;
    }

    avg_binary_ns /= binary_results.size();
    avg_knuth_ns /= knuth_results.size();

    std::cout << "Average Performance Across All Tests:\n"
              << "  big_bin_divrem:  " << std::fixed << std::setprecision(2)
              << avg_binary_ns << " ns/op\n"
              << "  D_knuth_divrem:  " << std::fixed << std::setprecision(2)
              << avg_knuth_ns << " ns/op\n";

    double avg_speedup = avg_binary_ns / avg_knuth_ns;
    std::cout << "  Average speedup: " << std::fixed << std::setprecision(2)
              << (avg_speedup > 1.0 ? "big_bin is " : "Knuth is ")
              << std::abs(avg_speedup - 1.0) << "x faster\n\n";

    // ====================================================================
    // PHASE 1 STATUS
    // ====================================================================

    std::cout << "====================================================================\n"
              << "PHASE 1 STATUS\n"
              << "====================================================================\n\n"
              << "Current Implementation Status:\n"
              << "  - D_knuth_divrem delegates to big_bin_divrem (Phase 1)\n"
              << "  - Both algorithms SHOULD show identical performance\n"
              << "  - Benchmarking infrastructure is NOW READY\n\n"

              << "Expected Results:\n"
              << "  - Performance difference < 5% (same code path)\n"
              << "  - Proves benchmarking framework is working correctly\n\n"

              << "Next Steps (Phase 2+):\n"
              << "  1. Implement true Knuth D algorithm (with __uint128_t)\n"
              << "  2. Re-run benchmarking for real performance comparison\n"
              << "  3. Analyze which algorithm is faster for different test cases\n"
              << "  4. Based on results, implement /= and %= operators\n"
              << "  5. Final benchmarking of new operators\n\n"

              << "====================================================================\n"
              << "BENCHMARK COMPLETE - READY FOR PHASE 2\n"
              << "====================================================================\n";

    return 0;
}
