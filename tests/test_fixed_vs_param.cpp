// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Oracle parity test: fixed_int_t<2> vs int128_param_t
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Verifies that uint_fixed_t<2> and int_fixed_t<2> (new unified template)
// produce bit-identical results to uint128_t and int128_tc_t (old type)
// across all operations.
//
// Memory layout: both types store data[0]=LSB, data[1]=MSB.
// Bridge: fixed.data[{0,1}] must equal param.{low(),high()}.
//
// Sections:
//   0.  Bridge validation (construction from raw limbs)
//   1.  Unsigned comparison (==, !=, <, <=, >, >=)
//   2.  Unsigned arithmetic (+, -, *, /, %)
//   3.  Unsigned bitwise (&, |, ^, ~, <<, >>)
//   4.  Unsigned utilities (is_zero, bit_width/leading_zeros, popcount/count_ones)
//   5.  Unsigned string (to_string, from_string round-trip)
//   6.  Signed construction (positive and negative values)
//   7.  Signed comparison
//   8.  Signed arithmetic (+, -, *, /, %)
//   9.  Signed arithmetic right shift
//   10. Signed string

#include "fixed_width_int_t.hpp"
#include "int128_parameterized.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

using nstd::int_fixed_t;
using nstd::uint_fixed_t;
using nstd::int128_tc_t;
using nstd::uint128_t;
using std::uint64_t;

using u128n = uint_fixed_t<2>;  // new unsigned 128
using i128n = int_fixed_t<2>;   // new signed 128 (two's complement)
using u128o = uint128_t;         // old unsigned 128
using i128o = int128_tc_t;       // old signed 128 (two's complement)

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
        }                                             \
        else                                          \
        {                                             \
            std::cout << "[FAIL] " << (name) << "\n"; \
            ++g_failed;                               \
        }                                             \
    } while (false)

// =============================================================================
// val128 — raw (hi, lo) input format; language for both old and new types
// =============================================================================

struct val128
{
    uint64_t hi{0};
    uint64_t lo{0};
};

// Build new type from raw limbs
static u128n nu(val128 v) { return u128n{std::array<uint64_t, 2>{v.lo, v.hi}}; }
static i128n ni(val128 v) { return i128n{std::array<uint64_t, 2>{v.lo, v.hi}}; }

// Build old type from raw limbs  (int128_param_t ctor takes (high, low))
static u128o ou(val128 v) { return u128o{v.hi, v.lo}; }
static i128o oi(val128 v) { return i128o{v.hi, v.lo}; }

// Compare new vs old: same raw bits?
static bool eq_u(const u128n &a, const u128o &b) { return a.data[0] == b.low() && a.data[1] == b.high(); }
static bool eq_i(const i128n &a, const i128o &b) { return a.data[0] == b.low() && a.data[1] == b.high(); }

// =============================================================================
// Curated test values
// =============================================================================

