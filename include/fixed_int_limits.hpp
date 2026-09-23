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
// @file       fixed_int_limits.hpp
// @brief      std::numeric_limits para fixed_int_t<N, Sign, Form>
// @author     Julián Calderón Almendros
// @date       2026-08-23 (last edit)
// @version    1.90.0
// =============================================================================

// =============================================================================
// fixed_int_limits.hpp — std::numeric_limits for fixed_int_t<N, Sign, Form>
// Part of int128 Library - Phase 1.81 (Fase MS-INTEROP, T5)
// License: BSL-1.0
// =============================================================================
//
// std::numeric_limits is specializable for user-defined types per the C++
// standard (§17.6.4.2.1). We provide a partial specialization covering ALL
// (N, Sign, Form) combinations of fixed_int_t. It compiles for any valid
// instantiation — currently:
//   - <N, unsigned_type, binnat>          → uint_fixed_t<N>
//   - <N, signed_type,   twos_complement> → int_fixed_t<N>
// Future Forms (MS, EK) gained for fixed_int_t will be picked up automatically
// because we dispatch on Sign / Form via if constexpr — no extra specialization
// is required unless the bit layout of min/max differs (then add an additional
// partial specialization).
//
// Values (mirrors std::numeric_limits for fundamental integers):
//   digits         = 64*N        (unsigned)         | 64*N - 1   (signed)
//   digits10       = (digits * 30103) / 100000      (floor(digits * log10(2)))
//   min() / max()  delegate to fixed_int_t<...>::min() / ::max()
//   lowest()       = min()
//   is_signed      true iff Sign == signed_type
//   is_modulo      true iff Sign == unsigned_type   (signed overflow is UB)
//   is_integer     true; is_exact true; is_bounded true; radix = 2

#ifndef FIXED_INT_LIMITS_HPP
#define FIXED_INT_LIMITS_HPP

#include "fixed_width_int_t.hpp"

#include <limits>

namespace std
{

    /// @brief `std::numeric_limits` para `nstd::fixed_int_t`.
    ///
    /// Especializacion parcial generica sobre los tres parametros, de modo que
    /// todo `fixed_int_t` la tiene sin declararla una por una.
    ///
    /// Los miembros son los que exige el estandar y significan lo mismo que para
    /// cualquier entero: `min()`, `max()`, `digits`, `is_signed`, etcetera. Lo
    /// unico que conviene destacar:
    ///
    /// - `is_modulo` es **`true`** tambien para los tipos con signo. La
    ///   aritmetica de esta biblioteca envuelve en vez de ser comportamiento
    ///   indefinido, que es la diferencia deliberada con los `int` del lenguaje.
    /// - `digits` es `64 * N` sin signo y `64 * N - 1` con signo, descontando el
    ///   bit de signo.
    /// - No hay infinitos ni NaN: `has_infinity` y los `has_*_NaN` son `false`.
    ///
    /// @tparam N Numero de limbos de 64 bits.
    /// @tparam Sign Con o sin signo.
    /// @tparam Form Representacion interna.
    template <size_t N, ::nstd::signedness Sign, ::nstd::representation_form Form,
              ::nstd::overflow_policy Policy>
    class numeric_limits<::nstd::fixed_int_t<N, Sign, Form, Policy>>
    {
    public:
        /// @brief El propio tipo al que se refieren estos limites.
        using value_type = ::nstd::fixed_int_t<N, Sign, Form, Policy>;

        /// @name Miembros exigidos por el estandar
        /// Significan lo mismo que para cualquier entero; ver `<limits>`. Lo
        /// unico que se aparta de lo esperable esta explicado en la descripcion
        /// de la clase: `is_modulo` es `true` tambien con signo.
        /// @{

        /// @brief Siempre cierto: esta especializacion existe.
        static constexpr bool is_specialized = true;

        /// @brief Si el tipo admite negativos.
        static constexpr bool is_signed = (Sign == ::nstd::signedness::signed_type);

        /// @brief **Cierto**: es un entero. Es lo que lo separa de
        ///        `fixed_point_t`, donde vale `false` (ADR-022).
        static constexpr bool is_integer = true;

        /// @brief Cierto: la representacion es exacta.
        static constexpr bool is_exact = true;

        /// @brief No hay infinito: el rango es finito y cerrado.
        static constexpr bool has_infinity = false;
        /// @brief No hay NaN silencioso, y por eso el orden es total.
        static constexpr bool has_quiet_NaN = false;
        /// @brief Ni senalizador.
        static constexpr bool has_signaling_NaN = false;
        /// @brief No hay subnormales: no es un tipo de coma flotante.
        static constexpr float_denorm_style has_denorm = denorm_absent;
        /// @brief Y por tanto no hay perdida por subnormalidad.
        static constexpr bool has_denorm_loss = false;

