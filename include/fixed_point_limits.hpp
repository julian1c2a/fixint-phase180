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
// @file       fixed_point_limits.hpp
// @brief      std::numeric_limits para nstd::fixed_point_t
// @author     Julián Calderón Almendros
// @date       2026-09-23
// =============================================================================
//
// EL PUNTO FIJO NO ES NI UN ENTERO NI UNA COMA FLOTANTE  (ADR-022)
// -----------------------------------------------------------------
// `std::numeric_limits` tiene una forma para los enteros y otra para la coma
// flotante, y la mitad de sus miembros significan algo distinto segun con cual
// se compare este tipo. Elegir mal no rompe la compilacion: **compila igual y
// hace otra cosa**.
//
// La regla que decide casi todo: **donde el tipo ya publica un miembro, esto
// tiene que coincidir con el**. `fixed_point_t` ya tiene `min()`, `max()` y
// `epsilon()`; dos formas de preguntar lo mismo tienen que dar lo mismo.
//
// Lo que se aparta de lo esperable, y por que:
//
//   min()        el MAS NEGATIVO, como en los enteros -- no el positivo mas
//                pequeno de la coma flotante. Para eso esta `epsilon()`.
//   is_integer   false, que es lo unico que lo separa del entero aqui
//   is_exact     TRUE: los valores son exactos aunque `*` y `/` redondeen
//   epsilon()    el ulp, y es ABSOLUTO: el mismo paso en todo el rango
//   digits       TODOS los bits de valor, no solo los fraccionarios
//   round_style  sale de la perilla `Redondeo`, no es fijo
//   is_modulo    sale de la POLITICA, no del signo
//
// =============================================================================

#ifndef NSTD_FIXED_POINT_LIMITS_HPP
#define NSTD_FIXED_POINT_LIMITS_HPP

#include "fixed_point_t.hpp"

#include <cstdint>
#include <limits>

namespace std
{

    /// @brief `std::numeric_limits` para `nstd::fixed_point_t` (ADR-022).
    ///
    /// Cubre las seis celdas de golpe --las cuatro representaciones y los dos
    /// signos-- porque nada de lo que hay aqui depende de `Form`: por
    /// [ADR-018](ADR-018) la representacion no se observa desde el
    /// comportamiento, y `min()`, `max()` y `epsilon()` son del **valor**.
    template <std::size_t N, std::size_t F, ::nstd::signedness Sign, ::nstd::representation_form Form,
              ::nstd::overflow_policy Policy, ::nstd::rounding_mode Redondeo>
    class numeric_limits<::nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo>>
    {
    public:
        /// @brief El tipo del que se habla.
        using value_type = ::nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo>;

        /// @brief Siempre cierto: esta especializacion existe.
        static constexpr bool is_specialized = true;
        /// @brief Si el tipo admite negativos.
        static constexpr bool is_signed = (Sign == ::nstd::signedness::signed_type);

        /// @brief **Falso**, y es lo unico que separa este tipo de un entero
        ///        desde aqui: hay valores entre dos enteros consecutivos.
        static constexpr bool is_integer = false;

        /// @brief Cierto: el conjunto representable es exacto --cada valor
        ///        es `k/2^(64F)`, sin aproximacion-- y eso es lo que mide este
        ///        miembro. Que el producto y la division redondeen no lo
        ///        cambia: `operator/` del entero tambien redondea y alli
        ///        `is_exact` es cierto.
        static constexpr bool is_exact = true;

        /// @brief No hay infinito: el rango es finito y cerrado.
        static constexpr bool has_infinity = false;
        /// @brief No hay NaN silencioso, y por eso `<=>` es un orden total.
        static constexpr bool has_quiet_NaN = false;
        /// @brief Ni senalizador.
        static constexpr bool has_signaling_NaN = false;
        /// @brief No hay subnormales: el paso es el mismo en todo el rango.
        static constexpr float_denorm_style has_denorm = denorm_absent;
        /// @brief Y por tanto no hay perdida por subnormalidad.
        static constexpr bool has_denorm_loss = false;
        /// @brief No es IEEE-754: no lo pretende.
        static constexpr bool is_iec559 = false;
        /// @brief Acotado por arriba y por abajo.
        static constexpr bool is_bounded = true;

        /// @brief Sale de la **politica**, no del signo.
        ///
        /// El entero de esta biblioteca lo pone a `!is_signed`, copiando la
        /// convencion del estandar, donde desbordar con signo es comportamiento
        /// indefinido. **Aqui no lo es**: `wrap` esta definido y envuelve, con
        /// signo y sin el (ADR-007). Y `checked` no envuelve: marca.
        static constexpr bool is_modulo = (Policy == ::nstd::overflow_policy::wrap);

        /// @brief **Todos** los bits de valor, no solo los fraccionarios.
        ///
        /// Se considero que contara los fraccionarios, por analogia con la
        /// mantisa. Se descarta: la mantisa mide precision RELATIVA, y aqui la
        /// precision no es relativa. En un tipo exacto, `digits` mide cuantos
        /// digitos en base `radix` caben, y son todos. Para los fraccionarios
        /// esta `value_type::escala_bits`.
        static constexpr int digits = static_cast<int>(64U * N) - (is_signed ? 1 : 0);
        /// @brief Cifras decimales completas que caben, `floor(digits * log10(2))`.
        static constexpr int digits10 = (digits * 30103) / 100000;

        /// @brief Cifras decimales para **distinguir** dos valores cualesquiera.
        ///
        /// @note No confundir con «imprimir sin perder nada». La expansion
        ///       decimal de `k/2^(64F)` **termina** y llega hasta `64*F` cifras
        ///       tras la coma; para eso esta `to_string(64*F)`. Aqui se responde
        ///       lo otro.
        static constexpr int max_digits10 = digits10 + 2;

