// =============================================================================
// Test: type traits & common_type & concepts for fixed_int_t
// Part of int128 Library - Phase 1.81 (Fase MS-INTEROP, T3+T4)
// License: BSL-1.0
// =============================================================================
//
// Verifies:
//   T3 — std::common_type<fixed_int_t<...>, fixed_int_t<...>>           (UAC)
//        std::common_type<fixed_int_t<...>, T_builtin>
//        nstd::mixed_iu_t<N, M> public alias
//   T4 — nstd::is_integral / is_arithmetic / is_signed / is_unsigned
//        nstd::make_signed / make_unsigned
//        nstd::is_fixed_int / is_signed_fixed_int / is_unsigned_fixed_int
//        Concepts: fixed_int_type, signed_fixed_int_type, unsigned_fixed_int_type,
//                  nstd::integral, nstd::signed_integral, nstd::unsigned_integral

#include "fixed_int_traits_specializations.hpp"
#include "fixed_int_concepts.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <type_traits>

using nstd::int_fixed_t;
using nstd::uint_fixed_t;

using u1 = uint_fixed_t<1>;
using u2 = uint_fixed_t<2>;
using u4 = uint_fixed_t<4>;
using u8 = uint_fixed_t<8>;
using i1 = int_fixed_t<1>;
using i2 = int_fixed_t<2>;
using i4 = int_fixed_t<4>;
using i8 = int_fixed_t<8>;

// =============================================================================
// T4.a — Detection traits
// =============================================================================

static_assert(nstd::is_fixed_int_v<u2>);
static_assert(nstd::is_fixed_int_v<i2>);
static_assert(nstd::is_fixed_int_v<const u4>); // remove_cv
static_assert(!nstd::is_fixed_int_v<int>);
static_assert(!nstd::is_fixed_int_v<unsigned long long>);
static_assert(!nstd::is_fixed_int_v<void>);

static_assert(nstd::is_signed_fixed_int_v<i2>);
static_assert(nstd::is_signed_fixed_int_v<i8>);
static_assert(!nstd::is_signed_fixed_int_v<u2>);
static_assert(!nstd::is_signed_fixed_int_v<int>); // built-in: not a fixed_int

static_assert(nstd::is_unsigned_fixed_int_v<u2>);
static_assert(nstd::is_unsigned_fixed_int_v<u8>);
static_assert(!nstd::is_unsigned_fixed_int_v<i2>);
static_assert(!nstd::is_unsigned_fixed_int_v<unsigned>); // built-in: not a fixed_int

// =============================================================================
// T4.b — nstd:: trait specializations for fixed_int_t
// =============================================================================

static_assert(nstd::is_integral_v<u2>);
static_assert(nstd::is_integral_v<i4>);
static_assert(nstd::is_integral_v<int>); // delegates to std::
static_assert(nstd::is_integral_v<unsigned long>);
static_assert(!nstd::is_integral_v<float>);
static_assert(!nstd::is_integral_v<void>);

static_assert(nstd::is_arithmetic_v<u2>);
static_assert(nstd::is_arithmetic_v<i4>);
static_assert(nstd::is_arithmetic_v<double>);
static_assert(!nstd::is_arithmetic_v<void *>);

static_assert(nstd::is_signed_v<i2>);
static_assert(nstd::is_signed_v<i8>);
static_assert(nstd::is_signed_v<int>);
static_assert(!nstd::is_signed_v<u2>);
static_assert(!nstd::is_signed_v<unsigned>);

static_assert(nstd::is_unsigned_v<u2>);
static_assert(nstd::is_unsigned_v<u8>);
static_assert(nstd::is_unsigned_v<unsigned>);
static_assert(!nstd::is_unsigned_v<i2>);
static_assert(!nstd::is_unsigned_v<int>);

// =============================================================================
// T4.c — nstd::make_signed / nstd::make_unsigned
// =============================================================================

