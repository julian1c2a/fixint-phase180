// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: uint_fixed_t<N> — unsigned fixed-width integer template
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Covers N=1 (64-bit), N=2 (128-bit), N=4 (256-bit), N=8 (512-bit).
// Sections:
//   1.  Construction and zero/one/max
//   2.  Comparison operators
//   3.  Bitwise: ~, &, |, ^
//   4.  Shifts: <<, >>
//   5.  Addition (mod 2^(64N)): identity, commutativity, wrap-around
//   6.  Subtraction (mod 2^(64N)): identity, inverse, borrow propagation
//   7.  Negation: -zero==zero, -(-x)==x (mod 2^(64N))
//   8.  Increment/decrement
//   9.  Multiplication: identity, commutativity, zero, known values
//   10. Utility: is_zero, bit_width, popcount
//   11. String: to_string and from_string round-trips
//   12. Mixed-type operators (uint_fixed_t op integral T)
//   12b. Cross-N operators (uint_fixed_t<N> op uint_fixed_t<M>, N != M)
//   13. Higher arithmetic: mul_wide, pow, sqrt, gcd, lcm, checked_add/sub/mul

#include "fixed_width_int_t.hpp"

#include <cstdlib>
#include <iostream>

using nstd::uint128_fixed_t;
using nstd::uint256_fixed_t;
using nstd::uint512_fixed_t;
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
// Helper: build a value with all limbs set to the same pattern
// =============================================================================

template <std::size_t N>
static uint_fixed_t<N> all_limbs(std::uint64_t v)
{
    std::array<std::uint64_t, N> a{};
    a.fill(v);
    return uint_fixed_t<N>{a};
}

// =============================================================================
// Section 1: Construction, zero / one / max
// =============================================================================

template <std::size_t N>
static void test_construction(const char *tag)
{
    std::cout << "\n--- Section 1: Construction [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    TEST("zero is_zero", z.is_zero());
    TEST("one  not zero", !o.is_zero());
    TEST("max  not zero", !m.is_zero());
    TEST("zero data[0]==0", z.limb(0) == 0);
    TEST("one  data[0]==1", o.limb(0) == 1);
    TEST("max  data[0]==~0", m.limb(0) == ~std::uint64_t{0});
    TEST("max  data[N-1]==~0", m.limb(N - 1) == ~std::uint64_t{0});
    if constexpr (N > 1)
        TEST("one  data[N-1]==0", o.limb(N - 1) == 0);

    // Default constructor gives zero
    const uint_fixed_t<N> def{};
    TEST("default == zero", def == z);

    // uint64_t constructor
    const uint_fixed_t<N> v42{std::uint64_t{42}};
    TEST("from_u64 data[0]==42", v42.limb(0) == 42);
    if constexpr (N > 1)
        TEST("from_u64 data[N-1]==0", v42.limb(N - 1) == 0);
}

// =============================================================================
// Section 2: Comparison
// =============================================================================

template <std::size_t N>
static void test_comparison(const char *tag)
{
    std::cout << "\n--- Section 2: Comparison [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    TEST("zero == zero", z == z);
    TEST("one  == one", o == o);
    TEST("max  == max", m == m);
    TEST("zero != one", z != o);
    TEST("zero != max", z != m);
    TEST("one  != max", o != m);

    TEST("zero < one", z < o);
    TEST("one  < max", o < m);
    TEST("zero < max", z < m);

    TEST("one  > zero", o > z);
    TEST("max  > zero", m > z);
    TEST("max  > one", m > o);

    TEST("zero <= zero", z <= z);
    TEST("zero <= one", z <= o);
    TEST("max  >= max", m >= m);
    TEST("max  >= one", m >= o);
    TEST("!(one < zero)", !(o < z));
    TEST("!(zero > one)", !(z > o));

    // Multi-limb ordering: compare only on high limb difference (N>1 only)
    if constexpr (N > 1)
    {
        uint_fixed_t<N> big{};
        big.set_limb(N - 1, 1); // 2^(64*(N-1)), much bigger than one
        TEST("big > one (MSL dominates)", big > o);
        TEST("one < big (MSL dominates)", o < big);
    }
}

