// ==============================================================================
// Priority 1: Build Validation + Constructors Test
// Phase 1.75 - int128 parameterized template
// ==============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julián Calderón Almendros
// Email: julian.calderon.almendros@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// ==============================================================================

#include "representation.hpp"
#include "int128_parameterized.hpp"

#include <iostream>
#include <cassert>
#include <string>

using namespace nstd;

// ==============================================================================
// Test Macros
// ==============================================================================

#define TEST(name)      \
    void test_##name(); \
    test_##name();      \
    std::cout << "✓ " << #name << std::endl;
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_TRUE(a) assert(a)
#define ASSERT_FALSE(a) assert(!(a))

// ==============================================================================
// Test Cases
// ==============================================================================

// TEST 1: Default constructor
void test_default_constructor()
{
    uint128_t x;
    ASSERT_EQ(x.high(), 0ULL);
    ASSERT_EQ(x.low(), 0ULL);
    ASSERT_TRUE(x.is_zero());
}

// TEST 2: Constructor from uint64_t
void test_constructor_uint64()
{
    uint128_t x(42ULL);
    ASSERT_EQ(x.low(), 42ULL);
    ASSERT_EQ(x.high(), 0ULL);
}

// TEST 3: Constructor from int64_t (positive)
void test_constructor_int64_positive()
{
    int128_t x(100LL);
    ASSERT_EQ(x.low(), 100ULL);
    ASSERT_EQ(x.high(), 0ULL);
    ASSERT_FALSE(x.is_negative());
}

// TEST 4: Constructor from int64_t (negative)
void test_constructor_int64_negative()
{
    int128_t x(-1LL);
    ASSERT_EQ(x.low(), 0xFFFFFFFFFFFFFFFFULL); // Two's complement: -1 = all bits 1
    ASSERT_EQ(x.high(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_TRUE(x.is_negative());
}

// TEST 5: Constructor from (high, low) pair
void test_constructor_high_low()
{
    uint128_t x(0x0000000000000001ULL, 0x0000000000000002ULL);
    ASSERT_EQ(x.low(), 0x0000000000000002ULL);
    ASSERT_EQ(x.high(), 0x0000000000000001ULL);
}

// TEST 6: Copy constructor
void test_copy_constructor()
{
    uint128_t x(123ULL);
    uint128_t y = x;
    ASSERT_EQ(y.low(), 123ULL);
    ASSERT_EQ(y.high(), 0ULL);
}

// TEST 7: Accessor high()
void test_accessor_high()
{
    uint128_t x(0x0102030405060708ULL, 0xDEADBEEFCAFEBABEULL); // high, low
    ASSERT_EQ(x.high(), 0x0102030405060708ULL);
}

// TEST 8: Accessor low()
void test_accessor_low()
{
    uint128_t x(0x0102030405060708ULL, 0xDEADBEEFCAFEBABEULL); // high, low
    ASSERT_EQ(x.low(), 0xDEADBEEFCAFEBABEULL);
}

// TEST 9: Setter set_high()
void test_setter_set_high()
{
    uint128_t x(0x0000000000000001ULL, 0x0000000000000002ULL);
    x.set_high(0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(x.high(), 0xFFFFFFFFFFFFFFFFULL);
    ASSERT_EQ(x.low(), 0x0000000000000002ULL);
}

// TEST 10: Setter set_low()
void test_setter_set_low()
{
    uint128_t x(0x0000000000000001ULL, 0x0000000000000002ULL);
    x.set_low(0xAAAAAAAAAAAAAAAAULL);
    ASSERT_EQ(x.low(), 0xAAAAAAAAAAAAAAAAULL);
    ASSERT_EQ(x.high(), 0x0000000000000001ULL);
}

// TEST 11: is_zero() on zero value
void test_is_zero_true()
{
    uint128_t x(0ULL);
    ASSERT_TRUE(x.is_zero());
}

// TEST 12: is_zero() on non-zero value
void test_is_zero_false()
{
    uint128_t x(1ULL);
    ASSERT_FALSE(x.is_zero());
}

// TEST 13: is_negative() on positive signed value
void test_is_negative_positive()
{
    int128_t x(42LL);
    ASSERT_FALSE(x.is_negative());
}

// TEST 14: is_negative() on negative signed value
void test_is_negative_negative()
{
    int128_t x(-42LL);
    ASSERT_TRUE(x.is_negative());
}

// TEST 15: Equality operator
void test_equality_operator()
{
    uint128_t x(42ULL);
    uint128_t y(42ULL);
    uint128_t z(43ULL);

    ASSERT_TRUE(x == y);
    ASSERT_FALSE(x == z);
}

// TEST 16: Inequality operator
void test_inequality_operator()
{
    uint128_t x(42ULL);
    uint128_t y(42ULL);
    uint128_t z(43ULL);

    ASSERT_FALSE(x != y);
    ASSERT_TRUE(x != z);
}

// TEST 17: Type traits - signedness
void test_type_traits_signedness()
{
    ASSERT_EQ(uint128_t::sign, signedness::unsigned_type);
    ASSERT_EQ(int128_t::sign, signedness::signed_type);
}

// TEST 18: Type traits - representation form
void test_type_traits_form()
{
    ASSERT_EQ(uint128_t::form, representation_form::twos_complement);
    ASSERT_EQ(int128_t::form, representation_form::twos_complement);
}

// TEST 19: Type traits - is_signed constant
void test_type_traits_is_signed()
{
    ASSERT_FALSE(uint128_t::is_signed);
    ASSERT_TRUE(int128_t::is_signed);
}

// TEST 20: Type aliases validation
void test_type_aliases()
{
    uint128_t x(42ULL);
    ASSERT_EQ(x.form, representation_form::magnitude_sign);

    int128_ek_t y(42LL);
    ASSERT_EQ(y.form, representation_form::excess_k);
}

// ==============================================================================
// Main Test Runner
// ==============================================================================

int main()
{
    try
    {
        std::cout << "\n==================== PRIORITY 1 TESTS ====================\n\n";

        std::cout << "Constructors and Accessors:\n";
        TEST(default_constructor);
        TEST(constructor_uint64);
        TEST(constructor_int64_positive);
        TEST(constructor_int64_negative);
        TEST(constructor_high_low);
        TEST(copy_constructor);

        std::cout << "\nAccessors and Setters:\n";
        TEST(accessor_high);
        TEST(accessor_low);
        TEST(setter_set_high);
        TEST(setter_set_low);

        std::cout << "\nLogical Operations:\n";
        TEST(is_zero_true);
        TEST(is_zero_false);
        TEST(is_negative_positive);
        TEST(is_negative_negative);

        std::cout << "\nComparison Operators:\n";
        TEST(equality_operator);
        TEST(inequality_operator);

        std::cout << "\nType System:\n";
        TEST(type_traits_signedness);
        TEST(type_traits_form);
        TEST(type_traits_is_signed);
        TEST(type_aliases);

        std::cout << "\n✅ All 20 tests PASSED!\n";
        std::cout << "========================================================\n\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
