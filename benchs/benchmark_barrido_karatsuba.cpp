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
// @file       benchmark_barrido_karatsuba.cpp
// @brief      Desde que anchura gana Karatsuba, y contra QUE gana
// @date       2026-09-10
// =============================================================================
//
// POR QUE VOLVER A MEDIR ESTO. `NSTD_KARATSUBA_MIN` esta en 32 porque «a partir
// de 32, Karatsuba gana en los cuatro compiladores». Pero esa comparacion se
// hizo CONTRA EL ESCOLAR DESENROLLADO, y el barrido del tope de desenrollado
// (10 sep 2026) acaba de ensenar que el desenrollado se HUNDE justo por esa
// zona: en N=32 ya solo gana al bucle 1,02x con MSVC y 1,22x con clang.
//
// Si el rival de Karatsuba en N=32 va a ser el BUCLE --que es lo que pasara en
// cuanto el tope de desenrollado quede por debajo de 32-- la pregunta «desde
// cuando gana Karatsuba» esta sin responder.
//
// DOS EJES, NO UNO. El segundo estaba escondido dentro del algoritmo hasta que
// `algorithms/mul_kernels.hpp` lo saco: **como se calculan los dos terminos del
// medio**, que son productos de N/2 limbos. Hoy la biblioteca los hace con el
// `operator*` de media anchura, que vuelve a repartir por las macros; a N=32 eso
// significa «kmul_full<16> mas escolar desenrollado a 16», y nadie lo habia
// medido por separado.
//
// LIMITACION: Karatsuba, tal como esta escrito hoy, SOLO admite potencias de
// dos. Por eso este barrido tiene seis puntos y no sesenta. Quitarla es el
// objeto del reparto equilibrado de PLAN_MULTIPLICACION.
// =============================================================================

#include "algorithms/mul_kernels.hpp"
#include "bench_adaptativo.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

static constexpr std::size_t OPERANDOS = 64;

/// @brief El tope de desenrollado con el que se mide el termino del medio.
///        26 es lo que recomienda el barrido del 10 sep: la ultima anchura
///        donde los cuatro compiladores ganan de forma demostrable.
static constexpr std::size_t TOPE_DESEN = 26;

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

/// @brief Nombre del ganador y por cuanto, con el ruido tenido en cuenta.
static void veredicto(const char *quien[4], const bench::Medida m[4])
{
    int mejor = 0;
    for (int i = 1; i < 4; ++i)
        if (m[i].minimo < m[mejor].minimo)
            mejor = i;

    // El segundo mejor, para saber por cuanto gana y si eso supera el ruido.
    int segundo = mejor == 0 ? 1 : 0;
    for (int i = 0; i < 4; ++i)
        if (i != mejor && m[i].minimo < m[segundo].minimo)
            segundo = i;

    const double razon = m[mejor].minimo > 0 ? m[segundo].minimo / m[mejor].minimo : 0.0;
    const double ruido = m[mejor].recorrido() + m[segundo].recorrido();
    std::printf("    -> gana %-24s por %.2fx sobre %s%s\n", quien[mejor], razon, quien[segundo],
                (razon - 1.0) <= ruido ? "   (DENTRO DEL RUIDO: empate)" : "");
}

template <std::size_t N>
static void una_anchura()
{
    static_assert(N >= 4 && (N & (N - 1)) == 0, "solo potencias de dos, es la limitacion de hoy");

    const auto a = operandos<N>(0xC0FFEE + N * 11);
    const auto b = operandos<N>(0xDECAF + N * 17);
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
    auto kara_esc = [&](std::size_t k)
    {
        nstd::algorithms::mul_karatsuba_pot2<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero,
                                                nstd::algorithms::medio_escolar<TOPE_DESEN>{});
        doNotOptimize(sumidero[0]);
    };
    auto kara_rec = [&](std::size_t k)
    {
        nstd::algorithms::mul_karatsuba_pot2<N>(a[k % OPERANDOS], b[k % OPERANDOS], sumidero,
                                                nstd::algorithms::medio_karatsuba<TOPE_DESEN, 8>{});
        doNotOptimize(sumidero[0]);
    };

    const auto m = bench::mide_entrelazado(std::make_tuple(bucle, desen, kara_esc, kara_rec));

    const char *quien[4] = {"escolar en bucle", "escolar desenrollado", "Karatsuba+medio escolar",
                            "Karatsuba+medio Karatsuba"};
    std::printf("\n  N = %zu\n", N);
    for (int i = 0; i < 4; ++i)
    {
        char et[80];
        std::snprintf(et, sizeof(et), "  %s", quien[i]);
        bench::imprime(et, m[i]);
    }
    veredicto(quien, m.data());

    // Lo que decide el umbral: Karatsuba (el mejor de sus dos) contra el mejor
    // escolar.
    const double mejor_kara = m[2].minimo < m[3].minimo ? m[2].minimo : m[3].minimo;
    const double mejor_esc = m[0].minimo < m[1].minimo ? m[0].minimo : m[1].minimo;
    char et[80];
    std::snprintf(et, sizeof(et), "karatsuba vs mejor escolar N=%zu", N);
    bench_record(et, mejor_esc / mejor_kara, "x");
    std::printf("    -> Karatsuba contra el mejor escolar: %.2fx\n", mejor_esc / mejor_kara);
}

int main()
{
    print_header("barrido de NSTD_KARATSUBA_MIN");

    std::printf("\nDos ejes: desde que anchura gana Karatsuba, y COMO se calculan sus dos\n"
                "terminos del medio -- que es otra decision de reparto, y estaba escondida\n"
                "dentro del algoritmo hasta que los nucleos se sacaron a funciones libres.\n\n"
                "Solo potencias de dos: es la limitacion del Karatsuba de hoy.\n\n"
                "Protocolo: %zu repeticiones, %.0f ms cada una, iteraciones calibradas, las\n"
                "CUATRO variantes entrelazadas con el orden rotando cada ronda.\n",
                bench::REPETICIONES, bench::MS_POR_CASILLA);

    bench::imprime_cabecera();
    una_anchura<4>();
    una_anchura<8>();
    una_anchura<16>();
    una_anchura<32>();
    una_anchura<64>();
    una_anchura<128>();

    std::printf("\nEl umbral sale de la primera anchura donde Karatsuba gana al MEJOR escolar\n"
                "de forma que supere el ruido, no de la primera donde gane por poco.\n");

    print_footer();
    return 0;
}
