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
// @file       test_fixed_checked_saturating.cpp
// @brief      P1.3: checked_div y las tres saturating_*, y el cambio de API
// @date       2026-09-05
// =============================================================================
//
// Cierra el hueco que ADR-009 senalaba como bloqueante de ADR-006: faltaba
// `checked_div` y faltaban las tres `saturating_*`, y mientras faltaran,
// `int128_param_t` no podia retirarse.
//
// Y comprueba el cambio de API: las `checked_*` devuelven ya el propio tipo con
// politica `checked` en vez de `std::optional`. La seccion del encadenado es la
// que justifica el cambio: con `optional` no se podia escribir.
//
// TODAS LAS FUNCIONES SE LLAMAN, no solo se declaran: una plantilla que no se
// instancia puede tener errores y compilar igual. Paso en la primera version de
// `checked_div`, que llamaba a un `one_wrap()` inexistente y compilaba tan
// campante.
// =============================================================================

#include "fixed_width_int_t.hpp"
#include <cstdio>

using namespace nstd;
using U = uint_fixed_t<4>;
using I = int_fixed_t<4>;

static int fallos = 0;
static void ok(const char *que, bool b)
{
    std::printf("  %s %s\n", b ? "[OK]  " : "[FAIL]", que);
    if (!b)
        ++fallos;
}

