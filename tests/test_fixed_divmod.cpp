// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: uint_fixed_t<N> — division and modulo
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Covers N=2 (128-bit), N=4 (256-bit), N=8 (512-bit).
// Sections:
//   1.  Division: identity, self, zero dividend, known values
//   2.  Modulo: identity, self, zero dividend, known values
//   3.  Fundamental theorem: a == (a/b)*b + (a%b)
//   4.  Cross-limb (N>=2): divisors and dividends spanning limb boundaries
//   5.  Division by zero throws std::domain_error
//   6.  divmod() returns consistent quotient and remainder pair

#include "int_fixed.hpp"

#include <cstdlib>
#include <iostream>

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
// Section 1: Division
// =============================================================================

template <std::size_t N>
static void test_division(const char *tag)
{
    std::cout << "\n--- Section 1: Division [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();

    const uint_fixed_t<N> three{std::uint64_t{3}};
    const uint_fixed_t<N> seven{std::uint64_t{7}};
    const uint_fixed_t<N> twentyone{std::uint64_t{21}};
    const uint_fixed_t<N> ten{std::uint64_t{10}};
    const uint_fixed_t<N> hundred{std::uint64_t{100}};

    // a / 1 == a
    TEST("21 / 1 == 21", twentyone / o == twentyone);
    TEST("0 / 1 == 0", z / o == z);

    // 0 / a == 0 (a != 0)
    TEST("0 / 7 == 0", z / seven == z);

    // a / a == 1
    TEST("7 / 7 == 1", seven / seven == o);
    TEST("21 / 21 == 1", twentyone / twentyone == o);

    // known values
    TEST("21 / 7 == 3", twentyone / seven == three);
    TEST("21 / 3 == 7", twentyone / three == seven);
    TEST("100 / 10 == 10", hundred / ten == ten);

    // a < b → quotient 0
    TEST("3 / 7 == 0", three / seven == z);

    // a / b * b <= a
    const uint_fixed_t<N> big{std::uint64_t{0x123456789ABCDEFULL}};
    const uint_fixed_t<N> div7 = big / seven;
    TEST("big/7*7 <= big", div7 * seven <= big);
}

// =============================================================================
// Section 2: Modulo
// =============================================================================

template <std::size_t N>
static void test_modulo(const char *tag)
{
    std::cout << "\n--- Section 2: Modulo [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();

    const uint_fixed_t<N> three{std::uint64_t{3}};
    const uint_fixed_t<N> seven{std::uint64_t{7}};
    const uint_fixed_t<N> ten{std::uint64_t{10}};
    const uint_fixed_t<N> hundred{std::uint64_t{100}};

    // a % 1 == 0
    TEST("100 % 1 == 0", hundred % o == z);

    // 0 % a == 0
    TEST("0 % 7 == 0", z % seven == z);

    // a % a == 0
    TEST("7 % 7 == 0", seven % seven == z);

    // known values
    TEST("100 % 7 == 2", hundred % seven == uint_fixed_t<N>{std::uint64_t{2}});
    TEST("100 % 10 == 0", hundred % ten == z);
    TEST("10 % 3 == 1", ten % three == o);
    TEST("7 % 10 == 7", seven % ten == seven);

    // remainder < divisor
    const uint_fixed_t<N> big{std::uint64_t{0xFEDCBA9876543210ULL}};
    TEST("big%7 < 7", (big % seven) < seven);
    TEST("big%10 < 10", (big % ten) < ten);
}

// =============================================================================
// Section 3: Fundamental theorem: a == (a/b)*b + (a%b)
// =============================================================================

template <std::size_t N>
static void test_fundamental(const char *tag)
{
    std::cout << "\n--- Section 3: Fundamental theorem [N=" << N << " " << tag << "] ---\n";

    auto check = [](const char *name, std::uint64_t av, std::uint64_t bv) -> bool
    {
        const uint_fixed_t<N> a{av};
        const uint_fixed_t<N> b{bv};
        const auto [q, r] = uint_fixed_t<N>::divmod(a, b);
        return (q * b + r) == a && r < b;
    };

    TEST("theorem: 0/7",        check("0/7",        0,                       7));
    TEST("theorem: 1/7",        check("1/7",        1,                       7));
    TEST("theorem: 6/7",        check("6/7",        6,                       7));
    TEST("theorem: 7/7",        check("7/7",        7,                       7));
    TEST("theorem: 100/7",      check("100/7",      100,                     7));
    TEST("theorem: 100/10",     check("100/10",     100,                     10));
    TEST("theorem: max64/1",    check("max64/1",    ~std::uint64_t{0},       1));
    TEST("theorem: max64/3",    check("max64/3",    ~std::uint64_t{0},       3));
    TEST("theorem: max64/7",    check("max64/7",    ~std::uint64_t{0},       7));
    TEST("theorem: max64/max64",check("max64/max64",~std::uint64_t{0}, ~std::uint64_t{0}));

    // Multi-limb check using larger values
    {
        const uint_fixed_t<N> a{std::uint64_t{0xCAFEBABEDEADBEEFULL}};
        const uint_fixed_t<N> b{std::uint64_t{0x0FEDCBA987654321ULL}};
        const auto [q, r] = uint_fixed_t<N>::divmod(a, b);
        TEST("theorem: cafe/0fed", (q * b + r) == a && r < b);
    }
}

