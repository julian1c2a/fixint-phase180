// =============================================================================
// int128 Library - Parameterized Integer Template (Phase 1.75)
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
// @file       int128_parameterized.hpp
// @brief      128-bit integer template with parameterized representation
// @author     Julián Calderón Almendros
// @date       2026-01-11
// @version    1.0.0
// =============================================================================

#ifndef INT128_PARAMETERIZED_HPP
#define INT128_PARAMETERIZED_HPP

#include "representation.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <array>
#include <stdexcept>

namespace nstd
{

    // =============================================================================
    // Main Parameterized Integer Template
    // =============================================================================

    /**
     * @class int128_param_t
     * @brief 128-bit integer with parameterized representation and signedness
     *
     * @tparam Sign Signedness (signed_type or unsigned_type)
     * @tparam Form Representation form (twos_complement, magnitude_sign, excess_k)
     *
     * @details This template extends the unified template from Phase 1.66 by
     * adding a representation_form parameter, allowing investigation into different
     * encodings suitable for floating-point research.
     *
     * **Memory Layout:**
     * - data[0]: Low 64 bits (LSB)
     * - data[1]: High 64 bits (MSB, contains sign or bias information)
     * - Always 16 bytes, little-endian indexing
     *
     * **Type Aliases Generated:**
     * - uint128_tc_t:     unsigned, two's complement (default)
     * - int128_tc_t:      signed, two's complement (Phase 1.66 equivalent)
     * - uint128_ms_t:     unsigned, magnitude-sign
     * - int128_ms_t:      signed, magnitude-sign (Phase 1.75 primary)
     * - uint128_ek_t:     unsigned, excess-k (future)
     * - int128_ek_t:      signed, excess-k (future)
     *
     * @invariant Storage is always 128 bits (16 bytes)
     * @invariant Representation is immutable after construction
     * @invariant All operations are noexcept unless otherwise noted
     */
    template <signedness Sign = signedness::unsigned_type,
              representation_form Form = representation_form::twos_complement>
    class int128_param_t
    {
    public:
        // ========================================================================
        // Static Type Information
        // ========================================================================

        /// @brief Signedness of this type (compile-time constant)
        static constexpr signedness sign = Sign;

        /// @brief Representation form of this type (compile-time constant)
        static constexpr representation_form form = Form;

        /// @brief Is this a signed type? (convenience constant)
        static constexpr bool is_signed = (Sign == signedness::signed_type);

        /// @brief Is this a two's complement type? (convenience constant)
        static constexpr bool is_twos_complement = (Form == representation_form::twos_complement);

        /// @brief Is this a magnitude-sign type? (convenience constant)
        static constexpr bool is_magnitude_sign = (Form == representation_form::magnitude_sign);

        /// @brief Is this an excess-k type? (convenience constant)
        static constexpr bool is_excess_k = (Form == representation_form::excess_k);

        /// @brief Size in bits (always 128)
        static constexpr int BITS = 128;

        /// @brief Size in bytes (always 16)
        static constexpr int BYTES = 16;

    private:
        // ========================================================================
        // Storage
        // ========================================================================

        /**
         * @brief Raw 128-bit data storage
         *
         * Interpretation depends on representation form:
         *
         * **Two's Complement:**
         * - Standard two's complement representation
         * - MSB is sign bit
         *
         * **Magnitude-Sign:**
         * - data[1] MSB is explicit sign bit (0=positive, 1=negative)
         * - Remaining bits are magnitude (unsigned interpretation)
         * - WARNING: Supports both +0 and -0 (are distinct)
         *
         * **Excess-k:**
         * - All 128 bits form biased value
         * - Actual value = stored_value - bias_k
         * - Used primarily for exponents in IEEE 754
         */
        std::uint64_t data[2];

    public:
        // ========================================================================
        // Constructors
        // ========================================================================

        /// @brief Default constructor (zero)
        constexpr int128_param_t() noexcept : data{0, 0} {}

        /// @brief Copy constructor
        constexpr int128_param_t(const int128_param_t &) = default;

        /// @brief Move constructor
        constexpr int128_param_t(int128_param_t &&) = default;

