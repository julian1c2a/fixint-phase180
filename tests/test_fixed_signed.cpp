// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int_fixed_t<N> — signed fixed-width integer template
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Covers N=2 (128-bit), N=4 (256-bit), N=8 (512-bit).
// Sections:
//   1.  Construction and zero/one/max_val/min_val
//   2.  Signed comparison
//   3.  Bitwise: ~, &, |, ^, <<, >> (arithmetic)
//   4.  Addition, subtraction, negation, increment/decrement
//   5.  Multiplication
//   6.  Division and modulo (truncation-toward-zero)
//   7.  String: to_string and from_string round-trips

#include "int_fixed.hpp"

#include <cstdlib>
#include <iostream>

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
    // max_val lower limbs all 0xFF
    TEST("max_val data[0]==~0", mx.bits.data[0] == ~std::uint64_t{0});
    // min_val: only MSB of data[N-1] set, rest 0
    TEST("min_val data[N-1] MSB==1", (mn.bits.data[N - 1] >> 63) == 1);
    TEST("min_val data[0]==0", mn.bits.data[0] == 0);

    // Default constructor gives zero
    const int_fixed_t<N> def{};
    TEST("default == zero", def == z);

    // From int64_t: positive
    const int_fixed_t<N> v42{std::int64_t{42}};
    TEST("from_i64(42) data[0]==42", v42.bits.data[0] == 42);
    TEST("from_i64(42) data[N-1]==0", v42.bits.data[N - 1] == 0);
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
    TEST("min < -1",   mn < m1);
    TEST("min < zero", mn < z);
    TEST("min < one",  mn < o);
    TEST("min < max",  mn < mx);
    TEST("-1  < zero", m1 < z);
    TEST("-1  < one",  m1 < o);
    TEST("-1  < max",  m1 < mx);
    TEST("zero < one", z < o);
    TEST("zero < max", z < mx);
    TEST("one  < max", o < mx);

    TEST("max > zero", mx > z);
    TEST("zero > min", z > mn);
    TEST("zero > -1",  z > m1);

    TEST("zero >= zero", z >= z);
    TEST("one  >= zero", o >= z);
    TEST("max  >= max",  mx >= mx);
    TEST("min  <= min",  mn <= mn);
    TEST("min  <= zero", mn <= z);

    // Cross-sign: positive always > negative
    const int_fixed_t<N> two{std::int64_t{2}};
    const int_fixed_t<N> m2{std::int64_t{-2}};
    TEST("2 > -2",   two > m2);
    TEST("-2 < 2",   m2 < two);
    TEST("-1 < 1",   m1 < o);

    // Same sign, magnitude dominates
    TEST("-1 > -2",  m1 > m2);
    TEST("-2 < -1",  m2 < m1);
}

// =============================================================================
// Section 3: Bitwise and arithmetic right shift
// =============================================================================

