// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Benchmark: fixed_int_t<2> vs int128_param_t — performance comparison
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Compares uint_fixed_t<2>/int_fixed_t<2> (new unified template) against
// uint128_t/int128_tc_t (old parameterized type) on all core operations.
// Both types have identical memory layout (data[0]=LSB, data[1]=MSB, 16 bytes).
//
// Benchmarks (cycles/op, median of 5 runs):
//   Unsigned 128-bit:
//     1. Addition
//     2. Subtraction
//     3. Multiplication
//     4. Division (variable divisor — runtime, no GM precomputation)
//     5. Modulo  (variable divisor)
//     6. to_string (decimal, smaller iteration count)
//
//   Signed 128-bit (two's complement):
//     7. Addition
//     8. Subtraction
//     9. Multiplication
//     10. Division (variable divisor)
//
//   Baseline:
//     uint64_t for operations 1-5

#include "fixed_width_int_t.hpp"
#include "int128_parameterized.hpp"
#include "bench_common.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

using nstd::int128_tc_t;
using nstd::int_fixed_t;
using nstd::uint128_t;
using nstd::uint_fixed_t;
using std::uint64_t;

using u128n = uint_fixed_t<2>;
using i128n = int_fixed_t<2>;
using u128o = uint128_t;
using i128o = int128_tc_t;

// =============================================================================
// Configuration
// =============================================================================

