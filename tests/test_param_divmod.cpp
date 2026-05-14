// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Division — operators, divmod(), Knuth D, Granlund-Montgomery constants
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
//
// Consolidates: test_division_operators, test_divmod_suite, test_divmod_final,
//               test_knuth_d_correctness, test_knuth_vs_binary,
//               test_divmod_const, test_div_by_const, test_div_by_const_extended
// =============================================================================

#include "int128_parameterized.hpp"
#include "algorithms/div_by_const.hpp"
#include "test_sweep_framework.hpp"

#include <iostream>
#include <iomanip>
#include <cstdint>

using namespace nstd;
using namespace nstd::algorithms;
using std::uint64_t;

using int128_tc_t = nstd::int128_param_t<nstd::signedness::signed_type,
                                         nstd::representation_form::twos_complement>;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                                    \
    do {                                                    \
        if (cond) { std::cout << "[OK] " << (name) << "\n"; ++g_passed; } \
        else      { std::cout << "[FAIL] " << (name) << "\n"; ++g_failed; } \
    } while (false)

// =============================================================================
// Section 1: Operators /, %, /=, %= — unsigned
// =============================================================================
static void test_operators_unsigned()
{
    std::cout << "\n--- Section 1: Operators /  %  /=  %= (unsigned) ---\n";

    // /= basic
    { uint128_t a{0,100}, b{0,10}; a /= b; TEST("100 /= 10 = 10", a.low() == 10); }
    { uint128_t a{0,128}, b{0,8};  a /= b; TEST("128 /= 8  = 16", a.low() == 16); }
    { uint128_t a{0,42},  b{0,6};  a /= b; TEST("42  /= 6  = 7",  a.low() == 7);  }
    { uint128_t a{0,12345}, b{0,1}; a /= b; TEST("n  /= 1   = n",  a.low() == 12345); }

    // %= basic
    { uint128_t a{0,17}, b{0,5}; a %= b; TEST("17 %= 5 = 2",  a.low() == 2); }
    { uint128_t a{0,20}, b{0,5}; a %= b; TEST("20 %= 5 = 0",  a.low() == 0); }
    { uint128_t a{0,19}, b{0,5}; a %= b; TEST("19 %= 5 = 4",  a.low() == 4); }

    // non-modifying /
    {
        uint128_t a{0,100}, b{0,10};
        uint128_t r = a / b;
        TEST("100 / 10 = 10 (original unchanged)", a.low() == 100 && r.low() == 10);
    }
    {
        uint128_t a{0,1000};
        uint128_t r = (a / uint128_t{0,10}) / uint128_t{0,10};
        TEST("(1000 / 10) / 10 = 10", r.low() == 10);
    }

    // non-modifying %
    {
        uint128_t a{0,17}, b{0,5};
        uint128_t r = a % b;
        TEST("17 % 5 = 2 (original unchanged)", a.low() == 17 && r.low() == 2);
    }
    {
        uint128_t a{0,37}, b{0,7};
        TEST("(37/7)*7 + 37%7 == 37", ((a/b)*b + (a%b)).low() == 37);
    }

    // edge cases
    { uint128_t a{0,42}, b{0,42}; TEST("n / n = 1",    (a/b).low() == 1); }
    { uint128_t a{0,42}, b{0,42}; TEST("n % n = 0",    (a%b).low() == 0); }
    { uint128_t a{0,3},  b{0,10}; TEST("3 / 10 = 0, 3 % 10 = 3",
                                        (a/b).low() == 0 && (a%b).low() == 3); }

    // 128-bit range
    {
        uint128_t a{1, 0};             // 2^64
        uint128_t b{0, (1ULL << 32)};  // 2^32
        TEST("2^64 / 2^32 = 2^32", (a/b).low() == (1ULL << 32));
    }
}

