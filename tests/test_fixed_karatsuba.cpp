// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// test_fixed_karatsuba.cpp — Karatsuba operator* correctness for N=4 and N=8
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Strategy: compare N=4/N=8 operator* (Karatsuba path) against a reference
// assembled from N=2 schoolbook products.
//
// For N=4 (a, b each 4 limbs):
//   ref = a_lo*b_lo (4-limb product from two N=2 results)
//       + (a_lo*b_hi + a_hi*b_lo) << 128 bits  (mod 2^256)
//
// For N=8 the reference is built similarly from N=4 Karatsuba parts.
//
// Sections:
//   1. Known-value products (N=4)
//   2. Commutativity / identity checks (N=4)
//   3. Cross-check vs N=2 reference assembly (N=4)
//   4. Known-value products (N=8)
//   5. Commutativity / identity checks (N=8)
//   6. Cross-check vs N=4 reference assembly (N=8)

#include "fixed_width_int_t.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

using nstd::int_fixed_t;
using nstd::uint_fixed_t;
using std::uint64_t;

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

static constexpr uint64_t MAX64 = ~uint64_t{0};

// Build a uint_fixed_t<N> from an initializer list of limbs (LSB first)
template <std::size_t N>
static uint_fixed_t<N> make(std::initializer_list<uint64_t> limbs)
{
    uint_fixed_t<N> v{};
    std::size_t i = 0;
    for (uint64_t x : limbs)
        if (i < N)
            v.set_limb(i++, x);
    return v;
}

// =============================================================================
// Section 1: Known-value products (N=4 / 256-bit)
// =============================================================================

static void test_known_n4(const char *tag)
{
    std::cout << "\n--- " << tag << " : Section 1 (N=4 known-value products) ---\n";

    using u256 = uint_fixed_t<4>;

    // 1×1 = 1
    {
        u256 a = make<4>({1, 0, 0, 0});
        TEST("1*1 = 1", (a * a) == make<4>({1, 0, 0, 0}));
    }
    // 0×anything = 0
    {
        u256 zero{};
        u256 big = make<4>({MAX64, MAX64, MAX64, MAX64});
        TEST("0*big = 0", (zero * big) == zero);
    }
    // 2×3 = 6
    {
        u256 two = make<4>({2, 0, 0, 0});
        u256 three = make<4>({3, 0, 0, 0});
        TEST("2*3 = 6", (two * three) == make<4>({6, 0, 0, 0}));
    }
    // (2^64)^2 = 2^128  →  {0, 0, 1, 0}
    {
        u256 b64 = make<4>({0, 1, 0, 0});
        TEST("2^64 * 2^64 = 2^128", (b64 * b64) == make<4>({0, 0, 1, 0}));
    }
    // (2^128)^2 = 2^256 ≡ 0 mod 2^256
    {
        u256 b128 = make<4>({0, 0, 1, 0});
        TEST("2^128 * 2^128 ≡ 0 mod 2^256", (b128 * b128) == u256{});
    }
    // (2^64 + 1)^2 = 2^128 + 2^65 + 1  →  {1, 2, 1, 0}
    {
        u256 a = make<4>({1, 1, 0, 0});
        TEST("(2^64+1)^2 = {1,2,1,0}", (a * a) == make<4>({1, 2, 1, 0}));
    }
    // (2^128 - 1)^2 mod 2^256 = 2^256 - 2^129 + 1 = {1, 0, MAX64-1, MAX64}
    {
        u256 a = make<4>({MAX64, MAX64, 0, 0});
        TEST("(2^128-1)^2 mod 2^256 = {1,0,MAX-1,MAX}", (a * a) == make<4>({1, 0, MAX64 - 1, MAX64}));
    }
    // max×1 = max
    {
        u256 mx = make<4>({MAX64, MAX64, MAX64, MAX64});
        u256 one = make<4>({1, 0, 0, 0});
        TEST("max*1 = max", (mx * one) == mx);
    }
    // max×max mod 2^256: (2^256-1)^2 mod 2^256 = 2^257-2 mod 2^256 = {MAX64-1, MAX64, MAX64, MAX64}
    // Actually: (2^256-1)^2 = 2^512 - 2^257 + 1 mod 2^256
    // mod 2^256: 2^512 ≡ 0, -2^257 ≡ -2^257 mod 2^256 = 2^256 - 2^257 + 2^256 ... let me use 2's complement:
    // -2^257 mod 2^256 = -2*2^256 mod 2^256 = 0
    // So (2^256-1)^2 mod 2^256 = 0 + 1 = 1? That can't be right...
    // (2^256-1)^2 mod 2^256 = (2^256-1)*(2^256-1) mod 2^256
    //   = (2^256*2^256 - 2*2^256 + 1) mod 2^256
    //   = (0 - 0 + 1) mod 2^256 = 1
    // So max*max mod 2^256 = 1
    {
        u256 mx = make<4>({MAX64, MAX64, MAX64, MAX64});
        TEST("max*max mod 2^256 = 1", (mx * mx) == make<4>({1, 0, 0, 0}));
    }
}

