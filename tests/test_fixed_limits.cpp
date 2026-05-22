// =============================================================================
// Test: std::numeric_limits<fixed_int_t<N, Sign, Form>>
// Part of int128 Library - Phase 1.81 (Fase MS-INTEROP, T5)
// License: BSL-1.0
// =============================================================================

#include "fixed_int_limits.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
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
// Compile-time checks on flags
// =============================================================================

// is_specialized
static_assert(std::numeric_limits<u2>::is_specialized);
static_assert(std::numeric_limits<i4>::is_specialized);

// is_signed / is_unsigned
static_assert(!std::numeric_limits<u1>::is_signed);
static_assert(!std::numeric_limits<u2>::is_signed);
static_assert(!std::numeric_limits<u4>::is_signed);
static_assert(!std::numeric_limits<u8>::is_signed);
static_assert(std::numeric_limits<i1>::is_signed);
static_assert(std::numeric_limits<i2>::is_signed);
static_assert(std::numeric_limits<i4>::is_signed);
static_assert(std::numeric_limits<i8>::is_signed);

// is_integer / is_exact / is_bounded
static_assert(std::numeric_limits<u2>::is_integer);
static_assert(std::numeric_limits<i2>::is_integer);
static_assert(std::numeric_limits<u2>::is_exact);
static_assert(std::numeric_limits<i2>::is_exact);
static_assert(std::numeric_limits<u2>::is_bounded);
static_assert(std::numeric_limits<i2>::is_bounded);

// is_modulo: true iff unsigned
static_assert(std::numeric_limits<u1>::is_modulo);
static_assert(std::numeric_limits<u2>::is_modulo);
static_assert(std::numeric_limits<u4>::is_modulo);
static_assert(std::numeric_limits<u8>::is_modulo);
static_assert(!std::numeric_limits<i1>::is_modulo);
static_assert(!std::numeric_limits<i2>::is_modulo);
static_assert(!std::numeric_limits<i4>::is_modulo);
static_assert(!std::numeric_limits<i8>::is_modulo);

// radix
static_assert(std::numeric_limits<u2>::radix == 2);
static_assert(std::numeric_limits<i2>::radix == 2);

// =============================================================================
// digits / digits10
// =============================================================================

static_assert(std::numeric_limits<u1>::digits == 64);
static_assert(std::numeric_limits<u2>::digits == 128);
static_assert(std::numeric_limits<u4>::digits == 256);
static_assert(std::numeric_limits<u8>::digits == 512);
static_assert(std::numeric_limits<i1>::digits == 63);   // excludes sign bit
static_assert(std::numeric_limits<i2>::digits == 127);
static_assert(std::numeric_limits<i4>::digits == 255);
static_assert(std::numeric_limits<i8>::digits == 511);

// digits10 = floor(digits * log10(2)), computed via integer arithmetic (30103/100000).
// uint_fixed_t<1> matches std::numeric_limits<uint64_t>::digits10 == 19.
static_assert(std::numeric_limits<u1>::digits10 == 19);
static_assert(std::numeric_limits<u2>::digits10 == 38);
static_assert(std::numeric_limits<u4>::digits10 == 77);
static_assert(std::numeric_limits<u8>::digits10 == 154);
static_assert(std::numeric_limits<i1>::digits10 == 18); // matches int64_t
static_assert(std::numeric_limits<i2>::digits10 == 38); // matches existing int128_tc_t
static_assert(std::numeric_limits<i4>::digits10 == 76);
static_assert(std::numeric_limits<i8>::digits10 == 153);

// max_digits10 (integers: irrelevant, 0)
static_assert(std::numeric_limits<u2>::max_digits10 == 0);
static_assert(std::numeric_limits<i2>::max_digits10 == 0);

// =============================================================================
// min(), max(), lowest() values
// =============================================================================

// min() for unsigned == 0
static_assert(std::numeric_limits<u2>::min() == u2{0});
static_assert(std::numeric_limits<u4>::min() == u4{0});

// max() for unsigned == all-ones
static_assert(std::numeric_limits<u1>::max() == u1{~std::uint64_t{0}});
static_assert(std::numeric_limits<u2>::max() != u2{0});
static_assert(std::numeric_limits<u2>::max() + u2{1} == u2{0}); // wraps (is_modulo)

