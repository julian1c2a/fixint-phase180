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
    // AQUI HABIA UNA TERCERA VARIANTE, `sqr_escolar_bucle`, Y SE FUE CON EL
    // NUCLEO. Se escribio y se retiro el 16 sep 2026 tras medirla --perdia
    // contra el producto normal en las dieciseis anchuras, de 0,35x a 0,80x--,
    // pero la llamada se quedo aqui y este fichero llevaba desde entonces **sin
    // compilar**. El motivo esta escrito en `mul_kernels.hpp`, junto al hueco
    // que dejo.
    auto sqr_kar = [&](std::size_t k)
    {
        nstd::algorithms::sqr_karatsuba_equilibrado<N>(a[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(normal, sqr_kar));

    const double r_kar = m[1].minimo > 0 ? m[0].minimo / m[1].minimo : 0.0;
    const double ruido_k = m[0].recorrido() + m[1].recorrido();

    std::printf("| %4zu | %9.1f | %9.1f | %6.2fx%-2s |\n", N, m[0].minimo, m[1].minimo, r_kar,
                (r_kar - 1.0) > ruido_k ? "" : " ?");

    // Las dos absolutas con su ruido, y la razon aparte. La razon es la que
    // aguanta el paso de los dias: las dos se midieron entrelazadas en la misma
    // tanda, asi que la deriva de la maquina les toca por igual.
    char et[72];
    std::snprintf(et, sizeof(et), "cuadrado N=%zu / a*a hoy", N);
    bench::registra(et, m[0]);
    std::snprintf(et, sizeof(et), "cuadrado N=%zu / sqr karatsuba", N);
    bench::registra(et, m[1]);
    std::snprintf(et, sizeof(et), "cuadrado N=%zu / razon", N);
    bench_record(et, r_kar, "x");
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

    std::printf("\n`a*a` por el camino normal, contra el nucleo de cuadrado.\n"
                "El ahorro teorico es 2x en productos; la mitad se va en doblar los\n"
                "cruzados, sumar la diagonal y arrastrar acarreos.\n\n"
                "Hubo una tercera columna, el cuadrado escolar, y se fue con su nucleo:\n"
                "medido el 16 sep, perdia en las dieciseis anchuras. Ver `mul_kernels.hpp`.\n\n"
                "Protocolo: %zu repeticiones, %.0f ms cada una, las DOS entrelazadas con el\n"
                "orden rotando. Un '?' marca una razon que no supera el ruido.\n\n",
                bench::REPETICIONES, bench::MS_POR_CASILLA);

    std::printf("|    N |  a*a hoy  |  sqr kar  |  kar/hoy    |\n");
    std::printf("|-----:|----------:|----------:|------------:|\n");

    barre<4, 64, 4>();

    std::printf("\nEsto ya no DECIDE nada: `operator*` detecta `a*a` por direccion desde el\n"
                "17 sep y lo desvia al nucleo de cuadrado. Lo que hace ahora es VIGILARLO:\n"
                "si la razon dejara de superar el ruido, el desvio habria dejado de pagar.\n");

    print_footer();
    return 0;
}
