// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julian Calderon Almendros
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
// @file       test_is_modulo.cpp
// @brief      P3.8: `numeric_limits::is_modulo` sale de la politica, no del signo
// @date       2026-09-23
// =============================================================================
//
// UN RASGO NO SE COMPRUEBA CONTRA SU PROPIA DEFINICION
//
// Lo facil seria escribir `static_assert(is_modulo == (Policy == wrap))`. Eso no
// prueba nada: repite la linea que se acaba de escribir, con otra sintaxis. Si
// la linea esta mal, el test tambien.
//
// Lo que se comprueba aqui es que **el rasgo describa lo que el tipo hace**:
// donde dice `is_modulo`, que `max() + 1 == min()`; y donde dice que no, que no
// envuelva. El rasgo y el comportamiento, enfrentados.
// =============================================================================
#include "fixed_int_limits.hpp"
#include "fixed_point_limits.hpp"
#include "fixed_point_t.hpp"
#include "fixed_width_int_t.hpp"

#include <cstdio>
#include <limits>

using namespace nstd;

static int g_pasan = 0;
static int g_fallan = 0;

static void ok(const char *que, bool cond)
{
    if (cond)
    {
        ++g_pasan;
        std::printf("[ OK ] %s\n", que);
    }
    else
    {
        ++g_fallan;
        std::printf("[FAIL] %s\n", que);
    }
}

// --- los ocho tipos: {sin signo, con signo} x {las cuatro formas} x politica --
using U_wrap = fixed_int_t<2, signedness::unsigned_type, representation_form::binnat,
                           overflow_policy::wrap>;
using U_chk = fixed_int_t<2, signedness::unsigned_type, representation_form::binnat,
                          overflow_policy::checked>;
using I_wrap = fixed_int_t<2, signedness::signed_type, representation_form::twos_complement,
                           overflow_policy::wrap>;
using I_chk = fixed_int_t<2, signedness::signed_type, representation_form::twos_complement,
                          overflow_policy::checked>;
using MS_wrap = fixed_int_t<2, signedness::signed_type, representation_form::magnitude_sign,
                            overflow_policy::wrap>;
using MS_chk = fixed_int_t<2, signedness::signed_type, representation_form::magnitude_sign,
                           overflow_policy::checked>;
using EK_wrap = fixed_int_t<2, signedness::signed_type, representation_form::excess_k,
                            overflow_policy::wrap>;
using EK_chk = fixed_int_t<2, signedness::signed_type, representation_form::excess_k,
                           overflow_policy::checked>;

template <typename T>
static constexpr bool modulo_v = std::numeric_limits<T>::is_modulo;

// --- lo que el rasgo PROMETE, comprobado ------------------------------------
//
// `is_modulo` significa que la aritmetica es modulo 2^digitos. La forma de
// verlo sin rodeos: `max() + 1` vuelve a `min()`.
template <typename T>
static bool envuelve_de_verdad()
{
    const T m = T::max();
    const T uno = T::one();
    return (m + uno) == T::min();
}