static_assert(std::is_same_v<nstd::make_signed_t<u1>, i1>);
static_assert(std::is_same_v<nstd::make_signed_t<u2>, i2>);
static_assert(std::is_same_v<nstd::make_signed_t<u4>, i4>);
static_assert(std::is_same_v<nstd::make_signed_t<u8>, i8>);
static_assert(std::is_same_v<nstd::make_signed_t<i2>, i2>);   // identity on signed
static_assert(std::is_same_v<nstd::make_signed_t<int>, int>); // built-in delegates

static_assert(std::is_same_v<nstd::make_unsigned_t<i1>, u1>);
static_assert(std::is_same_v<nstd::make_unsigned_t<i2>, u2>);
static_assert(std::is_same_v<nstd::make_unsigned_t<i4>, u4>);
static_assert(std::is_same_v<nstd::make_unsigned_t<i8>, u8>);
static_assert(std::is_same_v<nstd::make_unsigned_t<u2>, u2>); // identity on unsigned
static_assert(std::is_same_v<nstd::make_unsigned_t<int>, unsigned>);

// =============================================================================
// T3.a — nstd::mixed_iu_t public alias (promoted from detail::)
// =============================================================================

static_assert(std::is_same_v<nstd::mixed_iu_t<2, 2>, u2>); // N == M -> unsigned wins
static_assert(std::is_same_v<nstd::mixed_iu_t<2, 1>, i2>); // N > M  -> signed wins
static_assert(std::is_same_v<nstd::mixed_iu_t<1, 2>, u2>); // N < M  -> unsigned wins
static_assert(std::is_same_v<nstd::mixed_iu_t<4, 2>, i4>);
static_assert(std::is_same_v<nstd::mixed_iu_t<2, 4>, u4>);
static_assert(std::is_same_v<nstd::mixed_iu_t<8, 1>, i8>);

// =============================================================================
// T3.b — std::common_type for fixed_int_t × fixed_int_t
// =============================================================================

// same-sign cross-N
static_assert(std::is_same_v<std::common_type_t<i2, i4>, i4>);
static_assert(std::is_same_v<std::common_type_t<i4, i2>, i4>);
static_assert(std::is_same_v<std::common_type_t<u2, u4>, u4>);
static_assert(std::is_same_v<std::common_type_t<u4, u2>, u4>);
static_assert(std::is_same_v<std::common_type_t<u2, u2>, u2>);

// cross-sign same-N -> unsigned wins (mixed_iu_t<N,N> = uint_fixed_t<N>)
static_assert(std::is_same_v<std::common_type_t<i2, u2>, u2>);
static_assert(std::is_same_v<std::common_type_t<u2, i2>, u2>);

// cross-sign cross-N -> signed wins iff strictly wider
static_assert(std::is_same_v<std::common_type_t<i4, u2>, i4>); // N(signed)=4 > M(unsigned)=2
static_assert(std::is_same_v<std::common_type_t<u2, i4>, i4>);
static_assert(std::is_same_v<std::common_type_t<i2, u4>, u4>); // N(signed)=2 < M(unsigned)=4
static_assert(std::is_same_v<std::common_type_t<u4, i2>, u4>);
static_assert(std::is_same_v<std::common_type_t<i1, u8>, u8>);
static_assert(std::is_same_v<std::common_type_t<u8, i1>, u8>);

// =============================================================================
// T3.c — std::common_type for fixed_int_t × built-in integral
// =============================================================================

static_assert(std::is_same_v<std::common_type_t<u2, int>, u2>);
static_assert(std::is_same_v<std::common_type_t<int, u2>, u2>);
static_assert(std::is_same_v<std::common_type_t<i4, unsigned>, i4>);
static_assert(std::is_same_v<std::common_type_t<unsigned long long, u8>, u8>);
static_assert(std::is_same_v<std::common_type_t<u2, std::int64_t>, u2>);
static_assert(std::is_same_v<std::common_type_t<std::uint8_t, i2>, i2>);

// =============================================================================
// T4.d — nstd:: concepts
// =============================================================================

