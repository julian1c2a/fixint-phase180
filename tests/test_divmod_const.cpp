// =============================================================================
// Test: Division/Modulo/Mul by Compile-Time Constants (Granlund-Montgomery)
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================
//
// Tests div<D>(), mod<D>(), divmod_const<D>(), mul<K>() member functions.
// Uses sweep_unary framework for systematic 3-region coverage.
//
// Correctness reference: standard operator/ and operator% (D_knuth_divrem).
//
// Compile (GCC):
//   g++ -std=c++20 -O2 -Iinclude tests/test_divmod_const.cpp -o build_temp/test_divmod_const
// Compile (Clang):
//   clang++ -std=c++20 -O2 -Iinclude -fconstexpr-steps=100000000 \
//           tests/test_divmod_const.cpp -o build_temp/test_divmod_const
// =============================================================================

#include "int128_parameterized.hpp"
#include "test_sweep_framework.hpp"

#include <iostream>
#include <cstdint>

using namespace nstd;
using std::uint64_t;

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Granlund-Montgomery div<D>/mod<D>/divmod_const<D>/mul<K> Tests\n";
    std::cout << "  Region size: 2^" << SWEEP_REGION_BITS
              << " = " << SWEEP_REGION_SIZE << " values per region\n";
    std::cout << "====================================================================\n\n";

    int total{0};
    int passed{0};

    // ========================================================================
    // Section 1: div<D> matches standard n / uint128_t{D}
    // ========================================================================
    std::cout << "--- Section 1: div<D> vs standard division ---\n";

    // Helper macro to reduce boilerplate for sweep tests
