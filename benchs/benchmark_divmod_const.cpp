// =============================================================================
// Benchmark: div<D> (Granlund-Montgomery) vs operator/ (D_knuth_divrem)
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Compares cycle counts for compile-time constant division using:
//   1. n.div<D>()           - Granlund-Montgomery multiply-shift
//   2. n / uint128_t{D}     - General D_knuth_divrem (128-bit long division)
//   3. fast_divN()          - Handcoded algorithms from div_by_const.hpp
//
// Also benchmarks mul<K>() vs operator* for selected K values.
//
// Compile (GCC):
//   g++ -std=c++20 -O2 -Iinclude benchs/benchmark_divmod_const.cpp -o bench_divmod
// =============================================================================

#include "int128_parameterized.hpp"
#include "algorithms/div_by_const.hpp"
#include "bench_common.hpp"

#include <iostream>
#include <iomanip>
#include <cstdint>

using namespace nstd;
using namespace nstd::algorithms;
using std::uint64_t;

// ============================================================================
// Helpers: generate a pseudo-random 128-bit value for benchmarking
// ============================================================================

static uint128_t make_bench_value(uint64_t seed)
{
    // SplitMix64
    uint64_t z{seed + 0x9E3779B97F4A7C15ULL};
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    const uint64_t lo{z ^ (z >> 31)};
    z = (lo + 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    const uint64_t hi{z ^ (z >> 31)};
    return uint128_t{hi, lo};
}

// ============================================================================
// Division benchmarks
// ============================================================================

template <uint64_t D>
static BenchResult bench_div_gm(const char *name)
{
    uint128_t a{make_bench_value(0xDEADBEEF12345678ULL)};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q{a.div<D>()};
        doNotOptimize(q);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    a = make_bench_value(0xDEADBEEF12345678ULL);
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q{a.div<D>()};
        doNotOptimize(q);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {name, cycles / ITERATIONS};
}

template <uint64_t D>
static BenchResult bench_div_standard(const char *name)
{
    const auto d128{uint128_t{D}};
    uint128_t a{make_bench_value(0xDEADBEEF12345678ULL)};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q{a / d128};
        doNotOptimize(q);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    a = make_bench_value(0xDEADBEEF12345678ULL);
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q{a / d128};
        doNotOptimize(q);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {name, cycles / ITERATIONS};
}

// Handcoded fast_divN from div_by_const.hpp
template <typename DivFn>
static BenchResult bench_div_handcoded(DivFn fn, const char *name)
{
    uint128_t a{make_bench_value(0xDEADBEEF12345678ULL)};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q{fn(a)};
        doNotOptimize(q);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    a = make_bench_value(0xDEADBEEF12345678ULL);
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q{fn(a)};
        doNotOptimize(q);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {name, cycles / ITERATIONS};
}

// ============================================================================
// Multiplication benchmarks
// ============================================================================

template <uint64_t K>
static BenchResult bench_mul_template(const char *name)
{
    uint128_t a{make_bench_value(0xCAFEBABE87654321ULL)};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto r{a.mul<K>()};
        doNotOptimize(r);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    a = make_bench_value(0xCAFEBABE87654321ULL);
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto r{a.mul<K>()};
        doNotOptimize(r);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {name, cycles / ITERATIONS};
}

template <uint64_t K>
static BenchResult bench_mul_standard(const char *name)
{
    const auto k128{uint128_t{K}};
    uint128_t a{make_bench_value(0xCAFEBABE87654321ULL)};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto r{a * k128};
        doNotOptimize(r);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    a = make_bench_value(0xCAFEBABE87654321ULL);
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto r{a * k128};
        doNotOptimize(r);
        a = a + uint128_t{1};
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {name, cycles / ITERATIONS};
}

#ifdef __SIZEOF_INT128__
template <uint64_t D>
static BenchResult bench_div_builtin(const char *name)
{
    unsigned __int128 a{static_cast<unsigned __int128>(0xDEADBEEF12345678ULL) << 64 |
                        0x1234567890ABCDEFULL};
    constexpr unsigned __int128 d{D};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q{a / d};
        doNotOptimize(q);
        ++a;
        doNotOptimize(a);
    }
    a = static_cast<unsigned __int128>(0xDEADBEEF12345678ULL) << 64 | 0x1234567890ABCDEFULL;
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q{a / d};
        doNotOptimize(q);
        ++a;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {name, cycles / ITERATIONS};
}
#endif

