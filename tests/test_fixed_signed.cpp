// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int_fixed_t<N> — signed fixed-width integer template
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Covers N=1 (64-bit), N=2 (128-bit), N=4 (256-bit), N=8 (512-bit).
// Sections:
//   1.  Construction and zero/one/max_val/min_val
//   2.  Signed comparison
//   3.  Bitwise: ~, &, |, ^, <<, >> (arithmetic)
//   4.  Addition, subtraction, negation, increment/decrement
//   5.  Multiplication
//   6.  Division and modulo (truncation-toward-zero)
//   7.  String: to_string and from_string round-trips
//   8.  Bit utilities: is_positive, signum, clz, ctz, bit_width, popcount
//   9.  Mixed-type operators (int_fixed_t op integral T)
//   9b. Cross-N operators (int_fixed_t<N> op int_fixed_t<M>, N != M)
//   10. Higher arithmetic: mul_wide, pow, gcd, lcm, checked_add/sub/mul

#include "fixed_width_int_t.hpp"

#include <cstdlib>
#include <iostream>

using nstd::int_fixed_t;
using nstd::uint_fixed_t;

// =============================================================================
// Test framework
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

// =============================================================================
// Section 1: Construction, zero / one / max_val / min_val
// =============================================================================

template <std::size_t N>
static void test_construction(const char *tag)
{
    std::cout << "\n--- Section 1: Construction [N=" << N << " " << tag << "] ---\n";

    const auto z = int_fixed_t<N>::zero();
    const auto o = int_fixed_t<N>::one();
    const auto mx = int_fixed_t<N>::max_val();
    const auto mn = int_fixed_t<N>::min_val();

    TEST("zero.is_zero()", z.is_zero());
    TEST("one  not zero", !o.is_zero());
    TEST("max  not zero", !mx.is_zero());
    TEST("min  not zero", !mn.is_zero());

    TEST("zero not negative", !z.is_negative());
    TEST("one  not negative", !o.is_negative());
    TEST("max  not negative", !mx.is_negative());
    TEST("min  is  negative", mn.is_negative());

    // max_val MSB of data[N-1] must be 0
    TEST("max_val MSB==0", (mx.bits.data[N - 1] >> 63) == 0);
    // max_val lower limbs all 0xFF (for N=1 the only limb is INT64_MAX, not ~0)
    if constexpr (N > 1) TEST("max_val data[0]==~0", mx.bits.data[0] == ~std::uint64_t{0});
    // min_val: only MSB of data[N-1] set, rest 0
    TEST("min_val data[N-1] MSB==1", (mn.bits.data[N - 1] >> 63) == 1);
    if constexpr (N > 1) TEST("min_val data[0]==0", mn.bits.data[0] == 0);

    // Default constructor gives zero
    const int_fixed_t<N> def{};
    TEST("default == zero", def == z);

    // From int64_t: positive
    const int_fixed_t<N> v42{std::int64_t{42}};
    TEST("from_i64(42) data[0]==42", v42.bits.data[0] == 42);
    if constexpr (N > 1) TEST("from_i64(42) data[N-1]==0", v42.bits.data[N - 1] == 0);
    TEST("from_i64(42) not negative", !v42.is_negative());

    // From int64_t: negative
    const int_fixed_t<N> vm1{std::int64_t{-1}};
    TEST("from_i64(-1) data[0]==~0", vm1.bits.data[0] == ~std::uint64_t{0});
    TEST("from_i64(-1) data[N-1]==~0", vm1.bits.data[N - 1] == ~std::uint64_t{0});
    TEST("from_i64(-1) is negative", vm1.is_negative());

    // From uint_fixed_t (bit reinterpret)
    const uint_fixed_t<N> umax = uint_fixed_t<N>::max();
    const int_fixed_t<N> as_signed{umax};
    TEST("uint_max as_signed is negative", as_signed.is_negative());
    TEST("uint_max as_signed == -1", as_signed == vm1);
}

// =============================================================================
// Section 2: Signed comparison
// =============================================================================