// Detection concepts
static_assert(nstd::fixed_int_type<u2>);
static_assert(nstd::fixed_int_type<i4>);
static_assert(!nstd::fixed_int_type<int>);
static_assert(!nstd::fixed_int_type<void>);

static_assert(nstd::signed_fixed_int_type<i2>);
static_assert(!nstd::signed_fixed_int_type<u2>);
static_assert(!nstd::signed_fixed_int_type<int>);

static_assert(nstd::unsigned_fixed_int_type<u2>);
static_assert(!nstd::unsigned_fixed_int_type<i2>);
static_assert(!nstd::unsigned_fixed_int_type<unsigned>);

// Aglutinating concepts (built-in ∪ fixed_int_t).
// These are guarded against conflict with int128_param_concepts.hpp — they are
// only defined if that header was NOT included first.
#ifndef INT128_PARAM_CONCEPTS_HPP

static_assert(nstd::integral<int>);
static_assert(nstd::integral<unsigned long>);
static_assert(nstd::integral<u2>);
static_assert(nstd::integral<i4>);
static_assert(!nstd::integral<float>);
static_assert(!nstd::integral<bool>); // bool excluded by design
static_assert(!nstd::integral<void *>);

static_assert(nstd::signed_integral<int>);
static_assert(nstd::signed_integral<long long>);
static_assert(nstd::signed_integral<i2>);
static_assert(nstd::signed_integral<i8>);
static_assert(!nstd::signed_integral<unsigned>);
static_assert(!nstd::signed_integral<u2>);
static_assert(!nstd::signed_integral<float>);

static_assert(nstd::unsigned_integral<unsigned>);
static_assert(nstd::unsigned_integral<unsigned long long>);
static_assert(nstd::unsigned_integral<u2>);
static_assert(nstd::unsigned_integral<u8>);
static_assert(!nstd::unsigned_integral<int>);
static_assert(!nstd::unsigned_integral<i2>);
static_assert(!nstd::unsigned_integral<bool>); // bool excluded

#endif // !INT128_PARAM_CONCEPTS_HPP

// =============================================================================
// Runtime sanity: a tiny generic function that uses nstd::integral
// =============================================================================

template <typename T>
    requires nstd::integral<T>
constexpr T add_one(T x)
{
    return x + T{1};
}

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                              \
    do                                                \
    {                                                 \
        if (cond)                                     \
        {                                             \
            std::cout << "[OK]   " << (name) << "\n"; \
            ++g_passed;                               \
        }                                             \
        else                                          \
        {                                             \
            std::cout << "[FAIL] " << (name) << "\n"; \
            ++g_failed;                               \
        }                                             \
    } while (false)

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "fixed_int_t traits & concepts tests (v1.81)\n";
    std::cout << "====================================================================\n";

    // Runtime sanity for nstd::integral concept with a generic function.
    TEST("add_one(int{41}) == 42", add_one(41) == 42);
    TEST("add_one(unsigned{41}) == 42", add_one(41u) == 42u);
    TEST("add_one(u2{41}) == u2{42}", add_one(u2{41}) == u2{42});
    TEST("add_one(i2{-5}) == i2{-4}", add_one(i2{-5}) == i2{-4});
    TEST("add_one(u8{99}) == u8{100}", add_one(u8{99}) == u8{100});
    TEST("add_one(i4{-1}) == i4{0}", add_one(i4{-1}) == i4{0});

    // common_type roundtrip: build the common type and verify it stores the value.
    using ct1 = std::common_type_t<i2, u2>;
    static_assert(std::is_same_v<ct1, u2>);
    const ct1 c1 = ct1{42};
    TEST("common_type<i2,u2>=u2 stores 42", c1 == u2{42});

    using ct2 = std::common_type_t<i4, u2>;
    static_assert(std::is_same_v<ct2, i4>);
    const ct2 c2 = ct2{-7};
    TEST("common_type<i4,u2>=i4 stores -7", c2 == i4{-7});

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "(plus ~70 compile-time static_asserts above)\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