// =============================================================================
// Section 2: Operators /= %= — signed TC
// =============================================================================
static void test_operators_signed_tc()
{
    std::cout << "\n--- Section 2: Operators /= %= (signed TC) ---\n";

    { int128_tc_t a{0,20}, b{0,4}; a /= b;
      TEST("(+20)/=(+4)=+5", a.low() == 5 && !a.is_negative()); }
    { int128_tc_t a{static_cast<int64_t>(-20)}, b{0,4}; a /= b;
      TEST("(-20)/=(+4) is negative", a.is_negative()); }
    { int128_tc_t a{0,20}, b{static_cast<int64_t>(-4)}; a /= b;
      TEST("(+20)/=(-4) is negative", a.is_negative()); }
    { int128_tc_t a{static_cast<int64_t>(-20)}, b{static_cast<int64_t>(-4)}; a /= b;
      TEST("(-20)/=(-4)=+5", a.low() == 5 && !a.is_negative()); }
    { int128_tc_t a{0,17}, b{0,5}; a %= b;
      TEST("(+17)%=(+5)=+2", a.low() == 2 && !a.is_negative()); }
}

// =============================================================================
// Section 3: divmod() — optimization levels
// =============================================================================
static void test_divmod_levels()
{
    std::cout << "\n--- Section 3: divmod() optimization levels ---\n";

    // Level 1: power-of-2 → shift
    {
        auto [q,r] = uint128_t{0, 0x8000000000000000ULL}.divmod(uint128_t{0,2});
        TEST("2^127 / 2 = 2^126 r=0",
             q.low() == 0x4000000000000000ULL && r.is_zero());
    }
    {
        auto [q,r] = uint128_t{0,256}.divmod(uint128_t{0,16});
        TEST("256 / 16 = 16 r=0", q.low() == 16 && r.is_zero());
    }

    // Level 3: both 64-bit → native
    {
        auto [q,r] = uint128_t{0,100}.divmod(uint128_t{0,7});
        TEST("100 / 7 = 14 r=2", q.low() == 14 && r.low() == 2);
    }
    {
        auto [q,r] = uint128_t{0,1000}.divmod(uint128_t{0,13});
        TEST("1000 / 13 = 76 r=12", q.low() == 76 && r.low() == 12);
    }

    // Level 4: 128-bit / 64-bit
    {
        uint128_t a{0x0000000000000001ULL, 0};  // 2^64
        uint128_t b{0, 0x100ULL};               // 2^8
        auto [q,r] = a.divmod(b);
        TEST("2^64 / 2^8 = 2^56", q.low() == 0x0100000000000000ULL);
    }
    {
        uint128_t a{0, 0x0DE0B6B3A7640000ULL};  // 10^18
        auto [q,r] = a.divmod(uint128_t{0,1000});
        TEST("10^18 / 1000 = 10^15", q.low() == 0x38D7EA4C68000ULL);
    }

    // Level 6: 128-bit / 128-bit
    {
        uint128_t a{0x0000010000000000ULL, 0};   // 2^100
        uint128_t b{0x0000000000000001ULL, 0};   // 2^64
        auto [q,r] = a.divmod(b);
        TEST("2^100 / 2^64 = 2^36", q.low() == 0x0000010000000000ULL);
    }

    // divmod vs separate operators
    {
        uint128_t a{0,37}, b{0,5};
        auto [q,r] = a.divmod(b);
        TEST("divmod quotient == /",   q.low() == (a/b).low());
        TEST("divmod remainder == %",  r.low() == (a%b).low());
    }

    // Signed TC reconstruction
    {
        const int128_tc_t neg100{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFF9CULL};
        const int128_tc_t seven{0,7};
        auto [q,r] = neg100.divmod(seven);
        TEST("-100 / 7 reconstruction", (q * seven + r) == neg100);
    }

    // Large reconstruction
    {
        const uint128_t large{0x0FFFFFFFFFFFFFFFULL, 0x0FFFFFFFFFFFFFFFULL};
        const uint128_t div{0, 0x00000000FFFFFFFFULL};
        auto [q,r] = large.divmod(div);
        TEST("large divmod reconstruction", (q * div + r) == large);
    }
}