template <std::size_t N>
static void test_comparison(const char *tag)
{
    std::cout << "\n--- Section 2: Signed comparison [N=" << N << " " << tag << "] ---\n";

    const auto z = int_fixed_t<N>::zero();
    const auto o = int_fixed_t<N>::one();
    const auto mx = int_fixed_t<N>::max_val();
    const auto mn = int_fixed_t<N>::min_val();
    const int_fixed_t<N> m1{std::int64_t{-1}};

    TEST("zero == zero", z == z);
    TEST("one  == one", o == o);
    TEST("max  == max", mx == mx);
    TEST("min  == min", mn == mn);
    TEST("zero != one", z != o);
    TEST("zero != m1", z != m1);

    // Signed ordering: min < -1 < 0 < 1 < max
    TEST("min < -1", mn < m1);
    TEST("min < zero", mn < z);
    TEST("min < one", mn < o);
    TEST("min < max", mn < mx);
    TEST("-1  < zero", m1 < z);
    TEST("-1  < one", m1 < o);
    TEST("-1  < max", m1 < mx);
    TEST("zero < one", z < o);
    TEST("zero < max", z < mx);
    TEST("one  < max", o < mx);

    TEST("max > zero", mx > z);
    TEST("zero > min", z > mn);
    TEST("zero > -1", z > m1);

    TEST("zero >= zero", z >= z);
    TEST("one  >= zero", o >= z);
    TEST("max  >= max", mx >= mx);
    TEST("min  <= min", mn <= mn);
    TEST("min  <= zero", mn <= z);

    // Cross-sign: positive always > negative
    const int_fixed_t<N> two{std::int64_t{2}};
    const int_fixed_t<N> m2{std::int64_t{-2}};
    TEST("2 > -2", two > m2);
    TEST("-2 < 2", m2 < two);
    TEST("-1 < 1", m1 < o);

    // Same sign, magnitude dominates
    TEST("-1 > -2", m1 > m2);
    TEST("-2 < -1", m2 < m1);
}

// =============================================================================
// Section 3: Bitwise and arithmetic right shift
// =============================================================================

template <std::size_t N>
static void test_bitwise(const char *tag)
{
    std::cout << "\n--- Section 3: Bitwise [N=" << N << " " << tag << "] ---\n";

    const int_fixed_t<N> z = int_fixed_t<N>::zero();
    const int_fixed_t<N> m1{std::int64_t{-1}}; // all bits set
    const int_fixed_t<N> one = int_fixed_t<N>::one();

    // ~0 == -1 (all bits set), ~(-1) == 0
    TEST("~zero == -1", ~z == m1);
    TEST("~(-1) == zero", ~m1 == z);

    // AND, OR, XOR
    TEST("-1 & zero == zero", (m1 & z) == z);
    TEST("-1 & -1   == -1", (m1 & m1) == m1);
    TEST("-1 | zero == -1", (m1 | z) == m1);
    TEST("zero | zero == zero", (z | z) == z);
    TEST("-1 ^ -1   == zero", (m1 ^ m1) == z);
    TEST("-1 ^ zero == -1", (m1 ^ z) == m1);

    // Left shift (logical): -1 << 1 loses MSB, fills 0 at low end
    const int_fixed_t<N> m1_shl1 = m1 << 1;
    TEST("-1 << 1 == -2", m1_shl1 == int_fixed_t<N>{std::int64_t{-2}});

    // Arithmetic right shift: positive fills 0
    TEST("4 >> 1 == 2",
         int_fixed_t<N>{std::int64_t{4}} >> 1 == int_fixed_t<N>{std::int64_t{2}});
    TEST("1 >> 1 == 0",
         int_fixed_t<N>{std::int64_t{1}} >> 1 == z);

    // Arithmetic right shift: negative fills 1
    TEST("-1 >> 1 == -1", (m1 >> 1) == m1);
    TEST("-1 >> 63 == -1", (m1 >> 63) == m1);
    TEST("-2 >> 1 == -1",
         (int_fixed_t<N>{std::int64_t{-2}} >> 1) == m1);
    TEST("-4 >> 1 == -2",
         (int_fixed_t<N>{std::int64_t{-4}} >> 1) == int_fixed_t<N>{std::int64_t{-2}});

    // Shift by >= 64N → all zeros (positive) or all ones (negative)
    TEST("1 >> 64N == 0", (one >> (64U * N)) == z);
    TEST("-1 >> 64N == -1", (m1 >> (64U * N)) == m1);

    // Shift by 0 is identity
    TEST("-1 >> 0 == -1", (m1 >> 0) == m1);
    TEST("1 << 0 == 1", (one << 0) == one);

    // Cross-limb arithmetic shift for N>=2
    if constexpr (N >= 2)
    {
        // -1 (all ones) >> 64 == -1 (still all ones)
        TEST("-1 >> 64 == -1", (m1 >> 64) == m1);
        // 2^63 >> 63 == 1 (positive, no fill)
        const int_fixed_t<N> pow63{std::int64_t{std::int64_t{1} << 62}};
        TEST("2^62 >> 62 == 1", (pow63 >> 62) == one);
    }
}