// ============================================================================
// Custom output
// ============================================================================

static void print_bench_header(const char *section)
{
    std::cout << "\n[" << section << "]\n";
    std::cout << "+-----------------------------------+--------------+-----------+\n";
    std::cout << "| Method                            |  cyc/op      | speedup   |\n";
    std::cout << "+-----------------------------------+--------------+-----------+\n";
}

static void print_bench_row(const BenchResult &r, double baseline)
{
    const double speedup{(r.cycles_per_op > 0.0) ? baseline / r.cycles_per_op : 0.0};
    std::cout << "| " << std::left << std::setw(33) << r.name << " | "
              << std::right << std::fixed << std::setprecision(2) << std::setw(12) << r.cycles_per_op << " | "
              << std::fixed << std::setprecision(2) << std::setw(6) << speedup << "x   |\n";
}

static void print_bench_footer()
{
    std::cout << "+-----------------------------------+--------------+-----------+\n";
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Benchmark: div<D> (GM) vs operator/ (D_knuth) vs handcoded\n";
    std::cout << "  Iterations: " << ITERATIONS << "  Warmup: " << WARMUP << "\n";
    std::cout << "====================================================================\n";

    // --- Division by 3 ---
    {
        print_bench_header("Division by 3");
        const auto r_std{bench_div_standard<3>("n / uint128_t{3}")};
        const auto r_gm{bench_div_gm<3>("n.div<3>()")};
        const auto r_hc{bench_div_handcoded(fast_div3, "fast_div3(n)")};
#ifdef __SIZEOF_INT128__
        const auto r_builtin{bench_div_builtin<3>("__int128 / 3")};
#endif
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_gm, r_std.cycles_per_op);
        print_bench_row(r_hc, r_std.cycles_per_op);
#ifdef __SIZEOF_INT128__
        print_bench_row(r_builtin, r_std.cycles_per_op);
#endif
        print_bench_footer();
    }

    // --- Division by 5 ---
    {
        print_bench_header("Division by 5");
        const auto r_std{bench_div_standard<5>("n / uint128_t{5}")};
        const auto r_gm{bench_div_gm<5>("n.div<5>()")};
        const auto r_hc{bench_div_handcoded(fast_div5, "fast_div5(n)")};
#ifdef __SIZEOF_INT128__
        const auto r_builtin{bench_div_builtin<5>("__int128 / 5")};
#endif
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_gm, r_std.cycles_per_op);
        print_bench_row(r_hc, r_std.cycles_per_op);
#ifdef __SIZEOF_INT128__
        print_bench_row(r_builtin, r_std.cycles_per_op);
#endif
        print_bench_footer();
    }

    // --- Division by 7 ---
    {
        print_bench_header("Division by 7");
        const auto r_std{bench_div_standard<7>("n / uint128_t{7}")};
        const auto r_gm{bench_div_gm<7>("n.div<7>()")};
        const auto r_hc{bench_div_handcoded(fast_div7, "fast_div7(n)")};
#ifdef __SIZEOF_INT128__
        const auto r_builtin{bench_div_builtin<7>("__int128 / 7")};
#endif
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_gm, r_std.cycles_per_op);
        print_bench_row(r_hc, r_std.cycles_per_op);
#ifdef __SIZEOF_INT128__
        print_bench_row(r_builtin, r_std.cycles_per_op);