static constexpr int NUM_RUNS{5};
static constexpr size_t STR_ITERS{50'000}; // fewer for string ops (slow)

// =============================================================================
// Deterministic PRNG (SplitMix64)
// =============================================================================

struct SplitMix64
{
    uint64_t state;
    explicit constexpr SplitMix64(uint64_t seed) noexcept : state{seed} {}
    constexpr uint64_t next() noexcept
    {
        state += 0x9e3779b97f4a7c15ULL;
        uint64_t z{state};
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};

// Build a non-zero 128-bit value from two uint64 halves
static uint64_t nonzero(uint64_t x) { return x == 0 ? 1 : x; }

// =============================================================================
// Unsigned 128-bit benchmarks
// =============================================================================

// --- new: uint_fixed_t<2> ---

static double bench_new_u_add()
{
    SplitMix64 rng{0xDEAD'BEEF'0000'0001ULL};
    u128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const u128n b{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_u_sub()
{
    SplitMix64 rng{0xCAFE'BABE'0000'0002ULL};
    u128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const u128n b{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_u_mul()
{
    SplitMix64 rng{0x1234'5678'0000'0003ULL};
    u128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const u128n b{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_u_div()
{
    SplitMix64 rng{0xFEDC'BA98'0000'0004ULL};
    u128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const u128n b{std::array<uint64_t, 2>{nonzero(rng.next() >> 32), 0}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a /= b;
        a.set_limb(1, a.limb(1) | (uint64_t{1} << 32));
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a /= b;
        a.set_limb(1, a.limb(1) | (uint64_t{1} << 32));
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_u_mod()
{
    SplitMix64 rng{0xAAAA'BBBB'0000'0005ULL};
    u128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const u128n b{std::array<uint64_t, 2>{nonzero(rng.next() >> 32), 0}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        auto r = a % b;
        doNotOptimize(r);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        auto r = a % b;
        doNotOptimize(r);
        a += b;
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_u_tostring()
{
    SplitMix64 rng{0x1111'2222'0000'0006ULL};
    u128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    std::string s;
    for (size_t i{0}; i < 1000; ++i)
    {
        s = a.to_string();
        doNotOptimize(s);
        a += u128n{uint64_t{1}};
    }
    CycleTimer t;
    for (size_t i{0}; i < STR_ITERS; ++i)
    {
        s = a.to_string();
        doNotOptimize(s);
        a += u128n{uint64_t{1}};
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / STR_ITERS;
}

// --- old: uint128_t ---

static double bench_old_u_add()
{
    SplitMix64 rng{0xDEAD'BEEF'0000'0001ULL};
    u128o a{rng.next(), rng.next()};
    const u128o b{rng.next(), rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_u_sub()
{
    SplitMix64 rng{0xCAFE'BABE'0000'0002ULL};
    u128o a{rng.next(), rng.next()};
    const u128o b{rng.next(), rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_u_mul()
{
    SplitMix64 rng{0x1234'5678'0000'0003ULL};
    u128o a{rng.next(), rng.next()};
    const u128o b{rng.next(), rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_u_div()
{
    SplitMix64 rng{0xFEDC'BA98'0000'0004ULL};
    u128o a{rng.next(), rng.next()};
    const u128o b{nonzero(rng.next() >> 32), 0};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a /= b;
        a.set_high(a.high() | (uint64_t{1} << 32));
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a /= b;
        a.set_high(a.high() | (uint64_t{1} << 32));
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_u_mod()
{
    SplitMix64 rng{0xAAAA'BBBB'0000'0005ULL};
    u128o a{rng.next(), rng.next()};
    const u128o b{nonzero(rng.next() >> 32), 0};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        auto r = a % b;
        doNotOptimize(r);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        auto r = a % b;
        doNotOptimize(r);
        a += b;
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_u_tostring()
{
    SplitMix64 rng{0x1111'2222'0000'0006ULL};
    u128o a{rng.next(), rng.next()};
    std::string s;
    for (size_t i{0}; i < 1000; ++i)
    {
        s = a.to_string();
        doNotOptimize(s);
        a += u128o{0, 1};
    }
    CycleTimer t;
    for (size_t i{0}; i < STR_ITERS; ++i)
    {
        s = a.to_string();
        doNotOptimize(s);
        a += u128o{0, 1};
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / STR_ITERS;
}

// --- baseline: uint64_t ---

static double bench_u64_add()
{
    SplitMix64 rng{0xDEAD'BEEF'0000'0001ULL};
    uint64_t a{rng.next()};
    const uint64_t b{rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_u64_sub()
{
    SplitMix64 rng{0xCAFE'BABE'0000'0002ULL};
    uint64_t a{rng.next()};
    const uint64_t b{rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_u64_mul()
{
    SplitMix64 rng{0x1234'5678'0000'0003ULL};
    uint64_t a{rng.next()};
    const uint64_t b{rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_u64_div()
{
    SplitMix64 rng{0xFEDC'BA98'0000'0004ULL};
    uint64_t a{rng.next()};
    const uint64_t b{nonzero(rng.next() >> 32)}; // small divisor keeps a large
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a /= b;
        a |= uint64_t{1} << 32;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a /= b;
        a |= uint64_t{1} << 32;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

// =============================================================================
// Signed 128-bit benchmarks
// =============================================================================

// --- new: int_fixed_t<2> ---

static double bench_new_i_add()
{
    SplitMix64 rng{0xBEEF'CAFE'1111'0001ULL};
    i128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const i128n b{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_i_sub()
{
    SplitMix64 rng{0xDEAD'1234'1111'0002ULL};
    i128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const i128n b{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_i_mul()
{
    SplitMix64 rng{0xABCD'EF01'1111'0003ULL};
    i128n a{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    const i128n b{std::array<uint64_t, 2>{rng.next(), rng.next()}};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_new_i_div()
{
    SplitMix64 rng{0x5678'9012'1111'0004ULL};
    i128n a{std::array<uint64_t, 2>{rng.next() >> 1, rng.next() >> 1}}; // positive
    const i128n b{int64_t(nonzero(static_cast<int64_t>(rng.next() >> 32)))};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a /= b;
        a.set_limb(1, a.limb(1) | (uint64_t{1} << 30));
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a /= b;
        a.set_limb(1, a.limb(1) | (uint64_t{1} << 30));
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

// --- old: int128_tc_t ---

static double bench_old_i_add()
{
    SplitMix64 rng{0xBEEF'CAFE'1111'0001ULL};
    i128o a{rng.next(), rng.next()};
    const i128o b{rng.next(), rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_i_sub()
{
    SplitMix64 rng{0xDEAD'1234'1111'0002ULL};
    i128o a{rng.next(), rng.next()};
    const i128o b{rng.next(), rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_i_mul()
{
    SplitMix64 rng{0xABCD'EF01'1111'0003ULL};
    i128o a{rng.next(), rng.next()};
    const i128o b{rng.next(), rng.next()};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a *= b;
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

static double bench_old_i_div()
{
    SplitMix64 rng{0x5678'9012'1111'0004ULL};
    i128o a{rng.next() >> 1, rng.next() >> 1}; // positive values
    const i128o b{0, nonzero(rng.next() >> 32)};
    for (size_t i{0}; i < WARMUP; ++i)
    {
        a /= b;
        a.set_high(a.high() | (uint64_t{1} << 30));
        doNotOptimize(a);
    }
    CycleTimer t;
    for (size_t i{0}; i < ITERATIONS; ++i)
    {
        a /= b;
        a.set_high(a.high() | (uint64_t{1} << 30));
        doNotOptimize(a);
    }
    doNotOptimize(a);
    return static_cast<double>(t.elapsed_cycles()) / ITERATIONS;
}

// =============================================================================
// Reporting
// =============================================================================

static double median5(std::array<double, 5> &v)
{
    std::sort(v.begin(), v.end());
    return v[2];
}

static void print_row(const char *label, double cyc, double baseline)
{
    const double ratio = (baseline > 0.0) ? cyc / baseline : 0.0;
    std::printf("| %-35s | %10.2f | %8.2fx |\n", label, cyc, ratio);
}

static void print_sep() { std::printf("+-------------------------------------+------------+----------+\n"); }

static void print_hdr(const char *op)
{
    std::printf("\n[%s]\n", op);
    print_sep();
    std::printf("| %-35s | %10s | %8s |\n", "Type", "cyc/op", "vs u64");
    print_sep();
}

static void print_hdr_str(const char *op)
{
    std::printf("\n[%s]\n", op);
    print_sep();
    std::printf("| %-35s | %10s | %8s |\n", "Type", "cyc/op", "ratio");
    print_sep();
}

// =============================================================================
// Main
// =============================================================================

int main()
{
    std::printf("=================================================================\n");
    std::printf("  Benchmark: fixed_int_t<2> vs int128_param_t\n");
    std::printf("  Iterations: %zu | String: %zu | Runs: %d | Warmup: %zu\n", ITERATIONS, STR_ITERS, NUM_RUNS,
                WARMUP);
    std::printf("=================================================================\n");

    // -------------------------------------------------------------------------
    // Unsigned: Addition
    // -------------------------------------------------------------------------
    std::array<double, NUM_RUNS> r{};
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_add();
        const double u64_add = median5(r);
        print_hdr("UNSIGNED ADD");
        print_row("uint64_t (baseline)", u64_add, u64_add);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_u_add();
        print_row("uint128_t (old, int128_param_t)", median5(r), u64_add);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_u_add();
        print_row("uint_fixed_t<2> (new)", median5(r), u64_add);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Unsigned: Subtraction
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_sub();
        const double u64_sub = median5(r);
        print_hdr("UNSIGNED SUB");
        print_row("uint64_t (baseline)", u64_sub, u64_sub);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_u_sub();
        print_row("uint128_t (old)", median5(r), u64_sub);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_u_sub();
        print_row("uint_fixed_t<2> (new)", median5(r), u64_sub);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Unsigned: Multiplication
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_mul();
        const double u64_mul = median5(r);
        print_hdr("UNSIGNED MUL");
        print_row("uint64_t (baseline)", u64_mul, u64_mul);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_u_mul();
        print_row("uint128_t (old)", median5(r), u64_mul);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_u_mul();
        print_row("uint_fixed_t<2> (new)", median5(r), u64_mul);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Unsigned: Division (variable divisor)
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_div();
        const double u64_div = median5(r);
        print_hdr("UNSIGNED DIV (variable)");
        print_row("uint64_t (baseline)", u64_div, u64_div);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_u_div();
        print_row("uint128_t (old, Knuth D)", median5(r), u64_div);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_u_div();
        print_row("uint_fixed_t<2> (new, long div)", median5(r), u64_div);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Unsigned: Modulo (variable divisor)
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_div();
        const double u64_div = median5(r);
        print_hdr("UNSIGNED MOD (variable)");
        print_row("uint64_t / (baseline)", u64_div, u64_div);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_u_mod();
        print_row("uint128_t (old)", median5(r), u64_div);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_u_mod();
        print_row("uint_fixed_t<2> (new)", median5(r), u64_div);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Unsigned: to_string
    // -------------------------------------------------------------------------
    {
        std::printf("\n[UNSIGNED TO_STRING (decimal, %zu iters)]\n", STR_ITERS);
        print_sep();
        std::printf("| %-35s | %10s | %8s |\n", "Type", "cyc/op", "ratio");
        print_sep();

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_u_tostring();
        const double old_str = median5(r);
        print_row("uint128_t (old, GM division)", old_str, old_str);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_u_tostring();
        print_row("uint_fixed_t<2> (new, naive div)", median5(r), old_str);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Signed: Addition
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_add();
        const double u64_add = median5(r);
        print_hdr("SIGNED ADD");
        print_row("uint64_t (baseline, unsigned)", u64_add, u64_add);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_i_add();
        print_row("int128_tc_t (old)", median5(r), u64_add);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_i_add();
        print_row("int_fixed_t<2> (new)", median5(r), u64_add);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Signed: Subtraction
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_sub();
        const double u64_sub = median5(r);
        print_hdr("SIGNED SUB");
        print_row("uint64_t (baseline, unsigned)", u64_sub, u64_sub);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_i_sub();
        print_row("int128_tc_t (old)", median5(r), u64_sub);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_i_sub();
        print_row("int_fixed_t<2> (new)", median5(r), u64_sub);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Signed: Multiplication
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_mul();
        const double u64_mul = median5(r);
        print_hdr("SIGNED MUL");
        print_row("uint64_t (baseline, unsigned)", u64_mul, u64_mul);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_i_mul();
        print_row("int128_tc_t (old)", median5(r), u64_mul);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_i_mul();
        print_row("int_fixed_t<2> (new)", median5(r), u64_mul);
        print_sep();
    }

    // -------------------------------------------------------------------------
    // Signed: Division (variable)
    // -------------------------------------------------------------------------
    {
        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_u64_div();
        const double u64_div = median5(r);
        print_hdr("SIGNED DIV (variable)");
        print_row("uint64_t / (baseline)", u64_div, u64_div);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_old_i_div();
        print_row("int128_tc_t (old, Knuth D)", median5(r), u64_div);

        for (int i{0}; i < NUM_RUNS; ++i)
            r[i] = bench_new_i_div();
        print_row("int_fixed_t<2> (new, long div)", median5(r), u64_div);
        print_sep();
    }

    std::printf("\n=================================================================\n");
    std::printf("  Done.\n");
    std::printf("=================================================================\n");

    return 0;
}
