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
// @file       benchmark_toom3.cpp
// @brief      De donde salen NSTD_TOOM3_MIN y NSTD_TOOM3_REC
// @date       2026-09-17
// =============================================================================
//
// Los dos umbrales de Toom-3 estan medidos, y este es el banco que los mide. Si
// no se puede volver a medir desde el arbol, la macro se convierte en la
// constante magica que este proyecto lleva toda la fase quitando.
//
// POR QUE SON DOS UMBRALES Y NO UNO
// ---------------------------------
// Toom-3 hace cinco productos de M/3 donde Karatsuba hace tres de M/2. Gana por
// arriba y pierde por abajo. Pero el mismo M aparece en dos papeles distintos:
// como producto de arriba, y como subproducto DENTRO de uno mayor. Y no se
// comporta igual en los dos:
//
//   - Entrar en Toom-3 con 512 limbos pierde un 5-8 %.
//   - Un subproducto de ~512 limbos dentro de un producto de 4096 sale MEJOR con
//     Toom-3 que con Karatsuba.
//
// La explicacion mas probable es la cache: a esa altura no queda nada util en
// L2, y los subproblemas de Toom-3 son de M/3 frente a los M/2 de Karatsuba.
//
// Por eso `NSTD_TOOM3_MIN` fija la ENTRADA (alto, para no meter regresiones) y
// `NSTD_TOOM3_REC` fija la RECURSION (bajo, que es de donde sale la ganancia).
//
// EL TOPE DE 2048, Y NO 4096
// --------------------------
// El rango llega a `NSTD_LIMBOS_MAX` = 4096, pero este banco para en 2048: a
// 4096 la pila de Toom-3 mas la de Karatsuba se acerca al mega de `SizeOfStackReserve`
// que Windows da por defecto, y `scripts/build_generic.py` no pide mas. Para
// medir el tope hay que enlazar con mas pila (`-Wl,--stack,33554432` en
// gcc/clang, `/STACK:33554432` en MSVC). Lo medido asi el 17 sep 2026 da 1,12x
// con corte en 96 y 1,17x-1,20x con corte en 256.
// =============================================================================

#include "algorithms/mul_kernels.hpp"
#include "bench_adaptativo.hpp"
#include "fixed_width_int_t.hpp" // por las macros, para medir la config viva

#include <array>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

// Pocos operandos: a M=2048 cada uno son 16 KB y el resultado 32 KB.
static constexpr std::size_t OPERANDOS = 4;
static constexpr std::size_t REPES = 20;

/// Un umbral por encima de cualquier anchura medida deja a Toom-3 sin entrar:
/// es Karatsuba puro, y sirve de linea base y de comprobacion del banco.
static constexpr std::size_t SIN_TOOM = 100000000;

template <std::size_t M>
static std::vector<std::array<std::uint64_t, M>> operandos(std::uint64_t semilla)
{
    std::vector<std::array<std::uint64_t, M>> v;
    v.reserve(OPERANDOS);
    std::uint64_t s = semilla;
    auto sig = [&s]() noexcept
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return s;
    };
    for (std::size_t i = 0; i < OPERANDOS; ++i)
    {
        std::array<std::uint64_t, M> x{};
        for (std::size_t j = 0; j < M; ++j)
            x[j] = sig();
        v.push_back(x);
    }
    return v;
}

/// Functor y no lambda: hacen falta varios tipos distintos en un `std::tuple`.
template <std::size_t M, std::size_t T>
struct con_umbral
{
    const std::vector<std::array<std::uint64_t, M>> *a;
    const std::vector<std::array<std::uint64_t, M>> *b;
    std::array<std::uint64_t, 2 * M> *s;

    void operator()(std::size_t i) const noexcept
    {
        *s = nstd::algorithms::kmul_full_gen<M, 8, T>((*a)[i % OPERANDOS], (*b)[i % OPERANDOS]);
        doNotOptimize((*s)[0]);
    }
};

template <std::size_t M>
static void barre()
{
    static const auto a = operandos<M>(0xA11CE + M * 7);
    static const auto b = operandos<M>(0xB0B + M * 13);
    static std::array<std::uint64_t, 2 * M> s{};

    const auto m = bench::mide_entrelazado(
        std::make_tuple(con_umbral<M, SIN_TOOM>{&a, &b, &s}, con_umbral<M, 96>{&a, &b, &s},
                        con_umbral<M, 256>{&a, &b, &s}, con_umbral<M, 512>{&a, &b, &s},
                        con_umbral<M, 1024>{&a, &b, &s}),
        REPES);

    int mejor = 1;
    for (int i = 2; i < 5; ++i)
        if (m[i].minimo < m[mejor].minimo)
            mejor = i;
    const int umbrales[5] = {0, 96, 256, 512, 1024};

    std::printf("| %5zu | %11.0f | %6.3fx | %6.3fx | %6.3fx | %6.3fx |  %4d |\n", M, m[0].minimo,
                m[0].minimo / m[1].minimo, m[0].minimo / m[2].minimo, m[0].minimo / m[3].minimo,
                m[0].minimo / m[4].minimo, umbrales[mejor]);
    std::fflush(stdout);

    char et[80];
    std::snprintf(et, sizeof(et), "toom3 mejor umbral M=%zu", M);
    bench_record(et, m[0].minimo / m[mejor].minimo, "x");
}

int main()
{
    print_header("los dos umbrales de Toom-3");

    std::printf("\nConfiguracion viva: NSTD_TOOM3_MIN=%d (entrada), NSTD_TOOM3_REC=%d\n"
                "(recursion). La primera columna es Karatsuba puro --Toom-3 sin entrar--\n"
                "y las cuatro siguientes son la razon Karatsuba/Toom-3 segun donde se\n"
                "corte. Por encima de 1,000x Toom-3 gana. %zu repeticiones, entrelazadas.\n\n",
                (int)NSTD_TOOM3_MIN, (int)NSTD_TOOM3_REC, REPES);
    std::printf("|     M |   Karatsuba |  K/T>=96 | K/T>=256 | K/T>=512 | K/T>=1024 | mejor |\n");
    std::printf("|------:|------------:|---------:|---------:|---------:|----------:|------:|\n");
    std::fflush(stdout);

    // M=512 es la comprobacion del banco: ahi Toom-3 pierde, y con corte en 1024
    // no llega a entrar, asi que esa casilla tiene que dar 1,000x +- ruido. Si
    // se desviara, el banco estaria midiendo otra cosa.
    barre<512>();
    barre<1024>();
    barre<1536>();
    barre<2048>();

    print_footer();
    return 0;
}
