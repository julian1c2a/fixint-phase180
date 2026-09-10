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
// @file       benchmark_equilibrado.cpp
// @brief      Karatsuba equilibrado contra el bucle, en TODA N -- ¿se va el acantilado?
// @date       2026-09-10
// =============================================================================
//
// La pregunta que responde: **el acantilado de `operator*` desaparece?**
//
// Hoy, toda anchura mayor que el tope de desenrollado que no sea potencia de dos
// cae al bucle escolar y cuesta 3-4x por limbo -- N=24 tarda MAS que N=32. Es el
// acantilado de docs/PERFORMANCE.md, y la causa es que Karatsuba solo admite
// potencias de dos.
//
// `mul_karatsuba_equilibrado` quita esa limitacion: parte en dos mitades iguales
// cuando N es par y rellena con UN limbo cuando es impar. Si funciona, la razon
// contra el bucle deberia ser suave en N y no tener dientes.
//
// No se comparan aqui potencias de dos y no potencias por separado: se barren
// TODAS de 8 a 64. Un algoritmo que solo se mide en las anchuras que le
// convienen no se ha medido.
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

static double g_peor = 1e9;
static std::size_t g_peor_n = 0;

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
    auto equil = [&](std::size_t k)
    {
        nstd::algorithms::mul_karatsuba_equilibrado<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(bucle, equil));
    const double razon = m[1].minimo > 0 ? m[0].minimo / m[1].minimo : 0.0;
    const double ruido = m[0].recorrido() + m[1].recorrido();
    const bool sig = (razon > 1.0 ? razon - 1.0 : 1.0 - razon) > ruido;

    if (razon < g_peor)
    {
        g_peor = razon;
        g_peor_n = N;
    }

    const char *que = (N & (N - 1)) == 0 ? "pot2" : (N % 2 ? "impar" : "par");
    std::printf("| %4zu | %-5s | %11.1f %5.1f%% | %11.1f %5.1f%% | %6.2fx |%s\n", N, que, m[0].minimo,
                m[0].recorrido() * 100.0, m[1].minimo, m[1].recorrido() * 100.0, razon,
                sig ? "" : "  (ruido)");

    char et[72];
    std::snprintf(et, sizeof(et), "equilibrado vs bucle N=%zu", N);
    bench_record(et, razon, "x");
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
    print_header("Karatsuba equilibrado contra el bucle, en toda N");

    std::printf("\nSi el acantilado desaparece, la razon debe ser SUAVE en N y sin dientes:\n"
                "las potencias de dos no deben destacar sobre sus vecinas.\n\n"
                "Protocolo: %zu repeticiones, %.0f ms cada una, entrelazadas con el orden\n"
                "rotando. '(ruido)' = la diferencia no supera la suma de los recorridos.\n\n",
                bench::REPETICIONES, bench::MS_POR_CASILLA);

    std::printf("|    N | clase |    bucle cyc/op      | equilibrado cyc/op   |  razon  |\n");
    std::printf("|-----:|-------|---------------------:|---------------------:|--------:|\n");

    barre<8, 64>();

    std::printf("\nLa razon MAS BAJA de todo el barrido: %.2fx en N=%zu.\n", g_peor, g_peor_n);
    std::printf("Si es >= 1, el equilibrado no pierde en NINGUNA anchura, que es lo que el\n"
                "reparto de hoy no puede decir.\n");

    print_footer();
    return 0;
}
