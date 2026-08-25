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
// @file       fixed_int_hash.hpp
// @brief      std::hash<fixed_int_t<N, Sign, Form>> — uso en contenedores no ordenados
// @author     Julián Calderón Almendros
// @date       2026-08-23
// @version    1.0.0
// =============================================================================
//
// T4.3 (auditoria 23 ago 2026). Sin esta especializacion, fixed_int_t no podia
// usarse como clave de std::unordered_map / unordered_set.
//
// Especializar std::hash para un tipo propio SI esta permitido por el estandar
// ([namespace.std]/2): es una de las excepciones explicitas, a diferencia de
// std::is_integral, que el proyecto resuelve con nstd::integral.
//
// Mezcla: constante y desplazamientos de splitmix64 / xxHash, aplicados limbo a
// limbo. Barato, buena dispersion en los bits bajos (que es lo que usan las
// tablas hash de la libreria estandar) y sin dependencias.
// =============================================================================

#ifndef FIXED_INT_HASH_HPP
#define FIXED_INT_HASH_HPP

#include "fixed_width_int_t.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace std
{

    /// @brief `std::hash` para `nstd::fixed_int_t`, de modo que sirva de clave
    ///        en `unordered_map` y `unordered_set`.
    ///
    /// Mezcla los N limbos con el finalizador de splitmix64, que dispersa bien un
    /// `uint64_t` sobre si mismo. Cumple el requisito del estandar: dos valores
    /// iguales dan el mismo hash.
    /// @tparam N Numero de limbos.
    /// @tparam Sign Con o sin signo.
    /// @tparam Form Representacion interna.
    template <std::size_t N, nstd::signedness Sign, nstd::representation_form Form>
    struct hash<nstd::fixed_int_t<N, Sign, Form>>
    {
        /// @brief Calcula el hash del valor.
        /// @param v Valor a dispersar.
        /// @return El hash.
        [[nodiscard]] std::size_t operator()(const nstd::fixed_int_t<N, Sign, Form> &v) const noexcept
        {
            // Finalizador de splitmix64: mezcla bien un uint64 en si mismo.
            constexpr auto mix = [](std::uint64_t x) noexcept -> std::uint64_t
            {
                x ^= x >> 30;
                x *= 0xBF58476D1CE4E5B9ULL;
                x ^= x >> 27;
                x *= 0x94D049BB133111EBULL;
                x ^= x >> 31;
                return x;
            };

            std::uint64_t h = 0x9E3779B97F4A7C15ULL; // phi * 2^64
            for (std::size_t i = 0; i < N; ++i)
            {
                h ^= mix(v.limb(i) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2));
                h *= 0x100000001B3ULL; // primo de FNV-1a de 64 bits
            }
            return static_cast<std::size_t>(h);
        }
    };

} // namespace std

#endif // FIXED_INT_HASH_HPP
