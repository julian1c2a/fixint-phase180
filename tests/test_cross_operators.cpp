// =============================================================================
// Test: cross-N and mixed-sign operators
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Covers compound assignments and free operators between:
//   1. uint_fixed_t<N> op= uint_fixed_t<M>  (N != M)  — widen then truncate
//   2. int_fixed_t<N>  op= int_fixed_t<M>   (N != M)  — widen then truncate
//   3. uint_fixed_t<N> op= int_fixed_t<M>   mixed-sign UAC
//   4. int_fixed_t<N>  op= uint_fixed_t<M>  mixed-sign UAC
//   5. int_fixed_t<N>  op  uint_fixed_t<M>  free ops (both orientations)
//   6. uint_fixed_t<N> op  uint_fixed_t<M>  free ops (N != M) -> uint_fixed_t<max(N,M)>
//   7. int_fixed_t<N>  op  int_fixed_t<M>   free ops (N != M) -> int_fixed_t<max(N,M)>
//
// UAC rule (mirrors C++ built-in):
//   int_fixed_t<N> op uint_fixed_t<M>:  N > M  -> int_fixed_t<N>
//                                        N <= M -> uint_fixed_t<M>
//   same-sign cross-N: result is always the wider type

#include "fixed_width_int_t.hpp"
#include "fixed_int_limits.hpp"   // needed for std::numeric_limits<fixed_int_t<...>>

#include <compare>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <type_traits>

using nstd::int_fixed_t;
using nstd::uint_fixed_t;

// =============================================================================
// Test framework
// =============================================================================

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                               \
    do                                                 \
    {                                                  \
        if (cond)                                      \
        {                                              \
            std::cout << "[OK]   " << (name) << "\n"; \
            ++g_passed;                                \
        }                                              \
        else                                           \
        {                                              \
            std::cout << "[FAIL] " << (name) << "\n"; \
            ++g_failed;                                \
        }                                              \
    } while (false)

// =============================================================================
// Section 1: uint_fixed_t<N> op= uint_fixed_t<M>  (N != M)
// Semantics: promote both to max(N,M), compute, truncate to N.
// Mirrors: unsigned short a; unsigned long long b; a += b;
// =============================================================================

static void test_cross_n_uint_compound()
{
    std::cout << "\n--- Section 1: uint cross-N compound assignments ---\n";

    using u1 = uint_fixed_t<1>;  // 64-bit
    using u2 = uint_fixed_t<2>;  // 128-bit

    // --- += ---
    {
        u1 a{10}; a += u2{3};
        TEST("u1{10} += u2{3}  == u1{13}", a == u1{std::uint64_t{13}});
    }
    {
        u2 a{10}; a += u1{3};
        TEST("u2{10} += u1{3}  == u2{13}", a == u2{std::uint64_t{13}});
    }
    // wrap-around (result truncated to N=1)
    {
        u1 a{std::uint64_t{UINT64_MAX}}; a += u2{std::uint64_t{1}};
        TEST("u1{max} += u2{1} == u1{0}  (wrap)", a == u1{std::uint64_t{0}});
    }

    // --- -= ---
    {
        u1 a{10}; a -= u2{3};
        TEST("u1{10} -= u2{3}  == u1{7}", a == u1{std::uint64_t{7}});
    }
    {
        u2 a{10}; a -= u1{3};
        TEST("u2{10} -= u1{3}  == u2{7}", a == u2{std::uint64_t{7}});
    }
    // borrow wraps in N=1
    {
        u1 a{0}; a -= u2{std::uint64_t{1}};
        TEST("u1{0}  -= u2{1}  == u1{max} (wrap)", a == u1{std::uint64_t{UINT64_MAX}});
    }

    // --- *= ---
    {
        u1 a{7}; a *= u2{6};
        TEST("u1{7}  *= u2{6}  == u1{42}", a == u1{std::uint64_t{42}});
    }
    {
        u2 a{7}; a *= u1{6};
        TEST("u2{7}  *= u1{6}  == u2{42}", a == u2{std::uint64_t{42}});
    }

    // --- /= ---
    {
        u1 a{42}; a /= u2{std::uint64_t{6}};
        TEST("u1{42} /= u2{6}  == u1{7}", a == u1{std::uint64_t{7}});
    }
    {
        u2 a{42}; a /= u1{std::uint64_t{6}};
        TEST("u2{42} /= u1{6}  == u2{7}", a == u2{std::uint64_t{7}});
    }

    // --- %= ---
    {
        u1 a{10}; a %= u2{std::uint64_t{3}};
        TEST("u1{10} %= u2{3}  == u1{1}", a == u1{std::uint64_t{1}});
    }
    {
        u2 a{10}; a %= u1{std::uint64_t{3}};
        TEST("u2{10} %= u1{3}  == u2{1}", a == u2{std::uint64_t{1}});
    }

    // --- &= ---
    {
        u1 a{0xFF}; a &= u2{std::uint64_t{0x0F}};
        TEST("u1{0xFF} &= u2{0x0F} == u1{0x0F}", a == u1{std::uint64_t{0x0F}});
    }
    {
        u2 a{0xFF}; a &= u1{std::uint64_t{0x0F}};
        TEST("u2{0xFF} &= u1{0x0F} == u2{0x0F}", a == u2{std::uint64_t{0x0F}});
    }

    // --- |= ---
    {
        u1 a{0xF0}; a |= u2{std::uint64_t{0x0F}};
        TEST("u1{0xF0} |= u2{0x0F} == u1{0xFF}", a == u1{std::uint64_t{0xFF}});
    }

    // --- ^= ---
    {
        u1 a{0xFF}; a ^= u2{std::uint64_t{0xFF}};
        TEST("u1{0xFF} ^= u2{0xFF} == u1{0}",   a == u1{std::uint64_t{0}});
    }
}

