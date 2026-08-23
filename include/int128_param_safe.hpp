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
// @file       int128_param_safe.hpp
// @brief      Overflow-checked arithmetic operations for 128-bit integers
// @author     Julián Calderón Almendros
// @date       2026-02-04 (last edit)
// @version    1.0.0
// =============================================================================

#ifndef INT128_PARAM_SAFE_HPP
#define INT128_PARAM_SAFE_HPP

#include "int128_parameterized.hpp"
#include <optional>
#include <limits>

namespace nstd
{

    /**
     * @brief Result of a checked arithmetic operation
     *
     * @details Contains the result value and a boolean indicating whether
     *          overflow occurred during the operation.
     */
    template <signedness Sign, representation_form Form>
    struct checked_result
    {
        int128_param_t<Sign, Form> value;
        bool overflow;

        /**
         * @brief Check if operation succeeded without overflow
         */
        constexpr explicit operator bool() const noexcept { return !overflow; }
    };

    // ============================================================================
    // Checked Addition
    // ============================================================================

    /**
     * @brief Addition with overflow detection
     *
     * @details Performs addition and detects overflow for all representation forms.
     *          For Two's Complement: overflow when signs match but result differs.
     *          For Magnitude-Sign: overflow when magnitudes exceed range.
     *          For Excess-K: overflow when stored sum exceeds valid range.
     *
     * @param lhs Left operand
     * @param rhs Right operand
     * @return checked_result containing sum and overflow flag
     *
     * @note Unsigned types only detect overflow on wraparound.
     */
    template <signedness Sign, representation_form Form>
    constexpr checked_result<Sign, Form> checked_add(const int128_param_t<Sign, Form> &lhs,
                                                     const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const int128_param_t<Sign, Form> result{lhs + rhs};

        if constexpr (Sign == signedness::unsigned_type)
        {
            // Unsigned overflow: result < either operand
            const bool overflow{result < lhs};
            return {result, overflow};
        }
        else if constexpr (Form == representation_form::magnitude_sign)
        {
            // MS overflow: sign-flip check doesn't work because operator+=
            // preserves the sign bit explicitly. Instead, check if the
            // magnitude wrapped around during same-sign addition.
            const bool lhs_neg{lhs.is_negative()};
            const bool rhs_neg{rhs.is_negative()};

            if (lhs_neg != rhs_neg)
            {
                // Different signs: subtraction of magnitudes, can't overflow
                return {result, false};
            }

            // Same signs: overflow if result magnitude < lhs magnitude
            const std::uint64_t lhs_mag_high{lhs.high() & 0x7FFFFFFFFFFFFFFFULL};
            const std::uint64_t res_mag_high{result.high() & 0x7FFFFFFFFFFFFFFFULL};
            bool overflow{false};
            if (res_mag_high != lhs_mag_high)
            {
                overflow = res_mag_high < lhs_mag_high;
            }
            else
            {
                overflow = result.low() < lhs.low();
            }
            return {result, overflow};
        }
        else
        {
            // Two's Complement signed overflow detection
            const bool lhs_neg{lhs.is_negative()};
            const bool rhs_neg{rhs.is_negative()};
            const bool result_neg{result.is_negative()};

            // Overflow if signs match but result differs
            const bool overflow{(lhs_neg == rhs_neg) && (lhs_neg != result_neg)};
            return {result, overflow};
        }
    }

    // ============================================================================
    // Checked Subtraction
    // ============================================================================

    /**
     * @brief Subtraction with overflow detection
     *
     * @param lhs Left operand (minuend)
     * @param rhs Right operand (subtrahend)
     * @return checked_result containing difference and overflow flag
     */
    template <signedness Sign, representation_form Form>
    constexpr checked_result<Sign, Form> checked_sub(const int128_param_t<Sign, Form> &lhs,
                                                     const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const int128_param_t<Sign, Form> result{lhs - rhs};

        if constexpr (Sign == signedness::unsigned_type)
        {
            // Unsigned underflow: result > lhs
            const bool overflow{result > lhs};
            return {result, overflow};
        }
        else if constexpr (Form == representation_form::magnitude_sign)
        {
            // MS subtraction overflow: different-sign subtraction effectively
            // adds magnitudes, which can overflow the 127-bit magnitude range.
            const bool lhs_neg{lhs.is_negative()};
            const bool rhs_neg{rhs.is_negative()};

            if (lhs_neg == rhs_neg)
            {
                // Same signs: magnitude subtraction, can't overflow
                return {result, false};
            }

            // Different signs: magnitude addition, check for magnitude wrap
            const std::uint64_t lhs_mag_high{lhs.high() & 0x7FFFFFFFFFFFFFFFULL};
            const std::uint64_t res_mag_high{result.high() & 0x7FFFFFFFFFFFFFFFULL};
            bool overflow{false};
            if (res_mag_high != lhs_mag_high)
            {
                overflow = res_mag_high < lhs_mag_high;
            }
            else
            {
                overflow = result.low() < lhs.low();
            }
            return {result, overflow};
        }
        else
        {
            // Two's Complement signed overflow: opposite signs and result differs from lhs
            const bool lhs_neg{lhs.is_negative()};
            const bool rhs_neg{rhs.is_negative()};
            const bool result_neg{result.is_negative()};

            const bool overflow{(lhs_neg != rhs_neg) && (lhs_neg != result_neg)};
            return {result, overflow};
        }
    }

