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
// @file       int128_param_arithmetic.hpp
// @brief      Extended arithmetic operations: widening multiply, mulhi
// @author     Julián Calderón Almendros
// @date       2026-03-21 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_ARITHMETIC_HPP
#define INT128_PARAM_ARITHMETIC_HPP

#include "algorithms/karatsuba.hpp"

namespace nstd
{
    // ========================================================================
    // uint256_t — Public alias for 256-bit results
    // ========================================================================

    using uint256_t = algorithms::uint256_result;

    // ========================================================================
    // widening_mul — Full 128×128→256 multiplication (Karatsuba)
    // ========================================================================

    /**
     * @brief Compute the full 256-bit product of two 128-bit unsigned values.
     *
     * @details Uses the Karatsuba sub-quadratic algorithm (3 multiplications
     * instead of the schoolbook 4). Returns the complete 256-bit result
     * stored in a uint256_t (4 little-endian 64-bit limbs).
     *
     * @param a First 128-bit operand
     * @param b Second 128-bit operand
     * @return Full 256-bit product
     *
     * @code
     * const uint128_t a{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
     * const uint128_t b{2, 0};
     * const auto result{nstd::widening_mul(a, b)};
     * // result.low128()  == MAX-1  (lower 128 bits)
     * // result.high128() == 1      (upper 128 bits)
     * @endcode
     */
    inline uint256_t widening_mul(const uint128_t &a, const uint128_t &b) noexcept
    {
        return algorithms::karatsuba_full_mul(a, b);
    }

    // ========================================================================
    // mulhi — Upper 128 bits of 128×128→256 multiplication
    // ========================================================================

    /**
     * @brief Compute the upper 128 bits of a 128×128 multiplication.
     *
     * @details Equivalent to (a * b) >> 128 where the product is computed
     * in full 256-bit precision. Essential for Granlund-Montgomery
     * division by constants and other fixed-point arithmetic.
     *
     * @param a First 128-bit operand
     * @param b Second 128-bit operand
     * @return Upper 128 bits of the 256-bit product
     *
     * @code
     * const uint128_t a{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
     * const uint128_t b{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
     * const auto hi{nstd::mulhi(a, b)};
     * // hi == MAX - 1 (upper half of MAX*MAX)
     * @endcode
     */
    inline uint128_t mulhi(const uint128_t &a, const uint128_t &b) noexcept
    {
        return algorithms::karatsuba_full_mul(a, b).high128();
    }

    // ========================================================================
    // mullo — Lower 128 bits (same as operator*, for completeness)
    // ========================================================================

    /**
     * @brief Compute the lower 128 bits of a 128×128 multiplication.
     *
     * @details Equivalent to (a * b) & ((1<<128)-1). This is the same as
     * operator*, provided for API symmetry with mulhi().
     *
     * @param a First 128-bit operand
     * @param b Second 128-bit operand
     * @return Lower 128 bits of the 256-bit product
     */
    inline uint128_t mullo(const uint128_t &a, const uint128_t &b) noexcept
    {
        return a * b;
    }

} // namespace nstd

#endif // INT128_PARAM_ARITHMETIC_HPP
