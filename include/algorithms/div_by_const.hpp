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

#ifndef INT128_ALGORITHMS_DIV_BY_CONST_HPP
#define INT128_ALGORITHMS_DIV_BY_CONST_HPP

#include "karatsuba.hpp"

#include <cstdint>
#include <utility>

namespace nstd
{
    namespace algorithms
    {

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
        inline uint128_t mulhi_128(const uint128_t &a, const uint128_t &b) noexcept
        {
            return karatsuba_full_mul(a, b).high128();
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
        inline uint128_t fast_div10(const uint128_t &n) noexcept
        {
            const uint128_t M{0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCDull};
            return mulhi_128(n, M) >> 3;
        }

        /**
         * @brief Fast unsigned 128-bit modulo 10.
         *
         * @param n Dividend (128-bit unsigned)
         * @return n mod 10 (always 0..9)
         */
        inline uint64_t fast_mod10(const uint128_t &n) noexcept
        {
            const auto q{fast_div10(n)};
            return (n - q * uint128_t{10}).low();
        }

        /**
         * @brief Fast unsigned 128-bit division and modulo by 10.
         *
         * @param n Dividend (128-bit unsigned)
         * @return {floor(n/10), n mod 10}
         */
        inline std::pair<uint128_t, uint64_t> fast_divmod10(const uint128_t &n) noexcept
        {
            const auto q{fast_div10(n)};
            const uint64_t r{(n - q * uint128_t{10}).low()};
            return {q, r};
        }

        // =============================================================================
        // Overflow-corrected Granlund-Montgomery helper
        // =============================================================================
        //
        // For divisors where M = ceil(2^(128+S)/d) overflows 128 bits (needs 129 bits),
        // the correction formula from Hacker's Delight (Section 10-9) is:
        //   t = mulhi(n, M_low)
        //   t = t + ((n - t) >> 1)
        //   q = t >> (S - 1)
        //
        // where M_low = M_full & (2^128 - 1) is the lower 128 bits of the 129-bit M.

        /**
         * @brief Overflow-corrected Granlund-Montgomery division.
         *
         * @param n Dividend (128-bit unsigned)
         * @param M_low Lower 128 bits of the 129-bit magic number
         * @param shift_minus_1 S - 1, where S = ceil(log2(d))
         * @return floor(n / d)
         */
        inline uint128_t gm_div_overflow(const uint128_t &n, const uint128_t &M_low,
                                         int shift_minus_1) noexcept
        {
            const auto t{mulhi_128(n, M_low)};
            return (t + ((n - t) >> 1)) >> shift_minus_1;
        }

        // =============================================================================
        // Division by 3
        // =============================================================================
        //
        // Simple method: M = ceil(2^129 / 3) = 0xAAAAAAAAAAAAAAAA_AAAAAAAAAAAAAAAB
        // S = 1, q = mulhi(n, M) >> 1

        inline uint128_t fast_div3(const uint128_t &n) noexcept
        {
            const uint128_t M{0xAAAAAAAAAAAAAAAAull, 0xAAAAAAAAAAAAAAABull};
            return mulhi_128(n, M) >> 1;
        }

        inline uint64_t fast_mod3(const uint128_t &n) noexcept
        {
            return (n - fast_div3(n) * uint128_t{3}).low();
        }

        inline std::pair<uint128_t, uint64_t> fast_divmod3(const uint128_t &n) noexcept
        {
            const auto q{fast_div3(n)};
            return {q, (n - q * uint128_t{3}).low()};
        }

        // =============================================================================
        // Division by 5
        // =============================================================================
        //
        // Overflow method: M_low = 0x9999999999999999_999999999999999A, S = 3
        // q = gm_div_overflow(n, M_low, 2)

        inline uint128_t fast_div5(const uint128_t &n) noexcept
        {
            const uint128_t M_low{0x9999999999999999ull, 0x999999999999999Aull};
            return gm_div_overflow(n, M_low, 2);
        }

        inline uint64_t fast_mod5(const uint128_t &n) noexcept
        {
            return (n - fast_div5(n) * uint128_t{5}).low();
        }

        inline std::pair<uint128_t, uint64_t> fast_divmod5(const uint128_t &n) noexcept
        {
            const auto q{fast_div5(n)};
            return {q, (n - q * uint128_t{5}).low()};
        }

        // =============================================================================
        // Division by 7
        // =============================================================================
        //
        // Overflow method: M_low = 0x2492492492492492_4924924924924925, S = 3
        // q = gm_div_overflow(n, M_low, 2)

        inline uint128_t fast_div7(const uint128_t &n) noexcept
        {
            const uint128_t M_low{0x2492492492492492ull, 0x4924924924924925ull};
            return gm_div_overflow(n, M_low, 2);
        }

        inline uint64_t fast_mod7(const uint128_t &n) noexcept
        {
            return (n - fast_div7(n) * uint128_t{7}).low();
        }

        inline std::pair<uint128_t, uint64_t> fast_divmod7(const uint128_t &n) noexcept
        {
            const auto q{fast_div7(n)};
            return {q, (n - q * uint128_t{7}).low()};
        }

        // =============================================================================
        // Division by 9
        // =============================================================================
        //
        // Simple method: M = ceil(2^129 / 9) = 0x38E38E38E38E38E3_8E38E38E38E38E39
        // S = 1, q = mulhi(n, M) >> 1

        inline uint128_t fast_div9(const uint128_t &n) noexcept
        {
            const uint128_t M{0x38E38E38E38E38E3ull, 0x8E38E38E38E38E39ull};
            return mulhi_128(n, M) >> 1;
        }

        inline uint64_t fast_mod9(const uint128_t &n) noexcept
        {
            return (n - fast_div9(n) * uint128_t{9}).low();
        }

        inline std::pair<uint128_t, uint64_t> fast_divmod9(const uint128_t &n) noexcept
        {
            const auto q{fast_div9(n)};
            return {q, (n - q * uint128_t{9}).low()};
        }

        // =============================================================================
        // Division by 100
        // =============================================================================
        //
        // Overflow method: M_low = 0x47AE147AE147AE14_7AE147AE147AE148, S = 7
        // q = gm_div_overflow(n, M_low, 6)
        // Useful for extracting 2 decimal digits at a time.

        inline uint128_t fast_div100(const uint128_t &n) noexcept
        {
            const uint128_t M_low{0x47AE147AE147AE14ull, 0x7AE147AE147AE148ull};
            return gm_div_overflow(n, M_low, 6);
        }

        inline uint64_t fast_mod100(const uint128_t &n) noexcept
        {
            return (n - fast_div100(n) * uint128_t{100}).low();
        }

        inline std::pair<uint128_t, uint64_t> fast_divmod100(const uint128_t &n) noexcept
        {
            const auto q{fast_div100(n)};
            return {q, (n - q * uint128_t{100}).low()};
        }

        // =============================================================================
        // Division by 10^19 (10000000000000000000)
        // =============================================================================
        //
        // Overflow method: M_low = 0xD83C94FB6D2AC34A_5663D3C7A0D865CB, S = 64
        // q = gm_div_overflow(n, M_low, 63)
        //
        // Primary use case: to_string() chunked conversion.
        // 10^19 < 2^64, so the quotient can be divided further with fast 64-bit ops,
        // and the remainder fits in uint64_t (up to 19 decimal digits).

        inline uint128_t fast_div_1e19(const uint128_t &n) noexcept
        {
            const uint128_t M_low{0xD83C94FB6D2AC34Aull, 0x5663D3C7A0D865CBull};
            return gm_div_overflow(n, M_low, 63);
        }

        /**
         * @brief Fast division and modulo by 10^19 = 10000000000000000000.
         *
         * @param n Dividend (128-bit unsigned)
         * @return {floor(n/10^19), n mod 10^19} where remainder fits in uint64_t
         */
        inline std::pair<uint128_t, uint64_t> fast_divmod_1e19(const uint128_t &n) noexcept
        {
            const auto q{fast_div_1e19(n)};
            const uint64_t r{(n - q * uint128_t{10000000000000000000ull}).low()};
            return {q, r};
        }

    } // namespace algorithms
} // namespace nstd

#endif // INT128_ALGORITHMS_DIV_BY_CONST_HPP
