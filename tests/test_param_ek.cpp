// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Excess-K — type traits, representation, arithmetic, comparisons
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
//
// Consolidates: test_ek_constructor_minimal, test_ek_operator_semantics,
//               test_excess_k_basic, test_excess_k_arithmetic,
//               test_excess_k_comparison,
//               test_ms_ek_operators (EK portions of Groups 3-6)
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_safe.hpp"

#include <iostream>
#include <cstdint>

using namespace nstd;
using ek_t = int128_ek_t;
using ms_t = int128_ms_t;

// =============================================================================
// Compile-time interface checks
// =============================================================================

// EK: +, -, +=, -= are available
static_assert(requires(ek_t a, ek_t b) { a +  b; }, "EK operator+  must be available");
static_assert(requires(ek_t a, ek_t b) { a += b; }, "EK operator+= must be available");
static_assert(requires(ek_t a, ek_t b) { a -  b; }, "EK operator-  must be available");
static_assert(requires(ek_t a, ek_t b) { a -= b; }, "EK operator-= must be available");

// MS: *, *=, /, % are available (= delete applies to EK only, not MS)
static_assert(requires(ms_t a, ms_t b) { a *  b; }, "MS operator*  must be available");
static_assert(requires(ms_t a, ms_t b) { a *= b; }, "MS operator*= must be available");
static_assert(requires(ms_t a, ms_t b) { a /  b; }, "MS operator/  must be available");
static_assert(requires(ms_t a, ms_t b) { a %  b; }, "MS operator%  must be available");

// NOTE: EK *, *=, /, /=, %, %=, divmod are = delete.
// Negative compile tests live in tests/negative/ (future work).

// =============================================================================
// Test framework
// =============================================================================

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                                                    \
    do {                                                                    \
        if (cond) { std::cout << "[OK] " << (name) << "\n"; ++g_passed; }  \
        else      { std::cout << "[FAIL] " << (name) << "\n"; ++g_failed; }\
    } while (false)

static ek_t ek(long long v) { return ek_t(v); }

// =============================================================================
// Section 1: Type traits
// =============================================================================
static void test_type_traits()
{
    std::cout << "\n--- Section 1: Type traits ---\n";

    TEST("ek is_signed",   ek_t::is_signed);
    TEST("ek is_excess_k", ek_t::is_excess_k);

    // Constructor stores real_value + bias; bias_high = 2^62
    constexpr uint64_t K_high = 1ULL << 62;
    const ek_t v{100};
    TEST("ek{100} high == bias", v.high() == K_high);
    TEST("ek{100} low  == 100",  v.low()  == 100ULL);
}

// =============================================================================
// Section 2: Representation (raw bias-aware constructors)
// =============================================================================
static void test_representation()
{
    std::cout << "\n--- Section 2: Representation ---\n";

    constexpr uint64_t K = 1ULL << 62;

    // Zero: stored == bias = (K, 0)
    const ek_t zero{K, 0ULL};
    TEST("zero is_zero",            zero.is_zero());
    TEST("zero !is_negative",       !zero.is_negative());

    // Positive +1: stored = (K, 1)
    const ek_t pos_one{K, 1ULL};
    TEST("pos_one !is_zero",        !pos_one.is_zero());
    TEST("pos_one !is_negative",    !pos_one.is_negative());

    // Negative -1: stored = bias - 1 = (K-1, 0xFFFF...FFFF)
    const ek_t neg_one{K - 1ULL, 0xFFFFFFFFFFFFFFFFULL};
    TEST("neg_one !is_zero",        !neg_one.is_zero());
    TEST("neg_one is_negative",     neg_one.is_negative());

    // Negation of a positive value gives a negative value
    const ek_t neg_of_pos = -pos_one;
    TEST("neg_pos_one is_negative", neg_of_pos.is_negative());
}