// =============================================================================
// Section 2: int_fixed_t<N> op= int_fixed_t<M>  (N != M)
// =============================================================================

static void test_cross_n_int_compound()
{
    std::cout << "\n--- Section 2: int cross-N compound assignments ---\n";

    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;

    // --- += ---
    {
        i1 a{10}; a += i2{3};
        TEST("i1{10}  += i2{3}  == i1{13}",  a == i1{13});
    }
    {
        i2 a{10}; a += i1{3};
        TEST("i2{10}  += i1{3}  == i2{13}",  a == i2{13});
    }
    {
        i1 a{-5}; a += i2{3};
        TEST("i1{-5}  += i2{3}  == i1{-2}", a == i1{-2});
    }
    {
        i2 a{-5}; a += i1{3};
        TEST("i2{-5}  += i1{3}  == i2{-2}", a == i2{-2});
    }

    // --- -= ---
    {
        i1 a{10}; a -= i2{3};
        TEST("i1{10}  -= i2{3}  == i1{7}",  a == i1{7});
    }
    {
        i1 a{3}; a -= i2{10};
        TEST("i1{3}   -= i2{10} == i1{-7}", a == i1{-7});
    }

    // --- *= ---
    {
        i1 a{-3}; a *= i2{4};
        TEST("i1{-3}  *= i2{4}  == i1{-12}", a == i1{-12});
    }
    {
        i2 a{-3}; a *= i1{4};
        TEST("i2{-3}  *= i1{4}  == i2{-12}", a == i2{-12});
    }

    // --- /= ---
    {
        i1 a{-12}; a /= i2{4};
        TEST("i1{-12} /= i2{4}  == i1{-3}", a == i1{-3});
    }
    {
        i2 a{-12}; a /= i1{4};
        TEST("i2{-12} /= i1{4}  == i2{-3}", a == i2{-3});
    }

    // --- %= ---
    {
        i1 a{10}; a %= i2{3};
        TEST("i1{10}  %= i2{3}  == i1{1}",  a == i1{1});
    }
    {
        i1 a{-10}; a %= i2{3};
        TEST("i1{-10} %= i2{3}  == i1{-1}", a == i1{-1});
    }
}

// =============================================================================
// Section 3: uint_fixed_t<N> op= int_fixed_t<M>  (mixed-sign)
// UAC: N >= M -> uint_fixed_t<N> wins; N < M -> int_fixed_t<M> wins.
// Result is always stored in the LHS (uint_fixed_t<N>), so signed result
// is reinterpreted as unsigned on assignment.
// =============================================================================

static void test_mixed_uint_lhs()
{
    std::cout << "\n--- Section 3: uint op= int (mixed-sign) ---\n";

    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;
    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;

    // N >= M: uint wins, int zero-extends (positive int -> same bit pattern)
    {
        u2 a{10}; a += i1{3};
        TEST("u2{10}  += i1{3}  == u2{13}  (uint wins)", a == u2{std::uint64_t{13}});
    }
    {
        u2 a{10}; a -= i1{3};
        TEST("u2{10}  -= i1{3}  == u2{7}   (uint wins)", a == u2{std::uint64_t{7}});
    }
    // Negative RHS sign-extends to uint, then addition is mod 2^128
    // u2{5} + i1{-3}: i1{-3} sign-extends to u2 which is 2^128-3
    // 5 + (2^128-3) mod 2^128 = 2^128+2 mod 2^128 = 2  -> wraps!
    // Actually: UAC says N>=M, so uint wins -> both treated as uint_fixed_t<2>.
    // int_fixed_t<1>{-3} converts to uint_fixed_t<2> via sign extension:
    // -3 in two's complement 64-bit = 0xFFFFFFFFFFFFFFFD, sign-extended to 128-bit
    // = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD, which as uint_fixed_t<2> is 2^128-3.
    // 5 + (2^128-3) = 2^128+2, mod 2^128 = 2.
    {
        u2 a{5}; a += i1{-3};
        TEST("u2{5}  += i1{-3} wraps to u2{2} (two's-complement)", a == u2{std::uint64_t{2}});
    }
    {
        u1 a{5}; a += i1{-3};
        TEST("u1{5}  += i1{-3} == u1{2}  (same sign, uint wins)", a == u1{std::uint64_t{2}});
    }

    // N < M: int wins -> compute as int_fixed_t<M>, then store as uint_fixed_t<N>
    {
        u1 a{10}; a += i2{3};
        TEST("u1{10}  += i2{3}  == u1{13}  (int wins, truncate to u1)", a == u1{std::uint64_t{13}});
    }
    {
        u1 a{10}; a -= i2{3};
        TEST("u1{10}  -= i2{3}  == u1{7}   (int wins, truncate to u1)", a == u1{std::uint64_t{7}});
    }
    {
        u1 a{7}; a *= i2{6};
        TEST("u1{7}   *= i2{6}  == u1{42}  (int wins, truncate to u1)", a == u1{std::uint64_t{42}});
    }
}

// =============================================================================
// Section 4: int_fixed_t<N> op= uint_fixed_t<M>  (mixed-sign)
// UAC: N > M -> int_fixed_t<N> wins; N <= M -> uint_fixed_t<M> wins.
// =============================================================================

