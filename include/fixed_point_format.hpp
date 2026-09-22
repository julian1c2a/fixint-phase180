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
// @file       fixed_point_format.hpp
// @brief      std::formatter para nstd::fixed_point_t
// @author     Julián Calderón Almendros
// @date       2026-09-23
// =============================================================================
//
// LO QUE SE ADMITE
// ----------------
//     {}          seis cifras, como `printf("%f")`
//     {:.3}       tres cifras
//     {:.0}       sin coma
//     {:>12}      alineado a la derecha en 12 columnas
//     {:<12.2}    a la izquierda, dos cifras
//     {:^12}      centrado
//
// NO se admiten `{:x}`, `{:b}` ni las demas presentaciones enteras: en un punto
// fijo una base distinta de diez pediria decidir que se hace con la parte
// fraccionaria, y eso no esta decidido. Pedirlas es un error de formato, no una
// salida rara.
// =============================================================================

#ifndef NSTD_FIXED_POINT_FORMAT_HPP
#define NSTD_FIXED_POINT_FORMAT_HPP

#include "fixed_point_t.hpp"

#include <format>
#include <string>

/// @brief `std::formatter` para `nstd::fixed_point_t`.
template <std::size_t N, std::size_t F, nstd::signedness Sign, nstd::representation_form Form,
          nstd::overflow_policy Policy, nstd::rounding_mode Redondeo, typename CharT>
struct std::formatter<nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo>, CharT>
{
    /// Cifras tras la coma. Seis por omision, como `printf("%f")`.
    unsigned precision = 6;
    /// Ancho minimo del campo; cero si no se pide.
    std::size_t ancho = 0;
    /// `<`, `>` o `^`.
    CharT alineacion = CharT('>');
    /// Con que se rellena.
    CharT relleno = CharT(' ');

    /// @brief Lee la especificacion de formato.
    /// @param ctx El contexto de analisis.
    /// @return El iterador tras la especificacion.
    /// @throws std::format_error si se pide una presentacion que no existe.
    constexpr auto parse(std::basic_format_parse_context<CharT> &ctx)
    {
        auto it = ctx.begin();
        const auto fin = ctx.end();
        if (it == fin || *it == CharT('}'))
            return it;

        // --- relleno y alineacion -------------------------------------------
        // El relleno va DELANTE de la alineacion, asi que hay que mirar dos
        // caracteres antes de decidir.
        if (it + 1 != fin && (*(it + 1) == CharT('<') || *(it + 1) == CharT('>') || *(it + 1) == CharT('^')))
        {
            relleno = *it;
            alineacion = *(it + 1);
            it += 2;
        }
        else if (*it == CharT('<') || *it == CharT('>') || *it == CharT('^'))
        {
            alineacion = *it;
            ++it;
        }

        // --- ancho -----------------------------------------------------------
        std::size_t w = 0;
        bool hay_ancho = false;
        while (it != fin && *it >= CharT('0') && *it <= CharT('9'))
        {
            w = w * 10 + static_cast<std::size_t>(*it - CharT('0'));
            ++it;
            hay_ancho = true;
        }
        if (hay_ancho)
            ancho = w;

        // --- precision -------------------------------------------------------
        if (it != fin && *it == CharT('.'))
        {
            ++it;
            unsigned p = 0;
            bool hay = false;
            while (it != fin && *it >= CharT('0') && *it <= CharT('9'))
            {
                p = p * 10 + static_cast<unsigned>(*it - CharT('0'));
                ++it;
                hay = true;
            }
            if (!hay)
                throw std::format_error("fixed_point_t: falta el numero tras el punto");
            precision = p;
        }

        if (it != fin && *it != CharT('}'))
            throw std::format_error("fixed_point_t: solo se admiten relleno, alineacion, "
                                    "ancho y precision; no hay presentaciones de base");
        return it;
    }

    /// @brief Escribe el valor.
    /// @param value El valor.
    /// @param ctx El contexto de salida.
    /// @return El iterador de salida tras escribir.
    auto format(const nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo> &value,
                std::format_context &ctx) const
    {
        // `to_string` ya redondea segun la perilla (ADR-020), asi que el
        // formateo no decide nada sobre el redondeo: solo coloca.
        const std::string s = value.to_string(precision);

        if (s.size() >= ancho)
            return std::format_to(ctx.out(), "{}", s);

        const std::size_t hueco = ancho - s.size();

        std::string izquierda;
        std::string derecha;
        if (alineacion == CharT('<'))
        {
            derecha.assign(hueco, static_cast<char>(relleno));
        }
        else if (alineacion == CharT('^'))
        {
            izquierda.assign(hueco / 2, static_cast<char>(relleno));
            derecha.assign(hueco - hueco / 2, static_cast<char>(relleno));
        }
        else
        {
            izquierda.assign(hueco, static_cast<char>(relleno));
        }
        return std::format_to(ctx.out(), "{}{}{}", izquierda, s, derecha);
    }
};

#endif // NSTD_FIXED_POINT_FORMAT_HPP
