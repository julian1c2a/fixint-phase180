// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - Representation Forms (Phase 1.75)
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
// @file       representation.hpp
// @brief      Representation forms for 128-bit integers (Phase 1.75)
// @author     Julián Calderón Almendros
// @date       2026-01-11
// @version    1.0.0
// =============================================================================

#ifndef INT128_REPRESENTATION_HPP
#define INT128_REPRESENTATION_HPP

#include <cstdint>
#include <limits>
#include <type_traits>

namespace nstd
{

    // =============================================================================
    // Representation Forms Enumeration
    // =============================================================================

    /**
     * @enum representation_form
     * @brief Enumeration for different binary representations of signed integers
     *
     * @details Specifies how the sign and magnitude are encoded in the binary
     * representation of the integer. This allows the same underlying storage to
     * be interpreted in different ways (two's complement, magnitude-sign, etc.)
     */
    /**
     * @brief Formas de representación para int128_param_t
     * - binnat: binario natural (solo unsigned, sin signo, sin codificación especial)
     * - twos_complement: complemento a dos (solo signed)
     * - magnitude_sign: magnitud y signo (solo signed)
     * - excess_k: exceso-K (solo signed)
     */
    enum class representation_form : std::uint8_t
    {
        /**
         * @brief Binario natural (solo unsigned)
         *
         * No hay codificación de signo ni bias. Simplemente almacena el valor binario puro.
         * Range: [0, 2^128-1]
         * Ejemplo: 42 = 0x2A
         */
        binnat = 0,

        /**
         * @brief Two's Complement (Standard, Phase 1.66)
         *
         * Sign bit at MSB with inversion of remaining bits.
         * Range: [-2^127, 2^127-1]
         * Operations: Optimized via hardware intrinsics
         *
         * Encoding:
         * - Positive: MSB=0, rest=magnitude
         * - Negative: MSB=1, rest=inverted(|magnitude|)+1
         *
         * Example: -1 = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
         */
        twos_complement = 1,

        /**
         * @brief Magnitude-Sign (Phase 1.75 Investigation)
         *
         * Separate sign bit and magnitude representation.
         * Range: [-2^127+1, 2^127-1] (note: -0 differs from +0)
         * Operations: Requires explicit sign handling
         *
         * Encoding:
         * - Sign bit (MSB): 0=positive, 1=negative
         * - Remaining 127 bits: Magnitude (unsigned)
         *
         * Example: -42 = [1|0000...0101010] (sign=1, magnitude=42)
         */
        magnitude_sign = 2,

        /**
         * @brief Excess-k (Bias Notation, Future - Phase 1.75)
         *
         * Used primarily for exponents in IEEE 754 floating point.
         * Range: [-k, 2^127-k]
         * Operations: Add/subtract constant k
         *
         * Encoding:
         * - Value = stored_value - bias_k
         * - Typically used with k=2^(n-1)
         *
         * Example (k=64): stored=100 represents value=36
         */
        excess_k = 3
    };

    // =============================================================================
    // Signedness Enumeration (from Phase 1.66)
    // =============================================================================

    /**
     * @enum signedness
     * @brief Enumeration for signed vs unsigned interpretation
     *
     * @details This is orthogonal to representation_form.
     * For example, both can be combined:
     * - int128_t: signedness::signed_type + representation_form::twos_complement
     * - uint128_t: signedness::unsigned_type + any representation_form
     */
    enum class signedness : bool
    {
        unsigned_type = false, ///< Unsigned: [0, 2^128-1]
        signed_type = true     ///< Signed: dependent on representation_form
    };

    // =============================================================================
    // Representation Form Traits (Metaprogramming)
    // =============================================================================

    /**
     * @struct representation_traits
     * @brief Compile-time traits for representation forms
     *
     * @details Provides static information about each representation form
     * for metaprogramming and optimization decisions.
     *
     * @tparam Form The representation form to query
     */
    template <representation_form Form>
    struct representation_traits;

    /**
     * @brief Specialization for Two's Complement
     */
    template <>
    struct representation_traits<representation_form::twos_complement>
    {
        /// @brief Human-readable name
        static constexpr const char *name = "Two's Complement";

        /// @brief Whether sign bit is implicit (MSB)
        static constexpr bool has_implicit_sign_bit = true;

        /// @brief Whether magnitude needs inversion for negatives
        static constexpr bool uses_inversion = true;

        /// @brief Hardware support (intrinsics available)
        static constexpr bool hardware_optimized = true;

        /// @brief Minimum value for signed interpretation
        static constexpr std::int64_t min_i64 = std::numeric_limits<std::int64_t>::min();

        /// @brief Maximum value for signed interpretation
        static constexpr std::int64_t max_i64 = std::numeric_limits<std::int64_t>::max();
    };

    /**
     * @brief Specialization for Magnitude-Sign
     */
    template <>
    struct representation_traits<representation_form::magnitude_sign>
    {
        /// @brief Human-readable name
        static constexpr const char *name = "Magnitude-Sign";

        /// @brief Whether sign bit is implicit (MSB)
        static constexpr bool has_implicit_sign_bit = true;

        /// @brief Whether magnitude needs inversion for negatives
        static constexpr bool uses_inversion = false; // Direct magnitude

        /// @brief Hardware support
        static constexpr bool hardware_optimized = false; // Requires software

        /// @brief Note: Has both +0 and -0
        static constexpr bool has_two_zeros = true;

        /// @brief Minimum value (excluding -0 duplicate)
        static constexpr std::int64_t min_i64 = -(std::numeric_limits<std::int64_t>::max());