// =============================================================================
// Section 4: Addition, subtraction, negation, inc/dec
// =============================================================================

template <std::size_t N>
static void test_addsub(const char *tag)
{
    std::cout << "\n--- Section 4: Add/Sub/Neg [N=" << N << " " << tag << "] ---\n";

    const auto z = int_fixed_t<N>::zero();
    const auto o = int_fixed_t<N>::one();
    const auto mx = int_fixed_t<N>::max_val();
    const auto mn = int_fixed_t<N>::min_val();
    const int_fixed_t<N> m1{std::int64_t{-1}};
    const int_fixed_t<N> m2{std::int64_t{-2}};
    const int_fixed_t<N> two{std::int64_t{2}};

    // Identity
    TEST("zero + zero == zero", (z + z) == z);
    TEST("zero + one  == one", (z + o) == o);
    TEST("one  + zero == one", (o + z) == o);

    // Known values
    TEST("1 + (-1) == 0", (o + m1) == z);
    TEST("(-1)+(-1) == -2", (m1 + m1) == m2);
    TEST("2 + (-1) == 1", (two + m1) == o);

    // Subtraction
    TEST("1 - 1 == 0", (o - o) == z);
    TEST("0 - 1 == -1", (z - o) == m1);
    TEST("(-1) - 1 == -2", (m1 - o) == m2);

    // Negation
    TEST("-zero == zero", (-z) == z);
    TEST("-one  == -1", (-o) == m1);
    TEST("-(-1) == 1", (-m1) == o);
    TEST("-(-one) == one", (-(-o)) == o);

    // max_val + 1 wraps to min_val (overflow wraps)
    TEST("max+1 == min (wrap)", (mx + o) == mn);
    // min_val - 1 wraps to max_val
    TEST("min-1 == max (wrap)", (mn - o) == mx);

    // round-trip: a - b + b == a
    const int_fixed_t<N> a{std::int64_t{0x123456789ABCDEFLL}};
    const int_fixed_t<N> b{std::int64_t{-0x0FEDCBA987654321LL}};
    TEST("(a-b)+b == a", ((a - b) + b) == a);

    // Increment / decrement
    auto v = z;
    TEST("prefix ++ from zero gives one", (++v) == o);
    TEST("after prefix ++ v==one", v == o);

    auto w = z;
    const auto post = w++;
    TEST("postfix ++ returns zero (old)", post == z);
    TEST("after postfix ++ w==one", w == o);

    auto x = o;
    TEST("prefix -- from one gives zero", (--x) == z);
    TEST("after prefix -- x==zero", x == z);

    auto y = o;
    const auto post2 = y--;
    TEST("postfix -- returns one (old)", post2 == o);
    TEST("after postfix -- y==zero", y == z);

    // Wrap: max++ == min, min-- == max
    auto mxv = mx;
    TEST("max++ == min (wrap)", (++mxv) == mn);
    auto mnv = mn;
    TEST("min-- == max (wrap)", (--mnv) == mx);
}

// =============================================================================
// Section 5: Multiplication
// =============================================================================

