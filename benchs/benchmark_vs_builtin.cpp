// =============================================================================
// Benchmark: nstd::uint128_t vs builtin types vs __int128
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Compares performance of:
//   - uint64_t (baseline)
//   - nstd::uint128_t (this library, binary natural / unsigned)
//   - nstd::int128_t  (this library, two's complement / signed)
//   - unsigned __int128 (GCC/Clang compiler extension)
//   - __int128          (GCC/Clang compiler extension)
//
// Operations tested: add, sub, mul, div, mod, shift, xor, comparison
// =============================================================================

#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdint>
#include <string>

#ifdef __SIZEOF_INT128__
#define HAS_BUILTIN_INT128 1
#endif

using namespace nstd;

// ============================================================================
// Configuration
// ============================================================================

#ifndef BENCH_ITERATIONS
#define BENCH_ITERATIONS 5000000
#endif

static constexpr std::size_t ITERATIONS{BENCH_ITERATIONS};
static constexpr std::size_t WARMUP{10000};

// ============================================================================
// Timer
// ============================================================================

class Timer
{
    std::chrono::high_resolution_clock::time_point start_;

public:
    Timer() : start_{std::chrono::high_resolution_clock::now()} {}
    double elapsed_ns() const
    {
        const auto end{std::chrono::high_resolution_clock::now()};
        return std::chrono::duration<double, std::nano>(end - start_).count();
    }
};

// ============================================================================
// Result formatting
// ============================================================================

struct BenchResult
{
    std::string name;
    double ns_per_op;
    double ops_per_sec;
};

static void print_separator()
{
    std::cout << "+----------------------------+--------------+------------------+-----------+\n";
}

static void print_header(const char *operation)
{
    std::cout << "\n[" << operation << "]\n";
    print_separator();
    std::cout << "| Type                       |   ns/op      |       ops/sec    | vs u64    |\n";
    print_separator();
}

static void print_result(const BenchResult &r, double baseline_ns)
{
    const double ratio{(baseline_ns > 0.0) ? r.ns_per_op / baseline_ns : 0.0};
    std::cout << "| " << std::left << std::setw(26) << r.name << " | "
              << std::right << std::fixed << std::setprecision(2) << std::setw(12) << r.ns_per_op << " | "
              << std::scientific << std::setprecision(2) << std::setw(16) << r.ops_per_sec << " | "
              << std::fixed << std::setprecision(2) << std::setw(6) << ratio << "x   |\n";
}

// ============================================================================
// Prevent optimization (volatile sink)
// ============================================================================

template <typename T>
static void doNotOptimize(const T &val)
{
    asm volatile("" : : "g"(&val) : "memory");
}

// ============================================================================
// BENCHMARK: Addition
// ============================================================================