// =============================================================================
// Section 3: Bitwise
// =============================================================================

template <std::size_t N>
static void test_bitwise(const char *tag)
{
    std::cout << "\n--- Section 3: Bitwise [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto m = uint_fixed_t<N>::max();
    const auto o = uint_fixed_t<N>::one();

    // NOT
    TEST("~zero == max", ~z == m);
    TEST("~max  == zero", ~m == z);

    // AND
    TEST("max & zero == zero", (m & z) == z);
    TEST("max & max  == max", (m & m) == m);
    TEST("max & one  == one", (m & o) == o);

    // OR
    TEST("zero | zero == zero", (z | z) == z);
    TEST("zero | max  == max", (z | m) == m);
    TEST("one  | max  == max", (o | m) == m);

    // XOR
    TEST("max ^ max  == zero", (m ^ m) == z);
    TEST("zero ^ max == max", (z ^ m) == m);
    TEST("max ^ zero == max", (m ^ z) == m);

    // De Morgan: ~(a & b) == ~a | ~b
    const auto a = all_limbs<N>(0xAAAAAAAAAAAAAAAAULL);
    const auto b = all_limbs<N>(0x5555555555555555ULL);
    TEST("De Morgan AND: ~(a&b)==~a|~b", (~(a & b)) == (~a | ~b));
    TEST("De Morgan OR:  ~(a|b)==~a&~b", (~(a | b)) == (~a & ~b));
    TEST("a ^ b == (a|b) & ~(a&b)", (a ^ b) == ((a | b) & ~(a & b)));

    // a | b == (a ^ b) | (a & b)
    TEST("or_via_xor_and", (a | b) == ((a ^ b) | (a & b)));
}

// =============================================================================
// Section 4: Shifts
// =============================================================================

template <std::size_t N>
static void test_shifts(const char *tag)
{
    std::cout << "\n--- Section 4: Shifts [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    // Shift by 0
    TEST("one << 0 == one", (o << 0) == o);
    TEST("one >> 0 == one", (o >> 0) == o);
    TEST("max << 0 == max", (m << 0) == m);

    // Shift by total width = 0
    TEST("one << 64N == 0", (o << (64U * N)) == z);
    TEST("one >> 64N == 0", (o >> (64U * N)) == z);

    // Shift by 1
    const auto two = uint_fixed_t<N>{std::uint64_t{2}};
    TEST("one << 1 == two", (o << 1) == two);
    TEST("two >> 1 == one", (two >> 1) == o);

    // Shift by 63 within limb
    const uint_fixed_t<N> msb_of_limb0{std::uint64_t{std::uint64_t{1} << 63}};
    TEST("(1<<63) >> 63 == one", (msb_of_limb0 >> 63) == o);

    // Shift across limb boundary: one << 64 → data[1]=1, data[0]=0
    if constexpr (N >= 2)
    {
        const auto shifted = o << 64;
        TEST("(1<<64) data[0]==0", shifted.limb(0) == 0);
        TEST("(1<<64) data[1]==1", shifted.limb(1) == 1);
        const auto back = shifted >> 64;
        TEST("(1<<64)>>64 == one", back == o);
    }

    // Shift max right fills with zeros from MSB
    TEST("max >> (64N-1) == one", (m >> (64U * N - 1)) == o);
    TEST("max << (64N-1): only MSB set", (m << (64U * N - 1)).limb(N - 1) == (std::uint64_t{1} << 63));
}

// =============================================================================
// Section 5: Addition
// =============================================================================

