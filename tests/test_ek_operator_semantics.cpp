#include "int128_param_iostreams.hpp"
#include <iostream>
#include <cassert>

// =============================================================================
// Compile-time checks
//
// NOTE: Calling a `= delete`d function is always ill-formed in C++, even inside
// a requires-expression.  GCC issues a hard error rather than evaluating the
// requires-expression as false.  The negative tests (verifying that EK *,/,%
// do NOT compile) must therefore live in separate translation units that are
// expected to fail compilation.  See tests/negative/ (future).
//
// What we CAN test here (positive checks):
//   - EK  operator+ / operator-  ARE callable (not deleted)
//   - MS  operator* / operator/  ARE callable (not deleted)
// =============================================================================

using ek_t = nstd::int128_ek_t;
using ms_t = nstd::int128_ms_t;

// EK: + and - must be available
static_assert(requires(ek_t a, ek_t b) { a +  b; }, "EK operator+  must be available");
static_assert(requires(ek_t a, ek_t b) { a += b; }, "EK operator+= must be available");
static_assert(requires(ek_t a, ek_t b) { a -  b; }, "EK operator-  must be available");
static_assert(requires(ek_t a, ek_t b) { a -= b; }, "EK operator-= must be available");

// MS: *, /, % must be available (= delete only applies to EK)
static_assert(requires(ms_t a, ms_t b) { a *  b; }, "MS operator*  must be available");
static_assert(requires(ms_t a, ms_t b) { a *= b; }, "MS operator*= must be available");
static_assert(requires(ms_t a, ms_t b) { a /  b; }, "MS operator/  must be available");
static_assert(requires(ms_t a, ms_t b) { a %  b; }, "MS operator%  must be available");

// Macro definitions
#define TEST_CASE(name, func)                                                  \
    do                                                                         \
    {                                                                          \
        test_count++;                                                          \
        try                                                                    \
        {                                                                      \
            func();                                                            \
            pass_count++;                                                      \
            std::cout << "  PASS: " << name << std::endl;                      \
        }                                                                      \
        catch (const std::exception &e)                                        \
        {                                                                      \
            std::cout << "  FAIL: " << name << " - " << e.what() << std::endl; \
        }                                                                      \
    } while (false)
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_TRUE(a)  assert((a) == true)
#define ASSERT_FALSE(a) assert((a) == false)

int test_count = 0, pass_count = 0;

// Shorthand: EK value from int64_t
static ek_t ek(long long v) { return ek_t(v); }

// =============================================================================
// Group 1: Addition — semantic invariants
// =============================================================================

// 1 + 1 = 2
void test_add_basic()
{
    ek_t result = ek(1);
    result += ek(1);
    ASSERT_EQ(result, ek(2));
}

// 1 + (−1) = 0
void test_add_cancel()
{
    ek_t result = ek(1);
    result += ek(-1);
    ASSERT_EQ(result, ek(0));
}

// a + b == b + a  (commutativity)
void test_add_commutative()
{
    const ek_t a = ek(42);
    const ek_t b = ek(17);
    ASSERT_EQ(a + b, b + a);
}

// Large positive values: 1000 + 2000 = 3000
void test_add_large()
{
    ek_t result = ek(1000);
    result += ek(2000);
    ASSERT_EQ(result, ek(3000));
}

// Negative values: (−10) + (−20) = −30
void test_add_both_negative()
{
    ek_t result = ek(-10);
    result += ek(-20);
    ASSERT_EQ(result, ek(-30));
}

// =============================================================================
// Group 2: Subtraction — semantic invariants
// =============================================================================

// 5 − 3 = 2
void test_sub_basic()
{
    ek_t result = ek(5);
    result -= ek(3);
    ASSERT_EQ(result, ek(2));
}

// a − a = 0  (self-subtraction)
void test_sub_self()
{
    ek_t result = ek(999);
    result -= ek(999);
    ASSERT_EQ(result, ek(0));
}

// 3 − 5 = −2  (result negative)
void test_sub_negative_result()
{
    ek_t result = ek(3);
    result -= ek(5);
    ASSERT_EQ(result, ek(-2));
}

// (−5) − (−3) = −2  (subtracting a negative)
void test_sub_neg_neg()
{
    ek_t result = ek(-5);
    result -= ek(-3);
    ASSERT_EQ(result, ek(-2));
}

// =============================================================================
// Group 3: Combined round-trip invariants
// =============================================================================

// (a + b) − b == a
void test_roundtrip_add_sub()
{
    const ek_t a = ek(12345);
    const ek_t b = ek(6789);
    ek_t result = (a + b) - b;
    ASSERT_EQ(result, a);
}

// (a − b) + b == a
void test_roundtrip_sub_add()
{
    const ek_t a = ek(10000);
    const ek_t b = ek(3333);
    ek_t result = (a - b) + b;
    ASSERT_EQ(result, a);
}

// Negative round-trip: (a + b) − b == a  with a < 0
void test_roundtrip_negative_a()
{
    const ek_t a = ek(-42);
    const ek_t b = ek(100);
    ek_t result = (a + b) - b;
    ASSERT_EQ(result, a);
}

int main()
{
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "EK OPERATOR SEMANTICS TESTS: Excess-K + / - / = delete" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::cout << "\n[compile-time] Positive interface checks (8 static_asserts passed):" << std::endl;
    std::cout << "  PASS: EK operator+  available" << std::endl;
    std::cout << "  PASS: EK operator+= available" << std::endl;
    std::cout << "  PASS: EK operator-  available" << std::endl;
    std::cout << "  PASS: EK operator-= available" << std::endl;
    std::cout << "  PASS: MS operator*  available" << std::endl;
    std::cout << "  PASS: MS operator*= available" << std::endl;
    std::cout << "  PASS: MS operator/  available" << std::endl;
    std::cout << "  PASS: MS operator%  available" << std::endl;
    std::cout << "  NOTE: EK *, *=, /, /=, %, %=, divmod are = delete" << std::endl;
    std::cout << "        Negative compile tests live in tests/negative/ (future)" << std::endl;

    std::cout << "\n[1-5] Addition Invariants:" << std::endl;
    TEST_CASE("add_basic",          test_add_basic);
    TEST_CASE("add_cancel",         test_add_cancel);
    TEST_CASE("add_commutative",    test_add_commutative);
    TEST_CASE("add_large",          test_add_large);
    TEST_CASE("add_both_negative",  test_add_both_negative);

    std::cout << "\n[6-9] Subtraction Invariants:" << std::endl;
    TEST_CASE("sub_basic",           test_sub_basic);
    TEST_CASE("sub_self",            test_sub_self);
    TEST_CASE("sub_negative_result", test_sub_negative_result);
    TEST_CASE("sub_neg_neg",         test_sub_neg_neg);

    std::cout << "\n[10-12] Round-trip Invariants:" << std::endl;
    TEST_CASE("roundtrip_add_sub",   test_roundtrip_add_sub);
    TEST_CASE("roundtrip_sub_add",   test_roundtrip_sub_add);
    TEST_CASE("roundtrip_negative_a",test_roundtrip_negative_a);

    std::cout << std::string(70, '=') << std::endl;
    std::cout << "RESULTS: " << pass_count << "/" << test_count << " PASSED" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