        /// @brief Maximum value
        static constexpr std::int64_t max_i64 = std::numeric_limits<std::int64_t>::max();
    };

    /**
     * @brief Specialization for Excess-k (Bias)
     */
    template <>
    struct representation_traits<representation_form::excess_k>
    {
        /// @brief Human-readable name
        static constexpr const char *name = "Excess-k (Bias)";

        /// @brief Sign bit representation
        static constexpr bool has_implicit_sign_bit = false; // Uses offset

        /// @brief Uses constant offset (bias)
        static constexpr bool uses_bias = true;

        /// @brief Typical bias for 128-bit
        static constexpr std::uint64_t default_bias = (1ULL << 63) * 2ULL; // 2^126

        /// @brief Hardware support
        static constexpr bool hardware_optimized = false; // Custom bias handling
    };

    // =============================================================================
    // Conversion Functions between Representations
    // =============================================================================

    /**
     * @brief Convert magnitude-sign to two's complement
     *
     * @param magnitude_sign_value Value in magnitude-sign representation
     * @return Equivalent value in two's complement
     */
    inline constexpr std::uint64_t ms_to_twos_complement(std::uint64_t magnitude_sign_value) noexcept
    {
        const bool is_negative = (magnitude_sign_value & (1ULL << 63)) != 0;
        const std::uint64_t magnitude = magnitude_sign_value & ~(1ULL << 63);

        if (!is_negative)
        {
            return magnitude;
        }

        // Two's complement: invert and add 1
        return (~magnitude) + 1;
    }

    /**
     * @brief Convert two's complement to magnitude-sign
     *
     * @param twos_complement_value Value in two's complement
     * @return Equivalent value in magnitude-sign
     */
    inline constexpr std::uint64_t twos_complement_to_ms(std::uint64_t twos_complement_value) noexcept
    {
        const bool is_negative = (twos_complement_value & (1ULL << 63)) != 0;

        if (!is_negative)
        {
            return twos_complement_value;
        }

        // Extract magnitude: negate the two's complement value
        std::uint64_t magnitude = (~twos_complement_value) + 1;

        // Set sign bit
        return magnitude | (1ULL << 63);
    }

    // =============================================================================
    // 128-bit Conversion Functions Between Representations
    // =============================================================================

    // TC <-> MS (128 bits)
    inline constexpr void ms128_to_twos_complement(uint64_t ms_high, uint64_t ms_low, uint64_t &tc_high, uint64_t &tc_low) noexcept
    {
        // MS: sign bit is MSB of high
        const bool is_negative = (ms_high & (1ULL << 63)) != 0;
        const uint64_t mag_high = ms_high & ~(1ULL << 63);
        if (!is_negative)
        {
            tc_high = mag_high;
            tc_low = ms_low;
        }
        else
        {
            // Two's complement: invert and add 1 (128 bits)
            tc_high = ~mag_high;
            tc_low = ~ms_low;
            // Add 1 (handle carry)
            if (++tc_low == 0)
                ++tc_high;
        }
    }

    inline constexpr void twos_complement128_to_ms(uint64_t tc_high, uint64_t tc_low, uint64_t &ms_high, uint64_t &ms_low) noexcept
    {
        const bool is_negative = (tc_high & (1ULL << 63)) != 0;
        if (!is_negative)
        {
            ms_high = tc_high;
            ms_low = tc_low;
        }
        else
        {
            // Negate (128 bits)
            uint64_t mag_low = ~tc_low + 1;
            uint64_t mag_high = ~tc_high + (mag_low == 0 ? 1 : 0);
            // Set sign bit
            ms_high = (mag_high & ~(1ULL << 63)) | (1ULL << 63);
            ms_low = mag_low;
        }
    }

    // TC <-> EK (128 bits)
    inline constexpr void twos_complement128_to_excess_k(uint64_t tc_high, uint64_t tc_low, uint64_t &ek_high, uint64_t &ek_low) noexcept
    {
        // EK: stored = value + bias
        constexpr uint64_t bias_high = (1ULL << 62);
        constexpr uint64_t bias_low = 0;
        // Add bias (128 bits)
        ek_low = tc_low + bias_low;
        ek_high = tc_high + bias_high + (ek_low < tc_low ? 1 : 0);
    }

    inline constexpr void excess_k128_to_twos_complement(uint64_t ek_high, uint64_t ek_low, uint64_t &tc_high, uint64_t &tc_low) noexcept
    {
        constexpr uint64_t bias_high = (1ULL << 62);
        constexpr uint64_t bias_low = 0;
        // Subtract bias (128 bits)
        tc_low = ek_low - bias_low;
        tc_high = ek_high - bias_high - (ek_low < bias_low ? 1 : 0);
    }

    // MS <-> EK (128 bits)
    inline constexpr void ms128_to_excess_k(uint64_t ms_high, uint64_t ms_low, uint64_t &ek_high, uint64_t &ek_low) noexcept
    {
        uint64_t tc_high, tc_low;
        ms128_to_twos_complement(ms_high, ms_low, tc_high, tc_low);
        twos_complement128_to_excess_k(tc_high, tc_low, ek_high, ek_low);
    }

    inline constexpr void excess_k128_to_ms(uint64_t ek_high, uint64_t ek_low, uint64_t &ms_high, uint64_t &ms_low) noexcept
    {
        uint64_t tc_high, tc_low;
        excess_k128_to_twos_complement(ek_high, ek_low, tc_high, tc_low);
        twos_complement128_to_ms(tc_high, tc_low, ms_high, ms_low);
    }

} // namespace nstd

#endif // INT128_REPRESENTATION_HPP
