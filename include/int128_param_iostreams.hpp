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
// @file       int128_param_iostreams.hpp
// @brief      Stream I/O operators for int128_param_t<S, F>
// @author     Julián Calderón Almendros
// @date       2026-01-19 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_IOSTREAMS_HPP
#define INT128_PARAM_IOSTREAMS_HPP

#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

namespace nstd
{
    // ========================================================================
    // STREAM OUTPUT OPERATOR (operator<<)
    // ========================================================================

    /**
     * @brief Stream output operator
     * @param os Output stream
     * @param value Value to output
     * @return Reference to output stream
     *
     * @details
     * - Respects stream flags (hex, oct, dec)
     * - Respects width, fill, alignment
     * - Representation-aware (uses is_negative() internally)
     *
     * @note For EK: Outputs real value (not stored value).
     *       Uses is_negative() which is EK-aware.
     */
    template <signedness S, representation_form F>
    std::ostream &operator<<(std::ostream &os, const int128_param_t<S, F> &value)
    {
        // Get stream format flags
        const std::ios_base::fmtflags flags = os.flags();

        // Determine base from stream flags
        int base = 10;
        if (flags & std::ios_base::hex)
        {
            base = 16;
        }
        else if (flags & std::ios_base::oct)
        {
            base = 8;
        }

        // Convert to string using member function
        std::string str = value.to_string(base);

        // Step 1: Handle uppercase/lowercase (for hex)
        // to_string() returns uppercase, so convert to lowercase by default
        const bool use_uppercase = (flags & std::ios_base::uppercase) != 0;
        if (base == 16 && !use_uppercase)
        {
            // Convert to lowercase (default for hex)
            for (char &c : str)
            {
                if (c >= 'A' && c <= 'F')
                {
                    c = c - 'A' + 'a';
                }
            }
        }

        // Step 2: Handle prefixes (showbase flag)
        if (flags & std::ios_base::showbase)
        {
            if (base == 16 && str[0] != '-')
            {
                str = (use_uppercase ? "0X" : "0x") + str;
            }
            else if (base == 16 && str[0] == '-')
            {
                str = (use_uppercase ? "-0X" : "-0x") + str.substr(1);
            }
            else if (base == 8 && str != "0" && str[0] != '-')
            {
                str = "0" + str;
            }
            else if (base == 8 && str[0] == '-' && str != "-0")
            {
                str = "-0" + str.substr(1);
            }
        }

        // Step 3: Handle showpos flag (show + for positive)
        if ((flags & std::ios_base::showpos) && str[0] != '-' && str[0] != '0')
        {
            str = "+" + str;
        }

        // Handle width and alignment
        const std::streamsize width = os.width();
        if (width > 0 && static_cast<std::streamsize>(str.size()) < width)
        {
            const char fill_char = os.fill();
            const std::streamsize fill_count = width - str.size();

            if (flags & std::ios_base::left)
            {
                // Left alignment
                str.append(fill_count, fill_char);
            }
            else if (flags & std::ios_base::internal)
            {
                // Internal alignment (after sign/prefix)
                std::size_t insert_pos = 0;
                if (str[0] == '-' || str[0] == '+')
                {
                    insert_pos = 1;
                }
                if (str.substr(insert_pos, 2) == "0x" || str.substr(insert_pos, 2) == "0X")
                {
                    insert_pos += 2;
                }
                str.insert(insert_pos, fill_count, fill_char);
            }
            else
            {
                // Right alignment (default)
                str.insert(0, fill_count, fill_char);
            }
        }

        // Output and reset width
        os << str;
        os.width(0);

        return os;
    }

    // ========================================================================
    // STREAM INPUT OPERATOR (operator>>)
    // ========================================================================
    
    namespace detail {
        template <typename T>
        parse_result<T> parse_string_with_base(const char* str, int base) noexcept
        {
            parse_result<T> result{};

            if (!str) {
                result.error = parse_error::null_pointer;
                result.error_index = 0;
                return result;
            }

            if (*str == '\0') {
                result.error = parse_error::empty_string;
                result.error_index = 0;
                return result;
            }

            const char* ptr = str;
            size_t index = 0;
            bool is_negative = false;

            if constexpr (T::is_signed) {
                if (*ptr == '-') {
                    is_negative = true;
                    ++ptr;
                    ++index;
                } else if (*ptr == '+') {
                    ++ptr;
                    ++index;
                }
            }

            // Auto-detect base from prefix if not specified
            if (base == 0 || base == 16 || base == 8 || base == 2) {
                if (*ptr == '0' && *(ptr + 1) != '\0') {
                    char next = *(ptr + 1);
                    if ((next == 'x' || next == 'X') && (base == 0 || base == 16)) {
                        base = 16;
                        ptr += 2;
                        index += 2;
                    } else if ((next == 'b' || next == 'B') && (base == 0 || base == 2)) {
                        base = 2;
                        ptr += 2;
                        index += 2;
                    } else if (next >= '0' && next <= '7' && (base == 0 || base == 8)) {
                        base = 8;
                    }
                }
            }
             if (base == 0) base = 10;


            int128_param_t<signedness::signed_type, representation_form::twos_complement> temp_val{};
            bool found_digit = false;
            size_t digit_start_index = index;

            while (*ptr != '\0') {
                unsigned digit = 0;
                char c = *ptr;

                if (c >= '0' && c <= '9') { digit = c - '0'; }
                else if (c >= 'a' && c <= 'z') { digit = c - 'a' + 10; }
                else if (c >= 'A' && c <= 'Z') { digit = c - 'A' + 10; }
                else {
                    result.error = parse_error::invalid_character;
                    result.error_index = index;
                    return result;
                }

                if (digit >= static_cast<unsigned>(base)) {
                    result.error = parse_error::digit_out_of_range;
                    result.error_index = index;
                    return result;
                }

                found_digit = true;
                auto old_value = temp_val;
                temp_val = temp_val * base + digit;
                
                if (temp_val < old_value && digit != 0) {
                    result.error = parse_error::overflow;
                    result.error_index = index;
                    return result;
                }

                ++ptr;
                ++index;
            }

            if (!found_digit) {
                result.error = parse_error::no_digits;
                result.error_index = digit_start_index;
                return result;
            }

            if (is_negative) {
                temp_val = -temp_val;
            }

            // Convert to target representation
            result.value = T(temp_val);
            result.error = parse_error::success;
            return result;
        }
    } // namespace detail


