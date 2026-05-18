// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
// @file       int128_param_format.hpp
// @brief      std::format support for int128_param_t<S, F>
// @author     Julián Calderón Almendros
// @date       2026-02-04 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_FORMAT_HPP
#define INT128_PARAM_FORMAT_HPP

#include "int128_parameterized.hpp"

#if __has_include(<format>)
#include <format>
#include <string>
#include <algorithm>

// ============================================================================
// std::formatter specialization for int128_param_t
// ============================================================================

/**
 * @brief std::formatter specialization for int128_param_t<S, F>
 *
 * @details Enables use of int128 types with std::format() and std::format_to().
 *
 * Supported standard format spec: [[fill]align][sign][#][0][width][type]
 *
 * - fill:  Any character (default ' ')
 * - align: '<' (left), '>' (right, default), '^' (center)
 * - sign:  '+' (always), '-' (negative only, default), ' ' (space for positive)
 * - '#':   Show base prefix (0x, 0X, 0b, 0)
 * - '0':   Zero-pad between sign/prefix and digits
 * - width: Minimum field width (integer)
 * - type:  'd' (decimal, default), 'x' (hex lower), 'X' (hex upper),
 *          'b' (binary), 'o' (octal)
 *
 * Examples:
 * @code
 * const uint128_t x{0, 255};
 * std::format("{}", x);        // "255"
 * std::format("{:>10}", x);    // "       255"
 * std::format("{:<10}", x);    // "255       "
 * std::format("{:^10}", x);    // "   255    "
 * std::format("{:0>10}", x);   // "0000000255"
 * std::format("{:010}", x);    // "0000000255"
 * std::format("{:#x}", x);     // "0xff"
 * std::format("{:#010x}", x);  // "0x000000ff"
 * std::format("{:+d}", int128_tc_t{42});  // "+42"
 * @endcode
 */
template <nstd::signedness S, nstd::representation_form F>
struct std::formatter<nstd::int128_param_t<S, F>>
{
    char fill_char{' '};
    char align_char{'\0'}; // '\0' = default (right for numbers)
    char sign_char{'-'};   // '-' = only show negative sign
    bool alt_form{false};
    bool zero_pad{false};
    int width{0};
    char type_char{'d'};

    constexpr auto parse(std::format_parse_context &ctx)
    {
        auto it{ctx.begin()};
        const auto end{ctx.end()};

        if (it == end || *it == '}')
        {
            return it;
        }

        // [[fill]align] — if second char is align, first is fill
        {
            auto peek{it};
            ++peek;
            if (peek != end && (*peek == '<' || *peek == '>' || *peek == '^'))
            {
                fill_char = *it;
                align_char = *peek;
                it = peek;
                ++it;
            }
            else if (*it == '<' || *it == '>' || *it == '^')
            {
                align_char = *it;
                ++it;
            }
        }

        // [sign]
        if (it != end && (*it == '+' || *it == '-' || *it == ' '))
        {
            sign_char = *it;
            ++it;
        }

        // [#]
        if (it != end && *it == '#')
        {
            alt_form = true;
            ++it;
        }

        // [0]
        if (it != end && *it == '0')
        {
            zero_pad = true;
            ++it;
        }

        // [width]
        while (it != end && *it >= '0' && *it <= '9')
        {
            width = width * 10 + (*it - '0');
            ++it;
        }

        // [type]
        if (it != end && (*it == 'd' || *it == 'x' || *it == 'X' ||
                          *it == 'b' || *it == 'o'))
        {
            type_char = *it;
            ++it;
        }

        if (it != end && *it != '}')
        {
            throw std::format_error("Invalid format specifier for int128_param_t");
        }

        return it;
    }

    auto format(const nstd::int128_param_t<S, F> &value, std::format_context &ctx) const
    {
        // Step 1: Convert to string
        int base{10};
        switch (type_char)
        {
        case 'x':
        case 'X':
            base = 16;
            break;
        case 'b':
            base = 2;
            break;
        case 'o':
            base = 8;
            break;
        default:
            base = 10;
            break;
        }

        std::string raw{value.to_string(base)};

        // Step 2: Separate sign from digits
        bool is_neg{false};
        std::string digits{};
        if (!raw.empty() && raw[0] == '-')
        {
            is_neg = true;
            digits = raw.substr(1);
        }
        else
        {
            digits = std::move(raw);
        }

        // Step 3: Case conversion for hex
        if (type_char == 'x')
        {
            for (auto &c : digits)
            {
                if (c >= 'A' && c <= 'F')
                {
                    c = static_cast<char>(c - 'A' + 'a');
                }
            }
        }

        // Step 4: Build sign/prefix
        std::string prefix{};
        if (is_neg)
        {
            prefix = "-";
        }
        else if (sign_char == '+')
        {
            prefix = "+";
        }
        else if (sign_char == ' ')
        {
            prefix = " ";
        }

        if (alt_form)
        {
            switch (type_char)
            {
            case 'x':
                prefix += "0x";
                break;
            case 'X':
                prefix += "0X";
                break;
            case 'b':
                prefix += "0b";
                break;
            case 'o':
                if (digits.empty() || digits[0] != '0')
                {
                    prefix += "0";
                }
                break;
            default:
                break;
            }
        }

        // Step 5: Compute padding
        const auto content_len{static_cast<int>(prefix.size() + digits.size())};
        const int pad_count{(width > content_len) ? (width - content_len) : 0};

        // Effective alignment: default '>' for numbers
        const char eff_align{(align_char != '\0') ? align_char : '>'};

        // Step 6: Build output
        auto out{ctx.out()};

        if (zero_pad && pad_count > 0 && eff_align == '>')
        {
            // Zero-pad goes between prefix and digits
            for (const char c : prefix)
            {
                *out++ = c;
            }
            for (int i{0}; i < pad_count; ++i)
            {
                *out++ = '0';
            }
            for (const char c : digits)
            {
                *out++ = c;
            }
        }
        else
        {
            const int left_pad{(eff_align == '>') ? pad_count : (eff_align == '^') ? (pad_count / 2)
                                                                                   : 0};
            const int right_pad{pad_count - left_pad};

            for (int i{0}; i < left_pad; ++i)
            {
                *out++ = fill_char;
            }
            for (const char c : prefix)
            {
                *out++ = c;
            }
            for (const char c : digits)
            {
                *out++ = c;
            }
            for (int i{0}; i < right_pad; ++i)
            {
                *out++ = fill_char;
            }
        }

        return out;
    }
};

#endif // __has_include(<format>)

#endif // INT128_PARAM_FORMAT_HPP
