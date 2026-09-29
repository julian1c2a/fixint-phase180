// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
// `int128_param_t` esta deprecado (P1.5 tramo 2, ADR-006) y este fichero lo usa
// A PROPOSITO: prueba el tipo que se retira, o lo cruza contra el nuevo. Avisar
// aqui no informa de nada y entierra los avisos de verdad. Se va entero en 1.90.
#define NSTD_SILENCIA_INT128_PARAM_DEPRECADO
#define NSTD_QUIERO_INT128_PARAM

#include "int128_parameterized.hpp"
#include "bench_common.hpp"

// Boost.Multiprecision backends
#include <array>

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
//
// REESCRITO EL 29 SEP 2026 (P2.19). Lo que habia aqui NO MEDIA COMPARACIONES, y
// la propia tabla lo delataba: `uint64_t` y `unsigned __int128` salian MAS LENTOS
// que los tipos de 128 bits, lo cual es imposible -- son las mismas
// instrucciones mas una.
//
// EL FALLO. De los siete bucles habia DOS FORMAS:
//
//     uint64_t y unsigned __int128        los otros cinco
//     r = (a < b);                        r = (a < b);
//     a += r;                             if (r) { a += 1; }
//
// con `r` declarada `volatile bool`. Cada vuelta la escribe en memoria y la
// vuelve a leer. En la forma `a += r`, ese valor recien leido entra en `a`, que
// es lo que compara la vuelta siguiente: el REENVIO DE ALMACEN A CARGA --unos
// cuatro o cinco ciclos-- queda DENTRO de la cadena de dependencia del bucle. En
// la forma `if (r)` el salto esta perfectamente predicho --tras la primera
// vuelta `r` es siempre falso y `a` ya no cambia-- y esa misma latencia se
// solapa. Los 4,85 ciclos que marcaba `uint64_t` eran, casi exactos, el coste de
// ese reenvio; no el de comparar.
//
// Y DEBAJO HABIA UN SEGUNDO DEFECTO: los operandos no cambiaban. Tras la primera
// vuelta la comparacion era entre dos constantes y su resultado siempre el
// mismo, o sea el caso mas facil que existe para el predictor de saltos.
//
// LO QUE MIDE AHORA: el RENDIMIENTO de la comparacion --cuantas caben por
// ciclo-- con operandos que cambian. Un solo bucle para los siete tipos, la
// cadena de dependencia en el acumulador, ningun `volatile` en ninguna parte, y
// de los ocho valores que entran en el ciclo cuatro quedan por debajo del
// comparando y cuatro no, POR CONSTRUCCION.
//
// NO ES LO MISMO QUE MEDIR LATENCIA, y conviene tenerlo presente al leer la
// tabla: una comparacion cuya respuesta hace falta de inmediato cuesta mas que
// una de estas. Medir la latencia pide que el resultado realimente al operando,
// y eso obliga a hacer aritmetica sobre `T` dentro del bucle -- con lo que se
// mediria comparacion MAS suma, y el coste de la suma no es igual en los siete
// tipos. Por eso se mide rendimiento: es lo unico que se puede medir igual para
// todos.

/// @brief Cuantos valores distintos entran en el ciclo.
///
/// Ocho caben de sobra en cache y el `% CMP_VALORES` se compila a un `and`.
static constexpr std::size_t CMP_VALORES = 8;

/// @brief Ocho valores repartidos alrededor de `centro`, la mitad por debajo.
///
/// Se construyen con aritmetica en vez de escribirse a mano para cada tipo: asi
/// el reparto es identico en los siete **por construccion**, y no depende de que
/// quien escriba cincuenta y seis constantes no se equivoque en ninguna.
template <typename T>
static std::array<T, CMP_VALORES> cmp_valores(const T &centro, const T &paso)
{
    std::array<T, CMP_VALORES> v{};
    T x{centro - paso * T{CMP_VALORES / 2}};
    for (std::size_t i{0}; i < CMP_VALORES; ++i)
    {
        v[i] = x;
        x = x + paso;
    }
    return v;
}

/// @brief El bucle de medida, uno solo para los siete tipos.
///
/// La cadena de dependencia es `acc`, que es un `uint64_t` en todos los casos,
/// de modo que lo unico que cambia entre tipos es la comparacion. Sin
/// `volatile`: la barrera la pone `doNotOptimize`, que no obliga a pasar por
/// memoria.
template <typename T>
static double cmp_bucle(const std::array<T, CMP_VALORES> &izq, const T &der)
{
    std::uint64_t acc{0};
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        acc += static_cast<std::uint64_t>(izq[i % CMP_VALORES] < der);
        doNotOptimize(acc);
    }
    CycleTimer t;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        acc += static_cast<std::uint64_t>(izq[i % CMP_VALORES] < der);
        doNotOptimize(acc);
    }
    const double cycles{static_cast<double>(t.elapsed_cycles())};
    doNotOptimize(acc);
    return cycles / ITERATIONS;
}

/// @brief Vale uno, pero el compilador no puede saberlo.
///
/// Sin esto, TODO lo que sigue es constante de compilacion --los ocho valores,
/// el comparando, las ocho comparaciones-- y GCC evalua el bucle entero. Lo
/// avisa de una forma que conviene saber leer: `doNotOptimize` sobre lo que ya
/// es un inmediato no compila, «impossible constraint in 'asm'». Ese error no es
/// un problema del arnes; es el arnes diciendo que no quedaba nada que medir.
///
/// **El `volatile` se lee UNA vez y FUERA de la medida.** Meterlo dentro del
/// bucle es exactamente el fallo que este banco tenia y que P2.19 arregla: ahi
/// el reenvio de almacen a carga entra en la cadena de dependencia y se mide eso
/// en vez de la comparacion.
static volatile std::uint64_t cmp_semilla{1};

/// @brief Monta los valores y mide. Los exponentes evitan escribir constantes de
///        128 bits a mano y valen igual para un tipo de 64 que para uno de 128.
template <typename T>
static double cmp_mide(unsigned exp_centro, unsigned exp_paso)
{
    const T uno{static_cast<std::uint64_t>(cmp_semilla)};
    const T centro{uno << exp_centro};
    const T paso{uno << exp_paso};
    return cmp_bucle<T>(cmp_valores<T>(centro, paso), centro);
}

static BenchResult bench_cmp_u64() { return {"uint64_t", cmp_mide<std::uint64_t>(63, 59)}; }

static BenchResult bench_cmp_nstd_u128() { return {"nstd::uint128_t", cmp_mide<uint128_t>(127, 123)}; }

#ifdef HAS_BUILTIN_INT128
static BenchResult bench_cmp_builtin_u128()
{
    return {"unsigned __int128", cmp_mide<unsigned __int128>(127, 123)};
}
#endif

static BenchResult bench_cmp_boost_cpp_u128()
{
    return {"boost::cpp_int u128", cmp_mide<boost_cpp_u128>(127, 123)};
}

static BenchResult bench_cmp_boost_chk_u128()
{
    return {"boost::checked_uint128", cmp_mide<boost_checked_u128>(127, 123)};
}

#ifdef BENCH_HAS_GMP_TOMMATH
static BenchResult bench_cmp_boost_gmp() { return {"boost::gmp_int", cmp_mide<boost_gmp_int>(127, 123)}; }

static BenchResult bench_cmp_boost_tom() { return {"boost::tom_int", cmp_mide<boost_tom_int>(127, 123)}; }
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
