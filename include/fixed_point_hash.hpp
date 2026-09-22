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
// @file       fixed_point_hash.hpp
// @brief      std::hash para nstd::fixed_point_t
// @author     Julián Calderón Almendros
// @date       2026-09-23
// =============================================================================

#ifndef NSTD_FIXED_POINT_HASH_HPP
#define NSTD_FIXED_POINT_HASH_HPP

#include "fixed_int_hash.hpp"
#include "fixed_point_t.hpp"

#include <cstddef>
#include <functional>

namespace std
{

    /// @brief `std::hash` para `nstd::fixed_point_t`.
    ///
    /// Se delega en el hash del entero de abajo, que es lo correcto **porque la
    /// escala es una constante del tipo**: dos valores del mismo tipo son
    /// iguales si y solo si sus crudos lo son, asi que dispersar el crudo
    /// dispersa el valor.
    ///
    /// @note El hash del entero mezcla `limb()`, o sea los bits **guardados**,
    ///       asi que el mismo valor en complemento a dos y en Exceso-K da
    ///       hashes distintos. **No es un problema**: son tipos distintos, y el
    ///       contrato de `std::hash` solo obliga dentro de un tipo. Lo que si se
    ///       cumple, que es lo que importa, es que `a == b` implica
    ///       `hash(a) == hash(b)` para cualquier par del mismo tipo.
    ///
    /// @note En Magnitud-Signo hay dos codificaciones del cero, y si `-0` fuera
    ///       alcanzable habria dos valores `==` con limbos distintos, lo que
    ///       romperia el contrato. **No lo es**: toda operacion canonicaliza a
    ///       `+0`. Comprobado, no supuesto.
    template <std::size_t N, std::size_t F, ::nstd::signedness Sign, ::nstd::representation_form Form,
              ::nstd::overflow_policy Policy, ::nstd::rounding_mode Redondeo>
    struct hash<::nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo>>
    {
        /// @brief Dispersa el valor.
        /// @param v Valor a dispersar.
        /// @return El hash.
        [[nodiscard]] std::size_t
        operator()(const ::nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo> &v) const noexcept
        {
            return std::hash<::nstd::fixed_int_t<N, Sign, Form, Policy>>{}(v.crudo());
        }
    };

} // namespace std

#endif // NSTD_FIXED_POINT_HASH_HPP
