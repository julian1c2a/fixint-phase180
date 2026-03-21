// =============================================================================
// Test: Extended Granlund-Montgomery divisors (d=3,5,7,9,100,10^19)
// Part of int128 Library
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "algorithms/div_by_const.hpp"
#include "test_sweep_framework.hpp"

#include <iostream>
#include <cstdint>

using namespace nstd;
using namespace nstd::algorithms;

using std::uint64_t;

// Helper: test a divisor with sweep + known values
template <uint64_t D>
int test_divisor(
    uint128_t (*fast_div_fn)(const uint128_t &),
    uint64_t (*fast_mod_fn)(const uint128_t &),
    std::pair<uint128_t, uint64_t> (*fast_divmod_fn)(const uint128_t &),
    const char *label)
{
    const auto d128{uint128_t{D}};
    int total{0};
    int passed{0};

    std::cout << "--- " << label << " (d=" << D << ") ---" << std::endl;

    // Sweep: div matches standard
    {
        const bool ok{sweep_unary(
            [&](const uint128_t &n)
            { return fast_div_fn(n); },
            [&](const uint128_t &n)
            { return n / d128; },
            "fast_div_matches_standard")};
        ++total;
        if (ok) { ++passed; }
    }

    // Sweep: mod matches standard
    {
        const bool ok{sweep_unary(
            [&](const uint128_t &n) -> uint64_t
            { return fast_mod_fn(n); },
            [&](const uint128_t &n) -> uint64_t
            { return (n % d128).low(); },
            "fast_mod_matches_standard")};
        ++total;
        if (ok) { ++passed; }
    }

    // Sweep: divmod consistency q*d + r == n
    {
        const bool ok{sweep_unary(
            [&](const uint128_t &n)
            {
                const auto [q, r]{fast_divmod_fn(n)};
                return q * d128 + uint128_t{r} == n;
            },
            [](const uint128_t &)
            { return true; },
            "divmod_q*d+r_equals_n")};
        ++total;
        if (ok) { ++passed; }
    }

    // Known: zero
    {
        const bool ok{fast_div_fn(uint128_t{0}) == uint128_t{0} && fast_mod_fn(uint128_t{0}) == 0};
        std::cout << (ok ? "[OK]" : "[FAIL]") << " div(0)=0, mod(0)=0" << std::endl;
        ++total;
        if (ok) { ++passed; }
    }

    // Known: small values 0..D*100
    {
        bool ok{true};
        for (uint64_t i{0}; i < D * 100 && i < 10000; ++i)
        {
            const auto n{uint128_t{i}};
            if (fast_div_fn(n) != uint128_t{i / D}) { ok = false; }
            if (fast_mod_fn(n) != (i % D)) { ok = false; }
        }
        std::cout << (ok ? "[OK]" : "[FAIL]") << " div/mod for small values" << std::endl;
        ++total;
        if (ok) { ++passed; }
    }

    // Known: MAX128
    {
        constexpr uint64_t MAX64{0xFFFFFFFFFFFFFFFFull};
        const auto max128{uint128_t{MAX64, MAX64}};
        const auto [q, r]{fast_divmod_fn(max128)};
        const bool ok{q * d128 + uint128_t{r} == max128 && r < D};
        std::cout << (ok ? "[OK]" : "[FAIL]") << " divmod(MAX128) consistency" << std::endl;
        ++total;
        if (ok) { ++passed; }
    }

    std::cout << std::endl;
    return (passed == total) ? 0 : (total - passed);
}

int main()
{
    std::cout << "=== Extended Granlund-Montgomery Division Tests ===" << std::endl;
    std::cout << std::endl;

    int failures{0};
    int sections{0};

    // d=3
    failures += test_divisor<3>(fast_div3, fast_mod3, fast_divmod3, "Division by 3");
    ++sections;

    // d=5
    failures += test_divisor<5>(fast_div5, fast_mod5, fast_divmod5, "Division by 5");
    ++sections;

    // d=7
    failures += test_divisor<7>(fast_div7, fast_mod7, fast_divmod7, "Division by 7");
    ++sections;

    // d=9
    failures += test_divisor<9>(fast_div9, fast_mod9, fast_divmod9, "Division by 9");
    ++sections;

    // d=100
    failures += test_divisor<100>(fast_div100, fast_mod100, fast_divmod100, "Division by 100");
    ++sections;

    // =========================================================================
    // d=10^19 — special test (different API: divmod returns uint64_t remainder)
    // =========================================================================
    std::cout << "--- Division by 10^19 ---" << std::endl;
    {
        constexpr uint64_t E19{10000000000000000000ull};
        const auto e19_128{uint128_t{E19}};
        int total{0};
        int passed{0};

        // Sweep: div matches standard
        {
            const bool ok{sweep_unary(
                [&](const uint128_t &n)
                { return fast_div_1e19(n); },
                [&](const uint128_t &n)
                { return n / e19_128; },
                "fast_div_1e19_matches_standard")};
            ++total;
            if (ok) { ++passed; }
        }

        // Sweep: divmod consistency q*10^19 + r == n
        {
            const bool ok{sweep_unary(
                [&](const uint128_t &n)
                {
                    const auto [q, r]{fast_divmod_1e19(n)};
                    return q * e19_128 + uint128_t{r} == n;
                },
                [](const uint128_t &)
                { return true; },
                "divmod_1e19_consistency")};
            ++total;
            if (ok) { ++passed; }
        }

        // Known: zero
        {
            const bool ok{fast_div_1e19(uint128_t{0}) == uint128_t{0}};
            std::cout << (ok ? "[OK]" : "[FAIL]") << " div_1e19(0)=0" << std::endl;
            ++total;
            if (ok) { ++passed; }
        }

        // Known: MAX128
        {
            constexpr uint64_t MAX64{0xFFFFFFFFFFFFFFFFull};
            const auto max128{uint128_t{MAX64, MAX64}};
            const auto [q, r]{fast_divmod_1e19(max128)};
            const bool ok{q * e19_128 + uint128_t{r} == max128 && r < E19};
            std::cout << (ok ? "[OK]" : "[FAIL]") << " divmod_1e19(MAX128) consistency" << std::endl;
            ++total;
            if (ok) { ++passed; }
        }

        // Known: values just below/above 10^19
        {
            bool ok{true};
            const auto [q1, r1]{fast_divmod_1e19(uint128_t{E19 - 1})};
            if (q1 != uint128_t{0} || r1 != E19 - 1) { ok = false; }
            const auto [q2, r2]{fast_divmod_1e19(uint128_t{E19})};
            if (q2 != uint128_t{1} || r2 != 0) { ok = false; }
            const auto [q3, r3]{fast_divmod_1e19(uint128_t{E19 + 1})};
            if (q3 != uint128_t{1} || r3 != 1) { ok = false; }
            std::cout << (ok ? "[OK]" : "[FAIL]") << " boundary values around 10^19" << std::endl;
            ++total;
            if (ok) { ++passed; }
        }

        if (passed < total) { failures += (total - passed); }
    }
    ++sections;

    // =========================================================================
    // Summary
    // =========================================================================
    const int total_tests{sections * 6 + 5};  // 6 tests per divisor, 5 for 10^19
    const int total_passed{total_tests - failures};
    print_sweep_summary(total_passed, total_tests);

    return (failures == 0) ? 0 : 1;
}