// =============================================================================
// Section 4: Cross-limb (N>=2)
// =============================================================================

template <std::size_t N>
static void test_crosslimb(const char *tag)
{
    std::cout << "\n--- Section 4: Cross-limb [N=" << N << " " << tag << "] ---\n";

    if constexpr (N >= 2)
    {
        // 2^64 / 2 == 2^63
        uint_fixed_t<N> pow64{};
        pow64.data[1] = 1; // 2^64
        const uint_fixed_t<N> two{std::uint64_t{2}};
        uint_fixed_t<N> pow63{std::uint64_t{1} << 63};
        TEST("2^64 / 2 == 2^63", pow64 / two == pow63);
        TEST("2^64 % 2 == 0",    pow64 % two == uint_fixed_t<N>::zero());

        // 2^64 / 2^32 == 2^32
        const uint_fixed_t<N> pow32{std::uint64_t{1} << 32};
        TEST("2^64 / 2^32 == 2^32", pow64 / pow32 == pow32);
        TEST("2^64 % 2^32 == 0",    pow64 % pow32 == uint_fixed_t<N>::zero());

        // (2^64 + 1) / 2 == 2^63, remainder 1
        uint_fixed_t<N> pow64p1{};
        pow64p1.data[1] = 1;
        pow64p1.data[0] = 1;
        const auto [q2, r2] = uint_fixed_t<N>::divmod(pow64p1, two);
        TEST("(2^64+1)/2 == 2^63", q2 == pow63);
        TEST("(2^64+1)%2 == 1",    r2 == uint_fixed_t<N>::one());

        // large / large: dividend has high limb, divisor spans two limbs
        // (2^64 + 5) / (2^32 + 1): check fundamental theorem
        uint_fixed_t<N> big_a{};
        big_a.data[1] = 1;
        big_a.data[0] = 5;
        uint_fixed_t<N> big_b{std::uint64_t{1}};
        big_b.data[0] = (std::uint64_t{1} << 32) + 1U;
        const auto [q3, r3] = uint_fixed_t<N>::divmod(big_a, big_b);
        TEST("cross-limb theorem", (q3 * big_b + r3) == big_a && r3 < big_b);
    }
    else
    {
        std::cout << "  (skipped for N<2)\n";
    }
}

// =============================================================================
// Section 5: Division by zero throws
// =============================================================================

template <std::size_t N>
static void test_divzero(const char *tag)
{
    std::cout << "\n--- Section 5: Division by zero [N=" << N << " " << tag << "] ---\n";

    const auto z = uint_fixed_t<N>::zero();
    const auto o = uint_fixed_t<N>::one();
    const auto m = uint_fixed_t<N>::max();

    auto throws_domain = [](auto &&fn) -> bool
    {
        try { fn(); return false; }
        catch (const std::domain_error &) { return true; }
        catch (...) { return false; }
    };

    TEST("1/0 throws", throws_domain([&]{ (void)(o / z); }));
    TEST("max/0 throws", throws_domain([&]{ (void)(m / z); }));
    TEST("0/0 throws", throws_domain([&]{ (void)(z / z); }));
    TEST("1%0 throws", throws_domain([&]{ (void)(o % z); }));
    TEST("divmod(1,0) throws", throws_domain([&]{ (void)uint_fixed_t<N>::divmod(o, z); }));
}

// =============================================================================
// Section 6: divmod() consistency
// =============================================================================

template <std::size_t N>
static void test_divmod_consistency(const char *tag)
{
    std::cout << "\n--- Section 6: divmod() consistency [N=" << N << " " << tag << "] ---\n";

    auto check_pair = [](std::uint64_t av, std::uint64_t bv) -> bool
    {
        const uint_fixed_t<N> a{av};
        const uint_fixed_t<N> b{bv};
        const auto [q, r] = uint_fixed_t<N>::divmod(a, b);
        return q == a / b && r == a % b;
    };

    TEST("divmod==op pair: 100/7",    check_pair(100, 7));
    TEST("divmod==op pair: 999/37",   check_pair(999, 37));
    TEST("divmod==op pair: max64/255",check_pair(~std::uint64_t{0}, 255));
    TEST("divmod==op pair: 0/1",      check_pair(0, 1));
    TEST("divmod==op pair: 1/1",      check_pair(1, 1));
}

// =============================================================================
// Run all sections for one N
// =============================================================================

template <std::size_t N>
static void run_all(const char *tag)
{
    test_division<N>(tag);
    test_modulo<N>(tag);
    test_fundamental<N>(tag);
    test_crosslimb<N>(tag);
    test_divzero<N>(tag);
    test_divmod_consistency<N>(tag);
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "uint_fixed_t<N> Division/Modulo Tests (v1.90)\n";
    std::cout << "====================================================================\n";

    run_all<2>("uint128_fixed");
    run_all<4>("uint256_fixed");
    run_all<8>("uint512_fixed");

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
