// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Magnitude-Sign — 107 tests across 12 sections
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
//
// Sections 1-9:  info methods, casts, inc/dec, multiplication, division,
//                safe arithmetic, bitwise, shifts
// Sections 10-12 (added v1.77): addition/subtraction, unary minus + ±0
//                semantics, string I/O
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_safe.hpp"

#include <iostream>
#include <cstdint>
#include <limits>
#include <string>

using namespace nstd;
using ms_t = int128_ms_t;
using tc_t = int128_tc_t;
using ek_t = int128_ek_t;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                                                    \
    do {                                                                    \
        if (cond) { std::cout << "[OK] " << (name) << "\n"; ++g_passed; }  \
        else      { std::cout << "[FAIL] " << (name) << "\n"; ++g_failed; }\
    } while (false)

// =============================================================================
// Section 1: Info methods
// =============================================================================
static void test_info_methods()
{
    std::cout << "\n--- Section 1: Info methods ---\n";

    // Positive zero
    {
        const ms_t v{0};
        TEST("pos_zero is_zero",           v.is_zero());
        TEST("pos_zero is_positive_zero",  v.is_positive_zero());
        TEST("pos_zero !is_negative_zero", !v.is_negative_zero());
        TEST("pos_zero !is_negative",      !v.is_negative());
    }

    // Negative zero: set sign bit manually
    {
        ms_t v{0};
        v.set_high(v.high() | (1ULL << 63));
        TEST("neg_zero is_zero",           v.is_zero());
        TEST("neg_zero !is_positive_zero", !v.is_positive_zero());
        TEST("neg_zero is_negative_zero",  v.is_negative_zero());
        TEST("neg_zero is_negative",       v.is_negative());
    }

    // Positive and negative one
    {
        const ms_t pos{1}, neg{-1};
        TEST("pos_one !is_zero",     !pos.is_zero());
        TEST("pos_one !is_negative", !pos.is_negative());
        TEST("neg_one !is_zero",     !neg.is_zero());
        TEST("neg_one is_negative",  neg.is_negative());
    }

    // abs() and magnitude() — both return the absolute value as uint128_t
    {
        const ms_t n{-42}, p{42};
        TEST("abs(-42).low()==42",       n.abs().low()       == 42ULL);
        TEST("magnitude(-42).low()==42", n.magnitude().low() == 42ULL);
        TEST("abs(+42).low()==42",       p.abs().low()       == 42ULL);
        TEST("magnitude(+42).low()==42", p.magnitude().low() == 42ULL);
    }

    // abs/magnitude of int64_min has magnitude = 2^63
    {
        const int64_t min64 = std::numeric_limits<int64_t>::min();
        const ms_t v{min64};
        const uint64_t expected = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL;
        TEST("abs(int64_min).low()==2^63", v.abs().low() == expected);
    }

    // get_sign: +1 for positive, -1 for negative
    {
        const ms_t pos{42};
        const ms_t neg{-42};
        TEST("get_sign positive == +1", pos.get_sign() == 1);
        TEST("get_sign negative == -1", neg.get_sign() == -1);
        // Negation flips sign
        const ms_t neg_of_pos = -pos;
        TEST("negation flips sign", neg_of_pos.get_sign() == -1 && !pos.is_negative());
    }

    // abs via raw bit manipulation (sign bit set manually)
    {
        ms_t raw{0, 0};
        raw.set_high(1ULL << 63);
        raw.set_low(42);
        const auto a = raw.abs();
        TEST("abs raw sign+mag42 low==42",  a.low()  == 42);
        TEST("abs raw sign+mag42 high==0",  a.high() == 0);
    }
}

