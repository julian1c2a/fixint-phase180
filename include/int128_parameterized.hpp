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
#include <bitset>
#include <stdexcept>

namespace nstd
{
    // =============================================================================
    // Parse Error Enums and Result Structures
    // =============================================================================

    /**
     * @brief Enum for parsing error codes
     */
    enum class parse_error : std::uint8_t
    {
        success = 0,             ///< Parsing successful
        null_pointer,            ///< Null pointer provided
        empty_string,            ///< Empty string
        invalid_base,            ///< Base out of range [2, 36]
        invalid_base_value,      ///< Invalid base (alias of invalid_base)
        invalid_character,       ///< Invalid character for specified base
        digit_out_of_range,      ///< Digit out of range for base
        no_digits,               ///< No valid digits found
        overflow,                ///< Result exceeds type range
        separator_at_boundaries, ///< Separator at start or end
        unknown_error            ///< Unknown error
    };

    /**
     * @brief Structure encapsulating parse_ct result with error detection
     *
     * @tparam T Data type to parse (int128_param_t, etc.)
     *
     * @details Allows parse_ct_safe to return error with exact error location (error_index)
     * without throwing exceptions. Essential for constexpr contexts where exceptions not allowed.
     */
    template <typename T>
    struct parse_result
    {
        parse_error error;  ///< Error code
        T value;            ///< Parsed value
        size_t error_index; ///< Index of error in string (npos if success)

        /**
         * @brief Checks if parse was successful
         * @return true if error == parse_error::success, false otherwise
         */
        constexpr bool success() const noexcept
        {
            return error == parse_error::success;
        }

        /**
         * @brief Default constructor
         * Initializes with error=success, value=0, error_index=npos
         */
        constexpr parse_result() noexcept
            : error(parse_error::success),
              value(T{}),
              error_index(std::string::npos)
        {
        }