// =============================================================================
// Section 4: Knuth Algorithm D correctness
// =============================================================================
static void test_knuth_correctness()
{
    std::cout << "\n--- Section 4: Knuth Algorithm D correctness ---\n";

    // 64/64
    {
        auto [q,r] = uint128_t{0,100}.divmod(uint128_t{0,7});
        TEST("64/64: 100/7 = 14 r=2", q.low() == 14 && r.low() == 2);
    }
    {
        auto [q,r] = uint128_t{0, 0xFFFFFFFFFFFFFFFFull}.divmod(uint128_t{0, 0xFFFFFFFFull});
        TEST("64/64: (2^64-1)/(2^32-1) = 2^32+1 r=0",
             q.low() == 0x100000001ull && r.is_zero());
    }

    // 128/64
    {
        auto [q,r] = uint128_t{1,0}.divmod(uint128_t{0,3});
        TEST("128/64: 2^64 / 3 = 6148914691236517205 r=1",
             q.low() == 6148914691236517205ull && q.high() == 0 && r.low() == 1);
    }
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        auto [q,r] = a.divmod(uint128_t{0,2});
        TEST("128/64: (2^128-1)/2",
             q.high() == 0x7FFFFFFFFFFFFFFFull && q.low() == 0xFFFFFFFFFFFFFFFFull && r.low() == 1);
    }
    {
        uint128_t a{0xABCDEF0123456789ull, 0x1234567890ABCDEFull};
        uint128_t b{0, 0xFEDCBA9876543210ull};
        auto [q,r] = a.divmod(b);
        TEST("128/64: large (reconstruction)", (q * b + r) == a);
    }

    // 128/128
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{1,1};
        auto [q,r] = a.divmod(b);
        TEST("128/128: (2^128-1)/(2^64+1) reconstruction", (q * b + r) == a);
    }
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{0x8000000000000000ull, 0};
        auto [q,r] = a.divmod(b);
        TEST("128/128: (2^128-1)/2^127 = 1 r=(2^127-1)",
             q.low() == 1 && q.high() == 0 &&
             r.high() == 0x7FFFFFFFFFFFFFFFull && r.low() == 0xFFFFFFFFFFFFFFFFull);
    }
    {
        uint128_t a{0x8000000000000000ull, 0x0000000000000005ull};
        uint128_t b{0x8000000000000000ull, 0x0000000000000001ull};
        auto [q,r] = a.divmod(b);
        TEST("128/128: near-equal q=1 r=4",
             q.low() == 1 && q.high() == 0 && r.low() == 4 && r.high() == 0);
    }
    {
        uint128_t a{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
        uint128_t b{0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFEull};
        auto [q,r] = a.divmod(b);
        TEST("128/128: (2^128-1)/(2^128-2) = 1 r=1",
             q.low() == 1 && q.high() == 0 && r.low() == 1 && r.high() == 0);
    }

    // power-of-2 fast path
    {
        uint128_t a{0x8000000000000000ull, 0}, b{0,4};
        auto [q,r] = a.divmod(b);
        TEST("pow2 fast path: 2^127 / 4 = 2^125",
             q.high() == 0x2000000000000000ull && q.low() == 0 && r.is_zero());
    }
    {
        uint128_t a{0xABCDEF0123456789ull, 0x1234567890ABCDEFull};
        uint128_t b{0,256};
        auto [q,r] = a.divmod(b);
        TEST("pow2 fast path: large / 256 reconstruction", (q * b + r) == a);
    }

    // edge cases
    { auto [q,r] = uint128_t{0,0}.divmod(uint128_t{0,42});
      TEST("0 / 42 = 0 r=0", q.is_zero() && r.is_zero()); }
    { auto [q,r] = uint128_t{0,42}.divmod(uint128_t{0,0});
      TEST("42 / 0 defined (returns 0)", q.is_zero() && r.is_zero()); }
    { auto [q,r] = uint128_t{0,5}.divmod(uint128_t{0,100});
      TEST("5 / 100 = 0 r=5", q.is_zero() && r.low() == 5); }
    { auto [q,r] = uint128_t{0,1}.divmod(uint128_t{0,1});
      TEST("1 / 1 = 1 r=0", q.low() == 1 && r.is_zero()); }
}