// =============================================================================
// Section 2: Cross-representation casts (round-trips)
// =============================================================================
static void test_casts()
{
    std::cout << "\n--- Section 2: Cross-representation casts ---\n";

    struct Case { const char* name; int64_t val; };
    const Case signed_cases[] = {
        {"+1",   1LL},
        {"-1",  -1LL},
        {"+42",  42LL},
        {"-42", -42LL},
        {"max",  std::numeric_limits<int64_t>::max()},
        {"min",  std::numeric_limits<int64_t>::min()},
    };

    for (const auto& c : signed_cases) {
        const ms_t ms{c.val};
        const tc_t tc = static_cast<tc_t>(ms);
        const ek_t ek = static_cast<ek_t>(ms);

        const bool ok_ms_tc = (static_cast<ms_t>(tc) == ms);
        const bool ok_ms_ek = (static_cast<ms_t>(ek) == ms);

        std::string n1 = std::string{"ms_tc_ms "} + c.name;
        std::string n2 = std::string{"ms_ek_ms "} + c.name;
        TEST(n1, ok_ms_tc);
        TEST(n2, ok_ms_ek);
    }

    // Unsigned (binnat) round-trips through all three signed representations
    const uint64_t unsigned_vals[] = {0ULL, 1ULL, 42ULL, 0xFFFFFFFFFFFFFFFFULL};
    for (const auto uval : unsigned_vals) {
        const uint128_t bn{uval};
        const ms_t ms_from_bn = static_cast<ms_t>(bn);
        const tc_t tc_from_bn = static_cast<tc_t>(bn);
        const ek_t ek_from_bn = static_cast<ek_t>(bn);

        const bool ok_ms = (static_cast<uint128_t>(ms_from_bn) == bn);
        const bool ok_tc = (static_cast<uint128_t>(tc_from_bn) == bn);
        const bool ok_ek = (static_cast<uint128_t>(ek_from_bn) == bn);

        std::string su = std::to_string(uval);
        TEST(std::string{"bn_ms_bn "} + su, ok_ms);
        TEST(std::string{"bn_tc_bn "} + su, ok_tc);
        TEST(std::string{"bn_ek_bn "} + su, ok_ek);
    }
}

// =============================================================================
// Section 3: Increment / Decrement
// =============================================================================
static void test_inc_dec()
{
    std::cout << "\n--- Section 3: Increment/Decrement ---\n";

    { ms_t v{1};  ++v; TEST("ms pre_inc +1->+2",  v == ms_t{2});  }
    { ms_t v{1};  v++; TEST("ms post_inc +1->+2", v == ms_t{2});  }
    { ms_t v{-1}; ++v; TEST("ms pre_inc -1->0",   v.is_zero());   }
    { ms_t v{-1}; --v; TEST("ms pre_dec -1->-2",  v == ms_t{-2}); }
    { ms_t v{0};  ++v; TEST("ms inc 0->+1",        v == ms_t{1});  }
    { ms_t v{0};  --v; TEST("ms dec 0->-1",        v == ms_t{-1}); }

    // Post-increment/decrement must return the old value
    { ms_t v{5}; const ms_t r = v++;
      bool ok = (r == ms_t{5}) && (v == ms_t{6});
      TEST("ms post_inc returns old value", ok); }
    { ms_t v{5}; const ms_t r = v--;
      bool ok = (r == ms_t{5}) && (v == ms_t{4});
      TEST("ms post_dec returns old value", ok); }
}

// =============================================================================
// Section 4: Multiplication — sign rules and zero
// =============================================================================
static void test_multiplication_signs()
{
    std::cout << "\n--- Section 4: Multiplication — sign rules ---\n";

    // Sign rules via *=
    { ms_t a{3},   b{4};   a *= b; TEST("ms *=: 3*4=12 pos",        a == ms_t{12}  && !a.is_negative()); }
    { ms_t a{3},   b{-4};  a *= b; TEST("ms *=: 3*(-4)=-12",        a == ms_t{-12} &&  a.is_negative()); }
    { ms_t a{-3},  b{4};   a *= b; TEST("ms *=: (-3)*4=-12",        a == ms_t{-12} &&  a.is_negative()); }
    { ms_t a{-3},  b{-4};  a *= b; TEST("ms *=: (-3)*(-4)=12",      a == ms_t{12}  && !a.is_negative()); }

    // Zero products via *=
    { ms_t a{999}, b{0};   a *= b; TEST("ms *=: x*0=0 !neg",         a.is_zero() && !a.is_negative()); }
    { ms_t a{0},   b{999}; a *= b; TEST("ms *=: 0*x=0 !neg",         a.is_zero() && !a.is_negative()); }

    // Zero * negative must give +0, not -0
    { const ms_t a{0}, b{-100};  const auto r = a * b;
      TEST("ms *: 0*(-100)=+0",  r.is_zero() && !r.is_negative()); }
    { const ms_t a{-100}, b{0};  const auto r = a * b;
      TEST("ms *: (-100)*0=+0",  r.is_zero() && !r.is_negative()); }

    // operator* (non-mutating)
    { const ms_t a{50},  b{100};  const auto r = a * b; TEST("ms *: 50*100=5000",      r == ms_t{5000});   }
    { const ms_t a{-50}, b{100};  const auto r = a * b; TEST("ms *: -50*100=-5000",    r == ms_t{-5000});  }
    { const ms_t a{-50}, b{-100}; const auto r = a * b; TEST("ms *: -50*-100=5000",    r == ms_t{5000});   }
    { const ms_t a{-1},  b{1000}; const auto r = a * b; TEST("ms *: (-1)*1000=-1000",  r == ms_t{-1000});  }

    // Multiplicative identity
    { const ms_t a{-999999999LL}, one{1};
      TEST("ms *: identity -999999999*1==a", a * one == a); }
}

