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
// @file       benchmark_nucleos.cpp
// @brief      Los nucleos de multiplicacion, medidos ENTRELAZADOS y con dispersion
// @date       2026-09-10
// =============================================================================
//
// Es el primer banco que puede cumplir el protocolo entero, porque es el primero
// en el que las variantes existen A LA VEZ en el mismo binario --gracias a
// `algorithms/mul_kernels.hpp`-- y porque el arnes adaptativo hace que una
// casilla cueste lo mismo en N=8 que en N=2048.
//
// LO QUE VIENE A RESPONDER, y es el punto cero de la sesion de medicion: para el
// desenrollado frente al bucle en N=24 hay dos cifras publicadas en este
// proyecto, 1,11x (6 sep) y 2,44x (10 sep), que difieren 2,2x. Una esta mal.
// Aqui se miden las dos variantes entrelazadas, en el mismo proceso, con diez
// repeticiones y publicando la dispersion -- que es como debio medirse.
// =============================================================================

#include "algorithms/mul_kernels.hpp"
#include "bench_adaptativo.hpp"
#include "fixed_width_int_t.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

using namespace nstd;

/// @brief Cuantos operandos distintos se rotan, para no medir siempre el mismo
///        par y para que la cache no lo tenga todo en el primer nivel.
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

/// @brief Compara bucle y desenrollado en una anchura, entrelazados.
template <std::size_t N>
static void compara_escolares()
{
    const auto a = operandos<N>(0xA11CE + N);
    const auto b = operandos<N>(0xB0B + N);
    static std::array<std::uint64_t, N> sumidero{};

    auto bucle = [&](std::size_t k)
    {
        algorithms::mul_escolar_bucle<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };
    auto desen = [&](std::size_t k)
    {
        algorithms::mul_escolar_desenrollado<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(bucle, desen));

    char e1[64], e2[64];
    std::snprintf(e1, sizeof(e1), "N=%zu  escolar en bucle", N);
    std::snprintf(e2, sizeof(e2), "N=%zu  escolar desenrollado", N);
    bench::imprime(e1, m[0]);
    bench::imprime(e2, m[1]);

    const double razon = m[1].minimo > 0 ? m[0].minimo / m[1].minimo : 0.0;
    // El recorrido de las dos acotado: si la razon cae dentro del ruido, no
    // significa nada y hay que decirlo.
    const double ruido = m[0].recorrido() + m[1].recorrido();
    std::printf("  %-34s %10.2fx%s\n", "    -> desenrollado gana", razon,
                (razon - 1.0) < ruido ? "   (DENTRO DEL RUIDO)" : "");
    bench_record((std::string("nucleo bucle N=") + std::to_string(N)).c_str(), m[0].minimo);
    bench_record((std::string("nucleo desenrollado N=") + std::to_string(N)).c_str(), m[1].minimo);
}

/// @brief Los tres, donde los tres existen: potencias de dos hasta 128.
template <std::size_t N>
static void compara_tres()
{
    const auto a = operandos<N>(0xC0FFEE + N);
    const auto b = operandos<N>(0xDECAF + N);
    static std::array<std::uint64_t, N> sumidero{};

    auto bucle = [&](std::size_t k)
    {
        algorithms::mul_escolar_bucle<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };
    auto desen = [&](std::size_t k)
    {
        algorithms::mul_escolar_desenrollado<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero);
        doNotOptimize(sumidero[0]);
    };
    auto kara = [&](std::size_t k)
    {
        algorithms::mul_karatsuba_pot2<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero,
                                          algorithms::medio_como_hoy{});
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(bucle, desen, kara));

    char e[3][64];
    std::snprintf(e[0], sizeof(e[0]), "N=%zu  escolar en bucle", N);
    std::snprintf(e[1], sizeof(e[1]), "N=%zu  escolar desenrollado", N);
    std::snprintf(e[2], sizeof(e[2]), "N=%zu  Karatsuba", N);
    for (int i = 0; i < 3; ++i)
        bench::imprime(e[i], m[i]);
}

int main()
{
    print_header("nucleos de multiplicacion, entrelazados");

    std::printf("\nProtocolo: %zu repeticiones por casilla, %.0f ms cada una, iteraciones\n"
                "calibradas, y las variantes ENTRELAZADAS con el orden rotando cada ronda.\n"
                "La columna 'recorrido' es (max-min)/min: si la diferencia entre dos\n"
                "variantes no la supera, esa diferencia no significa nada.\n\n",
                bench::REPETICIONES, bench::MS_POR_CASILLA);

    std::printf("--- EL PUNTO CERO: bucle frente a desenrollado alrededor de N=24 ---\n");
    bench::imprime_cabecera();
    compara_escolares<16>();
    compara_escolares<20>();
    compara_escolares<24>();
    compara_escolares<28>();
    compara_escolares<32>();

    std::printf("\n--- los TRES, donde los tres se pueden instanciar ---\n");
    bench::imprime_cabecera();
    compara_tres<32>();
    compara_tres<64>();
    compara_tres<128>();

    print_footer();
    return 0;
}