// =============================================================================
// Section 2: Commutativity and algebraic identities (N=4)
// =============================================================================

static void test_identities_n4(const char *tag)
{
    std::cout << "\n--- " << tag << " : Section 2 (N=4 identities) ---\n";

    using u256 = uint_fixed_t<4>;

    // Some non-trivial test values
    const u256 a =
        make<4>({0xDEADBEEFCAFEBABEULL, 0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL, 0x1111222233334444ULL});
    const u256 b =
        make<4>({0x9999888877776666ULL, 0xAAAABBBBCCCCDDDDULL, 0xEEEEFFFF00001111ULL, 0x2222333344445555ULL});
    const u256 one = make<4>({1, 0, 0, 0});
    const u256 zero{};

    TEST("N=4 commutativity a*b == b*a", (a * b) == (b * a));
    TEST("N=4 a*1 == a", (a * one) == a);
    TEST("N=4 1*a == a", (one * a) == a);
    TEST("N=4 a*0 == 0", (a * zero) == zero);
    TEST("N=4 0*a == 0", (zero * a) == zero);
    TEST("N=4 a*a commutativity (trivial)", (a * a) == (a * a));

    // Distributivity: a*(b+c) == a*b + a*c (for small values where no overflow confusion)
    const u256 c = make<4>({3, 0, 0, 0});
    const u256 d = make<4>({7, 0, 0, 0});
    TEST("N=4 distributivity: a*(b+c) == a*b+a*c (small)", (c * (d + one)) == (c * d + c * one));

    // a*a == a*a (regression: Karatsuba symmetry)
    const u256 x = make<4>({0xFEDCBA9876543210ULL, 0x123456789ABCDEFULL, 1, 0});
    TEST("N=4 a*a self-consistency", (x * x) == (x * x));
}

// =============================================================================
// Section 3: Cross-check N=4 Karatsuba vs N=2 schoolbook reference
// For a = [a1|a0], b = [b1|b0] (each N=2 halves):
//   ref = a0*b0 (full 4-limb) + (a0*b1 + a1*b0) << 128  mod 2^256
// =============================================================================