// =============================================================================
// Section 5: Multiplication — large values and carry
// =============================================================================
static void test_multiplication_large()
{
    std::cout << "\n--- Section 5: Multiplication — large values ---\n";

    // (2^32-1)^2 = 2^64 - 2^33 + 1 = 0xFFFFFFFE00000001
    { const ms_t a{0xFFFFFFFFULL}, b{0xFFFFFFFFULL};
      const auto r = a * b;
      TEST("ms (2^32-1)^2 low",  r.low()  == 0xFFFFFFFE00000001ULL);
      TEST("ms (2^32-1)^2 high", r.high() == 0ULL); }

    // (2^64-1) * 2 = 2^65 - 2
    { const ms_t a{0xFFFFFFFFFFFFFFFFULL}, b{2};
      const auto r = a * b;
      TEST("ms (2^64-1)*2 low",  r.low()  == 0xFFFFFFFFFFFFFFFEULL);
      TEST("ms (2^64-1)*2 high", r.high() == 1ULL); }

    // 10^9 * (-10^9) = -10^18
    { const ms_t a{1000000000LL}, b{-1000000000LL};
      const auto r = a * b;
      TEST("ms 10^9*(-10^9)=-10^18", r == ms_t{-1000000000000000000LL}); }

    // Magnitude overflow: 2^126 * 2 => 2^127 overflows 127-bit magnitude field, wraps to 0
    { const ms_t a{0x4000000000000000ULL, 0ULL}, b{2};
      const auto r = a * b;
      TEST("ms magnitude_overflow wraps to 0", r.is_zero() && !r.is_negative()); }
}

// =============================================================================
// Section 6: Division
// =============================================================================
static void test_division()
{
    std::cout << "\n--- Section 6: Division (truncate toward zero) ---\n";

    // divmod with all four sign combinations
    { const ms_t a{100}, b{7};
      const auto [q, r] = a.divmod(b);
      bool ok = (q == ms_t{14}) && (r == ms_t{2});
      TEST("ms divmod  100/ 7: q=14 r=2",   ok); }

    { const ms_t a{-100}, b{7};
      const auto [q, r] = a.divmod(b);
      bool ok = (q == ms_t{-14}) && (r == ms_t{-2});
      TEST("ms divmod -100/ 7: q=-14 r=-2", ok); }

    { const ms_t a{100}, b{-7};
      const auto [q, r] = a.divmod(b);
      bool ok = (q == ms_t{-14}) && (r == ms_t{2});
      TEST("ms divmod  100/-7: q=-14 r=2",  ok); }

    { const ms_t a{-100}, b{-7};
      const auto [q, r] = a.divmod(b);
      bool ok = (q == ms_t{14}) && (r == ms_t{-2});
      TEST("ms divmod -100/-7: q=14 r=-2",  ok); }

    // /= operator
    { ms_t a{100}; a /= ms_t{7}; TEST("ms /=: 100/7=14", a == ms_t{14}); }

    // %= operator
    { ms_t a{100}; a %= ms_t{7}; TEST("ms %=: 100%7=2",  a == ms_t{2});  }

    // Non-mutating / and %
    { const ms_t a{-100}, b{7};
      TEST("ms /: -100/7=-14",  a / b == ms_t{-14}); }
    { const ms_t a{-100}, b{7};
      TEST("ms %: -100%7=-2",   a % b == ms_t{-2});  }
}