template <std::size_t N>
static void test_addition(const char *tag)
{
    std::cout << "\n--- Section 5: Addition [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    // Identity
    TEST("zero + zero == zero", (z + z) == z);
    TEST("zero + one  == one", (z + o) == o);
    TEST("one  + zero == one", (o + z) == o);

    // Commutativity
    const auto a = all_limbs<N>(0x123456789ABCDEFULL);
    const auto b = all_limbs<N>(0xFEDCBA9876543210ULL);
    TEST("a+b == b+a", (a + b) == (b + a));

    // Wrap-around: max + 1 == 0 (mod 2^(64N))
    TEST("max + one == zero (wrap)", (m + o) == z);
    TEST("max + max == max-1 (wrap)", (m + m) == (m - o));

    // Carry propagation across all limbs: max+1=0, verified via is_zero()
    // Use volatile reads to prevent Clang 22's incorrect constexpr-folding of
    // is_zero() on multi-limb results when evaluated after a prior N=2 call.
    {
        const auto sum = m + o;
        volatile bool all_zero{true};
        for (std::size_t i{0}; i < N; ++i)
        {
            volatile std::uint64_t limb = sum.limb(i);
            if (limb != 0)
                all_zero = false;
        }
        TEST("all_0xFF + 1 == 0 (full carry)", static_cast<bool>(all_zero));
    }

    // a - b + b == a (round-trip)
    TEST("(a-b)+b == a", ((a - b) + b) == a);

    // Binary adder identity: a+b == (a^b) + ((a&b)<<1)
    TEST("add_via_xor_carry", (a + b) == ((a ^ b) + ((a & b) << 1)));
}

// =============================================================================
// Section 6: Subtraction
// =============================================================================

template <std::size_t N>
static void test_subtraction(const char *tag)
{
    std::cout << "\n--- Section 6: Subtraction [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    TEST("zero - zero == zero", (z - z) == z);
    TEST("one  - one  == zero", (o - o) == z);
    TEST("one  - zero == one", (o - z) == o);

    // Borrow propagation: zero - one == max (mod 2^(64N))
    TEST("zero - one == max (borrow wrap)", (z - o) == m);

    // sub via complement: a - b == a + (~b + 1)
    const auto a = all_limbs<N>(0xCAFEBABEDEADBEEFULL);
    const auto b = all_limbs<N>(0x0001000200030004ULL);
    TEST("sub_via_complement", (a - b) == (a + (~b + o)));

    // Self-subtraction
    TEST("a - a == zero", (a - a) == z);

    // Borrow propagates through consecutive 0-limbs
    // Build 2^64 (N>=2): data[1]=1, data[0]=0
    if constexpr (N >= 2)
    {
        uint_fixed_t<N> pow64{};
        pow64.set_limb(1, 1);
        // 2^64 - 1 == max of lower 64 bits = 0xFFFF...FFFF (low), 0 (rest)
        const auto res = pow64 - o;
        TEST("2^64 - 1: data[0]==~0", res.limb(0) == ~std::uint64_t{0});
        TEST("2^64 - 1: data[1]==0", res.limb(1) == 0);
    }
}

// =============================================================================
// Section 7: Negation
// =============================================================================

template <std::size_t N>
static void test_negation(const char *tag)
{
    std::cout << "\n--- Section 7: Negation [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    TEST("-zero == zero", (-z) == z);
    TEST("-one  == max", (-o) == m);
    TEST("-max  == one", (-m) == o);

    const auto a = all_limbs<N>(0xDEADBEEFCAFEBABEULL);
    TEST("a + (-a) == zero", (a + (-a)) == z);
    TEST("-(-a) == a", (-(-a)) == a);
}

// =============================================================================
// Section 8: Increment / Decrement
// =============================================================================

template <std::size_t N>
static void test_inc_dec(const char *tag)
{
    std::cout << "\n--- Section 8: Inc/Dec [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

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

    // Wrap-around
    auto mmax = m;
    TEST("max++ == zero (wrap)", (++mmax) == z);

    auto zzero = z;
    TEST("zero-- == max (wrap)", (--zzero) == m);
}

// =============================================================================
// Section 9: Multiplication
// =============================================================================