// =============================================================================
// Section 5: Knuth D == big_bin_divrem parity
// =============================================================================
static void test_knuth_vs_binary()
{
    std::cout << "\n--- Section 5: D_knuth_divrem == big_bin_divrem parity ---\n";

    struct Case { uint64_t ah, al, bh, bl; const char* name; };
    const Case cases[] = {
        {0,100,0,7,                                             "100/7"},
        {1,0,0,3,                                               "2^64/3"},
        {0xFFFFFFFFFFFFFFFFull,0xFFFFFFFFFFFFFFFFull,0,10,      "(2^128-1)/10"},
        {0xFFFFFFFFFFFFFFFFull,0xFFFFFFFFFFFFFFFFull,1,1,       "(2^128-1)/(2^64+1)"},
        {0x8000000000000000ull,0,0x4000000000000000ull,0,       "2^127/2^126"},
        {0xABCDEF0123456789ull,0x1234567890ABCDEFull,
         0x1234567890ABCDEFull,0xABCDEF0123456789ull,           "large/large"},
        {0xFFFFFFFFFFFFFFFFull,0xFFFFFFFFFFFFFFFFull,
         0xFFFFFFFFFFFFFFFFull,0xFFFFFFFFFFFFFFFEull,           "max/(max-1)"},
        {0x123ull,0x456ull,0,0x789ull,                          "small 128/64"},
        {0x8000000000000000ull,5,0x8000000000000000ull,1,       "near-equal"},
        {0,0xFFFFFFFFFFFFFFFFull,0,0xFFFFFFFFull,               "(2^64-1)/(2^32-1)"},
    };

    int matches{0};
    for (const auto& tc : cases)
    {
        uint128_t a{tc.ah, tc.al}, b{tc.bh, tc.bl};
        const auto [qk,rk] = a.D_knuth_divrem(b);
        const auto [qb,rb] = a.big_bin_divrem(b);
        if (qk == qb && rk == rb) { ++matches; }
        else std::cout << "[FAIL] mismatch on " << tc.name << "\n";
    }
    TEST("All 10 cases: Knuth D == big_bin_divrem", matches == 10);
}

