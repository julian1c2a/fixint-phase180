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
// @file       fixed_point_iostreams.hpp
// @brief      operator<< y operator>> para nstd::fixed_point_t
// @author     Julián Calderón Almendros
// @date       2026-09-23
// =============================================================================
//
// LA LECTURA SE APOYA EN `operator/`, Y ESO NO ES PEREZA
// -------------------------------------------------------
// Leer `123.456` es leer `123` y `456/1000`. Esa division **ya existe** y **ya
// redondea segun la perilla**, asi que reimplementarla aqui seria escribir por
// segunda vez la maquinaria de ADR-020 y arriesgarse a que las dos se separen.
//
// El limite de eso es que `10^k` tiene que caber en la parte entera del tipo:
// con `F == N` no cabe ninguno. Ahi se lee con la resolucion que haya y se
// documenta, que es mejor que una segunda implementacion.
// =============================================================================

#ifndef NSTD_FIXED_POINT_IOSTREAMS_HPP
#define NSTD_FIXED_POINT_IOSTREAMS_HPP

#include "fixed_int_iostreams.hpp"
#include "fixed_point_t.hpp"

#include <istream>
#include <ostream>
#include <string>

namespace nstd
{

    /// @brief Escribe el valor con la precision de la corriente.
    ///
    /// Respeta `std::setprecision`; si no se ha fijado ninguna, usa la de por
    /// omision de la corriente, que es **6**, igual que `printf("%f")`.
    ///
    /// @note Redondea, porque `to_string` redondea (ADR-020, decision 5).
    template <std::size_t N, std::size_t F, signedness Sign, representation_form Form, overflow_policy Policy,
              rounding_mode Redondeo>
    std::ostream &operator<<(std::ostream &os, const fixed_point_t<N, F, Sign, Form, Policy, Redondeo> &value)
    {
        const std::streamsize p = os.precision();
        const unsigned decimales = (p < 0) ? 6U : static_cast<unsigned>(p);
        return os << value.to_string(decimales);
    }

    /// @brief Lee un decimal con signo: `-12.345`.
    ///
    /// Acepta espacios delante, un signo opcional, cifras, y opcionalmente una
    /// coma decimal (`.`) con mas cifras. Si no hay ninguna cifra, pone
    /// `failbit` y **no toca** el destino, que es lo que hacen los operadores de
    /// la biblioteca estandar.
    ///
    /// @note **La fraccion se construye con `operator/`**, que ya redondea segun
    ///       la perilla. Asi la lectura y la aritmetica no pueden separarse.
    ///
    /// @note Sólo se usan las cifras decimales cuyo `10^k` **cabe en la parte
    ///       entera** del tipo; las de mas alla se descartan. Con `F == N` no
    ///       cabe ninguna potencia de diez y la parte fraccionaria se ignora
    ///       entera: ese tipo solo representa `[0, 1)` y leer texto en el pide
    ///       una conversion que no existe todavia.
    template <std::size_t N, std::size_t F, signedness Sign, representation_form Form, overflow_policy Policy,
              rounding_mode Redondeo>
    std::istream &operator>>(std::istream &is, fixed_point_t<N, F, Sign, Form, Policy, Redondeo> &value)
    {
        using T = fixed_point_t<N, F, Sign, Form, Policy, Redondeo>;

        std::istream::sentry centinela(is); // se come los espacios de delante
        if (!centinela)
            return is;

        bool negativo = false;
        int c = is.peek();
        if (c == '+' || c == '-')
        {
            negativo = (c == '-');
            is.get();
            c = is.peek();
        }

        // --- la parte entera ------------------------------------------------
        std::string enteras;
        while (c >= '0' && c <= '9')
        {
            enteras.push_back(static_cast<char>(is.get()));
            c = is.peek();
        }

        // --- la parte fraccionaria ------------------------------------------
        std::string decimales;
        if (c == '.')
        {
            is.get();
            c = is.peek();
            while (c >= '0' && c <= '9')
            {
                decimales.push_back(static_cast<char>(is.get()));
                c = is.peek();
            }
        }

        if (enteras.empty() && decimales.empty())
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }

        // --- montarlo -------------------------------------------------------
        T resultado{};
        if (!enteras.empty())
        {
            typename T::entero e{};
            for (char d : enteras)
            {
                e *= typename T::entero{std::uint64_t{10}};
                e += typename T::entero{static_cast<std::uint64_t>(d - '0')};
            }
            resultado = T::desde_crudo(e << static_cast<unsigned>(T::escala_bits));
        }

        if constexpr (F < N)
        {
            // Cuantas cifras caben. Se calcula, no se detecta: buscar el
            // desbordamiento comparando `10^k` con `10^(k-1)` falla si la
            // envoltura cae en un numero mayor, y eso es una suposicion que no
            // hace falta hacer.
            //
            // La parte entera tiene `64*(N-F)` bits, o sea
            // `64*(N-F)*log10(2)` cifras decimales. Con `F == N` da cero, que
            // es justo la limitacion documentada arriba.
            constexpr std::size_t max_cifras = (64U * (N - F) * 30103U) / 100000U;
            const std::size_t usadas = (decimales.size() < max_cifras) ? decimales.size() : max_cifras;

            if (usadas > 0)
            {
                const typename T::entero diez{std::uint64_t{10}};
                typename T::entero potencia = T::entero::one();
                typename T::entero num{};
                for (std::size_t i = 0; i < usadas; ++i)
                {
                    potencia *= diez;
                    num *= diez;
                    num += typename T::entero{static_cast<std::uint64_t>(decimales[i] - '0')};
                }
                // `operator/` del punto fijo: ya redondea segun la perilla, asi
                // que la lectura y la aritmetica no pueden separarse.
                const T a = T::desde_crudo(num << static_cast<unsigned>(T::escala_bits));
                const T b = T::desde_crudo(potencia << static_cast<unsigned>(T::escala_bits));
                resultado = resultado + (a / b);
            }
        }

        value = negativo ? -resultado : resultado;
        return is;
    }

} // namespace nstd

#endif // NSTD_FIXED_POINT_IOSTREAMS_HPP