// =============================================================================
// Section 3: Addition
// =============================================================================
static void test_addition()
{
    std::cout << "\n--- Section 3: Addition ---\n";

    { ek_t r = ek(1);    r += ek(1);    TEST("add 1+1=2",            r == ek(2));    }
    { ek_t r = ek(1);    r += ek(-1);   TEST("add 1+(-1)=0",         r == ek(0));    }
    { ek_t r = ek(1000); r += ek(2000); TEST("add 1000+2000=3000",   r == ek(3000)); }
    { ek_t r = ek(-10);  r += ek(-20);  TEST("add (-10)+(-20)=-30",  r == ek(-30));  }

    { const ek_t a = ek(42),  b = ek(17);  TEST("add commutative a+b==b+a", a+b == b+a);    }
    { const ek_t a = ek(100), b = ek(200); TEST("add operator+: 100+200=300", a+b == ek(300)); }
    { const ek_t a = ek(100), z = ek(0);   TEST("add identity: a+0==a",        a+z == a);       }
}

// =============================================================================
// Section 4: Subtraction
// =============================================================================
static void test_subtraction()
{
    std::cout << "\n--- Section 4: Subtraction ---\n";

    { ek_t r = ek(5);   r -= ek(3);   TEST("sub 5-3=2",           r == ek(2));   }
    { ek_t r = ek(999); r -= ek(999); TEST("sub a-a=0",           r == ek(0));   }
    { ek_t r = ek(3);   r -= ek(5);   TEST("sub 3-5=-2",          r == ek(-2));  }
    { ek_t r = ek(-5);  r -= ek(-3);  TEST("sub (-5)-(-3)=-2",    r == ek(-2));  }

    { const ek_t a = ek(100), b = ek(200); TEST("sub operator-: 100-200=-100", a-b == ek(-100)); }
    { const ek_t a = ek(100), z = ek(0);   TEST("sub identity: a-0==a",         a-z == a);        }
}

// =============================================================================
// Section 5: Round-trip invariants
// =============================================================================
static void test_roundtrip()
{
    std::cout << "\n--- Section 5: Round-trip invariants ---\n";

    { const ek_t a = ek(12345), b = ek(6789); TEST("(a+b)-b==a positive",   (a+b)-b == a); }
    { const ek_t a = ek(10000), b = ek(3333); TEST("(a-b)+b==a positive",   (a-b)+b == a); }
    { const ek_t a = ek(-42),   b = ek(100);  TEST("(a+b)-b==a negative a", (a+b)-b == a); }
}

// =============================================================================
// Section 6: Increment / Decrement
// =============================================================================
static void test_inc_dec()
{
    std::cout << "\n--- Section 6: Increment/Decrement ---\n";

    { ek_t v{100};  ++v; TEST("pre_inc 100->101",     v == ek(101)); }
    { ek_t v{100};  v++; TEST("post_inc 100->101",    v == ek(101)); }
    { ek_t v{100};  --v; TEST("pre_dec 100->99",      v == ek(99));  }
    { ek_t v{100};  v--; TEST("post_dec 100->99",     v == ek(99));  }
    { ek_t v{-100}; ++v; TEST("inc neg -100->-99",    v == ek(-99)); }
    { ek_t v{-100}; --v; TEST("dec neg -100->-101",   v == ek(-101));}
    { ek_t v{0};    ++v; TEST("inc zero 0->1",         v == ek(1));   }
    { ek_t v{0};    --v; TEST("dec zero 0->-1",        v == ek(-1));  }

    { ek_t v{50}; const ek_t r = v++;
      bool ok = (r == ek(50)) && (v == ek(51));
      TEST("post_inc returns old value", ok); }
    { ek_t v{50}; const ek_t r = v--;
      bool ok = (r == ek(50)) && (v == ek(49));
      TEST("post_dec returns old value", ok); }
}