// =============================================================================
// Section 7: Safe arithmetic
// =============================================================================
static void test_safe_arithmetic()
{
    std::cout << "\n--- Section 7: Safe arithmetic ---\n";

    {
        const ms_t a{-50}, b{100};
        const auto r = nstd::checked_mul(a, b);
        bool ok = !r.overflow && r.value == ms_t{-5000};
        TEST("ms checked_mul(-50,100) no overflow value=-5000", ok);
    }

    // divmod via method cross-check (from test_ms_ek_operators Group 6)
    {
        const ms_t a{-100}, b{7};
        const auto [q, r] = a.divmod(b);
        bool ok = (q == ms_t{-14}) && (r == ms_t{-2});
        TEST("ms divmod_via_method -100/7: q=-14 r=-2", ok);
    }
}

// =============================================================================
// Section 8: MS-specific bitwise operators
// =============================================================================
static void test_ms_bitwise()
{
    std::cout << "\n--- Section 8: MS Bitwise ---\n";

    // AND of two positives: result is non-negative
    { const ms_t a{15}, b{7}; TEST("ms & two positives non-negative", !(a & b).is_negative()); }
    // OR of two positives: result is non-negative
    { const ms_t a{8}, b{4};  TEST("ms | two positives non-negative", !(a | b).is_negative()); }
    // NOT changes value
    { const ms_t x{42}; TEST("ms not changes value", ~x != x); }
    // DeMorgan on MS
    { const ms_t a{0x0F}, b{0xF0};
      TEST("ms demorgan ~(a&b)==~a|~b", ~(a & b) == (~a | ~b)); }
    // XOR self = zero
    { const ms_t a{0x55};
      TEST("ms xor self all bits zero", (a ^ a) == ms_t{0}); }
}

// =============================================================================
// Section 9: MS-specific shift operators
// =============================================================================
static void test_ms_shifts()
{
    std::cout << "\n--- Section 9: MS Shifts ---\n";

    // Left shift positive: sign preserved
    { const ms_t x{5};   TEST("ms shl pos sign preserved", !(x << 1).is_negative()); }
    // Left shift negative: sign preserved
    { const ms_t x{-5};  TEST("ms shl neg sign preserved",  (x << 1).is_negative()); }
    // Right shift positive: sign preserved
    { const ms_t x{20};  TEST("ms shr pos sign preserved", !(x >> 1).is_negative()); }
    // Right shift negative: sign preserved
    { const ms_t x{-20}; TEST("ms shr neg sign preserved",  (x >> 1).is_negative()); }
    // Shift by 0 is identity
    { const ms_t x{42};
      TEST("ms shl 0 identity", (x << 0) == x);
      TEST("ms shr 0 identity", (x >> 0) == x); }
    // Shift zero value is still zero
    { const ms_t z{0};
      TEST("ms shl zero stays zero", (z << 4) == ms_t{0});
      TEST("ms shr zero stays zero", (z >> 4) == ms_t{0}); }
}