int main()
{
    std::printf("=== checked_* devuelven el tipo con politica checked ===\n");
    {
        const auto r = checked_add(U{1}, U{2});
        ok("checked_add(1,2) vale y es 3", r.valid() && r == con_comprobacion(U{3}));
        ok("el tipo devuelto es checked, no optional",
           std::is_same_v<decltype(r), const uint_fixed_t<4, overflow_policy::checked>>);
    }
    ok("checked_add(max,1) marca", !checked_add(U::max(), U{1}).valid());
    ok("checked_sub(3,5) marca", !checked_sub(U{3}, U{5}).valid());
    ok("checked_sub(5,3) vale", checked_sub(U{5}, U{3}).valid());
    ok("checked_mul(max,2) marca", !checked_mul(U::max(), U{2}).valid());
    ok("checked_mul(3,4) vale y es 12", checked_mul(U{3}, U{4}) == con_comprobacion(U{12}));

    std::printf("\n=== se pueden ENCADENAR, que era el motivo del cambio ===\n");
    {
        // Con std::optional esto no se podia escribir sin desenvolver.
        const auto r = checked_add(U{1}, U{2}) * con_comprobacion(U{10});
        ok("checked_add(1,2)*10 == 30 y sigue valido", r.valid() && r == con_comprobacion(U{30}));
        const auto malo = checked_mul(U::max(), U{2}) + con_comprobacion(U{1});
        ok("la marca sobrevive al encadenar", !malo.valid());
    }

    std::printf("\n=== checked_div, que faltaba ===\n");
    ok("checked_div(10,3) == 3", checked_div(U{10}, U{3}) == con_comprobacion(U{3}));
    ok("checked_div(10,0) marca, y NO lanza", !checked_div(U{10}, U{}).valid());
    ok("checked_div con signo: -10/3 == -3", checked_div(I{-10}, I{3}) == con_comprobacion(I{-3}));
    ok("checked_div(min,-1) marca (unico desbordamiento de la division)",
       !checked_div(I::min(), -I::one()).valid());

    std::printf("\n=== saturating_*, que devuelven el tipo de siempre ===\n");
    ok("saturating_add(max,1) == max", saturating_add(U::max(), U{1}) == U::max());
    ok("saturating_add(1,2) == 3", saturating_add(U{1}, U{2}) == U{3});
    ok("el tipo devuelto es el de siempre (wrap)", std::is_same_v<decltype(saturating_add(U{1}, U{2})), U>);
    ok("saturating_sub(3,5) == 0 sin signo", saturating_sub(U{3}, U{5}) == U{});
    ok("saturating_mul(max,2) == max", saturating_mul(U::max(), U{2}) == U::max());
    ok("saturating_mul(3,4) == 12", saturating_mul(U{3}, U{4}) == U{12});

    ok("con signo: saturating_add(max,1) == max", saturating_add(I::max(), I{1}) == I::max());
    ok("con signo: saturating_add(min,-1) == min", saturating_add(I::min(), I{-1}) == I::min());
    ok("con signo: saturating_sub(min,1) == min", saturating_sub(I::min(), I{1}) == I::min());
    ok("con signo: saturating_mul(max,2) == max", saturating_mul(I::max(), I{2}) == I::max());
    ok("con signo: saturating_mul(max,-2) == min", saturating_mul(I::max(), I{-2}) == I::min());

    // =========================================================================
    // REGRESION del 5 sep 2026: el producto con signo se leia como sin signo.
    //
    // `producto_desborda` multiplicaba los PATRONES DE BITS y comparaba la mitad
    // alta contra la extension de signo. Para `(-1) * (-1)` la mitad alta del
    // producto sin signo sale llena de unos, asi que decia "desborda" sobre un
    // resultado que es 1. Faltaba la correccion de signo:
    //
    //     con_signo(a*b) = sin_signo(a*b) - (a<0 ? b<<64N : 0) - (b<0 ? a<<64N : 0)
    //
    // Contrastado despues contra __int128 (16,2 M de pares con N=1) y contra un
    // calculo por magnitud y signo (5,9 M de pares con N=2): cero discrepancias,
    // tanto en la marca como en el valor.
    // =========================================================================
    std::printf("\n=== el producto con signo se lee CON signo (regresion) ===\n");
    {
        using J = int_fixed_t<2>; // dos limbos: la cadena de prestamos importa

        ok("(-1)*(-1) == 1 y NO marca", checked_mul(I{-1}, I{-1}) == con_comprobacion(I{1}));
        ok("(-2)*(-3) == 6 y NO marca", checked_mul(I{-2}, I{-3}) == con_comprobacion(I{6}));
        ok("(-2)*3 == -6 y NO marca", checked_mul(I{-2}, I{3}) == con_comprobacion(I{-6}));
        ok("3*(-2) == -6 y NO marca", checked_mul(I{3}, I{-2}) == con_comprobacion(I{-6}));
        ok("min*(-1) SI marca", !checked_mul(I::min(), I{-1}).valid());
        ok("max*(-1) == -max y NO marca", checked_mul(I::max(), I{-1}) == con_comprobacion(-I::max()));
        ok("(-1)*0 == 0 y NO marca", checked_mul(I{-1}, I{}) == con_comprobacion(I{}));

        ok("N=2: (-1)*(-1) == 1 y NO marca", checked_mul(J{-1}, J{-1}) == con_comprobacion(J{1}));
        ok("N=2: (-7)*(-9) == 63 y NO marca", checked_mul(J{-7}, J{-9}) == con_comprobacion(J{63}));
        ok("N=2: min*(-1) SI marca", !checked_mul(J::min(), J{-1}).valid());
        ok("N=2: min*2 SI marca", !checked_mul(J::min(), J{2}).valid());

        // saturating_mul se apoya en checked_mul, asi que heredaba el fallo:
        // antes de la correccion, (-1)*(-1) saturaba a max en vez de dar 1.
        ok("saturating_mul(-1,-1) == 1", saturating_mul(I{-1}, I{-1}) == I{1});
        ok("saturating_mul(-3,-4) == 12", saturating_mul(I{-3}, I{-4}) == I{12});
        ok("saturating_mul(-3,4) == -12", saturating_mul(I{-3}, I{4}) == I{-12});
        ok("saturating_mul(min,-1) == max", saturating_mul(I::min(), I{-1}) == I::max());
    }

    std::printf("\n=== invalido(), la unica forma de fabricar una marca ===\n");
    {
        using C = uint_fixed_t<4, overflow_policy::checked>;
        ok("invalido() no vale", !C::invalido().valid());
        ok("invalido(x) conserva el valor dentro", C::invalido(con_comprobacion(U{42})).limb(0) == 42);
    }

    // =========================================================================
    // P1.5 tramo 2f: las siete aceptan CUALQUIER politica, y la marca es
    // PEGAJOSA.
    // =========================================================================
    //
    // Antes solo tomaban operandos `wrap`: `checked_add(a, b)` sobre un tipo
    // `checked` no compilaba, y codigo generico se rompia en cuanto alguien
    // cambiaba la politica del tipo. Lo destapo la matriz de paridad en su
    // primera pasada.
    std::printf("\n=== 2f: cualquier politica, y la marca no se limpia ===\n");
    {
        using CU = uint_fixed_t<4, overflow_policy::checked>;
        using CI = int_fixed_t<4, overflow_policy::checked>;

        const CU a{std::uint64_t{7}}, b{std::uint64_t{3}};
        ok("checked_add acepta checked", checked_add(a, b) == CU{std::uint64_t{10}});
        ok("checked_div acepta checked", checked_div(a, b) == CU{std::uint64_t{2}});
        ok("saturating_add acepta checked", saturating_add(a, b) == CU{std::uint64_t{10}});
        ok("saturating_add CONSERVA la politica", std::is_same_v<decltype(saturating_add(a, b)), CU>);
        ok("y con wrap sigue devolviendo wrap", std::is_same_v<decltype(saturating_add(U{7}, U{3})), U>);

        // EL CASO QUE DECIDE LA SEMANTICA. Un valor marcado guarda dentro el
        // resultado ENVUELTO, no el verdadero: `max()+1` deja un cero. Saturar
        // a partir de ahi NO da el valor saturado correcto, asi que limpiar la
        // marca seria afirmar que esta bien un numero que no lo esta -- y
        // despues no habria forma de saberlo.
        const CU marcado = CU::max() + CU{std::uint64_t{1}};
        ok("el operando llega marcado", !marcado.valid());
        ok("y lo que guarda es el envuelto (cero), no max()", marcado.limb(0) == 0 && marcado.limb(3) == 0);

        const CU r = saturating_add(marcado, CU{std::uint64_t{5}});
        ok("saturating_add sobre un marcado NO limpia la marca", !r.valid());
        ok("checked_add hereda la marca de entrada", !checked_add(marcado, CU{std::uint64_t{1}}).valid());
        ok("checked_div hereda la marca de entrada", !checked_div(marcado, CU{std::uint64_t{2}}).valid());

        // Pero saturar SIN marca previa no marca: saturar es el resultado
        // pedido, no un error.
        const CU sat = saturating_add(CU::max(), CU{std::uint64_t{1}});
        ok("saturar sin marca previa NO marca", sat.valid());
        ok("...y da max()", sat == CU::max());

        // Con signo, los dos extremos.
        ok("saturating_mul con signo y checked", saturating_mul(CI{-100}, CI{2}) == CI{-200});
        ok("saturating_sub(min,1) satura a min con checked", saturating_sub(CI::min(), CI{1}) == CI::min());
        ok("...y sin marcar, porque no habia marca previa", saturating_sub(CI::min(), CI{1}).valid());
    }

    std::printf("\n%s  (%d fallos)\n", fallos == 0 ? "TODO BIEN" : "HAY FALLOS", fallos);
    return fallos == 0 ? 0 : 1;
}
