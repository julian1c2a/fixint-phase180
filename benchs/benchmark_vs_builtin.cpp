// =============================================================================
// Benchmark: nstd::uint128_t vs builtin types vs __int128 vs Boost
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
//   - boost::multiprecision::uint128_t       (cpp_int backend, header-only)
//   - boost::multiprecision::int128_t        (cpp_int backend, header-only)
//   - boost::multiprecision::checked_uint128_t (overflow-checked cpp_int)
//   - boost::multiprecision::mpz_int         (GMP backend, requires libgmp)
//   - boost::multiprecision::tom_int         (tommath backend, requires libtommath)
//
// Operations tested: add, sub, mul, div, shift, xor, comparison
//
// Compile (GCC):
//   g++ -std=c++20 -O2 -Iinclude benchs/benchmark_vs_builtin.cpp -lgmp -ltommath -o bench
// Compile (Clang):
//   clang++ -std=c++20 -O2 -Iinclude benchs/benchmark_vs_builtin.cpp -lgmp -ltommath -o bench
//
// =============================================================================

#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>

// RDTSC: cycle-accurate timing independent of clock frequency
#if defined(_MSC_VER) || (defined(__INTEL_LLVM_COMPILER) && defined(_WIN32))
#include <intrin.h>
static inline std::uint64_t rdtsc() { return __rdtsc(); }
#elif defined(__INTEL_LLVM_COMPILER)
#include <x86intrin.h>
static inline std::uint64_t rdtsc() { return __rdtsc(); }
#else
static inline std::uint64_t rdtsc() { return __builtin_ia32_rdtsc(); }
#endif

// Boost.Multiprecision backends
#include <boost/multiprecision/cpp_int.hpp>
#if !defined(_MSC_VER) || defined(FORCE_GMP_TOMMATH)
#define BENCH_HAS_GMP_TOMMATH 1
#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/tommath.hpp>
#endif

#ifdef __SIZEOF_INT128__
#define HAS_BUILTIN_INT128 1
#endif

using namespace nstd;

namespace bmp = boost::multiprecision;

// Boost type aliases
using boost_cpp_u128 = bmp::uint128_t;
using boost_cpp_i128 = bmp::int128_t;
using boost_checked_u128 = bmp::checked_uint128_t;
#ifdef BENCH_HAS_GMP_TOMMATH
using boost_gmp_int = bmp::mpz_int;
using boost_tom_int = bmp::tom_int;
#endif

// ============================================================================
// Configuration
// ============================================================================

#ifndef BENCH_ITERATIONS
#define BENCH_ITERATIONS 5000000
#endif

static constexpr std::size_t ITERATIONS{BENCH_ITERATIONS};
static constexpr std::size_t WARMUP{10000};

// ============================================================================
// Cycle counter (RDTSC-based, clock-frequency independent)
// ============================================================================

class CycleTimer
{
    std::uint64_t start_;

public:
    CycleTimer() : start_{rdtsc()} {}
    std::uint64_t elapsed_cycles() const
    {
        return rdtsc() - start_;
    }
};

// ============================================================================
// Result formatting
// ============================================================================

struct BenchResult
{
    std::string name;
    double cycles_per_op;
};

static void print_separator()
{
    std::cout << "+-------------------------------+--------------+-----------+\n";
}

static void print_header(const char *operation)
{
    std::cout << "\n[" << operation << "]\n";
    print_separator();
    std::cout << "| Type                          |  cyc/op      | vs u64    |\n";
    print_separator();
}

static void print_result(const BenchResult &r, double baseline_cyc)
{
    const double ratio{(baseline_cyc > 0.0) ? r.cycles_per_op / baseline_cyc : 0.0};
    std::cout << "| " << std::left << std::setw(29) << r.name << " | "
              << std::right << std::fixed << std::setprecision(2) << std::setw(12) << r.cycles_per_op << " | "
              << std::fixed << std::setprecision(2) << std::setw(6) << ratio << "x   |\n";
}

// ============================================================================
// Prevent optimization (volatile sink)
// ============================================================================

