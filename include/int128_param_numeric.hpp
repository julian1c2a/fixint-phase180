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
// @file       int128_param_numeric.hpp
// @brief      Additional numeric algorithms for int128_param_t<S, F>
// @author     Julián Calderón Almendros
// @date       2026-01-19 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_NUMERIC_HPP
#define INT128_PARAM_NUMERIC_HPP

#include "int128_parameterized.hpp"
#include "int128_param_cmath.hpp" // For abs, gcd, etc.
#include <utility>
#include <cstdint>

namespace nstd
{
    // ========================================================================
    // SIGN FUNCTION - Returns -1, 0, or +1
    // ========================================================================

    /**
     * @brief Returns the sign of a value
     * @param x Value to check
     * @return -1 if negative, 0 if zero, +1 if positive
     *
     * @details Representation-aware:
     * - TC: Check MSB for sign
     * - MS: Check sign bit directly
     * - EK: Compare against bias
     */
    template <signedness S, representation_form F>
    constexpr int sign(const int128_param_t<S, F> &x) noexcept
    {
        if (x.is_zero())
            return 0;
        if constexpr (S == signedness::unsigned_type)
        {
            return 1; // Unsigned is always positive
        }
        else
        {
            return x.is_negative() ? -1 : 1;
        }
    }

    // ========================================================================
    // IS_EVEN / IS_ODD - Parity checks
    // ========================================================================

    /**
     * @brief Check if value is even
     * @param x Value to check
     * @return true if even, false if odd
     *
     * @details Works on LSB regardless of representation
     */
    template <signedness S, representation_form F>
    constexpr bool is_even(const int128_param_t<S, F> &x) noexcept
    {
        return (x.low() & 1) == 0;
    }

    /**
     * @brief Check if value is odd
     * @param x Value to check
     * @return true if odd, false if even
     */
    template <signedness S, representation_form F>
    constexpr bool is_odd(const int128_param_t<S, F> &x) noexcept
    {
        return (x.low() & 1) != 0;
    }

    // ========================================================================
    // ABS_DIFF - Absolute difference (no overflow)
    // ========================================================================

    /**
     * @brief Absolute difference between two values
     * @param a First value
     * @param b Second value
     * @return |a - b| (always non-negative)
     *
     * @details More efficient than abs(a - b) as it avoids intermediate overflow
     */
    template <signedness S, representation_form F>
    constexpr int128_param_t<S, F> abs_diff(const int128_param_t<S, F> &a,
                                            const int128_param_t<S, F> &b) noexcept
    {
        return (a >= b) ? (a - b) : (b - a);
    }

    // ========================================================================
    // ILOG2 - Integer log base 2 (floor(log2(x)))
    // ========================================================================

    /**
     * @brief Integer logarithm base 2
     * @param x Value (must be > 0)
     * @return floor(log2(x))
     *
     * @details
     * - Returns position of highest set bit (0-based)
     * - For x=0, behavior is undefined (returns -1)
     * - TC/EK: Full 128 bits
     * - MS: 127-bit magnitude space
     *
     * @note For EK: Operates on stored value (not real value).
     *       Convert to TC for semantic correctness.
     */
    template <signedness S, representation_form F>
    constexpr int ilog2(const int128_param_t<S, F> &x) noexcept
    {
        if (x.is_zero())
            return -1;

        // Use bit_width - 1 (bit_width returns 1-based position)
        // Need to use countl_zero from int128_param_bits.hpp
        // For now, implement directly here

        const uint64_t high = x.high();
        const uint64_t low = x.low();

        if constexpr (F == representation_form::magnitude_sign)
        {
            // MS: Work on 127-bit magnitude
            const uint64_t mag_high = high & ~(1ULL << 63); // Clear sign bit
            if (mag_high != 0)
            {
                return 63 + (63 - __builtin_clzll(mag_high));
            }
            else if (low != 0)
            {
                return 63 - __builtin_clzll(low);
            }
            return -1;
        }
        else
        {
            // TC/EK: Full 128 bits
            if (high != 0)
            {
                return 64 + (63 - __builtin_clzll(high));
            }
            else if (low != 0)
            {
                return 63 - __builtin_clzll(low);
            }
            return -1;
        }
    }

