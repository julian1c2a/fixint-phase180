// =============================================================================
// int128 Library - Numeric Limits Specialization (Phase 1.75)
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
// @file       int128_param_limits.hpp
// @brief      std::numeric_limits for parameterized 128-bit types (4 valid types)
// @author     Julián Calderón Almendros
// @date       2026-02-04 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_LIMITS_HPP
#define INT128_PARAM_LIMITS_HPP

#include "int128_parameterized.hpp"
#include <limits>

namespace std
{
    // ========================================================================
    // 1. BINNAT (unsigned binary natural)
    // ========================================================================

    template <>
    class numeric_limits<nstd::uint128_t>
    {
    public:
        using value_type = nstd::uint128_t;

        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = false;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = true; // Unsigned wraps
        static constexpr int digits = 128;
        static constexpr int digits10 = 38; // floor(log10(2^128))
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        static constexpr value_type min() noexcept { return value_type{0, 0}; }
        static constexpr value_type lowest() noexcept { return min(); }
        static constexpr value_type max() noexcept { return value_type{~0ULL, ~0ULL}; }
        static constexpr value_type epsilon() noexcept { return value_type{0, 0}; }
        static constexpr value_type round_error() noexcept { return value_type{0, 0}; }
        static constexpr value_type infinity() noexcept { return value_type{0, 0}; }
        static constexpr value_type quiet_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type signaling_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type denorm_min() noexcept { return value_type{0, 0}; }
    };

    // ========================================================================
    // 2. TWO'S COMPLEMENT (signed)
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_tc_t>
    {
    public:
        using value_type = nstd::int128_tc_t;

        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false; // Signed overflow is UB
        static constexpr int digits = 127;       // Excluding sign bit
        static constexpr int digits10 = 38;      // floor(log10(2^127))
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        static constexpr value_type min() noexcept
        {
            return value_type{1ULL << 63, 0}; // -2^127
        }

        static constexpr value_type lowest() noexcept { return min(); }

        static constexpr value_type max() noexcept
        {
            return value_type{(1ULL << 63) - 1, ~0ULL}; // 2^127 - 1
        }

        static constexpr value_type epsilon() noexcept { return value_type{0, 0}; }
        static constexpr value_type round_error() noexcept { return value_type{0, 0}; }
        static constexpr value_type infinity() noexcept { return value_type{0, 0}; }
        static constexpr value_type quiet_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type signaling_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type denorm_min() noexcept { return value_type{0, 0}; }
    };

    // ========================================================================
    // 3. MAGNITUDE-SIGN (signed)
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_ms_t>
    {
    public:
        using value_type = nstd::int128_ms_t;

        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false; // Signed overflow is UB
        static constexpr int digits = 127;       // Magnitude bits (excluding sign bit)
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        static constexpr value_type min() noexcept
        {
            // MS min = -(2^127 - 1) with sign bit set
            return value_type{(1ULL << 63) | ((1ULL << 63) - 1), ~0ULL};
        }

        static constexpr value_type lowest() noexcept { return min(); }

        static constexpr value_type max() noexcept
        {
            // MS max = +(2^127 - 1) with sign bit clear
            return value_type{(1ULL << 63) - 1, ~0ULL};
        }

        static constexpr value_type epsilon() noexcept { return value_type{0, 0}; }
        static constexpr value_type round_error() noexcept { return value_type{0, 0}; }
        static constexpr value_type infinity() noexcept { return value_type{0, 0}; }
        static constexpr value_type quiet_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type signaling_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type denorm_min() noexcept { return value_type{0, 0}; }
    };

    // ========================================================================
    // 4. EXCESS-K (signed)
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_ek_t>
    {
    public:
        using value_type = nstd::int128_ek_t;

        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false; // Signed overflow is UB
        static constexpr int digits = 126;       // max = 2^126-1 (bias K=2^126)
        static constexpr int digits10 = 37;      // floor(126 * log10(2))
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        static constexpr value_type min() noexcept
        {
            // EK min = -2^126 (stored as 0, since K = 2^126)
            // Real value -2^126 + K = 0
            return value_type{0, 0};
        }

        static constexpr value_type lowest() noexcept { return min(); }

        static constexpr value_type max() noexcept
        {
            // EK max = 2^126 - 1 (real value)
            // Stored value = (2^126 - 1) + K = 2^127 - 1
            return value_type{(1ULL << 63) - 1, ~0ULL};
        }

        static constexpr value_type epsilon() noexcept { return value_type{0, 0}; }
        static constexpr value_type round_error() noexcept { return value_type{0, 0}; }
        static constexpr value_type infinity() noexcept { return value_type{0, 0}; }
        static constexpr value_type quiet_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type signaling_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type denorm_min() noexcept { return value_type{0, 0}; }
    };

} // namespace std

#endif // INT128_PARAM_LIMITS_HPP
