// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: uint_fixed_t<N> — unsigned fixed-width integer template
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Covers N=2 (128-bit), N=4 (256-bit), N=8 (512-bit).
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

#include "int_fixed.hpp"

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
    TEST("zero data[0]==0", z.data[0] == 0);
    TEST("one  data[0]==1", o.data[0] == 1);
    TEST("max  data[0]==~0", m.data[0] == ~std::uint64_t{0});
    TEST("max  data[N-1]==~0", m.data[N - 1] == ~std::uint64_t{0});
    TEST("one  data[N-1]==0", o.data[N - 1] == 0);

    // Default constructor gives zero
    const uint_fixed_t<N> def{};
    TEST("default == zero", def == z);

    // uint64_t constructor
    const uint_fixed_t<N> v42{std::uint64_t{42}};
    TEST("from_u64 data[0]==42", v42.data[0] == 42);
    TEST("from_u64 data[N-1]==0", v42.data[N - 1] == 0);
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

    // Multi-limb ordering: compare only on high limb difference
    uint_fixed_t<N> big{};
    big.data[N - 1] = 1; // 2^(64*(N-1)), much bigger than one
    TEST("big > one (MSL dominates)", big > o);
    TEST("one < big (MSL dominates)", o < big);
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
        TEST("(1<<64) data[0]==0", shifted.data[0] == 0);
        TEST("(1<<64) data[1]==1", shifted.data[1] == 1);
        const auto back = shifted >> 64;
        TEST("(1<<64)>>64 == one", back == o);
    }

    // Shift max right fills with zeros from MSB
    TEST("max >> (64N-1) == one", (m >> (64U * N - 1)) == o);
    TEST("max << (64N-1): only MSB set",
         (m << (64U * N - 1)).data[N - 1] == (std::uint64_t{1} << 63));
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
            volatile std::uint64_t limb = sum.data[i];
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
        pow64.data[1] = 1;
        // 2^64 - 1 == max of lower 64 bits = 0xFFFF...FFFF (low), 0 (rest)
        const auto res = pow64 - o;
        TEST("2^64 - 1: data[0]==~0", res.data[0] == ~std::uint64_t{0});
        TEST("2^64 - 1: data[1]==0", res.data[1] == 0);
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
        pow64.data[1] = 1; // 2^64
        TEST("2^63 * 2 == 2^64 (cross-limb)", (pow63 * two) == pow64);
    }

    // Square: (2^32)^2 == 2^64 (cross-limb, N>=2)
    if constexpr (N >= 2)
    {
        const uint_fixed_t<N> pow32{std::uint64_t{1} << 32};
        uint_fixed_t<N> pow64{};
        pow64.data[1] = 1;
        TEST("(2^32)^2 == 2^64", (pow32 * pow32) == pow64);
    }
}

// =============================================================================
// Section 10: Utility — is_zero, bit_width, popcount
// =============================================================================

template <std::size_t N>
static void test_utility(const char *tag)
{
    std::cout << "\n--- Section 10: Utility [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    TEST("zero.is_zero()", z.is_zero());
    TEST("one .is_zero()==false", !o.is_zero());
    TEST("max .is_zero()==false", !m.is_zero());

    TEST("zero.bit_width()==0", z.bit_width() == 0);
    TEST("one .bit_width()==1", o.bit_width() == 1);
    TEST("max .bit_width()==64N", m.bit_width() == 64U * N);

    const uint_fixed_t<N> two{std::uint64_t{2}};
    TEST("2.bit_width()==2", two.bit_width() == 2);

    TEST("zero.popcount()==0", z.popcount() == 0);
    TEST("one .popcount()==1", o.popcount() == 1);
    TEST("max .popcount()==64N", m.popcount() == 64U * N);
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
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "uint_fixed_t<N> Tests (v1.90)\n";
    std::cout << "====================================================================\n";

    run_all<2>("uint128_fixed");
    run_all<4>("uint256_fixed");
    run_all<8>("uint512_fixed");

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