#endif
        print_bench_footer();
    }

    // --- Division by 10 ---
    {
        print_bench_header("Division by 10");
        const auto r_std{bench_div_standard<10>("n / uint128_t{10}")};
        const auto r_gm{bench_div_gm<10>("n.div<10>()")};
        const auto r_hc{bench_div_handcoded(fast_div10, "fast_div10(n)")};
#ifdef __SIZEOF_INT128__
        const auto r_builtin{bench_div_builtin<10>("__int128 / 10")};
#endif
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_gm, r_std.cycles_per_op);
        print_bench_row(r_hc, r_std.cycles_per_op);
#ifdef __SIZEOF_INT128__
        print_bench_row(r_builtin, r_std.cycles_per_op);
#endif
        print_bench_footer();
    }

    // --- Division by 100 ---
    {
        print_bench_header("Division by 100");
        const auto r_std{bench_div_standard<100>("n / uint128_t{100}")};
        const auto r_gm{bench_div_gm<100>("n.div<100>()")};
        const auto r_hc{bench_div_handcoded(fast_div100, "fast_div100(n)")};
#ifdef __SIZEOF_INT128__
        const auto r_builtin{bench_div_builtin<100>("__int128 / 100")};
#endif
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_gm, r_std.cycles_per_op);
        print_bench_row(r_hc, r_std.cycles_per_op);
#ifdef __SIZEOF_INT128__
        print_bench_row(r_builtin, r_std.cycles_per_op);
#endif
        print_bench_footer();
    }

    // --- Division by 10^19 ---
    {
        constexpr uint64_t E19{10000000000000000000ULL};
        print_bench_header("Division by 10^19");
        const auto r_std{bench_div_standard<E19>("n / uint128_t{1e19}")};
        const auto r_gm{bench_div_gm<E19>("n.div<1e19>()")};
        const auto r_hc{bench_div_handcoded(fast_div_1e19, "fast_div_1e19(n)")};
#ifdef __SIZEOF_INT128__
        const auto r_builtin{bench_div_builtin<E19>("__int128 / 1e19")};
#endif
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_gm, r_std.cycles_per_op);
        print_bench_row(r_hc, r_std.cycles_per_op);
#ifdef __SIZEOF_INT128__
        print_bench_row(r_builtin, r_std.cycles_per_op);
#endif
        print_bench_footer();
    }

    // --- Division by 1000000007 (prime > 1023, on-the-fly) ---
    {
        print_bench_header("Division by 1000000007 (computed on-the-fly)");
        const auto r_std{bench_div_standard<1000000007>("n / uint128_t{1e9+7}")};
        const auto r_gm{bench_div_gm<1000000007>("n.div<1e9+7>()")};
#ifdef __SIZEOF_INT128__
        const auto r_builtin{bench_div_builtin<1000000007>("__int128 / 1e9+7")};
#endif
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_gm, r_std.cycles_per_op);
#ifdef __SIZEOF_INT128__
        print_bench_row(r_builtin, r_std.cycles_per_op);
#endif
        print_bench_footer();
    }

    // ================================================================
    // Multiplication benchmarks
    // ================================================================

    // --- mul<10> ---
    {
        print_bench_header("Multiplication by 10");
        const auto r_std{bench_mul_standard<10>("n * uint128_t{10}")};
        const auto r_tmpl{bench_mul_template<10>("n.mul<10>()")};
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_tmpl, r_std.cycles_per_op);
        print_bench_footer();
    }

    // --- mul<7> ---
    {
        print_bench_header("Multiplication by 7");
        const auto r_std{bench_mul_standard<7>("n * uint128_t{7}")};
        const auto r_tmpl{bench_mul_template<7>("n.mul<7>()")};
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_tmpl, r_std.cycles_per_op);
        print_bench_footer();
    }

    // --- mul<100> ---
    {
        print_bench_header("Multiplication by 100");
        const auto r_std{bench_mul_standard<100>("n * uint128_t{100}")};
        const auto r_tmpl{bench_mul_template<100>("n.mul<100>()")};
        print_bench_row(r_std, r_std.cycles_per_op);
        print_bench_row(r_tmpl, r_std.cycles_per_op);
        print_bench_footer();
    }

    std::cout << "\n[DONE]\n";
    return 0;
}
