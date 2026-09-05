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
// @file       test_fixed_policy_storage.cpp
// @brief      P1.1: el almacenamiento de la politica de desbordamiento
// @date       2026-09-05
// =============================================================================
//
// Comprueba lo que decide ADR-009: que la marca de invalido viva en un miembro
// que SOLO EXISTE con `checked`, de modo que quien usa `wrap` --que es la
// politica por defecto y la de todo el codigo anterior-- NO PAGUE NADA.
//
// Casi todo son `static_assert`: son propiedades del tipo, no del valor, y
// comprobarlas en compilacion es mas fuerte que comprobarlas al ejecutar.
//
// OJO CON LO QUE ESTE FICHERO **NO** COMPRUEBA. Aqui solo esta el
// almacenamiento. Que la marca se PROPAGUE por la aritmetica es P1.2 y todavia
// no esta escrito: hoy `valid()` de un `checked` devuelve siempre `true` porque
// nada la pone nunca. Cuando llegue P1.2, este fichero se amplia.
// =============================================================================

#include "fixed_width_int_t.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <type_traits>

// -----------------------------------------------------------------------------
// ESTE TEST COMPRUEBA CON assert() Y LA SUITE SE COMPILA CON -DNDEBUG.
// Sin este `#undef`, en release no verificaria nada. Ver P0.3.
// -----------------------------------------------------------------------------
#undef NDEBUG
#include <cassert>

using namespace nstd;

static int g_passed{0};
static int g_failed{0};

static void check(const char *que, bool ok)
{
    if (ok)
    {
        ++g_passed;
        std::cout << "  [OK]   " << que << "\n";
    }
    else
    {
        ++g_failed;
        std::cout << "  [FAIL] " << que << "\n";
    }
}

// =============================================================================
// 1. `wrap` no paga nada -- el corazon de ADR-009
// =============================================================================

template <std::size_t N>
static constexpr bool wrap_no_paga()
{
    using u = uint_fixed_t<N>;
    using i = int_fixed_t<N>;
    return sizeof(u) == 8 * N && sizeof(i) == 8 * N && alignof(u) == alignof(std::uint64_t) &&
           std::is_standard_layout_v<u> && std::is_trivially_copyable_v<u> && std::is_standard_layout_v<i> &&
           std::is_trivially_copyable_v<i>;
}

static_assert(wrap_no_paga<1>(), "N=1");
static_assert(wrap_no_paga<2>(), "N=2");
static_assert(wrap_no_paga<3>(), "N=3, impar");
static_assert(wrap_no_paga<4>(), "N=4");
static_assert(wrap_no_paga<8>(), "N=8");
static_assert(wrap_no_paga<16>(), "N=16");

// =============================================================================
// 2. El tipo de siempre y el de `wrap` son EL MISMO tipo
// =============================================================================
//
// No "equivalente" ni "compatible": el mismo. Es lo que garantiza que el codigo
// escrito antes de existir la politica siga significando exactamente lo mismo.

static_assert(
    std::is_same_v<uint_fixed_t<4>, fixed_int_t<4, signedness::unsigned_type, representation_form::binnat>>,
    "el alias de siempre sigue siendo el mismo tipo");

static_assert(std::is_same_v<uint256_fixed_t, uint_fixed_t<4, overflow_policy::wrap>>,
              "y wrap es explicitamente ese mismo");

static_assert(!std::is_same_v<uint_fixed_t<4>, uint_fixed_t<4, overflow_policy::checked>>,
              "checked SI es un tipo distinto");

// =============================================================================
// 3. `checked` paga exactamente un limbo, y ni uno mas
// =============================================================================

template <std::size_t N>
static constexpr bool checked_paga_un_limbo()
{
    using u = uint_fixed_t<N>;
    using c = uint_fixed_t<N, overflow_policy::checked>;
    // Un limbo mas, contando el relleno por alineamiento.
    return sizeof(c) == sizeof(u) + sizeof(std::uint64_t) && alignof(c) == alignof(u) &&
           std::is_standard_layout_v<c> && std::is_trivially_copyable_v<c>;
}

static_assert(checked_paga_un_limbo<1>(), "N=1");
static_assert(checked_paga_un_limbo<2>(), "N=2");
static_assert(checked_paga_un_limbo<4>(), "N=4");
static_assert(checked_paga_un_limbo<8>(), "N=8");

// El standard layout es lo que decidio la eleccion entre miembro condicional y
// clase base: con base, `checked` lo perdia en los cuatro compiladores.
static_assert(std::is_standard_layout_v<uint_fixed_t<4, overflow_policy::checked>>,
              "checked conserva el standard layout (por eso miembro y no base)");

// =============================================================================
// 4. La aritmetica de `wrap` sigue siendo constexpr y no cambia
// =============================================================================

static_assert(
    []
    {
        constexpr uint256_fixed_t a{1000000};
        constexpr auto b = a * a;
        return b / a == a;
    }(),
    "la aritmetica constexpr de siempre no cambia");

// =============================================================================
// 5. Los rasgos de la politica, consultables
// =============================================================================

static_assert(uint_fixed_t<4>::policy == overflow_policy::wrap, "por defecto, wrap");
static_assert(!uint_fixed_t<4>::comprueba_desbordamiento, "wrap no comprueba");
static_assert(uint_fixed_t<4, overflow_policy::checked>::comprueba_desbordamiento, "checked si comprueba");

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "P1.1 - almacenamiento de la politica de desbordamiento (ADR-009)\n";
    std::cout << "====================================================================\n\n";

    std::cout << "Tamano y disposicion: comprobados en compilacion (static_assert).\n";
    std::cout << "  uint_fixed_t<4>          = " << sizeof(uint_fixed_t<4>) << " bytes\n";
    std::cout << "  uint_fixed_t<4, checked> = " << sizeof(uint_fixed_t<4, overflow_policy::checked>)
              << " bytes\n\n";

    using u4 = uint_fixed_t<4>;
    using u4c = uint_fixed_t<4, overflow_policy::checked>;

    // ---- valid() -------------------------------------------------------------
    check("wrap: valid() es siempre cierto", u4{7}.valid() && u4{}.valid() && u4::max().valid());
    check("checked: un valor recien construido es valido", u4c{7}.valid());

    // ---- las dos conversiones con nombre --------------------------------------
    const u4 a{123456789};
    const auto c = con_comprobacion(a);
    check("con_comprobacion: conserva el numero", c.limb(0) == a.limb(0));
    check("con_comprobacion: nace valido", c.valid());

    const auto v = descartar_marca(c);
    check("descartar_marca: conserva el numero", v.limb(0) == a.limb(0));
    check("descartar_marca: devuelve el tipo wrap", std::is_same_v<decltype(v), const u4>);

    // La ida y vuelta no pierde nada.
    check("ida y vuelta wrap -> checked -> wrap", descartar_marca(con_comprobacion(a)) == a);

    // ---- que el valor sigue estando donde estaba ------------------------------
    // La conversion a bytes es parte del contrato de ADR-002 y ADR-003, y no la
    // puede haber roto el miembro nuevo.
    const auto bytes = static_cast<std::array<std::byte, 32>>(a);
    const u4 vuelta{bytes};
    check("la conversion a bytes y de vuelta sigue intacta con wrap", vuelta == a);

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
