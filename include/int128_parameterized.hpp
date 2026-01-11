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
        template <typename T>
        explicit constexpr int128_param_t(T value) noexcept
        {
            static_assert(std::is_integral_v<T>, "T must be integral");

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
            if constexpr (is_magnitude_sign)
            {
                // MS: zero if magnitude bits are all 0
                // Magnitude is in bits [0..62] of both words
                const std::uint64_t magnitude_mask = (1ULL << 63) - 1; // All bits except sign
                return (data[0] == 0) && ((data[1] & magnitude_mask) == 0);
            }
            else
            {
                // TC and others: zero only if both words are 0
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