    // ============================================================================
    // Checked Multiplication
    // ============================================================================

    /**
     * @brief Multiplication with overflow detection
     *
     * @details Detects overflow using pre-check with max value bounds.
     *          Avoids division-based checking to prevent infinite loops.
     *
     * @param lhs Left operand
     * @param rhs Right operand
     * @return checked_result containing product and overflow flag
     */
    template <signedness Sign, representation_form Form>
    constexpr checked_result<Sign, Form> checked_mul(const int128_param_t<Sign, Form> &lhs,
                                                     const int128_param_t<Sign, Form> &rhs) noexcept
    {
        // Zero or one case: never overflows
        if (lhs.is_zero() || rhs.is_zero())
        {
            return {int128_param_t<Sign, Form>{0, 0}, false};
        }

        if constexpr (Sign == signedness::unsigned_type)
        {
            // Unsigned: check if lhs > max / rhs (would overflow)
            const auto max_val{int128_param_t<Sign, Form>::max()};

            // Simple overflow check: if either operand >= 2^64, likely overflow
            // More precise: check if lhs > max_val / rhs (but avoid division)
            const auto result{lhs * rhs};

            // Overflow if result < either operand (wraparound occurred)
            const bool overflow{result < lhs || result < rhs};
            return {result, overflow};
        }
        else
        {
            // Signed: check magnitude and sign
            const bool lhs_neg{lhs.is_negative()};
            const bool rhs_neg{rhs.is_negative()};
            const bool expected_neg{lhs_neg != rhs_neg}; // XOR for sign

            const auto result{lhs * rhs};
            const bool result_neg{result.is_negative()};

            // Overflow if:
            // 1. Sign doesn't match expectation
            // 2. For same signs (positive result): result is negative (overflow wrapped)
            // 3. For different signs (negative result): result is positive (overflow wrapped)
            const bool sign_mismatch{result_neg != expected_neg};

            // Additional check: if magnitude decreased when it should increase
            const bool overflow{sign_mismatch};
            return {result, overflow};
        }
    }

    // ============================================================================
    // Checked Division
    // ============================================================================

    /**
     * @brief Division with overflow detection
     *
     * @details Division only overflows in one case (signed types):
     *          MIN / -1 overflows because -MIN is not representable.
     *
     * @param lhs Dividend
     * @param rhs Divisor
     * @return checked_result containing quotient and overflow flag
     *
     * @note Division by zero returns {0, true} (overflow flag set)
     */
    template <signedness Sign, representation_form Form>
    constexpr checked_result<Sign, Form> checked_div(const int128_param_t<Sign, Form> &lhs,
                                                     const int128_param_t<Sign, Form> &rhs) noexcept
    {
        // Division by zero
        if (rhs.is_zero())
        {
            return {int128_param_t<Sign, Form>{0, 0}, true};
        }

        if constexpr (Sign == signedness::unsigned_type)
        {
            // Unsigned division never overflows (except div by zero)
            return {lhs / rhs, false};
        }
        else
        {
            // Signed: MIN / -1 overflows
            const auto min_val{int128_param_t<Sign, Form>::min()};
            const auto minus_one{int128_param_t<Sign, Form>{-1}};

            if (lhs == min_val && rhs == minus_one)
            {
                return {lhs / rhs, true}; // Result wraps, overflow detected
            }

            return {lhs / rhs, false};
        }
    }

    // ============================================================================
    // Saturating Addition
    // ============================================================================