template <typename T>
static void doNotOptimize(T &val)
{
#if defined(_MSC_VER) || defined(__INTEL_LLVM_COMPILER)
    // MSVC/Intel-Windows: read+write one byte through volatile pointer.
    // Forces the compiler to actually compute val (can't be eliminated).
    *reinterpret_cast<char volatile *>(&val) =
        *reinterpret_cast<char volatile *>(&val);
#elif defined(__clang__)
    // Clang: "+r,m" works well — Clang chooses memory operands for structs,
    // which avoids register shuffling. Using split "+r" would cause
    // redundant stores on Clang.
    asm volatile("" : "+r,m"(val) : : "memory");
#else
    // GCC: For 128-bit structs, "+r,m" causes 4 unnecessary register
    // shuffles per iteration (GCC can't map a struct to a register pair).
    // Fix: split into two separate "+r" constraints on each uint64_t half.
    // This generates the same optimal addq/adcq or subq/sbbq as __int128.
    if constexpr (sizeof(T) == 16 && alignof(T) >= alignof(std::uint64_t))
    {
        auto *p = reinterpret_cast<std::uint64_t *>(&val);
        asm volatile("" : "+r"(p[0]), "+r"(p[1]) : : "memory");
    }
    else
    {
        asm volatile("" : "+r,m"(val) : : "memory");
    }
#endif
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::uint128_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::int128_t (TC)", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"unsigned __int128", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"__int128", cycles / ITERATIONS};
}
#endif

static BenchResult bench_add_boost_cpp_u128()
{
    boost_cpp_u128 a{"0xDEADBEEF12345678"};
    boost_cpp_u128 b{"0x1234567890ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int u128", cycles / ITERATIONS};
}

static BenchResult bench_add_boost_cpp_i128()
{
    boost_cpp_i128 a{"0xDEADBEEF12345678"};
    boost_cpp_i128 b{"0x1234567890ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int i128", cycles / ITERATIONS};
}

