// =============================================================================
// Benchmark: to_string() end-to-end for uint128_t / int128_t
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Measures full to_string() conversion cost (base 10) with RDTSC.
// Groups:
//   1. uint64_t baseline (std::to_string)
//   2. nstd::uint128_t — small (fits in 64 bits)
//   3. nstd::uint128_t — medium (2-chunk, 20-38 digits)
//   4. nstd::uint128_t — large (3-chunk, 39 digits, near MAX)
//   5. nstd::int128_t  — negative (sign + conversion)
//   6. __int128 baseline (GCC/Clang only, manual itoa)
//
// Compile:
//   python make.py build uint128 tostring_fast benchs gcc release
//
// =============================================================================

#include "int128_parameterized.hpp"
#include "bench_common.hpp"
#include <string>

using namespace nstd;

// ============================================================================
// Helper: convert __int128 to string (baseline reference)
// ============================================================================
#ifdef __SIZEOF_INT128__
static std::string builtin_u128_to_string(unsigned __int128 v)
{
    if (v == 0)
    {
        return "0";
    }
    char buf[40];
    int pos{39};
    buf[pos] = '\0';
    while (v != 0)
    {
        buf[--pos] = static_cast<char>('0' + static_cast<int>(v % 10));
        v /= 10;
    }
    return std::string{&buf[pos]};
}
#endif

// ============================================================================
// BENCHMARKS
// ============================================================================

// --- uint64_t baseline (std::to_string) ---
static BenchResult bench_tostring_u64()
{
    std::uint64_t val{18446744073709551615ull}; // UINT64_MAX
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto s{std::to_string(val)};
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto s{std::to_string(val)};
        doNotOptimize(s);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t (std::to_string)", cycles / ITERATIONS};
}

// --- uint128_t small (fits in 64 bits) ---
static BenchResult bench_tostring_u128_small()
{
    const uint128_t val{18446744073709551615ull}; // same as UINT64_MAX
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint128_t (small, <2^64)", cycles / ITERATIONS};
}

// --- uint128_t medium (2-chunk: 20-38 digits) ---
static BenchResult bench_tostring_u128_medium()
{
    // ~1.84 * 10^19 — requires 2 chunks
    const uint128_t val{1ull, 0ull}; // 2^64 = 18446744073709551616
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint128_t (medium, 2^64)", cycles / ITERATIONS};
}

// --- uint128_t large (3-chunk: 39 digits, MAX) ---
static BenchResult bench_tostring_u128_large()
{
    const uint128_t val{uint128_t::max()}; // 340282366920938463463374607431768211455
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint128_t (large, MAX)", cycles / ITERATIONS};
}

// --- int128_t negative ---
static BenchResult bench_tostring_i128_neg()
{
    const int128_t val{int128_t::min()}; // -170141183460469231731687303715884105728
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto s{val.to_string()};
        doNotOptimize(s);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"int128_t (negative, MIN)", cycles / ITERATIONS};
}

// --- __int128 baseline (NOT using our library) ---
#ifdef __SIZEOF_INT128__
static BenchResult bench_tostring_builtin128()
{
    unsigned __int128 val{static_cast<unsigned __int128>(-1)}; // MAX
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto s{builtin_u128_to_string(val)};
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto s{builtin_u128_to_string(val)};
        doNotOptimize(s);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"__uint128_t (naive /10)", cycles / ITERATIONS};
}
#endif

// ============================================================================
// MAIN
// ============================================================================

int main()
{
    std::cout << "========================================\n"
              << "  BENCHMARK: to_string() end-to-end\n"
              << "  Iterations: " << ITERATIONS << "\n"
              << "  Warmup:     " << WARMUP << "\n"
              << "========================================\n";

    print_header("to_string (base 10)");

    const auto r_u64{bench_tostring_u64()};
    const auto r_small{bench_tostring_u128_small()};
    const auto r_medium{bench_tostring_u128_medium()};
    const auto r_large{bench_tostring_u128_large()};
    const auto r_neg{bench_tostring_i128_neg()};

    print_result(r_u64, r_u64.cycles_per_op);
    print_result(r_small, r_u64.cycles_per_op);
    print_result(r_medium, r_u64.cycles_per_op);
    print_result(r_large, r_u64.cycles_per_op);
    print_result(r_neg, r_u64.cycles_per_op);

#ifdef __SIZEOF_INT128__
    const auto r_builtin{bench_tostring_builtin128()};
    print_result(r_builtin, r_u64.cycles_per_op);
#endif

    print_footer();

    return 0;
}
