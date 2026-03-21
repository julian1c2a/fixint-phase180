// =============================================================================
// Test: Granlund-Montgomery division by constants (128-bit)
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

int main() {
    std::cout << "=== Granlund-Montgomery Division Tests ===" << std::endl;
    std::cout << std::endl;

    int total{0};
    int passed{0};

    const auto ten{uint128_t{10}};

    // =========================================================================
    // Section 1: fast_div10 vs Standard Division
    // =========================================================================
    std::cout << "--- Section 1: fast_div10 vs Standard Division ---" << std::endl;

    {
        const bool ok{sweep_unary(
            [&](const uint128_t& n) { return fast_div10(n); },
            [&](const uint128_t& n) { return n / ten; },
            "fast_div10_matches_standard"
        )};
        ++total;
        if (ok) { ++passed; }
    }

    // =========================================================================
    // Section 2: fast_mod10 vs Standard Modulo
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 2: fast_mod10 vs Standard Modulo ---" << std::endl;

    {
        const bool ok{sweep_unary(
            [&](const uint128_t& n) -> uint64_t { return fast_mod10(n); },
            [&](const uint128_t& n) -> uint64_t { return (n % ten).low(); },
            "fast_mod10_matches_standard"
        )};
        ++total;
        if (ok) { ++passed; }
    }

    // =========================================================================
    // Section 3: Divmod Consistency (q*10 + r == n)
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 3: Divmod Consistency ---" << std::endl;

    {
        const bool ok{sweep_unary(
            [&](const uint128_t& n) {
                const auto [q, r]{fast_divmod10(n)};
                return q * ten + uint128_t{r} == n;
            },
            [](const uint128_t&) { return true; },
            "divmod10_q*10+r_equals_n"
        )};
        ++total;
        if (ok) { ++passed; }
    }

    // =========================================================================
    // Section 4: mulhi_128 Cross-validation
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 4: mulhi_128 Cross-validation ---" << std::endl;

    {
        const bool ok{sweep_binary(
            [](const uint128_t& a, const uint128_t& b) {
                return mulhi_128(a, b);
            },
            [](const uint128_t& a, const uint128_t& b) {
                return karatsuba_full_mul(a, b).high128();
            },
            "mulhi128_matches_karatsuba"
        )};
        ++total;
        if (ok) { ++passed; }
    }

    // =========================================================================
    // Section 5: Known Values
    // =========================================================================
    std::cout << std::endl;
    std::cout << "--- Section 5: Known Values ---" << std::endl;

    constexpr uint64_t MAX64{0xFFFFFFFFFFFFFFFFull};

    // Zero
    {
        const bool ok{fast_div10(uint128_t{0}) == uint128_t{0}
            && fast_mod10(uint128_t{0}) == 0};
        std::cout << (ok ? "[OK]" : "[FAIL]") << " div10(0) = 0, mod10(0) = 0" << std::endl;
        ++total;
        if (ok) { ++passed; }
    }

    // Small values
    {
        bool ok{true};
        for (uint64_t i{0}; i < 100; ++i) {
            const auto n{uint128_t{i}};
            if (fast_div10(n) != uint128_t{i / 10}) { ok = false; }
            if (fast_mod10(n) != (i % 10)) { ok = false; }
        }
        std::cout << (ok ? "[OK]" : "[FAIL]") << " div10/mod10 for 0..99" << std::endl;
        ++total;
        if (ok) { ++passed; }
    }

    // MAX128
    {
        const auto max128{uint128_t{MAX64, MAX64}};
        const auto [q, r]{fast_divmod10(max128)};
        const bool ok{q * ten + uint128_t{r} == max128 && r < 10};
        std::cout << (ok ? "[OK]" : "[FAIL]") << " divmod10(MAX128) consistency" << std::endl;
        ++total;
        if (ok) { ++passed; }
    }

    // Powers of 10
    {
        bool ok{true};
        auto pow10{uint128_t{1}};
        for (int exp{0}; exp < 38; ++exp) {
            const auto [q, r]{fast_divmod10(pow10)};
            if (exp == 0) {
                if (q != uint128_t{0} || r != 1) { ok = false; }
            } else {
                if (r != 0) { ok = false; }
                if (q * ten != pow10) { ok = false; }
            }
            pow10 = pow10 * ten;
        }
        std::cout << (ok ? "[OK]" : "[FAIL]") << " divmod10 for 10^0..10^37" << std::endl;
        ++total;
        if (ok) { ++passed; }
    }

    // =========================================================================
    // Summary
    // =========================================================================
    print_sweep_summary(passed, total);

    return (passed == total) ? 0 : 1;
}