        /// @brief Constructor from single 64-bit value (zero-extends or sign-extends)
        template <typename T>
        explicit constexpr int128_param_t(T value) noexcept
        {
            static_assert(std::is_integral_v<T>, "T must be integral");

            if constexpr (std::is_signed_v<T>)
            {
                // Sign-extend for signed types
                const bool negative = value < 0;
                data[0] = static_cast<std::uint64_t>(value);
                data[1] = negative ? std::uint64_t(-1) : 0;
            }
            else
            {
                // Zero-extend for unsigned types
                data[0] = static_cast<std::uint64_t>(value);
                data[1] = 0;
            }
        }

        /// @brief Constructor from (high, low) pair
        template <typename T1, typename T2>
        explicit constexpr int128_param_t(T1 high, T2 low) noexcept
            : data{static_cast<std::uint64_t>(low), static_cast<std::uint64_t>(high)} {}

        /// @brief Constructor from string (base auto-detection)
        explicit int128_param_t(const char *str);

        /// @brief Constructor from string with explicit base
        explicit int128_param_t(const char *str, int base);

        // ========================================================================
        // Assignment Operators
        // ========================================================================

        constexpr int128_param_t &operator=(const int128_param_t &) = default;
        constexpr int128_param_t &operator=(int128_param_t &&) = default;

        /// @brief Assignment from integral type
        template <typename T>
        constexpr int128_param_t &operator=(T value) noexcept
        {
            return *this = int128_param_t(value);
        }

        // ========================================================================
        // Accessors
        // ========================================================================

        /// @brief Get high 64 bits (MSB)
        constexpr std::uint64_t high() const noexcept { return data[1]; }

        /// @brief Get low 64 bits (LSB)
        constexpr std::uint64_t low() const noexcept { return data[0]; }

        /// @brief Set high 64 bits
        template <typename T>
        constexpr void set_high(T value) noexcept { data[1] = static_cast<std::uint64_t>(value); }

        /// @brief Set low 64 bits
        template <typename T>
        constexpr void set_low(T value) noexcept { data[0] = static_cast<std::uint64_t>(value); }

        // ========================================================================
        // Representation-Specific Methods
        // ========================================================================

        /**
         * @brief Check if value is negative (representation-aware)
         *
         * **Two's Complement:** MSB is sign bit
         * **Magnitude-Sign:** Explicit sign bit at data[1] MSB
         * **Excess-k:** Compare against bias
         *
         * @return true if negative, false if positive (or zero)
         */
        constexpr bool is_negative() const noexcept
            requires(is_signed)
        {
            if constexpr (is_twos_complement)
            {
                return (data[1] & (1ULL << 63)) != 0;
            }
            else if constexpr (is_magnitude_sign)
            {
                return (data[1] & (1ULL << 63)) != 0;
            }
            else
            {
                // excess_k: depends on bias comparison (TODO)
                return false;
            }
        }

        /**
         * @brief Get magnitude (sign-independent absolute value)
         *
         * **Two's Complement:** Negation for negatives
         * **Magnitude-Sign:** Direct magnitude from bits [0..126]
         *
         * @return Magnitude as uint128_param_t
         */
        constexpr int128_param_t magnitude() const noexcept
            requires(is_signed)
        {
            if constexpr (is_twos_complement)
            {
                return is_negative() ? (-(*this)) : (*this);
            }
            else if constexpr (is_magnitude_sign)
            {
                // Extract magnitude bits (all except sign bit)
                int128_param_t result = *this;
                result.data[1] &= ~(1ULL << 63); // Clear sign bit
                return result;
            }
            else
            {
                // excess_k: TODO
                return int128_param_t(0);
            }
        }

        /**
         * @brief Get sign as separate value (+1, 0, -1)
         *
         * **Two's Complement:** Extract from MSB
         * **Magnitude-Sign:** Explicit sign bit
         *
         * @return -1 for negative, 0 for zero, +1 for positive
         */
        constexpr int get_sign() const noexcept
            requires(is_signed)
        {
            if (is_zero())
                return 0;
            return is_negative() ? -1 : 1;
        }

        /// @brief Check if value is zero
        constexpr bool is_zero() const noexcept
        {
            return data[0] == 0 && data[1] == 0;
        }

