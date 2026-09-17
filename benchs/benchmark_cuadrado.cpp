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
// @file       benchmark_cuadrado.cpp
// @brief      Cuanto ahorra de verdad tratar `a*a` como un caso propio
// @date       2026-09-16
// =============================================================================
//
// La teoria dice que el cuadrado cuesta la mitad que el producto:
//
//   - En el escolar, los cruzados `a[i]*a[j]` con i != j salen dos veces y son
//     iguales: N(N+1)/2 productos en vez de N^2.
//   - En Karatsuba, los dos terminos del medio son el mismo: uno en vez de dos.
//
// La teoria tambien decia que Karatsuba ganaba desde N=23 y hubo que medirlo.
// Aqui se compara, ENTRELAZADO y con dispersion, `a*a` por el camino normal
// contra los dos nucleos de cuadrado.
//
// El ahorro NO puede llegar a 2x: doblar los cruzados, sumar la diagonal y
// arrastrar acarreos cuesta, y en Karatsuba solo se ahorra uno de los tres
// productos. Un 1,3x-1,5x seria lo esperable; lo que diga la medida es lo que
// vale.
// =============================================================================

#include "algorithms/mul_kernels.hpp"
#include "bench_adaptativo.hpp"
// Por `NSTD_KARATSUBA_MIN`: el reparto que la biblioteca usa de verdad, para
// que el "camino de hoy" del banco sea el de hoy y no una aproximacion.
#include "fixed_width_int_t.hpp"

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

template <std::size_t N>
static void una_anchura()
{
    const auto a = operandos<N>(0xC0FFEE + N * 11);
    static std::array<std::uint64_t, N> sumidero{};

    // El camino de hoy: `a * a` como si los factores fueran distintos, con el
    // reparto que la biblioteca usa de verdad.
    auto normal = [&](std::size_t k)
    {
        const auto &x = a[k % OPERANDOS];
        if constexpr (N >= NSTD_KARATSUBA_MIN)
            nstd::algorithms::mul_karatsuba_equilibrado<N>(x, x, sumidero);
        else
            nstd::algorithms::mul_escolar_desenrollado<N>(x, x, sumidero);
        doNotOptimize(sumidero[0]);
    };
    auto sqr_esc = [&](std::size_t k)
    {
        nstd::algorithms::sqr_escolar_bucle<N>(a[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };
    auto sqr_kar = [&](std::size_t k)
    {
        nstd::algorithms::sqr_karatsuba_equilibrado<N>(a[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(normal, sqr_esc, sqr_kar));

    const double r_esc = m[1].minimo > 0 ? m[0].minimo / m[1].minimo : 0.0;
    const double r_kar = m[2].minimo > 0 ? m[0].minimo / m[2].minimo : 0.0;
    const double ruido_e = m[0].recorrido() + m[1].recorrido();
    const double ruido_k = m[0].recorrido() + m[2].recorrido();

    const double mejor = r_esc > r_kar ? r_esc : r_kar;
    const char *quien = r_esc > r_kar ? "escolar" : "Karatsuba";

    std::printf("| %4zu | %9.1f | %9.1f | %9.1f | %6.2fx%-2s | %6.2fx%-2s | %-9s %.2fx |\n", N, m[0].minimo,
                m[1].minimo, m[2].minimo, r_esc, (r_esc - 1.0) > ruido_e ? "" : " ?", r_kar,
                (r_kar - 1.0) > ruido_k ? "" : " ?", quien, mejor);

    char et[72];
    std::snprintf(et, sizeof(et), "cuadrado N=%zu", N);
    bench_record(et, mejor, "x");
}

template <std::size_t N, std::size_t Tope, std::size_t Paso>
static void barre()
{
    if constexpr (N <= Tope)
    {
        una_anchura<N>();
        barre<N + Paso, Tope, Paso>();
    }
}

int main()
{
    print_header("el cuadrado como caso propio");

    std::printf("\n`a*a` por el camino normal, contra los dos nucleos de cuadrado.\n"
                "El ahorro teorico es 2x en productos; la mitad se va en doblar los\n"
                "cruzados, sumar la diagonal y arrastrar acarreos.\n\n"
                "Protocolo: %zu repeticiones, %.0f ms cada una, las TRES entrelazadas con el\n"
                "orden rotando. Un '?' marca una razon que no supera el ruido.\n\n",
                bench::REPETICIONES, bench::MS_POR_CASILLA);

    std::printf("|    N |  a*a hoy  |  sqr esc  |  sqr kar  |  esc/hoy    |  kar/hoy    | mejor       |\n");
    std::printf("|-----:|----------:|----------:|----------:|------------:|------------:|-------------|\n");

    barre<4, 64, 4>();

    std::printf("\nLo que decide si merece la pena: si el mejor de los dos supera el ruido en\n"
                "la mayoria de las anchuras, `operator*` debe detectar `a*a` y desviarlo.\n");

    print_footer();
    return 0;
}