        /**
         * @brief Custom constructor
         */
        constexpr parse_result(parse_error err, T val, size_t idx) noexcept
            : error(err),
              value(val),
              error_index(idx)
        {
        }
    };

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
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        explicit constexpr int128_param_t(T value) noexcept
        {

            if constexpr (std::is_signed_v<T>)
            {
                // Always sign-extend (interpret as Two's Complement internally)
                // Users can convert to other representations explicitly if needed
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

        /// @brief Constructor from (high, low) pair (written in Western order, stored little-endian)
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
            else // excess_k
            {
                // Excess-K: negative when stored_value < bias
                // bias = 2^126 = 0x4000000000000000 0x0000000000000000
                constexpr uint64_t bias_high = (1ULL << 62);
                constexpr uint64_t bias_low = 0;

                if (data[1] < bias_high)
                {
                    return true;
                }
                else if (data[1] > bias_high)
                {
                    return false;
                }
                else
                {
                    return data[0] < bias_low;
                }
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
            else // excess_k
            {
                // Excess-K: magnitude = |stored_value - bias|
                // Convert to real value first, then take absolute value
                constexpr uint64_t bias_high = (1ULL << 62);
                constexpr uint64_t bias_low = 0;

                int128_param_t result{*this};

                // Subtract bias (real_value = stored - bias)
                bool borrow = false;
                if (result.data[0] < bias_low)
                {
                    borrow = true;
                }
                result.data[0] -= bias_low;

                if (borrow)
                {
                    if (result.data[1] == 0)
                    {
                        // Underflow, value was negative
                        result.data[1] = ~0ULL - bias_high + 1;
                    }
                    else
                    {
                        result.data[1] = result.data[1] - bias_high - 1;
                    }
                }
                else
                {
                    result.data[1] -= bias_high;
                }

                // If negative (MSB set in TC interpretation), negate
                if ((result.data[1] & (1ULL << 63)) != 0)
                {
                    // Negate (two's complement negation)
                    result.data[0] = ~result.data[0];
                    result.data[1] = ~result.data[1];

                    // Add 1
                    result.data[0]++;
                    if (result.data[0] == 0)
                    {
                        result.data[1]++;
                    }
                }

                return result;
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
            if constexpr (is_magnitude_sign)
            {
                // MS: zero if magnitude bits are all 0
                // Magnitude is in bits [0..62] of both words
                const std::uint64_t magnitude_mask = (1ULL << 63) - 1; // All bits except sign
                return (data[0] == 0) && ((data[1] & magnitude_mask) == 0);
            }
            else if constexpr (is_excess_k)
            {
                // Excess-K: zero when stored_value == bias
                // bias = 2^126 = 0x4000000000000000 0x0000000000000000
                constexpr uint64_t bias_high = (1ULL << 62);
                constexpr uint64_t bias_low = 0;
                return (data[0] == bias_low) && (data[1] == bias_high);
            }
            else // twos_complement
            {
                // TC: zero only if both words are 0
                return data[0] == 0 && data[1] == 0;
            }
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

        /**
         * @brief Convert to decimal string representation
         *
         * Converts the 128-bit value to its decimal string representation.
         * For signed types, includes the minus sign if negative.
         *
         * @return Decimal string (e.g., "12345", "-67890")
         */
        std::string to_string() const noexcept
        {
            return to_string(10);
        }

        /**
         * @brief Convert to string in specified base (2-36)
         *
         * Converts the 128-bit value to string representation in the given base.
         * - Base 2: Binary (digits 0-1)
         * - Base 8: Octal (digits 0-7)
         * - Base 10: Decimal (digits 0-9)
         * - Base 16: Hexadecimal (digits 0-9, A-F)
         * - Base 2-36: General (digits 0-9, A-Z)
         *
         * For signed types, includes the minus sign if negative.
         *
         * @param base Base for conversion (2-36). Default is 10 (decimal).
         * @return String representation in the specified base
         */
        std::string to_string(int base) const noexcept
        {
            // Validate base
            if (base < 2 || base > 36)
                base = 10; // Default to decimal if invalid

            // Handle zero
            if (is_zero())
                return "0";

            // For signed types, handle negative numbers
            bool is_negative_value = false;
            int128_param_t abs_value = *this;

            if constexpr (is_signed)
            {
                if (is_negative_value = this->is_negative())
                {
                    abs_value = -(*this);
                }
            }

            // Convert absolute value to string
            std::string result;
            int128_param_t value = abs_value;

            while (!value.is_zero())
            {
                // Get remainder when divided by base
                std::uint64_t remainder;
                if constexpr (std::is_same_v<std::remove_const_t<decltype(base)>, int>)
                {
                    // Simple division for bases
                    if (value.data[1] == 0)
                    {
                        remainder = value.data[0] % base;
                        value.data[0] /= base;
                    }
                    else
                    {
                        // For large numbers, use simpler approach
                        remainder = value.data[0] % base;
                        value.data[0] = value.data[0] / base;
                        if (value.data[1] > 0)
                        {
                            // Carry from high word
                            std::uint64_t carry = (value.data[1] % base) * (std::numeric_limits<std::uint64_t>::max() / base + 1);
                            value.data[0] += carry;
                            value.data[1] /= base;
                        }
                    }
                }

                // Convert digit to character
                const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
                result = digits[remainder] + result;
            }

            // Add sign if negative
            if (is_negative_value)
                result = "-" + result;

            return result.empty() ? "0" : result;
        }

        // ========================================================================
        // Conversions from String
        // ========================================================================

        /**
         * @brief Safe constexpr parser for string input with error reporting
         *
         * @details Parses string at compile-time (constexpr). Supports:
         * - Decimal (default): "12345"
         * - Hexadecimal: "0xDEADBEEF", "0XDEADBEEF"
         * - Binary: "0b11110000", "0B11110000"
         * - Octal: "0777"
         * - Separators: ignored (underscores, single quotes)
         *
         * @param str Null-terminated string to parse
         * @return parse_result<> containing error code, value, and error index
         *
         * @code
         * constexpr auto result = int128_tc_t::parse_ct_safe("0xDEADBEEF");
         * if (result.success()) {
         *     // Use result.value
         * } else {
         *     // Check result.error and result.error_index
         * }
         * @endcode
         */
        static constexpr parse_result<int128_param_t> parse_ct_safe(const char *str) noexcept
        {
            parse_result<int128_param_t> result{};

            // Check for null pointer
            if (!str)
            {
                result.error = parse_error::null_pointer;
                result.error_index = 0;
                return result;
            }

            // Check for empty string
            if (*str == '\0')
            {
                result.error = parse_error::empty_string;
                result.error_index = 0;
                return result;
            }

            int base = 10;
            const char *ptr = str;
            size_t index = 0;
            bool is_negative = false;

            // Handle sign for signed types
            if constexpr (is_signed)
            {
                if (*ptr == '-')
                {
                    is_negative = true;
                    ++ptr;
                    ++index;
                }
                else if (*ptr == '+')
                {
                    ++ptr;
                    ++index;
                }
            }

            // Auto-detect base from prefix
            if (*ptr == '0' && *(ptr + 1) != '\0')
            {
                char next = *(ptr + 1);
                if (next == 'x' || next == 'X')
                {
                    base = 16;
                    ptr += 2;
                    index += 2;
                }
                else if (next == 'b' || next == 'B')
                {
                    base = 2;
                    ptr += 2;
                    index += 2;
                }
                else if (next >= '0' && next <= '7')
                {
                    base = 8;
                    ptr += 1;
                    index += 1;
                }
            }

            int128_param_t parsed_value{};
            bool found_digit = false;
            size_t digit_start_index = index;

            while (*ptr != '\0')
            {
                unsigned digit = 0;
                char c = *ptr;

                // Skip separators (but track position)
                if (c == '_' || c == '\'')
                {
                    // Check for separator at start of digits
                    if (!found_digit && *(ptr + 1) != '\0')
                    {
                        result.error = parse_error::separator_at_boundaries;
                        result.error_index = index;
                        return result;
                    }

                    ++ptr;
                    ++index;
                    continue;
                }

                // Parse digit
                if (c >= '0' && c <= '9')
                {
                    digit = c - '0';
                    found_digit = true;
                }
                else if (c >= 'a' && c <= 'f')
                {
                    digit = c - 'a' + 10;
                    found_digit = true;
                }
                else if (c >= 'A' && c <= 'F')
                {
                    digit = c - 'A' + 10;
                    found_digit = true;
                }
                else
                {
                    result.error = parse_error::invalid_character;
                    result.error_index = index;
                    return result;
                }

                // Check digit is valid for base
                if (digit >= static_cast<unsigned>(base))
                {
                    result.error = parse_error::digit_out_of_range;
                    result.error_index = index;
                    return result;
                }

                // Multiply by base and add digit (with overflow check)
                int128_param_t old_value = parsed_value;
                parsed_value = parsed_value * int128_param_t(base) + int128_param_t(digit);

                // Check overflow
                if (parsed_value < old_value && digit != 0)
                {
                    result.error = parse_error::overflow;
                    result.error_index = index;
                    return result;
                }

                ++ptr;
                ++index;
            }

            // Check if we found any digits
            if (!found_digit)
            {
                result.error = parse_error::no_digits;
                result.error_index = digit_start_index;
                return result;
            }

            // Apply sign for signed types
            if constexpr (is_signed)
            {
                if (is_negative)
                {
                    parsed_value = -parsed_value;
                }
            }

            result.error = parse_error::success;
            result.value = parsed_value;
            result.error_index = static_cast<size_t>(-1); // npos
            return result;
        }

        /**
         * @brief Parse string with automatic base detection (constexpr)
         *
         * @details Can be evaluated at compile-time or runtime. Throws on error.
         * For safe version with error reporting, use parse_ct_safe() instead.
         *
         * @param str Null-terminated string to parse
         * @return Parsed value
         * @throw std::invalid_argument if string is invalid
         * @throw std::out_of_range if value overflows
         *
         * @code
         * // Compile-time usage:
         * constexpr auto val = int128_tc_t::from_string("0xDEADBEEF");
         *
         * // Runtime usage:
         * auto val = int128_tc_t::from_string(user_input);  // Can fail at runtime
         * @endcode
         */
        static constexpr int128_param_t from_string(const char *str)
        {
            auto safe_result = parse_ct_safe(str);
            if (!safe_result.success())
            {
                // Provide informative error message
                switch (safe_result.error)
                {
                case parse_error::null_pointer:
                    throw std::invalid_argument("Null pointer");
                case parse_error::empty_string:
                    throw std::invalid_argument("Empty string");
                case parse_error::invalid_character:
                    throw std::invalid_argument("Invalid character");
                case parse_error::digit_out_of_range:
                    throw std::invalid_argument("Digit out of range");
                case parse_error::no_digits:
                    throw std::invalid_argument("No digits found");
                case parse_error::overflow:
                    throw std::out_of_range("Number too large");
                case parse_error::separator_at_boundaries:
                    throw std::invalid_argument("Separator at invalid position");
                default:
                    throw std::invalid_argument("Parse error");
                }
            }
            return safe_result.value;
        }

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

        /// @brief Equality operator (representation-aware)
        constexpr bool operator==(const int128_param_t &other) const noexcept
        {
            // Special handling for MS: +0 and -0 are mathematically equal
            if constexpr (is_magnitude_sign)
            {
                // Both zero? (magnitude bits = 0)
                if (is_zero() && other.is_zero())
                    return true; // +0 == -0 in MS
            }

            // Otherwise compare bit patterns
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
         * **Magnitude-Sign Signed:** Compare signs first, then magnitudes
         *                            For negatives: INVERTED comparison (-2 < -1 means |2| > |1|)
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
                    return this_negative;

                // Same sign: compare magnitudes
                std::uint64_t this_mag_low = data[0];
                std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
                std::uint64_t other_mag_low = other.data[0];
                std::uint64_t other_mag_high = other.data[1] & ~(1ULL << 63);

                if constexpr (is_signed)
                {
                    if (this_negative)
                    {
                        // Both negative: INVERT comparison (larger magnitude = smaller value)
                        // -2 < -1 means |2| > |1|
                        if (this_mag_high != other_mag_high)
                            return this_mag_high > other_mag_high; // INVERTED
                        return this_mag_low > other_mag_low;       // INVERTED
                    }
                    else
                    {
                        // Both positive: NORMAL comparison
                        // 1 < 2 means |1| < |2|
                        if (this_mag_high != other_mag_high)
                            return this_mag_high < other_mag_high;
                        return this_mag_low < other_mag_low;
                    }
                }
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
         * **Excess-K:** Negate via: -x = bias - (x - bias) = 2·bias - x
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
            else if constexpr (is_excess_k)
            {
                // Excess-K: -x = 2·bias - x
                // bias = 2^126, so 2·bias = 2^127
                constexpr uint64_t two_bias_high = (1ULL << 63);
                constexpr uint64_t two_bias_low = 0;

                int128_param_t result;

                // Subtract this value from 2·bias
                bool borrow = false;
                if (two_bias_low < data[0])
                {
                    borrow = true;
                }
                result.data[0] = two_bias_low - data[0];

                if (borrow)
                {
                    result.data[1] = two_bias_high - data[1] - 1;
                }
                else
                {
                    result.data[1] = two_bias_high - data[1];
                }

                return result;
            }
            else // twos_complement
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

        // ========================================================================
        // Float/Double Conversions (Priority 10)
        // ========================================================================

        /**
         * @brief Explicit conversion to double
         *
         * @return Double representation of this value
         *
         * @details
         * - Precision loss may occur (double has 52-bit mantissa)
         * - For MS signed: converts magnitude, applies sign
         * - For TC signed: handles negative values via two's complement
         * - Large values may lose precision or become infinity
         *
         * @example
         * uint128_tc_t x{0, 100};
         * double d = static_cast<double>(x);  // d = 100.0
         */
        explicit constexpr operator double() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: get magnitude, then apply sign
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                const bool negative{(data[1] & (1ULL << 63)) != 0};

                // Convert magnitude to double
                double result{static_cast<double>(mag_high) * 18446744073709551616.0 + // 2^64
                              static_cast<double>(data[0])};

                return negative ? -result : result;
            }
            else if constexpr (is_signed)
            {
                // TC: check if negative
                if (is_negative())
                {
                    // Convert -x to positive, then negate result
                    const int128_param_t abs_val{-(*this)};
                    // Reinterpret as unsigned for conversion
                    const int128_param_t<signedness::unsigned_type, Form> unsigned_val{
                        abs_val.high(), abs_val.low()};
                    return -static_cast<double>(unsigned_val);
                }
            }

            // Unsigned or positive: standard conversion
            return static_cast<double>(data[1]) * 18446744073709551616.0 + // 2^64
                   static_cast<double>(data[0]);
        }

        /**
         * @brief Explicit conversion to long double
         *
         * @return Long double representation of this value
         *
         * @details
         * - Better precision than double (typically 64-bit mantissa on x86)
         * - Same overflow considerations as double
         * - Representation-aware for MS signed values
         */
        explicit constexpr operator long double() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: get magnitude, then apply sign
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                const bool negative{(data[1] & (1ULL << 63)) != 0};

                long double result{static_cast<long double>(mag_high) * 18446744073709551616.0L +
                                   static_cast<long double>(data[0])};

                return negative ? -result : result;
            }
            else if constexpr (is_signed)
            {
                // TC: check if negative
                if (is_negative())
                {
                    const int128_param_t abs_val{-(*this)};
                    const int128_param_t<signedness::unsigned_type, Form> unsigned_val{
                        abs_val.high(), abs_val.low()};
                    return -static_cast<long double>(unsigned_val);
                }
            }

            // Unsigned or positive
            return static_cast<long double>(data[1]) * 18446744073709551616.0L +
                   static_cast<long double>(data[0]);
        }

        /**
         * @brief Constructor from double (explicit)
         *
         * @param value Double value to convert
         *
         * @details
         * - Truncates fractional part
         * - Handles negative values appropriately for TC/MS
         * - Overflow/underflow behavior: saturates to max/min value
         *
         * @note Requires std::isfinite(), std::isnan() checks at runtime
         */
        explicit constexpr int128_param_t(double value) noexcept
            : data{0, 0}
        {
            // Handle special values
            if (value != value) // NaN check
            {
                return; // Zero
            }

            const bool negative{value < 0.0};
            double abs_val{negative ? -value : value};

            // Handle overflow (too large for 128-bit)
            if (abs_val >= 340282366920938463463374607431768211456.0) // 2^128
            {
                // Saturate to max
                if constexpr (is_signed)
                {
                    if (negative)
                    {
                        // Set to minimum value
                        data[0] = 0;
                        data[1] = 0x8000000000000000ULL;
                        return;
                    }
                }
                // Max value
                data[0] = ~0ULL;
                data[1] = ~0ULL;
                return;
            }

            // Extract high and low parts
            if (abs_val >= 18446744073709551616.0) // 2^64
            {
                const double high_part{abs_val / 18446744073709551616.0};
                data[1] = static_cast<uint64_t>(high_part);
                abs_val -= static_cast<double>(data[1]) * 18446744073709551616.0;
            }

            data[0] = static_cast<uint64_t>(abs_val);

            // Apply sign for signed types
            if constexpr (is_signed)
            {
                if (negative)
                {
                    if constexpr (is_magnitude_sign)
                    {
                        // MS: set sign bit
                        data[1] |= (1ULL << 63);
                    }
                    else
                    {
                        // TC: negate
                        *this = -*this;
                    }
                }
            }
        }

        /**
         * @brief Constructor from long double (explicit)
         *
         * @param value Long double value to convert
         *
         * @details Same behavior as double constructor but with better precision
         */
        explicit constexpr int128_param_t(long double value) noexcept
            : data{0, 0}
        {
            // Handle special values
            if (value != value) // NaN
            {
                return;
            }

            const bool negative{value < 0.0L};
            long double abs_val{negative ? -value : value};

            // Handle overflow
            if (abs_val >= 340282366920938463463374607431768211456.0L) // 2^128
            {
                if constexpr (is_signed)
                {
                    if (negative)
                    {
                        data[0] = 0;
                        data[1] = 0x8000000000000000ULL;
                        return;
                    }
                }
                data[0] = ~0ULL;
                data[1] = ~0ULL;
                return;
            }

            // Extract parts
            if (abs_val >= 18446744073709551616.0L) // 2^64
            {
                const long double high_part{abs_val / 18446744073709551616.0L};
                data[1] = static_cast<uint64_t>(high_part);
                abs_val -= static_cast<long double>(data[1]) * 18446744073709551616.0L;
            }

            data[0] = static_cast<uint64_t>(abs_val);

            // Apply sign
            if constexpr (is_signed)
            {
                if (negative)
                {
                    if constexpr (is_magnitude_sign)
                    {
                        data[1] |= (1ULL << 63);
                    }
                    else
                    {
                        *this = -*this;
                    }
                }
            }
        }

        // ========================================================================
        // Array & Bitset Conversions (Priority 11)
        // ========================================================================

        /**
         * @brief Convert to std::array of bytes (little-endian)
         *
         * Serializes the 128-bit value to a 16-byte array in little-endian order.
         * - Bytes [0..7] = low 64 bits (data[0])
         * - Bytes [8..15] = high 64 bits (data[1])
         *
         * For MS representation, the sign bit is included in the serialization.
         *
         * @return std::array<std::byte, 16> containing the byte representation
         *
         * @note Little-endian byte order matches internal storage (data[0]=low)
         *
         * @code
         * uint128_tc_t x{0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL};
         * auto bytes = static_cast<std::array<std::byte, 16>>(x);
         * // bytes[0] = 0xEF, bytes[1] = 0xCD, ...
         * // bytes[15] = 0xFE
         * @endcode
         */
        explicit constexpr operator std::array<std::byte, 16>() const noexcept
        {
            std::array<std::byte, 16> result{};

            // Low 64 bits (data[0]) → bytes [0..7]
            for (int i{0}; i < 8; ++i)
            {
                result[i] = static_cast<std::byte>((data[0] >> (i * 8)) & 0xFF);
            }

            // High 64 bits (data[1]) → bytes [8..15]
            for (int i{0}; i < 8; ++i)
            {
                result[i + 8] = static_cast<std::byte>((data[1] >> (i * 8)) & 0xFF);
            }

            return result;
        }

        /**
         * @brief Convert to std::bitset<128>
         *
         * Creates a bitset representation where:
         * - bit 0 = LSB (least significant bit of data[0])
         * - bit 63 = MSB of data[0]
         * - bit 64 = LSB of data[1]
         * - bit 127 = MSB (for MS signed: this is the sign bit)
         *
         * @return std::bitset<128> with bits set according to internal storage
         *
         * @code
         * uint128_tc_t x{0xFF};
         * auto bits = static_cast<std::bitset<128>>(x);
         * // bits[0..7] = 1, bits[8..127] = 0
         * @endcode
         */
        explicit constexpr operator std::bitset<128>() const noexcept
        {
            std::bitset<128> result{};

            // Set bits from low limb (data[0])
            for (int i{0}; i < 64; ++i)
            {
                if ((data[0] & (1ULL << i)) != 0)
                {
                    result.set(i);
                }
            }

            // Set bits from high limb (data[1])
            for (int i{0}; i < 64; ++i)
            {
                if ((data[1] & (1ULL << i)) != 0)
                {
                    result.set(i + 64);
                }
            }

            return result;
        }

        /**
         * @brief Construct from std::array of bytes (little-endian)
         *
         * Deserializes a 16-byte array in little-endian order to a 128-bit value.
         * - Bytes [0..7] → low 64 bits (data[0])
         * - Bytes [8..15] → high 64 bits (data[1])
         *
         * For MS signed types, the sign bit (MSB of byte[15]) is preserved.
         *
         * @param bytes Byte array in little-endian order
         *
         * @note This is the inverse operation of operator std::array<std::byte, 16>()
         *
         * @code
         * std::array<std::byte, 16> bytes{};
         * bytes[0] = std::byte{0xFF};
         * bytes[1] = std::byte{0x00};
         * // ... (remaining bytes)
         * uint128_tc_t x{bytes};  // Deserializes from byte array
         * @endcode
         */
        explicit constexpr int128_param_t(const std::array<std::byte, 16> &bytes) noexcept
            : data{0, 0}
        {
            // Reconstruct low 64 bits from bytes [0..7]
            for (int i{0}; i < 8; ++i)
            {
                data[0] |= (static_cast<uint64_t>(bytes[i]) << (i * 8));
            }

            // Reconstruct high 64 bits from bytes [8..15]
            for (int i{0}; i < 8; ++i)
            {
                data[1] |= (static_cast<uint64_t>(bytes[i + 8]) << (i * 8));
            }
        }

        /**
         * @brief Construct from std::bitset<128>
         *
         * Creates an int128 value from a bitset where:
         * - bit 0 = LSB (least significant bit of data[0])
         * - bit 63 = MSB of data[0]
         * - bit 64 = LSB of data[1]
         * - bit 127 = MSB (for MS signed: this becomes the sign bit)
         *
         * @param bits Bitset with 128 bits
         *
         * @note This is the inverse operation of operator std::bitset<128>()
         *
         * @code
         * std::bitset<128> bits{};
         * bits.set(0);  // Set LSB
         * bits.set(127);  // Set MSB
         * uint128_tc_t x{bits};
         * @endcode
         */
        explicit constexpr int128_param_t(const std::bitset<128> &bits) noexcept
            : data{0, 0}
        {
            // Reconstruct low 64 bits (bits 0..63)
            for (int i{0}; i < 64; ++i)
            {
                if (bits.test(i))
                {
                    data[0] |= (1ULL << i);
                }
            }

            // Reconstruct high 64 bits (bits 64..127)
            for (int i{0}; i < 64; ++i)
            {
                if (bits.test(i + 64))
                {
                    data[1] |= (1ULL << i);
                }
            }
        }

        // ========================================================================
        // Arithmetic Operations
        // ========================================================================

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

        // ========================================================================
        // Bitwise Operators (AND, OR, XOR, NOT)
        // ========================================================================

        /**
         * @brief Bitwise AND operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise AND
         * **Magnitude-Sign:** Apply AND to magnitude bits only, preserve signs
         */
        constexpr int128_param_t operator&(const int128_param_t &other) const noexcept
        {
            int128_param_t result;

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: AND magnitudes, preserve signs separately
                std::uint64_t this_mag_low = data[0];
                std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
                std::uint64_t other_mag_low = other.data[0];
                std::uint64_t other_mag_high = other.data[1] & ~(1ULL << 63);

                result.data[0] = this_mag_low & other_mag_low;
                result.data[1] = (this_mag_high & other_mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise AND
                result.data[0] = data[0] & other.data[0];
                result.data[1] = data[1] & other.data[1];
            }

            return result;
        }

        /**
         * @brief Bitwise AND assignment operator
         */
        constexpr int128_param_t &operator&=(const int128_param_t &other) noexcept
        {
            *this = *this & other;
            return *this;
        }

        /**
         * @brief Bitwise OR operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise OR
         * **Magnitude-Sign:** Apply OR to magnitude bits only, preserve signs
         */
        constexpr int128_param_t operator|(const int128_param_t &other) const noexcept
        {
            int128_param_t result;

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: OR magnitudes, preserve signs separately
                std::uint64_t this_mag_low = data[0];
                std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
                std::uint64_t other_mag_low = other.data[0];
                std::uint64_t other_mag_high = other.data[1] & ~(1ULL << 63);

                result.data[0] = this_mag_low | other_mag_low;
                result.data[1] = (this_mag_high | other_mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise OR
                result.data[0] = data[0] | other.data[0];
                result.data[1] = data[1] | other.data[1];
            }

            return result;
        }

        /**
         * @brief Bitwise OR assignment operator
         */
        constexpr int128_param_t &operator|=(const int128_param_t &other) noexcept
        {
            *this = *this | other;
            return *this;
        }

        /**
         * @brief Bitwise XOR operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise XOR
         * **Magnitude-Sign:** Apply XOR to magnitude bits only, preserve signs
         */
        constexpr int128_param_t operator^(const int128_param_t &other) const noexcept
        {
            int128_param_t result;

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: XOR magnitudes, preserve signs separately
                std::uint64_t this_mag_low = data[0];
                std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
                std::uint64_t other_mag_low = other.data[0];
                std::uint64_t other_mag_high = other.data[1] & ~(1ULL << 63);

                result.data[0] = this_mag_low ^ other_mag_low;
                result.data[1] = (this_mag_high ^ other_mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise XOR
                result.data[0] = data[0] ^ other.data[0];
                result.data[1] = data[1] ^ other.data[1];
            }

            return result;
        }

        /**
         * @brief Bitwise XOR assignment operator
         */
        constexpr int128_param_t &operator^=(const int128_param_t &other) noexcept
        {
            *this = *this ^ other;
            return *this;
        }

        /**
         * @brief Bitwise NOT operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise complement (flip all bits)
         * **Magnitude-Sign:** Invert magnitude bits only, preserve sign bit
         */
        constexpr int128_param_t operator~() const noexcept
        {
            int128_param_t result;

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: Invert magnitude bits, preserve sign bit
                std::uint64_t mag_low = data[0];
                std::uint64_t mag_high = data[1] & ~(1ULL << 63);

                result.data[0] = ~mag_low;
                result.data[1] = (~mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise NOT
                result.data[0] = ~data[0];
                result.data[1] = ~data[1];
            }

            return result;
        }

        // ========================================================================
        // Shift Operators
        // ========================================================================

        /// @brief Left shift assignment operator
        constexpr int128_param_t &operator<<=(int shift) noexcept
        {
            if (shift <= 0)
            {
                return *this;
            }
            if (shift >= 128)
            {
                data[0] = 0;
                data[1] = 0;
                return *this;
            }

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: Extract sign, shift magnitude as unsigned, restore sign
                uint64_t sign_bit = data[1] & (1ULL << 63);
                uint64_t mag_high = data[1] & ~(1ULL << 63);

                if (shift >= 64)
                {
                    // Shift magnitude (as if unsigned)
                    uint64_t new_high = data[0] << (shift - 64);
                    data[0] = 0;
                    // Restore sign
                    data[1] = new_high | sign_bit;
                }
                else
                {
                    // Shift magnitude (as if unsigned)
                    uint64_t new_high = (mag_high << shift) | (data[0] >> (64 - shift));
                    uint64_t new_low = data[0] << shift;
                    data[0] = new_low;
                    // Restore sign
                    data[1] = new_high | sign_bit;
                }
            }
            else
            {
                // TC and unsigned: standard left shift
                if (shift >= 64)
                {
                    uint64_t new_high = data[0] << (shift - 64);
                    data[0] = 0;
                    data[1] = new_high;
                }
                else
                {
                    uint64_t new_high = (data[1] << shift) | (data[0] >> (64 - shift));
                    uint64_t new_low = data[0] << shift;
                    data[0] = new_low;
                    data[1] = new_high;
                }
            }

            return *this;
        }

