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
// @file       benchmark_barrido_desenrollado.cpp
// @brief      Donde CRUZA el escolar desenrollado con el escolar en bucle
// @date       2026-09-10
// =============================================================================
//
// Sale a buscar UN CRUCE, no un punto de rendimientos decrecientes. La primera
// tanda del 10 sep 2026 dejo claro que el desenrollado no solo deja de compensar
// pasado cierto N: **pasa a perder**, y por mucho.
//
//     clang N=24    gana  1,75x        gcc N=24    gana  2,74x
//     clang N=64   PIERDE 1,47x        gcc N=64    gana  1,12x
//     clang N=128  PIERDE 2,09x        gcc N=128  PIERDE 1,49x
//
// Es coherente con que el desenrollado sea O(N^2) en TAMANO DE CODIGO: pasado
// cierto punto deja de caber en la cache de instrucciones y el bucle, que es
// diminuto, gana. El cruce esta entre 32 y 64 con clang y entre 64 y 128 con
// gcc, asi que el barrido llega hasta 96 para que quede DENTRO de los datos y no
// haya que extrapolarlo.
//
// La rejilla es densa de 1 a 40 --ahi es donde estan los umbrales que interesan--
// y de cuatro en cuatro hasta 96, porque el desenrollado es O(N^2) tambien en
// TIEMPO DE COMPILACION y meter las 96 anchuras seguidas dispara el coste sin
// anadir informacion.
//
// Protocolo: las dos variantes ENTRELAZADAS con el orden rotando, diez
// repeticiones, iteraciones calibradas a 200 ms, y se publica la dispersion. Una
// razon que no supere la suma de los recorridos NO significa nada, y se marca.
// =============================================================================

#include "algorithms/mul_kernels.hpp"
#include "bench_adaptativo.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

static constexpr std::size_t OPERANDOS = 64;

template <std::size_t N>
static std::vector<std::array<std::uint64_t, N>> operandos(std::uint64_t semilla)
{
    std::vector<std::array<std::uint64_t, N>> v;
    v.reserve(OPERANDOS);
    std::uint64_t s = semilla;
    auto siguiente = [&s]() noexcept
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return s;
    };
    for (std::size_t i = 0; i < OPERANDOS; ++i)
    {
        std::array<std::uint64_t, N> x{};
        for (std::size_t k = 0; k < N; ++k)
            x[k] = siguiente();
        v.push_back(x);
    }
    return v;
}

/// @brief Cuantas anchuras se han visto ya con el desenrollado perdiendo.
static int g_perdiendo = 0;
/// @brief La ultima anchura en la que el desenrollado gana de forma clara.
static std::size_t g_ultima_clara = 0;

template <std::size_t N>
static void una_anchura()
{
    const auto a = operandos<N>(0xA11CE + N * 7);
    const auto b = operandos<N>(0xB0B + N * 13);
    static std::array<std::uint64_t, N> sumidero{};

    auto bucle = [&](std::size_t k)
    {
        nstd::algorithms::mul_escolar_bucle<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };
    auto desen = [&](std::size_t k)
    {
        nstd::algorithms::mul_escolar_desenrollado<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(bucle, desen));
    const double razon = m[1].minimo > 0 ? m[0].minimo / m[1].minimo : 0.0;
    const double ruido = m[0].recorrido() + m[1].recorrido();

    // Una razon vale si se separa de 1 mas de lo que suman los dos recorridos.
    const bool significativa = (razon > 1.0 ? razon - 1.0 : 1.0 - razon) > ruido;
    const char *marca = !significativa ? "  (ruido)" : (razon < 1.0 ? "  <-PIERDE" : "");

    if (significativa && razon < 1.0)
        ++g_perdiendo;
    if (significativa && razon > 1.0)
        g_ultima_clara = N;

    std::printf("| %4zu | %11.1f %5.1f%% | %11.1f %5.1f%% | %6.2fx |%s\n", N, m[0].minimo,
                m[0].recorrido() * 100.0, m[1].minimo, m[1].recorrido() * 100.0, razon, marca);

    char et[64];
    std::snprintf(et, sizeof(et), "barrido desenrollado N=%zu", N);
    bench_record(et, razon, "x sobre bucle");
}

/// @brief Densa de Ini a Fin, de una en una.
template <std::size_t Ini, std::size_t Fin>
static void densa()
{
    if constexpr (Ini <= Fin)
    {
        una_anchura<Ini>();
        densa<Ini + 1, Fin>();
    }
}

/// @brief De Ini a Fin con paso Paso.
template <std::size_t Ini, std::size_t Fin, std::size_t Paso>
static void con_paso()
{
    if constexpr (Ini <= Fin)
    {
        una_anchura<Ini>();
        con_paso<Ini + Paso, Fin, Paso>();
    }
}

int main()
{
    print_header("barrido de NSTD_DESENROLLA_MAX: donde cruza");

    std::printf("\nBusca el CRUCE, no el punto de rendimientos decrecientes: el desenrollado\n"
                "pasa a PERDER en N grande, porque es O(N^2) en tamano de codigo y deja de\n"
                "caber en la cache de instrucciones.\n\n"
                "Protocolo: %zu repeticiones, %.0f ms cada una, iteraciones calibradas,\n"
                "variantes entrelazadas con el orden rotando. La columna de porcentaje es el\n"
                "recorrido (max-min)/min. Una razon marcada '(ruido)' NO significa nada.\n\n",
                bench::REPETICIONES, bench::MS_POR_CASILLA);

    std::printf("|    N |   bucle cyc/op       | desenrollado cyc/op  |  razon  |\n");
    std::printf("|-----:|---------------------:|---------------------:|--------:|\n");

    densa<1, 40>();
    con_paso<44, 96, 4>();

    std::printf("\n");
    std::printf("Ultima anchura donde el desenrollado gana de forma SIGNIFICATIVA: N=%zu\n", g_ultima_clara);
    std::printf("Anchuras en las que PIERDE de forma significativa: %d\n", g_perdiendo);
    std::printf("\nEl tope sale de la primera cifra, no de la ultima que gane por poco: una\n"
                "razon dentro del ruido no es una ganancia.\n");

    print_footer();
    return 0;
}
