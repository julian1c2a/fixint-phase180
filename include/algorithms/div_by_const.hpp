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
// @file       algorithms/div_by_const.hpp
// @brief      Granlund-Montgomery division by compile-time constants
// @author     Julián Calderón Almendros
// @date       2025-07-23
// @version    1.0.0
// =============================================================================

#pragma once

#include "karatsuba.hpp"

#include <cstdint>
#include <utility>

namespace nstd {
namespace algorithms {

using std::uint64_t;

// =============================================================================
// mulhi_128 — High 128 bits of 128×128→256 multiplication
// =============================================================================

/**
 * @brief Compute the upper 128 bits of a full 128×128 multiplication.
 *
 * @details Equivalent to: (a * b) >> 128, where the product is computed
 * in 256-bit precision. This is the fundamental operation for
 * Granlund-Montgomery division by constants.
 *
 * @param a First 128-bit operand
 * @param b Second 128-bit operand
 * @return Upper 128 bits of the 256-bit product
 */
inline uint128_t mulhi_128(const uint128_t& a, const uint128_t& b) noexcept {
    return schoolbook_full_mul(a, b).high128();
}

// =============================================================================
// Granlund-Montgomery Division by 10
// =============================================================================
//
// For unsigned 128-bit division by 10:
//   M = ceil(2^131 / 10) = 0xCCCCCCCCCCCCCCCC_CCCCCCCCCCCCCCCD
//   S = 3
//   q = mulhi(n, M) >> S
//
// This replaces expensive 128-bit software division with a single
// widening multiply + shift, providing ~2-5x speedup.
// Primary use case: to_string() decimal conversion.

/**
 * @brief Fast unsigned 128-bit division by 10.
 *
 * @details Uses Granlund-Montgomery reciprocal multiplication:
 *   q = mulhi(n, M) >> 3
 * where M = ceil(2^131 / 10).
 *
 * @param n Dividend (128-bit unsigned)
 * @return floor(n / 10)
 */
inline uint128_t fast_div10(const uint128_t& n) noexcept {
    const uint128_t M{0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCDull};
    return mulhi_128(n, M) >> 3;
}

/**
 * @brief Fast unsigned 128-bit modulo 10.
 *
 * @param n Dividend (128-bit unsigned)
 * @return n mod 10 (always 0..9)
 */
inline uint64_t fast_mod10(const uint128_t& n) noexcept {
    const auto q{fast_div10(n)};
    return (n - q * uint128_t{10}).low();
}

/**
 * @brief Fast unsigned 128-bit division and modulo by 10.
 *
 * @param n Dividend (128-bit unsigned)
 * @return {floor(n/10), n mod 10}
 */
inline std::pair<uint128_t, uint64_t> fast_divmod10(const uint128_t& n) noexcept {
    const auto q{fast_div10(n)};
    const uint64_t r{(n - q * uint128_t{10}).low()};
    return {q, r};
}

} // namespace algorithms
} // namespace nstd
