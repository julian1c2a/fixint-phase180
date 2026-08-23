// =============================================================================
// Benchmark: Granlund-Montgomery fast division vs operator/
// Part of int128 Library
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "algorithms/div_by_const.hpp"
#include "bench_common.hpp"

#include <cstdint>

using namespace nstd;
using namespace nstd::algorithms;

using std::uint64_t;

// ============================================================================
// Generic benchmark: measure cycles/op for a unary operation
// ============================================================================
template <typename F>
static BenchResult measure(const char *name, F &&func)
{
    // Generate test values (deterministic PRNG)
    constexpr std::size_t N{1024};
    uint128_t values[N];
    {
        uint64_t state{0x123456789ABCDEFull};
        for (std::size_t i{0}; i < N; ++i)
        {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            const uint64_t lo{state};
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            const uint64_t hi{state};
            values[i] = uint128_t{hi, lo};
        }
    }

    // Warmup
    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto v{func(values[i & (N - 1)])};
        doNotOptimize(v);
    }

    // Measure
    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto v{func(values[i & (N - 1)])};
        doNotOptimize(v);
    }
    const uint64_t cycles{timer.elapsed_cycles()};
    const double cyc_per_op{static_cast<double>(cycles) / ITERATIONS};

    return BenchResult{name, cyc_per_op};
}

// ============================================================================
// Baseline: uint64_t division
// ============================================================================
template <uint64_t D>
static BenchResult measure_u64_div()
{
    constexpr std::size_t N{1024};
    uint64_t values[N];
    {
        uint64_t state{0x123456789ABCDEFull};
        for (std::size_t i{0}; i < N; ++i)
        {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            values[i] = state;
        }
    }

    for (std::size_t i{0}; i < WARMUP; ++i)
    {
        auto v{values[i & (N - 1)] / D};
        doNotOptimize(v);
    }

    CycleTimer timer;
    for (std::size_t i{0}; i < ITERATIONS; ++i)
    {
        auto v{values[i & (N - 1)] / D};
        doNotOptimize(v);
    }
    const uint64_t cycles{timer.elapsed_cycles()};
    return BenchResult{"uint64_t / const", static_cast<double>(cycles) / ITERATIONS};
}

int main()
{
    std::cout << "============================================================" << std::endl;
    std::cout << "  Granlund-Montgomery Benchmark: fast_divN vs operator/" << std::endl;
    std::cout << "  Iterations: " << ITERATIONS << "  Warmup: " << WARMUP << std::endl;
    std::cout << "============================================================" << std::endl;

    // ====================================================================
    // Division by 10
    // ====================================================================
    {
        const auto baseline{measure_u64_div<10>()};
        const auto gm{measure("fast_div10(uint128)", [](const uint128_t &n) { return fast_div10(n); })};
        const auto std_div{
            measure("uint128 / uint128(10)", [](const uint128_t &n) { return n / uint128_t{10}; })};

        print_header("Division by 10");
        print_result(baseline, baseline.cycles_per_op);
        print_result(gm, baseline.cycles_per_op);
        print_result(std_div, baseline.cycles_per_op);
        print_footer();
    }

    // ====================================================================
    // Division by 3
    // ====================================================================
    {
        const auto baseline{measure_u64_div<3>()};
        const auto gm{measure("fast_div3(uint128)", [](const uint128_t &n) { return fast_div3(n); })};
        const auto std_div{
            measure("uint128 / uint128(3)", [](const uint128_t &n) { return n / uint128_t{3}; })};

        print_header("Division by 3");
        print_result(baseline, baseline.cycles_per_op);
        print_result(gm, baseline.cycles_per_op);
        print_result(std_div, baseline.cycles_per_op);
        print_footer();
    }

    // ====================================================================
    // Division by 5
    // ====================================================================
    {
        const auto baseline{measure_u64_div<5>()};
        const auto gm{measure("fast_div5(uint128)", [](const uint128_t &n) { return fast_div5(n); })};
        const auto std_div{
            measure("uint128 / uint128(5)", [](const uint128_t &n) { return n / uint128_t{5}; })};

        print_header("Division by 5");
        print_result(baseline, baseline.cycles_per_op);
        print_result(gm, baseline.cycles_per_op);
        print_result(std_div, baseline.cycles_per_op);
        print_footer();
    }

    // ====================================================================
    // Division by 7
    // ====================================================================
    {
        const auto baseline{measure_u64_div<7>()};
        const auto gm{measure("fast_div7(uint128)", [](const uint128_t &n) { return fast_div7(n); })};
        const auto std_div{
            measure("uint128 / uint128(7)", [](const uint128_t &n) { return n / uint128_t{7}; })};

        print_header("Division by 7");
        print_result(baseline, baseline.cycles_per_op);
        print_result(gm, baseline.cycles_per_op);
        print_result(std_div, baseline.cycles_per_op);
        print_footer();
    }

    // ====================================================================
    // Division by 100
    // ====================================================================
    {
        const auto baseline{measure_u64_div<100>()};
        const auto gm{measure("fast_div100(uint128)", [](const uint128_t &n) { return fast_div100(n); })};
        const auto std_div{
            measure("uint128 / uint128(100)", [](const uint128_t &n) { return n / uint128_t{100}; })};

        print_header("Division by 100");
        print_result(baseline, baseline.cycles_per_op);
        print_result(gm, baseline.cycles_per_op);
        print_result(std_div, baseline.cycles_per_op);
        print_footer();
    }

    // ====================================================================
    // Division by 10^19
    // ====================================================================
    {
        constexpr uint64_t E19{10000000000000000000ull};
        const auto baseline{measure_u64_div<E19>()};
        const auto gm{measure("fast_div_1e19(uint128)", [](const uint128_t &n) { return fast_div_1e19(n); })};
        const auto std_div{
            measure("uint128 / uint128(1e19)", [&](const uint128_t &n) { return n / uint128_t{E19}; })};

        print_header("Division by 10^19");
        print_result(baseline, baseline.cycles_per_op);
        print_result(gm, baseline.cycles_per_op);
        print_result(std_div, baseline.cycles_per_op);
        print_footer();
    }

    return 0;
}
