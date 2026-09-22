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
// @file       fixed_point_traits_specializations.hpp
// @brief      Traits nstd::is_*, make_* y std::common_type para fixed_point_t
// @author     Julián Calderón Almendros
// @date       2026-09-23
// =============================================================================
//
// LO QUE ESTE TIPO **NO** ES
// --------------------------
// `nstd::is_integral_v<fixed_point_t>` es **falso**, y no por descuido: hay
// valores entre dos enteros consecutivos. Es lo mismo que dice
// `numeric_limits::is_integer` (ADR-022, decision 2).
//
// `is_arithmetic_v` **si** es cierto: suma, resta, multiplica y divide.
//
// Y se anade `nstd::is_fixed_point_v`, que es la pregunta que el codigo
// generico querra hacer de verdad: «¿tiene escala?».
// =============================================================================

#ifndef NSTD_FIXED_POINT_TRAITS_SPECIALIZATIONS_HPP
#define NSTD_FIXED_POINT_TRAITS_SPECIALIZATIONS_HPP

#include "fixed_int_traits_specializations.hpp"
#include "fixed_point_t.hpp"

#include <cstdint>
#include <type_traits>

namespace nstd
{
    // =========================================================================
    // is_arithmetic: si. is_integral: NO.
    // =========================================================================
    //
    // `is_integral` no se especializa a `true_type` a proposito: la primaria ya
    // da `false` y dejarlo asi es la respuesta correcta. Se documenta aqui para
    // que nadie lo lea como un olvido.

    /// @brief Un punto fijo **es** aritmetico: suma, resta, multiplica y divide.
    template <std::size_t N, std::size_t F, signedness S, representation_form Fm, overflow_policy P,
              rounding_mode R>
    struct is_arithmetic<fixed_point_t<N, F, S, Fm, P, R>> : std::true_type
    {
    };

    /// @brief Con signo.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct is_signed<fixed_point_t<N, F, signedness::signed_type, Fm, P, R>> : std::true_type
    {
    };

    /// @brief Sin signo.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct is_signed<fixed_point_t<N, F, signedness::unsigned_type, Fm, P, R>> : std::false_type
    {
    };

    /// @brief Sin signo.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct is_unsigned<fixed_point_t<N, F, signedness::unsigned_type, Fm, P, R>> : std::true_type
    {
    };

    /// @brief Con signo.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct is_unsigned<fixed_point_t<N, F, signedness::signed_type, Fm, P, R>> : std::false_type
    {
    };

    // =========================================================================
    // is_fixed_point: la pregunta que el codigo generico quiere hacer
    // =========================================================================

    /// @brief ¿Es un punto fijo? Primaria: no.
    template <typename T>
    struct is_fixed_point : std::false_type
    {
    };

    /// @brief Lo es.
    template <std::size_t N, std::size_t F, signedness S, representation_form Fm, overflow_policy P,
              rounding_mode R>
    struct is_fixed_point<fixed_point_t<N, F, S, Fm, P, R>> : std::true_type
    {
    };

    /// @brief Atajo de `is_fixed_point`.
    ///
    /// Distinguir «tiene escala» de «es entero» es lo que permite escribir una
    /// plantilla que valga para los dos sin preguntar por el tipo concreto.
    template <typename T>
    inline constexpr bool is_fixed_point_v = is_fixed_point<T>::value;

    // =========================================================================
    // make_signed / make_unsigned: cambian el SIGNO, no la escala
    // =========================================================================
    //
    // `F` se conserva: el hermano sin signo de un Q64.64 con signo es un Q64.64
    // sin signo, no otra cosa. Y `Form` pasa a `binnat` al quitar el signo,
    // porque ADR-011 ata sin-signo con binnat.

    /// @brief El hermano **con signo**, misma escala.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct make_signed<fixed_point_t<N, F, signedness::unsigned_type, Fm, P, R>>
    {
        /// @brief El tipo resultante.
        using type = fixed_point_t<N, F, signedness::signed_type, representation_form::twos_complement, P, R>;
    };