// Unsigned 128-bit values (raw hi:lo)
constexpr val128 U0   {0, 0};
constexpr val128 U1   {0, 1};
constexpr val128 U2   {0, 2};
constexpr val128 U42  {0, 42};
constexpr val128 U100 {0, 100};
constexpr val128 U7   {0, 7};
constexpr val128 U64M {0, 0xFFFFFFFFFFFFFFFFULL};   // UINT64_MAX
constexpr val128 U2P64{1, 0};                        // 2^64
constexpr val128 U2P64P1{1, 1};                      // 2^64 + 1
constexpr val128 U2P64P42{1, 42};                    // 2^64 + 42
constexpr val128 UMAX1{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL};  // UMAX-1
constexpr val128 UMAX {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // UMAX
constexpr val128 UHEX {0xDEADBEEF00000000ULL, 0x00000000CAFEBABE};     // mixed
constexpr val128 UBIG {0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL};  // large

// Signed 128-bit values (same raw bits, TC interpretation)
constexpr val128 I0   {0, 0};
constexpr val128 I1   {0, 1};
constexpr val128 I2   {0, 2};
constexpr val128 I7   {0, 7};
constexpr val128 I42  {0, 42};
constexpr val128 I100 {0, 100};
constexpr val128 I64M {0, 0x7FFFFFFFFFFFFFFFULL};    // INT64_MAX (positive)
constexpr val128 IMAX {0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // INT128_MAX
constexpr val128 IMIN {0x8000000000000000ULL, 0};    // INT128_MIN
constexpr val128 IM1  {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // -1
constexpr val128 IM7  {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFF9ULL};  // -7
constexpr val128 IM42 {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFD6ULL};  // -42
constexpr val128 INEG {0xFFFFFFFFFFFFFFFFULL, 0};    // -2^64
constexpr val128 IBIG {0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL};  // positive

// =============================================================================
// Section 0: Bridge validation
// =============================================================================

static void section0_bridge()
{
    std::cout << "\n--- Section 0: Bridge Validation ---\n";

    const val128 cases[] = {U0, U1, U42, U64M, U2P64, UMAX, UHEX, UBIG};
    for (const auto &v : cases)
        TEST("u bridge", eq_u(nu(v), ou(v)));

    const val128 icases[] = {I0, I1, I42, IMAX, IMIN, IM1, IM42, IBIG};
    for (const auto &v : icases)
        TEST("i bridge", eq_i(ni(v), oi(v)));
}

// =============================================================================
// Section 1: Unsigned comparison
// =============================================================================

static void section1_unsigned_cmp()
{
    std::cout << "\n--- Section 1: Unsigned Comparison ---\n";

    // Representative pairs: (a, b)
    const val128 pairs[][2] = {
        {U0, U0},      {U0, U1},      {U1, U0},
        {U42, U42},    {U42, U100},   {U100, U42},
        {U64M, U2P64}, {U2P64, U64M}, {U2P64, U2P64P1},
        {UMAX, UMAX},  {UMAX1, UMAX}, {UMAX, U0},
        {UHEX, UBIG},  {UBIG, UHEX},
    };

    for (const auto &p : pairs)
    {
        const auto na = nu(p[0]), nb = nu(p[1]);
        const auto oa = ou(p[0]), ob = ou(p[1]);
        TEST("u==", (na == nb) == (oa == ob));
        TEST("u!=", (na != nb) == (oa != ob));
        TEST("u< ", (na <  nb) == (oa <  ob));
        TEST("u<=", (na <= nb) == (oa <= ob));
        TEST("u> ", (na >  nb) == (oa >  ob));
        TEST("u>=", (na >= nb) == (oa >= ob));
    }
}

// =============================================================================
// Section 2: Unsigned arithmetic
// =============================================================================

static void section2_unsigned_arith()
{
    std::cout << "\n--- Section 2: Unsigned Arithmetic ---\n";

    const val128 add_pairs[][2] = {
        {U0, U0}, {U0, U1}, {U1, U1}, {U42, U100},
        {U64M, U1},          // carry into high limb
        {U64M, U64M},        // carry + large
        {U2P64, U42},        // add into high
        {UMAX, U1},          // overflow wrap
        {UMAX, UMAX},        // max+max
        {UHEX, UBIG},
    };
    for (const auto &p : add_pairs)
    {
        TEST("u+", eq_u(nu(p[0]) + nu(p[1]), ou(p[0]) + ou(p[1])));
        TEST("u-", eq_u(nu(p[0]) - nu(p[1]), ou(p[0]) - ou(p[1])));
    }

    const val128 mul_pairs[][2] = {
        {U0, U42},       {U1, U42},    {U2, U100},
        {U42, U100},     {U64M, U2},   {U64M, U64M},
        {U2P64, U42},    {UHEX, U7},   {UBIG, U2},
        {U2P64P1, U2P64P1},
    };
    for (const auto &p : mul_pairs)
        TEST("u*", eq_u(nu(p[0]) * nu(p[1]), ou(p[0]) * ou(p[1])));

    const val128 div_pairs[][2] = {
        {U0, U1},          {U42, U7},        {U100, U42},
        {U64M, U2},        {U64M, U100},     {U2P64, U2},
        {U2P64P42, U42},   {UMAX, U2P64},    {UMAX, U42},
        {UHEX, U100},      {UBIG, U2P64P1},  {UBIG, U7},
    };
    for (const auto &p : div_pairs)
    {
        TEST("u/", eq_u(nu(p[0]) / nu(p[1]), ou(p[0]) / ou(p[1])));
        TEST("u%", eq_u(nu(p[0]) % nu(p[1]), ou(p[0]) % ou(p[1])));
    }

    // Unary negation (mod 2^128)
    const val128 neg_vals[] = {U0, U1, U42, U64M, U2P64, UMAX, UHEX};
    for (const auto &v : neg_vals)
        TEST("u-u", eq_u(-nu(v), -ou(v)));
}

// =============================================================================
// Section 3: Unsigned bitwise
// =============================================================================

static void section3_unsigned_bitwise()
{
    std::cout << "\n--- Section 3: Unsigned Bitwise ---\n";

    const val128 pairs[][2] = {
        {U0, U0},    {U0, UMAX},   {UMAX, U0},    {UMAX, UMAX},
        {U42, U100}, {U64M, U2P64},{U2P64, U64M}, {UHEX, UBIG},
        {UBIG, UMAX},{U0, UBIG},
    };

    for (const auto &p : pairs)
    {
        TEST("u&", eq_u(nu(p[0]) & nu(p[1]), ou(p[0]) & ou(p[1])));
        TEST("u|", eq_u(nu(p[0]) | nu(p[1]), ou(p[0]) | ou(p[1])));
        TEST("u^", eq_u(nu(p[0]) ^ nu(p[1]), ou(p[0]) ^ ou(p[1])));
    }

    const val128 unary_vals[] = {U0, U1, U42, U64M, U2P64, UMAX, UHEX, UBIG};
    for (const auto &v : unary_vals)
        TEST("u~", eq_u(~nu(v), ~ou(v)));

    // Shifts
    const unsigned shifts[] = {0, 1, 7, 31, 32, 63, 64, 65, 95, 127, 128, 129};
    const val128 shift_vals[] = {U1, U42, U64M, U2P64, UHEX, UBIG, UMAX};
    for (const auto &v : shift_vals)
    {
        for (unsigned sh : shifts)
        {
            TEST("u<<", eq_u(nu(v) << sh, ou(v) << static_cast<int>(sh)));
            TEST("u>>", eq_u(nu(v) >> sh, ou(v) >> static_cast<int>(sh)));
        }
    }
}

// =============================================================================
// Section 4: Unsigned utilities
// =============================================================================

static void section4_unsigned_utils()
{
    std::cout << "\n--- Section 4: Unsigned Utilities ---\n";

    const val128 vals[] = {U0, U1, U42, U64M, U2P64, U2P64P1, UMAX, UHEX, UBIG};
    for (const auto &v : vals)
    {
        const auto n = nu(v);
        const auto o = ou(v);
        TEST("u is_zero", n.is_zero() == o.is_zero());
        // bit_width(new) == 128 - leading_zeros(old), except both are 0 when value=0
        const unsigned bw_n = n.bit_width();
        const unsigned lz_o = static_cast<unsigned>(o.leading_zeros());
        TEST("u bit_width", bw_n == (v.hi == 0 && v.lo == 0 ? 0u : 128u - lz_o));
        // popcount(new) == count_ones(old)
        TEST("u popcount", n.popcount() == static_cast<unsigned>(o.count_ones()));
    }
}

// =============================================================================
// Section 5: Unsigned string
// =============================================================================

static void section5_unsigned_string()
{
    std::cout << "\n--- Section 5: Unsigned String ---\n";

    const val128 vals[] = {U0, U1, U42, U100, U64M, U2P64, U2P64P42, UMAX1, UMAX, UHEX, UBIG};
    for (const auto &v : vals)
    {
        const std::string sn = nu(v).to_string();
        const std::string so = ou(v).to_string();
        TEST("u to_string", sn == so);

        // Round-trip: from_string of new result reconstructs same bits
        const u128n rt = u128n::from_string(sn.c_str());
        TEST("u from_string rt", eq_u(rt, ou(v)));
    }
}

// =============================================================================
// Section 6: Signed construction
// =============================================================================

static void section6_signed_ctor()
{
    std::cout << "\n--- Section 6: Signed Construction ---\n";

    // From int64_t
    const std::int64_t ints[] = {0, 1, -1, 42, -42, 1000, -1000,
                                  0x7FFFFFFFFFFFFFFFLL, // INT64_MAX
                                  static_cast<std::int64_t>(0x8000000000000000ULL)}; // INT64_MIN
    for (const auto v : ints)
    {
        const i128n n{v};
        const i128o o{v};
        TEST("i from int64", eq_i(n, o));
    }

    // max / min
    TEST("i max", eq_i(i128n::max(), i128o::max()));
    TEST("i min", eq_i(i128n::min(), i128o::min()));

    // From raw limbs
    const val128 ivals[] = {I0, I1, I42, IMAX, IMIN, IM1, IM42, INEG, IBIG};
    for (const auto &v : ivals)
        TEST("i bridge", eq_i(ni(v), oi(v)));
}

// =============================================================================
// Section 7: Signed comparison
// =============================================================================

static void section7_signed_cmp()
{
    std::cout << "\n--- Section 7: Signed Comparison ---\n";

    const val128 pairs[][2] = {
        {I0, I0},     {I0, I1},     {I1, I0},
        {I42, I42},   {IM1, I0},    {I0, IM1},
        {IM42, IM1},  {IM1, IM42},
        {IMIN, IMAX}, {IMAX, IMIN},
        {IMIN, I0},   {I0, IMIN},
        {IMAX, I0},   {IM42, I42},
        {INEG, IM1},  {IM1, INEG},
    };

    for (const auto &p : pairs)
    {
        const auto na = ni(p[0]), nb = ni(p[1]);
        const auto oa = oi(p[0]), ob = oi(p[1]);
        TEST("i==", (na == nb) == (oa == ob));
        TEST("i!=", (na != nb) == (oa != ob));
        TEST("i< ", (na <  nb) == (oa <  ob));
        TEST("i<=", (na <= nb) == (oa <= ob));
        TEST("i> ", (na >  nb) == (oa >  ob));
        TEST("i>=", (na >= nb) == (oa >= ob));
    }
}

// =============================================================================
// Section 8: Signed arithmetic
// =============================================================================

static void section8_signed_arith()
{
    std::cout << "\n--- Section 8: Signed Arithmetic ---\n";

    const val128 add_pairs[][2] = {
        {I0, I0},      {I0, I1},     {I1, IM1},
        {I42, IM42},   {IM42, I42},
        {I42, I42},    {IM1, IM1},
        {IMAX, I1},    // overflow wrap
        {IMIN, IM1},   // overflow wrap
        {IMAX, IMAX},  {IMIN, IMIN},
        {IBIG, IM42},  {IM42, IBIG},
        {INEG, I42},
    };
    for (const auto &p : add_pairs)
    {
        TEST("i+", eq_i(ni(p[0]) + ni(p[1]), oi(p[0]) + oi(p[1])));
        TEST("i-", eq_i(ni(p[0]) - ni(p[1]), oi(p[0]) - oi(p[1])));
    }

    const val128 mul_pairs[][2] = {
        {I0, I42},    {I1, I42},     {I42, IM1},
        {IM1, I42},   {IM42, IM42},  {I42, I42},
        {I64M, I2},   {IM42, I100},  {IBIG, I1},
        {IBIG, IM1},  {IMIN, IM1},   // -MIN wraps back to MIN
    };
    for (const auto &p : mul_pairs)
        TEST("i*", eq_i(ni(p[0]) * ni(p[1]), oi(p[0]) * oi(p[1])));

    const val128 div_pairs[][2] = {
        {I0, I1},      {I42, I7},       {IM42, I7},
        {I42, IM7},    {IM42, IM7},
        {IBIG, I42},   {IBIG, IM42},
        {IMIN, IM1},   // -MIN / -1 = MIN (overflow wraps back)
    };
    for (const auto &p : div_pairs)
    {
        TEST("i/", eq_i(ni(p[0]) / ni(p[1]), oi(p[0]) / oi(p[1])));
        TEST("i%", eq_i(ni(p[0]) % ni(p[1]), oi(p[0]) % oi(p[1])));
    }

    // Unary negation
    const val128 neg_vals[] = {I0, I1, IM1, I42, IM42, IMAX, IMIN};
    for (const auto &v : neg_vals)
        TEST("i-u", eq_i(-ni(v), -oi(v)));
}

// =============================================================================
// Section 9: Signed arithmetic right shift
// =============================================================================

static void section9_signed_shift()
{
    std::cout << "\n--- Section 9: Signed Shift ---\n";

    const unsigned shifts[] = {0, 1, 7, 31, 32, 63, 64, 65, 95, 126, 127, 128};
    const val128 vals[] = {I0, I1, I42, IMAX, IMIN, IM1, IM42, IBIG};

    for (const auto &v : vals)
    {
        for (unsigned sh : shifts)
        {
            TEST("i<<", eq_i(ni(v) << sh, oi(v) << static_cast<int>(sh)));
            TEST("i>>", eq_i(ni(v) >> sh, oi(v) >> static_cast<int>(sh)));
        }
    }
}

// =============================================================================
// Section 10: Signed string
// =============================================================================

static void section10_signed_string()
{
    std::cout << "\n--- Section 10: Signed String ---\n";

    const val128 vals[] = {I0, I1, I42, IM1, IM42, I64M, IMAX, IMIN, IBIG};
    for (const auto &v : vals)
    {
        const std::string sn = ni(v).to_string();
        const std::string so = oi(v).to_string();
        TEST("i to_string", sn == so);

        // Round-trip
        const i128n rt = i128n::from_string(sn.c_str());
        TEST("i from_string rt", eq_i(rt, oi(v)));
    }
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=== Oracle Parity Test: fixed_int_t<2> vs int128_param_t ===\n";

    section0_bridge();
    section1_unsigned_cmp();
    section2_unsigned_arith();
    section3_unsigned_bitwise();
    section4_unsigned_utils();
    section5_unsigned_string();
    section6_signed_ctor();
    section7_signed_cmp();
    section8_signed_arith();
    section9_signed_shift();
    section10_signed_string();

    std::cout << "\n=== SUMMARY: " << g_passed << " passed, " << g_failed << " failed ===\n";
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