template <std::size_t N>
static void test_multiply(const char *tag)
{
    std::cout << "\n--- Section 5: Multiplication [N=" << N << " " << tag << "] ---\n";

    const auto z = int_fixed_t<N>::zero();
    const auto o = int_fixed_t<N>::one();
    const int_fixed_t<N> m1{std::int64_t{-1}};
    const int_fixed_t<N> three{std::int64_t{3}};
    const int_fixed_t<N> m3{std::int64_t{-3}};
    const int_fixed_t<N> seven{std::int64_t{7}};
    const int_fixed_t<N> m7{std::int64_t{-7}};
    const int_fixed_t<N> twentyone{std::int64_t{21}};
    const int_fixed_t<N> m21{std::int64_t{-21}};

    TEST("a * one  == a", (three * o) == three);
    TEST("one  * a == a", (o * three) == three);
    TEST("a * zero == zero", (three * z) == z);
    TEST("zero * a == zero", (z * three) == z);

    // Sign rules
    TEST("3 * 7   == 21", (three * seven) == twentyone);
    TEST("3 * (-7) == -21", (three * m7) == m21);
    TEST("(-3)*7   == -21", (m3 * seven) == m21);
    TEST("(-3)*(-7)==21", (m3 * m7) == twentyone);

    // (-1)*(-1) == 1
    TEST("(-1)*(-1)==1", (m1 * m1) == o);

    // Commutativity
    const int_fixed_t<N> a{std::int64_t{1234567}};
    const int_fixed_t<N> mb{std::int64_t{-9876543}};
    TEST("a * mb == mb * a", (a * mb) == (mb * a));

    // Distributivity: a*(b+c) == a*b + a*c
    const int_fixed_t<N> c{std::int64_t{111}};
    TEST("distributivity", (a * (mb + c)) == (a * mb + a * c));
}

// =============================================================================
// Section 6: Division and modulo (truncation-toward-zero)
// =============================================================================

template <std::size_t N>
static void test_divmod(const char *tag)
{
    std::cout << "\n--- Section 6: Div/Mod [N=" << N << " " << tag << "] ---\n";

    const auto z = int_fixed_t<N>::zero();
    const auto o = int_fixed_t<N>::one();
    const int_fixed_t<N> m1{std::int64_t{-1}};
    const int_fixed_t<N> seven{std::int64_t{7}};
    const int_fixed_t<N> m7{std::int64_t{-7}};
    const int_fixed_t<N> three{std::int64_t{3}};
    const int_fixed_t<N> m3{std::int64_t{-3}};
    const int_fixed_t<N> twentyone{std::int64_t{21}};
    const int_fixed_t<N> m21{std::int64_t{-21}};
    const int_fixed_t<N> hundred{std::int64_t{100}};
    const int_fixed_t<N> m100{std::int64_t{-100}};
    const int_fixed_t<N> ten{std::int64_t{10}};
    const int_fixed_t<N> m10{std::int64_t{-10}};

    // Basic positive
    TEST("21/7 == 3", twentyone / seven == three);
    TEST("21%7 == 0", twentyone % seven == z);
    TEST("21/3 == 7", twentyone / three == seven);
    TEST("100/10 == 10", hundred / ten == ten);
    TEST("100%10 == 0", hundred % ten == z);
    TEST("7/7 == 1", seven / seven == o);
    TEST("7%7 == 0", seven % seven == z);
    TEST("3/7 == 0", three / seven == z);
    TEST("3%7 == 3", three % seven == three);

    // Sign rules (truncation toward zero, C++ semantics)
    // 21 / (-7) == -3
    TEST("21/(-7) == -3", twentyone / m7 == m3);
    TEST("21%(-7) == 0", twentyone % m7 == z);
    // (-21) / 7 == -3
    TEST("(-21)/7 == -3", m21 / seven == m3);
    TEST("(-21)%7 == 0", m21 % seven == z);
    // (-21) / (-7) == 3
    TEST("(-21)/(-7) == 3", m21 / m7 == three);
    TEST("(-21)%(-7) == 0", m21 % m7 == z);

    // 100 / (-10) == -10
    TEST("100/(-10) == -10", hundred / m10 == m10);
    TEST("100%(-10) == 0", hundred % m10 == z);
    // (-100) / 10 == -10
    TEST("(-100)/10 == -10", m100 / ten == m10);
    TEST("(-100)%10 == 0", m100 % ten == z);

    // Fundamental theorem with signs: a == (a/b)*b + (a%b)
    auto theorem = [](std::int64_t av, std::int64_t bv) -> bool
    {
        const int_fixed_t<N> a{av};
        const int_fixed_t<N> b{bv};
        const auto [q, r] = int_fixed_t<N>::divmod(a, b);
        // C++ semantics: remainder has same sign as dividend
        const bool rem_sign_ok = r.is_zero() || (r.is_negative() == a.is_negative());
        return (q * b + r) == a && rem_sign_ok;
    };

    TEST("theorem: 7/3", theorem(7, 3));
    TEST("theorem: -7/3", theorem(-7, 3));
    TEST("theorem: 7/(-3)", theorem(7, -3));
    TEST("theorem: -7/(-3)", theorem(-7, -3));
    TEST("theorem: 100/7", theorem(100, 7));
    TEST("theorem: -100/7", theorem(-100, 7));
    TEST("theorem: 100/(-7)", theorem(100, -7));
    TEST("theorem: -100/(-7)", theorem(-100, -7));
    TEST("theorem: 1/1", theorem(1, 1));
    TEST("theorem: -1/1", theorem(-1, 1));
    TEST("theorem: -1/(-1)", theorem(-1, -1));

    // Division by zero throws
    auto throws_domain = [](auto &&fn) -> bool
    {
        try
        {
            fn();
            return false;
        }
        catch (const std::domain_error &)
        {
            return true;
        }
        catch (...)
        {
            return false;
        }
    };
    TEST("1/0 throws", throws_domain([&]
                                     { (void)(o / z); }));
    TEST("-1/0 throws", throws_domain([&]
                                      { (void)(m1 / z); }));
    TEST("divmod(1,0) throws", throws_domain([&]
                                             { (void)int_fixed_t<N>::divmod(o, z); }));
}