template <std::size_t N>
static void test_multiplication(const char *tag)
{
    std::cout << "\n--- Section 9: Multiplication [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();

    // Identity
    const auto a = all_limbs<N>(0x0102030405060708ULL);
    TEST("a * one == a", (a * o) == a);
    TEST("one * a == a", (o * a) == a);
    TEST("a * zero == zero", (a * z) == z);
    TEST("zero * a == zero", (z * a) == z);

    // Commutativity
    const auto b = all_limbs<N>(0x0807060504030201ULL);
    TEST("a * b == b * a", (a * b) == (b * a));

    // Known value: 3 * 7 == 21
    const uint_fixed_t<N> three{std::uint64_t{3}};
    const uint_fixed_t<N> seven{std::uint64_t{7}};
    const uint_fixed_t<N> twentyone{std::uint64_t{21}};
    TEST("3 * 7 == 21", (three * seven) == twentyone);

    // Distributivity: a*(b+c) == a*b + a*c (mod 2^(64N))
    const auto c = all_limbs<N>(0x1111111111111111ULL);
    TEST("distributivity: a*(b+c)==a*b+a*c", (a * (b + c)) == (a * b + a * c));

    // Associativity: (a*b)*c == a*(b*c) (mod 2^(64N))
    TEST("associativity: (a*b)*c==a*(b*c)", ((a * b) * c) == (a * (b * c)));

    // Known value for N>=2: 2^63 * 2 == 2^64 (cross-limb)
    if constexpr (N >= 2)
    {
        const uint_fixed_t<N> pow63{std::uint64_t{1} << 63};
        const uint_fixed_t<N> two{std::uint64_t{2}};
        uint_fixed_t<N> pow64{};
        pow64.set_limb(1, 1); // 2^64
        TEST("2^63 * 2 == 2^64 (cross-limb)", (pow63 * two) == pow64);
    }

    // Square: (2^32)^2 == 2^64 (cross-limb, N>=2)
    if constexpr (N >= 2)
    {
        const uint_fixed_t<N> pow32{std::uint64_t{1} << 32};
        uint_fixed_t<N> pow64{};
        pow64.set_limb(1, 1);
        TEST("(2^32)^2 == 2^64", (pow32 * pow32) == pow64);
    }
}

// =============================================================================
// Section 10: Utility — is_zero, bit_width, popcount, clz, ctz, is_power_of_two
// =============================================================================

template <std::size_t N>
static void test_utility(const char *tag)
{
    std::cout << "\n--- Section 10: Utility [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();
    const uint_fixed_t<N> two{std::uint64_t{2}};
    const uint_fixed_t<N> three{std::uint64_t{3}};
    const uint_fixed_t<N> four{std::uint64_t{4}};

    TEST("zero.is_zero()", z.is_zero());
    TEST("one .is_zero()==false", !o.is_zero());
    TEST("max .is_zero()==false", !m.is_zero());

    TEST("zero.bit_width()==0", z.bit_width() == 0);
    TEST("one .bit_width()==1", o.bit_width() == 1);
    TEST("max .bit_width()==64N", m.bit_width() == 64U * N);
    TEST("2.bit_width()==2", two.bit_width() == 2);

    TEST("zero.popcount()==0", z.popcount() == 0);
    TEST("one .popcount()==1", o.popcount() == 1);
    TEST("max .popcount()==64N", m.popcount() == 64U * N);

    // count_leading_zeros
    TEST("clz(zero)==64N", z.count_leading_zeros() == 64U * N);
    TEST("clz(one)==64N-1", o.count_leading_zeros() == 64U * N - 1U);
    TEST("clz(max)==0", m.count_leading_zeros() == 0U);
    TEST("clz(2)==64N-2", two.count_leading_zeros() == 64U * N - 2U);

    // count_trailing_zeros
    TEST("ctz(zero)==64N", z.count_trailing_zeros() == 64U * N);
    TEST("ctz(one)==0", o.count_trailing_zeros() == 0U);
    TEST("ctz(max)==0", m.count_trailing_zeros() == 0U);
    TEST("ctz(2)==1", two.count_trailing_zeros() == 1U);
    TEST("ctz(4)==2", four.count_trailing_zeros() == 2U);

    // is_power_of_two
    TEST("is_pow2(zero)==false", !z.is_power_of_two());
    TEST("is_pow2(one)==true", o.is_power_of_two());
    TEST("is_pow2(two)==true", two.is_power_of_two());
    TEST("is_pow2(four)==true", four.is_power_of_two());
    TEST("is_pow2(three)==false", !three.is_power_of_two());
    TEST("is_pow2(max)==false", !m.is_power_of_two());

    // cross-limb cases (N>=2): 2^64
    if constexpr (N >= 2)
    {
        uint_fixed_t<N> pow64{};
        pow64.set_limb(1, 1);
        TEST("clz(2^64)==64N-65", pow64.count_leading_zeros() == 64U * N - 65U);
        TEST("ctz(2^64)==64", pow64.count_trailing_zeros() == 64U);
        TEST("is_pow2(2^64)", pow64.is_power_of_two());
    }
}

