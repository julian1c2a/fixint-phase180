// =============================================================================
// Benchmark Infrastructure: RDTSC Cycle Measurement & Anti-Optimization
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Shared infrastructure for all benchmarks in the int128 library.
// Provides: rdtsc(), CycleTimer, doNotOptimize<T>(), BenchResult, print helpers.
//
// Usage:
//   #include "bench_common.hpp"
//
// All benchmarks MUST use this header instead of defining their own
// timing infrastructure. See docs/PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md.
//
// =============================================================================

#ifndef BENCH_COMMON_HPP
#define BENCH_COMMON_HPP

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>

// ============================================================================
// Cycle/time counter — architecture-aware
//   x86_64:  RDTSC (cycle-accurate)
//   ARM64:   CNTVCT_EL0 virtual counter (sub-nanosecond, ~1-50 MHz freq)
//   RISC-V:  rdtime CSR (platform timer, comparable to ARM64 virtual counter)
//   Fallback: std::chrono nanoseconds cast to uint64_t
// ============================================================================

#if defined(_MSC_VER) || (defined(__INTEL_LLVM_COMPILER) && defined(_WIN32))
#include <intrin.h>
static inline std::uint64_t rdtsc() { return __rdtsc(); }
#elif defined(__INTEL_LLVM_COMPILER)
#include <x86intrin.h>
static inline std::uint64_t rdtsc() { return __rdtsc(); }
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
static inline std::uint64_t rdtsc() { return __builtin_ia32_rdtsc(); }
#elif defined(__aarch64__) || defined(_M_ARM64)
static inline std::uint64_t rdtsc()
{
    std::uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
#elif defined(__riscv)
static inline std::uint64_t rdtsc()
{
    std::uint64_t val;
    __asm__ volatile("rdtime %0" : "=r"(val));
    return val;
}
#else
#include <chrono>
static inline std::uint64_t rdtsc()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          std::chrono::steady_clock::now().time_since_epoch())
                                          .count());
}
#endif

// ============================================================================
// Configuration defaults (overridable via -D at compile time)
// ============================================================================

#ifndef BENCH_ITERATIONS
#define BENCH_ITERATIONS 5000000
#endif

#ifndef BENCH_WARMUP
#define BENCH_WARMUP 10000
#endif

static constexpr std::size_t ITERATIONS{BENCH_ITERATIONS};
static constexpr std::size_t WARMUP{BENCH_WARMUP};

// ============================================================================
// Cycle counter (RDTSC-based, clock-frequency independent)
// ============================================================================

class CycleTimer
{
    std::uint64_t start_;

public:
    CycleTimer() : start_{rdtsc()} {}

    void reset() { start_ = rdtsc(); }

    std::uint64_t elapsed_cycles() const { return rdtsc() - start_; }
};

// ============================================================================
// Prevent optimization (volatile sink)
// ============================================================================
//
// Compiler-specific implementations to prevent dead code elimination
// without introducing unnecessary memory traffic or register shuffling.

template <typename T>
static void doNotOptimize(T &val)
{
#if defined(_MSC_VER) || defined(__INTEL_LLVM_COMPILER)
    // MSVC/Intel-Windows: read+write one byte through volatile pointer.
    // Forces the compiler to actually compute val (can't be eliminated).
    *reinterpret_cast<char volatile *>(&val) = *reinterpret_cast<char volatile *>(&val);
#elif defined(__clang__)
    // Clang: "+r,m" works well -- Clang chooses memory operands for structs,
    // which avoids register shuffling.
    asm volatile("" : "+r,m"(val) : : "memory");
#else
    // GCC: For 128-bit structs, "+r,m" causes 4 unnecessary register
    // shuffles per iteration (GCC can't map a struct to a register pair).
    // Fix: split into two separate "+r" constraints on each uint64_t half.
    // This generates the same optimal addq/adcq or subq/sbbq as __int128.
    // NOTE: No "memory" clobber for 16-byte types — the "+r" constraints
    // already prevent dead-code elimination. The "memory" clobber causes
    // GCC to spill structs to stack (but not __int128 register pairs),
    // creating a measurement artifact.
    if constexpr (sizeof(T) == 16 && alignof(T) >= alignof(std::uint64_t))
    {
        auto *p = reinterpret_cast<std::uint64_t *>(&val);
        asm volatile("" : "+r"(p[0]), "+r"(p[1]));
    }
    else
    {
        asm volatile("" : "+r,m"(val) : : "memory");
    }
#endif
}

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
    std::cout << "| " << std::left << std::setw(29) << r.name << " | " << std::right << std::fixed
              << std::setprecision(2) << std::setw(12) << r.cycles_per_op << " | " << std::fixed
              << std::setprecision(2) << std::setw(6) << ratio << "x   |\n";
}

static void print_footer() { print_separator(); }

#endif // BENCH_COMMON_HPP
