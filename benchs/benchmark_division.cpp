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
// @file       benchmark_division.cpp
// @brief      La linea base de `divmod`, antes de tocar el algoritmo
// @date       2026-09-17
// =============================================================================
//
// Es el primer paso del frente de la division, y va ANTES de cambiar nada: una
// cifra tomada despues de reescribir el codigo no se puede comparar con las de
// hoy. Es el mismo criterio que puso los benchmarks de P2 antes de la 2.0.
//
// LO QUE HAY QUE MEDIR NO ES UNA CURVA, ES UNA SUPERFICIE
// -------------------------------------------------------
// La multiplicacion depende de una sola variable: la anchura. La division
// depende de DOS, y quien no lo tenga en cuenta medira la casilla equivocada:
//
//   - N, la anchura de los operandos;
//   - n, los limbos SIGNIFICATIVOS del divisor.
//
// El coste de Knuth D es (N - n + 1) pasadas de O(n) trabajo cada una, o sea
// O((N-n)*n). Eso tiene un maximo en n = N/2 y **se desploma en los dos
// extremos**: con n = 1 entra el camino rapido de un limbo --N instrucciones
// DIV-- y con n = N hay una sola pasada. Medir solo "dividir dos numeros de N
// limbos al azar" da casi siempre n = N, que es el caso BARATO.
//
// Por eso cada fila barre cuatro formas de divisor, entrelazadas entre si:
//
//     n = 1      el camino rapido: `div_un_limbo`
//     n = 2      el minimo de Knuth D, con el cociente mas largo
//     n = N/2    donde la cuenta dice que esta el maximo
//     n = N      una sola pasada
//
// La columna `peor/N` es el coste del maximo dividido por la anchura: si Knuth D
// se comporta como dice la cuenta, tiene que crecer de forma lineal en N.
// =============================================================================

#include "bench_adaptativo.hpp"
#include "fixed_width_int_t.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

static constexpr std::size_t OPERANDOS = 8;

/// Dividendos al azar, todos los limbos llenos.
template <std::size_t N>
static std::vector<nstd::uint_fixed_t<N>> dividendos(std::uint64_t semilla)
{
    std::vector<nstd::uint_fixed_t<N>> v;
    v.reserve(OPERANDOS);
    std::uint64_t s = semilla;
    auto sig = [&s]() noexcept
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return s;
    };
    for (std::size_t i = 0; i < OPERANDOS; ++i)
    {
        nstd::uint_fixed_t<N> x{};
        for (std::size_t k = 0; k < N; ++k)
            x.set_limb(k, sig());
        v.push_back(x);
    }
    return v;
}

/// Divisores con EXACTAMENTE `sig` limbos significativos.
template <std::size_t N>
static std::vector<nstd::uint_fixed_t<N>> divisores(std::uint64_t semilla, std::size_t sig)
{
    std::vector<nstd::uint_fixed_t<N>> v;
    v.reserve(OPERANDOS);
    std::uint64_t s = semilla;
    auto sg = [&s]() noexcept
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return s;
    };
    for (std::size_t i = 0; i < OPERANDOS; ++i)
    {
        nstd::uint_fixed_t<N> x{};
        for (std::size_t k = 0; k < sig; ++k)
            x.set_limb(k, sg());
        // El limbo alto tiene que ser distinto de cero, o `sig` seria mentira.
        if (x.limb(sig - 1) == 0)
            x.set_limb(sig - 1, 1);
        v.push_back(x);
    }
    return v;
}

/// Un functor por forma de divisor: hacen falta cuatro tipos en un `std::tuple`.
template <std::size_t N>
struct una_forma
{
    const std::vector<nstd::uint_fixed_t<N>> *a;
    const std::vector<nstd::uint_fixed_t<N>> *b;
    nstd::uint_fixed_t<N> *sumidero;

    void operator()(std::size_t i) const
    {
        const auto par = nstd::uint_fixed_t<N>::divmod((*a)[i % OPERANDOS], (*b)[i % OPERANDOS]);
        *sumidero = par.first;
        // `limb()` devuelve por valor y `doNotOptimize` toma referencia: hace
        // falta la variable intermedia.
        std::uint64_t v = sumidero->limb(0);
        doNotOptimize(v);
    }
};

template <std::size_t N>
static void una_anchura()
{
    static const auto a = dividendos<N>(0xD1D1D3 + N * 7);
    static const auto b1 = divisores<N>(0xB1 + N * 13, 1);
    static const auto b2 = divisores<N>(0xB2 + N * 13, 2);
    static const auto bm = divisores<N>(0xB3 + N * 13, N / 2);
    static const auto bn = divisores<N>(0xB4 + N * 13, N);
    static nstd::uint_fixed_t<N> s{};

    const auto m =
        bench::mide_entrelazado(std::make_tuple(una_forma<N>{&a, &b1, &s}, una_forma<N>{&a, &b2, &s},
                                                una_forma<N>{&a, &bm, &s}, una_forma<N>{&a, &bn, &s}));

    double peor = m[0].minimo;
    for (int i = 1; i < 4; ++i)
        if (m[i].minimo > peor)
            peor = m[i].minimo;

    std::printf("| %5zu | %10.0f | %10.0f | %10.0f | %10.0f | %9.1f |\n", N, m[0].minimo, m[1].minimo,
                m[2].minimo, m[3].minimo, peor / double(N));
    std::fflush(stdout);

    char et[80];
    std::snprintf(et, sizeof(et), "divmod peor caso N=%zu", N);
    bench_record(et, peor, "cyc");
}

int main()
{
    print_header("la linea base de la division");

    std::printf("\nCiclos por `divmod`, segun la anchura N y los limbos SIGNIFICATIVOS del\n"
                "divisor. Las cuatro formas van entrelazadas, que es lo que hace\n"
                "comparables las columnas entre si.\n\n"
                "n=1 es el camino rapido de un limbo; n=N/2 es donde la cuenta de Knuth D\n"
                "pone el maximo; n=N es una sola pasada, o sea el caso BARATO -- y es el\n"
                "que sale casi siempre si uno divide dos numeros al azar sin pensarlo.\n\n");
    std::printf("|     N |      n = 1 |      n = 2 |    n = N/2 |      n = N |   peor/N |\n");
    std::printf("|------:|-----------:|-----------:|-----------:|-----------:|---------:|\n");
    std::fflush(stdout);

    una_anchura<4>();
    una_anchura<8>();
    una_anchura<16>();
    una_anchura<32>();
    una_anchura<64>();
    una_anchura<128>();
    una_anchura<256>();

    std::printf("\nSi `peor/N` crece de forma lineal, Knuth D se comporta como dice la\n"
                "cuenta: O((N-n)*n), maximo en n = N/2.\n");

    print_footer();
    return 0;
}