static void test_vs_n2_ref(const char *tag)
{
    std::cout << "\n--- " << tag << " : Section 3 (N=4 vs N=2 reference) ---\n";

    using u256 = uint_fixed_t<4>;
    using u128 = uint_fixed_t<2>;

    // Assemble 4-limb product from N=2 pieces
    auto ref_mul = [](u256 a, u256 b) -> u256
    {
        // Extract halves (each 2 limbs)
        u128 a_lo = make<2>({a.limb(0), a.limb(1)});
        u128 a_hi = make<2>({a.limb(2), a.limb(3)});
        u128 b_lo = make<2>({b.limb(0), b.limb(1)});
        u128 b_hi = make<2>({b.limb(2), b.limb(3)});

        // Full 2-limb×2-limb products using N=2 schoolbook (via __uint128_t or _umul128)
        // We need the full 4-limb product a_lo*b_lo, computed as:
        //   using uint128_t arithmetic
        // Schoolbook for 2×2→4 (a0*b0, a0*b1, a1*b0, a1*b1):
        //   result[0] = lo(a0*b0)
        //   result[1] = hi(a0*b0) + lo(a0*b1) + lo(a1*b0)  [+ carries]
        //   result[2] = hi(a0*b1) + hi(a1*b0) + lo(a1*b1)  [+ carries]
        //   result[3] = hi(a1*b1)                           [+ carries]
        // But we can just use 4 N=2 multiplications:
        //
        // Instead, build z0 (4 limbs) using 4 separate 64×64 products
        // This is a clean schoolbook reference, independent of Karatsuba.
        const uint64_t a0 = a_lo.limb(0), a1 = a_lo.limb(1);
        const uint64_t a2 = a_hi.limb(0), a3 = a_hi.limb(1);
        const uint64_t b0 = b_lo.limb(0), b1 = b_lo.limb(1);
        const uint64_t b2 = b_hi.limb(0), b3 = b_hi.limb(1);

        // Full schoolbook 4×4→4 (truncated to 4 limbs)
        u256 r{};
        auto add_to = [&r](std::size_t pos, uint64_t lo, uint64_t hi)
        {
            // Add lo at r[pos], hi at r[pos+1], with carry propagation
            if (pos >= 4)
                return;
            uint64_t old = r.limb(pos);
            r.set_limb(pos, r.limb(pos) + (lo));
            uint64_t c = (r.limb(pos) < old) ? 1 : 0;
            if (pos + 1 < 4)
            {
                old = r.limb(pos + 1);
                r.set_limb(pos + 1, r.limb(pos + 1) + (hi + c));
                c = (r.limb(pos + 1) < old || (c && r.limb(pos + 1) == old)) ? 1 : 0;
                for (std::size_t k = pos + 2; k < 4 && c; ++k)
                {
                    old = r.limb(k);
                    r.set_limb(k, r.limb(k) + (c));
                    c = (r.limb(k) < old) ? 1 : 0;
                }
            }
        };

        // Use __uint128_t for 64×64→128 products (available on GCC/Clang/ICX)
        // On MSVC we rely on the N=2 path being tested independently.
        // This reference test is most useful on GCC/Clang where __uint128_t is available.
#ifdef __SIZEOF_INT128__
        auto mul64 = [](uint64_t x, uint64_t y, uint64_t *hi) -> uint64_t
        {
            unsigned __int128 p = (unsigned __int128)x * y;
            *hi = (uint64_t)(p >> 64);
            return (uint64_t)p;
        };
#else
        // On MSVC, use _umul128
        auto mul64 = [](uint64_t x, uint64_t y, uint64_t *hi) -> uint64_t { return _umul128(x, y, hi); };
#endif
        for (std::size_t i = 0; i < 4; ++i)
        {
            uint64_t ai = (i == 0 ? a0 : i == 1 ? a1 : i == 2 ? a2 : a3);
            for (std::size_t j = 0; i + j < 4; ++j)
            {
                uint64_t bj = (j == 0 ? b0 : j == 1 ? b1 : j == 2 ? b2 : b3);
                uint64_t hi = 0;
                uint64_t lo = mul64(ai, bj, &hi);
                add_to(i + j, lo, hi);
            }
        }
        return r;
    };

    auto check = [&](const char *name, u256 a, u256 b) { TEST(name, (a * b) == ref_mul(a, b)); };

    check("N=4 vs ref: {1,2,3,4}×{5,6,7,8}", make<4>({1, 2, 3, 4}), make<4>({5, 6, 7, 8}));
    check("N=4 vs ref: {MAX,0,0,0}×{MAX,0,0,0}", make<4>({MAX64, 0, 0, 0}), make<4>({MAX64, 0, 0, 0}));
    check("N=4 vs ref: {MAX,MAX,0,0}×{MAX,MAX,0,0}", make<4>({MAX64, MAX64, 0, 0}),
          make<4>({MAX64, MAX64, 0, 0}));
    check("N=4 vs ref: {MAX,MAX,MAX,MAX}×{2,0,0,0}", make<4>({MAX64, MAX64, MAX64, MAX64}),
          make<4>({2, 0, 0, 0}));
    check("N=4 vs ref: all-MAX × all-MAX", make<4>({MAX64, MAX64, MAX64, MAX64}),
          make<4>({MAX64, MAX64, MAX64, MAX64}));
    check("N=4 vs ref: mixed-limbs A", make<4>({0xDEADBEEFULL, 0xCAFEBABEULL, 0x01234567ULL, 0x89ABCDEFULL}),
          make<4>({0xFEDCBA98ULL, 0x76543210ULL, 0x11223344ULL, 0x55667788ULL}));
    check(
        "N=4 vs ref: mixed-limbs B",
        make<4>({0x1111111111111111ULL, 0x2222222222222222ULL, 0x3333333333333333ULL, 0x4444444444444444ULL}),
        make<4>(
            {0x5555555555555555ULL, 0x6666666666666666ULL, 0x7777777777777777ULL, 0x8888888888888888ULL}));
    check("N=4 vs ref: single high limb", make<4>({0, 0, 0, 0x123456789ABCDEFULL}),
          make<4>({0, 0, 0, 0xFEDCBA9876543210ULL}));
    check("N=4 vs ref: interleaved non-zero limbs",
          make<4>({0xAAAAAAAAAAAAAAAAULL, 0, 0xBBBBBBBBBBBBBBBBULL, 0}),
          make<4>({0, 0xCCCCCCCCCCCCCCCCULL, 0, 0xDDDDDDDDDDDDDDDDULL}));
    check("N=4 vs ref: (2^64+1)^2", make<4>({1, 1, 0, 0}), make<4>({1, 1, 0, 0}));
    check("N=4 vs ref: Fibonacci-like limbs", make<4>({1, 1, 2, 3}), make<4>({5, 8, 13, 21}));

    // operator*= consistency
    {
        u256 a = make<4>({0xDEADBEEFCAFEBABEULL, 0x123456789ABCDEFULL, 1, 0});
        u256 b = make<4>({0x999, 0x888, 0x777, 0x666});
        u256 r1 = a * b;
        u256 r2 = a;
        r2 *= b;
        TEST("N=4 *=  consistent with *", r1 == r2);
    }
}