        /// @brief Left shift operator
        constexpr int128_param_t operator<<(int shift) const noexcept
        {
            int128_param_t result(*this);
            result <<= shift;
            return result;
        }

        /// @brief Left shift assignment with integral type
        template <typename T>
        constexpr int128_param_t &operator<<=(T shift) noexcept
        {
            return *this <<= static_cast<int>(shift);
        }

        /// @brief Left shift with integral type
        template <typename T>
        constexpr int128_param_t operator<<(T shift) const noexcept
        {
            return *this << static_cast<int>(shift);
        }

        /// @brief Right shift assignment operator (arithmetic for signed TC, logical for unsigned and MS)
        constexpr int128_param_t &operator>>=(int shift) noexcept
        {
            if (shift <= 0)
            {
                return *this;
            }
            if (shift >= 128)
            {
                if constexpr (is_magnitude_sign && is_signed)
                {
                    // MS: Shift magnitude logically, preserve sign
                    uint64_t sign_bit = data[1] & (1ULL << 63);
                    data[0] = 0;
                    data[1] = sign_bit;
                }
                else if constexpr (is_signed)
                {
                    // TC signed: arithmetic shift - propagate sign bit
                    bool is_negative = (static_cast<int64_t>(data[1]) < 0);
                    data[0] = is_negative ? ~0ull : 0ull;
                    data[1] = is_negative ? ~0ull : 0ull;
                }
                else
                {
                    // Unsigned: logical shift - fill with 0s
                    data[0] = 0;
                    data[1] = 0;
                }
                return *this;
            }

            if (shift >= 64)
            {
                if constexpr (is_magnitude_sign && is_signed)
                {
                    // MS: Extract sign, shift magnitude as unsigned, restore sign
                    uint64_t sign_bit = data[1] & (1ULL << 63);
                    uint64_t mag_high = data[1] & ~(1ULL << 63);
                    // Shift magnitude (as if unsigned)
                    uint64_t new_low = mag_high >> (shift - 64);
                    data[0] = new_low;
                    // Restore sign
                    data[1] = sign_bit;
                }
                else if constexpr (is_signed)
                {
                    // TC signed: arithmetic shift
                    int64_t sign_extended = static_cast<int64_t>(data[1]) >> (shift - 64);
                    int64_t all_sign = static_cast<int64_t>(data[1]) >> 63;
                    data[0] = static_cast<uint64_t>(sign_extended);
                    data[1] = static_cast<uint64_t>(all_sign);
                }
                else
                {
                    // Unsigned: logical shift
                    uint64_t new_low = data[1] >> (shift - 64);
                    data[0] = new_low;
                    data[1] = 0;
                }
            }
            else
            {
                uint64_t new_low = (data[0] >> shift) | (data[1] << (64 - shift));
                if constexpr (is_magnitude_sign && is_signed)
                {
                    // MS: Extract sign, shift magnitude as unsigned, restore sign
                    uint64_t sign_bit = data[1] & (1ULL << 63);
                    uint64_t mag_high = data[1] & ~(1ULL << 63);
                    // Shift magnitude (as if unsigned)
                    uint64_t new_high = mag_high >> shift;
                    data[0] = new_low;
                    // Restore sign
                    data[1] = new_high | sign_bit;
                }
                else if constexpr (is_signed)
                {
                    // TC signed: arithmetic shift
                    int64_t sign_extended = static_cast<int64_t>(data[1]) >> shift;
                    data[0] = new_low;
                    data[1] = static_cast<uint64_t>(sign_extended);
                }
                else
                {
                    // Unsigned: logical shift
                    uint64_t new_high = data[1] >> shift;
                    data[0] = new_low;
                    data[1] = new_high;
                }
            }

            return *this;
        }