    /// @brief Ya lo tiene.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct make_signed<fixed_point_t<N, F, signedness::signed_type, Fm, P, R>>
    {
        /// @brief El tipo resultante.
        using type = fixed_point_t<N, F, signedness::signed_type, Fm, P, R>;
    };

    /// @brief El hermano **sin signo**, misma escala.
    ///
    /// @note `Form` pasa a `binnat`: sin signo no hay nada que codificar, y
    ///       ADR-011 ata las dos cosas.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct make_unsigned<fixed_point_t<N, F, signedness::signed_type, Fm, P, R>>
    {
        /// @brief El tipo resultante.
        using type = fixed_point_t<N, F, signedness::unsigned_type, representation_form::binnat, P, R>;
    };

    /// @brief Ya lo es.
    template <std::size_t N, std::size_t F, representation_form Fm, overflow_policy P, rounding_mode R>
    struct make_unsigned<fixed_point_t<N, F, signedness::unsigned_type, Fm, P, R>>
    {
        /// @brief El tipo resultante.
        using type = fixed_point_t<N, F, signedness::unsigned_type, Fm, P, R>;
    };

} // namespace nstd

namespace std
{
    // =========================================================================
    // common_type: el punto fijo gana
    // =========================================================================
    //
    // Mezclar un punto fijo con un entero del lenguaje da el punto fijo: es el
    // que puede representar al otro, no al reves. Es la misma direccion que
    // `common_type<double, int>`, que da `double`.

    /// @brief Punto fijo con entero del lenguaje: gana el punto fijo.
    template <std::size_t N, std::size_t F, ::nstd::signedness S, ::nstd::representation_form Fm,
              ::nstd::overflow_policy P, ::nstd::rounding_mode R, typename T>
        requires std::is_integral_v<T>
    struct common_type<::nstd::fixed_point_t<N, F, S, Fm, P, R>, T>
    {
        /// @brief El tipo resultante.
        using type = ::nstd::fixed_point_t<N, F, S, Fm, P, R>;
    };

    /// @brief Y en el otro orden.
    template <typename T, std::size_t N, std::size_t F, ::nstd::signedness S, ::nstd::representation_form Fm,
              ::nstd::overflow_policy P, ::nstd::rounding_mode R>
        requires std::is_integral_v<T>
    struct common_type<T, ::nstd::fixed_point_t<N, F, S, Fm, P, R>>
    {
        /// @brief El tipo resultante.
        using type = ::nstd::fixed_point_t<N, F, S, Fm, P, R>;
    };

    /// @brief Punto fijo con el entero de su misma familia: gana el punto fijo.
    template <std::size_t N, std::size_t F, ::nstd::signedness S, ::nstd::representation_form Fm,
              ::nstd::overflow_policy P, ::nstd::rounding_mode R, std::size_t N2, ::nstd::signedness S2,
              ::nstd::representation_form Fm2, ::nstd::overflow_policy P2>
    struct common_type<::nstd::fixed_point_t<N, F, S, Fm, P, R>, ::nstd::fixed_int_t<N2, S2, Fm2, P2>>
    {
        /// @brief El tipo resultante.
        using type = ::nstd::fixed_point_t<N, F, S, Fm, P, R>;
    };

    /// @brief Y en el otro orden.
    template <std::size_t N2, ::nstd::signedness S2, ::nstd::representation_form Fm2,
              ::nstd::overflow_policy P2, std::size_t N, std::size_t F, ::nstd::signedness S,
              ::nstd::representation_form Fm, ::nstd::overflow_policy P, ::nstd::rounding_mode R>
    struct common_type<::nstd::fixed_int_t<N2, S2, Fm2, P2>, ::nstd::fixed_point_t<N, F, S, Fm, P, R>>
    {
        /// @brief El tipo resultante.
        using type = ::nstd::fixed_point_t<N, F, S, Fm, P, R>;
    };

} // namespace std

#endif // NSTD_FIXED_POINT_TRAITS_SPECIALIZATIONS_HPP