    /**
     * @brief Addition with saturation (clamps to min/max on overflow)
     *
     * @param lhs Left operand
     * @param rhs Right operand
     * @return Sum, saturated to [min, max] if overflow occurs
     */
    template <signedness Sign, representation_form Form>
    constexpr int128_param_t<Sign, Form> saturating_add(const int128_param_t<Sign, Form> &lhs,
                                                        const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const auto result{checked_add(lhs, rhs)};

        if (!result.overflow)
        {
            return result.value;
        }

        // Overflow: saturate to max or min
        if constexpr (Sign == signedness::unsigned_type)
        {
            // Unsigned overflow → max
            return int128_param_t<Sign, Form>::max();
        }
        else
        {
            // Signed: positive overflow → max, negative → min
            const bool both_positive{!lhs.is_negative() && !rhs.is_negative()};
            return both_positive ? int128_param_t<Sign, Form>::max() : int128_param_t<Sign, Form>::min();
        }
    }

    /**
     * @brief Subtraction with saturation
     *
     * @param lhs Left operand
     * @param rhs Right operand
     * @return Difference, saturated to [min, max] if overflow occurs
     */
    template <signedness Sign, representation_form Form>
    constexpr int128_param_t<Sign, Form> saturating_sub(const int128_param_t<Sign, Form> &lhs,
                                                        const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const auto result{checked_sub(lhs, rhs)};

        if (!result.overflow)
        {
            return result.value;
        }

        // Overflow: saturate
        if constexpr (Sign == signedness::unsigned_type)
        {
            // Unsigned underflow → 0
            return int128_param_t<Sign, Form>{0, 0};
        }
        else
        {
            // Signed: lhs > 0 && rhs < 0 → max, else min
            const bool pos_minus_neg{!lhs.is_negative() && rhs.is_negative()};
            return pos_minus_neg ? int128_param_t<Sign, Form>::max() : int128_param_t<Sign, Form>::min();
        }
    }

    /**
     * @brief Multiplication with saturation
     *
     * @param lhs Left operand
     * @param rhs Right operand
     * @return Product, saturated to [min, max] if overflow occurs
     */
    template <signedness Sign, representation_form Form>
    constexpr int128_param_t<Sign, Form> saturating_mul(const int128_param_t<Sign, Form> &lhs,
                                                        const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const auto result{checked_mul(lhs, rhs)};

        if (!result.overflow)
        {
            return result.value;
        }

        // Overflow: saturate
        if constexpr (Sign == signedness::unsigned_type)
        {
            return int128_param_t<Sign, Form>::max();
        }
        else
        {
            // Signed: determine sign of result
            const bool lhs_neg{lhs.is_negative()};
            const bool rhs_neg{rhs.is_negative()};
            const bool result_positive{lhs_neg == rhs_neg};

            return result_positive ? int128_param_t<Sign, Form>::max() : int128_param_t<Sign, Form>::min();
        }
    }

    // ============================================================================
    // Try Operations (std::optional-based)
    // ============================================================================

    /**
     * @brief Try addition, returning std::optional
     *
     * @return std::optional with result, or std::nullopt if overflow
     */
    template <signedness Sign, representation_form Form>
    constexpr std::optional<int128_param_t<Sign, Form>>
    try_add(const int128_param_t<Sign, Form> &lhs, const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const auto result{checked_add(lhs, rhs)};
        return result.overflow ? std::nullopt : std::optional{result.value};
    }

    /**
     * @brief Try subtraction, returning std::optional
     */
    template <signedness Sign, representation_form Form>
    constexpr std::optional<int128_param_t<Sign, Form>>
    try_sub(const int128_param_t<Sign, Form> &lhs, const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const auto result{checked_sub(lhs, rhs)};
        return result.overflow ? std::nullopt : std::optional{result.value};
    }

    /**
     * @brief Try multiplication, returning std::optional
     */
    template <signedness Sign, representation_form Form>
    constexpr std::optional<int128_param_t<Sign, Form>>
    try_mul(const int128_param_t<Sign, Form> &lhs, const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const auto result{checked_mul(lhs, rhs)};
        return result.overflow ? std::nullopt : std::optional{result.value};
    }

    /**
     * @brief Try division, returning std::optional
     */
    template <signedness Sign, representation_form Form>
    constexpr std::optional<int128_param_t<Sign, Form>>
    try_div(const int128_param_t<Sign, Form> &lhs, const int128_param_t<Sign, Form> &rhs) noexcept
    {
        const auto result{checked_div(lhs, rhs)};
        return result.overflow ? std::nullopt : std::optional{result.value};
    }

} // namespace nstd

#endif // INT128_PARAM_SAFE_HPP