#define SWEEP_DIV(D_VAL)                                       \
    do                                                         \
    {                                                          \
        const auto d128{uint128_t{D_VAL}};                     \
        const bool ok{sweep_unary(                             \
            [](const uint128_t &n) { return n.div<D_VAL>(); }, \
            [&](const uint128_t &n) { return n / d128; },      \
            "div<" #D_VAL ">")};                               \
        ++total;                                               \
        if (ok)                                                \
        {                                                      \
            ++passed;                                          \
        }                                                      \
    } while (false)

    SWEEP_DIV(3);
    SWEEP_DIV(5);
    SWEEP_DIV(7);
    SWEEP_DIV(9);
    SWEEP_DIV(10);
    SWEEP_DIV(100);
    SWEEP_DIV(1000);
    SWEEP_DIV(65537);

    // Power-of-2 divisors (shift path)
    SWEEP_DIV(2);
    SWEEP_DIV(4);
    SWEEP_DIV(16);
    SWEEP_DIV(256);
    SWEEP_DIV(1024);

    // D=1 (identity path)
    SWEEP_DIV(1);

    std::cout << "\n";

    // ========================================================================
    // Section 2: mod<D> matches standard n % uint128_t{D}
    // ========================================================================
    std::cout << "--- Section 2: mod<D> vs standard modulo ---\n";

#define SWEEP_MOD(D_VAL)                                       \
    do                                                         \
    {                                                          \
        const auto d128{uint128_t{D_VAL}};                     \
        const bool ok{sweep_unary(                             \
            [](const uint128_t &n) { return n.mod<D_VAL>(); }, \
            [&](const uint128_t &n) { return n % d128; },      \
            "mod<" #D_VAL ">")};                               \
        ++total;                                               \
        if (ok)                                                \
        {                                                      \
            ++passed;                                          \
        }                                                      \
    } while (false)

    SWEEP_MOD(3);
    SWEEP_MOD(5);
    SWEEP_MOD(7);
    SWEEP_MOD(9);
    SWEEP_MOD(10);
    SWEEP_MOD(100);
    SWEEP_MOD(1000);
    SWEEP_MOD(65537);

    // Power-of-2 modulo
    SWEEP_MOD(2);
    SWEEP_MOD(4);
    SWEEP_MOD(16);
    SWEEP_MOD(256);
    SWEEP_MOD(1024);

    SWEEP_MOD(1);

    std::cout << "\n";

    // ========================================================================
    // Section 3: divmod_const<D> consistency (q * D + r == n)
    // ========================================================================
    std::cout << "--- Section 3: divmod_const<D> consistency ---\n";

#define SWEEP_DIVMOD(D_VAL)                         \
    do                                              \
    {                                               \
        const bool ok{sweep_unary(                  \
            [](const uint128_t &n) {                                          \
                const auto [q, r]{n.divmod_const<D_VAL>()};                   \
                return q * uint128_t{D_VAL} + r == n; },             \
            [](const uint128_t &) { return true; }, \
            "divmod_const<" #D_VAL ">")};           \
        ++total;                                    \
        if (ok)                                     \
        {                                           \
            ++passed;                               \
        }                                           \
    } while (false)

    SWEEP_DIVMOD(3);
    SWEEP_DIVMOD(5);
    SWEEP_DIVMOD(7);
    SWEEP_DIVMOD(9);
    SWEEP_DIVMOD(10);
    SWEEP_DIVMOD(100);
    SWEEP_DIVMOD(1000);
    SWEEP_DIVMOD(65537);
    SWEEP_DIVMOD(2);
    SWEEP_DIVMOD(16);
    SWEEP_DIVMOD(1024);

    std::cout << "\n";

    // ========================================================================
    // Section 4: mul<K> matches standard n * uint128_t{K}
    // ========================================================================
    std::cout << "--- Section 4: mul<K> vs standard multiplication ---\n";

#define SWEEP_MUL(K_VAL)                                       \
    do                                                         \
    {                                                          \
        const auto k128{uint128_t{K_VAL}};                     \
        const bool ok{sweep_unary(                             \
            [](const uint128_t &n) { return n.mul<K_VAL>(); }, \
            [&](const uint128_t &n) { return n * k128; },      \
            "mul<" #K_VAL ">")};                               \
        ++total;                                               \
        if (ok)                                                \
        {                                                      \
            ++passed;                                          \
        }                                                      \
    } while (false)

    SWEEP_MUL(0);
    SWEEP_MUL(1);
    SWEEP_MUL(2);
    SWEEP_MUL(3);
    SWEEP_MUL(5);
    SWEEP_MUL(7);
    SWEEP_MUL(9);
    SWEEP_MUL(10);
    SWEEP_MUL(100);
    SWEEP_MUL(1000);
    SWEEP_MUL(65536);
    SWEEP_MUL(65537);

    std::cout << "\n";

    // ========================================================================
    // Section 5: Signed div<D> and mod<D>
    // ========================================================================
    std::cout << "--- Section 5: Signed division and modulo ---\n";

    {
        // Test signed by sweeping small values -10000..+10000 for each divisor
        constexpr uint64_t DIVISORS[] = {3, 5, 7, 9, 10, 100, 1000};
        constexpr int HALF{10000};

        int s_total{0};
        int s_passed{0};

        // Helper: for each divisor, sweep int128_t values and compare with native int64_t
        auto test_signed_div = [&](auto div_fn, auto ref_fn, const char *name)
        {
            bool ok{true};
            for (int v{-HALF}; v <= HALF; ++v)
            {
                const int128_t n{static_cast<int64_t>(v)};
                const auto got{div_fn(n)};
                const auto expected{ref_fn(v)};
                if (got != expected)
                {
                    if (ok)
                    {
                        std::cout << "  [FAIL] " << name << " at v=" << v << "\n";
                    }
                    ok = false;
                }
            }
            ++s_total;
            if (ok)
            {
                std::cout << "  [OK]   " << name << "\n";
                ++s_passed;
            }
        };

        // d=3 signed
        test_signed_div(
            [](const int128_t &n)
            { return n.div<3>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v / 3)}; },
            "signed div<3>");

        test_signed_div(
            [](const int128_t &n)
            { return n.mod<3>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v % 3)}; },
            "signed mod<3>");

        // d=5 signed
        test_signed_div(
            [](const int128_t &n)
            { return n.div<5>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v / 5)}; },
            "signed div<5>");

        test_signed_div(
            [](const int128_t &n)
            { return n.mod<5>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v % 5)}; },
            "signed mod<5>");

        // d=7 signed
        test_signed_div(
            [](const int128_t &n)
            { return n.div<7>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v / 7)}; },
            "signed div<7>");

        test_signed_div(
            [](const int128_t &n)
            { return n.mod<7>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v % 7)}; },
            "signed mod<7>");

        // d=10 signed
        test_signed_div(
            [](const int128_t &n)
            { return n.div<10>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v / 10)}; },
            "signed div<10>");

        test_signed_div(
            [](const int128_t &n)
            { return n.mod<10>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v % 10)}; },
            "signed mod<10>");

        // d=100 signed
        test_signed_div(
            [](const int128_t &n)
            { return n.div<100>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v / 100)}; },
            "signed div<100>");

        test_signed_div(
            [](const int128_t &n)
            { return n.mod<100>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v % 100)}; },
            "signed mod<100>");

        // Signed mul<K>
        test_signed_div(
            [](const int128_t &n)
            { return n.mul<10>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v * 10)}; },
            "signed mul<10>");

        test_signed_div(
            [](const int128_t &n)
            { return n.mul<7>(); },
            [](int v)
            { return int128_t{static_cast<int64_t>(v * 7)}; },
            "signed mul<7>");

        // Signed divmod_const roundtrip
        {
            bool ok{true};
            for (int v{-HALF}; v <= HALF; ++v)
            {
                if (v == 0)
                {
                    continue;
                }
                const int128_t n{static_cast<int64_t>(v)};
                const auto [q, r]{n.divmod_const<7>()};
                if (q * int128_t{7} + r != n)
                {
                    ok = false;
                    break;
                }
            }
            ++s_total;
            if (ok)
            {
                std::cout << "  [OK]   signed divmod_const<7> roundtrip\n";
                ++s_passed;
            }
            else
            {
                std::cout << "  [FAIL] signed divmod_const<7> roundtrip\n";
            }
        }

        std::cout << "  Signed sub-total: " << s_passed << "/" << s_total << "\n";
        total += s_total;
        passed += s_passed;
    }

    std::cout << "\n";

    // ========================================================================
    // Section 6: Static assertions (compile-time verification)
    // ========================================================================
    std::cout << "--- Section 6: Static assertions (compile-time) ---\n";

    // Unsigned
    static_assert(uint128_t{100}.div<10>() == uint128_t{10});
    static_assert(uint128_t{100}.mod<10>() == uint128_t{0});
    static_assert(uint128_t{17}.div<5>() == uint128_t{3});
    static_assert(uint128_t{17}.mod<5>() == uint128_t{2});
    static_assert(uint128_t{0}.div<7>() == uint128_t{0});
    static_assert(uint128_t{1}.div<1>() == uint128_t{1});
    static_assert(uint128_t{255}.div<16>() == uint128_t{15});
    static_assert(uint128_t{255}.mod<16>() == uint128_t{15});
    static_assert(uint128_t{1024}.div<1024>() == uint128_t{1});
    static_assert(uint128_t{1024}.mod<1024>() == uint128_t{0});

    // mul
    static_assert(uint128_t{42}.mul<10>() == uint128_t{420});
    static_assert(uint128_t{6}.mul<7>() == uint128_t{42});
    static_assert(uint128_t{100}.mul<0>() == uint128_t{0});
    static_assert(uint128_t{100}.mul<1>() == uint128_t{100});
    static_assert(uint128_t{3}.mul<256>() == uint128_t{768});

    // divmod_const
    static_assert(uint128_t{17}.divmod_const<5>().first == uint128_t{3});
    static_assert(uint128_t{17}.divmod_const<5>().second == uint128_t{2});

    // Signed
    static_assert(int128_t{static_cast<int64_t>(-17)}.div<5>() == int128_t{static_cast<int64_t>(-3)});
    static_assert(int128_t{static_cast<int64_t>(-17)}.mod<5>() == int128_t{static_cast<int64_t>(-2)});
    static_assert(int128_t{static_cast<int64_t>(17)}.div<5>() == int128_t{static_cast<int64_t>(3)});
    static_assert(int128_t{static_cast<int64_t>(-100)}.mul<10>() == int128_t{static_cast<int64_t>(-1000)});

    std::cout << "  [OK]   All static_assert passed (compile-time verified)\n";
    ++total;
    ++passed;

    // ========================================================================
    // Section 7: Large divisors (D > GM_TABLE range)
    // ========================================================================
    std::cout << "\n--- Section 7: Large divisors (D > 1023, computed on-the-fly) ---\n";

    SWEEP_DIV(10000);
    SWEEP_DIV(1000000007);
    SWEEP_MOD(10000);
    SWEEP_MOD(1000000007);

    // 10^19 (used by to_string)
    {
        constexpr uint64_t E19{10000000000000000000ULL};
        const auto e19_128{uint128_t{E19}};
        const bool ok{sweep_unary(
            [](const uint128_t &n)
            { return n.div<10000000000000000000ULL>(); },
            [&](const uint128_t &n)
            { return n / e19_128; },
            "div<1e19>")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }
    {
        const bool ok{sweep_unary(
            [](const uint128_t &n)
            {
                constexpr uint64_t E19{10000000000000000000ULL};
                const auto [q, r]{n.divmod_const<E19>()};
                return q * uint128_t{E19} + r == n;
            },
            [](const uint128_t &)
            { return true; },
            "divmod_const<1e19> roundtrip")};
        ++total;
        if (ok)
        {
            ++passed;
        }
    }

    std::cout << "\n";

    // ========================================================================
    // Summary
    // ========================================================================
    print_sweep_summary(passed, total);

#undef SWEEP_DIV
#undef SWEEP_MOD
#undef SWEEP_DIVMOD
#undef SWEEP_MUL

    return (passed == total) ? 0 : 1;
}