static BenchResult bench_add_u64()
{
    std::uint64_t a{0xDEADBEEF12345678ull};
    std::uint64_t b{0x1234567890ABCDEFull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"uint64_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_add_nstd_u128()
{
    uint128_t a{0, 0xDEADBEEF12345678ull};
    const uint128_t b{0, 0x1234567890ABCDEFull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::uint128_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_add_nstd_i128()
{
    int128_t a{0, 0xDEADBEEF12345678ull};
    const int128_t b{0, 0x1234567890ABCDEFull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::int128_t (TC)", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_add_builtin_u128()
{
    unsigned __int128 a{0xDEADBEEF12345678ull};
    const unsigned __int128 b{0x1234567890ABCDEFull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"unsigned __int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_add_builtin_i128()
{
    __int128 a{static_cast<__int128>(0xDEADBEEF12345678ull)};
    const __int128 b{static_cast<__int128>(0x1234567890ABCDEFull)};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"__int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}
#endif

// ============================================================================
// BENCHMARK: Subtraction
// ============================================================================

static BenchResult bench_sub_u64()
{
    std::uint64_t a{0xFFFFFFFFFFFFFFFFull};
    const std::uint64_t b{0x0000000000000001ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"uint64_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_sub_nstd_u128()
{
    uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
    const uint128_t b{0, 1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::uint128_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_sub_nstd_i128()
{
    int128_t a{0x7FFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
    const int128_t b{0, 1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::int128_t (TC)", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_sub_builtin_u128()
{
    unsigned __int128 a{~static_cast<unsigned __int128>(0)};
    const unsigned __int128 b{1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"unsigned __int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_sub_builtin_i128()
{
    __int128 a{static_cast<__int128>(0x7FFFFFFFFFFFFFFFll)};
    const __int128 b{1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"__int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}
#endif

// ============================================================================
// BENCHMARK: Multiplication
// ============================================================================

static BenchResult bench_mul_u64()
{
    std::uint64_t a{123456789ull};
    const std::uint64_t b{987654321ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"uint64_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_mul_nstd_u128()
{
    uint128_t a{0, 123456789ull};
    const uint128_t b{0, 987654321ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::uint128_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_mul_nstd_i128()
{
    int128_t a{0, 123456789ull};
    const int128_t b{0, 987654321ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::int128_t (TC)", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_mul_builtin_u128()
{
    unsigned __int128 a{123456789ull};
    const unsigned __int128 b{987654321ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"unsigned __int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_mul_builtin_i128()
{
    __int128 a{123456789ll};
    const __int128 b{987654321ll};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"__int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}
#endif

// ============================================================================
// BENCHMARK: Division
// ============================================================================

static BenchResult bench_div_u64()
{
    std::uint64_t a{0xDEADBEEF12345678ull};
    const std::uint64_t b{12345ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double ns{t.elapsed_ns()};
    return {"uint64_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_div_nstd_u128()
{
    uint128_t a{0, 0xDEADBEEF12345678ull};
    const uint128_t b{0, 12345ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + uint128_t{0, 1};
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + uint128_t{0, 1};
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::uint128_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_div_nstd_i128()
{
    int128_t a{0, 0xDEADBEEF12345678ull};
    const int128_t b{0, 12345ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + int128_t{0, 1};
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + int128_t{0, 1};
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::int128_t (TC)", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_div_builtin_u128()
{
    unsigned __int128 a{0xDEADBEEF12345678ull};
    const unsigned __int128 b{12345ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double ns{t.elapsed_ns()};
    return {"unsigned __int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_div_builtin_i128()
{
    __int128 a{static_cast<__int128>(0xDEADBEEF12345678ull)};
    const __int128 b{12345ll};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double ns{t.elapsed_ns()};
    return {"__int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}
#endif

// ============================================================================
// BENCHMARK: Left Shift
// ============================================================================

static BenchResult bench_shl_u64()
{
    std::uint64_t a{0xDEADBEEF12345678ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a << 3) | (a >> 61);
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a << 3) | (a >> 61);
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"uint64_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_shl_nstd_u128()
{
    uint128_t a{0xDEADBEEFull, 0x12345678ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::uint128_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_shl_builtin_u128()
{
    unsigned __int128 a{0xDEADBEEFull};
    a = (a << 64) | 0x12345678ull;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"unsigned __int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}
#endif

// ============================================================================
// BENCHMARK: XOR (bitwise)
// ============================================================================

static BenchResult bench_xor_u64()
{
    std::uint64_t a{0xDEADBEEF12345678ull};
    const std::uint64_t b{0xCAFEBABE90ABCDEFull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"uint64_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_xor_nstd_u128()
{
    uint128_t a{0xDEADBEEFull, 0x12345678ull};
    const uint128_t b{0xCAFEBABEull, 0x90ABCDEFull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::uint128_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_xor_builtin_u128()
{
    unsigned __int128 a{0xDEADBEEFull};
    a = (a << 64) | 0x12345678ull;
    unsigned __int128 b{0xCAFEBABEull};
    b = (b << 64) | 0x90ABCDEFull;
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"unsigned __int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}
#endif

// ============================================================================
// BENCHMARK: Comparison (<)
// ============================================================================

static BenchResult bench_cmp_u64()
{
    std::uint64_t a{0xDEADBEEF12345678ull};
    std::uint64_t b{0xDEADBEEF12345679ull};
    volatile bool r{false};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = (a < b);
        a += r;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        a += r;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"uint64_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

static BenchResult bench_cmp_nstd_u128()
{
    uint128_t a{0xDEADBEEFull, 0x12345678ull};
    const uint128_t b{0xDEADBEEFull, 0x12345679ull};
    volatile bool r{false};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = (a < b);
        if (r)
            a += uint128_t{0, 1};
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        if (r)
            a += uint128_t{0, 1};
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"nstd::uint128_t", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_cmp_builtin_u128()
{
    unsigned __int128 a{0xDEADBEEFull};
    a = (a << 64) | 0x12345678ull;
    unsigned __int128 b{0xDEADBEEFull};
    b = (b << 64) | 0x12345679ull;
    volatile bool r{false};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = (a < b);
        a += r;
        doNotOptimize(a);
    }
    Timer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        a += r;
        doNotOptimize(a);
    }
    const double ns{t.elapsed_ns()};
    return {"unsigned __int128", ns / ITERATIONS, ITERATIONS / (ns / 1e9)};
}
#endif

// ============================================================================
// MAIN
// ============================================================================

int main()
{
    std::cout << "================================================================\n";
    std::cout << "  BENCHMARK: nstd::uint128_t vs builtin types vs __int128\n";
    std::cout << "================================================================\n";
    std::cout << "  Iterations: " << ITERATIONS << "\n";
    std::cout << "  Warmup:     " << WARMUP << "\n";
#ifdef HAS_BUILTIN_INT128
    std::cout << "  __int128:   available\n";
#else
    std::cout << "  __int128:   NOT available\n";
#endif
    std::cout << "================================================================\n";

    BenchResult r{};
    double baseline{0.0};

    // --- Addition ---
    print_header("Addition (+)");
    r = bench_add_u64();
    baseline = r.ns_per_op;
    print_result(r, baseline);
    r = bench_add_nstd_u128();
    print_result(r, baseline);
    r = bench_add_nstd_i128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_add_builtin_u128();
    print_result(r, baseline);
    r = bench_add_builtin_i128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Subtraction ---
    print_header("Subtraction (-)");
    r = bench_sub_u64();
    baseline = r.ns_per_op;
    print_result(r, baseline);
    r = bench_sub_nstd_u128();
    print_result(r, baseline);
    r = bench_sub_nstd_i128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_sub_builtin_u128();
    print_result(r, baseline);
    r = bench_sub_builtin_i128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Multiplication ---
    print_header("Multiplication (*)");
    r = bench_mul_u64();
    baseline = r.ns_per_op;
    print_result(r, baseline);
    r = bench_mul_nstd_u128();
    print_result(r, baseline);
    r = bench_mul_nstd_i128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_mul_builtin_u128();
    print_result(r, baseline);
    r = bench_mul_builtin_i128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Division ---
    print_header("Division (/)");
    r = bench_div_u64();
    baseline = r.ns_per_op;
    print_result(r, baseline);
    r = bench_div_nstd_u128();
    print_result(r, baseline);
    r = bench_div_nstd_i128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_div_builtin_u128();
    print_result(r, baseline);
    r = bench_div_builtin_i128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Shift ---
    print_header("Shift (<<3 | >>125, rotate)");
    r = bench_shl_u64();
    baseline = r.ns_per_op;
    print_result(r, baseline);
    r = bench_shl_nstd_u128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_shl_builtin_u128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- XOR ---
    print_header("Bitwise XOR (^)");
    r = bench_xor_u64();
    baseline = r.ns_per_op;
    print_result(r, baseline);
    r = bench_xor_nstd_u128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_xor_builtin_u128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Comparison ---
    print_header("Comparison (<)");
    r = bench_cmp_u64();
    baseline = r.ns_per_op;
    print_result(r, baseline);
    r = bench_cmp_nstd_u128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_cmp_builtin_u128();
    print_result(r, baseline);
#endif
    print_separator();

    std::cout << "\n================================================================\n";
    std::cout << "  vs u64 = ratio vs uint64_t baseline (1.00x = same speed)\n";
    std::cout << "  Lower ratio = faster. >1.00x = slower than uint64_t.\n";
    std::cout << "================================================================\n";

    return 0;
}