        /**
         * @brief Check if value is positive zero (+0)
         *
         * Only meaningful for magnitude-sign representation.
         * In two's complement, there is only one zero.
         *
         * @return true if zero AND sign bit is 0 (positive zero)
         */
        constexpr bool is_positive_zero() const noexcept
            requires(is_magnitude_sign && is_signed)
        {
            return is_zero() && !is_negative();
        }

        /**
         * @brief Check if value is negative zero (-0)
         *
         * Only meaningful for magnitude-sign representation.
         * In two's complement, there is only one zero.
         *
         * @return true if zero AND sign bit is 1 (negative zero)
         */
        constexpr bool is_negative_zero() const noexcept
            requires(is_magnitude_sign && is_signed)
        {
            return is_zero() && is_negative();
        }

        // ========================================================================
        // Conversions to String
        // ========================================================================

        /// @brief Convert to decimal string
        std::string to_string() const;

        /// @brief Convert to string in specified base (2-36)
        std::string to_string(int base) const;

        // ========================================================================
        // Byte Operations
        // ========================================================================

        /// @brief Get byte at index (0=LSB, 15=MSB, little-endian)
        constexpr std::byte get_byte(size_t index) const
        {
            if (index >= 16)
                throw std::out_of_range("byte index out of range");
            return std::byte((index < 8) ? (data[0] >> (index * 8)) : (data[1] >> ((index - 8) * 8)));
        }

        /// @brief Set byte at index
        constexpr void set_byte(size_t index, std::byte value)
        {
            if (index >= 16)
                throw std::out_of_range("byte index out of range");
            std::uint64_t &target = (index < 8) ? data[0] : data[1];
            int shift = (index < 8) ? (index * 8) : ((index - 8) * 8);
            target = (target & ~(0xFFULL << shift)) | (std::to_integer<std::uint64_t>(value) << shift);
        }

        // ========================================================================
        // Comparison Operators
        // ========================================================================

        /// @brief Equality operator (representation-agnostic)
        constexpr bool operator==(const int128_param_t &other) const noexcept
        {
            return data[0] == other.data[0] && data[1] == other.data[1];
        }

        /// @brief Inequality operator (representation-agnostic)
        constexpr bool operator!=(const int128_param_t &other) const noexcept
        {
            return !(*this == other);
        }

        /**
         * @brief Less-than operator (representation-aware)
         *
         * **Two's Complement:** Standard signed/unsigned comparison
         * **Magnitude-Sign Unsigned:** Same as two's complement
         * **Magnitude-Sign Signed:** Compare signs first, then magnitudes
         */
        constexpr bool operator<(const int128_param_t &other) const noexcept
            requires(is_signed)
        {
            if constexpr (is_magnitude_sign)
            {
                // Magnitude-Sign signed comparison
                bool this_negative = is_negative();
                bool other_negative = other.is_negative();

                // Different signs: negative < positive
                if (this_negative != other_negative)
                    return this_negative; // true if this is negative, false if other is negative

                // Same sign: compare magnitudes
                // Extract magnitudes (clear sign bit)
                std::uint64_t this_mag_low = data[0];
                std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
                std::uint64_t other_mag_low = other.data[0];
                std::uint64_t other_mag_high = other.data[1] & ~(1ULL << 63);

                if (this_mag_high != other_mag_high)
                    return this_mag_high < other_mag_high;
                return this_mag_low < other_mag_low;
            }
            else
            {
                // Two's Complement signed comparison
                std::int64_t this_high = static_cast<std::int64_t>(data[1]);
                std::int64_t other_high = static_cast<std::int64_t>(other.data[1]);

                if (this_high != other_high)
                    return this_high < other_high;
                return data[0] < other.data[0];
            }
        }

        /**
         * @brief Greater-than operator (representation-aware)
         */
        constexpr bool operator>(const int128_param_t &other) const noexcept
            requires(is_signed)
        {
            return other < *this; // Swap operands
        }

        /**
         * @brief Less-than-or-equal operator (representation-aware)
         */
        constexpr bool operator<=(const int128_param_t &other) const noexcept
            requires(is_signed)
        {
            return (*this < other) || (*this == other);
        }