        /// @brief Truncar hacia cero, que es lo que hace `operator/`.
        static constexpr float_round_style round_style = round_toward_zero;

        /// @brief No es IEEE-754: no lo pretende.
        static constexpr bool is_iec559 = false;
        /// @brief Acotado por arriba y por abajo.
        static constexpr bool is_bounded = true;

        /// @brief Cierto sin signo, falso con signo.
        ///
        /// @warning **Esto sigue la convencion del estandar, no el
        ///          comportamiento de este tipo.** Ahi `is_modulo` es falso con
        ///          signo porque desbordar con signo es comportamiento
        ///          indefinido; **aqui no lo es**: `overflow_policy::wrap` esta
        ///          definido y envuelve, con signo y sin el (ADR-007). Y con
        ///          `checked` no envuelve ni con uno ni con otro: marca.
        ///
        ///          Lo correcto seria `Policy == overflow_policy::wrap`, que es
        ///          lo que hace `numeric_limits` de `fixed_point_t` (ADR-022,
        ///          decision 6). No se cambia aqui porque es API publicada y el
        ///          cambio merece su propia entrega; esta anotado en
        ///          `NEXT_STEPS`.
        static constexpr bool is_modulo = !is_signed;

        /// @brief Bits de valor: `64*N`, menos uno si hay signo.
        static constexpr int digits = static_cast<int>(64 * N) - (is_signed ? 1 : 0);
        /// @brief Cifras decimales completas, `floor(digits * log10(2))`.
        static constexpr int digits10 = (digits * 30103) / 100000;
        /// @brief Cero, como en cualquier tipo exacto: `digits10` basta para
        ///        distinguir dos valores.
        static constexpr int max_digits10 = 0;
        /// @brief Base de la representacion.
        static constexpr int radix = 2;

        /// @name Sin exponente: es un entero
        /// @{

        /// @brief Cero: no hay exponente binario.
        static constexpr int min_exponent = 0;
        /// @brief Cero: no hay exponente decimal.
        static constexpr int min_exponent10 = 0;
        /// @brief Cero: no hay exponente binario.
        static constexpr int max_exponent = 0;
        /// @brief Cero: no hay exponente decimal.
        static constexpr int max_exponent10 = 0;
        /// @}

        /// @brief `trap` esta en el enum pero no escrita (ADR-009), asi que
        ///        ninguna instanciacion valida atrapa.
        static constexpr bool traps = false;
        /// @brief Sin subnormales, la pregunta no aplica.
        static constexpr bool tinyness_before = false;

        /// @brief El menor valor representable: `0` sin signo, `-2^(64N-1)`
        ///        con signo.
        ///
        /// @note En **Magnitud-Signo** es `-(2^(64N-1) - 1)`, uno mas alto: ese
        ///       minimo no tiene representacion alli y la conversion satura
        ///       (ADR-017). Es la unica asimetria real entre representaciones.
        static constexpr value_type min() noexcept { return value_type::min(); }

        /// @brief El mayor valor representable.
        static constexpr value_type max() noexcept { return value_type::max(); }

        /// @brief El mas negativo. En un entero coincide con `min()`.
        static constexpr value_type lowest() noexcept { return value_type::min(); }

        /// @brief **Cero**, como en cualquier entero: el concepto de «distancia
        ///        al siguiente representable» solo tiene sentido en un tipo con
        ///        redondeo. En `fixed_point_t` vale el ulp (ADR-022).
        static constexpr value_type epsilon() noexcept { return value_type::zero(); }

        /// @brief Cero: las operaciones exactas no redondean.
        static constexpr value_type round_error() noexcept { return value_type::zero(); }

        /// @name No existen: devuelven cero, como en cualquier entero
        /// @{

        /// @brief No existe: cero.
        static constexpr value_type infinity() noexcept { return value_type::zero(); }
        /// @brief No existe: cero.
        static constexpr value_type quiet_NaN() noexcept { return value_type::zero(); }
        /// @brief No existe: cero.
        static constexpr value_type signaling_NaN() noexcept { return value_type::zero(); }
        /// @brief No hay subnormales: cero.
        static constexpr value_type denorm_min() noexcept { return value_type::zero(); }
        /// @}
        /// @}
    };

} // namespace std

#endif // FIXED_INT_LIMITS_HPP
