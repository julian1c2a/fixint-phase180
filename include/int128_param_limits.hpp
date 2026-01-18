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
// @file       int128_param_limits.hpp
// @brief      Numeric limits for int128_param_t<S, F>
// @author     Julián Calderón Almendros
// @date       2026-01-19 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_LIMITS_HPP
#define INT128_PARAM_LIMITS_HPP

#include "int128_parameterized.hpp"
#include <limits>

namespace std
{
    // ========================================================================
    // NUMERIC_LIMITS SPECIALIZATION - TWO'S COMPLEMENT UNSIGNED
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_param_t<nstd::signedness::unsigned_type,
                                              nstd::representation_form::twos_complement>>
    {
    public:
        using value_type = nstd::int128_param_t<nstd::signedness::unsigned_type,
                                                nstd::representation_form::twos_complement>;

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
        static constexpr bool is_modulo = true;
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
    // NUMERIC_LIMITS SPECIALIZATION - TWO'S COMPLEMENT SIGNED
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_param_t<nstd::signedness::signed_type,
                                              nstd::representation_form::twos_complement>>
    {
    public:
        using value_type = nstd::int128_param_t<nstd::signedness::signed_type,
                                                nstd::representation_form::twos_complement>;

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
        static constexpr bool is_modulo = false;
        static constexpr int digits = 127;  // 1 bit for sign
        static constexpr int digits10 = 38; // floor(log10(2^127))
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        static constexpr value_type min() noexcept { return value_type{1ULL << 63, 0}; } // -2^127
        static constexpr value_type lowest() noexcept { return min(); }
        static constexpr value_type max() noexcept { return value_type{(1ULL << 63) - 1, ~0ULL}; } // 2^127 - 1
        static constexpr value_type epsilon() noexcept { return value_type{0, 0}; }
        static constexpr value_type round_error() noexcept { return value_type{0, 0}; }
        static constexpr value_type infinity() noexcept { return value_type{0, 0}; }
        static constexpr value_type quiet_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type signaling_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type denorm_min() noexcept { return value_type{0, 0}; }
    };

    // ========================================================================
    // NUMERIC_LIMITS SPECIALIZATION - MAGNITUDE-SIGN UNSIGNED
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_param_t<nstd::signedness::unsigned_type,
                                              nstd::representation_form::magnitude_sign>>
    {
    public:
        using value_type = nstd::int128_param_t<nstd::signedness::unsigned_type,
                                                nstd::representation_form::magnitude_sign>;

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
        static constexpr bool is_modulo = true;
        static constexpr int digits = 127; // Sign bit excluded from magnitude
        static constexpr int digits10 = 38;
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
        static constexpr value_type max() noexcept { return value_type{(1ULL << 63) - 1, ~0ULL}; } // 2^127 - 1
        static constexpr value_type epsilon() noexcept { return value_type{0, 0}; }
        static constexpr value_type round_error() noexcept { return value_type{0, 0}; }
        static constexpr value_type infinity() noexcept { return value_type{0, 0}; }
        static constexpr value_type quiet_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type signaling_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type denorm_min() noexcept { return value_type{0, 0}; }
    };

    // ========================================================================
    // NUMERIC_LIMITS SPECIALIZATION - MAGNITUDE-SIGN SIGNED
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_param_t<nstd::signedness::signed_type,
                                              nstd::representation_form::magnitude_sign>>
    {
    public:
        using value_type = nstd::int128_param_t<nstd::signedness::signed_type,
                                                nstd::representation_form::magnitude_sign>;

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
        static constexpr bool is_modulo = false;
        static constexpr int digits = 127; // Magnitude bits only
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        // MS has symmetric range: -(2^127 - 1) to +(2^127 - 1)
        static constexpr value_type min() noexcept { return value_type{(1ULL << 63) | ((1ULL << 63) - 1), ~0ULL}; } // -(2^127 - 1)
        static constexpr value_type lowest() noexcept { return min(); }
        static constexpr value_type max() noexcept { return value_type{(1ULL << 63) - 1, ~0ULL}; } // +(2^127 - 1)
        static constexpr value_type epsilon() noexcept { return value_type{0, 0}; }
        static constexpr value_type round_error() noexcept { return value_type{0, 0}; }
        static constexpr value_type infinity() noexcept { return value_type{0, 0}; }
        static constexpr value_type quiet_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type signaling_NaN() noexcept { return value_type{0, 0}; }
        static constexpr value_type denorm_min() noexcept { return value_type{0, 0}; }
    };

    // ========================================================================
    // NUMERIC_LIMITS SPECIALIZATION - EXCESS-K UNSIGNED
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_param_t<nstd::signedness::unsigned_type,
                                              nstd::representation_form::excess_k>>
    {
    public:
        using value_type = nstd::int128_param_t<nstd::signedness::unsigned_type,
                                                nstd::representation_form::excess_k>;

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
        static constexpr bool is_modulo = true;
        static constexpr int digits = 128;
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        // EK unsigned: real_value = stored_value - bias
        // min = 0 - bias = -bias (stored as 0)
        // max = 2^128 - 1 - bias (stored as all 1s)
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
    // NUMERIC_LIMITS SPECIALIZATION - EXCESS-K SIGNED
    // ========================================================================

    template <>
    class numeric_limits<nstd::int128_param_t<nstd::signedness::signed_type,
                                              nstd::representation_form::excess_k>>
    {
    public:
        using value_type = nstd::int128_param_t<nstd::signedness::signed_type,
                                                nstd::representation_form::excess_k>;

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
        static constexpr bool is_modulo = false;
        static constexpr int digits = 127; // Effective signed range
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        // EK signed with bias = 2^126:
        // min = -2^126 (stored as 0)
        // max = 2^127 - 2^126 - 1 (stored as all 1s)
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

} // namespace std

#endif // INT128_PARAM_LIMITS_HPP