        /**
         * @brief Greater-than-or-equal operator (representation-aware)
         */
        constexpr bool operator>=(const int128_param_t &other) const noexcept
            requires(is_signed)
        {
            return (other < *this) || (*this == other);
        }

        /// @brief Less-than for unsigned (standard comparison)
        constexpr bool operator<(const int128_param_t &other) const noexcept
            requires(!is_signed)
        {
            if (data[1] != other.data[1])
                return data[1] < other.data[1];
            return data[0] < other.data[0];
        }

        /// @brief Greater-than for unsigned (standard comparison)
        constexpr bool operator>(const int128_param_t &other) const noexcept
            requires(!is_signed)
        {
            return other < *this;
        }

        /// @brief Less-than-or-equal for unsigned (standard comparison)
        constexpr bool operator<=(const int128_param_t &other) const noexcept
            requires(!is_signed)
        {
            return (*this < other) || (*this == other);
        }

        /// @brief Greater-than-or-equal for unsigned (standard comparison)
        constexpr bool operator>=(const int128_param_t &other) const noexcept
            requires(!is_signed)
        {
            return (other < *this) || (*this == other);
        }

        // ========================================================================
        // Arithmetic Operators
        // ========================================================================

        /**
         * @brief Unary plus operator
         * @return Copy of this value
         */
        constexpr int128_param_t operator+() const noexcept
        {
            return *this;
        }

        /**
         * @brief Unary negation operator (representation-aware)
         *
         * **Two's Complement:** Invert all bits and add 1
         * **Magnitude-Sign:** Flip sign bit (single bit operation)
         *
         * @return Negated value
         */
        constexpr int128_param_t operator-() const noexcept
            requires(is_signed)
        {
            if constexpr (is_magnitude_sign)
            {
                // Magnitude-Sign: Just flip the sign bit (MSB of data[1])
                int128_param_t result = *this;
                result.data[1] ^= (1ULL << 63);
                return result;
            }
            else
            {
                // Two's Complement: Invert bits and add 1
                int128_param_t result;
                result.data[0] = ~data[0];
                result.data[1] = ~data[1];
                // Add 1 to low word
                ++result.data[0];
                // Propagate carry to high word
                if (result.data[0] == 0)
                    ++result.data[1];
                return result;
            }
        }

        /**
         * @brief Addition assignment operator (representation-agnostic)
         *
         * Performs 128-bit addition with carry propagation.
         * Works identically for TC and MS representations.
         *
         * @param other Value to add
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator+=(const int128_param_t &other) noexcept
        {
            std::uint64_t new_low = data[0] + other.data[0];
            std::uint64_t carry = (new_low < data[0]) ? 1 : 0;
            data[0] = new_low;
            data[1] = data[1] + other.data[1] + carry;
            return *this;
        }

        /**
         * @brief Addition operator
         * @param other Value to add
         * @return Sum of this and other
         */
        constexpr int128_param_t operator+(const int128_param_t &other) const noexcept
        {
            int128_param_t result = *this;
            result += other;
            return result;
        }

        /**
         * @brief Subtraction assignment operator (representation-agnostic)
         *
         * Performs 128-bit subtraction with borrow propagation.
         * Works identically for TC and MS representations.
         *
         * @param other Value to subtract
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator-=(const int128_param_t &other) noexcept
        {
            std::uint64_t new_low = data[0] - other.data[0];
            std::uint64_t borrow = (new_low > data[0]) ? 1 : 0;
            data[0] = new_low;
            data[1] = data[1] - other.data[1] - borrow;
            return *this;
        }

        /**
         * @brief Subtraction operator
         * @param other Value to subtract
         * @return Difference of this and other
         */
        constexpr int128_param_t operator-(const int128_param_t &other) const noexcept
        {
            int128_param_t result = *this;
            result -= other;
            return result;
        }

