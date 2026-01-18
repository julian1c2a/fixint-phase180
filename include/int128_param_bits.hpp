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
// @file       int128_param_bits.hpp
// @brief      Bit manipulation functions for int128_param_t<S, F>
// @author     Julián Calderón Almendros
// @date       2026-01-18 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_BITS_HPP
#define INT128_PARAM_BITS_HPP

#include "int128_parameterized.hpp"
#include <bitset>
#include <type_traits>

namespace nstd
{
    // ========================================================================
    // BIT COUNTING FUNCTIONS (Representation-Aware)
    // ========================================================================

    /**
     * @brief Count number of set bits (population count)
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to examine
     * @return Number of set bits (0-128 for TC, 0-127 for MS, 0-128 for EK)
     *
     * @details Behavior varies by representation:
     * - **Two's Complement**: Counts all 128 bits
     * - **Magnitude-Sign**: Counts only magnitude bits (127 bits, excludes sign)
     * - **Excess-K**: Counts all 128 bits of stored value
     *
     * @note For MS, the sign bit is excluded from the count
     * @note For EK, this counts bits in stored representation (not real value)
     */
    template <signedness S, representation_form F>
    inline constexpr int popcount(const int128_param_t<S, F> &value) noexcept
    {
        if constexpr (F == representation_form::magnitude_sign)
        {
            // MS: Count only magnitude bits (127 bits)
            const std::uint64_t magnitude_mask = (1ULL << 63) - 1;
            const int high_count = __builtin_popcountll(value.high() & magnitude_mask);
            const int low_count = __builtin_popcountll(value.low());
            return high_count + low_count;
        }
        else
        {
            // TC and EK: Count all 128 bits
            return __builtin_popcountll(value.high()) + __builtin_popcountll(value.low());
        }
    }

    /**
     * @brief Count leading zero bits from MSB
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to examine
     * @return Number of leading zeros (0-128 for TC, 0-127 for MS, 0-128 for EK)
     *
     * @details Behavior varies by representation:
     * - **Two's Complement signed**: Returns 0 for negative numbers (MSB=1)
     * - **Magnitude-Sign**: Counts leading zeros in 127-bit magnitude
     * - **Excess-K**: Counts leading zeros in stored value
     *
     * @note Returns 128 (or 127 for MS) if value is zero
     */
    template <signedness S, representation_form F>
    inline constexpr int countl_zero(const int128_param_t<S, F> &value) noexcept
    {
        if constexpr (F == representation_form::magnitude_sign)
        {
            // MS: Count in 127-bit magnitude space
            const std::uint64_t magnitude_mask = (1ULL << 63) - 1;
            const std::uint64_t high_mag = value.high() & magnitude_mask;

            if (high_mag != 0)
            {
                return __builtin_clzll(high_mag) - 1; // -1 because only 63 bits used
            }
            else if (value.low() != 0)
            {
                return 63 + __builtin_clzll(value.low());
            }
            else
            {
                return 127; // All magnitude bits are zero
            }
        }
        else if constexpr (F == representation_form::twos_complement && S == signedness::signed_type)
        {
            // TC signed: Return 0 for negative numbers
            if (value.is_negative())
            {
                return 0;
            }

            if (value.high() != 0)
            {
                return __builtin_clzll(value.high());
            }
            else if (value.low() != 0)
            {
                return 64 + __builtin_clzll(value.low());
            }
            else
            {
                return 128;
            }
        }
        else
        {
            // TC unsigned, EK: Standard 128-bit count
            if (value.high() != 0)
            {
                return __builtin_clzll(value.high());
            }
            else if (value.low() != 0)
            {
                return 64 + __builtin_clzll(value.low());
            }
            else
            {
                return 128;
            }
        }
    }

    /**
     * @brief Count trailing zero bits from LSB
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to examine
     * @return Number of trailing zeros (0-128 for TC/EK, 0-127 for MS)
     *
     * @details Behavior varies by representation:
     * - **Two's Complement**: Counts trailing zeros in all 128 bits
     * - **Magnitude-Sign**: Counts trailing zeros in 127-bit magnitude
     * - **Excess-K**: Counts trailing zeros in stored value
     *
     * @note Returns 128 (or 127 for MS) if value is zero
     */
    template <signedness S, representation_form F>
    inline constexpr int countr_zero(const int128_param_t<S, F> &value) noexcept
    {
        if constexpr (F == representation_form::magnitude_sign)
        {
            // MS: Count in 127-bit magnitude space
            if (value.low() != 0)
            {
                return __builtin_ctzll(value.low());
            }

            const std::uint64_t magnitude_mask = (1ULL << 63) - 1;
            const std::uint64_t high_mag = value.high() & magnitude_mask;

            if (high_mag != 0)
            {
                return 64 + __builtin_ctzll(high_mag);
            }
            else
            {
                return 127; // All magnitude bits are zero
            }
        }
        else
        {
            // TC and EK: Standard 128-bit count
            if (value.low() != 0)
            {
                return __builtin_ctzll(value.low());
            }
            else if (value.high() != 0)
            {
                return 64 + __builtin_ctzll(value.high());
            }
            else
            {
                return 128;
            }
        }
    }

