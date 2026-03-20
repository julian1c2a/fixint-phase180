// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
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
// =============================================================================
// @file       int128_param_cmath.hpp
// @brief      Mathematical functions for int128_param_t<S, F>
// @author     Julián Calderón Almendros
// @date       2026-01-18 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_CMATH_HPP
#define INT128_PARAM_CMATH_HPP

#include "int128_parameterized.hpp"
#include <algorithm>
#include <numeric>

namespace nstd
{
    // ========================================================================
    // ABSOLUTE VALUE (Already implemented in main header, provided here for completeness)
    // ========================================================================

    /**
     * @brief Absolute value / magnitude
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to get absolute value of
     * @return Absolute value (always non-negative)
     *
     * @details Behavior varies by representation:
     * - **Two's Complement**: Negates if negative
     * - **Magnitude-Sign**: Clears sign bit (zero overhead)
     * - **Excess-K**: Subtracts bias, takes absolute value
     *
     * @note Unsigned types return self (no-op)
     * @note Already implemented in int128_parameterized.hpp as member function
     */
    template <signedness S, representation_form F>
    inline constexpr int128_param_t<S, F> abs(const int128_param_t<S, F> &value) noexcept
    {
        return value.abs();
    }

    // ========================================================================
    // MIN/MAX FUNCTIONS (Representation-Aware)
    // ========================================================================

    /**
     * @brief Return minimum of two values
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param a First value
     * @param b Second value
     * @return Minimum value
     *
     * @details Uses comparison operator< which is representation-aware
     */
    template <signedness S, representation_form F>
    inline constexpr int128_param_t<S, F> min(const int128_param_t<S, F> &a,
                                              const int128_param_t<S, F> &b) noexcept
    {
        return (a < b) ? a : b;
    }

    /**
     * @brief Return maximum of two values
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param a First value
     * @param b Second value
     * @return Maximum value
     *
     * @details Uses comparison operator> which is representation-aware
     */
    template <signedness S, representation_form F>
    inline constexpr int128_param_t<S, F> max(const int128_param_t<S, F> &a,
                                              const int128_param_t<S, F> &b) noexcept
    {
        return (a > b) ? a : b;
    }

    /**
     * @brief Clamp value to range [lo, hi]
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to clamp
     * @param lo Lower bound
     * @param hi Upper bound
     * @return Clamped value
     *
     * @details Returns:
     * - lo if value < lo
     * - hi if value > hi
     * - value otherwise
     *
     * @pre lo <= hi
     */
    template <signedness S, representation_form F>
    inline constexpr int128_param_t<S, F> clamp(const int128_param_t<S, F> &value,
                                                const int128_param_t<S, F> &lo,
                                                const int128_param_t<S, F> &hi) noexcept
    {
        return (value < lo) ? lo : ((value > hi) ? hi : value);
    }

    // ========================================================================
    // GREATEST COMMON DIVISOR (GCD) - Binary Stein's Algorithm
    // ========================================================================

    /**
     * @brief Greatest Common Divisor using binary GCD algorithm
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param a First value
     * @param b Second value
     * @return GCD of |a| and |b|
     *
     * @details Uses Stein's binary GCD algorithm (efficient for 128-bit integers):
     * - O(n) where n is bit width
     * - No division operations (uses shifts and subtractions)
     * - Works with absolute values
     *
     * @note Returns 0 if both values are zero
     * @note For signed types, returns GCD of absolute values
     * @note For EK: ⚠️ Convert to TC first for semantic correctness
     */
    template <signedness S, representation_form F>
    constexpr int128_param_t<S, F> gcd(const int128_param_t<S, F> &a,
                                       const int128_param_t<S, F> &b) noexcept
    {
        // Work with absolute values
        auto u = abs(a);
        auto v = abs(b);

        // GCD(0, v) = v, GCD(u, 0) = u
        if (u.is_zero())
            return v;
        if (v.is_zero())
            return u;

        // Find common factor of 2 (trailing zeros)
        const int shift_u = u.trailing_zeros();
        const int shift_v = v.trailing_zeros();
        const int shift = (shift_u < shift_v) ? shift_u : shift_v;

        // Remove factors of 2 from both
        u >>= shift_u;
        v >>= shift_v;

        // Binary GCD loop
        while (true)
        {
            // u is always odd here
            // If v is even, remove factor of 2
            if ((v.low() & 1) == 0)
            {
                v >>= v.trailing_zeros();
            }

            // Ensure u <= v
            if (u > v)
            {
                auto temp = u;
                u = v;
                v = temp;
            }

            // v -= u (v is now even)
            v -= u;

            if (v.is_zero())
            {
                break;
            }
        }

        // Restore common factor of 2
        return u << shift;
    }

    /**
     * @brief GCD with mixed types (int128 and builtin integral)
     */
    template <signedness S, representation_form F, typename T,
              typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr int128_param_t<S, F> gcd(const int128_param_t<S, F> &a, T b) noexcept
    {
        return gcd(a, int128_param_t<S, F>{b});
    }

