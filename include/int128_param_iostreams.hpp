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

        // Handle prefixes (showbase flag)
        if (flags & std::ios_base::showbase)
        {
            if (base == 16 && str[0] != '-')
            {
                str = "0x" + str;
            }
            else if (base == 16 && str[0] == '-')
            {
                str = "-0x" + str.substr(1);
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

        // Handle showpos flag (show + for positive)
        if ((flags & std::ios_base::showpos) && str[0] != '-' && str[0] != '0')
        {
            str = "+" + str;
        }

        // Handle uppercase (for hex)
        if ((flags & std::ios_base::uppercase) && base == 16)
        {
            for (char &c : str)
            {
                if (c >= 'a' && c <= 'f')
                {
                    c = c - 'a' + 'A';
                }
            }
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
     * - Representation-aware (uses from_string() internally)
     *
     * @note For EK: Reads real value (not stored value).
     *       Uses from_string() which creates proper EK encoding.
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

            // Stop at reasonable length (max 130 chars for 128-bit in binary)
            if (str.size() > 130)
                break;
        }

        // Try to parse
        if (str.empty())
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }

        try
        {
            value = int128_param_t<S, F>::from_string(str.c_str());
        }
        catch (...)
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