// =============================================================================
// Section 11: String conversion
// =============================================================================

template <std::size_t N>
static void test_strings(const char *tag)
{
    std::cout << "\n--- Section 11: String [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();

    TEST("zero.to_string()==\"0\"", z.to_string() == "0");
    TEST("one .to_string()==\"1\"", o.to_string() == "1");

    // Known small values
    const uint_fixed_t<N> v{std::uint64_t{12345678901234567ULL}};
    TEST("12345678901234567 rt", uint_fixed_t<N>::from_string("12345678901234567") == v);

    // Round-trip: from_string(to_string(x)) == x
    const auto a = all_limbs<N>(0x0102030405060708ULL);
    {
        const std::string s = a.to_string();
        const auto back = uint_fixed_t<N>::from_string(s.c_str());
        TEST("all-limbs round-trip", back == a);
    }

    // max round-trip
    {
        const auto m = uint_fixed_t<N>::max();
        const std::string s = m.to_string();
        const auto back = uint_fixed_t<N>::from_string(s.c_str());
        TEST("max round-trip", back == m);
    }

    // parse zero
    TEST("from_string(\"0\")==zero", uint_fixed_t<N>::from_string("0") == z);

    // invalid input throws
    bool threw{false};
    try
    {
        (void)uint_fixed_t<N>::from_string("abc");
    }
    catch (const std::invalid_argument &)
    {
        threw = true;
    }
    TEST("from_string invalid throws", threw);

    bool threw2{false};
    try
    {
        (void)uint_fixed_t<N>::from_string("");
    }
    catch (const std::invalid_argument &)
    {
        threw2 = true;
    }
    TEST("from_string empty throws", threw2);
}

// =============================================================================
// Run all sections for one N
// =============================================================================

// =============================================================================
// Section 12: Mixed-type operators (uint_fixed_t op integral T)
// =============================================================================

