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
// @file       benchmark_curva_n.cpp
// @brief      Coste de cada operacion frente a la anchura N, en limbos
// @date       2026-09-09
// =============================================================================
//
// QUE DA. Una curva por operacion: cyc/op frente a N, y --lo que de verdad se
// lee-- el COSTE POR LIMBO, que es cyc/op dividido por N. Normalizado asi, una
// operacion lineal sale plana y una cuadratica sale creciendo como N.
//
// PARA QUE SIRVE. Los benchmarks que habia miden anchuras sueltas: N=2, N=4,
// N=8. Con eso se ve un punto, no una curva, y no se ve DONDE cambia el
// comportamiento. `operator*` reparte hoy en tres caminos --especializado en
// N=2, escolar desenrollado hasta N=20, Karatsuba desde N=32-- y los saltos
// entre ellos tienen que verse aqui. Si no se ven, o el reparto no hace lo que
// dice, o la medida no vale.
//
// COMO SE LEE, y esto es la cuarta pieza del desguace (P2.7): cada operacion
// declara su coste teorico, y se publica lo medido AL LADO de lo esperado. La
// distancia entre los dos es el resultado interesante.
//
//     suma, resta, desplazamiento, comparacion   O(N)      -> por limbo, PLANA
//     producto (escolar)                          O(N^2)    -> por limbo, LINEAL
//     producto (Karatsuba)                        O(N^1.585)-> por limbo, N^0.585
//     division (Knuth D)                          O(N^2)    -> por limbo, LINEAL
//
// LO QUE NO MIDE: nada de esto dice si el resultado es correcto. De eso se
// ocupan los tests; aqui se da por hecho.
// =============================================================================

#include "fixed_width_int_t.hpp"

#include "bench_common.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using namespace nstd;

static constexpr std::size_t OPERANDOS{64};
static constexpr std::size_t VUELTAS{50000};
static constexpr std::size_t RONDAS{5};

// El suelo fisico, en las mismas unidades que RDTSC. Ver la nota larga de
// benchmark_karatsuba.cpp: RDTSC cuenta a la frecuencia invariante del TSC y no
// a la del nucleo, asi que con turbo un ciclo real mide menos de un "ciclo".
static constexpr double CICLOS_MINIMOS_POR_LIMBO{0.05};
static int g_dudosas{0};