// min() for signed == 2^(64N-1) with top bit set
static_assert(std::numeric_limits<i2>::min() < i2{0});
static_assert(std::numeric_limits<i2>::max() > i2{0});
static_assert(std::numeric_limits<i1>::min() < i1{0});

// lowest() == min() for integer types
static_assert(std::numeric_limits<u2>::lowest() == std::numeric_limits<u2>::min());
static_assert(std::numeric_limits<i2>::lowest() == std::numeric_limits<i2>::min());
static_assert(std::numeric_limits<i4>::lowest() == std::numeric_limits<i4>::min());

// min() < max() always
static_assert(std::numeric_limits<u2>::min() < std::numeric_limits<u2>::max());
static_assert(std::numeric_limits<i2>::min() < std::numeric_limits<i2>::max());

// =============================================================================
// epsilon, round_error, infinity, NaNs all zero for integers
// =============================================================================

static_assert(std::numeric_limits<u2>::epsilon() == u2{0});
static_assert(std::numeric_limits<u2>::round_error() == u2{0});
static_assert(std::numeric_limits<u2>::infinity() == u2{0});
static_assert(std::numeric_limits<u2>::quiet_NaN() == u2{0});
static_assert(std::numeric_limits<u2>::signaling_NaN() == u2{0});
static_assert(std::numeric_limits<u2>::denorm_min() == u2{0});

// has_* flags
static_assert(!std::numeric_limits<u2>::has_infinity);
static_assert(!std::numeric_limits<u2>::has_quiet_NaN);
static_assert(!std::numeric_limits<u2>::has_signaling_NaN);

// =============================================================================
// Runtime tests for boundary behavior
// =============================================================================

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
    std::cout << "fixed_int_t std::numeric_limits tests (v1.81)\n";
    std::cout << "====================================================================\n";

    // Unsigned wraparound at max() + 1 == 0
    TEST("u2 max() + 1 == 0",        std::numeric_limits<u2>::max() + u2{1} == u2{0});
    TEST("u4 max() + 1 == 0",        std::numeric_limits<u4>::max() + u4{1} == u4{0});
    TEST("u8 max() + 1 == 0",        std::numeric_limits<u8>::max() + u8{1} == u8{0});

    // Unsigned 0 - 1 == max() (wraparound under)
    TEST("u2 0 - 1 == max()",        u2{0} - u2{1} == std::numeric_limits<u2>::max());

    // Signed min() - 1 == max() (wraparound; technically UB for signed but we wrap)
    TEST("i2 max() + 1 == min()",    std::numeric_limits<i2>::max() + i2{1} == std::numeric_limits<i2>::min());

    // For i1, our min/max should equal int64_t's
    TEST("i1 min() lowest bit pattern", std::numeric_limits<i1>::min() == i1{INT64_MIN});
    TEST("i1 max() ==  INT64_MAX",   std::numeric_limits<i1>::max() == i1{INT64_MAX});

    // For u1, max() == UINT64_MAX
    TEST("u1 max() == UINT64_MAX",   std::numeric_limits<u1>::max() == u1{UINT64_MAX});
    TEST("u1 min() == 0",            std::numeric_limits<u1>::min() == u1{0});

    // is_signed parity with std for the equivalent built-in
    TEST("u1 is_signed == uint64_t is_signed",
         std::numeric_limits<u1>::is_signed == std::numeric_limits<std::uint64_t>::is_signed);
    TEST("i1 is_signed == int64_t  is_signed",
         std::numeric_limits<i1>::is_signed == std::numeric_limits<std::int64_t>::is_signed);

    // is_modulo parity (uint should be modulo; int should not)
    TEST("u1 is_modulo == uint64_t is_modulo",
         std::numeric_limits<u1>::is_modulo == std::numeric_limits<std::uint64_t>::is_modulo);

    // digits parity for N=1
    TEST("u1 digits == 64",  std::numeric_limits<u1>::digits == 64);
    TEST("i1 digits == 63",  std::numeric_limits<i1>::digits == 63);

    // digits10 parity for N=1 against built-in 64-bit ints
    TEST("u1 digits10 == uint64_t digits10",
         std::numeric_limits<u1>::digits10 == std::numeric_limits<std::uint64_t>::digits10);
    TEST("i1 digits10 == int64_t  digits10",
         std::numeric_limits<i1>::digits10 == std::numeric_limits<std::int64_t>::digits10);

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "(plus ~50 compile-time static_asserts above)\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