template <std::size_t N>
static void test_mixed_ops(const char *tag)
{
    std::cout << "\n--- Section 12: Mixed-type ops [N=" << N << " " << tag << "] ---\n";

    const uint_fixed_t<N> five{std::uint64_t{5}};
    const uint_fixed_t<N> ten{std::uint64_t{10}};

    // arithmetic free functions
    TEST("five+3==8", (five + 3) == uint_fixed_t<N>{std::uint64_t{8}});
    TEST("3+five==8", (3 + five) == uint_fixed_t<N>{std::uint64_t{8}});
    TEST("ten-3==7", (ten - 3) == uint_fixed_t<N>{std::uint64_t{7}});
    TEST("five*3==15", (five * 3) == uint_fixed_t<N>{std::uint64_t{15}});
    TEST("3*five==15", (3 * five) == uint_fixed_t<N>{std::uint64_t{15}});
    TEST("ten/3==3", (ten / 3) == uint_fixed_t<N>{std::uint64_t{3}});
    TEST("10u/five==2", (std::uint64_t{10} / five) == uint_fixed_t<N>{std::uint64_t{2}});
    TEST("ten%3==1", (ten % 3) == uint_fixed_t<N>{std::uint64_t{1}});
    TEST("3%five==3", (3 % five) == uint_fixed_t<N>{std::uint64_t{3}});

    // comparison free functions
    TEST("five==5", (five == 5));
    TEST("5==five", (5 == five));
    TEST("five!=6", (five != 6));
    TEST("6!=five", (6 != five));
    TEST("five<6", (five < 6));
    TEST("4<five", (4 < five));
    TEST("five<=5", (five <= 5));
    TEST("5<=five", (5 <= five));
    TEST("five>4", (five > 4));
    TEST("6>five", (6 > five));
    TEST("five>=5", (five >= 5));
    TEST("5>=five", (5 >= five));

    // bitwise free functions
    const uint_fixed_t<N> oxf{std::uint64_t{0xF}};
    TEST("oxf&3==3", (oxf & 3) == uint_fixed_t<N>{std::uint64_t{3}});
    TEST("3&oxf==3", (3 & oxf) == uint_fixed_t<N>{std::uint64_t{3}});
    TEST("oxf|0x10==0x1F", (oxf | 0x10) == uint_fixed_t<N>{std::uint64_t{0x1F}});
    TEST("0x10|oxf==0x1F", (0x10 | oxf) == uint_fixed_t<N>{std::uint64_t{0x1F}});
    TEST("oxf^3==0xC", (oxf ^ 3) == uint_fixed_t<N>{std::uint64_t{0xC}});
    TEST("3^oxf==0xC", (3 ^ oxf) == uint_fixed_t<N>{std::uint64_t{0xC}});

    // compound assignments
    uint_fixed_t<N> x{std::uint64_t{7}};
    x += 3;
    TEST("x+=3→10", x == uint_fixed_t<N>{std::uint64_t{10}});
    x -= 2;
    TEST("x-=2→8", x == uint_fixed_t<N>{std::uint64_t{8}});
    x *= 3;
    TEST("x*=3→24", x == uint_fixed_t<N>{std::uint64_t{24}});
    x /= 4;
    TEST("x/=4→6", x == uint_fixed_t<N>{std::uint64_t{6}});
    x %= 4;
    TEST("x%=4→2", x == uint_fixed_t<N>{std::uint64_t{2}});
    x |= 0xC;
    TEST("x|=0xC→E", x == uint_fixed_t<N>{std::uint64_t{0xE}});
    x &= 0xA;
    TEST("x&=0xA→A", x == uint_fixed_t<N>{std::uint64_t{0xA}});
    x ^= 3;
    TEST("x^=3→9", x == uint_fixed_t<N>{std::uint64_t{9}});

#ifdef __SIZEOF_INT128__
    if constexpr (N >= 2)
    {
        const unsigned __int128 big = ((unsigned __int128)1 << 65) + 7U;
        const uint_fixed_t<N> big_f{big};
        TEST("big_f+big==2*big_f", (big_f + big) == (big_f + big_f));
        TEST("big==big_f", (big == big_f));
        TEST("big_f==big", (big_f == big));
        TEST("big_f<big+1", (big_f < (big + 1)));
    }
#endif
}

// =============================================================================
// Section 13: Higher arithmetic — mul_wide, pow, sqrt, gcd, lcm, checked_*
// =============================================================================

