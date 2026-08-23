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
// @file       fixed_int_format.hpp
// @brief      std::formatter para fixed_int_t<N, Sign, Form>
// @author     Julián Calderón Almendros
// @date       2026-08-23
// @version    1.0.0
// =============================================================================
//
// T4.2 (auditoria 23 ago 2026). Espejo de int128_param_format.hpp para el tipo
// nuevo. Sin esto, `std::format("{}", x)` no compilaba con fixed_int_t.
//
// Especificacion soportada (la misma que los enteros built-in):
//
//   [[fill]align][sign][#][0][width][type]
//
//   fill   cualquier caracter (por defecto ' ')
//   align  '<' izquierda, '>' derecha (por defecto en numeros), '^' centrado
//   sign   '+' siempre, '-' solo negativos (por defecto), ' ' espacio si positivo
//   '#'    prefijo de base: 0x, 0X, 0b, 0B, 0
//   '0'    relleno con ceros entre el signo/prefijo y los digitos
//   width  ancho minimo del campo
//   type   'd' decimal (por defecto), 'x'/'X' hex, 'b'/'B' binario, 'o' octal
//
// Con los tipos con signo y una base distinta de 10 se imprime el signo y la
// magnitud ("-ff"), que es lo que hace std::format con los enteros built-in con
// signo -- a diferencia de iostreams, que imprime el patron de bits.
// =============================================================================

#ifndef FIXED_INT_FORMAT_HPP
#define FIXED_INT_FORMAT_HPP

#include "fixed_width_int_t.hpp"

#if __has_include(<format>)
#include <format>
#include <string>

template <std::size_t N, nstd::signedness Sign, nstd::representation_form Form, typename CharT>
struct std::formatter<nstd::fixed_int_t<N, Sign, Form>, CharT>
{
    char fill_char{' '};
    char align_char{'\0'}; // '\0' = por defecto (derecha en numeros)
    char sign_char{'-'};
    bool alt_form{false};
    bool zero_pad{false};
    int width{0};
    char type_char{'d'};

    constexpr auto parse(std::format_parse_context &ctx)
    {
        auto it{ctx.begin()};
        const auto end{ctx.end()};

        if (it == end || *it == '}')
            return it;

        // [[fill]align]
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
        if (it != end && (*it == 'd' || *it == 'x' || *it == 'X' || *it == 'b' || *it == 'B' || *it == 'o'))
        {
            type_char = *it;
            ++it;
        }

        if (it != end && *it != '}')
            throw std::format_error("especificador de formato invalido para fixed_int_t");

        return it;
    }

    auto format(const nstd::fixed_int_t<N, Sign, Form> &value, std::format_context &ctx) const
    {
        int base{10};
        switch (type_char)
        {
            case 'x':
            case 'X':
                base = 16;
                break;
            case 'b':
            case 'B':
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

        // to_string devuelve mayusculas; 'x' y 'b' las quieren en minuscula.
        if (type_char == 'x')
        {
            for (auto &c : digits)
                if (c >= 'A' && c <= 'F')
                    c = static_cast<char>(c - 'A' + 'a');
        }

        std::string prefix{};
        if (is_neg)
            prefix = "-";
        else if (sign_char == '+')
            prefix = "+";
        else if (sign_char == ' ')
            prefix = " ";

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
                case 'B':
                    prefix += "0B";
                    break;
                case 'o':
                    if (digits.empty() || digits[0] != '0')
                        prefix += "0";
                    break;
                default:
                    break;
            }
        }

        const auto content_len{static_cast<int>(prefix.size() + digits.size())};
        const int pad_count{(width > content_len) ? (width - content_len) : 0};
        const char eff_align{(align_char != '\0') ? align_char : '>'};

        auto out{ctx.out()};

        if (zero_pad && pad_count > 0 && align_char == '\0')
        {
            // El relleno con ceros va entre el prefijo y los digitos, y solo
            // cuando no se ha pedido alineacion explicita (como en la libreria
            // estandar).
            for (const char c : prefix)
                *out++ = c;
            for (int i{0}; i < pad_count; ++i)
                *out++ = '0';
            for (const char c : digits)
                *out++ = c;
        }
        else
        {
            const int left_pad{(eff_align == '>') ? pad_count : (eff_align == '^') ? (pad_count / 2) : 0};
            const int right_pad{pad_count - left_pad};

            for (int i{0}; i < left_pad; ++i)
                *out++ = fill_char;
            for (const char c : prefix)
                *out++ = c;
            for (const char c : digits)
                *out++ = c;
            for (int i{0}; i < right_pad; ++i)
                *out++ = fill_char;
        }

        return out;
    }
};

#endif // __has_include(<format>)

#endif // FIXED_INT_FORMAT_HPP
