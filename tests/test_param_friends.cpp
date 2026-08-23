// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Friend Operators, abs, swap — mixed-type arithmetic and helpers
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
//
// Consolidates: test_priority9_friends
// =============================================================================

#include "int128_parameterized.hpp"

#include <iostream>
#include <utility>

using namespace nstd;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                              \
    do                                                \
    {                                                 \
        if (cond)                                     \
        {                                             \
            std::cout << "[OK] " << (name) << "\n";   \
            ++g_passed;                               \
        }                                             \
        else                                          \
        {                                             \
            std::cout << "[FAIL] " << (name) << "\n"; \
            ++g_failed;                               \
        }                                             \
    } while (false)

// =============================================================================
// Section 1: abs — TC positive and negative
// =============================================================================
static void test_abs()
{
    std::cout << "\n--- Section 1: abs ---\n";

    // TC positive
    {
        const int128_tc_t x{0, 42};
        const auto r = x.abs();
        TEST("abs tc +42 low==42", r.low() == 42);
        TEST("abs tc +42 high==0", r.high() == 0);
    }

    // TC negative
    {
        const int128_tc_t x{-1};
        const auto r = x.abs();
        TEST("abs tc -1 low==1", r.low() == 1);
        TEST("abs tc -1 high==0", r.high() == 0);
    }

    // MS negative via raw bit manipulation
    {
        int128_ms_t x{0, 0};
        x.set_high(1ULL << 63);
        x.set_low(42);
        const auto r = x.abs();
        TEST("abs ms raw sign+mag42 low==42", r.low() == 42);
        TEST("abs ms raw sign+mag42 high==0", r.high() == 0);
    }
}

// =============================================================================
// Section 2: swap and ADL swap
// =============================================================================
static void test_swap()
{
    std::cout << "\n--- Section 2: swap / ADL swap ---\n";

    // Member swap
    {
        uint128_t a{0x1234, 0x5678};
        uint128_t b{0xABCD, 0xEF01};
        a.swap(b);
        bool ok = (a.high() == 0xABCD) && (a.low() == 0xEF01) && (b.high() == 0x1234) && (b.low() == 0x5678);
        TEST("member swap", ok);
    }

    // ADL swap via std::swap
    {
        uint128_t a{0x1234, 0x5678};
        uint128_t b{0xABCD, 0xEF01};
        using std::swap;
        swap(a, b);
        bool ok = (a.high() == 0xABCD) && (a.low() == 0xEF01) && (b.high() == 0x1234) && (b.low() == 0x5678);
        TEST("ADL swap", ok);
    }
}

// =============================================================================
// Section 3: Friend subtraction (scalar op)
// =============================================================================
static void test_friend_sub()
{
    std::cout << "\n--- Section 3: friend sub ---\n";

    {
        const uint128_t x{0, 100};
        TEST("x - 30 == 70", (x - 30).low() == 70);
    }

    {
        const uint128_t x{0, 30};
        TEST("100 - x == 70", (100 - x).low() == 70);
    }

    // Carry: 0x100 subtracted from a boundary value
    {
        const uint128_t x{1, 0}; // 2^64
        TEST("2^64 - 1 == UINT64_MAX", (x - 1).low() == ~0ULL && (x - 1).high() == 0);
    }
}

// =============================================================================
// Section 4: Friend multiplication (scalar op)
// =============================================================================
static void test_friend_mul()
{
    std::cout << "\n--- Section 4: friend mul ---\n";

    {
        const uint128_t x{0, 10};
        TEST("x * 5 == 50", (x * 5).low() == 50);
    }

    {
        const uint128_t x{0, 10};
        TEST("5 * x == 50", (5 * x).low() == 50);
    }

    // Carry into high limb
    {
        const uint128_t x{0, 0xFFFFFFFFFFFFFFFFULL};
        const auto r = x * 2;
        TEST("(2^64-1)*2 low == FFFE", r.low() == 0xFFFFFFFFFFFFFFFEULL);
        TEST("(2^64-1)*2 high == 1", r.high() == 1);
    }
}

// =============================================================================
// Section 5: Friend comparisons (int128 op scalar, scalar op int128)
// =============================================================================
static void test_friend_comparison()
{
    std::cout << "\n--- Section 5: friend comparisons ---\n";

    const uint128_t x{0, 42};

    // Equality both directions
    TEST("x == 42", x == 42);
    TEST("42 == x", 42 == x);

    // Less than both directions
    TEST("x < 100", x < 100);
    TEST("10 < x", 10 < x);
    TEST("!(x < 10)", !(x < 10));
    TEST("!(100 < x)", !(100 < x));

    // Greater than both directions
    TEST("x > 10", x > 10);
    TEST("100 > x", 100 > x);
    TEST("!(x > 100)", !(x > 100));
    TEST("!(10 > x)", !(10 > x));

    // MS comparison with scalar
    {
        const int128_ms_t ms{0, 42};
        TEST("ms == 42", ms == 42);
        TEST("42 == ms", 42 == ms);
        TEST("ms > 10", ms > 10);
        TEST("!(ms < 10)", !(ms < 10));
    }
}

// =============================================================================
// Section 6: Friend bitwise (int128 op scalar, scalar op int128)
// =============================================================================
static void test_friend_bitwise()
{
    std::cout << "\n--- Section 6: friend bitwise ---\n";

    {
        const uint128_t x{0, 0xFF};
        TEST("x & 0x0F == 0x0F", (x & 0x0F).low() == 0x0F);
    }

    {
        const uint128_t x{0, 0xF0};
        TEST("0x0F | x == 0xFF", (0x0F | x).low() == 0xFF);
    }

    {
        const uint128_t x{0, 0xFF};
        TEST("x ^ 0x0F == 0xF0", (x ^ 0x0F).low() == 0xF0);
    }

    // With carry into high limb: OR adds high bits
    {
        const uint128_t x{0, 0xFF};
        const auto r = 0x0F00ULL | x;
        TEST("0x0F00 | x low == 0x0FFF", r.low() == 0x0FFF);
    }
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "================================================================\n";
    std::cout << "  test_param_friends: abs, swap, friend ops (sub/mul/cmp/bitwise)\n";
    std::cout << "================================================================\n";

    test_abs();
    test_swap();
    test_friend_sub();
    test_friend_mul();
    test_friend_comparison();
    test_friend_bitwise();

    std::cout << "\n================================================================\n";
    std::cout << "  RESULTS: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "================================================================\n";

    return g_failed ? 1 : 0;
}