int main()
{
    std::printf("====================================================================\n");
    std::printf("P3.8: is_modulo sale de la politica, no del signo\n");
    std::printf("====================================================================\n\n");

    // --- 1. el valor del rasgo -----------------------------------------------
    static_assert(modulo_v<U_wrap>, "sin signo con wrap: envuelve");
    static_assert(modulo_v<I_wrap>, "CON SIGNO y wrap: tambien envuelve (ADR-007)");
    static_assert(modulo_v<MS_wrap>, "Magnitud-Signo con wrap");
    static_assert(modulo_v<EK_wrap>, "Exceso-K con wrap");
    static_assert(!modulo_v<U_chk>, "sin signo con checked: NO envuelve, marca");
    static_assert(!modulo_v<I_chk>, "con signo y checked");
    static_assert(!modulo_v<MS_chk>, "Magnitud-Signo con checked");
    static_assert(!modulo_v<EK_chk>, "Exceso-K con checked");
    ok("el rasgo depende de la politica en las ocho combinaciones", true);

    // --- 2. Y QUE EL RASGO NO MIENTA ------------------------------------------
    //
    // Esto es lo que separa este test de repetir la definicion: se enfrenta el
    // rasgo al comportamiento. Si alguien cambiara `wrap` para que saturase, el
    // rasgo seguiria diciendo `true` y AQUI saltaria.
    ok("donde is_modulo es cierto, max()+1 vuelve a min(): sin signo",
       envuelve_de_verdad<U_wrap>());
    ok("donde is_modulo es cierto, max()+1 vuelve a min(): CON SIGNO",
       envuelve_de_verdad<I_wrap>());
    ok("... tambien en Magnitud-Signo", envuelve_de_verdad<MS_wrap>());
    ok("... tambien en Exceso-K", envuelve_de_verdad<EK_wrap>());

    // --- 3. y donde dice que NO, que de verdad no envuelva -------------------
    //
    // Con `checked` el desbordamiento no da la vuelta: marca el valor. Asi que
    // `max() + 1` NO es `min()`, es un valor invalido.
    {
        const U_chk s = U_chk::max() + U_chk::one();
        ok("con checked sin signo, max()+1 marca en vez de envolver",
           !s.valid() && !(s == U_chk::min() && s.valid()));
        const I_chk t = I_chk::max() + I_chk::one();
        ok("con checked con signo, max()+1 marca en vez de envolver", !t.valid());
    }

    // --- 4. la mitad del arreglo que se olvida: SIN SIGNO con checked --------
    //
    // La linea vieja era `!is_signed`, asi que decia `true` para
    // `uint + checked`, que NO envuelve. Los dos lados estaban mal, no solo el
    // del signo.
    ok("sin signo con checked NO es modular (la otra mitad del arreglo)",
       !modulo_v<U_chk>);

    // --- 5. coherencia con el punto fijo -------------------------------------
    //
    // ADR-022 ya lo resolvio bien ahi. Que las dos familias digan lo mismo es
    // la comprobacion de que el arreglo va en la direccion buena y no crea una
    // segunda convencion.
    {
        using FPw = fixed_point_t<2, 1, signedness::signed_type,
                                  representation_form::twos_complement, overflow_policy::wrap>;
        using FPc = fixed_point_t<2, 1, signedness::signed_type,
                                  representation_form::twos_complement, overflow_policy::checked>;
        static_assert(std::numeric_limits<FPw>::is_modulo, "punto fijo con wrap");
        static_assert(!std::numeric_limits<FPc>::is_modulo, "punto fijo con checked");
        ok("el entero y el punto fijo dicen lo mismo, con signo y wrap",
           modulo_v<I_wrap> == std::numeric_limits<FPw>::is_modulo);
        ok("el entero y el punto fijo dicen lo mismo, con checked",
           modulo_v<I_chk> == std::numeric_limits<FPc>::is_modulo);
    }

    // --- 6. y que el rasgo NO dependa de la representacion (ADR-018) ---------
    ok("las cuatro representaciones dan el mismo is_modulo con wrap",
       modulo_v<U_wrap> == modulo_v<I_wrap> && modulo_v<I_wrap> == modulo_v<MS_wrap> &&
           modulo_v<MS_wrap> == modulo_v<EK_wrap>);
    ok("las cuatro representaciones dan el mismo is_modulo con checked",
       modulo_v<U_chk> == modulo_v<I_chk> && modulo_v<I_chk> == modulo_v<MS_chk> &&
           modulo_v<MS_chk> == modulo_v<EK_chk>);

    // --- 7. lo que NO cambia: is_signed sigue siendo is_signed ---------------
    //
    // El arreglo desacopla `is_modulo` del signo. Conviene comprobar que no se
    // ha desacoplado de mas: `is_signed` tiene que seguir dependiendo del signo.
    static_assert(!std::numeric_limits<U_wrap>::is_signed, "");
    static_assert(std::numeric_limits<I_wrap>::is_signed, "");
    static_assert(!std::numeric_limits<U_chk>::is_signed, "");
    static_assert(std::numeric_limits<I_chk>::is_signed, "");
    ok("is_signed sigue dependiendo del signo, no de la politica", true);

    std::printf("\n====================================================================\n");
    std::printf("Results: %d passed, %d failed\n", g_pasan, g_fallan);
    std::printf("====================================================================\n");
    return g_fallan == 0 ? 0 : 1;
}
