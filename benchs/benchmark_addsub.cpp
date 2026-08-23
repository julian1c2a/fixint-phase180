// =============================================================================
// Benchmark: Addition/Subtraction optimization (nstd vs __int128)
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Focused benchmark comparing nstd::uint128_t vs unsigned __int128
// for addition and subtraction operations, measuring in CPU cycles (RDTSC).
//
// Purpose: Validate A1 optimization (sub128/add128 using __uint128_t codegen)
//
// =============================================================================

#include "int128_parameterized.hpp"
#include "bench_common.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>

using namespace nstd;

// ============================================================================
// Configuration
// ============================================================================
static constexpr int NUM_RUNS{5};

// ============================================================================
// Deterministic PRNG (SplitMix64)
// ============================================================================
struct SplitMix64
{
    std::uint64_t state;
    explicit constexpr SplitMix64(std::uint64_t seed) noexcept : state{seed} {}
    constexpr std::uint64_t next() noexcept
    {
        state += 0x9e3779b97f4a7c15ULL;
        std::uint64_t z{state};
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};

// ============================================================================
// Benchmark helpers
// ============================================================================

static double bench_sub_nstd()
{
    SplitMix64 rng{0xDEADBEEF12345678ULL};
    uint128_t a{rng.next(), rng.next()};
    const uint128_t b{rng.next(), rng.next()};

    // Warmup
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }

    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const std::uint64_t cycles{timer.elapsed_cycles()};
    doNotOptimize(a);
    return static_cast<double>(cycles) / ITERATIONS;
}

static double bench_add_nstd()
{
    SplitMix64 rng{0xCAFEBABE87654321ULL};
    uint128_t a{rng.next(), rng.next()};
    const uint128_t b{rng.next(), rng.next()};

    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }

    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const std::uint64_t cycles{timer.elapsed_cycles()};
    doNotOptimize(a);
    return static_cast<double>(cycles) / ITERATIONS;
}

#ifdef __SIZEOF_INT128__
static double bench_sub_builtin()
{
    SplitMix64 rng{0xDEADBEEF12345678ULL};
    unsigned __int128 a = static_cast<unsigned __int128>(rng.next()) << 64 | rng.next();
    const unsigned __int128 b = static_cast<unsigned __int128>(rng.next()) << 64 | rng.next();

    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }

    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const std::uint64_t cycles{timer.elapsed_cycles()};
    doNotOptimize(a);
    return static_cast<double>(cycles) / ITERATIONS;
}

static double bench_add_builtin()
{
    SplitMix64 rng{0xCAFEBABE87654321ULL};
    unsigned __int128 a = static_cast<unsigned __int128>(rng.next()) << 64 | rng.next();
    const unsigned __int128 b = static_cast<unsigned __int128>(rng.next()) << 64 | rng.next();

    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }

    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const std::uint64_t cycles{timer.elapsed_cycles()};
    doNotOptimize(a);
    return static_cast<double>(cycles) / ITERATIONS;
}
#endif

static double bench_sub_u64()
{
    SplitMix64 rng{0xDEADBEEF12345678ULL};
    std::uint64_t a{rng.next()};
    const std::uint64_t b{rng.next()};

    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }

    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const std::uint64_t cycles{timer.elapsed_cycles()};
    doNotOptimize(a);
    return static_cast<double>(cycles) / ITERATIONS;
}

static double bench_add_u64()
{
    SplitMix64 rng{0xCAFEBABE87654321ULL};
    std::uint64_t a{rng.next()};
    const std::uint64_t b{rng.next()};

    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }

    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const std::uint64_t cycles{timer.elapsed_cycles()};
    doNotOptimize(a);
    return static_cast<double>(cycles) / ITERATIONS;
}

// ============================================================================
// Main
// ============================================================================
int main()
{
    std::printf("================================================================\n");
    std::printf("  Benchmark: ADD/SUB optimization (nstd vs __int128 vs uint64)\n");
    std::printf("  Iterations: %zu  |  Warmup: %zu  |  Runs: %d\n", ITERATIONS, WARMUP, NUM_RUNS);
    std::printf("================================================================\n\n");

    // Collect multiple runs and report median
    std::array<double, NUM_RUNS> sub_nstd{};
    std::array<double, NUM_RUNS> add_nstd{};
    std::array<double, NUM_RUNS> sub_u64{};
    std::array<double, NUM_RUNS> add_u64{};
#ifdef __SIZEOF_INT128__
    std::array<double, NUM_RUNS> sub_builtin{};
    std::array<double, NUM_RUNS> add_builtin{};
#endif

    for (int r{0}; r < NUM_RUNS; ++r)
    {
        sub_u64[r] = bench_sub_u64();
        add_u64[r] = bench_add_u64();
        sub_nstd[r] = bench_sub_nstd();
        add_nstd[r] = bench_add_nstd();
#ifdef __SIZEOF_INT128__
        sub_builtin[r] = bench_sub_builtin();
        add_builtin[r] = bench_add_builtin();
#endif
    }

    // Sort for median
    std::sort(sub_nstd.begin(), sub_nstd.end());
    std::sort(add_nstd.begin(), add_nstd.end());
    std::sort(sub_u64.begin(), sub_u64.end());
    std::sort(add_u64.begin(), add_u64.end());
#ifdef __SIZEOF_INT128__
    std::sort(sub_builtin.begin(), sub_builtin.end());
    std::sort(add_builtin.begin(), add_builtin.end());
#endif

    const int mid{NUM_RUNS / 2};
    const double sub_u64_med{sub_u64[mid]};
    const double add_u64_med{add_u64[mid]};
    const double sub_nstd_med{sub_nstd[mid]};
    const double add_nstd_med{add_nstd[mid]};

    std::printf("%-25s %12s %12s\n", "Type", "SUB cyc/op", "ADD cyc/op");
    std::printf("------------------------------------------------------\n");
    std::printf("%-25s %12.2f %12.2f\n", "uint64_t (baseline)", sub_u64_med, add_u64_med);
    std::printf("%-25s %12.2f %12.2f\n", "nstd::uint128_t", sub_nstd_med, add_nstd_med);

#ifdef __SIZEOF_INT128__
    const double sub_bi_med{sub_builtin[mid]};
    const double add_bi_med{add_builtin[mid]};
    std::printf("%-25s %12.2f %12.2f\n", "unsigned __int128", sub_bi_med, add_bi_med);

    std::printf("\n--- Ratios (nstd / __int128) ---\n");
    std::printf("  SUB: %.3fx\n", sub_nstd_med / sub_bi_med);
    std::printf("  ADD: %.3fx\n", add_nstd_med / add_bi_med);

    const bool sub_ok{sub_nstd_med / sub_bi_med < 1.10};
    const bool add_ok{add_nstd_med / add_bi_med < 1.10};
    std::printf("\n--- Verdict ---\n");
    std::printf("  SUB: %s (target: < 1.10x)\n", sub_ok ? "[OK]" : "[SLOW]");
    std::printf("  ADD: %s (target: < 1.10x)\n", add_ok ? "[OK]" : "[SLOW]");
#else
    std::printf("\n[INFO] __int128 not available on this compiler\n");
    std::printf("  SUB ratio vs u64: %.3fx\n", sub_nstd_med / sub_u64_med);
    std::printf("  ADD ratio vs u64: %.3fx\n", add_nstd_med / add_u64_med);
#endif

    std::printf("\n================================================================\n");
    return 0;
}
