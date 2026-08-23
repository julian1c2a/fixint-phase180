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

#include "fixed_width_int_t.hpp"

#include <cstdlib>
#include <iostream>

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

    TEST("theorem: 0/7", check("0/7", 0, 7));
    TEST("theorem: 1/7", check("1/7", 1, 7));
    TEST("theorem: 6/7", check("6/7", 6, 7));
    TEST("theorem: 7/7", check("7/7", 7, 7));
    TEST("theorem: 100/7", check("100/7", 100, 7));
    TEST("theorem: 100/10", check("100/10", 100, 10));
    TEST("theorem: max64/1", check("max64/1", ~std::uint64_t{0}, 1));
    TEST("theorem: max64/3", check("max64/3", ~std::uint64_t{0}, 3));
    TEST("theorem: max64/7", check("max64/7", ~std::uint64_t{0}, 7));
    TEST("theorem: max64/max64", check("max64/max64", ~std::uint64_t{0}, ~std::uint64_t{0}));

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
        pow64.set_limb(1, 1); // 2^64
        const uint_fixed_t<N> two{std::uint64_t{2}};
        uint_fixed_t<N> pow63{std::uint64_t{1} << 63};
        TEST("2^64 / 2 == 2^63", pow64 / two == pow63);
        TEST("2^64 % 2 == 0", pow64 % two == uint_fixed_t<N>::zero());

        // 2^64 / 2^32 == 2^32
        const uint_fixed_t<N> pow32{std::uint64_t{1} << 32};
        TEST("2^64 / 2^32 == 2^32", pow64 / pow32 == pow32);
        TEST("2^64 % 2^32 == 0", pow64 % pow32 == uint_fixed_t<N>::zero());

        // (2^64 + 1) / 2 == 2^63, remainder 1
        uint_fixed_t<N> pow64p1{};
        pow64p1.set_limb(1, 1);
        pow64p1.set_limb(0, 1);
        const auto [q2, r2] = uint_fixed_t<N>::divmod(pow64p1, two);
        TEST("(2^64+1)/2 == 2^63", q2 == pow63);
        TEST("(2^64+1)%2 == 1", r2 == uint_fixed_t<N>::one());

        // large / large: dividend has high limb, divisor spans two limbs
        // (2^64 + 5) / (2^32 + 1): check fundamental theorem
        uint_fixed_t<N> big_a{};
        big_a.set_limb(1, 1);
        big_a.set_limb(0, 5);
        uint_fixed_t<N> big_b{std::uint64_t{1}};
        big_b.set_limb(0, (std::uint64_t{1} << 32) + 1U);
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

    TEST("1/0 throws", throws_domain([&] { (void)(o / z); }));
    TEST("max/0 throws", throws_domain([&] { (void)(m / z); }));
    TEST("0/0 throws", throws_domain([&] { (void)(z / z); }));
    TEST("1%0 throws", throws_domain([&] { (void)(o % z); }));
    TEST("divmod(1,0) throws", throws_domain([&] { (void)uint_fixed_t<N>::divmod(o, z); }));
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

    TEST("divmod==op pair: 100/7", check_pair(100, 7));
    TEST("divmod==op pair: 999/37", check_pair(999, 37));
    TEST("divmod==op pair: max64/255", check_pair(~std::uint64_t{0}, 255));
    TEST("divmod==op pair: 0/1", check_pair(0, 1));
    TEST("divmod==op pair: 1/1", check_pair(1, 1));
}

// =============================================================================
// Section 7: Single-limb divisor fast path — multi-limb dividend
//
// Exercises the O(N) hardware-division path added for the case where the
// divisor fits in a single 64-bit limb (data[1..N-1] == 0).
// All N limbs of the dividend are non-zero to trigger carry propagation
// through every divq step.
// Correctness criterion: q * d + r == a  AND  r < d.
// =============================================================================