        /// @brief Base de la representacion: dos, como el nombre «punto fijo
        ///        binario» indica.
        static constexpr int radix = 2;

        /// @name Sin exponente: ese es el punto del punto fijo
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

        /// @brief El modo de redondeo, **leido de la perilla**.
        ///
        /// @warning El enum del estandar **no distingue los dos modos «al mas
        ///          cercano»**: solo tiene `round_to_nearest`, asi que
        ///          `to_nearest_even` y `to_nearest_away` caen en el mismo
        ///          sitio. Es una perdida de informacion del estandar, no de
        ///          aqui: **no se puede deducir el desempate de `round_style`**.
        static constexpr float_round_style round_style =
            (Redondeo == ::nstd::rounding_mode::toward_zero)      ? round_toward_zero
            : (Redondeo == ::nstd::rounding_mode::toward_neg_inf) ? round_toward_neg_infinity
            : (Redondeo == ::nstd::rounding_mode::toward_pos_inf) ? round_toward_infinity
                                                                  : round_to_nearest;

        /// @brief El **mas negativo**, como en los enteros. Sin signo, cero.
        ///
        /// @warning **No es el positivo mas pequeno**, que es lo que devuelve
        ///          `min()` en coma flotante. Un algoritmo generico que use esto
        ///          como «el mas pequeno» hara cosas distintas segun la
        ///          respuesta **y compilara en los dos casos** (ADR-022).
        ///          El positivo mas pequeno es `epsilon()`.
        static constexpr value_type min() noexcept { return value_type::min(); }

        /// @brief El mayor valor representable.
        static constexpr value_type max() noexcept { return value_type::max(); }

        /// @brief El mas negativo. Aqui coincide con `min()`, al contrario que
        ///        en coma flotante, y por la misma razon que da la decision 1
        ///        de ADR-022.
        static constexpr value_type lowest() noexcept { return value_type::min(); }

        /// @brief El **ulp**, y es **absoluto**.
        ///
        /// En coma flotante `epsilon` es relativo --la distancia de 1 al
        /// siguiente, que crece con la magnitud--. Aqui el paso es el mismo en
        /// todo el rango, asi que cumple tres cosas a la vez:
        ///
        ///     epsilon() == el paso entre dos consecutivos, en cualquier punto
        ///               == el valor positivo mas pequeno
        ///               == denorm_min()
        ///
        /// Eso es lo que hace que el error absoluto de una suma redondeada este
        /// acotado por `epsilon/2` **en todo el rango**, cosa que en coma
        /// flotante no pasa.
        ///
        /// @note Con `F == 0` vale **uno**, mientras que
        ///       `numeric_limits<int>::epsilon()` vale cero. Se mantiene el uno
        ///       porque `value_type::epsilon()` ya vale uno, y tener dos
        ///       respuestas seria peor que apartarse de la convencion entera en
        ///       un borde.
        static constexpr value_type epsilon() noexcept { return value_type::epsilon(); }

        /// @brief El error maximo de un redondeo, **medido en ULPs**.
        ///
        /// `0.5` al mas cercano y `1` con los modos dirigidos, igual que
        /// `numeric_limits<float>::round_error()` vale `0.5`.
        ///
        /// @note **Se mide en ULPs, no en unidades del tipo.** La primera
        ///       version de esto devolvia «medio ulp» --`epsilon()/2`-- y el
        ///       test la desmintio en una linea: **medio ulp no es
        ///       representable**, porque el ulp ES el valor positivo mas
        ///       pequeno. Que la mitad de la unidad minima no quepa es la
        ///       prueba de que aquella lectura era la equivocada.
        ///
        /// @note Los dos bordes:
        ///       - Con `F == 0` no hay parte fraccionaria y `0.5` no cabe:
        ///         devuelve **cero**, igual que los enteros.
        ///       - Con `F == N` el uno no cabe --el tipo llega hasta `[0,1)`--
        ///         y los modos dirigidos devuelven `max()`. Sin signo eso es
        ///         exactamente `1 - epsilon`, que es el supremo real del error
        ///         de truncar; con signo se queda corto, y ese tipo
        ///         --`[-0.5, 0.5)`-- es de por si una rareza.
        static constexpr value_type round_error() noexcept
        {
            constexpr bool al_mas_cercano = (Redondeo == ::nstd::rounding_mode::to_nearest_even ||
                                             Redondeo == ::nstd::rounding_mode::to_nearest_away);
            if constexpr (al_mas_cercano)
            {
                if constexpr (F == 0)
                    return value_type::zero(); // medio no cabe en un entero
                else
                    return value_type::desde_crudo(typename value_type::entero{std::uint64_t{1}}
                                                   << static_cast<unsigned>(value_type::escala_bits - 1U));
            }
            else if constexpr (F < N)
            {
                return value_type{1};
            }
            else
            {
                return value_type::max();
            }
        }

        /// @name No existen: devuelven cero, como en los enteros
        /// @{

        /// @brief No existe: cero.
        static constexpr value_type infinity() noexcept { return value_type::zero(); }
        /// @brief No existe: cero.
        static constexpr value_type quiet_NaN() noexcept { return value_type::zero(); }
        /// @brief No existe: cero.
        static constexpr value_type signaling_NaN() noexcept { return value_type::zero(); }
        /// @}

        /// @brief No hay subnormales, pero el positivo mas pequeno si existe y
        ///        es `epsilon()`. Los enteros hacen lo analogo con `min()`.
        static constexpr value_type denorm_min() noexcept { return value_type::epsilon(); }
    };

} // namespace std

#endif // NSTD_FIXED_POINT_LIMITS_HPP
