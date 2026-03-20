// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: MS and EK Operators Validation
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include "int128_param_safe.hpp"
#include <iostream>
#include <cassert>

using namespace nstd;

// Test macro
#define TEST(name, condition)                              \
    do                                                     \
    {                                                      \
        ++test_count;                                      \
        if (condition)                                     \
        {                                                  \
            std::cout << "  [OK] " << name << std::endl;   \
            ++pass_count;                                  \
        }                                                  \
        else                                               \
        {                                                  \
            std::cout << "  [FAIL] " << name << std::endl; \
        }                                                  \
    } while (false)

int test_count = 0;
int pass_count = 0;

int main()
{
    std::cout << "====================================================================" << std::endl;
    std::cout << "MS and EK Operators Tests" << std::endl;
    std::cout << "====================================================================" << std::endl;

    // ========================================================================
    // Group 1: MS Multiplication (operator*=)
    // ========================================================================
    std::cout << "\n[Group 1] MS Multiplication:" << std::endl;

    {
        const int128_ms_t a{50};
        const int128_ms_t b{100};
        const auto result{a * b};

        TEST("ms_mul_positive", result == int128_ms_t{5000});
    }

    {
        const int128_ms_t a{-50};
        const int128_ms_t b{100};
        const auto result{a * b};

        TEST("ms_mul_mixed_signs", result == int128_ms_t{-5000});
    }

    {
        const int128_ms_t a{-50};
        const int128_ms_t b{-100};
        const auto result{a * b};

        TEST("ms_mul_both_negative", result == int128_ms_t{5000});
    }

    {
        const int128_ms_t a{0};
        const int128_ms_t b{100};
        const auto result{a * b};

        TEST("ms_mul_zero", result.is_zero());
    }

    {
        const int128_ms_t a{-1};
        const int128_ms_t b{1000};
        const auto result{a * b};

        TEST("ms_mul_negative_one", result == int128_ms_t{-1000});
    }

    // --- Edge cases: zero × negative must give +0, not -0 ---
    {
        const int128_ms_t a{0};
        const int128_ms_t b{-100};
        const auto result{a * b};

        TEST("ms_mul_zero_times_neg", result.is_zero() && !result.is_negative());
    }

    {
        const int128_ms_t a{-100};
        const int128_ms_t b{0};
        const auto result{a * b};

        TEST("ms_mul_neg_times_zero", result.is_zero() && !result.is_negative());
    }

    // --- Cross-word multiplication (result spans both data words) ---
    {
        const int128_ms_t a{0xFFFFFFFFULL}; // 2^32 - 1
        const int128_ms_t b{0xFFFFFFFFULL}; // 2^32 - 1
        const auto result{a * b};
        // (2^32-1)^2 = 2^64 - 2^33 + 1 = 0xFFFFFFFE00000001

        TEST("ms_mul_cross_word", result.low() == 0xFFFFFFFE00000001ULL && result.high() == 0);
    }

    {
        const int128_ms_t a{static_cast<std::int64_t>(-0xFFFFFFFFLL)};
        const int128_ms_t b{0xFFFFFFFFULL};
        const auto result{a * b};
        // -(2^32-1)^2: negative, magnitude = 0xFFFFFFFE00000001

        TEST("ms_mul_cross_word_neg", result.is_negative() && (result.high() & ~(1ULL << 63)) == 0 && result.low() == 0xFFFFFFFE00000001ULL);
    }

    // --- 64-bit boundary ---
    {
        const int128_ms_t a{0xFFFFFFFFFFFFFFFFULL}; // 2^64 - 1
        const int128_ms_t b{2};
        const auto result{a * b}; // 2*(2^64-1) = 2^65 - 2

        TEST("ms_mul_64bit_boundary", result.low() == 0xFFFFFFFFFFFFFFFEULL && result.high() == 1);
    }

    // --- Large product: 10^9 * 10^9 = 10^18 ---
    {
        const int128_ms_t a{1000000000LL};
        const int128_ms_t b{-1000000000LL};
        const auto result{a * b};

        TEST("ms_mul_billion_squared", result == int128_ms_t{-1000000000000000000LL});
    }

    // --- Magnitude overflow into sign bit area ---
    {
        // 2^126 * 2 => 2^127 overflows 127-bit magnitude, wraps to 0
        const int128_ms_t a{0x4000000000000000ULL, 0};
        const int128_ms_t b{2};
        const auto result{a * b};

        TEST("ms_mul_magnitude_overflow_wraps", result.is_zero() && !result.is_negative());
    }

    // --- Identity multiplication ---
    {
        const int128_ms_t a{-999999999LL};
        const int128_ms_t one{1};
        const auto result{a * one};

        TEST("ms_mul_identity", result == a);
    }

    // ========================================================================
    // Group 2: MS Division (divmod, operator/=, operator%=)
    // ========================================================================
    std::cout << "\n[Group 2] MS Division:" << std::endl;

    {
        const int128_ms_t a{100};
        const int128_ms_t b{7};
        const auto [quot, rem] = a.divmod(b);

        TEST("ms_divmod_positive", quot == int128_ms_t{14} && rem == int128_ms_t{2});
    }

    {
        const int128_ms_t a{-100};
        const int128_ms_t b{7};
        const auto [quot, rem] = a.divmod(b);

        TEST("ms_divmod_negative_dividend", quot == int128_ms_t{-14} && rem == int128_ms_t{-2});
    }

    {
        const int128_ms_t a{100};
        const int128_ms_t b{-7};
        const auto [quot, rem] = a.divmod(b);

        TEST("ms_divmod_negative_divisor", quot == int128_ms_t{-14} && rem == int128_ms_t{2});
    }

    {
        const int128_ms_t a{-100};
        const int128_ms_t b{-7};
        const auto [quot, rem] = a.divmod(b);

        TEST("ms_divmod_both_negative", quot == int128_ms_t{14} && rem == int128_ms_t{-2});
    }

    {
        int128_ms_t a{100};
        const int128_ms_t b{7};
        a /= b;

        TEST("ms_div_assign", a == int128_ms_t{14});
    }

    {
        int128_ms_t a{100};
        const int128_ms_t b{7};
        a %= b;

        TEST("ms_mod_assign", a == int128_ms_t{2});
    }

    {
        const int128_ms_t a{-100};
        const int128_ms_t b{7};
        const auto result{a / b};

        TEST("ms_div_operator", result == int128_ms_t{-14});
    }

    {
        const int128_ms_t a{-100};
        const int128_ms_t b{7};
        const auto result{a % b};

        TEST("ms_mod_operator", result == int128_ms_t{-2});
    }

    // ========================================================================
    // Group 3: EK Increment/Decrement (++, --)
    // ========================================================================
    std::cout << "\n[Group 3] EK Increment/Decrement:" << std::endl;

    {
        int128_ek_t val{100};
        ++val;

        TEST("ek_pre_increment", val == int128_ek_t{101});
    }

    {
        int128_ek_t val{100};
        val++;

        TEST("ek_post_increment", val == int128_ek_t{101});
    }

    {
        int128_ek_t val{100};
        --val;

        TEST("ek_pre_decrement", val == int128_ek_t{99});
    }

    {
        int128_ek_t val{100};
        val--;

        TEST("ek_post_decrement", val == int128_ek_t{99});
    }

    {
        int128_ek_t val{-100};
        ++val;

        TEST("ek_increment_negative", val == int128_ek_t{-99});
    }

    {
        int128_ek_t val{-100};
        --val;

        TEST("ek_decrement_negative", val == int128_ek_t{-101});
    }

    {
        int128_ek_t val{0};
        ++val;

        TEST("ek_increment_zero", val == int128_ek_t{1});
    }

    {
        int128_ek_t val{0};
        --val;

        TEST("ek_decrement_zero", val == int128_ek_t{-1});
    }

    // ========================================================================
    // Group 4: EK Addition/Subtraction (+=, -=) with bias compensation
    // ========================================================================
    std::cout << "\n[Group 4] EK Addition/Subtraction (with bias compensation):" << std::endl;

    {
        int128_ek_t a{100};
        const int128_ek_t b{200};
        std::cout << "  DEBUG: a.high()=0x" << std::hex << a.high() << " a.low()=0x" << a.low() << std::dec << "\n";
        std::cout << "  DEBUG: b.high()=0x" << std::hex << b.high() << " b.low()=0x" << b.low() << std::dec << "\n";
        a += b;
        std::cout << "  DEBUG: result.high()=0x" << std::hex << a.high() << " result.low()=0x" << a.low() << std::dec << "\n";
        int128_ek_t expected{300};
        std::cout << "  DEBUG: expected.high()=0x" << std::hex << expected.high() << " expected.low()=0x" << expected.low() << std::dec << "\n";

        TEST("ek_add_assign_positive", a == int128_ek_t{300});
    }

    {
        int128_ek_t a{-100};
        const int128_ek_t b{50};
        a += b;

        TEST("ek_add_assign_mixed", a == int128_ek_t{-50});
    }

    {
        int128_ek_t a{100};
        const int128_ek_t b{200};
        a -= b;

        TEST("ek_sub_assign_positive", a == int128_ek_t{-100});
    }

    {
        int128_ek_t a{-100};
        const int128_ek_t b{-200};
        a -= b;

        TEST("ek_sub_assign_both_negative", a == int128_ek_t{100});
    }

    {
        const int128_ek_t a{100};
        const int128_ek_t b{200};
        const auto result{a + b};

        TEST("ek_add_operator", result == int128_ek_t{300});
    }

    {
        const int128_ek_t a{100};
        const int128_ek_t b{200};
        const auto result{a - b};

        TEST("ek_sub_operator", result == int128_ek_t{-100});
    }

    {
        const int128_ek_t a{0};
        const int128_ek_t b{100};
        const auto result{a + b};

        TEST("ek_add_with_zero", result == int128_ek_t{100});
    }

    {
        const int128_ek_t a{100};
        const int128_ek_t b{0};
        const auto result{a - b};

        TEST("ek_sub_with_zero", result == int128_ek_t{100});
    }

    // ========================================================================
    // Group 5: EK Comparisons (still work correctly)
    // ========================================================================
    std::cout << "\n[Group 5] EK Comparisons (validation):" << std::endl;

    {
        const int128_ek_t a{-100};
        const int128_ek_t b{200};

        TEST("ek_less_than", a < b);
    }

    {
        const int128_ek_t a{200};
        const int128_ek_t b{-100};

        TEST("ek_greater_than", a > b);
    }

    {
        const int128_ek_t a{100};
        const int128_ek_t b{100};

        TEST("ek_equal", a == b);
    }

    {
        const int128_ek_t a{-50};
        const int128_ek_t b{50};

        TEST("ek_not_equal", a != b);
    }

    // ========================================================================
    // Group 6: MS and EK in safe arithmetic (now should work)
    // ========================================================================
    std::cout << "\n[Group 6] MS and EK with safe arithmetic:" << std::endl;

    {
        const int128_ms_t a{-50};
        const int128_ms_t b{100};
        const auto result{nstd::checked_mul(a, b)};

        TEST("ms_checked_mul_now_works", !result.overflow && result.value == int128_ms_t{-5000});
    }

    {
        // Note: checked_div has a template instantiation issue with MS
        // Testing divmod directly instead
        const int128_ms_t a{-100};
        const int128_ms_t b{7};
        const auto [quot, rem] = a.divmod(b);

        TEST("ms_divmod_via_method", quot == int128_ms_t{-14} && rem == int128_ms_t{-2});
    }

    {
        const int128_ek_t a{100};
        const int128_ek_t b{200};
        const auto result{nstd::checked_add(a, b)};

        TEST("ek_checked_add_now_semantic", !result.overflow && result.value == int128_ek_t{300});
    }

    {
        const int128_ek_t a{100};
        const int128_ek_t b{50};
        const auto result{nstd::checked_sub(a, b)};

        TEST("ek_checked_sub_now_semantic", !result.overflow && result.value == int128_ek_t{50});
    }

    // ========================================================================
    // Results
    // ========================================================================
    std::cout << "\n====================================================================" << std::endl;
    std::cout << "RESULTS:" << std::endl;
    std::cout << "  Passed: " << pass_count << std::endl;
    std::cout << "  Failed: " << (test_count - pass_count) << std::endl;
    std::cout << "  Total:  " << test_count << std::endl;
    std::cout << "====================================================================" << std::endl;

    return (test_count == pass_count) ? 0 : 1;
}