// =============================================================================
// Section 6: GM member API — div<D>, mod<D>, divmod_const<D>, mul<K>
// =============================================================================
static void test_gm_member_api()
{
    std::cout << "\n--- Section 6: GM member API div<D>/mod<D>/divmod_const<D>/mul<K> ---\n";

    int total{0}, passed_local{0};

#define SWEEP_DIV(D_VAL) \
    do { \
        const auto d128{uint128_t{D_VAL}}; \
        const bool ok{sweep_unary( \
            [](const uint128_t& n){ return n.template div<D_VAL>(); }, \
            [&](const uint128_t& n){ return n / d128; }, \
            "div<" #D_VAL ">")}; \
        ++total; if (ok) ++passed_local; \
    } while (false)

#define SWEEP_MOD(D_VAL) \
    do { \
        const auto d128{uint128_t{D_VAL}}; \
        const bool ok{sweep_unary( \
            [](const uint128_t& n){ return n.template mod<D_VAL>(); }, \
            [&](const uint128_t& n){ return n % d128; }, \
            "mod<" #D_VAL ">")}; \
        ++total; if (ok) ++passed_local; \
    } while (false)

#define SWEEP_DIVMOD(D_VAL) \
    do { \
        const auto d128{uint128_t{D_VAL}}; \
        const bool ok{sweep_unary( \
            [&](const uint128_t& n){ \
                const auto [q,r] = n.template divmod_const<D_VAL>(); \
                return q * d128 + uint128_t{r} == n; }, \
            [](const uint128_t&){ return true; }, \
            "divmod_const<" #D_VAL "> consistency")}; \
        ++total; if (ok) ++passed_local; \
    } while (false)

    SWEEP_DIV(3);  SWEEP_MOD(3);  SWEEP_DIVMOD(3);
    SWEEP_DIV(5);  SWEEP_MOD(5);  SWEEP_DIVMOD(5);
    SWEEP_DIV(7);  SWEEP_MOD(7);  SWEEP_DIVMOD(7);
    SWEEP_DIV(9);  SWEEP_MOD(9);  SWEEP_DIVMOD(9);
    SWEEP_DIV(10); SWEEP_MOD(10); SWEEP_DIVMOD(10);
    SWEEP_DIV(100);SWEEP_MOD(100);SWEEP_DIVMOD(100);

    g_passed += passed_local;
    g_failed += (total - passed_local);

#undef SWEEP_DIV
#undef SWEEP_MOD
#undef SWEEP_DIVMOD
}

// =============================================================================
// Section 7: GM algorithm API — fast_div10/mod10 and d=3,5,7,9,100,10^19
// =============================================================================
static void test_gm_algorithm_api()
{
    std::cout << "\n--- Section 7: GM algorithm API (fast_div/mod functions) ---\n";

    constexpr uint64_t MAX64{0xFFFFFFFFFFFFFFFFull};
    const uint128_t max128{MAX64, MAX64};
    int total{0}, passed_local{0};

    // fast_div10 / fast_mod10 sweeps
    {
        const auto ten{uint128_t{10}};
        bool ok{true};
        ok &= sweep_unary([](const uint128_t& n){ return fast_div10(n); },
                          [&](const uint128_t& n){ return n / ten; },
                          "fast_div10_matches_standard");
        ok &= sweep_unary([](const uint128_t& n) -> uint64_t { return fast_mod10(n); },
                          [&](const uint128_t& n) -> uint64_t { return (n % ten).low(); },
                          "fast_mod10_matches_standard");
        ++total; if (ok) ++passed_local;
    }

    // divmod10 consistency
    {
        const auto ten{uint128_t{10}};
        const bool ok{sweep_unary(
            [&](const uint128_t& n){
                const auto [q,r] = fast_divmod10(n);
                return q * ten + uint128_t{r} == n;
            },
            [](const uint128_t&){ return true; },
            "divmod10 consistency q*10+r==n")};
        ++total; if (ok) ++passed_local;
    }

    // mulhi_128 cross-validation against karatsuba
    {
        const bool ok{sweep_binary(
            [](const uint128_t& a, const uint128_t& b){ return mulhi_128(a, b); },
            [](const uint128_t& a, const uint128_t& b){ return karatsuba_full_mul(a, b).high128(); },
            "mulhi_128 == karatsuba high128")};
        ++total; if (ok) ++passed_local;
    }

    // Known values: fast_div10/fast_mod10
    {
        bool ok{true};
        ok &= (fast_div10(uint128_t{0}) == uint128_t{0} && fast_mod10(uint128_t{0}) == 0);
        for (uint64_t i{0}; i < 100; ++i)
        {
            const uint128_t n{i};
            ok &= (fast_div10(n) == uint128_t{i/10});
            ok &= (fast_mod10(n) == (i % 10));
        }
        // MAX128 consistency
        const auto [q,r] = fast_divmod10(max128);
        ok &= (q * uint128_t{10} + uint128_t{r} == max128 && r < 10);
        ++total; if (ok) ++passed_local;
        std::cout << (ok ? "[OK]" : "[FAIL]") << " fast_div10/mod10 known values + MAX128\n";
    }

    // Helper for each constant divisor
    const auto test_one_divisor = [&](
        uint64_t D,
        uint128_t (*fdiv)(const uint128_t&),
        std::pair<uint128_t,uint64_t> (*fdivmod)(const uint128_t&),
        const char* label)
    {
        const uint128_t d128{D};
        bool ok{true};
        ok &= sweep_unary([=](const uint128_t& n){ return fdiv(n); },
                          [&](const uint128_t& n){ return n / d128; },
                          label);
        ok &= sweep_unary([=,&d128](const uint128_t& n){
            const auto [q,r] = fdivmod(n);
            return q * d128 + uint128_t{r} == n;
        }, [](const uint128_t&){ return true; }, label);
        ok &= (fdiv(uint128_t{0}) == uint128_t{0});
        const auto [q,r] = fdivmod(max128);
        ok &= (q * d128 + uint128_t{r} == max128 && r < D);
        ++total; if (ok) ++passed_local;
        std::cout << (ok ? "[OK]" : "[FAIL]") << " " << label << "\n";
    };

    test_one_divisor(3,   fast_div3,   fast_divmod3,   "div/divmod3");
    test_one_divisor(5,   fast_div5,   fast_divmod5,   "div/divmod5");
    test_one_divisor(7,   fast_div7,   fast_divmod7,   "div/divmod7");
    test_one_divisor(9,   fast_div9,   fast_divmod9,   "div/divmod9");
    test_one_divisor(100, fast_div100, fast_divmod100, "div/divmod100");

    // 10^19 (special API: remainder is uint64_t)
    {
        constexpr uint64_t E19{10000000000000000000ull};
        const uint128_t e19{E19};
        bool ok{true};
        ok &= sweep_unary([&](const uint128_t& n){ return fast_div_1e19(n); },
                          [&](const uint128_t& n){ return n / e19; },
                          "fast_div_1e19");
        ok &= sweep_unary([&](const uint128_t& n){
            const auto [q,r] = fast_divmod_1e19(n);
            return q * e19 + uint128_t{r} == n;
        }, [](const uint128_t&){ return true; }, "divmod_1e19 consistency");
        ok &= (fast_div_1e19(uint128_t{0}) == uint128_t{0});
        { const auto [q,r] = fast_divmod_1e19(max128);
          ok &= (q * e19 + uint128_t{r} == max128 && r < E19); }
        { const auto [q1,r1] = fast_divmod_1e19(uint128_t{E19-1});
          ok &= (q1.is_zero() && r1 == E19-1); }
        { const auto [q2,r2] = fast_divmod_1e19(uint128_t{E19});
          ok &= (q2 == uint128_t{1} && r2 == 0); }
        { const auto [q3,r3] = fast_divmod_1e19(uint128_t{E19+1});
          ok &= (q3 == uint128_t{1} && r3 == 1); }
        ++total; if (ok) ++passed_local;
        std::cout << (ok ? "[OK]" : "[FAIL]") << " fast_div/divmod 10^19\n";
    }

    g_passed += passed_local;
    g_failed += (total - passed_local);
}

// =============================================================================
// Main
// =============================================================================
int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Division Tests: operators, divmod, Knuth D, Granlund-Montgomery\n";
    std::cout << "====================================================================\n";

    test_operators_unsigned();
    test_operators_signed_tc();
    test_divmod_levels();
    test_knuth_correctness();
    test_knuth_vs_binary();
    test_gm_member_api();
    test_gm_algorithm_api();

    std::cout << "\n====================================================================\n";
    std::cout << "SUMMARY\n";
    std::cout << "====================================================================\n";
    std::cout << "Passed: " << g_passed << "\n";
    std::cout << "Failed: " << g_failed << "\n";
    std::cout << "Total:  " << (g_passed + g_failed) << "\n";
    std::cout << "====================================================================\n";

    if (g_failed == 0) std::cout << "[OK] ALL TESTS PASSED\n";
    else               std::cout << "[FAIL] " << g_failed << " TESTS FAILED\n";

    return g_failed > 0 ? 1 : 0;
}