// =============================================================================
// Section 7: Comparison operators
// =============================================================================
static void test_comparisons()
{
    std::cout << "\n--- Section 7: Comparison operators ---\n";

    constexpr uint64_t K = 1ULL << 62;
    const ek_t zero    {K,       0ULL};
    const ek_t pos_one {K,       1ULL};
    const ek_t pos_ten {K,       10ULL};
    const ek_t neg_one {K - 1ULL, 0xFFFFFFFFFFFFFFFFULL};
    const ek_t neg_ten {K - 1ULL, 0xFFFFFFFFFFFFFFF6ULL};

    // Equality
    const ek_t another_zero{K, 0ULL};
    TEST("eq: zero == zero",           zero == another_zero);
    TEST("eq: pos_one == pos_one",     pos_one == pos_one);
    TEST("eq: !(pos_one == pos_ten)",  !(pos_one == pos_ten));

    // Inequality
    TEST("ne: zero != pos_one",        zero != pos_one);
    TEST("ne: neg_one != zero",        neg_one != zero);
    TEST("ne: !(zero != zero)",        !(zero != another_zero));

    // Less than
    TEST("lt: neg_one < zero",         neg_one < zero);
    TEST("lt: zero < pos_one",         zero < pos_one);
    TEST("lt: pos_one < pos_ten",      pos_one < pos_ten);
    TEST("lt: neg_ten < neg_one",      neg_ten < neg_one);
    TEST("lt: !(pos_one < pos_one)",   !(pos_one < pos_one));

    // Less than or equal
    TEST("le: neg_one <= zero",        neg_one <= zero);
    TEST("le: pos_one <= pos_one",     pos_one <= pos_one);
    TEST("le: !(pos_ten <= pos_one)",  !(pos_ten <= pos_one));

    // Greater than
    TEST("gt: pos_one > zero",         pos_one > zero);
    TEST("gt: zero > neg_one",         zero > neg_one);
    TEST("gt: neg_one > neg_ten",      neg_one > neg_ten);
    TEST("gt: !(pos_one > pos_one)",   !(pos_one > pos_one));

    // Greater than or equal
    TEST("ge: pos_one >= zero",        pos_one >= zero);
    TEST("ge: pos_one >= pos_one",     pos_one >= pos_one);
    TEST("ge: !(pos_one >= pos_ten)",  !(pos_one >= pos_ten));

    // Mathematical properties
    bool trans_neg = neg_ten < neg_one && neg_one < zero && neg_ten < zero;
    TEST("transitivity neg: neg_ten<neg_one<zero", trans_neg);
    bool trans_pos = zero < pos_one && pos_one < pos_ten && zero < pos_ten;
    TEST("transitivity pos: zero<pos_one<pos_ten", trans_pos);
    TEST("reflexivity: zero == zero",       zero == zero);
    TEST("reflexivity: pos_one == pos_one", pos_one == pos_one);

    { const ek_t a{K, 42ULL}, b{K, 42ULL};
      bool antisym = (a <= b) && (b <= a) && (a == b);
      TEST("antisymmetry: a<=b && b<=a => a==b", antisym); }

    TEST("total order holds for pos_one vs pos_ten",
         (pos_one < pos_ten) || (pos_one == pos_ten) || (pos_one > pos_ten));

    // Via integer constructor
    TEST("cmp via ctor: ek(-100) < ek(200)",  ek(-100) < ek(200));
    TEST("cmp via ctor: ek(200) > ek(-100)",  ek(200) > ek(-100));
    TEST("cmp via ctor: ek(100) == ek(100)",  ek(100) == ek(100));
    TEST("cmp via ctor: ek(-50) != ek(50)",   ek(-50) != ek(50));
}

// =============================================================================
// Section 8: Safe arithmetic
// =============================================================================
static void test_safe_arithmetic()
{
    std::cout << "\n--- Section 8: Safe arithmetic ---\n";

    {
        const ek_t a{100}, b{200};
        const auto r = nstd::checked_add(a, b);
        bool ok = !r.overflow && r.value == ek(300);
        TEST("checked_add(100,200) no overflow value=300", ok);
    }
    {
        const ek_t a{100}, b{50};
        const auto r = nstd::checked_sub(a, b);
        bool ok = !r.overflow && r.value == ek(50);
        TEST("checked_sub(100,50) no overflow value=50", ok);
    }
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "================================================================\n";
    std::cout << "  test_param_ek: Excess-K type traits, arithmetic, comparisons\n";
    std::cout << "================================================================\n";
    std::cout << "[compile-time] 8 static_asserts: EK +/-/+=/- =, MS */*=//%  OK\n";

    test_type_traits();
    test_representation();
    test_addition();
    test_subtraction();
    test_roundtrip();
    test_inc_dec();
    test_comparisons();
    test_safe_arithmetic();

    std::cout << "\n================================================================\n";
    std::cout << "  RESULTS: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "================================================================\n";

    return g_failed ? 1 : 0;
}
