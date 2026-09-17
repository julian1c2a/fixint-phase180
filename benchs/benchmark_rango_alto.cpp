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
// @file       benchmark_rango_alto.cpp
// @brief      El rango 64..4096, que nunca se habia medido
// @date       2026-09-16
// =============================================================================
//
// TODO lo medido en esta sesion llega a N=64, y `NSTD_LIMBOS_MAX` es 4096. La
// mitad alta del rango que la biblioteca admite esta sin mirar.
//
// Tres preguntas:
//
//   1. El equilibrado, .sigue ganando al bucle arriba? El exponente dice que
//      si, y cada vez mas: 1,585 frente a 2. Pero el exponente no cuenta la
//      cache, y a N=4096 un operando son 32 KB -- los dos no caben en L2.
//
//   2. .Donde deja de valer la pena la RECURSION? `kmul_full_gen` corta en
//      `Base` limbos y baja al escolar completo. Base esta en 8 PORQUE SE
//      ESCRIBIO ASI, sin medirlo. Es una perilla entera sin tocar.
//
//   3. La linea base contra la que se medira Toom-3. Sin ella no se puede decir
//      si Toom-3 aporta, y el estudio proyecta que por debajo de N=128 no
//      aporta nada y que en N=128 incluso pierde.
//
// La rejilla NO es densa: a estas anchuras no hace falta, porque lo que se busca
// es una tendencia y no un escalon. Se usan potencias de dos y un punto
// intermedio entre cada par, para que ninguna conclusion dependa de mirar solo
// potencias de dos -- que es el error que escondio el acantilado.
//
// SE MIDE CON `medio_reparto`, NO CON EL `Medio` POR DEFECTO.
// `mul_karatsuba_equilibrado` trae `medio_escolar<26>` de serie, y con el todo
// termino del medio de mas de 26 limbos cae al bucle CUADRATICO. A N>=64 eso
// convierte el Karatsuba en un hibrido lisiado que no es el que usa la
// biblioteca: `operator*` pasa `medio_reparto<NSTD_DESENROLLA_MAX,
// NSTD_KARATSUBA_MIN>`. Medir el otro habria dado un numero real de un
// algoritmo que nadie ejecuta.
// =============================================================================

#include "algorithms/mul_kernels.hpp"
#include "bench_adaptativo.hpp"
#include "fixed_width_int_t.hpp" // solo por las macros de umbral, para que el
                                 // banco mida EXACTAMENTE la configuracion viva

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

// Pocos operandos: a N=4096 cada uno son 32 KB, y 64 de ellos serian 2 MB por
// array. Con 8 se rota lo justo para que no se mida siempre el mismo par sin
// convertir el banco en una prueba de memoria.
static constexpr std::size_t OPERANDOS = 8;

/// El `Medio` que usa la biblioteca de verdad, parametrizado por el corte.
template <std::size_t Base>
using medio_t = nstd::algorithms::medio_reparto<NSTD_DESENROLLA_MAX, NSTD_KARATSUBA_MIN, Base>;

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

// =============================================================================
// 1. El equilibrado contra el bucle, arriba
// =============================================================================

static double g_ant = 0.0;
static std::size_t g_ant_n = 0;

template <std::size_t N>
static void una_anchura()
{
    static const auto a = operandos<N>(0xA11CE + N * 7);
    static const auto b = operandos<N>(0xB0B + N * 13);
    static std::array<std::uint64_t, N> sumidero{};

    auto bucle = [&](std::size_t k)
    {
        nstd::algorithms::mul_escolar_bucle<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };
    auto equil = [&](std::size_t k)
    {
        nstd::algorithms::mul_karatsuba_equilibrado<N, 8, medio_t<8>>(a[k % OPERANDOS], b[k % OPERANDOS],
                                                                      sumidero, medio_t<8>{});
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(bucle, equil));
    const double razon = m[1].minimo > 0 ? m[0].minimo / m[1].minimo : 0.0;
    const double ruido = m[0].recorrido() + m[1].recorrido();

    // El exponente efectivo del equilibrado entre esta anchura y la anterior.
    // Karatsuba deberia dar 1,585; si sube hacia 2 es que la cache manda.
    double exp_ef = 0.0;
    if (g_ant > 0.0)
        exp_ef = std::log(m[1].minimo / g_ant) / std::log(double(N) / double(g_ant_n));
    g_ant = m[1].minimo;
    g_ant_n = N;

    std::printf("| %5zu | %12.0f | %12.0f | %6.2fx%-2s | %5.3f |\n", N, m[0].minimo, m[1].minimo, razon,
                (razon - 1.0) > ruido ? "" : " ?", exp_ef);
    std::fflush(stdout);

    char et[72];
    std::snprintf(et, sizeof(et), "rango alto equil vs bucle N=%zu", N);
    bench_record(et, razon, "x");
}