template <std::size_t N>
static void test_bitwise(const char *tag)
{
    std::cout << "\n--- Section 3: Bitwise [N=" << N << " " << tag << "] ---\n";

    const int_fixed_t<N> z = int_fixed_t<N>::zero();
    const int_fixed_t<N> m1{std::int64_t{-1}};   // all bits set
    const int_fixed_t<N> one = int_fixed_t<N>::one();

    // ~0 == -1 (all bits set), ~(-1) == 0
    TEST("~zero == -1",  ~z == m1);
    TEST("~(-1) == zero", ~m1 == z);

    // AND, OR, XOR
    TEST("-1 & zero == zero", (m1 & z) == z);
    TEST("-1 & -1   == -1",   (m1 & m1) == m1);
    TEST("-1 | zero == -1",   (m1 | z) == m1);
    TEST("zero | zero == zero",(z | z) == z);
    TEST("-1 ^ -1   == zero", (m1 ^ m1) == z);
    TEST("-1 ^ zero == -1",   (m1 ^ z) == m1);

    // Left shift (logical): -1 << 1 loses MSB, fills 0 at low end
    const int_fixed_t<N> m1_shl1 = m1 << 1;
    TEST("-1 << 1 == -2", m1_shl1 == int_fixed_t<N>{std::int64_t{-2}});

    // Arithmetic right shift: positive fills 0
    TEST("4 >> 1 == 2",
         int_fixed_t<N>{std::int64_t{4}} >> 1 == int_fixed_t<N>{std::int64_t{2}});
    TEST("1 >> 1 == 0",
         int_fixed_t<N>{std::int64_t{1}} >> 1 == z);

    // Arithmetic right shift: negative fills 1
    TEST("-1 >> 1 == -1",  (m1 >> 1) == m1);
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
    TEST("1 << 0 == 1",   (one << 0) == one);

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
    TEST("zero + one  == one",  (z + o) == o);
    TEST("one  + zero == one",  (o + z) == o);

    // Known values
    TEST("1 + (-1) == 0",  (o + m1) == z);
    TEST("(-1)+(-1) == -2",(m1 + m1) == m2);
    TEST("2 + (-1) == 1",  (two + m1) == o);

    // Subtraction
    TEST("1 - 1 == 0",   (o - o) == z);
    TEST("0 - 1 == -1",  (z - o) == m1);
    TEST("(-1) - 1 == -2",(m1 - o) == m2);

    // Negation
    TEST("-zero == zero", (-z) == z);
    TEST("-one  == -1",   (-o) == m1);
    TEST("-(-1) == 1",    (-m1) == o);
    TEST("-(-one) == one",(-(-o)) == o);

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

    TEST("a * one  == a",   (three * o) == three);
    TEST("one  * a == a",   (o * three) == three);
    TEST("a * zero == zero",(three * z) == z);
    TEST("zero * a == zero",(z * three) == z);

    // Sign rules
    TEST("3 * 7   == 21",   (three * seven) == twentyone);
    TEST("3 * (-7) == -21", (three * m7) == m21);
    TEST("(-3)*7   == -21", (m3 * seven) == m21);
    TEST("(-3)*(-7)==21",   (m3 * m7) == twentyone);

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
    TEST("21/7 == 3",    twentyone / seven == three);
    TEST("21%7 == 0",    twentyone % seven == z);
    TEST("21/3 == 7",    twentyone / three == seven);
    TEST("100/10 == 10", hundred / ten == ten);
    TEST("100%10 == 0",  hundred % ten == z);
    TEST("7/7 == 1",     seven / seven == o);
    TEST("7%7 == 0",     seven % seven == z);
    TEST("3/7 == 0",     three / seven == z);
    TEST("3%7 == 3",     three % seven == three);

    // Sign rules (truncation toward zero, C++ semantics)
    // 21 / (-7) == -3
    TEST("21/(-7) == -3",    twentyone / m7 == m3);
    TEST("21%(-7) == 0",     twentyone % m7 == z);
    // (-21) / 7 == -3
    TEST("(-21)/7 == -3",    m21 / seven == m3);
    TEST("(-21)%7 == 0",     m21 % seven == z);
    // (-21) / (-7) == 3
    TEST("(-21)/(-7) == 3",  m21 / m7 == three);
    TEST("(-21)%(-7) == 0",  m21 % m7 == z);

    // 100 / (-10) == -10
    TEST("100/(-10) == -10", hundred / m10 == m10);
    TEST("100%(-10) == 0",   hundred % m10 == z);
    // (-100) / 10 == -10
    TEST("(-100)/10 == -10", m100 / ten == m10);
    TEST("(-100)%10 == 0",   m100 % ten == z);

    // Fundamental theorem with signs: a == (a/b)*b + (a%b)
    auto theorem = [](std::int64_t av, std::int64_t bv) -> bool {
        const int_fixed_t<N> a{av};
        const int_fixed_t<N> b{bv};
        const auto [q, r] = int_fixed_t<N>::divmod(a, b);
        // C++ semantics: remainder has same sign as dividend
        const bool rem_sign_ok = r.is_zero() || (r.is_negative() == a.is_negative());
        return (q * b + r) == a && rem_sign_ok;
    };

    TEST("theorem: 7/3",      theorem(7, 3));
    TEST("theorem: -7/3",     theorem(-7, 3));
    TEST("theorem: 7/(-3)",   theorem(7, -3));
    TEST("theorem: -7/(-3)",  theorem(-7, -3));
    TEST("theorem: 100/7",    theorem(100, 7));
    TEST("theorem: -100/7",   theorem(-100, 7));
    TEST("theorem: 100/(-7)", theorem(100, -7));
    TEST("theorem: -100/(-7)",theorem(-100, -7));
    TEST("theorem: 1/1",      theorem(1, 1));
    TEST("theorem: -1/1",     theorem(-1, 1));
    TEST("theorem: -1/(-1)",  theorem(-1, -1));

    // Division by zero throws
    auto throws_domain = [](auto &&fn) -> bool {
        try { fn(); return false; }
        catch (const std::domain_error &) { return true; }
        catch (...) { return false; }
    };
    TEST("1/0 throws",  throws_domain([&]{ (void)(o / z); }));
    TEST("-1/0 throws", throws_domain([&]{ (void)(m1 / z); }));
    TEST("divmod(1,0) throws", throws_domain([&]{ (void)int_fixed_t<N>::divmod(o, z); }));
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
    auto check_invalid = [](const char *s) -> bool {
        try { (void)int_fixed_t<N>::from_string(s); return false; }
        catch (const std::invalid_argument &) { return true; }
        catch (...) { return false; }
    };
    TEST("invalid: \"abc\"",  check_invalid("abc"));
    TEST("invalid: \"\"",     check_invalid(""));
    TEST("invalid: \"-\"",    check_invalid("-"));
    TEST("invalid: \"-abc\"", check_invalid("-abc"));
}

// =============================================================================
// Run all sections for one N
// =============================================================================

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
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "int_fixed_t<N> Signed Tests (v1.90)\n";
    std::cout << "====================================================================\n";

    run_all<2>("int128_fixed");
    run_all<4>("int256_fixed");
    run_all<8>("int512_fixed");

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
