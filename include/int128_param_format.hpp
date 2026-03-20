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
#include <format>
#include <string>

// ============================================================================
// std::formatter specialization for int128_param_t
// ============================================================================

/**
 * @brief std::formatter specialization for int128_param_t<S, F>
 *
 * @details Enables use of int128 types with std::format()
 *
 * Supported format specifiers:
 * - 'd' or none: Decimal (default)
 * - 'x': Lowercase hexadecimal (0x prefix)
 * - 'X': Uppercase hexadecimal (0X prefix)
 * - 'b': Binary (0b prefix)
 * - 'o': Octal (0 prefix)
 *
 * Examples:
 * @code
 * uint128_t x{0, 255};
 * std::format("{}", x);      // "255"
 * std::format("{:x}", x);    // "0xff"
 * std::format("{:X}", x);    // "0XFF"
 * std::format("{:b}", x);    // "0b11111111"
 * std::format("{:o}", x);    // "0377"
 * @endcode
 */
template <nstd::signedness S, nstd::representation_form F>
struct std::formatter<nstd::int128_param_t<S, F>>
{
    /**
     * @brief Format presentation type
     */
    char presentation{'d'}; // 'd'=decimal, 'x'=hex, 'X'=HEX, 'b'=binary, 'o'=octal

    /**
     * @brief Parse format specifier
     *
     * @param ctx Format parse context
     * @return Iterator past the parsed format spec
     *
     * @details Parses format string and extracts presentation type
     */
    constexpr auto parse(std::format_parse_context &ctx)
    {
        auto it{ctx.begin()};
        const auto end{ctx.end()};

        if (it != end && (*it == 'd' || *it == 'x' || *it == 'X' ||
                          *it == 'b' || *it == 'o'))
        {
            presentation = *it;
            ++it;
        }

        if (it != end && *it != '}')
        {
            throw std::format_error("Invalid format specifier for int128_param_t");
        }

        return it;
    }

    /**
     * @brief Format value to output
     *
     * @param value Value to format
     * @param ctx Format context
     * @return Iterator to end of output
     *
     * @details Formats int128 value according to presentation type
     */
    auto format(const nstd::int128_param_t<S, F> &value, std::format_context &ctx) const
    {
        std::string result{};

        switch (presentation)
        {
        case 'd':
        default:
            // Decimal (default)
            result = value.to_string(10);
            break;

        case 'x':
            // Lowercase hexadecimal with 0x prefix
            result = value.to_string(16);
            break;

        case 'X':
        {
            // Uppercase hexadecimal with 0X prefix
            result = value.to_string(16);
            // Convert to uppercase
            for (auto &c : result)
            {
                if (c >= 'a' && c <= 'f')
                {
                    c = static_cast<char>(c - 'a' + 'A');
                }
                else if (c == 'x')
                {
                    c = 'X';
                }
            }
            break;
        }

        case 'b':
            // Binary with 0b prefix
            result = value.to_string(2);
            break;

        case 'o':
        {
            // Octal with 0 prefix
            result = value.to_string(8);
            break;
        }
        }

        return std::format_to(ctx.out(), "{}", result);
    }
};

#endif // INT128_PARAM_FORMAT_HPP