// =============================================================================
// Section 4: Known-value products (N=8 / 512-bit)
// =============================================================================

static void test_known_n8(const char *tag)
{
    std::cout << "\n--- " << tag << " : Section 4 (N=8 known-value products) ---\n";

    using u512 = uint_fixed_t<8>;

    // 1×1 = 1
    {
        u512 one = make<8>({1});
        TEST("N=8 1*1 = 1", (one * one) == one);
    }
    // 0×big = 0
    {
        u512 zero{};
        u512 big = make<8>({MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64});
        TEST("N=8 0*big = 0", (zero * big) == zero);
    }
    // 2×3 = 6
    {
        u512 two = make<8>({2});
        u512 three = make<8>({3});
        TEST("N=8 2*3 = 6", (two * three) == make<8>({6}));
    }
    // (2^128)^2 = 2^256  →  bit 256 = limb 4 = 1
    {
        u512 b128 = make<8>({0, 0, 1, 0, 0, 0, 0, 0});
        u512 expected = make<8>({0, 0, 0, 0, 1, 0, 0, 0});
        TEST("N=8 2^128 * 2^128 = 2^256", (b128 * b128) == expected);
    }
    // max × 1 = max
    {
        u512 mx = make<8>({MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64});
        u512 one = make<8>({1});
        TEST("N=8 max*1 = max", (mx * one) == mx);
    }
    // max × max mod 2^512 = 1 (same reasoning as N=4)
    {
        u512 mx = make<8>({MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64});
        TEST("N=8 max*max mod 2^512 = 1", (mx * mx) == make<8>({1}));
    }
}

// =============================================================================
// Section 5: Commutativity and identities (N=8)
// =============================================================================

static void test_identities_n8(const char *tag)
{
    std::cout << "\n--- " << tag << " : Section 5 (N=8 identities) ---\n";

    using u512 = uint_fixed_t<8>;

    const u512 a = make<8>({0xDEAD, 0xBEEF, 0xCAFE, 0xBABE, 0x1234, 0x5678, 0x9ABC, 0xDEF0});
    const u512 b = make<8>({0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888});
    const u512 one = make<8>({1});
    const u512 zero{};

    TEST("N=8 commutativity a*b == b*a", (a * b) == (b * a));
    TEST("N=8 a*1 == a", (a * one) == a);
    TEST("N=8 1*a == a", (one * a) == a);
    TEST("N=8 a*0 == 0", (a * zero) == zero);
    TEST("N=8 a*a self-consistency", (a * a) == (a * a));

    // operator*= consistency
    {
        u512 x = a;
        x *= b;
        TEST("N=8 *= consistent with *", x == (a * b));
    }
}

// =============================================================================
// Section 6: Cross-check N=8 Karatsuba vs N=4 schoolbook reference
// =============================================================================