static BenchResult bench_add_boost_chk_u128()
{
    boost_checked_u128 a{"0xDEADBEEF12345678"};
    boost_checked_u128 b{"0x1234567890ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::checked_uint128", cycles / ITERATIONS};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_add_boost_gmp()
{
    boost_gmp_int a{"0xDEADBEEF12345678"};
    boost_gmp_int b{"0x1234567890ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int", cycles / ITERATIONS};
}

static BenchResult bench_add_boost_tom()
{
    boost_tom_int a{"0xDEADBEEF12345678"};
    boost_tom_int b{"0x1234567890ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a += b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int", cycles / ITERATIONS};
}

// --- GMP & tommath constrained to 128 bits (& mask128) ---
static const boost_gmp_int gmp_mask128{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
static const boost_tom_int tom_mask128{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};

static BenchResult bench_add_boost_gmp128()
{
    boost_gmp_int a{"0xDEADBEEF12345678"};
    boost_gmp_int b{"0x1234567890ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a + b) & gmp_mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a + b) & gmp_mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int [128]", cycles / ITERATIONS};
}

static BenchResult bench_add_boost_tom128()
{
    boost_tom_int a{"0xDEADBEEF12345678"};
    boost_tom_int b{"0x1234567890ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a + b) & tom_mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a + b) & tom_mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int [128]", cycles / ITERATIONS};
}
#endif // BENCH_HAS_GMP_TOMMATH

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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::uint128_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::int128_t (TC)", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"unsigned __int128", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"__int128", cycles / ITERATIONS};
}
#endif

static BenchResult bench_sub_boost_cpp_u128()
{
    boost_cpp_u128 a{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    boost_cpp_u128 b{1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int u128", cycles / ITERATIONS};
}

static BenchResult bench_sub_boost_cpp_i128()
{
    boost_cpp_i128 a{"0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    boost_cpp_i128 b{1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int i128", cycles / ITERATIONS};
}

static BenchResult bench_sub_boost_chk_u128()
{
    boost_checked_u128 a{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    boost_checked_u128 b{1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::checked_uint128", cycles / ITERATIONS};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_sub_boost_gmp()
{
    boost_gmp_int a{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    boost_gmp_int b{1};
    const boost_gmp_int wrap{"0x100000000000000000000000000000000"}; // 2^128
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        if (a < 0)
        {
            a += wrap;
        }
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        if (a < 0)
        {
            a += wrap;
        }
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int", cycles / ITERATIONS};
}

static BenchResult bench_sub_boost_tom()
{
    boost_tom_int a{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    boost_tom_int b{1};
    const boost_tom_int wrap{"0x100000000000000000000000000000000"}; // 2^128
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a -= b;
        if (a < 0)
        {
            a += wrap;
        }
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a -= b;
        if (a < 0)
        {
            a += wrap;
        }
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int", cycles / ITERATIONS};
}

static BenchResult bench_sub_boost_gmp128()
{
    boost_gmp_int a{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    boost_gmp_int b{1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a - b) & gmp_mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a - b) & gmp_mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int [128]", cycles / ITERATIONS};
}

static BenchResult bench_sub_boost_tom128()
{
    boost_tom_int a{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    boost_tom_int b{1};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a - b) & tom_mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a - b) & tom_mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int [128]", cycles / ITERATIONS};
}
#endif // BENCH_HAS_GMP_TOMMATH

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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::uint128_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::int128_t (TC)", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"unsigned __int128", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"__int128", cycles / ITERATIONS};
}
#endif

static BenchResult bench_mul_boost_cpp_u128()
{
    boost_cpp_u128 a{123456789};
    boost_cpp_u128 b{987654321};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int u128", cycles / ITERATIONS};
}

static BenchResult bench_mul_boost_cpp_i128()
{
    boost_cpp_i128 a{123456789};
    boost_cpp_i128 b{987654321};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = a * b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int i128", cycles / ITERATIONS};
}

static BenchResult bench_mul_boost_chk_u128()
{
    // checked_uint128 throws on overflow; use non-accumulating pattern
    const boost_checked_u128 a{123456789};
    const boost_checked_u128 b{987654321};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto r = a * b;
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto r = a * b;
        doNotOptimize(r);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::checked_uint128(*)", cycles / ITERATIONS};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_mul_boost_gmp()
{
    // Arbitrary precision: non-accumulating to avoid unbounded growth
    const boost_gmp_int a{123456789};
    const boost_gmp_int b{987654321};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto r = a * b;
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto r = a * b;
        doNotOptimize(r);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int(*)", cycles / ITERATIONS};
}

static BenchResult bench_mul_boost_tom()
{
    // Arbitrary precision: non-accumulating to avoid unbounded growth
    const boost_tom_int a{123456789};
    const boost_tom_int b{987654321};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto r = a * b;
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto r = a * b;
        doNotOptimize(r);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int(*)", cycles / ITERATIONS};
}

static BenchResult bench_mul_boost_gmp128()
{
    boost_gmp_int a{123456789};
    const boost_gmp_int b{987654321};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a * b) & gmp_mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a * b) & gmp_mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int [128]", cycles / ITERATIONS};
}

static BenchResult bench_mul_boost_tom128()
{
    boost_tom_int a{123456789};
    const boost_tom_int b{987654321};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a * b) & tom_mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a * b) & tom_mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int [128]", cycles / ITERATIONS};
}
#endif // BENCH_HAS_GMP_TOMMATH

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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + uint128_t{0, 1};
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::uint128_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + int128_t{0, 1};
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::int128_t (TC)", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"unsigned __int128", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"__int128", cycles / ITERATIONS};
}
#endif

static BenchResult bench_div_boost_cpp_u128()
{
    boost_cpp_u128 a{"0xDEADBEEF12345678"};
    boost_cpp_u128 b{12345};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int u128", cycles / ITERATIONS};
}

static BenchResult bench_div_boost_cpp_i128()
{
    boost_cpp_i128 a{"0xDEADBEEF12345678"};
    boost_cpp_i128 b{12345};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int i128", cycles / ITERATIONS};
}

static BenchResult bench_div_boost_chk_u128()
{
    boost_checked_u128 a{"0xDEADBEEF12345678"};
    boost_checked_u128 b{12345};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::checked_uint128", cycles / ITERATIONS};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_div_boost_gmp()
{
    boost_gmp_int a{"0xDEADBEEF12345678"};
    boost_gmp_int b{12345};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int", cycles / ITERATIONS};
}

static BenchResult bench_div_boost_tom()
{
    boost_tom_int a{"0xDEADBEEF12345678"};
    boost_tom_int b{12345};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = q + 1;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int", cycles / ITERATIONS};
}

static BenchResult bench_div_boost_gmp128()
{
    boost_gmp_int a{"0xDEADBEEF12345678"};
    boost_gmp_int b{12345};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = (q + 1) & gmp_mask128;
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = (q + 1) & gmp_mask128;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int [128]", cycles / ITERATIONS};
}

static BenchResult bench_div_boost_tom128()
{
    boost_tom_int a{"0xDEADBEEF12345678"};
    boost_tom_int b{12345};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = (q + 1) & tom_mask128;
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto q = a / b;
        doNotOptimize(q);
        a = (q + 1) & tom_mask128;
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int [128]", cycles / ITERATIONS};
}
#endif // BENCH_HAS_GMP_TOMMATH

// ============================================================================
// BENCHMARK: Left Shift (rotate)
// ============================================================================

static BenchResult bench_shl_u64()
{
    std::uint64_t a{0xDEADBEEF12345678ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a << 3) | (a >> 61);
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a << 3) | (a >> 61);
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t", cycles / ITERATIONS};
}

static BenchResult bench_shl_nstd_u128()
{
    uint128_t a{0xDEADBEEFull, 0x12345678ull};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::uint128_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"unsigned __int128", cycles / ITERATIONS};
}
#endif

static BenchResult bench_shl_boost_cpp_u128()
{
    boost_cpp_u128 a{"0xDEADBEEF0000000012345678"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = (a << 3) | (a >> 125);
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int u128", cycles / ITERATIONS};
}

static BenchResult bench_shl_boost_chk_u128()
{
    // (*) Non-accumulating: checked throws on shift overflow
    const boost_checked_u128 a{"0x12345678"};
    boost_checked_u128 r{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = a << (i % 97); // 29 bits + 96 = 125 bits, fits in 128
        doNotOptimize(r);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = a << (i % 97);
        doNotOptimize(r);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::checked_uint128 (*)", cycles / ITERATIONS};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_shl_boost_gmp()
{
    boost_gmp_int a{"0xDEADBEEF0000000012345678"};
    const boost_gmp_int mask128{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = ((a << 3) | (a >> 125)) & mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = ((a << 3) | (a >> 125)) & mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int", cycles / ITERATIONS};
}

static BenchResult bench_shl_boost_tom()
{
    boost_tom_int a{"0xDEADBEEF0000000012345678"};
    const boost_tom_int mask128{"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a = ((a << 3) | (a >> 125)) & mask128;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a = ((a << 3) | (a >> 125)) & mask128;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int", cycles / ITERATIONS};
}
#endif // BENCH_HAS_GMP_TOMMATH

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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::uint128_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"unsigned __int128", cycles / ITERATIONS};
}
#endif

static BenchResult bench_xor_boost_cpp_u128()
{
    boost_cpp_u128 a{"0xDEADBEEF0000000012345678"};
    boost_cpp_u128 b{"0xCAFEBABE0000000090ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int u128", cycles / ITERATIONS};
}

static BenchResult bench_xor_boost_chk_u128()
{
    boost_checked_u128 a{"0xDEADBEEF0000000012345678"};
    boost_checked_u128 b{"0xCAFEBABE0000000090ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::checked_uint128", cycles / ITERATIONS};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_xor_boost_gmp()
{
    boost_gmp_int a{"0xDEADBEEF0000000012345678"};
    boost_gmp_int b{"0xCAFEBABE0000000090ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int", cycles / ITERATIONS};
}

static BenchResult bench_xor_boost_tom()
{
    boost_tom_int a{"0xDEADBEEF0000000012345678"};
    boost_tom_int b{"0xCAFEBABE0000000090ABCDEF"};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        a ^= b;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int", cycles / ITERATIONS};
}
#endif // BENCH_HAS_GMP_TOMMATH

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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        a += r;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"uint64_t", cycles / ITERATIONS};
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
        {
            a += uint128_t{0, 1};
        }
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += uint128_t{0, 1};
        }
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"nstd::uint128_t", cycles / ITERATIONS};
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
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        a += r;
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"unsigned __int128", cycles / ITERATIONS};
}
#endif