    /**
     * @brief GCD with mixed types (builtin integral and int128)
     */
    template <signedness S, representation_form F, typename T,
              typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr int128_param_t<S, F> gcd(T a, const int128_param_t<S, F> &b) noexcept
    {
        return gcd(int128_param_t<S, F>{a}, b);
    }

    // ========================================================================
    // LEAST COMMON MULTIPLE (LCM)
    // ========================================================================

    /**
     * @brief Least Common Multiple
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param a First value
     * @param b Second value
     * @return LCM of |a| and |b|
     *
     * @details Uses formula: lcm(a,b) = |a| / gcd(a,b) * |b|
     * - Division before multiplication to avoid overflow
     * - Works with absolute values
     *
     * @note Returns 0 if either value is zero
     * @note For EK: ⚠️ Convert to TC first for semantic correctness
     */
    template <signedness S, representation_form F>
    constexpr int128_param_t<S, F> lcm(const int128_param_t<S, F> &a,
                                       const int128_param_t<S, F> &b) noexcept
    {
        // Work with absolute values
        const auto abs_a = abs(a);
        const auto abs_b = abs(b);

        // LCM(0, v) = 0, LCM(u, 0) = 0
        if (abs_a.is_zero() || abs_b.is_zero())
        {
            return int128_param_t<S, F>{0, 0};
        }

        // lcm(a,b) = (a / gcd(a,b)) * b
        const auto gcd_val = gcd(abs_a, abs_b);
        return (abs_a / gcd_val) * abs_b;
    }

    /**
     * @brief LCM with mixed types (int128 and builtin integral)
     */
    template <signedness S, representation_form F, typename T,
              typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr int128_param_t<S, F> lcm(const int128_param_t<S, F> &a, T b) noexcept
    {
        return lcm(a, int128_param_t<S, F>{b});
    }

    /**
     * @brief LCM with mixed types (builtin integral and int128)
     */
    template <signedness S, representation_form F, typename T,
              typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr int128_param_t<S, F> lcm(T a, const int128_param_t<S, F> &b) noexcept
    {
        return lcm(int128_param_t<S, F>{a}, b);
    }

    // ========================================================================
    // MIDPOINT (Overflow-Safe)
    // ========================================================================

    /**
     * @brief Midpoint of two values (overflow-safe)
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param a First value
     * @param b Second value
     * @return (a + b) / 2, rounded towards a
     *
     * @details Uses overflow-safe formula: a + (b-a)/2
     * - No intermediate overflow
     * - Rounds towards first argument
     *
     * @note For EK: ⚠️ Result may be incorrect - convert to TC first
     */
    template <signedness S, representation_form F>
    constexpr int128_param_t<S, F> midpoint(const int128_param_t<S, F> &a,
                                            const int128_param_t<S, F> &b) noexcept
    {
        // Overflow-safe: a + (b-a)/2
        const auto diff = b - a;
        const auto half_diff = diff >> 1; // Arithmetic shift for signed
        return a + half_diff;
    }

    // ========================================================================
    // POWER FUNCTION (Integer Exponentiation by Squaring)
    // ========================================================================

    /**
     * @brief Integer power (exponentiation by squaring)
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param base Base value
     * @param exponent Non-negative exponent
     * @return base^exponent
     *
     * @details Uses binary exponentiation:
     * - O(log n) complexity
     * - Efficient for large exponents
     *
     * @note Returns 1 for exponent = 0 (even if base = 0)
     * @note For negative exponents, use floating-point conversion
     * @note For EK: ⚠️ Convert to TC first for semantic correctness
     *
     * @pre exponent >= 0
     */
    template <signedness S, representation_form F>
    constexpr int128_param_t<S, F> pow(const int128_param_t<S, F> &base,
                                       unsigned int exponent) noexcept
    {
        if (exponent == 0)
        {
            return int128_param_t<S, F>{0, 1}; // base^0 = 1
        }

        int128_param_t<S, F> result{0, 1};
        int128_param_t<S, F> current_base = base;

        while (exponent > 0)
        {
            if (exponent & 1)
            {
                result *= current_base;
            }
            current_base *= current_base;
            exponent >>= 1;
        }

        return result;
    }

    // ========================================================================
    // TYPE ALIASES FOR COMMON OPERATIONS
    // ========================================================================

    // Two's Complement (backward compatible)
    using uint128_tc_t = int128_param_t<signedness::unsigned_type, representation_form::twos_complement>;
    using int128_tc_t = int128_param_t<signedness::signed_type, representation_form::twos_complement>;

    // Magnitude-Sign
    using uint128_ms_t = int128_param_t<signedness::unsigned_type, representation_form::magnitude_sign>;
    using int128_ms_t = int128_param_t<signedness::signed_type, representation_form::magnitude_sign>;

    // Excess-K
    using uint128_ek_t = int128_param_t<signedness::unsigned_type, representation_form::excess_k>;
    using int128_ek_t = int128_param_t<signedness::signed_type, representation_form::excess_k>;

} // namespace nstd

#endif // INT128_PARAM_CMATH_HPP