    // ========================================================================
    // ISQRT - Integer square root (floor(sqrt(x)))
    // ========================================================================

    /**
     * @brief Integer square root
     * @param x Value (must be >= 0)
     * @return floor(sqrt(x))
     *
     * @details
     * - Uses Newton's method for convergence
     * - O(log log x) iterations
     * - For negative x, returns 0 (undefined behavior)
     *
     * @note For EK: Operates on stored value (not real value).
     *       Convert to TC for semantic correctness.
     */
    template <signedness S, representation_form F>
    int128_param_t<S, F> isqrt(const int128_param_t<S, F> &x) noexcept
    {
        if (x.is_zero())
            return int128_param_t<S, F>{0, 0};
        if constexpr (S == signedness::signed_type)
        {
            if (x.is_negative())
                return int128_param_t<S, F>{0, 0}; // Undefined
        }

        // Newton's method: x_{n+1} = (x_n + value/x_n) / 2
        // Start with a good initial guess based on bit width
        int128_param_t<S, F> guess = x;

        // Better initial guess: x >> (bit_width / 2)
        const int log2_val = ilog2(x);
        if (log2_val > 0)
        {
            const int shift = log2_val / 2;
            for (int i = 0; i < shift && i < 64; ++i)
            {
                guess = guess >> 1;
            }
        }
        if (guess.is_zero())
        {
            guess = int128_param_t<S, F>{0, 1};
        }

        for (int i = 0; i < 128; ++i)
        {                                               // Max 128 iterations
            const auto next = (guess + x / guess) >> 1; // Divide by 2 using shift
            if (next >= guess)
                break; // Converged
            guess = next;
        }

        return guess;
    }

    // ========================================================================
    // FACTORIAL - n! for small n
    // ========================================================================

    /**
     * @brief Factorial function
     * @param n Input value (0 <= n <= ~34)
     * @return n!
     *
     * @details
     * - 0! = 1 (by definition)
     * - Maximum n depends on bit width:
     *   - 64-bit: n <= 20
     *   - 128-bit: n <= ~34
     * - For large n, result overflows (wraps around)
     *
     * @warning No overflow checking - use safe operations if needed
     */
    template <signedness S, representation_form F>
    int128_param_t<S, F> factorial(unsigned int n) noexcept
    {
        int128_param_t<S, F> result{0, 1};
        for (unsigned int i = 2; i <= n; ++i)
        {
            result *= int128_param_t<S, F>{0, i};
        }
        return result;
    }

    // ========================================================================
    // DIVMOD - Combined division and modulo
    // ========================================================================

    /**
     * @brief Compute quotient and remainder simultaneously
     * @param dividend Value to divide
     * @param divisor Value to divide by
     * @return pair<quotient, remainder>
     *
     * @details
     * - More efficient than separate / and % operations
     * - Returns {quotient, remainder} where dividend = quotient*divisor + remainder
     * - For divisor=0, behavior is undefined (returns {0, 0})
     */
    template <signedness S, representation_form F>
    constexpr std::pair<int128_param_t<S, F>, int128_param_t<S, F>>
    divmod(const int128_param_t<S, F> &dividend, const int128_param_t<S, F> &divisor) noexcept
    {
        if (divisor.is_zero())
        {
            return {int128_param_t<S, F>{0, 0}, int128_param_t<S, F>{0, 0}};
        }

        const auto quotient = dividend / divisor;
        const auto remainder = dividend % divisor;
        return {quotient, remainder};
    }

    // ========================================================================
    // POWER - Alias for pow (consistency with phase166)
    // ========================================================================

    /**
     * @brief Integer exponentiation (alias for pow)
     * @param base Base value
     * @param exponent Exponent (must be >= 0)
     * @return base^exponent
     *
     * @see pow
     */
    template <signedness S, representation_form F>
    constexpr int128_param_t<S, F> power(const int128_param_t<S, F> &base,
                                         unsigned int exponent) noexcept
    {
        return pow(base, exponent);
    }

} // namespace nstd

#endif // INT128_PARAM_NUMERIC_HPP