// =============================================================================
// 2. La perilla sin medir: donde corta la recursion
// =============================================================================

/// Un functor y no una lambda con plantilla: hacen falta cinco variantes en un
/// `std::tuple` y cada una tiene que ser de un tipo propio.
template <std::size_t N, std::size_t B>
struct con_base
{
    const std::vector<std::array<std::uint64_t, N>> *a;
    const std::vector<std::array<std::uint64_t, N>> *b;
    std::array<std::uint64_t, N> *s;

    void operator()(std::size_t k) const noexcept
    {
        nstd::algorithms::mul_karatsuba_equilibrado<N, B, medio_t<B>>((*a)[k % OPERANDOS],
                                                                      (*b)[k % OPERANDOS], *s, medio_t<B>{});
        doNotOptimize((*s)[0]);
    }
};

template <std::size_t N>
static void barre_base()
{
    static const auto a = operandos<N>(0xC0FFEE + N);
    static const auto b = operandos<N>(0xDECAF + N);
    static std::array<std::uint64_t, N> s{};

    // Las cinco entrelazadas: es la unica forma de que la comparacion valga.
    const auto m = bench::mide_entrelazado(
        std::make_tuple(con_base<N, 4>{&a, &b, &s}, con_base<N, 8>{&a, &b, &s}, con_base<N, 16>{&a, &b, &s},
                        con_base<N, 32>{&a, &b, &s}, con_base<N, 64>{&a, &b, &s}));

    int mejor = 0;
    for (int i = 1; i < 5; ++i)
        if (m[i].minimo < m[mejor].minimo)
            mejor = i;
    const int bases[5] = {4, 8, 16, 32, 64};

    // El recorrido de las dos casillas que se comparan: si la ganancia del mejor
    // sobre el 8 no lo supera, no hay hallazgo que contar.
    const double ruido = m[1].recorrido() + m[mejor].recorrido();
    const double gana = m[1].minimo / m[mejor].minimo;

    std::printf("| %5zu | %9.0f | %9.0f | %9.0f | %9.0f | %9.0f |   %2d  | %5.2fx%-2s |\n", N, m[0].minimo,
                m[1].minimo, m[2].minimo, m[3].minimo, m[4].minimo, bases[mejor], gana,
                (gana - 1.0) > ruido ? "" : " ?");
    std::fflush(stdout);
}

int main()
{
    print_header("el rango 64..4096");

    std::printf("\n--- 1. el equilibrado contra el bucle, arriba ---\n\n"
                "La columna exp es el exponente efectivo del equilibrado entre esta\n"
                "anchura y la anterior. Karatsuba deberia dar 1,585; si sube hacia 2 es\n"
                "que la cache empieza a mandar sobre el algoritmo.\n"
                "Una razon marcada con ? no supera el ruido de las dos casillas.\n\n");
    std::printf("|     N |        bucle |  equilibrado |   razon     |  exp  |\n");
    std::printf("|------:|-------------:|-------------:|------------:|------:|\n");
    std::fflush(stdout);
    una_anchura<64>();
    una_anchura<96>();
    una_anchura<128>();
    una_anchura<192>();
    una_anchura<256>();
    una_anchura<384>();
    una_anchura<512>();
    una_anchura<768>();
    una_anchura<1024>();
    una_anchura<1536>();
    una_anchura<2048>();
    una_anchura<3072>();
    una_anchura<4096>();

    std::printf("\n--- 2. donde corta la recursion: la perilla Base, sin medir hasta hoy ---\n\n"
                "Por debajo de `Base` limbos, `kmul_full_gen` baja al escolar completo.\n"
                "Esta en 8 porque se escribio asi. Las cinco variantes entrelazadas, y el\n"
                "corte se arrastra hasta el fondo: tambien lo usan los terminos del medio.\n\n");
    std::printf("|     N |    Base=4 |    Base=8 |   Base=16 |   Base=32 |   Base=64 | mejor | gana     |\n");
    std::printf("|------:|----------:|----------:|----------:|----------:|----------:|------:|---------:|\n");
    std::fflush(stdout);
    barre_base<128>();
    barre_base<512>();
    barre_base<2048>();

    std::printf("\nSi el mejor `Base` no es 8, hay una ganancia gratis en toda anchura que\n"
                "use Karatsuba: es un parametro, no un algoritmo.\n");

    print_footer();
    return 0;
}
