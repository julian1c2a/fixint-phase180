// =============================================================================
// Benchmark: from_string() / to_string() round-trip performance
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Measures:
//   - from_string() for decimal, hex, binary, octal (various value sizes)
//   - to_string() for decimal, hex, binary, octal (various value sizes)
//   - Round-trip: to_string → from_string
//
// Compile:
//   g++ -std=c++20 -O2 -Iinclude benchs/benchmark_fromstring.cpp -o bench_fromstring
//
// =============================================================================

#include "int128_parameterized.hpp"
#include "bench_common.hpp"
#include <array>
#include <cstring>

using uint128_t = nstd::uint128_t;
using int128_t = nstd::int128_t;

// ============================================================================
// Test values at different magnitudes
// ============================================================================

// Small: fits in uint64_t low limb
static const uint128_t VAL_SMALL{42};

// Medium: ~64 bits
static const uint128_t VAL_MEDIUM{uint64_t{0}, uint64_t{0xDEADBEEF12345678ull}};

// Large: full 128 bits (MAX)
static const uint128_t VAL_LARGE{uint64_t{0xFFFFFFFFFFFFFFFFull}, uint64_t{0xFFFFFFFFFFFFFFFFull}};

// Signed negative (full range)
static const int128_t VAL_NEG{int64_t{-1}, uint64_t{0xDEADBEEF12345678ull}};

// ============================================================================
// Pre-computed string representations
// ============================================================================

static const std::string STR_SMALL_DEC{VAL_SMALL.to_string(10)};
static const std::string STR_MEDIUM_DEC{VAL_MEDIUM.to_string(10)};
static const std::string STR_LARGE_DEC{VAL_LARGE.to_string(10)};
static const std::string STR_NEG_DEC{VAL_NEG.to_string(10)};

// from_string auto-detects base from prefix: 0x=hex, 0b=bin, 0=oct
static const std::string STR_SMALL_HEX{"0x" + VAL_SMALL.to_string(16)};
static const std::string STR_MEDIUM_HEX{"0x" + VAL_MEDIUM.to_string(16)};
static const std::string STR_LARGE_HEX{"0x" + VAL_LARGE.to_string(16)};

static const std::string STR_SMALL_BIN{"0b" + VAL_SMALL.to_string(2)};
static const std::string STR_MEDIUM_BIN{"0b" + VAL_MEDIUM.to_string(2)};
static const std::string STR_LARGE_BIN{"0b" + VAL_LARGE.to_string(2)};

static const std::string STR_SMALL_OCT{"0" + VAL_SMALL.to_string(8)};
static const std::string STR_MEDIUM_OCT{"0" + VAL_MEDIUM.to_string(8)};
static const std::string STR_LARGE_OCT{"0" + VAL_LARGE.to_string(8)};

// ============================================================================
// BENCHMARK: from_string() — decimal
// ============================================================================