template <std::size_t N>
static void test_single_limb_divisor(const char *tag)
{
    std::cout << "\n--- Section 7: Single-limb divisor fast path [N=" << N << " " << tag << "] ---\n";

    // Helper: build from explicit limb array and verify fundamental theorem
    auto check = [](const char *name, uint_fixed_t<N> a, std::uint64_t d) -> bool
    {
        const uint_fixed_t<N> divisor{d};
        const auto [q, r] = uint_fixed_t<N>::divmod(a, divisor);
        bool ok = (q * divisor + r) == a && r < divisor;
        if (!ok)
            std::cout << "  FAIL detail: " << name << "\n";
        return ok;
    };

    // 1. All-ones dividend (0xFFFF...FFFF) / small divisors
    const auto all_ones = uint_fixed_t<N>::max();
    TEST("all_ones/1", check("all_ones/1", all_ones, 1));
    TEST("all_ones/2", check("all_ones/2", all_ones, 2));
    TEST("all_ones/3", check("all_ones/3", all_ones, 3));
    TEST("all_ones/7", check("all_ones/7", all_ones, 7));
    TEST("all_ones/10", check("all_ones/10", all_ones, 10));
    TEST("all_ones/100", check("all_ones/100", all_ones, 100));

    // 2. Powers of 2 as divisor (shift-like behaviour)
    TEST("all_ones/2^32", check("all_ones/2^32", all_ones, std::uint64_t{1} << 32));
    TEST("all_ones/2^48", check("all_ones/2^48", all_ones, std::uint64_t{1} << 48));
    TEST("all_ones/2^63", check("all_ones/2^63", all_ones, std::uint64_t{1} << 63));

    // 3. Near-max divisor: divisor = 2^64 - 1 (= 0xFFFF...FFFF in 64 bits)
    TEST("all_ones/max64", check("all_ones/max64", all_ones, ~std::uint64_t{0}));

    // 4. Dividend with all limbs set to distinct non-zero patterns
    if constexpr (N >= 2)
    {
        uint_fixed_t<N> patterned{};
        for (std::size_t i = 0; i < N; ++i)
            patterned.set_limb(i, 0xDEADBEEF00000000ULL | static_cast<std::uint64_t>(i + 1));

        TEST("patterned/3", check("patterned/3", patterned, 3));
        TEST("patterned/7", check("patterned/7", patterned, 7));
        TEST("patterned/10^9+7", check("patterned/10^9+7", patterned, 1'000'000'007ULL));
        TEST("patterned/2^32+1", check("patterned/2^32+1", patterned, (std::uint64_t{1} << 32) + 1));
        TEST("patterned/max64-1", check("patterned/max64-1", patterned, ~std::uint64_t{0} - 1));
        TEST("patterned/max64", check("patterned/max64", patterned, ~std::uint64_t{0}));
    }

    // 5. Dividend fits in one limb (a < 2^64): verify same result as native uint64_t
    if constexpr (N >= 2)
    {
        const std::uint64_t av = 0xCAFEBABEDEADBEEFULL;
        const std::uint64_t dv = 0x0000'0000'FFFF'FFFFULL;
        const uint_fixed_t<N> a{av};
        const uint_fixed_t<N> divisor{dv};
        const auto [q, r] = uint_fixed_t<N>::divmod(a, divisor);
        TEST("single-limb a/d matches u64", q == uint_fixed_t<N>{av / dv} && r == uint_fixed_t<N>{av % dv});
    }

    // 6. Dividend just larger than divisor (quotient = 1, remainder = a - d)
    {
        const std::uint64_t d = 0xABCDEF0123456789ULL;
        uint_fixed_t<N> a{d};
        a += uint_fixed_t<N>{std::uint64_t{1}}; // a = d + 1
        const auto [q, r] = uint_fixed_t<N>::divmod(a, uint_fixed_t<N>{d});
        TEST("d+1 / d == 1 r1", q == uint_fixed_t<N>::one() && r == uint_fixed_t<N>::one());
    }
}

// =============================================================================
// Section 8: Knuth Algorithm D — multi-limb divisor (M ≥ 2)
//
// Exercises the Knuth D path: divisor has ≥ 2 significant limbs.
// Only meaningful for N ≥ 4 (N=2 with 2-limb divisors is handled by the
// __uint128_t fast path on GCC/Clang, and by Knuth D on MSVC/ICX-Win).
// Correctness criterion: q * b + r == a  AND  r < b.
// =============================================================================

template <std::size_t N>
static void test_knuth_d(const char *tag)
{
    std::cout << "\n--- Section 8: Knuth D multi-limb divisor [N=" << N << " " << tag << "] ---\n";

    if constexpr (N < 4)
    {
        std::cout << "  (skipped: N<4 covered by __uint128_t path)\n";
        return;
    }

    // Helper: verify fundamental theorem  q*b + r == a  AND  r < b
    auto check = [](const char *name, uint_fixed_t<N> a, uint_fixed_t<N> b) -> bool
    {
        const auto [q, r] = uint_fixed_t<N>::divmod(a, b);
        bool ok = (q * b + r) == a && r < b;
        if (!ok)
            std::cout << "  FAIL detail: " << name << "\n";
        return ok;
    };

    // Build helpers: set specific limbs
    auto make = [](std::initializer_list<std::uint64_t> limbs) -> uint_fixed_t<N>
    {
        uint_fixed_t<N> v{};
        std::size_t i = 0;
        for (std::uint64_t w : limbs)
        {
            if (i < N)
                v.set_limb(i++, w);
        }
        return v;
    };

    // ── 1. Dividend just above divisor (quotient = 1) ──
    {
        uint_fixed_t<N> b{};
        b.set_limb(0, 0xFEDCBA9876543210ULL);
        b.set_limb(1, 0x0123456789ABCDEFULL); // 2-limb divisor
        uint_fixed_t<N> a = b;
        a += uint_fixed_t<N>{std::uint64_t{1}};
        const auto [q, r] = uint_fixed_t<N>::divmod(a, b);
        TEST("(b+1)/b == 1 r1 (2-limb div)", q == uint_fixed_t<N>::one() && r == uint_fixed_t<N>::one());
    }

    // ── 2. 2^128 / (2^64 + 1)  ──  divisor has 2 limbs, dividend has 3
    if constexpr (N >= 4)
    {
        uint_fixed_t<N> a{};
        a.set_limb(2, 1); // a = 2^128
        uint_fixed_t<N> b{};
        b.set_limb(0, 1);
        b.set_limb(1, 1); // b = 2^64 + 1
        TEST("2^128 / (2^64+1)", check("2^128/(2^64+1)", a, b));
    }

    // ── 3. max-value / 3-limb divisor ──
    if constexpr (N >= 4)
    {
        const auto a = uint_fixed_t<N>::max();
        uint_fixed_t<N> b{};
        b.set_limb(0, 0xFFFFFFFFFFFFFFFFULL);
        b.set_limb(1, 0xAAAAAAAAAAAAAAAAULL);
        b.set_limb(2, 0x5555555555555555ULL);
        TEST("maxN / 3-limb divisor", check("maxN/3limb", a, b));
    }

    // ── 4. Alternating-pattern 4-limb dividend, 2-limb divisor ──
    if constexpr (N >= 4)
    {
        uint_fixed_t<N> a{};
        a.set_limb(0, 0xDEADBEEFCAFEBABEULL);
        a.set_limb(1, 0x0123456789ABCDEFULL);
        a.set_limb(2, 0xFEDCBA9876543210ULL);
        a.set_limb(3, 0x1111111111111111ULL);
        uint_fixed_t<N> b{};
        b.set_limb(0, 0xFFFFFFFF00000001ULL);
        b.set_limb(1, 0x0000000100000000ULL);
        TEST("4-limb / 2-limb patterned", check("4limb/2limb", a, b));
    }

    // ── 5. 4-limb dividend / 3-limb divisor ──
    if constexpr (N >= 4)
    {
        uint_fixed_t<N> a{};
        a.set_limb(0, 0x123456789ABCDEF0ULL);
        a.set_limb(1, 0xFEDCBA9876543210ULL);
        a.set_limb(2, 0xAAAABBBBCCCCDDDDULL);
        a.set_limb(3, 0x0000000000000007ULL);
        uint_fixed_t<N> b{};
        b.set_limb(0, 0x9999999999999999ULL);
        b.set_limb(1, 0x7777777777777777ULL);
        b.set_limb(2, 0x0000000000000003ULL);
        TEST("4-limb / 3-limb divisor", check("4limb/3limb", a, b));
    }

    // ── 6. Self-division (N-limb / N-limb == 1, rem == 0) ──
    {
        uint_fixed_t<N> a{};
        for (std::size_t i = 0; i < N; ++i)
            a.set_limb(i, 0xABCDEF0123456789ULL ^ (std::uint64_t{i + 1} * 0x1111111111111111ULL));
        const auto [q, r] = uint_fixed_t<N>::divmod(a, a);
        TEST("self-division == 1 r0", q == uint_fixed_t<N>::one() && r == uint_fixed_t<N>::zero());
    }

    // ── 7. Divisor = 2^64 (exactly, in limb 1) ──  quotient = a >> 64
    if constexpr (N >= 4)
    {
        uint_fixed_t<N> a{};
        a.set_limb(0, 0xABCDEF0123456789ULL);
        a.set_limb(1, 0x0FEDCBA987654321ULL);
        a.set_limb(2, 0x1234567890ABCDEFULL);
        uint_fixed_t<N> b{};
        b.set_limb(1, 1); // b = 2^64
        const auto [q, r] = uint_fixed_t<N>::divmod(a, b);
        // q should be a >> 64: data[0]=a.limb(1), data[1]=a.limb(2), ...
        uint_fixed_t<N> expected_q{};
        for (std::size_t i = 0; i < N - 1; ++i)
            expected_q.set_limb(i, a.limb(i + 1));
        TEST("a / 2^64 == a>>64", q == expected_q);
        TEST("a % 2^64 == a[0]", r == uint_fixed_t<N>{a.limb(0)});
    }

    // ── 8. max / (max/2 + 1): quotient = 1, remainder = max/2 - 1 ──
    {
        const auto max_val = uint_fixed_t<N>::max();
        uint_fixed_t<N> half = max_val >> 1;                          // max/2
        uint_fixed_t<N> b = half + uint_fixed_t<N>{std::uint64_t{1}}; // max/2+1
        TEST("max/(max/2+1)", check("max/(max/2+1)", max_val, b));
    }

    // ── 9. Large N: 8-limb / 4-limb ──
    if constexpr (N >= 8)
    {
        uint_fixed_t<N> a{};
        for (std::size_t i = 0; i < N; ++i)
            a.set_limb(i, 0xFEDCBA9876543210ULL - i * 0x1111111111111111ULL);
        uint_fixed_t<N> b{};
        for (std::size_t i = 0; i < 4; ++i)
            b.set_limb(i, 0x123456789ABCDEF0ULL + i * 0x0101010101010101ULL);
        TEST("8-limb / 4-limb", check("8limb/4limb", a, b));
    }

    // ── 10. Normalisation: divisor top limb = 1 (s=63, maximum shift) ──
    if constexpr (N >= 4)
    {
        uint_fixed_t<N> b{};
        b.set_limb(0, 0xFFFFFFFFFFFFFFFFULL);
        b.set_limb(1, 1); // top limb = 1  ⟹  s = 63
        uint_fixed_t<N> a{};
        a.set_limb(0, 0);
        a.set_limb(1, 0xFFFFFFFFFFFFFFFFULL);
        a.set_limb(2, 2); // a = 2·(2^128) + 0xFFFF...:0
        TEST("s=63 normalisation path", check("s63norm", a, b));
    }

    // ── 11. Normalisation: divisor top limb = 0x8000..0 (s=0, no shift) ──
    if constexpr (N >= 4)
    {
        uint_fixed_t<N> b{};
        b.set_limb(0, 0x1ULL);
        b.set_limb(1, std::uint64_t{1} << 63); // top limb MSB set ⟹ s = 0
        const auto a = uint_fixed_t<N>::max();
        TEST("s=0 normalisation path", check("s0norm", a, b));
    }
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
    test_single_limb_divisor<N>(tag);
    test_knuth_d<N>(tag);
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