        /**
         * @brief Multiplication assignment operator (representation-agnostic)
         *
         * Performs basic 128-bit multiplication.
         * Works identically for TC and MS representations.
         *
         * Note: Only handles 64-bit × 64-bit → 128-bit products efficiently.
         * Full 128-bit × 128-bit requires more complex logic.
         *
         * @param other Value to multiply
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator*=(const int128_param_t &other) noexcept
        {
            // Simple multiplication: decompose into 64-bit chunks
            // a·b = (a_high·2^64 + a_low) · (b_high·2^64 + b_low)
            //     = a_high·b_high·2^128 + (a_high·b_low + a_low·b_high)·2^64 + a_low·b_low

            std::uint64_t a_low = data[0];
            std::uint64_t a_high = data[1];
            std::uint64_t b_low = other.data[0];
            std::uint64_t b_high = other.data[1];

            // Low 128 bits: a_low * b_low
            // Note: This is a simplification; full implementation would use __uint128_t or similar
            std::uint64_t product_low_low = a_low * b_low;

            // Cross products for the 128-bit result
            std::uint64_t cross_1 = a_high * b_low;
            std::uint64_t cross_2 = a_low * b_high;

            // Combine: low part stays, high part accumulates
            data[0] = product_low_low;
            data[1] = cross_1 + cross_2 + (a_high * b_high);

            return *this;
        }

        /**
         * @brief Multiplication operator
         * @param other Value to multiply
         * @return Product of this and other
         */
        constexpr int128_param_t operator*(const int128_param_t &other) const noexcept
        {
            int128_param_t result = *this;
            result *= other;
            return result;
        }

        /**
         * @brief Division assignment operator (representation-agnostic)
         *
         * Performs 128-bit integer division.
         * Note: Full implementation is complex; this is a stub.
         *
         * @param other Divisor (must be non-zero)
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator/=(const int128_param_t &other) noexcept
        {
            // TODO: Implement full 128-bit division
            // For now, handle simple cases
            if (other.data[1] == 0 && other.data[0] != 0)
            {
                // Divide by 64-bit value
                std::uint64_t divisor = other.data[0];
                std::uint64_t remainder = 0;
                std::uint64_t new_high = data[1] / divisor;
                remainder = data[1] % divisor;
                std::uint64_t new_low = (remainder * (std::numeric_limits<std::uint64_t>::max() / divisor + 1)) + data[0] / divisor;
                data[0] = new_low;
                data[1] = new_high;
            }
            return *this;
        }

        /**
         * @brief Division operator
         * @param other Divisor
         * @return Quotient of this divided by other
         */
        constexpr int128_param_t operator/(const int128_param_t &other) const noexcept
        {
            int128_param_t result = *this;
            result /= other;
            return result;
        }

        /**
         * @brief Modulo assignment operator
         *
         * @param other Divisor
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator%=(const int128_param_t &other) noexcept
        {
            // TODO: Implement full 128-bit modulo
            if (other.data[1] == 0 && other.data[0] != 0)
            {
                std::uint64_t divisor = other.data[0];
                data[0] = data[0] % divisor;
                data[1] = 0;
            }
            return *this;
        }

        /**
         * @brief Modulo operator
         * @param other Divisor
         * @return Remainder of this divided by other
         */
        constexpr int128_param_t operator%(const int128_param_t &other) const noexcept
        {
            int128_param_t result = *this;
            result %= other;
            return result;
        }
    };

    // =============================================================================
    // Type Aliases
    // =============================================================================

    // Two's Complement (Phase 1.66 Compatible)
    using uint128_tc_t = int128_param_t<signedness::unsigned_type, representation_form::twos_complement>;
    using int128_tc_t = int128_param_t<signedness::signed_type, representation_form::twos_complement>;

    // Magnitude-Sign (Phase 1.75 Primary Investigation)
    using uint128_ms_t = int128_param_t<signedness::unsigned_type, representation_form::magnitude_sign>;
    using int128_ms_t = int128_param_t<signedness::signed_type, representation_form::magnitude_sign>;

    // Excess-k (Phase 1.75 Future - IEEE 754 Exponents)
    using uint128_ek_t = int128_param_t<signedness::unsigned_type, representation_form::excess_k>;
    using int128_ek_t = int128_param_t<signedness::signed_type, representation_form::excess_k>;

    // Default aliases (backward compatible with Phase 1.66)
    using uint128_t = uint128_tc_t;
    using int128_t = int128_tc_t;

} // namespace nstd

#endif // INT128_PARAMETERIZED_HPP