static void test_mixed_int_lhs()
{
    std::cout << "\n--- Section 4: int op= uint (mixed-sign) ---\n";

    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;
    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;

    // N > M: int wins
    {
        i2 a{10}; a += u1{3};
        TEST("i2{10}  += u1{3}  == i2{13}  (int wins)", a == i2{13});
    }
    {
        i2 a{-5}; a += u1{3};
        TEST("i2{-5}  += u1{3}  == i2{-2}  (int wins)", a == i2{-2});
    }
    {
        i2 a{10}; a -= u1{3};
        TEST("i2{10}  -= u1{3}  == i2{7}   (int wins)", a == i2{7});
    }
    {
        i2 a{-3}; a *= u1{4};
        TEST("i2{-3}  *= u1{4}  == i2{-12} (int wins)", a == i2{-12});
    }
    {
        i2 a{-12}; a /= u1{4};
        TEST("i2{-12} /= u1{4}  == i2{-3}  (int wins)", a == i2{-3});
    }
    {
        i2 a{10}; a %= u1{3};
        TEST("i2{10}  %= u1{3}  == i2{1}   (int wins)", a == i2{1});
    }

    // N <= M: uint wins -> compute as uint, result stored as int_fixed_t<N>
    {
        i1 a{10}; a += u2{3};
        TEST("i1{10}  += u2{3}  == i1{13}  (uint wins, truncate to i1)", a == i1{13});
    }
    {
        i1 a{10}; a -= u2{3};
        TEST("i1{10}  -= u2{3}  == i1{7}   (uint wins, truncate to i1)", a == i1{7});
    }
    {
        i1 a{7}; a *= u2{6};
        TEST("i1{7}   *= u2{6}  == i1{42}  (uint wins, truncate to i1)", a == i1{42});
    }
    {
        i1 a{10}; a %= u2{std::uint64_t{3}};
        TEST("i1{10}  %= u2{3}  == i1{1}   (uint wins, truncate to i1)", a == i1{1});
    }
}

// =============================================================================
// Section 5: int_fixed_t<N> op uint_fixed_t<M> free operators
// mixed_iu_t<N,M>: N>M -> int_fixed_t<N>; N<=M -> uint_fixed_t<M>
// =============================================================================