// =============================================================================
// Section 10: Addition and subtraction
// =============================================================================
static void test_add_sub()
{
    std::cout << "\n--- Section 10: Addition and subtraction ---\n";

    // All four sign combinations
    { const ms_t a{3},   b{4};   TEST("ms +: 3+4=7",          a + b == ms_t{7});   }
    { const ms_t a{-3},  b{-4};  TEST("ms +: -3+(-4)=-7",     a + b == ms_t{-7});  }
    { const ms_t a{10},  b{-4};  TEST("ms +: 10+(-4)=6",      a + b == ms_t{6});   }
    { const ms_t a{-10}, b{4};   TEST("ms +: -10+4=-6",       a + b == ms_t{-6});  }

    // Identity and self-cancellation
    { const ms_t a{42};
      TEST("ms add zero identity", a + ms_t{0LL} == a);
      TEST("ms sub zero identity", a - ms_t{0LL} == a);
      TEST("ms sub self is zero",  (a - a).is_zero()); }

    // Commutativity and round-trip
    { const ms_t a{12345LL}, b{-6789LL};
      TEST("ms add commutativity",  a + b == b + a);
      TEST("ms add/sub round-trip", (a + b) - b == a); }

    // Cross-limb carry: UINT64_MAX + 1 must propagate into high word
    { const ms_t a{0xFFFFFFFFFFFFFFFFULL}, one{1LL};
      const ms_t r = a + one;
      TEST("ms carry low->high: low==0",  r.low()  == 0ULL);
      TEST("ms carry low->high: high==1", r.high() == 1ULL); }

    // Subtraction producing negative
    { const ms_t a{3LL}, b{10LL};
      TEST("ms sub: 3-10=-7", a - b == ms_t{-7LL}); }
}

// =============================================================================
// Section 11: Unary minus and ±0 semantics
// =============================================================================
static void test_unary_minus_and_zeros()
{
    std::cout << "\n--- Section 11: Unary minus and ±0 semantics ---\n";

    // Basic unary minus behaviour
    TEST("ms -ms(0) is zero",        (-ms_t{0LL}).is_zero());
    TEST("ms -ms(42) is negative",   (-ms_t{42LL}).is_negative());
    TEST("ms -ms(-42) not negative", !(-ms_t{-42LL}).is_negative());
    TEST("ms double negation",       -(-ms_t{42LL}) == ms_t{42LL});

    // a + (-a) == 0 for positive and negative
    { const ms_t a{12345LL};
      TEST("ms a+(-a)==0 positive", (a + (-a)).is_zero()); }
    { const ms_t a{-12345LL};
      TEST("ms a+(-a)==0 negative", (a + (-a)).is_zero()); }

    // ±0: distinct bit patterns but must compare equal, and convert to TC(0)
    { ms_t neg_zero{0LL};
      neg_zero.set_high(neg_zero.high() | (1ULL << 63));   // set sign bit manually
      TEST("ms +0 == -0",         ms_t{0LL} == neg_zero);
      TEST("ms -0 to TC gives 0", static_cast<tc_t>(neg_zero) == tc_t{0LL}); }
}

// =============================================================================
// Section 12: String I/O
// =============================================================================
static void test_string_io()
{
    std::cout << "\n--- Section 12: String I/O ---\n";

    // to_string
    TEST("ms to_string  42",   ms_t{42LL}.to_string()   == "42");
    TEST("ms to_string -42",   ms_t{-42LL}.to_string()  == "-42");
    TEST("ms to_string  0",    ms_t{0LL}.to_string()    == "0");

    // from_string
    TEST("ms from_string  42", ms_t::from_string("42")  == ms_t{42LL});
    TEST("ms from_string -42", ms_t::from_string("-42") == ms_t{-42LL});
    TEST("ms from_string  0",  ms_t::from_string("0")   == ms_t{0LL});

    // Round-trips
    { const ms_t orig{999999999LL};
      TEST("ms round-trip positive", ms_t::from_string(orig.to_string().c_str()) == orig); }
    { const ms_t orig{-999999999LL};
      TEST("ms round-trip negative", ms_t::from_string(orig.to_string().c_str()) == orig); }
    { const ms_t orig{std::numeric_limits<int64_t>::max()};
      TEST("ms round-trip int64_max", ms_t::from_string(orig.to_string().c_str()) == orig); }
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "================================================================\n";
    std::cout << "  test_param_ms: Magnitude-Sign (12 sections, 107 tests)\n";
    std::cout << "================================================================\n";

    test_info_methods();
    test_casts();
    test_inc_dec();
    test_multiplication_signs();
    test_multiplication_large();
    test_division();
    test_safe_arithmetic();
    test_ms_bitwise();
    test_ms_shifts();
    test_add_sub();
    test_unary_minus_and_zeros();
    test_string_io();

    std::cout << "\n================================================================\n";
    std::cout << "  RESULTS: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "================================================================\n";

    return g_failed ? 1 : 0;
}