// =============================================================================
// Section 7: String conversion
// =============================================================================

template <std::size_t N>
static void test_strings(const char *tag)
{
    std::cout << "\n--- Section 7: Strings [N=" << N << " " << tag << "] ---\n";

    const auto z = int_fixed_t<N>::zero();
    const auto o = int_fixed_t<N>::one();
    const int_fixed_t<N> m1{std::int64_t{-1}};

    TEST("zero.to_string()==\"0\"", z.to_string() == "0");
    TEST("one .to_string()==\"1\"", o.to_string() == "1");
    TEST("-1  .to_string()==\"-1\"", m1.to_string() == "-1");

    // Known value round-trips
    const int_fixed_t<N> pos{std::int64_t{12345678901234LL}};
    TEST("positive round-trip",
         int_fixed_t<N>::from_string("12345678901234") == pos);

    const int_fixed_t<N> neg{std::int64_t{-12345678901234LL}};
    TEST("negative round-trip",
         int_fixed_t<N>::from_string("-12345678901234") == neg);

    // max_val and min_val round-trips
    {
        const auto mx = int_fixed_t<N>::max_val();
        TEST("max_val round-trip",
             int_fixed_t<N>::from_string(mx.to_string().c_str()) == mx);
    }
    {
        const auto mn = int_fixed_t<N>::min_val();
        TEST("min_val round-trip",
             int_fixed_t<N>::from_string(mn.to_string().c_str()) == mn);
    }

    // parse zero
    TEST("from_string(\"0\")==zero", int_fixed_t<N>::from_string("0") == z);

    // Invalid input throws
    auto check_invalid = [](const char *s) -> bool
    {
        try
        {
            (void)int_fixed_t<N>::from_string(s);
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
        catch (...)
        {
            return false;
        }
    };
    TEST("invalid: \"abc\"", check_invalid("abc"));
    TEST("invalid: \"\"", check_invalid(""));
    TEST("invalid: \"-\"", check_invalid("-"));
    TEST("invalid: \"-abc\"", check_invalid("-abc"));
}

// =============================================================================
// Section 8: Bit utilities — is_positive, signum, clz, ctz, bit_width, popcount
// =============================================================================

template <std::size_t N>
static void test_bit_utilities(const char *tag)
{
    std::cout << "\n--- Section 8: Bit utilities [N=" << N << " " << tag << "] ---\n";

    const auto z  = int_fixed_t<N>::zero();
    const auto o  = int_fixed_t<N>::one();
    const auto mx = int_fixed_t<N>::max_val();
    const auto mn = int_fixed_t<N>::min_val();
    const int_fixed_t<N> m1{std::int64_t{-1}};
    const int_fixed_t<N> two{std::int64_t{2}};

    // is_positive
    TEST("is_positive(zero)==false",    !z.is_positive());
    TEST("is_positive(one)==true",      o.is_positive());
    TEST("is_positive(-1)==false",      !m1.is_positive());
    TEST("is_positive(max_val)==true",  mx.is_positive());
    TEST("is_positive(min_val)==false", !mn.is_positive());

    // signum
    TEST("signum(zero)==0",     z.signum() == 0);
    TEST("signum(one)==1",      o.signum() == 1);
    TEST("signum(-1)==-1",      m1.signum() == -1);
    TEST("signum(max_val)==1",  mx.signum() == 1);
    TEST("signum(min_val)==-1", mn.signum() == -1);

    // count_leading_zeros (bit pattern, not sign-aware)
    TEST("clz(zero)==64N",    z.count_leading_zeros() == 64U * N);
    TEST("clz(one)==64N-1",   o.count_leading_zeros() == 64U * N - 1U);
    TEST("clz(max_val)==1",   mx.count_leading_zeros() == 1U);
    TEST("clz(-1)==0",        m1.count_leading_zeros() == 0U);

    // count_trailing_zeros
    TEST("ctz(zero)==64N", z.count_trailing_zeros() == 64U * N);
    TEST("ctz(one)==0",    o.count_trailing_zeros() == 0U);
    TEST("ctz(-1)==0",     m1.count_trailing_zeros() == 0U);
    TEST("ctz(2)==1",      two.count_trailing_zeros() == 1U);

    // bit_width (raw bit pattern)
    TEST("bit_width(zero)==0",        z.bit_width() == 0U);
    TEST("bit_width(one)==1",         o.bit_width() == 1U);
    TEST("bit_width(max_val)==64N-1", mx.bit_width() == 64U * N - 1U);
    TEST("bit_width(-1)==64N",        m1.bit_width() == 64U * N);

    // popcount
    TEST("popcount(zero)==0",        z.popcount() == 0U);
    TEST("popcount(one)==1",         o.popcount() == 1U);
    TEST("popcount(-1)==64N",        m1.popcount() == 64U * N);
    TEST("popcount(max_val)==64N-1", mx.popcount() == 64U * N - 1U);
}

// =============================================================================
// Run all sections for one N
// =============================================================================

// =============================================================================
// Section 9: Mixed-type operators (int_fixed_t op integral T)
// =============================================================================

template <std::size_t N>
static void test_mixed_ops_int(const char *tag)
{
    std::cout << "\n--- Section 9: Mixed-type ops [N=" << N << " " << tag << "] ---\n";

    const int_fixed_t<N> five{std::int64_t{5}};
    const int_fixed_t<N> neg3{std::int64_t{-3}};
    const int_fixed_t<N> ten{std::int64_t{10}};

    // arithmetic free functions
    TEST("five+3==8",   (five + 3) == int_fixed_t<N>{std::int64_t{8}});
    TEST("3+five==8",   (3 + five) == int_fixed_t<N>{std::int64_t{8}});
    TEST("five-3==2",   (five - 3) == int_fixed_t<N>{std::int64_t{2}});
    TEST("3-five==-2",  (3 - five) == int_fixed_t<N>{std::int64_t{-2}});
    TEST("five*(-3)",   (five * (-3)) == int_fixed_t<N>{std::int64_t{-15}});
    TEST("(-3)*five",   ((-3) * five) == int_fixed_t<N>{std::int64_t{-15}});
    TEST("ten/3==3",    (ten / 3) == int_fixed_t<N>{std::int64_t{3}});
    TEST("10/neg3==-3", (10 / neg3) == int_fixed_t<N>{std::int64_t{-3}});
    TEST("ten%3==1",    (ten % 3) == int_fixed_t<N>{std::int64_t{1}});
    TEST("10%neg3==1",  (10 % neg3) == int_fixed_t<N>{std::int64_t{1}});

    // comparison free functions
    TEST("five==5",   (five == 5));
    TEST("5==five",   (5 == five));
    TEST("five!=6",   (five != 6));
    TEST("five<6",    (five < 6));
    TEST("4<five",    (4 < five));
    TEST("five>4",    (five > 4));
    TEST("five>=5",   (five >= 5));
    TEST("five<=5",   (five <= 5));
    TEST("neg3<0",    (neg3 < 0));
    TEST("0>neg3",    (0 > neg3));

    // bitwise free functions
    const int_fixed_t<N> oxf{std::int64_t{0xF}};
    TEST("oxf&3==3",     (oxf & 3) == int_fixed_t<N>{std::int64_t{3}});
    TEST("3&oxf==3",     (3 & oxf) == int_fixed_t<N>{std::int64_t{3}});
    TEST("oxf|0x10",     (oxf | 0x10) == int_fixed_t<N>{std::int64_t{0x1F}});
    TEST("oxf^3==0xC",   (oxf ^ 3) == int_fixed_t<N>{std::int64_t{0xC}});

    // compound assignments
    int_fixed_t<N> x{std::int64_t{7}};
    x += 3;   TEST("x+=3→10",  x == int_fixed_t<N>{std::int64_t{10}});
    x -= 4;   TEST("x-=4→6",   x == int_fixed_t<N>{std::int64_t{6}});
    x *= (-2); TEST("x*=-2→-12", x == int_fixed_t<N>{std::int64_t{-12}});
    x /= 3;   TEST("x/=3→-4",  x == int_fixed_t<N>{std::int64_t{-4}});
    x %= 3;   TEST("x%=3→-1",  x == int_fixed_t<N>{std::int64_t{-1}});

#ifdef __SIZEOF_INT128__
    if constexpr (N >= 2)
    {
        const __int128 big = ((__int128)1 << 65) + 7;
        const int_fixed_t<N> big_f{big};
        TEST("big_f+big==2*big_f", (big_f + big) == (big_f + big_f));
        TEST("big==big_f",         (big == big_f));
        TEST("big_f==big",         (big_f == big));
        TEST("big_f<big+1",        (big_f < (big + 1)));
    }
#endif
}

// =============================================================================
// Section 10: Higher arithmetic — mul_wide, pow, gcd, lcm, checked_*
// =============================================================================

template <std::size_t N>
static void test_higher_arith_int(const char *tag)
{
    std::cout << "\n--- Section 10: Higher arithmetic (signed) [N=" << N << " " << tag << "] ---\n";

    using I  = nstd::int_fixed_t<N>;
    using I2 = nstd::int_fixed_t<2 * N>;
    using U  = nstd::uint_fixed_t<N>;

    // mul_wide (signed)
    {
        const I2 w1 = nstd::mul_wide(I{-3}, I{5});
        TEST("mul_wide(-3,5)==-15",  w1 == I2{-15});
        const I2 w2 = nstd::mul_wide(I{-3}, I{-5});
        TEST("mul_wide(-3,-5)==15",  w2 == I2{15});
        TEST("mul_wide(1,1)==1",     nstd::mul_wide(I{1}, I{1}) == I2{1});
        TEST("mul_wide(0,-7)==0",    nstd::mul_wide(I{}, I{-7}) == I2{});
    }

    // pow (signed base, unsigned exponent)
    {
        const U u3{3};
        const U u4{4};
        TEST("pow(-2,3)==-8",  nstd::pow(I{-2}, U{3}) == I{-8});
        TEST("pow(3,4)==81",   nstd::pow(I{3},  u4)   == I{81});
        TEST("pow(-1,2)==1",   nstd::pow(I{-1}, U{2}) == I{1});
        TEST("pow(-1,3)==-1",  nstd::pow(I{-1}, u3)   == I{-1});
        TEST("pow(base,0)==1", nstd::pow(I{99}, U{})  == I{1});
    }

    // gcd (signed delegates to unsigned abs)
    {
        TEST("gcd(-6,9)==3",    nstd::gcd(I{-6},  I{9})  == U{3});
        TEST("gcd(12,-8)==4",   nstd::gcd(I{12},  I{-8}) == U{4});
        TEST("gcd(-35,-21)==7", nstd::gcd(I{-35}, I{-21})== U{7});
        TEST("gcd(0,5)==5",     nstd::gcd(I{},    I{5})  == U{5});
        TEST("gcd(5,0)==5",     nstd::gcd(I{5},   I{})   == U{5});
    }

    // lcm (signed delegates to unsigned abs)
    {
        TEST("lcm(-4,6)==12",   nstd::lcm(I{-4}, I{6})  == U{12});
        TEST("lcm(0,5)==0",     nstd::lcm(I{},   I{5})  == U{});
        TEST("lcm(3,3)==3",     nstd::lcm(I{3},  I{3})  == U{3});
    }

    // checked_add (signed)
    {
        const I mx = I::max_val();
        const I mn = I::min_val();
        TEST("checked_add(3,-4)==-1",    nstd::checked_add(I{3},  I{-4}) == std::optional<I>{I{-1}});
        TEST("checked_add(max,min)==-1", nstd::checked_add(mx,    mn)    == std::optional<I>{I{-1}});
        TEST("checked_add(max,1)==null", !nstd::checked_add(mx,   I{1}).has_value());
        TEST("checked_add(min,-1)==null",!nstd::checked_add(mn,   I{-1}).has_value());
    }

    // checked_sub (signed)
    {
        const I mx = I::max_val();
        const I mn = I::min_val();
        TEST("checked_sub(5,3)==2",      nstd::checked_sub(I{5},  I{3})  == std::optional<I>{I{2}});
        TEST("checked_sub(3,5)==-2",     nstd::checked_sub(I{3},  I{5})  == std::optional<I>{I{-2}});
        TEST("checked_sub(min,1)==null", !nstd::checked_sub(mn,   I{1}).has_value());
        TEST("checked_sub(max,-1)==null",!nstd::checked_sub(mx,   I{-1}).has_value());
    }

    // checked_mul (signed)
    {
        const I mx = I::max_val();
        TEST("checked_mul(3,4)==12",    nstd::checked_mul(I{3},  I{4})  == std::optional<I>{I{12}});
        TEST("checked_mul(-1,-1)==1",   nstd::checked_mul(I{-1}, I{-1}) == std::optional<I>{I{1}});
        TEST("checked_mul(max,2)==null",!nstd::checked_mul(mx,   I{2}).has_value());
        TEST("checked_mul(0,max)==0",   nstd::checked_mul(I{},   mx)    == std::optional<I>{I{}});
    }
}

// =============================================================================
// Section 9b: Cross-N operators (int_fixed_t<N> op int_fixed_t<M>, N != M)
// =============================================================================

static void test_cross_n_int()
{
    std::cout << "\n--- Section 9b: Cross-N int operators ---\n";

    using i1 = int_fixed_t<1>;
    using i2 = int_fixed_t<2>;
    using i4 = int_fixed_t<4>;

    const i1 a1{std::int64_t{10}};
    const i2 a2{std::int64_t{-3}};

    auto sum_21 = a2 + a1;
    static_assert(std::is_same_v<decltype(sum_21), i2>, "cross-N int: wider wins");
    TEST("i2{-3}+i1{10}==i2{7}", sum_21 == i2{std::int64_t{7}});

    auto sum_12 = a1 + a2;
    static_assert(std::is_same_v<decltype(sum_12), i2>, "cross-N int reversed");
    TEST("i1{10}+i2{-3}==i2{7}", sum_12 == i2{std::int64_t{7}});

    auto diff = a2 - a1;
    TEST("i2{-3}-i1{10}==i2{-13}", diff == i2{std::int64_t{-13}});

    auto prod = a1 * a2;
    TEST("i1{10}*i2{-3}==i2{-30}", prod == i2{std::int64_t{-30}});

    const i1 neg7{std::int64_t{-7}};
    const i2 three2{std::int64_t{3}};
    auto quot = neg7 / three2;
    TEST("i1{-7}/i2{3}==i2{-2}",  quot == i2{std::int64_t{-2}});
    auto rem = neg7 % three2;
    TEST("i1{-7}%i2{3}==i2{-1}",  rem == i2{std::int64_t{-1}});

    TEST("i1{3}==i2{3}",  i1{std::int64_t{3}} == i2{std::int64_t{3}});
    TEST("i2{3}==i1{3}",  i2{std::int64_t{3}} == i1{std::int64_t{3}});
    TEST("i2{-3}<i1{10}", a2 < a1);
    TEST("i1{10}>i2{-3}", a1 > a2);

    const i4 x4{std::int64_t{-100}};
    const i2 x2{std::int64_t{7}};
    auto cross42 = x4 + x2;
    static_assert(std::is_same_v<decltype(cross42), i4>, "i4+i2 gives i4");
    TEST("i4{-100}+i2{7}==i4{-93}", cross42 == i4{std::int64_t{-93}});
}

template <std::size_t N>
static void run_all(const char *tag)
{
    test_construction<N>(tag);
    test_comparison<N>(tag);
    test_bitwise<N>(tag);
    test_addsub<N>(tag);
    test_multiply<N>(tag);
    test_divmod<N>(tag);
    test_strings<N>(tag);
    test_bit_utilities<N>(tag);
    test_mixed_ops_int<N>(tag);
    test_higher_arith_int<N>(tag);
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "int_fixed_t<N> Signed Tests (v1.90)\n";
    std::cout << "====================================================================\n";

    run_all<1>("int64_fixed");
    run_all<2>("int128_fixed");
    run_all<4>("int256_fixed");
    run_all<8>("int512_fixed");

    test_cross_n_int();

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
