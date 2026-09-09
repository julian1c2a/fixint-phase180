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
// @file       benchmark_bases.cpp
// @brief      P2.4: coste de to_string / from_string en las bases 2..36
// @date       2026-09-09
// =============================================================================
//
// POR QUE. `to_string(base)` y `from_string(s, base)` aceptan 2..36 desde
// v1.90.1 y NUNCA SE HABIAN MEDIDO: los dos benchmarks que habia cubren la 10 y
// la 16, que son justo las dos que el codigo trata de forma especial --la 10 por
// ser la de `std::to_string`, y las potencias de dos porque se hacen a
// desplazamientos--. Las 33 bases restantes, que son las que van por division
// general, no tenia nadie medidas.
//
// LO QUE SE ESPERA, para poder contrastarlo con lo medido (P2.7, pieza 4):
//
//   - Las potencias de dos --2, 4, 8, 16, 32-- deberian ser claramente mas
//     baratas: convertir es partir en grupos de bits, sin dividir.
//   - El resto deberia costar aproximadamente lo mismo entre si, y bajar
//     despacio segun crece la base, porque salen menos digitos: el numero de
//     divisiones es log_base(valor).
//
// Si lo medido no se parece a eso, hay algo que explicar antes de publicarlo.
// =============================================================================

#include "fixed_width_int_t.hpp"

#include "bench_common.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using namespace nstd;

static constexpr std::size_t OPERANDOS{128};
static constexpr std::size_t VUELTAS{20000};
static constexpr std::size_t RONDAS{5};

/// @brief Es potencia de dos, o sea de las que se hacen a desplazamientos.
static constexpr bool es_potencia_de_dos(int base) noexcept { return base > 0 && (base & (base - 1)) == 0; }

template <std::size_t N>
static std::vector<uint_fixed_t<N>> operandos()
{
    std::mt19937_64 rng(0xBA5E5);
    std::vector<uint_fixed_t<N>> v;
    v.reserve(OPERANDOS);
    for (std::size_t i = 0; i < OPERANDOS; ++i)
    {
        uint_fixed_t<N> x{};
        for (std::size_t k = 0; k < N; ++k)
            x.set_limb(k, rng());
        v.push_back(x);
    }
    return v;
}

template <std::size_t N>
static double mide_to_string(const std::vector<uint_fixed_t<N>> &xs, int base)
{
    std::string sumidero;
    CycleTimer t;
    for (std::size_t k = 0; k < VUELTAS; ++k)
    {
        sumidero = xs[k % OPERANDOS].to_string(base);
        doNotOptimize(sumidero);
    }
    return static_cast<double>(t.elapsed_cycles()) / static_cast<double>(VUELTAS);
}

template <std::size_t N>
static double mide_from_string(const std::vector<std::string> &ss, int base)
{
    uint_fixed_t<N> sumidero{};
    CycleTimer t;
    for (std::size_t k = 0; k < VUELTAS; ++k)
    {
        sumidero = uint_fixed_t<N>::from_string(ss[k % OPERANDOS].c_str(), base);
        doNotOptimize(sumidero);
    }
    return static_cast<double>(t.elapsed_cycles()) / static_cast<double>(VUELTAS);
}

template <std::size_t N>
static void una_anchura(const char *etiqueta)
{
    const auto xs = operandos<N>();

    std::printf("\n[%s]  %zu bits\n", etiqueta, 64 * N);
    std::printf("+------+-----------+-------------+-----------+\n");
    std::printf("| base | to_string | from_string |  digitos  |\n");
    std::printf("+------+-----------+-------------+-----------+\n");

    for (int base = 2; base <= 36; ++base)
    {
        // Las cadenas de entrada se generan con la propia biblioteca. Es
        // deliberado: lo que se mide es el coste, no la correccion --de eso se
        // ocupan los tests-- y asi la entrada esta garantizada valida.
        std::vector<std::string> ss;
        ss.reserve(OPERANDOS);
        for (const auto &x : xs)
            ss.push_back(x.to_string(base));

        double mejor_ts{1e300}, mejor_fs{1e300};
        for (std::size_t r = 0; r < RONDAS; ++r)
        {
            const double ts = mide_to_string<N>(xs, base);
            const double fs = mide_from_string<N>(ss, base);
            if (ts < mejor_ts)
                mejor_ts = ts;
            if (fs < mejor_fs)
                mejor_fs = fs;
        }

        char nombre[64];
        std::snprintf(nombre, sizeof(nombre), "to_string N=%zu base %d", N, base);
        bench_record(nombre, mejor_ts);
        std::snprintf(nombre, sizeof(nombre), "from_string N=%zu base %d", N, base);
        bench_record(nombre, mejor_fs);

        std::printf("| %4d | %9.1f | %11.1f | %9zu |%s\n", base, mejor_ts, mejor_fs, ss[0].size(),
                    es_potencia_de_dos(base) ? "  <- potencia de dos" : "");
    }
    std::printf("+------+-----------+-------------+-----------+\n");
}

int main()
{
    std::printf("=== to_string / from_string en las bases 2..36 ===\n");
    std::printf("%zu operandos aleatorios, %zu iteraciones x %zu rondas, minimo por caso\n", OPERANDOS,
                VUELTAS, RONDAS);
    std::printf("\nLo esperado: las potencias de dos, mas baratas (van a "
                "desplazamientos);\nel resto, parecidas entre si y bajando despacio segun "
                "crece la base.\n");

    una_anchura<2>("uint128");
    una_anchura<4>("uint256");

    return 0;
}
