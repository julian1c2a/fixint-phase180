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
// @file       fixed_int_iostreams.hpp
// @brief      operator<< y operator>> de iostreams para fixed_int_t<N, Sign, Form>
// @author     Julián Calderón Almendros
// @date       2026-08-23
// @version    1.0.0
// =============================================================================
//
// T4.1 (auditoria 23 ago 2026). `int128_param_t` tenia iostreams desde hacia
// tiempo (int128_param_iostreams.hpp); `fixed_int_t` no, asi que no se podia
// ni imprimir con std::cout. Como el objetivo de la rama es que fixed_int_t
// ocupe el sitio de los tipos de 256 bits anteriores, esto es paridad, no
// adorno.
//
// Se respetan los manipuladores del flujo, igual que con un entero built-in:
//   base       dec / hex / oct                (basefield)
//   prefijo    showbase                       (0x, 0, 0b no existe en iostreams)
//   caja       uppercase                      (solo afecta a hex)
//   signo      showpos
//   relleno    width / fill / left / right / internal
//
// La lectura acepta signo, prefijo coherente con la base del flujo y para los
// tipos sin signo tambien '-' (como hacen los flujos con unsigned built-in:
// se lee la magnitud y se niega en modulo 2^(64N)).
// =============================================================================

#ifndef FIXED_INT_IOSTREAMS_HPP
#define FIXED_INT_IOSTREAMS_HPP

#include "fixed_width_int_t.hpp"

#include <istream>
#include <ostream>
#include <string>

namespace nstd
{

    // =========================================================================
    // Salida
    // =========================================================================

    template <std::size_t N, signedness Sign, representation_form Form>
    std::ostream &operator<<(std::ostream &os, const fixed_int_t<N, Sign, Form> &value)
    {
        const std::ios_base::fmtflags flags = os.flags();

        int base = 10;
        if ((flags & std::ios_base::basefield) == std::ios_base::hex)
            base = 16;
        else if ((flags & std::ios_base::basefield) == std::ios_base::oct)
            base = 8;

        // Con hex y oct, los flujos imprimen el patron de bits sin signo, igual
        // que con los enteros built-in: `std::cout << std::hex << -1` da ffffffff.
        std::string str;
        if (base == 10)
        {
            str = value.to_string();
        }
        else
        {
            str = uint_fixed_t<N>{value}.to_string(base);
        }

        const bool use_uppercase = (flags & std::ios_base::uppercase) != 0;
        if (base == 16 && !use_uppercase)
        {
            for (char &c : str)
                if (c >= 'A' && c <= 'F')
                    c = static_cast<char>(c - 'A' + 'a');
        }

        if (flags & std::ios_base::showbase)
        {
            const bool neg = !str.empty() && str[0] == '-';
            const std::string body = neg ? str.substr(1) : str;
            if (base == 16)
                str = (neg ? "-" : "") + std::string(use_uppercase ? "0X" : "0x") + body;
            else if (base == 8 && body != "0")
                str = (neg ? "-" : "") + std::string("0") + body;
        }

        if ((flags & std::ios_base::showpos) && !str.empty() && str[0] != '-' && !value.is_zero())
            str.insert(str.begin(), '+');

        const std::streamsize width = os.width();
        if (width > 0 && static_cast<std::streamsize>(str.size()) < width)
        {
            const char fill_char = os.fill();
            const std::size_t fill_count = static_cast<std::size_t>(width) - str.size();

            if ((flags & std::ios_base::adjustfield) == std::ios_base::left)
            {
                str.append(fill_count, fill_char);
            }
            else if ((flags & std::ios_base::adjustfield) == std::ios_base::internal)
            {
                // Detras del signo y del prefijo.
                std::size_t at = 0;
                if (at < str.size() && (str[at] == '-' || str[at] == '+'))
                    ++at;
                if (str.size() >= at + 2 && str[at] == '0' && (str[at + 1] == 'x' || str[at + 1] == 'X'))
                    at += 2;
                str.insert(at, fill_count, fill_char);
            }
            else
            {
                str.insert(0, fill_count, fill_char);
            }
        }

        os << str;
        os.width(0);
        return os;
    }

    // =========================================================================
    // Entrada
    // =========================================================================

    template <std::size_t N, signedness Sign, representation_form Form>
    std::istream &operator>>(std::istream &is, fixed_int_t<N, Sign, Form> &value)
    {
        const std::istream::sentry guard(is); // salta espacios si skipws
        if (!guard)
            return is;

        const std::ios_base::fmtflags flags = is.flags();
        int base = 10;
        if ((flags & std::ios_base::basefield) == std::ios_base::hex)
            base = 16;
        else if ((flags & std::ios_base::basefield) == std::ios_base::oct)
            base = 8;
        else if ((flags & std::ios_base::basefield) == std::ios_base::fmtflags{})
            base = 0; // sin basefield: se deduce del prefijo, como los built-in

        std::string token;
        std::istream::int_type ch = is.peek();

        if (ch != std::istream::traits_type::eof() &&
            (static_cast<char>(ch) == '+' || static_cast<char>(ch) == '-'))
        {
            token.push_back(static_cast<char>(ch));
            is.get();
            ch = is.peek();
        }

        // Prefijo 0x / 0b / 0o, si lo hay.
        if (ch != std::istream::traits_type::eof() && static_cast<char>(ch) == '0')
        {
            token.push_back('0');
            is.get();
            ch = is.peek();
            if (ch != std::istream::traits_type::eof())
            {
                const char k = static_cast<char>(ch);
                if (k == 'x' || k == 'X' || k == 'b' || k == 'B' || k == 'o' || k == 'O')
                {
                    token.push_back(k);
                    is.get();
                    ch = is.peek();
                }
            }
        }

        while (ch != std::istream::traits_type::eof())
        {
            const char c = static_cast<char>(ch);
            const bool is_digit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
            if (!is_digit)
                break;
            token.push_back(c);
            is.get();
            ch = is.peek();
        }

        if (token.empty())
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }

        // Para los tipos sin signo, '-' se acepta y se aplica en modulo 2^(64N),
        // igual que hacen los flujos con `unsigned int`.
        bool negate_unsigned = false;
        std::string body = token;
        if constexpr (Sign == signedness::unsigned_type)
        {
            if (body[0] == '-' || body[0] == '+')
            {
                negate_unsigned = (body[0] == '-');
                body.erase(body.begin());
            }
        }

        const auto parsed = fixed_int_t<N, Sign, Form>::try_from_string(body.c_str(), base);
        if (!parsed.success())
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }

        value = negate_unsigned ? -parsed.value : parsed.value;
        return is;
    }

} // namespace nstd

#endif // FIXED_INT_IOSTREAMS_HPP