template <std::size_t N>
static void test_higher_arith(const char *tag)
{
    std::cout << "\n--- Section 13: Higher arithmetic [N=" << N << " " << tag << "] ---\n";

    using U = uint_fixed_t<N>;
    using U2 = uint_fixed_t<2 * N>;

    // mul_wide: result type is 2N wide, value is correct
    {
        const U a = U{12};
        const U b = U{7};
        const U2 w = nstd::mul_wide(a, b);
        TEST("mul_wide(12,7)==84", w == U2{84});

        const U mx = U::max();
        const U2 sq = nstd::mul_wide(mx, mx);
        // (2^(64N)-1)^2 = 2^(128N) - 2^(64N+1) + 1 => low limb is always 1
        TEST("mul_wide(max,max): low limb==1", sq.limb(0) == std::uint64_t{1});
        TEST("mul_wide(1,1)==1", nstd::mul_wide(U{1}, U{1}) == U2{1});
        TEST("mul_wide(0,max)==0", nstd::mul_wide(U{}, mx) == U2{});
    }

    // pow
    {
        const U two{2};
        const U three{3};
        const U exp10{10};
        const U exp5{5};
        TEST("pow(2,10)==1024", nstd::pow(two, exp10) == U{1024});
        TEST("pow(3,5)==243", nstd::pow(three, exp5) == U{243});
        TEST("pow(base,0)==1", nstd::pow(U{99}, U{}) == U{1});
        TEST("pow(base,1)==base", nstd::pow(U{42}, U{1}) == U{42});
        TEST("pow(0,5)==0", nstd::pow(U{}, exp5) == U{});
    }

    // sqrt
    {
        TEST("sqrt(0)==0", nstd::sqrt(U{}) == U{});
        TEST("sqrt(1)==1", nstd::sqrt(U{1}) == U{1});
        TEST("sqrt(4)==2", nstd::sqrt(U{4}) == U{2});
        TEST("sqrt(9)==3", nstd::sqrt(U{9}) == U{3});
        TEST("sqrt(99)==9", nstd::sqrt(U{99}) == U{9});
        TEST("sqrt(100)==10", nstd::sqrt(U{100}) == U{10});
        TEST("sqrt(12345)==111", nstd::sqrt(U{12345}) == U{111});
    }

    // gcd (unsigned)
    {
        TEST("gcd(12,8)==4", nstd::gcd(U{12}, U{8}) == U{4});
        TEST("gcd(0,5)==5", nstd::gcd(U{}, U{5}) == U{5});
        TEST("gcd(5,0)==5", nstd::gcd(U{5}, U{}) == U{5});
        TEST("gcd(35,21)==7", nstd::gcd(U{35}, U{21}) == U{7});
        TEST("gcd(7,13)==1", nstd::gcd(U{7}, U{13}) == U{1});
    }

    // lcm (unsigned)
    {
        TEST("lcm(4,6)==12", nstd::lcm(U{4}, U{6}) == U{12});
        TEST("lcm(0,5)==0", nstd::lcm(U{}, U{5}) == U{});
        TEST("lcm(3,3)==3", nstd::lcm(U{3}, U{3}) == U{3});
    }

    // checked_add unsigned
    {
        const U mx = U::max();
        U one{std::uint64_t{1}};
        TEST("checked_add(1,2)==3", nstd::checked_add(U{1}, U{2}) == std::optional<U>{U{3}});
        TEST("checked_add(max,1)==nullopt", !nstd::checked_add(mx, one).has_value());
        TEST("checked_add(max,max)==nullopt", !nstd::checked_add(mx, mx).has_value());
        TEST("checked_add(0,0)==0", nstd::checked_add(U{}, U{}) == std::optional<U>{U{}});
    }

    // checked_sub unsigned
    {
        const U mx = U::max();
        U three{std::uint64_t{3}};
        U five{std::uint64_t{5}};
        TEST("checked_sub(5,3)==2", nstd::checked_sub(U{5}, three) == std::optional<U>{U{2}});
        TEST("checked_sub(3,5)==nullopt", !nstd::checked_sub(U{3}, five).has_value());
        TEST("checked_sub(max,max)==0", nstd::checked_sub(mx, mx) == std::optional<U>{U{}});
        TEST("checked_sub(0,0)==0", nstd::checked_sub(U{}, U{}) == std::optional<U>{U{}});
    }

    // checked_mul unsigned
    {
        const U mx = U::max();
        U two{std::uint64_t{2}};
        TEST("checked_mul(3,4)==12", nstd::checked_mul(U{3}, U{4}) == std::optional<U>{U{12}});
        TEST("checked_mul(max,2)==nullopt", !nstd::checked_mul(mx, two).has_value());
        TEST("checked_mul(0,max)==0", nstd::checked_mul(U{}, mx) == std::optional<U>{U{}});
        TEST("checked_mul(1,max)==max", nstd::checked_mul(U{1}, mx) == std::optional<U>{mx});
    }
}

