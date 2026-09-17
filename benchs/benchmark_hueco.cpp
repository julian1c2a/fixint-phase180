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
// @file       benchmark_hueco.cpp
// @brief      Las TRES variantes a la vez, para cerrar el hueco entre umbrales
// @date       2026-09-16
// =============================================================================
//
// EL HUECO. Con `NSTD_DESENROLLA_MAX` = 20 y `NSTD_KARATSUBA_MIN` = 32, las
// anchuras 21..31 van al BUCLE, que es el peor de los tres caminos. El
// equilibrado le gana ahi ~2,2x -- pero eso se midio contra el bucle, no contra
// el desenrollado, y el desenrollado tambien le gana al bucle en esa banda.
//
// La pregunta que falta: **entre el equilibrado y el desenrollado, cual gana en
// cada N?** De ahi salen los dos umbrales a la vez:
//
//   - donde el equilibrado empieza a ganar al desenrollado -> NSTD_KARATSUBA_MIN
//   - donde el desenrollado deja de ganar al bucle        -> NSTD_DESENROLLA_MAX
//
// y si los dos coinciden, el hueco desaparece por construccion: no queda banda
// en la que el bucle sea el elegido.
//
// Las TRES entrelazadas, con el orden rotando cada ronda, diez repeticiones y
// dispersion publicada. Rejilla densa de 4 a 64: sin saltar ninguna, porque
// interpolar entre potencias de dos es lo que escondio el acantilado durante
// meses.
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

/// @brief La primera N en la que el equilibrado gana al desenrollado de forma
///        significativa, y se mantiene ganando.
static std::size_t g_primera_equil = 0;
/// @brief La ultima N en la que el desenrollado gana al bucle de forma
///        significativa.
static std::size_t g_ultima_desen = 0;

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
    auto equil = [&](std::size_t k)
    {
        nstd::algorithms::mul_karatsuba_equilibrado<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(bucle, desen, equil));

    const double r_desen = m[1].minimo > 0 ? m[0].minimo / m[1].minimo : 0.0; // desen sobre bucle
    const double r_equil = m[2].minimo > 0 ? m[1].minimo / m[2].minimo : 0.0; // equil sobre desen

    const double ruido_d = m[0].recorrido() + m[1].recorrido();
    const double ruido_e = m[1].recorrido() + m[2].recorrido();
    const bool sig_d = (r_desen > 1 ? r_desen - 1 : 1 - r_desen) > ruido_d;
    const bool sig_e = (r_equil > 1 ? r_equil - 1 : 1 - r_equil) > ruido_e;

    if (sig_d && r_desen > 1.0)
        g_ultima_desen = N;
    if (sig_e && r_equil > 1.0 && g_primera_equil == 0)
        g_primera_equil = N;

    // Quien gana de los tres, que es lo que el reparto deberia elegir.
    int mejor = 0;
    for (int i = 1; i < 3; ++i)
        if (m[i].minimo < m[mejor].minimo)
            mejor = i;
    const char *nombre[3] = {"bucle", "desenrollado", "equilibrado"};

    std::printf("| %4zu | %9.1f | %9.1f | %9.1f | %6.2fx%-2s | %6.2fx%-2s | %-12s |\n", N, m[0].minimo,
                m[1].minimo, m[2].minimo, r_desen, sig_d ? "" : " ?", r_equil, sig_e ? "" : " ?",
                nombre[mejor]);

    char et[72];
    std::snprintf(et, sizeof(et), "hueco equil/desen N=%zu", N);
    bench_record(et, r_equil, "x");
}

template <std::size_t N, std::size_t Tope>
static void barre()
{
    if constexpr (N <= Tope)
    {
        una_anchura<N>();
        barre<N + 1, Tope>();
    }
}

int main()
{
    print_header("el hueco 21..31: las tres variantes a la vez");

    std::printf("\nDe aqui salen LOS DOS UMBRALES a la vez:\n"
                "  - donde el equilibrado empieza a ganar al desenrollado -> KARATSUBA_MIN\n"
                "  - donde el desenrollado deja de ganar al bucle         -> DESENROLLA_MAX\n"
                "Si coinciden, el hueco desaparece: no queda banda donde el bucle sea el\n"
                "elegido.\n\n"
                "Protocolo: %zu repeticiones, %.0f ms cada una, las TRES entrelazadas con el\n"
                "orden rotando. Un '?' marca una razon que no supera la suma de los\n"
                "recorridos de sus dos casillas: no significa nada.\n\n",
                bench::REPETICIONES, bench::MS_POR_CASILLA);

    std::printf("|    N |     bucle |    desenr |    equilib | desen/bucle | equil/desen | gana        |\n");
    std::printf("|-----:|----------:|----------:|-----------:|------------:|------------:|-------------|\n");

    barre<4, 64>();

    std::printf("\nUltima N donde el desenrollado gana al bucle de forma significativa: %zu\n",
                g_ultima_desen);
    std::printf("Primera N donde el equilibrado gana al desenrollado de forma significativa: %zu\n",
                g_primera_equil);
    std::printf("\nSi la segunda es <= la primera + 1, los dos umbrales se tocan y el hueco\n"
                "desaparece. Si no, queda una banda y hay que decidir que poner en ella.\n");

    print_footer();
    return 0;
}
