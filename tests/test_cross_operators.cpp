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

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