    /**
     * @brief Get position of highest set bit (1-based)
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to examine
     * @return Position of highest bit (1-128 for TC/EK, 1-127 for MS), 0 if zero
     *
     * @details Bit width = total_bits - leading_zeros
     * - **Two's Complement**: 128 - countl_zero(value)
     * - **Magnitude-Sign**: 127 - countl_zero(value)
     * - **Excess-K**: 128 - countl_zero(value)
     */
    template <signedness S, representation_form F>
    inline constexpr int bit_width(const int128_param_t<S, F> &value) noexcept
    {
        if constexpr (F == representation_form::magnitude_sign)
        {
            return 127 - countl_zero(value);
        }
        else
        {
            return 128 - countl_zero(value);
        }
    }

    /**
     * @brief Check if value is a power of 2 (exactly one bit set)
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to examine
     * @return true if exactly one bit is set, false otherwise
     *
     * @details Uses algorithm: (x & (x-1)) == 0 for non-zero x
     * - **Two's Complement**: Standard check on all 128 bits
     * - **Magnitude-Sign**: Checks magnitude only (negative powers of 2 return false)
     * - **Excess-K**: ⚠️ Checks stored value (NOT real value)
     *
     * @note Returns false for zero
     * @note For EK: Result is meaningless - use conversion to TC first
     */
    template <signedness S, representation_form F>
    inline constexpr bool is_power_of_2(const int128_param_t<S, F> &value) noexcept
    {
        if (value.is_zero())
        {
            return false;
        }

        if constexpr (F == representation_form::magnitude_sign)
        {
            // MS: Check magnitude only
            const auto mag = value.magnitude();
            return popcount(mag) == 1;
        }
        else
        {
            // TC and EK: Use bit trick
            // Note: For EK, this checks stored value (not semantically meaningful)
            const auto prev = value - int128_param_t<S, F>{0, 1};
            const auto result = value & prev;
            return result.is_zero();
        }
    }

    // ========================================================================
    // BIT ROTATION FUNCTIONS (Representation-Aware)
    // ========================================================================

    /**
     * @brief Rotate bits left (circular shift)
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to rotate
     * @param shift Number of positions to rotate (normalized to 0-127/128)
     * @return Rotated value
     *
     * @details Behavior varies by representation:
     * - **Two's Complement**: Rotates all 128 bits
     * - **Magnitude-Sign**: Rotates 127-bit magnitude, preserves sign bit
     * - **Excess-K**: Rotates all 128 bits of stored value
     *
     * @note Shift is automatically normalized (shift &= 127 or 128)
     * @note MS preserves sign bit across rotation
     */
    template <signedness S, representation_form F>
    inline constexpr int128_param_t<S, F> rotl(const int128_param_t<S, F> &value, int shift) noexcept
    {
        if constexpr (F == representation_form::magnitude_sign)
        {
            // MS: Rotate 127-bit magnitude, preserve sign
            shift &= 126; // Normalize to 0-126 (127-bit space)
            if (shift == 0)
                return value;

            // Extract sign and magnitude
            const bool negative = value.is_negative();
            const std::uint64_t magnitude_mask = (1ULL << 63) - 1;
            std::uint64_t high_mag = value.high() & magnitude_mask;
            std::uint64_t low = value.low();

            // Rotate magnitude
            if (shift < 64)
            {
                // Rotate within 128 bits, then mask to 127 bits
                const std::uint64_t high_rot = (high_mag << shift) | (low >> (64 - shift));
                const std::uint64_t low_rot = (low << shift) | (high_mag >> (63 - shift));

                // Reconstruct with sign bit
                int128_param_t<S, F> result;
                result.set_low(low_rot);
                result.set_high((high_rot & magnitude_mask) | (negative ? (1ULL << 63) : 0));
                return result;
            }
            else
            {
                // Cross 64-bit boundary
                shift -= 64;
                const std::uint64_t high_rot = (low << shift) | (high_mag >> (63 - shift));
                const std::uint64_t low_rot = (high_mag << shift) | (low >> (63 - shift));

                int128_param_t<S, F> result;
                result.set_low(low_rot);
                result.set_high((high_rot & magnitude_mask) | (negative ? (1ULL << 63) : 0));
                return result;
            }
        }
        else
        {
            // TC and EK: Standard 128-bit rotation
            shift &= 127;
            if (shift == 0)
                return value;

            if (shift < 64)
            {
                const std::uint64_t high_rot = (value.high() << shift) | (value.low() >> (64 - shift));
                const std::uint64_t low_rot = (value.low() << shift) | (value.high() >> (64 - shift));
                return int128_param_t<S, F>{high_rot, low_rot};
            }
            else
            {
                shift -= 64;
                const std::uint64_t high_rot = (value.low() << shift) | (value.high() >> (64 - shift));
                const std::uint64_t low_rot = (value.high() << shift) | (value.low() >> (64 - shift));
                return int128_param_t<S, F>{high_rot, low_rot};
            }
        }
    }

    /**
     * @brief Rotate bits right (circular shift)
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @param value Value to rotate
     * @param shift Number of positions to rotate
     * @return Rotated value
     *
     * @details Implemented as rotl(value, -shift)
     * @see rotl for detailed behavior
     */
    template <signedness S, representation_form F>
    inline constexpr int128_param_t<S, F> rotr(const int128_param_t<S, F> &value, int shift) noexcept
    {
        if constexpr (F == representation_form::magnitude_sign)
        {
            return rotl(value, 127 - (shift & 126));
        }
        else
        {
            return rotl(value, 128 - (shift & 127));
        }
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

#endif // INT128_PARAM_BITS_HPP