static BenchResult bench_cmp_boost_cpp_u128()
{
    boost_cpp_u128 a{"0xDEADBEEF0000000012345678"};
    const boost_cpp_u128 b{"0xDEADBEEF0000000012345679"};
    volatile bool r{false};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::cpp_int u128", cycles / ITERATIONS};
}

static BenchResult bench_cmp_boost_chk_u128()
{
    boost_checked_u128 a{"0xDEADBEEF0000000012345678"};
    const boost_checked_u128 b{"0xDEADBEEF0000000012345679"};
    volatile bool r{false};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::checked_uint128", cycles / ITERATIONS};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_cmp_boost_gmp()
{
    boost_gmp_int a{"0xDEADBEEF0000000012345678"};
    const boost_gmp_int b{"0xDEADBEEF0000000012345679"};
    volatile bool r{false};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::gmp_int", cycles / ITERATIONS};
}

static BenchResult bench_cmp_boost_tom()
{
    boost_tom_int a{"0xDEADBEEF0000000012345678"};
    const boost_tom_int b{"0xDEADBEEF0000000012345679"};
    volatile bool r{false};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        r = (a < b);
        if (r)
        {
            a += 1;
        }
        doNotOptimize(a);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    return {"boost::tom_int", cycles / ITERATIONS};
}
#endif // BENCH_HAS_GMP_TOMMATH

// ============================================================================
// MAIN
// ============================================================================

int main()
{
    std::cout << "================================================================\n";
    std::cout << "  BENCHMARK: nstd vs builtin vs __int128 vs Boost\n";
    std::cout << "================================================================\n";
    std::cout << "  Iterations: " << ITERATIONS << "\n";
    std::cout << "  Warmup:     " << WARMUP << "\n";
#ifdef HAS_BUILTIN_INT128
    std::cout << "  __int128:   available\n";
#else
    std::cout << "  __int128:   NOT available\n";
#endif
    std::cout << "  Boost:      cpp_int, checked";
#ifdef BENCH_HAS_GMP_TOMMATH
    std::cout << ", GMP, tommath";
#endif
    std::cout << "\n";
    std::cout << "================================================================\n";

    BenchResult r{};
    double baseline{0.0};

    // --- Addition ---
    print_header("Addition (+)");
    r = bench_add_u64();
    baseline = r.cycles_per_op;
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
    r = bench_add_boost_cpp_u128();
    print_result(r, baseline);
    r = bench_add_boost_cpp_i128();
    print_result(r, baseline);
    r = bench_add_boost_chk_u128();
    print_result(r, baseline);
#ifdef BENCH_HAS_GMP_TOMMATH
    r = bench_add_boost_gmp();
    print_result(r, baseline);
    r = bench_add_boost_gmp128();
    print_result(r, baseline);
    r = bench_add_boost_tom();
    print_result(r, baseline);
    r = bench_add_boost_tom128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Subtraction ---
    print_header("Subtraction (-)");
    r = bench_sub_u64();
    baseline = r.cycles_per_op;
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
    r = bench_sub_boost_cpp_u128();
    print_result(r, baseline);
    r = bench_sub_boost_cpp_i128();
    print_result(r, baseline);
    r = bench_sub_boost_chk_u128();
    print_result(r, baseline);
#ifdef BENCH_HAS_GMP_TOMMATH
    r = bench_sub_boost_gmp();
    print_result(r, baseline);
    r = bench_sub_boost_gmp128();
    print_result(r, baseline);
    r = bench_sub_boost_tom();
    print_result(r, baseline);
    r = bench_sub_boost_tom128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Multiplication ---
    print_header("Multiplication (*)");
    r = bench_mul_u64();
    baseline = r.cycles_per_op;
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
    r = bench_mul_boost_cpp_u128();
    print_result(r, baseline);
    r = bench_mul_boost_cpp_i128();
    print_result(r, baseline);
    r = bench_mul_boost_chk_u128();
    print_result(r, baseline);
#ifdef BENCH_HAS_GMP_TOMMATH
    r = bench_mul_boost_gmp();
    print_result(r, baseline);
    r = bench_mul_boost_gmp128();
    print_result(r, baseline);
    r = bench_mul_boost_tom();
    print_result(r, baseline);
    r = bench_mul_boost_tom128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Division ---
    print_header("Division (/)");
    r = bench_div_u64();
    baseline = r.cycles_per_op;
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
    r = bench_div_boost_cpp_u128();
    print_result(r, baseline);
    r = bench_div_boost_cpp_i128();
    print_result(r, baseline);
    r = bench_div_boost_chk_u128();
    print_result(r, baseline);
#ifdef BENCH_HAS_GMP_TOMMATH
    r = bench_div_boost_gmp();
    print_result(r, baseline);
    r = bench_div_boost_gmp128();
    print_result(r, baseline);
    r = bench_div_boost_tom();
    print_result(r, baseline);
    r = bench_div_boost_tom128();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Shift ---
    print_header("Shift (<<3 | >>125, rotate)");
    r = bench_shl_u64();
    baseline = r.cycles_per_op;
    print_result(r, baseline);
    r = bench_shl_nstd_u128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_shl_builtin_u128();
    print_result(r, baseline);
#endif
    r = bench_shl_boost_cpp_u128();
    print_result(r, baseline);
    r = bench_shl_boost_chk_u128();
    print_result(r, baseline);
#ifdef BENCH_HAS_GMP_TOMMATH
    r = bench_shl_boost_gmp();
    print_result(r, baseline);
    r = bench_shl_boost_tom();
    print_result(r, baseline);
#endif
    print_separator();

    // --- XOR ---
    print_header("Bitwise XOR (^)");
    r = bench_xor_u64();
    baseline = r.cycles_per_op;
    print_result(r, baseline);
    r = bench_xor_nstd_u128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_xor_builtin_u128();
    print_result(r, baseline);
#endif
    r = bench_xor_boost_cpp_u128();
    print_result(r, baseline);
    r = bench_xor_boost_chk_u128();
    print_result(r, baseline);
#ifdef BENCH_HAS_GMP_TOMMATH
    r = bench_xor_boost_gmp();
    print_result(r, baseline);
    r = bench_xor_boost_tom();
    print_result(r, baseline);
#endif
    print_separator();

    // --- Comparison ---
    print_header("Comparison (<)");
    r = bench_cmp_u64();
    baseline = r.cycles_per_op;
    print_result(r, baseline);
    r = bench_cmp_nstd_u128();
    print_result(r, baseline);
#ifdef HAS_BUILTIN_INT128
    r = bench_cmp_builtin_u128();
    print_result(r, baseline);
#endif
    r = bench_cmp_boost_cpp_u128();
    print_result(r, baseline);
    r = bench_cmp_boost_chk_u128();
    print_result(r, baseline);
#ifdef BENCH_HAS_GMP_TOMMATH
    r = bench_cmp_boost_gmp();
    print_result(r, baseline);
    r = bench_cmp_boost_tom();
    print_result(r, baseline);
#endif
    print_separator();

    std::cout << "\n================================================================\n";
    std::cout << "  vs u64 = ratio vs uint64_t baseline (1.00x = same speed)\n";
    std::cout << "  Lower ratio = faster. >1.00x = slower than uint64_t.\n";
    std::cout << "  (*) = non-accumulating pattern (no loop dependency)\n";
    std::cout << "        used for arb-precision/checked to avoid overflow\n";
    std::cout << "  [128] = arbitrary-precision backend masked to 128 bits\n";
    std::cout << "          (& mask128 after each op) for fair comparison\n";
    std::cout << "================================================================\n";

    return 0;
}