static BenchResult bench_fromstr_dec_small()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_SMALL_DEC.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_SMALL_DEC.c_str());
        doNotOptimize(r);
    }
    return {"dec small (42)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_fromstr_dec_medium()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_MEDIUM_DEC.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_MEDIUM_DEC.c_str());
        doNotOptimize(r);
    }
    return {"dec medium (64-bit)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_fromstr_dec_large()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_DEC.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_DEC.c_str());
        doNotOptimize(r);
    }
    return {"dec large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_fromstr_dec_neg()
{
    int128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = int128_t::from_string(STR_NEG_DEC.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = int128_t::from_string(STR_NEG_DEC.c_str());
        doNotOptimize(r);
    }
    return {"dec negative (i128)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: from_string() — hex (pow2 shift path)
// ============================================================================

static BenchResult bench_fromstr_hex_small()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_SMALL_HEX.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_SMALL_HEX.c_str());
        doNotOptimize(r);
    }
    return {"hex small (42)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_fromstr_hex_large()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_HEX.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_HEX.c_str());
        doNotOptimize(r);
    }
    return {"hex large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: from_string() — binary (pow2 shift path)
// ============================================================================

static BenchResult bench_fromstr_bin_small()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_SMALL_BIN.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_SMALL_BIN.c_str());
        doNotOptimize(r);
    }
    return {"bin small (42)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_fromstr_bin_large()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_BIN.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_BIN.c_str());
        doNotOptimize(r);
    }
    return {"bin large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: from_string() — octal (pow2 shift path)
// ============================================================================

static BenchResult bench_fromstr_oct_large()
{
    uint128_t r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_OCT.c_str());
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = uint128_t::from_string(STR_LARGE_OCT.c_str());
        doNotOptimize(r);
    }
    return {"oct large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: to_string() — decimal
// ============================================================================

static BenchResult bench_tostr_dec_small()
{
    std::string s;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        s = VAL_SMALL.to_string(10);
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        s = VAL_SMALL.to_string(10);
        doNotOptimize(s);
    }
    return {"dec small (42)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_tostr_dec_large()
{
    std::string s;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        s = VAL_LARGE.to_string(10);
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        s = VAL_LARGE.to_string(10);
        doNotOptimize(s);
    }
    return {"dec large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: to_string() — hex (pow2 shift path)
// ============================================================================

static BenchResult bench_tostr_hex_small()
{
    std::string s;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        s = VAL_SMALL.to_string(16);
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        s = VAL_SMALL.to_string(16);
        doNotOptimize(s);
    }
    return {"hex small (42)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_tostr_hex_large()
{
    std::string s;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        s = VAL_LARGE.to_string(16);
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        s = VAL_LARGE.to_string(16);
        doNotOptimize(s);
    }
    return {"hex large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: to_string() — binary (pow2 shift path)
// ============================================================================

static BenchResult bench_tostr_bin_large()
{
    std::string s;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        s = VAL_LARGE.to_string(2);
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        s = VAL_LARGE.to_string(2);
        doNotOptimize(s);
    }
    return {"bin large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: to_cstr() — decimal (no allocation)
// ============================================================================

static BenchResult bench_tocstr_dec_small()
{
    const char *p{nullptr};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        p = VAL_SMALL.to_cstr(10);
        doNotOptimize(p);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        p = VAL_SMALL.to_cstr(10);
        doNotOptimize(p);
    }
    return {"cstr dec small (42)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

static BenchResult bench_tocstr_dec_large()
{
    const char *p{nullptr};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        p = VAL_LARGE.to_cstr(10);
        doNotOptimize(p);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        p = VAL_LARGE.to_cstr(10);
        doNotOptimize(p);
    }
    return {"cstr dec large (MAX)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// BENCHMARK: Baseline — std::to_string(uint64_t)
// ============================================================================

static BenchResult bench_stdlib_tostr_u64()
{
    std::uint64_t v{0xDEADBEEF12345678ull};
    std::string s;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        s = std::to_string(v);
        doNotOptimize(s);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        s = std::to_string(v);
        doNotOptimize(s);
    }
    return {"std::to_string(u64)", static_cast<double>(t.elapsed_cycles()) / ITERATIONS};
}

// ============================================================================
// MAIN
// ============================================================================

int main()
{
    std::cout << "============================================================\n"
              << "  Benchmark: from_string / to_string / to_cstr Performance\n"
              << "  int128 Library - Phase 1.75\n"
              << "============================================================\n";

    // Print test values
    std::cout << "\nTest values:\n"
              << "  SMALL  : " << STR_SMALL_DEC << "\n"
              << "  MEDIUM : " << STR_MEDIUM_DEC << "\n"
              << "  LARGE  : " << STR_LARGE_DEC << "\n"
              << "  NEG    : " << STR_NEG_DEC << "\n"
              << "  Iterations: " << ITERATIONS << "  Warmup: " << WARMUP << "\n";

    // ---- from_string() decimal ----
    {
        print_header("from_string() decimal");
        const auto r_small{bench_fromstr_dec_small()};
        const auto r_medium{bench_fromstr_dec_medium()};
        const auto r_large{bench_fromstr_dec_large()};
        const auto r_neg{bench_fromstr_dec_neg()};
        print_result(r_small, r_small.cycles_per_op);
        print_result(r_medium, r_small.cycles_per_op);
        print_result(r_large, r_small.cycles_per_op);
        print_result(r_neg, r_small.cycles_per_op);
        print_footer();
    }

    // ---- from_string() hex ----
    {
        print_header("from_string() hex (pow2 shift)");
        const auto r_small{bench_fromstr_hex_small()};
        const auto r_large{bench_fromstr_hex_large()};
        print_result(r_small, r_small.cycles_per_op);
        print_result(r_large, r_small.cycles_per_op);
        print_footer();
    }

    // ---- from_string() binary ----
    {
        print_header("from_string() binary (pow2 shift)");
        const auto r_small{bench_fromstr_bin_small()};
        const auto r_large{bench_fromstr_bin_large()};
        print_result(r_small, r_small.cycles_per_op);
        print_result(r_large, r_small.cycles_per_op);
        print_footer();
    }

    // ---- from_string() octal ----
    {
        print_header("from_string() octal (pow2 shift)");
        const auto r_large{bench_fromstr_oct_large()};
        print_result(r_large, r_large.cycles_per_op);
        print_footer();
    }

    // ---- to_string() decimal ----
    {
        print_header("to_string() decimal");
        const auto r_baseline{bench_stdlib_tostr_u64()};
        const auto r_small{bench_tostr_dec_small()};
        const auto r_large{bench_tostr_dec_large()};
        print_result(r_baseline, r_baseline.cycles_per_op);
        print_result(r_small, r_baseline.cycles_per_op);
        print_result(r_large, r_baseline.cycles_per_op);
        print_footer();
    }

    // ---- to_string() hex ----
    {
        print_header("to_string() hex (pow2 shift)");
        const auto r_small{bench_tostr_hex_small()};
        const auto r_large{bench_tostr_hex_large()};
        print_result(r_small, r_small.cycles_per_op);
        print_result(r_large, r_small.cycles_per_op);
        print_footer();
    }

    // ---- to_string() binary ----
    {
        print_header("to_string() binary (pow2 shift)");
        const auto r_large{bench_tostr_bin_large()};
        print_result(r_large, r_large.cycles_per_op);
        print_footer();
    }

    // ---- to_cstr() decimal ----
    {
        print_header("to_cstr() decimal (no alloc)");
        const auto r_small{bench_tocstr_dec_small()};
        const auto r_large{bench_tocstr_dec_large()};
        print_result(r_small, r_small.cycles_per_op);
        print_result(r_large, r_small.cycles_per_op);
        print_footer();
    }

    std::cout << "\n[DONE]\n";
    return 0;
}
