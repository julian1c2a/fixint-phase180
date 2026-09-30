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

#include "bench_adaptativo.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using namespace nstd;

static constexpr std::size_t OPERANDOS{128};

// VUELTAS y RONDAS se fueron con el arnes viejo (P2.17). Fijaban 20.000
// iteraciones para todo, y aqui `to_string` cuesta 329 ciclos en base 10 y 2.576
// en base 3 -- ocho veces mas, con la misma ventana. Ahora la ventana es de
// tiempo fijo y la eligen las iteraciones.

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

// `mide_to_string` y `mide_from_string` se fueron: cronometraban un numero fijo
// de vueltas, cada una por su lado. Ahora son dos lambdas que `mide_entrelazado`
// alterna dentro de la misma tanda, con el orden rotando -- que es lo que hace
// legitimo comparar escribir contra leer.

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

        std::string s_sumidero;
        uint_fixed_t<N> n_sumidero{};

        auto f_escribe = [&](std::size_t k)
        {
            s_sumidero = xs[k % OPERANDOS].to_string(base);
            doNotOptimize(s_sumidero);
        };
        auto f_lee = [&](std::size_t k)
        {
            n_sumidero = uint_fixed_t<N>::from_string(ss[k % OPERANDOS].c_str(), base);
            doNotOptimize(n_sumidero);
        };

        const auto m = bench::mide_entrelazado(std::make_tuple(f_escribe, f_lee));

        char nombre[64];
        std::snprintf(nombre, sizeof(nombre), "to_string N=%zu base %d", N, base);
        bench::registra(nombre, m[0]);
        std::snprintf(nombre, sizeof(nombre), "from_string N=%zu base %d", N, base);
        bench::registra(nombre, m[1]);

        std::printf("| %4d | %9.1f | %11.1f | %9zu |%s\n", base, m[0].minimo, m[1].minimo,
                    ss[0].size(), es_potencia_de_dos(base) ? "  <- potencia de dos" : "");
    }
    std::printf("+------+-----------+-------------+-----------+\n");
}

int main()
{
    std::printf("=== to_string / from_string en las bases 2..36 ===\n");
    std::printf("%zu operandos aleatorios; %zu vueltas de %.0f ms por casilla, escribir y\n"
                "leer ENTRELAZADAS y con el orden rotando. Se publica el minimo.\n",
                OPERANDOS, bench::REPETICIONES, bench::MS_POR_CASILLA);
    std::printf("\nLo esperado: las potencias de dos, mas baratas (van a "
                "desplazamientos);\nel resto, parecidas entre si y bajando despacio segun "
                "crece la base.\n");

    una_anchura<2>("uint128");
    una_anchura<4>("uint256");

    return 0;
}