static void test_vs_n4_ref(const char *tag)
{
    std::cout << "\n--- " << tag << " : Section 6 (N=8 vs N=4 reference) ---\n";

    using u512 = uint_fixed_t<8>;
    using u256 = uint_fixed_t<4>;

    // Reference: schoolbook 8×8→8 using N=4 multiplications
    // For a = [a_hi|a_lo], b = [b_hi|b_lo] (each N=4 halves):
    //   result = a_lo*b_lo (8 limbs, low 8 of full product)
    //          + (a_lo*b_hi + a_hi*b_lo) << 256  mod 2^512
    auto ref_mul = [](u512 a, u512 b) -> u512
    {
        u256 a_lo = make<4>({a.limb(0), a.limb(1), a.limb(2), a.limb(3)});
        u256 a_hi = make<4>({a.limb(4), a.limb(5), a.limb(6), a.limb(7)});
        u256 b_lo = make<4>({b.limb(0), b.limb(1), b.limb(2), b.limb(3)});
        u256 b_hi = make<4>({b.limb(4), b.limb(5), b.limb(6), b.limb(7)});

        // Full product a_lo*b_lo (need 8 limbs: lower 4 from N=4 result, upper 4 from carry)
        // Since N=4 operator* gives truncated 4-limb result, we use schoolbook to get all 8 limbs
        // Schoolbook 4×4→8 reference using N=2 products:
        const uint64_t *al = a_lo.limbs().data();
        const uint64_t *bl = b_lo.limbs().data();

        u512 r{};
        auto add_to = [&r](std::size_t pos, uint64_t lo, uint64_t hi)
        {
            if (pos >= 8)
                return;
            uint64_t old = r.limb(pos);
            r.set_limb(pos, r.limb(pos) + (lo));
            uint64_t c = (r.limb(pos) < old) ? 1 : 0;
            if (pos + 1 < 8)
            {
                old = r.limb(pos + 1);
                r.set_limb(pos + 1, r.limb(pos + 1) + (hi + c));
                c = (r.limb(pos + 1) < old || (c && r.limb(pos + 1) == old)) ? 1 : 0;
                for (std::size_t k = pos + 2; k < 8 && c; ++k)
                {
                    old = r.limb(k);
                    r.set_limb(k, r.limb(k) + (c));
                    c = (r.limb(k) < old) ? 1 : 0;
                }
            }
        };

#ifdef __SIZEOF_INT128__
        auto mul64 = [](uint64_t x, uint64_t y, uint64_t *hi) -> uint64_t
        {
            unsigned __int128 p = (unsigned __int128)x * y;
            *hi = (uint64_t)(p >> 64);
            return (uint64_t)p;
        };
#else
        auto mul64 = [](uint64_t x, uint64_t y, uint64_t *hi) -> uint64_t { return _umul128(x, y, hi); };
#endif

        // Full schoolbook 8×8→8 (truncated)
        const uint64_t *av = a.limbs().data();
        const uint64_t *bv = b.limbs().data();
        for (std::size_t i = 0; i < 8; ++i)
            for (std::size_t j = 0; i + j < 8; ++j)
            {
                uint64_t hi = 0;
                uint64_t lo = mul64(av[i], bv[j], &hi);
                add_to(i + j, lo, hi);
            }
        return r;
    };

    auto check = [&](const char *name, u512 a, u512 b) { TEST(name, (a * b) == ref_mul(a, b)); };

    check("N=8 vs ref: {1,2,...,8}×{9,10,...,16}", make<8>({1, 2, 3, 4, 5, 6, 7, 8}),
          make<8>({9, 10, 11, 12, 13, 14, 15, 16}));
    check("N=8 vs ref: {MAX,0,...}×{MAX,0,...}", make<8>({MAX64, 0, 0, 0, 0, 0, 0, 0}),
          make<8>({MAX64, 0, 0, 0, 0, 0, 0, 0}));
    check("N=8 vs ref: lower 4 limbs only", make<8>({MAX64, MAX64, MAX64, MAX64, 0, 0, 0, 0}),
          make<8>({MAX64, MAX64, MAX64, MAX64, 0, 0, 0, 0}));
    check("N=8 vs ref: mixed all-limb A",
          make<8>({0xDEAD, 0xBEEF, 0xCAFE, 0xBABE, 0x1234, 0x5678, 0x9ABC, 0xDEF0}),
          make<8>({0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}));
    check("N=8 vs ref: mixed all-limb B",
          make<8>({0xAAAAAAAAAAAAAAAAULL, 0, 0xBBBBBBBBBBBBBBBBULL, 0, 0xCCCCCCCCCCCCCCCCULL, 0,
                   0xDDDDDDDDDDDDDDDDULL, 0}),
          make<8>({0, 0xEEEEEEEEEEEEEEEEULL, 0, 0xFFFFFFFFFFFFFFFFULL, 0, 0x1111111111111111ULL, 0,
                   0x2222222222222222ULL}));
    check("N=8 vs ref: all MAX", make<8>({MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64}),
          make<8>({MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64, MAX64}));
    check("N=8 vs ref: 2^256 × 3", make<8>({0, 0, 0, 0, 1, 0, 0, 0}), make<8>({3}));
    check("N=8 vs ref: commutativity via ref", make<8>({1, 2, 3, 4, 5, 6, 7, 8}),
          make<8>({8, 7, 6, 5, 4, 3, 2, 1}));
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=== test_fixed_karatsuba ===\n";

    test_known_n4("uint256");
    test_identities_n4("uint256");
    test_vs_n2_ref("uint256");
    test_known_n8("uint512");
    test_identities_n8("uint512");
    test_vs_n4_ref("uint512");

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return g_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