static void test_mixed_free_ops()
{
    std::cout << "\n--- Section 5: int op uint free operators ---\n";

    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;
    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;

    // --- Arithmetic: N > M -> int wins ---
    {
        auto r = i2{10} + u1{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i2+u1 -> i2");
        TEST("i2{10}+u1{3}==i2{13}  (int wins)", r == i2{13});
    }
    {
        auto r = u1{3} + i2{10};
        static_assert(std::is_same_v<decltype(r), i2>, "u1+i2 -> i2");
        TEST("u1{3}+i2{10}==i2{13}  (symmetric)", r == i2{13});
    }
    {
        auto r = i2{10} - u1{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i2-u1 -> i2");
        TEST("i2{10}-u1{3}==i2{7}   (int wins)", r == i2{7});
    }
    {
        auto r = i2{-3} * u1{4};
        static_assert(std::is_same_v<decltype(r), i2>, "i2*u1 -> i2");
        TEST("i2{-3}*u1{4}==i2{-12} (int wins)", r == i2{-12});
    }
    {
        auto r = i2{-12} / u1{4};
        static_assert(std::is_same_v<decltype(r), i2>, "i2/u1 -> i2");
        TEST("i2{-12}/u1{4}==i2{-3} (int wins)", r == i2{-3});
    }
    {
        auto r = i2{10} % u1{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i2%u1 -> i2");
        TEST("i2{10}%u1{3}==i2{1}   (int wins)", r == i2{1});
    }

    // --- Arithmetic: N <= M -> uint wins ---
    {
        auto r = i1{10} + u2{3};
        static_assert(std::is_same_v<decltype(r), u2>, "i1+u2 -> u2");
        TEST("i1{10}+u2{3}==u2{13}  (uint wins)", r == u2{std::uint64_t{13}});
    }
    {
        auto r = u2{3} + i1{10};
        static_assert(std::is_same_v<decltype(r), u2>, "u2+i1 -> u2");
        TEST("u2{3}+i1{10}==u2{13}  (symmetric)", r == u2{std::uint64_t{13}});
    }
    {
        auto r = i1{10} - u2{3};
        static_assert(std::is_same_v<decltype(r), u2>, "i1-u2 -> u2");
        TEST("i1{10}-u2{3}==u2{7}   (uint wins)", r == u2{std::uint64_t{7}});
    }
    {
        auto r = i1{6} * u2{7};
        static_assert(std::is_same_v<decltype(r), u2>, "i1*u2 -> u2");
        TEST("i1{6}*u2{7}==u2{42}   (uint wins)", r == u2{std::uint64_t{42}});
    }
    {
        auto r = i1{42} / u2{std::uint64_t{6}};
        static_assert(std::is_same_v<decltype(r), u2>, "i1/u2 -> u2");
        TEST("i1{42}/u2{6}==u2{7}   (uint wins)", r == u2{std::uint64_t{7}});
    }
    {
        auto r = i1{10} % u2{std::uint64_t{3}};
        static_assert(std::is_same_v<decltype(r), u2>, "i1%u2 -> u2");
        TEST("i1{10}%u2{3}==u2{1}   (uint wins)", r == u2{std::uint64_t{1}});
    }

    // --- Bitwise ---
    {
        auto r = i2{0xFF} & u1{std::uint64_t{0x0F}};
        static_assert(std::is_same_v<decltype(r), i2>, "i2&u1 -> i2");
        TEST("i2{0xFF}&u1{0x0F}==i2{0x0F}", r == i2{0x0F});
    }
    {
        auto r = i2{0xF0} | u1{std::uint64_t{0x0F}};
        static_assert(std::is_same_v<decltype(r), i2>, "i2|u1 -> i2");
        TEST("i2{0xF0}|u1{0x0F}==i2{0xFF}", r == i2{0xFF});
    }
    {
        auto r = i2{0xFF} ^ u1{std::uint64_t{0xFF}};
        static_assert(std::is_same_v<decltype(r), i2>, "i2^u1 -> i2");
        TEST("i2{0xFF}^u1{0xFF}==i2{0}",   r == i2{0});
    }

    // --- Comparisons: N > M -> int wins ---
    TEST("i2{10} == u1{10}",  i2{10} == u1{std::uint64_t{10}});
    TEST("u1{10} == i2{10}",  u1{std::uint64_t{10}} == i2{10});
    TEST("i2{10} != u1{3}",   i2{10} != u1{std::uint64_t{3}});
    TEST("i2{3}  <  u1{10}",  i2{3}  <  u1{std::uint64_t{10}});
    TEST("u1{10} >  i2{3}",   u1{std::uint64_t{10}} >  i2{3});
    TEST("i2{10} <= u1{10}",  i2{10} <= u1{std::uint64_t{10}});
    TEST("i2{10} >= u1{10}",  i2{10} >= u1{std::uint64_t{10}});

    // --- Comparisons: N <= M -> uint wins ---
    TEST("i1{10} == u2{10}",  i1{10} == u2{std::uint64_t{10}});
    TEST("u2{10} == i1{10}",  u2{std::uint64_t{10}} == i1{10});
    TEST("i1{3}  <  u2{10}",  i1{3}  <  u2{std::uint64_t{10}});
    TEST("u2{10} >  i1{3}",   u2{std::uint64_t{10}} >  i1{3});
}

// =============================================================================
// Section 6: uint_fixed_t<N> op uint_fixed_t<M>  free operators (N != M)
// Result type: uint_fixed_t<max(N,M)>
// =============================================================================

static void test_cross_n_uint_free()
{
    std::cout << "\n--- Section 6: uint cross-N free operators ---\n";

    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;

    // --- Arithmetic: result is always u2 (the wider type) ---
    {
        auto r = u1{10} + u2{3};
        static_assert(std::is_same_v<decltype(r), u2>, "u1+u2 -> u2");
        TEST("u1{10}+u2{3}  == u2{13}", r == u2{std::uint64_t{13}});
    }
    {
        auto r = u2{10} + u1{3};
        static_assert(std::is_same_v<decltype(r), u2>, "u2+u1 -> u2");
        TEST("u2{10}+u1{3}  == u2{13}  (symmetric)", r == u2{std::uint64_t{13}});
    }
    {
        auto r = u1{10} - u2{3};
        static_assert(std::is_same_v<decltype(r), u2>, "u1-u2 -> u2");
        TEST("u1{10}-u2{3}  == u2{7}", r == u2{std::uint64_t{7}});
    }
    {
        auto r = u2{10} - u1{3};
        static_assert(std::is_same_v<decltype(r), u2>, "u2-u1 -> u2");
        TEST("u2{10}-u1{3}  == u2{7}   (symmetric)", r == u2{std::uint64_t{7}});
    }
    {
        auto r = u1{7} * u2{6};
        static_assert(std::is_same_v<decltype(r), u2>, "u1*u2 -> u2");
        TEST("u1{7}*u2{6}   == u2{42}", r == u2{std::uint64_t{42}});
    }
    {
        auto r = u2{42} / u1{std::uint64_t{6}};
        static_assert(std::is_same_v<decltype(r), u2>, "u2/u1 -> u2");
        TEST("u2{42}/u1{6}  == u2{7}", r == u2{std::uint64_t{7}});
    }
    {
        auto r = u1{10} % u2{std::uint64_t{3}};
        static_assert(std::is_same_v<decltype(r), u2>, "u1%u2 -> u2");
        TEST("u1{10}%u2{3}  == u2{1}", r == u2{std::uint64_t{1}});
    }

    // --- Bitwise ---
    {
        auto r = u1{std::uint64_t{0xFF}} & u2{std::uint64_t{0x0F}};
        static_assert(std::is_same_v<decltype(r), u2>, "u1&u2 -> u2");
        TEST("u1{0xFF}&u2{0x0F} == u2{0x0F}", r == u2{std::uint64_t{0x0F}});
    }
    {
        auto r = u1{std::uint64_t{0xF0}} | u2{std::uint64_t{0x0F}};
        static_assert(std::is_same_v<decltype(r), u2>, "u1|u2 -> u2");
        TEST("u1{0xF0}|u2{0x0F} == u2{0xFF}", r == u2{std::uint64_t{0xFF}});
    }
    {
        auto r = u1{std::uint64_t{0xFF}} ^ u2{std::uint64_t{0xFF}};
        static_assert(std::is_same_v<decltype(r), u2>, "u1^u2 -> u2");
        TEST("u1{0xFF}^u2{0xFF} == u2{0}", r == u2{std::uint64_t{0}});
    }

    // --- Comparisons ---
    TEST("u1{10} == u2{10}", u1{std::uint64_t{10}} == u2{std::uint64_t{10}});
    TEST("u2{10} == u1{10}", u2{std::uint64_t{10}} == u1{std::uint64_t{10}});
    TEST("u1{3}  <  u2{10}", u1{std::uint64_t{3}}  <  u2{std::uint64_t{10}});
    TEST("u2{10} >  u1{3}",  u2{std::uint64_t{10}} >  u1{std::uint64_t{3}});
    TEST("u1{10} <= u2{10}", u1{std::uint64_t{10}} <= u2{std::uint64_t{10}});
    TEST("u1{10} >= u2{10}", u1{std::uint64_t{10}} >= u2{std::uint64_t{10}});
    TEST("u1{10} != u2{3}",  u1{std::uint64_t{10}} != u2{std::uint64_t{3}});
}

// =============================================================================
// Section 7: int_fixed_t<N> op int_fixed_t<M>  free operators (N != M)
// Result type: int_fixed_t<max(N,M)>
// =============================================================================

static void test_cross_n_int_free()
{
    std::cout << "\n--- Section 7: int cross-N free operators ---\n";

    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;

    // --- Arithmetic: result is always i2 (the wider type) ---
    {
        auto r = i1{10} + i2{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i1+i2 -> i2");
        TEST("i1{10}+i2{3}   == i2{13}", r == i2{13});
    }
    {
        auto r = i2{10} + i1{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i2+i1 -> i2");
        TEST("i2{10}+i1{3}   == i2{13}  (symmetric)", r == i2{13});
    }
    {
        auto r = i1{-5} + i2{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i1+i2 -> i2 (negative)");
        TEST("i1{-5}+i2{3}   == i2{-2}", r == i2{-2});
    }
    {
        auto r = i1{10} - i2{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i1-i2 -> i2");
        TEST("i1{10}-i2{3}   == i2{7}", r == i2{7});
    }
    {
        auto r = i2{3} - i1{10};
        static_assert(std::is_same_v<decltype(r), i2>, "i2-i1 -> i2");
        TEST("i2{3}-i1{10}   == i2{-7}", r == i2{-7});
    }
    {
        auto r = i1{-3} * i2{4};
        static_assert(std::is_same_v<decltype(r), i2>, "i1*i2 -> i2");
        TEST("i1{-3}*i2{4}   == i2{-12}", r == i2{-12});
    }
    {
        auto r = i2{-12} / i1{4};
        static_assert(std::is_same_v<decltype(r), i2>, "i2/i1 -> i2");
        TEST("i2{-12}/i1{4}  == i2{-3}", r == i2{-3});
    }
    {
        auto r = i1{-10} % i2{3};
        static_assert(std::is_same_v<decltype(r), i2>, "i1%i2 -> i2");
        TEST("i1{-10}%i2{3}  == i2{-1}", r == i2{-1});
    }

    // --- Bitwise ---
    {
        auto r = i1{0xFF} & i2{0x0F};
        static_assert(std::is_same_v<decltype(r), i2>, "i1&i2 -> i2");
        TEST("i1{0xFF}&i2{0x0F} == i2{0x0F}", r == i2{0x0F});
    }
    {
        auto r = i1{0xF0} | i2{0x0F};
        static_assert(std::is_same_v<decltype(r), i2>, "i1|i2 -> i2");
        TEST("i1{0xF0}|i2{0x0F} == i2{0xFF}", r == i2{0xFF});
    }
    {
        auto r = i1{0xFF} ^ i2{0xFF};
        static_assert(std::is_same_v<decltype(r), i2>, "i1^i2 -> i2");
        TEST("i1{0xFF}^i2{0xFF} == i2{0}", r == i2{0});
    }

    // --- Comparisons ---
    TEST("i1{10} == i2{10}", i1{10} == i2{10});
    TEST("i2{10} == i1{10}", i2{10} == i1{10});
    TEST("i1{-1} <  i2{0}",  i1{-1} <  i2{0});
    TEST("i2{0}  >  i1{-1}", i2{0}  >  i1{-1});
    TEST("i1{10} <= i2{10}", i1{10} <= i2{10});
    TEST("i1{10} >= i2{10}", i1{10} >= i2{10});
    TEST("i1{10} != i2{3}",  i1{10} != i2{3});
}

// =============================================================================
// Section 8: Unary operator+ (T6 — Fase MS-INTEROP)
// Mirrors built-in: `+x` is a copy. Should work for both signed and unsigned.
// =============================================================================

static void test_unary_plus()
{
    using u2 = uint_fixed_t<2>;
    using i2 = int_fixed_t<2>;
    using u4 = uint_fixed_t<4>;
    using i4 = int_fixed_t<4>;

    TEST("+u2{42}==u2{42}",            +u2{42} == u2{42});
    TEST("+i2{-5}==i2{-5}",            +i2{-5} == i2{-5});
    TEST("+u4{0}==u4{0}",              +u4{0}  == u4{0});
    TEST("+i4{-1}==i4{-1}",            +i4{-1} == i4{-1});
    // Type-preserving: +x is the same type as x.
    static_assert(std::is_same_v<decltype(+u2{0}), u2>);
    static_assert(std::is_same_v<decltype(+i2{0}), i2>);
    static_assert(std::is_same_v<decltype(+u4{0}), u4>);
    static_assert(std::is_same_v<decltype(+i4{0}), i4>);
    // constexpr: returns a copy at compile time.
    static_assert(+u2{7} == u2{7});
    static_assert(+i2{-7} == i2{-7});
    TEST("decltype(+u2)==u2 (static_assert)", true);
    TEST("decltype(+i2)==i2 (static_assert)", true);
}

// =============================================================================
// Section 9: Shift operators with fixed_int_t<M> count (T1 — Fase MS-INTEROP)
// Mirrors built-in: shift count can be any integral; LHS type is preserved.
// Negative signed counts mirror the wraparound UB of built-in shifts.
// =============================================================================

static void test_shift_cross_sign()
{
    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;
    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;

    // ---- u2 << / >> with various count types ----
    TEST("u2{1}<<u1{4}==u2{16}",       (u2{1} << u1{4})  == u2{16});
    TEST("u2{1}<<i1{4}==u2{16}",       (u2{1} << i1{4})  == u2{16});
    TEST("u2{1}<<u2{4}==u2{16}",       (u2{1} << u2{4})  == u2{16});
    TEST("u2{1}<<i2{4}==u2{16}",       (u2{1} << i2{4})  == u2{16});
    TEST("u2{256}>>u1{4}==u2{16}",     (u2{256} >> u1{4}) == u2{16});
    TEST("u2{256}>>i1{4}==u2{16}",     (u2{256} >> i1{4}) == u2{16});
    TEST("u2{256}>>u2{4}==u2{16}",     (u2{256} >> u2{4}) == u2{16});
    TEST("u2{256}>>i2{4}==u2{16}",     (u2{256} >> i2{4}) == u2{16});

    // ---- i2 << / >> with various count types ----
    TEST("i2{1}<<u1{4}==i2{16}",       (i2{1} << u1{4})  == i2{16});
    TEST("i2{1}<<i1{4}==i2{16}",       (i2{1} << i1{4})  == i2{16});
    TEST("i2{-1}>>u1{1}==i2{-1}",      (i2{-1} >> u1{1}) == i2{-1}); // arithmetic shift preserves sign
    TEST("i2{-256}>>i1{4}==i2{-16}",   (i2{-256} >> i1{4}) == i2{-16});
    TEST("i2{-1}>>u2{63}==i2{-1}",     (i2{-1} >> u2{63}) == i2{-1});

    // ---- compound assignment cross-sign count ----
    {
        u2 x{1};
        x <<= u1{8};
        TEST("u2{1} <<= u1{8} -> u2{256}", x == u2{256});
    }
    {
        u2 x{1};
        x <<= i1{8};
        TEST("u2{1} <<= i1{8} -> u2{256}", x == u2{256});
    }
    {
        i2 x{-1024};
        x >>= u1{2};
        TEST("i2{-1024} >>= u1{2} -> i2{-256}", x == i2{-256});
    }
    {
        i2 x{-1024};
        x >>= i1{2};
        TEST("i2{-1024} >>= i1{2} -> i2{-256}", x == i2{-256});
    }

    // ---- LHS type is preserved (decltype check) ----
    static_assert(std::is_same_v<decltype(u2{1} << u1{1}), u2>);
    static_assert(std::is_same_v<decltype(u2{1} << i1{1}), u2>);
    static_assert(std::is_same_v<decltype(i2{1} << u2{1}), i2>);
    static_assert(std::is_same_v<decltype(i2{1} >> i1{1}), i2>);

    // ---- Edge cases ----
    // Shift by 0 is identity for both signed and unsigned LHS.
    TEST("u2{42}<<u1{0}==u2{42}",      (u2{42} << u1{0})  == u2{42});
    TEST("i2{-7}>>i1{0}==i2{-7}",      (i2{-7} >> i1{0})  == i2{-7});

    // Shift by >= 64*N: zero for unsigned, sign-fill for signed arithmetic >>.
    TEST("u2{1}<<u2{128}==u2{0}",      (u2{1} << u2{128}) == u2{0});
    TEST("u2{1}<<u1{255}==u2{0}",      (u2{1} << u1{255}) == u2{0});

    // Negative signed count: wraps to huge unsigned -> shift >= 64*N -> zero.
    // Mirrors built-in `int x = 1; x << -1;` (UB), but at least we don't crash.
    TEST("u2{1}<<i1{-1}==u2{0}",       (u2{1} << i1{-1})  == u2{0});

    // constexpr: all shift overloads are constexpr.
    static_assert((u2{1} << u1{3})  == u2{8});
    static_assert((u2{8} >> u1{3})  == u2{1});
    static_assert((i2{-1} >> u1{0}) == i2{-1});
}

// =============================================================================
// Section 10: operator<=> three-way comparison (T2 — Fase MS-INTEROP)
// COEXISTS with the 12 manual comparators; explicit < / <= / > / >= continue
// to work unchanged. <=> is available for direct use and for generic algorithms
// that route through std::compare_three_way.
// =============================================================================

static void test_three_way()
{
    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;
    using u4 = uint_fixed_t<4>;
    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;
    using i4 = int_fixed_t<4>;

    using ord = std::strong_ordering;

    // ---- Same-type: member <=> ----
    TEST("u2{1} <=> u2{2} == less",       (u2{1}  <=> u2{2})  == ord::less);
    TEST("u2{2} <=> u2{1} == greater",    (u2{2}  <=> u2{1})  == ord::greater);
    TEST("u2{5} <=> u2{5} == equal",      (u2{5}  <=> u2{5})  == ord::equal);
    TEST("i2{-1} <=> i2{0} == less",      (i2{-1} <=> i2{0})  == ord::less);
    TEST("i2{0} <=> i2{-1} == greater",   (i2{0}  <=> i2{-1}) == ord::greater);

    // ---- Same-sign cross-N: free <=> ----
    TEST("u2{1} <=> u4{2} == less",       (u2{1}  <=> u4{2})  == ord::less);
    TEST("u4{5} <=> u2{5} == equal",      (u4{5}  <=> u2{5})  == ord::equal);
    TEST("i2{-1} <=> i4{1} == less",      (i2{-1} <=> i4{1})  == ord::less);
    TEST("i4{-1} <=> i2{-1} == equal",    (i4{-1} <=> i2{-1}) == ord::equal);

    // ---- Cross-sign same-N: free <=> uses mixed_iu_t<N,N> = uint (unsigned wins) ----
    TEST("u2{5} <=> i2{5} == equal",      (u2{5} <=> i2{5})   == ord::equal);
    TEST("u2{5} <=> i2{-1} == less",      // i2{-1} promotes to u2{2^128-1} -> u2{5} < that
                                          (u2{5} <=> i2{-1})  == ord::less);
    TEST("i2{-1} <=> u2{5} == greater",   (i2{-1} <=> u2{5})  == ord::greater);

    // ---- Cross-sign cross-N where signed wins (N(signed) > N(unsigned)) ----
    TEST("i4{-1} <=> u2{5} == less",      // mixed_iu_t<4,2> = i4; i4{-1} < i4{5}
                                          (i4{-1} <=> u2{5})  == ord::less);
    TEST("u2{5} <=> i4{-1} == greater",   (u2{5}  <=> i4{-1}) == ord::greater);
    TEST("i4{100} <=> u2{50} == greater", (i4{100} <=> u2{50}) == ord::greater);

    // ---- Cross-sign cross-N where unsigned wins (N(unsigned) >= N(signed)) ----
    TEST("i2{5} <=> u4{5} == equal",      (i2{5} <=> u4{5})   == ord::equal);
    TEST("i1{-1} <=> u4{0} == greater",   // i1{-1} promotes to u4{2^256-1}
                                          (i1{-1} <=> u4{0})  == ord::greater);

    // ---- Return type is std::strong_ordering ----
    static_assert(std::is_same_v<decltype(u2{0} <=> u2{0}),     ord>);
    static_assert(std::is_same_v<decltype(i2{0} <=> i2{0}),     ord>);
    static_assert(std::is_same_v<decltype(u2{0} <=> u4{0}),     ord>);
    static_assert(std::is_same_v<decltype(i2{0} <=> u4{0}),     ord>);
    static_assert(std::is_same_v<decltype(u4{0} <=> i2{0}),     ord>);

    // ---- constexpr: <=> works at compile time ----
    static_assert((u2{1} <=> u2{2}) == ord::less);
    static_assert((i2{-1} <=> i2{0}) == ord::less);
    static_assert((u2{5} <=> i2{5}) == ord::equal);

    // ---- Coexistence: manual operators still work the same ----
    TEST("u2{1} < u2{2} still works",     u2{1}  <  u2{2});
    TEST("u2{1} <= u2{1} still works",    u2{1}  <= u2{1});
    TEST("u2{5} > u2{4} still works",     u2{5}  >  u2{4});
    TEST("u2{5} >= u2{5} still works",    u2{5}  >= u2{5});
    TEST("u2{5} == u2{5} still works",    u2{5}  == u2{5});
    TEST("u2{5} != u2{4} still works",    u2{5}  != u2{4});

    // ---- Cross-sign manual operators still work (no regression) ----
    TEST("u2{5} < i4{10} (manual) ",      u2{5}  < i4{10});
    TEST("i4{-1} == u2{...} via manual",  i4{-1} != u2{0});
}

// =============================================================================
// Section 11: edge cases — sign extension, wraparound, extremes, <=> on boundary
// (T7 — Fase MS-INTEROP).
// Goals:
//   A. Sign extension across cross-sign cross-N promotion
//   B. Wraparound at type boundaries (unsigned wraps, signed 2's-complement wraps)
//   C. Cross-sign arithmetic at extremes (INT_MIN ± UINT_MAX, etc.)
//   D. Cross-sign comparison at extremes — the "negative-vs-unsigned" gotcha
//   E. Cross-sign division/modulo with extremes
//   F. <=> on the numeric_limits boundary
//   G. Shifts on boundary counts and extreme LHS
// =============================================================================

#include <limits>

static void test_edge_cases_cross_sign()
{
    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;
    using u4 = uint_fixed_t<4>;
    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;
    using i4 = int_fixed_t<4>;

    using ord = std::strong_ordering;

    // ----- A. Sign extension across cross-sign cross-N promotion -----
    // i1{-1} stored as data[0] = 2^64-1. Promoted to u2 (sign-extends): u2 = 2^128-1.
    TEST("static_cast<u2>(i1{-1}) == u2::max()", static_cast<u2>(i1{-1}) == u2{std::numeric_limits<u2>::max()});
    TEST("static_cast<u4>(i1{-1}) == u4::max()", static_cast<u4>(i1{-1}) == u4{std::numeric_limits<u4>::max()});
    TEST("static_cast<u4>(i2{-1}) == u4::max()", static_cast<u4>(i2{-1}) == u4{std::numeric_limits<u4>::max()});
    // i1{-2} promotes to u4 as ...FFFE (all ones except low bit)
    TEST("static_cast<u4>(i1{-2}) == u4::max()-1",
         static_cast<u4>(i1{-2}) == std::numeric_limits<u4>::max() - u4{1});
    // Non-negative source: zero-extends (no sign bit set)
    TEST("static_cast<u4>(i1{5}) == u4{5}", static_cast<u4>(i1{5}) == u4{5});

    // ----- B. Wraparound at type boundaries -----
    TEST("u2::max() + 1 == u2{0} (modular)",
         std::numeric_limits<u2>::max() + u2{1} == u2{0});
    TEST("u2{0} - 1 == u2::max()",
         u2{0} - u2{1} == std::numeric_limits<u2>::max());
    TEST("i2::max() + 1 == i2::min() (TC wrap)",
         std::numeric_limits<i2>::max() + i2{1} == std::numeric_limits<i2>::min());
    TEST("i2::min() - 1 == i2::max() (TC wrap)",
         std::numeric_limits<i2>::min() - i2{1} == std::numeric_limits<i2>::max());
    // -INT_MIN wraps back to INT_MIN in 2's complement (classic).
    TEST("-i2::min() == i2::min()",
         -std::numeric_limits<i2>::min() == std::numeric_limits<i2>::min());
    // INT_MIN * -1 — same wraparound.
    TEST("i2::min() * i2{-1} == i2::min()",
         std::numeric_limits<i2>::min() * i2{-1} == std::numeric_limits<i2>::min());

    // ----- C. Cross-sign arithmetic at extremes -----
    // i2::max() + u2{1}: mixed_iu_t<2,2> = u2. i2::max() = 2^127-1; +1 = 2^127.
    {
        const u2 expected = u2{1} << u1{127};
        TEST("i2::max() + u2{1} == 2^127",
             std::numeric_limits<i2>::max() + u2{1} == expected);
    }
    // i2{-1} + u2::max(): i2{-1} -> u2{2^128-1}; +u2{2^128-1} wraps to u2{2^128-2}.
    TEST("i2{-1} + u2::max() == u2::max()-1",
         i2{-1} + std::numeric_limits<u2>::max()
             == std::numeric_limits<u2>::max() - u2{1});
    // u2::max() - i2{-1}: i2{-1} -> u2::max; max - max = 0.
    TEST("u2::max() - i2{-1} == u2{0}",
         std::numeric_limits<u2>::max() - i2{-1} == u2{0});
    // u2{0} + i2{-1}: -> u2::max
    TEST("u2{0} + i2{-1} == u2::max()",
         u2{0} + i2{-1} == std::numeric_limits<u2>::max());
    // Cross-N where signed wins: i4 + u2 -> i4.
    TEST("i4::min() + u2{1} == i4::min()+1",
         std::numeric_limits<i4>::min() + u2{1} == std::numeric_limits<i4>::min() + i4{1});
    TEST("i4::max() + u2{0} == i4::max()",
         std::numeric_limits<i4>::max() + u2{0} == std::numeric_limits<i4>::max());

    // ----- D. Cross-sign comparison at extremes — the "negative-vs-unsigned" gotcha -----
    // (int)INT_MIN < (unsigned)0  is FALSE in C++. Same here for mixed_iu_t<N,N>=u.
    TEST("i2::min() > u2{0} (gotcha)",
         std::numeric_limits<i2>::min() > u2{0});
    TEST("i2::min() == u2{1<<127}",
         std::numeric_limits<i2>::min() == (u2{1} << u1{127}));
    // i4::min() vs u2: i4 wins (mixed_iu_t<4,2>=i4). u2::max() < i4::max() (positive),
    // and i4::min() is far below 0. So i4::min() < u2::max().
    TEST("i4::min() < u2::max()",
         std::numeric_limits<i4>::min() < std::numeric_limits<u2>::max());
    TEST("i4::max() > u2::max()",
         std::numeric_limits<i4>::max() > std::numeric_limits<u2>::max());
    // Sign-extended -1 equals all-ones uint of higher rank.
    TEST("i1{-1} == u4::max() (sign ext)",
         i1{-1} == std::numeric_limits<u4>::max());
    TEST("i2{-1} == u4::max() (sign ext)",
         i2{-1} == std::numeric_limits<u4>::max());
    // Manual operators still consistent at boundary.
    TEST("i2::max() < u2::max() (manual)",
         std::numeric_limits<i2>::max() < std::numeric_limits<u2>::max());
    TEST("i2{-1} != u2{0} (manual)",
         i2{-1} != u2{0});

    // ----- E. Cross-sign division/modulo with extremes -----
    // i2::min() / i2{-1}: classic TC overflow. Our impl wraps to i2::min().
    TEST("i2::min() / i2{-1} == i2::min() (TC wrap)",
         std::numeric_limits<i2>::min() / i2{-1} == std::numeric_limits<i2>::min());
    // u2::max() / u2{2} == 2^127 - 1 (truncating)
    TEST("u2::max() / u2{2} == (1<<127) - 1",
         std::numeric_limits<u2>::max() / u2{2} == (u2{1} << u1{127}) - u2{1});
    TEST("u2::max() % u2{2} == u2{1}",
         std::numeric_limits<u2>::max() % u2{2} == u2{1});
    // i2{-10} / u2{3}: mixed_iu_t<2,2>=u2. i2{-10}->u2{huge}; huge / 3 = some big value.
    // Just check it's not zero and the type is u2.
    {
        const auto r = i2{-10} / u2{3};
        static_assert(std::is_same_v<decltype(r), const u2>);
        TEST("i2{-10} / u2{3} != u2{0}", r != u2{0});
    }

    // ----- F. <=> on the numeric_limits boundary -----
    TEST("i2::min() <=> i2::max() == less",
         (std::numeric_limits<i2>::min() <=> std::numeric_limits<i2>::max()) == ord::less);
    TEST("u2::max() <=> u2::min() == greater",
         (std::numeric_limits<u2>::max() <=> std::numeric_limits<u2>::min()) == ord::greater);
    // Cross-sign at extreme: i2::min() vs u2{0} via <=> -> greater (gotcha holds here too).
    TEST("i2::min() <=> u2{0} == greater",
         (std::numeric_limits<i2>::min() <=> u2{0}) == ord::greater);
    // i4::min() vs u4::min(): mixed_iu_t<4,4>=u4. i4::min->u4{2^255}. > u4{0}.
    TEST("i4::min() <=> u4::min() == greater",
         (std::numeric_limits<i4>::min() <=> std::numeric_limits<u4>::min()) == ord::greater);
    // Cross-N where signed wins: i4 vs u2 at extremes.
    TEST("i4::min() <=> u2::max() == less",
         (std::numeric_limits<i4>::min() <=> std::numeric_limits<u2>::max()) == ord::less);

    // ----- G. Shifts with boundary counts and extreme LHS -----
    // Shift by exact 64*N - 1: top bit only.
    TEST("u2{1} << u1{127} top bit set",
         (u2{1} << u1{127}) == (std::numeric_limits<u2>::max() - (std::numeric_limits<u2>::max() >> u1{1})));
    // Arithmetic shift of i2::min() right by 1: half (still negative).
    TEST("i2::min() >> u1{1} == i2::min()/2",
         (std::numeric_limits<i2>::min() >> u1{1}) == std::numeric_limits<i2>::min() / i2{2});
    // u2::max() >> u2{1} == 2^127 - 1
    TEST("u2::max() >> u2{1} == (1<<127)-1",
         (std::numeric_limits<u2>::max() >> u2{1}) == (u2{1} << u1{127}) - u2{1});
    // Out-of-range shift via signed negative count: maps to huge unsigned -> 0
    TEST("u2{42} << i2{-1} == u2{0}",
         (u2{42} << i2{-1}) == u2{0});
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Cross-N and Mixed-Sign Operator Tests (v1.90)\n";
    std::cout << "====================================================================\n";

    test_cross_n_uint_compound();
    test_cross_n_int_compound();
    test_mixed_uint_lhs();
    test_mixed_int_lhs();
    test_mixed_free_ops();
    test_cross_n_uint_free();
    test_cross_n_int_free();
    test_unary_plus();
    test_shift_cross_sign();
    test_three_way();
    test_edge_cases_cross_sign();

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