template <std::size_t N>
static std::vector<uint_fixed_t<N>> operandos(std::uint64_t semilla)
{
    std::mt19937_64 rng(semilla);
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

/// @brief Divisores de aproximadamente media anchura, para que la division
///        tome el camino de Knuth D y no el atajo de `a < b`.
template <std::size_t N>
static std::vector<uint_fixed_t<N>> divisores(std::uint64_t semilla)
{
    std::mt19937_64 rng(semilla);
    std::vector<uint_fixed_t<N>> v;
    v.reserve(OPERANDOS);
    for (std::size_t i = 0; i < OPERANDOS; ++i)
    {
        uint_fixed_t<N> x{};
        for (std::size_t k = 0; k < (N + 1) / 2; ++k)
            x.set_limb(k, rng() | 1U); // impar y no nulo
        v.push_back(x);
    }
    return v;
}

template <std::size_t N, typename F>
static double mide(F op)
{
    // Calentamiento, que se descarta.
    for (std::size_t k = 0; k < 1000; ++k)
        op(k);

    CycleTimer t;
    for (std::size_t k = 0; k < VUELTAS; ++k)
        op(k);
    return static_cast<double>(t.elapsed_cycles()) / static_cast<double>(VUELTAS);
}

template <std::size_t N, typename F>
static double mejor(F op)
{
    double m{1e300};
    for (std::size_t r = 0; r < RONDAS; ++r)
    {
        const double c = mide<N>(op);
        if (c < m)
            m = c;
    }
    return m;
}

/// @brief Registra, comprueba verosimilitud e imprime una casilla.
///
/// @param toca_todos_los_limbos Si la operacion tiene que recorrer las N
///        palabras por fuerza. La COMPARACION no: sale por el limbo mas alto en
///        cuanto los dos difieren, y con operandos aleatorios eso pasa casi
///        siempre en el primero que mira. Su coste es O(1) en la practica, asi
///        que el suelo por limbo NO le aplica.
///
/// La primera version de esta funcion no distinguia, y marcaba `cmp N=64` como
/// medida imposible. Era la guarda la que mentia, no la medida: una alarma que
/// salta donde no debe se acaba ignorando, y entonces no sirve para cuando si.
static double casilla(std::size_t N, const char *operacion, double cyc, bool toca_todos_los_limbos = true)
{
    char nombre[64];
    std::snprintf(nombre, sizeof(nombre), "%s N=%zu", operacion, N);
    bench_record(nombre, cyc);

    const double por_limbo = cyc / static_cast<double>(N);
    if (toca_todos_los_limbos && por_limbo < CICLOS_MINIMOS_POR_LIMBO)
    {
        std::printf("  [OJO] %s N=%zu: %.3f cyc/op son %.4f por limbo, por debajo del suelo.\n"
                    "        El compilador se ha llevado el trabajo; la cifra NO VALE.\n",
                    operacion, N, cyc, por_limbo);
        ++g_dudosas;
    }
    return por_limbo;
}

template <std::size_t N>
static void una_anchura()
{
    const auto a = operandos<N>(0xA11CE + N);
    const auto b = operandos<N>(0xB0B + N);
    const auto d = divisores<N>(0xD1F5 + N);

    uint_fixed_t<N> sumidero{};
    volatile bool bandera = false;

    const double c_add = mejor<N>(
        [&](std::size_t k)
        {
            sumidero = a[k % OPERANDOS] + b[k % OPERANDOS];
            doNotOptimize(sumidero);
        });
    const double c_sub = mejor<N>(
        [&](std::size_t k)
        {
            sumidero = a[k % OPERANDOS] - b[k % OPERANDOS];
            doNotOptimize(sumidero);
        });
    const double c_mul = mejor<N>(
        [&](std::size_t k)
        {
            sumidero = a[k % OPERANDOS] * b[k % OPERANDOS];
            doNotOptimize(sumidero);
        });
    const double c_div = mejor<N>(
        [&](std::size_t k)
        {
            sumidero = a[k % OPERANDOS] / d[k % OPERANDOS];
            doNotOptimize(sumidero);
        });
    const double c_shl = mejor<N>(
        [&](std::size_t k)
        {
            sumidero = a[k % OPERANDOS] << 13;
            doNotOptimize(sumidero);
        });
    const double c_cmp = mejor<N>(
        [&](std::size_t k)
        {
            bandera = a[k % OPERANDOS] < b[k % OPERANDOS];
            doNotOptimize(bandera);
        });

    const double pl_add = casilla(N, "add", c_add);
    const double pl_sub = casilla(N, "sub", c_sub);
    const double pl_mul = casilla(N, "mul", c_mul);
    const double pl_div = casilla(N, "div", c_div);
    const double pl_shl = casilla(N, "shl", c_shl);
    const double pl_cmp = casilla(N, "cmp", c_cmp, /*toca_todos_los_limbos=*/false);

    // El camino que toma `operator*` a esta anchura, para que la curva se lea
    // sabiendo que se esta mirando.
    const char *camino = "escolar (bucle)";
    if (N == 2)
        camino = "128 bits";
    else if ((N & (N - 1)) == 0 && N >= NSTD_KARATSUBA_MIN && N <= NSTD_KARATSUBA_MAX)
        camino = "Karatsuba";
    else if (N <= NSTD_DESENROLLA_MAX)
        camino = "desenrollado";

    std::printf("| %4zu | %8.1f %6.2f | %8.1f %6.2f | %9.1f %7.2f | %9.1f %7.2f |"
                " %7.1f %5.2f | %6.1f %5.2f | %-15s |\n",
                N, c_add, pl_add, c_sub, pl_sub, c_mul, pl_mul, c_div, pl_div, c_shl, pl_shl, c_cmp, pl_cmp,
                camino);
}

int main()
{
    std::printf("=== Coste por operacion frente a la anchura N ===\n");
    std::printf("%zu operandos aleatorios, %zu iteraciones x %zu rondas, minimo por caso\n", OPERANDOS,
                VUELTAS, RONDAS);
    std::printf("\nCada par de columnas es: cyc/op y cyc/op POR LIMBO.\n"
                "Por limbo, lo lineal sale plano y lo cuadratico sale creciendo.\n\n");

    std::printf("+------+-----------------+-----------------+-------------------+"
                "-------------------+---------------+--------------+-----------------+\n");
    std::printf("|    N |      add        |      sub        |       mul         |"
                "       div         |     shl       |     cmp      | camino de mul   |\n");
    std::printf("+------+-----------------+-----------------+-------------------+"
                "-------------------+---------------+--------------+-----------------+\n");

    una_anchura<1>();
    una_anchura<2>();
    una_anchura<3>();
    una_anchura<4>();
    una_anchura<6>();
    una_anchura<8>();
    una_anchura<12>();
    una_anchura<16>();
    una_anchura<20>();
    una_anchura<24>();
    una_anchura<32>();
    una_anchura<48>();
    una_anchura<64>();

    std::printf("+------+-----------------+-----------------+-------------------+"
                "-------------------+---------------+--------------+-----------------+\n");

    std::printf("\nComo leerlo:\n"
                "  add, sub, shl        son O(N): su columna POR LIMBO deberia ser plana.\n"
                "  cmp                  es O(1) en la practica: sale por el limbo alto en\n"
                "                       cuanto los operandos difieren, asi que su cyc/op es\n"
                "                       casi constante y su 'por limbo' baja. Es correcto.\n"
                "  mul escolar         es O(N^2): por limbo, lineal en N.\n"
                "  mul Karatsuba       es O(N^1.585): por limbo, crece mas despacio.\n"
                "  div (Knuth D)       es O(N^2) en el peor caso.\n"
                "\nSi una columna que deberia ser plana no lo es, hay algo que explicar\n"
                "antes de publicar la cifra.\n");

    if (g_dudosas > 0)
    {
        std::printf("\n%d medida(s) por debajo del suelo fisico. Ver los [OJO]: no se pueden usar.\n",
                    g_dudosas);
        return 1;
    }
    return 0;
}
