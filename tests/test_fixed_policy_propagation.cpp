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
// @file       test_fixed_policy_propagation.cpp
// @brief      P1.2: propagacion de la marca, comparacion y to_string
// @date       2026-09-05
// =============================================================================
//
// Comprueba lo que ADR-008 y ADR-010 prometen: que un desbordamiento en
// CUALQUIER punto de una cadena envenene el resultado, que la marca se consulte
// una sola vez al final con valid(), y que los invalidos SE ORDENEN en vez de
// volverse incomparables como el NaN.
//
// El caso que de verdad importa es `a * b + c` con el desbordamiento en el
// producto: la suma posterior no desborda por si sola, y aun asi el resultado
// tiene que salir marcado.
//
// La ultima seccion es la mas importante de todas: que con `wrap` --la politica
// por defecto y la de todo el codigo anterior-- NO CAMBIE ABSOLUTAMENTE NADA.
// =============================================================================
#include "fixed_width_int_t.hpp"
#include <cstdio>
#include <map>
#include <algorithm>
#include <vector>

using namespace nstd;
using u4 = uint_fixed_t<4>;
using u4c = uint_fixed_t<4, overflow_policy::checked>;
using i4c = int_fixed_t<4, overflow_policy::checked>;

static int fallos = 0;
static void ok(const char *que, bool b)
{
    std::printf("  %s %s\n", b ? "[OK]  " : "[FAIL]", que);
    if (!b)
        ++fallos;
}

int main()
{
    std::printf("=== propagacion ===\n");

    // -- suma sin signo ------------------------------------------------------
    ok("wrap: max()+1 envuelve y sigue valido", (u4::max() + u4{1}).valid());
    ok("checked: max()+1 marca", !(u4c::max() + u4c{1}).valid());
    ok("checked: 2+2 no marca", (u4c{2} + u4c{2}).valid());

    // -- resta ---------------------------------------------------------------
    ok("checked: 0-1 marca (desborda por abajo)", !(u4c{0} - u4c{1}).valid());
    ok("checked: 5-3 no marca", (u4c{5} - u4c{3}).valid());

    // -- producto ------------------------------------------------------------
    ok("checked: max()*2 marca", !(u4c::max() * u4c{2}).valid());
    ok("checked: 1000*1000 no marca", (u4c{1000} * u4c{1000}).valid());

    // -- LA CADENA: a*b + c, con el desbordamiento en el producto -------------
    const u4c a = u4c::max();
    const u4c b{2};
    const u4c c{7};
    const u4c r = a * b + c;
    ok("CADENA a*b+c: la marca sobrevive a la suma posterior", !r.valid());

    // y que la suma posterior no la limpia aunque ella no desborde
    ok("CADENA: la suma final por si sola no desbordaba", (u4c{1} + c).valid());

    // -- desplazamiento ------------------------------------------------------
    ok("checked: 1<<10 no marca", (u4c{1} << 10).valid());
    ok("checked: max()<<1 marca", !(u4c::max() << 1).valid());

    // -- opuesto y ++/-- -----------------------------------------------------
    ok("checked sin signo: -x marca si x != 0", !(-u4c{5}).valid());
    ok("checked sin signo: -0 no marca", (-u4c{0}).valid());
    ok("checked con signo: -min() marca", !(-i4c::min()).valid());
    {
        u4c x = u4c::max();
        ++x;
        ok("checked: ++ sobre max() marca", !x.valid());
    }
    {
        u4c y{0};
        --y;
        ok("checked: -- sobre 0 marca", !y.valid());
    }

    // -- los op= tambien -----------------------------------------------------
    {
        u4c x = u4c::max();
        x += u4c{1};
        ok("checked: += marca igual que +", !x.valid());
        u4c z{3};
        z *= u4c{4};
        ok("checked: *= no marca si no desborda", z.valid() && z == u4c{12});
    }

    std::printf("\n=== comparacion (ADR-010) ===\n");
    const u4c malo = u4c::max() + u4c{1};
    const u4c bueno{5};
    ok("x == x es cierto tambien para un invalido", malo == malo);
    ok("un invalido es MAYOR que cualquier valido", malo > bueno);
    ok("y el orden es total: <=> da strong_ordering",
       std::is_same_v<decltype(malo <=> bueno), std::strong_ordering>);
    ok("dos invalidos con distinto valor NO son iguales", !((u4c::max() + u4c{1}) == (u4c::max() + u4c{2})));

    // std::map exige orden debil estricto: con NaN esto seria UB.
    std::map<u4c, int> m;
    m[bueno] = 1;
    m[malo] = 2;
    ok("std::map acepta invalidos sin romperse", m.size() == 2);

    std::vector<u4c> v{malo, bueno, u4c{9}};
    std::sort(v.begin(), v.end());
    ok("std::sort los ordena y el invalido queda al final", !v.back().valid());
    ok("max_element devuelve el invalido, no lo esconde", !std::max_element(v.begin(), v.end())->valid());

    std::printf("\n=== to_string ===\n");
    ok("un invalido lo dice, no da basura", malo.to_string() == "invalido");
    ok("un valido imprime su numero", bueno.to_string() == "5");

    std::printf("\n=== wrap no cambia en nada ===\n");
    ok("wrap: max()+1 sigue envolviendo a 0", (u4::max() + u4{1}) == u4{0});
    ok("wrap: to_string de max()+1 es 0", (u4::max() + u4{1}).to_string() == "0");

    std::printf("\n%s  (%d fallos)\n", fallos == 0 ? "TODO BIEN" : "HAY FALLOS", fallos);
    return fallos == 0 ? 0 : 1;
}