    /**
     * @brief Stream input operator
     * @param is Input stream
     * @param value Value to read into
     * @return Reference to input stream
     *
     * @details
     * - Respects stream flags (hex, oct, dec)
     * - Auto-detects base from prefix (0x, 0)
     * - Sets failbit on parse error
     * - Representation-aware
     */
    template <signedness S, representation_form F>
    std::istream &operator>>(std::istream &is, int128_param_t<S, F> &value)
    {
        // Skip leading whitespace
        is >> std::ws;

        // Read string from stream
        std::string str;
        char c;
        while (is.get(c))
        {
            if (std::isspace(c))
            {
                is.unget();
                break;
            }
            str += c;

            // Stop at reasonable length
            if (str.size() > 130)
                break;
        }

        if (str.empty())
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }

        // Determine base from stream flags
        const std::ios_base::fmtflags flags = is.flags();
        int base = 0; // 0 means auto-detect from prefix
        if (flags & std::ios_base::hex) base = 16;
        else if (flags & std::ios_base::oct) base = 8;
        else if (flags & std::ios_base::dec) base = 10;
        

        auto result = detail::parse_string_with_base<int128_param_t<S, F>>(str.c_str(), base);

        if (result.success())
        {
            value = result.value;
        }
        else
        {
            is.setstate(std::ios_base::failbit);
        }

        return is;
    }


    // ========================================================================
    // CONVENIENCE FORMATTING FUNCTIONS
    // ========================================================================

    namespace iostreams
    {
        /**
         * @brief Format value with full control
         * @param value Value to format
         * @param base Numeric base (10, 16, 8, 2)
         * @param width Minimum width
         * @param fill Fill character
         * @param show_base Show base prefix (0x, 0, 0b)
         * @param show_pos Show + for positive
         * @param uppercase Use uppercase (hex)
         * @param left_align Align left
         * @return Formatted string
         */
        template <signedness S, representation_form F>
        std::string format(const int128_param_t<S, F> &value,
                           int base = 10,
                           int width = 0,
                           char fill = ' ',
                           bool show_base = false,
                           bool show_pos = false,
                           bool uppercase = false,
                           bool left_align = false)
        {
            std::ostringstream oss;

            // Set base
            if (base == 16)
                oss << std::hex;
            else if (base == 8)
                oss << std::oct;
            else
                oss << std::dec;

            // Set flags
            if (show_base)
                oss << std::showbase;
            if (show_pos)
                oss << std::showpos;
            if (uppercase)
                oss << std::uppercase;
            if (left_align)
                oss << std::left;
            else
                oss << std::right;

            // Set width and fill
            if (width > 0)
                oss << std::setw(width) << std::setfill(fill);

            // Output value
            oss << value;

            return oss.str();
        }

        /**
         * @brief Format as hexadecimal
         */
        template <signedness S, representation_form F>
        std::string hex(const int128_param_t<S, F> &value,
                        bool show_base = true,
                        bool uppercase = true)
        {
            return format(value, 16, 0, ' ', show_base, false, uppercase, false);
        }

        /**
         * @brief Format as octal
         */
        template <signedness S, representation_form F>
        std::string oct(const int128_param_t<S, F> &value,
                        bool show_base = true)
        {
            return format(value, 8, 0, ' ', show_base, false, false, false);
        }

        /**
         * @brief Format as decimal
         */
        template <signedness S, representation_form F>
        std::string dec(const int128_param_t<S, F> &value,
                        bool show_pos = false)
        {
            return format(value, 10, 0, ' ', false, show_pos, false, false);
        }

        /**
         * @brief Format as binary (custom base 2)
         */
        template <signedness S, representation_form F>
        std::string bin(const int128_param_t<S, F> &value,
                        bool show_base = true)
        {
            std::string result = value.to_string(2);
            if (show_base && result[0] != '-')
            {
                result = "0b" + result;
            }
            else if (show_base && result[0] == '-')
            {
                result = "-0b" + result.substr(1);
            }
            return result;
        }

    } // namespace iostreams

} // namespace nstd

#endif // INT128_PARAM_IOSTREAMS_HPP