// =============================================================================
// Section 12b: Cross-N operators (uint_fixed_t<N> op uint_fixed_t<M>, N != M)
// =============================================================================

static void test_cross_n_uint()
{
    std::cout << "\n--- Section 12b: Cross-N uint operators ---\n";

    using u1 = uint_fixed_t<1>;
    using u2 = uint_fixed_t<2>;
    using u4 = uint_fixed_t<4>;

    const u1 a1{std::uint64_t{10}};
    const u2 a2{std::uint64_t{3}};

    auto sum_21 = a2 + a1;
    static_assert(std::is_same_v<decltype(sum_21), u2>, "cross-N: wider wins");
    TEST("u2{3}+u1{10}==u2{13}", sum_21 == u2{std::uint64_t{13}});

    auto sum_12 = a1 + a2;
    static_assert(std::is_same_v<decltype(sum_12), u2>, "cross-N reversed");
    TEST("u1{10}+u2{3}==u2{13}", sum_12 == u2{std::uint64_t{13}});

    auto diff = a1 - a2;
    TEST("u1{10}-u2{3}==u2{7}", diff == u2{std::uint64_t{7}});

    auto prod = a1 * a2;
    TEST("u1{10}*u2{3}==u2{30}", prod == u2{std::uint64_t{30}});

    auto quot = a1 / a2;
    TEST("u1{10}/u2{3}==u2{3}", quot == u2{std::uint64_t{3}});

    auto rem = a1 % a2;
    TEST("u1{10}%u2{3}==u2{1}", rem == u2{std::uint64_t{1}});

    TEST("u1{3}==u2{3}", u1{std::uint64_t{3}} == u2{std::uint64_t{3}});
    TEST("u2{3}==u1{3}", u2{std::uint64_t{3}} == u1{std::uint64_t{3}});
    TEST("u1{10}>u2{3}", a1 > a2);
    TEST("u2{3}<u1{10}", a2 < a1);
    TEST("u2{10}!=u1{3}", u2{std::uint64_t{10}} != u1{std::uint64_t{3}});

    const u4 x4{std::uint64_t{100}};
    const u2 x2{std::uint64_t{7}};
    auto cross42 = x4 + x2;
    static_assert(std::is_same_v<decltype(cross42), u4>, "u4+u2 gives u4");
    TEST("u4{100}+u2{7}==u4{107}", cross42 == u4{std::uint64_t{107}});
    TEST("u4{100}>u2{7}", x4 > x2);
}

template <std::size_t N>
static void run_all(const char *tag)
{
    test_construction<N>(tag);
    test_comparison<N>(tag);
    test_bitwise<N>(tag);
    test_shifts<N>(tag);
    test_addition<N>(tag);
    test_subtraction<N>(tag);
    test_negation<N>(tag);
    test_inc_dec<N>(tag);
    test_multiplication<N>(tag);
    test_utility<N>(tag);
    test_strings<N>(tag);
    test_mixed_ops<N>(tag);
    test_higher_arith<N>(tag);
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "uint_fixed_t<N> Tests (v1.90)\n";
    std::cout << "====================================================================\n";

    run_all<1>("uint64_fixed");
    run_all<2>("uint128_fixed");
    run_all<4>("uint256_fixed");
    run_all<8>("uint512_fixed");

    test_cross_n_uint();

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