        /// @brief Right shift operator
        constexpr int128_param_t operator>>(int shift) const noexcept
        {
            int128_param_t result(*this);
            result >>= shift;
            return result;
        }

        /// @brief Right shift assignment with integral type
        template <typename T>
        constexpr int128_param_t &operator>>=(T shift) noexcept
        {
            return *this >>= static_cast<int>(shift);
        }

        /// @brief Right shift with integral type
        template <typename T>
        constexpr int128_param_t operator>>(T shift) const noexcept
        {
            return *this >> static_cast<int>(shift);
        }

        // =========================================================================
        // Bit Manipulation Functions (Priority 8)
        // =========================================================================

        /**
         * @brief Count trailing zero bits (from right/LSB)
         *
         * @return Number of consecutive zero bits starting from LSB
         *
         * @details
         * - Returns 128 if all bits are zero
         * - For MS signed: operates on magnitude only (ignores sign bit)
         * - Uses __builtin_ctzll for hardware optimization
         *
         * @example
         * uint128_tc_t(0, 8).trailing_zeros() == 3  // 0b1000 has 3 trailing zeros
         */
        constexpr int trailing_zeros() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: operate on magnitude (clear sign bit)
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                if (data[0] != 0)
                {
                    return __builtin_ctzll(data[0]);
                }
                if (mag_high != 0)
                {
                    return 64 + __builtin_ctzll(mag_high);
                }
                return 127; // All magnitude bits are zero (only sign bit may be set)
            }
            else
            {
                // TC/unsigned: standard implementation
                if (data[0] != 0)
                {
                    return __builtin_ctzll(data[0]);
                }
                if (data[1] != 0)
                {
                    return 64 + __builtin_ctzll(data[1]);
                }
                return 128; // All bits zero
            }
        }

        /**
         * @brief Count leading zero bits (from left/MSB)
         *
         * @return Number of consecutive zero bits starting from MSB
         *
         * @details
         * - Returns 128 if all bits are zero
         * - For MS signed: counts in magnitude only (127 bits)
         * - For TC signed: counts including sign bit (128 bits)
         *
         * @example
         * uint128_tc_t(0, 1).leading_zeros() == 127  // Only LSB set
         */
        constexpr int leading_zeros() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: count in magnitude (127 bits, MSB is sign)
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                if (mag_high != 0)
                {
                    return __builtin_clzll(mag_high) - 1; // -1 because MSB is sign
                }
                if (data[0] != 0)
                {
                    return 63 + __builtin_clzll(data[0]);
                }
                return 127; // All magnitude bits zero
            }
            else
            {
                // TC/unsigned: standard implementation
                if (data[1] != 0)
                {
                    return __builtin_clzll(data[1]);
                }
                if (data[0] != 0)
                {
                    return 64 + __builtin_clzll(data[0]);
                }
                return 128; // All bits zero
            }
        }

        /**
         * @brief Get position of highest set bit (1-based)
         *
         * @return Position of highest set bit, or 0 if value is zero
         *
         * @details
         * - For MS: operates on magnitude only (127 bits)
         * - For TC/unsigned: operates on full 128 bits
         * - Equivalent to: total_bits - leading_zeros() (for non-zero values)
         *
         * @example
         * uint128_tc_t(0, 8).bit_width() == 4  // 0b1000 -> bit 4
         */
        constexpr int bit_width() const noexcept
        {
            if (is_zero())
            {
                return 0;
            }

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: bit_width in 127-bit magnitude
                return 127 - leading_zeros();
            }
            else
            {
                // TC/unsigned: full 128 bits
                return 128 - leading_zeros();
            }
        }

        /**
         * @brief Check if value is a power of 2
         *
         * @return true if exactly one bit is set, false otherwise
         *
         * @details
         * - For MS signed: checks magnitude only (negative powers of 2 return false)
         * - Zero returns false
         *
         * @example
         * uint128_tc_t(0, 8).is_power_of_2() == true   // 8 = 2^3
         * uint128_tc_t(0, 6).is_power_of_2() == false  // 6 = 0b110
         */
        constexpr bool is_power_of_2() const noexcept
        {
            if (is_zero())
            {
                return false;
            }

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: check magnitude only, ignore sign
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                const uint64_t mag_low{data[0]};

                // Power of 2: exactly one bit set in magnitude
                if (mag_high != 0)
                {
                    return (mag_high & (mag_high - 1)) == 0 && mag_low == 0;
                }
                else
                {
                    return (mag_low & (mag_low - 1)) == 0;
                }
            }
            else
            {
                // TC/unsigned: standard implementation
                // Power of 2: n & (n-1) == 0 for n != 0
                const int128_param_t minus_one{*this};
                int128_param_t temp{*this};
                temp -= int128_param_t{0, 1};

                // Check if exactly one bit set
                if (data[1] != 0)
                {
                    return (data[1] & (data[1] - 1)) == 0 && data[0] == 0;
                }
                else
                {
                    return (data[0] & (data[0] - 1)) == 0;
                }
            }
        }

        /**
         * @brief Count number of bits set to 1 (population count)
         *
         * @return Number of bits set to 1
         *
         * @details
         * - For MS signed: counts bits in magnitude only (127 bits)
         * - Uses __builtin_popcountll for hardware optimization
         *
         * @example
         * uint128_tc_t(0, 0b1101).count_ones() == 3
         */
        constexpr int count_ones() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: count in magnitude only (ignore sign bit)
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                return __builtin_popcountll(data[0]) + __builtin_popcountll(mag_high);
            }
            else
            {
                // TC/unsigned: count all bits
                return __builtin_popcountll(data[0]) + __builtin_popcountll(data[1]);
            }
        }

        /**
         * @brief Alias for count_ones() (STL-compatible name)
         */
        constexpr int popcount() const noexcept
        {
            return count_ones();
        }

        /**
         * @brief Rotate bits left (circular shift)
         *
         * @param shift Number of positions to rotate (modulo 128)
         * @return New value with bits rotated left
         *
         * @details
         * - For MS signed: rotates magnitude, preserves sign bit
         * - Bits shifted out from left re-enter at right
         *
         * @example
         * uint128_tc_t(0x8000000000000000ULL, 0x1).rotate_left(1)
         *   -> uint128_tc_t(0x0000000000000000ULL, 0x3)
         */
        constexpr int128_param_t rotate_left(int shift) const noexcept
        {
            shift &= 127; // Normalize to [0, 127]
            if (shift == 0)
            {
                return *this;
            }

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: rotate magnitude, preserve sign
                const uint64_t sign_bit{data[1] & (1ULL << 63)};
                uint64_t mag_high{data[1] & ~(1ULL << 63)};
                uint64_t mag_low{data[0]};

                // Rotate 127-bit magnitude
                if (shift >= 64)
                {
                    // Major rotation: swap limbs
                    const int sub_shift{shift - 64};
                    const uint64_t new_high{(mag_low << sub_shift) | (mag_high >> (63 - sub_shift))};
                    const uint64_t new_low{(mag_high << sub_shift) | (mag_low >> (63 - sub_shift))};
                    return int128_param_t{new_high | sign_bit, new_low};
                }
                else
                {
                    // Minor rotation
                    const uint64_t new_high{(mag_high << shift) | (mag_low >> (64 - shift))};
                    const uint64_t new_low{(mag_low << shift) | (mag_high >> (63 - shift))};
                    return int128_param_t{new_high | sign_bit, new_low & ~(1ULL << 63)};
                }
            }
            else
            {
                // TC/unsigned: standard 128-bit rotation
                if (shift >= 64)
                {
                    const int sub_shift{shift - 64};
                    const uint64_t new_high{(data[0] << sub_shift) | (data[1] >> (64 - sub_shift))};
                    const uint64_t new_low{(data[1] << sub_shift) | (data[0] >> (64 - sub_shift))};
                    return int128_param_t{new_high, new_low};
                }
                else
                {
                    const uint64_t new_high{(data[1] << shift) | (data[0] >> (64 - shift))};
                    const uint64_t new_low{(data[0] << shift) | (data[1] >> (64 - shift))};
                    return int128_param_t{new_high, new_low};
                }
            }
        }

        /**
         * @brief Rotate bits right (circular shift)
         *
         * @param shift Number of positions to rotate (modulo 128)
         * @return New value with bits rotated right
         *
         * @details
         * - For MS signed: rotates magnitude, preserves sign bit
         * - Bits shifted out from right re-enter at left
         *
         * @example
         * uint128_tc_t(0, 0x8000000000000001ULL).rotate_right(1)
         *   -> uint128_tc_t(0x8000000000000000ULL, 0x4000000000000000ULL)
         */
        constexpr int128_param_t rotate_right(int shift) const noexcept
        {
            shift &= 127; // Normalize to [0, 127]
            if (shift == 0)
            {
                return *this;
            }

            // Rotate right is same as rotate left by (128 - shift)
            return rotate_left(128 - shift);
        }

        // =========================================================================
        // Helper Methods (Priority 9)
        // =========================================================================

        /**
         * @brief Division with remainder (divmod operation)
         *
         * @param divisor The divisor
         * @return Pair of (quotient, remainder)
         *
         * @details
         * Efficient combined division and modulo operation.
         * Representation-aware for MS (operates on magnitude).
         *
         * @example
         * auto [quot, rem] = uint128_tc_t{100}.divmod(uint128_tc_t{7});
         * // quot = 14, rem = 2
         */
        constexpr std::pair<int128_param_t, int128_param_t> divmod(const int128_param_t &divisor) const noexcept
        {
            const int128_param_t quotient{*this / divisor};
            const int128_param_t remainder{*this % divisor};
            return {quotient, remainder};
        }

        /**
         * @brief Get absolute value (magnitude)
         *
         * @return Absolute value of this number
         *
         * @details
         * - For unsigned types: returns self
         * - For TC signed: negates if negative
         * - For MS signed: returns magnitude directly
         */
        constexpr int128_param_t abs() const noexcept
        {
            if constexpr (!is_signed)
            {
                return *this;
            }
            else if constexpr (is_magnitude_sign)
            {
                // MS: clear sign bit to get magnitude
                int128_param_t result{*this};
                result.data[1] &= ~(1ULL << 63);
                return result;
            }
            else
            {
                // TC: negate if negative
                return is_negative() ? -*this : *this;
            }
        }

        /**
         * @brief Swap with another value
         *
         * @param other Value to swap with
         */
        constexpr void swap(int128_param_t &other) noexcept
        {
            const uint64_t temp_low{data[0]};
            const uint64_t temp_high{data[1]};
            data[0] = other.data[0];
            data[1] = other.data[1];
            other.data[0] = temp_low;
            other.data[1] = temp_high;
        }

        // =========================================================================
        // Friend Operators for Symmetric Operations (Priority 9)
        // =========================================================================

        /**
         * @brief Friend addition operator for mixed-type operations
         *
         * @details Enables: int128 + int, int + int128, etc.
         */
        template <typename T>
        friend constexpr int128_param_t operator+(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs + int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator+(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} + rhs;
        }

        /**
         * @brief Friend subtraction operator for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator-(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs - int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator-(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} - rhs;
        }

        /**
         * @brief Friend multiplication operator for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator*(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs * int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator*(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} * rhs;
        }

        /**
         * @brief Friend comparison operators for mixed-type operations
         */
        template <typename T>
        friend constexpr bool operator==(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs == int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator==(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} == rhs;
        }

        template <typename T>
        friend constexpr bool operator!=(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs != int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator!=(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} != rhs;
        }

        template <typename T>
        friend constexpr bool operator<(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs < int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator<(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} < rhs;
        }

        template <typename T>
        friend constexpr bool operator<=(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs <= int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator<=(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} <= rhs;
        }

        template <typename T>
        friend constexpr bool operator>(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs > int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator>(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} > rhs;
        }

        template <typename T>
        friend constexpr bool operator>=(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs >= int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator>=(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} >= rhs;
        }

        /**
         * @brief Friend bitwise AND for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator&(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs & int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator&(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} & rhs;
        }

        /**
         * @brief Friend bitwise OR for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator|(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs | int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator|(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} | rhs;
        }

        /**
         * @brief Friend bitwise XOR for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator^(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs ^ int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator^(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} ^ rhs;
        }

        /**
         * @brief Friend swap function (ADL-findable)
         */
        friend constexpr void swap(int128_param_t &a, int128_param_t &b) noexcept
        {
            a.swap(b);
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
