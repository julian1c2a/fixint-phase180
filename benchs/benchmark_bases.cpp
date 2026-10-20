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
//
// Y EL TIPO VIEJO, EN 128 BITS, DESDE EL 20 OCT 2026. La 1.90 borra
// `int128_param_t`, y quien lo use pasara a `uint_fixed_t<2>`: el «antes» de esa
// conversion hay que medirlo antes, con LA MISMA llamada sobre LOS MISMOS
// valores y en la misma ventana. Lo hacia `benchmark_fixed_vs_param` --solo en
// decimal, con un bucle a mano por tipo y en el arnes viejo--, que se retiro ese
// dia. Aqui el viejo ESCRIBE en las seis bases, y LEE solo en la 10: no tiene
// lectura con base, solo la decimal y las de prefijo (`0x`, `0b`, `0`), y con
// prefijo la cadena de entrada ya no seria la misma. Se va con el borrado.
// =============================================================================

#define NSTD_SILENCIA_INT128_PARAM_DEPRECADO
#define NSTD_QUIERO_INT128_PARAM

#include "fixed_width_int_t.hpp"
#include "int128_parameterized.hpp"

#include "bench_adaptativo.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using namespace nstd;

static constexpr std::size_t OPERANDOS{128};

// LAS BASES QUE SE MIDEN, Y POR QUE NO SON LAS 35.
//
// Con el arnes adaptativo cada casilla cuesta 10 s, y barrer 2..36 en dos
// anchuras salia a 13,9 min por toma -- mas que todas las demas suites migradas
// juntas. Se recorta la REJILLA, no el protocolo: estas seis se miden con las
// mismas 25 vueltas de 200 ms que todo lo demas, asi que siguen comparandose con
// el resto del historico.
//
//    2  potencia de dos, y el extremo: 127 digitos en uint128
//    3  la mas cara de todas (2.596 cyc/op en to_string N=2), impar pequeña
//   10  la decimal, la que se usa
//   16  la otra potencia de dos -- y la unica forma de que este barrido pueda
//       seguir comprobando lo que el mismo afirma abajo sobre los desplazamientos
//   23  primo intermedio, sin nada especial: el caso general
//   36  el borde superior del rango soportado
//
// La forma de la curva completa ya esta medida y publicada; lo que hace falta en
// cada toma es detectar que una de ellas se mueva, y para eso bastan seis.
static constexpr int BASES[]{2, 3, 10, 16, 23, 36};

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
// de vueltas, cada una por su lado. Ahora son lambdas que `mide_entrelazado`
// alterna dentro de la misma tanda, con el orden al azar -- que es lo que hace
// legitimo comparar escribir contra leer, y el tipo nuevo contra el viejo.

/// @brief El tipo viejo de 128 bits: el que la 1.90 borra.
using viejo_t = nstd::uint128_t;

/// @brief Los mismos operandos, en el tipo viejo.
static std::vector<viejo_t> en_el_tipo_viejo(const std::vector<uint_fixed_t<2>> &xs)
{
    std::vector<viejo_t> v;
    v.reserve(xs.size());
    for (const auto &x : xs)
        v.push_back(viejo_t{x.limb(1), x.limb(0)});
    return v;
}

/// @brief ANTES DE MEDIR, que los dos tipos escriban y lean LO MISMO. Una tabla
///        que compara dos conversiones que dan cosas distintas no compara nada.
/// @return Cuantos operandos discrepan.
static int discrepan_los_tipos(const std::vector<uint_fixed_t<2>> &xs, const std::vector<viejo_t> &vs,
                               int base)
{
    int malos = 0;
    for (std::size_t i = 0; i < xs.size(); ++i)
    {
        const std::string nuevo = xs[i].to_string(base);
        bool bien = vs[i].to_string(base) == nuevo;
        if (base == 10)
            bien = bien && viejo_t::from_string(nuevo.c_str()) == vs[i];
        if (!bien)
        {
            if (malos == 0)
                std::printf("  [MAL] base %d, operando %zu: el tipo nuevo escribe %s y el viejo %s\n", base,
                            i, nuevo.c_str(), vs[i].to_string(base).c_str());
            ++malos;
        }
    }
    return malos;
}

static int g_discrepancias = 0;

template <std::size_t N>
static void una_anchura(const char *etiqueta)
{
    const auto xs = operandos<N>();
    constexpr bool con_el_viejo = (N == 2);

    std::printf("\n[%s]  %zu bits\n", etiqueta, 64 * N);
    std::printf("+------+-----------+-------------+-----------+%s\n",
                con_el_viejo ? "-------------+---------------+" : "");
    std::printf("| base | to_string | from_string |  digitos  |%s\n",
                con_el_viejo ? " viejo: to_s | viejo: from_s |" : "");
    std::printf("+------+-----------+-------------+-----------+%s\n",
                con_el_viejo ? "-------------+---------------+" : "");

    for (const int base : BASES)
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

        char nombre[64];
        if constexpr (!con_el_viejo)
        {
            const auto m = bench::mide_entrelazado(std::make_tuple(f_escribe, f_lee));
            std::snprintf(nombre, sizeof(nombre), "to_string N=%zu base %d", N, base);
            bench::registra(nombre, m[0]);
            std::snprintf(nombre, sizeof(nombre), "from_string N=%zu base %d", N, base);
            bench::registra(nombre, m[1]);
            std::printf("| %4d | %9.1f | %11.1f | %9zu |%s\n", base, m[0].minimo, m[1].minimo, ss[0].size(),
                        es_potencia_de_dos(base) ? "  <- potencia de dos" : "");
        }
        else
        {
            // EL MISMO CUERPO que las dos de arriba, cambiando solo el tipo.
            const std::vector<viejo_t> vs = en_el_tipo_viejo(xs);
            const int malos = discrepan_los_tipos(xs, vs, base);
            if (malos != 0)
            {
                std::printf("  %d operandos discrepan en base %d: NO SE MIDE.\n", malos, base);
                g_discrepancias += malos;
                continue;
            }
            std::string s_sumidero_viejo;
            viejo_t n_sumidero_viejo{};
            auto f_escribe_viejo = [&](std::size_t k)
            {
                s_sumidero_viejo = vs[k % OPERANDOS].to_string(base);
                doNotOptimize(s_sumidero_viejo);
            };
            auto f_lee_viejo = [&](std::size_t k)
            {
                n_sumidero_viejo = viejo_t::from_string(ss[k % OPERANDOS].c_str());
                doNotOptimize(n_sumidero_viejo);
            };

            std::array<bench::Medida, 4> m{};
            if (base == 10)
                m = bench::mide_entrelazado(std::make_tuple(f_escribe, f_lee, f_escribe_viejo, f_lee_viejo));
            else
            {
                const auto t = bench::mide_entrelazado(std::make_tuple(f_escribe, f_lee, f_escribe_viejo));
                m = {t[0], t[1], t[2], bench::Medida{}};
            }

            std::snprintf(nombre, sizeof(nombre), "to_string N=%zu base %d", N, base);
            bench::registra(nombre, m[0]);
            std::snprintf(nombre, sizeof(nombre), "from_string N=%zu base %d", N, base);
            bench::registra(nombre, m[1]);
            std::snprintf(nombre, sizeof(nombre), "to_string N=%zu base %d (tipo viejo)", N, base);
            bench::registra(nombre, m[2]);
            if (base == 10)
            {
                std::snprintf(nombre, sizeof(nombre), "from_string N=%zu base %d (tipo viejo)", N, base);
                bench::registra(nombre, m[3]);
                std::printf("| %4d | %9.1f | %11.1f | %9zu | %11.1f | %13.1f |%s\n", base, m[0].minimo,
                            m[1].minimo, ss[0].size(), m[2].minimo, m[3].minimo,
                            es_potencia_de_dos(base) ? "  <- potencia de dos" : "");
            }
            else
                std::printf("| %4d | %9.1f | %11.1f | %9zu | %11.1f | %13s |%s\n", base, m[0].minimo,
                            m[1].minimo, ss[0].size(), m[2].minimo, "-",
                            es_potencia_de_dos(base) ? "  <- potencia de dos" : "");
        }
    }
    std::printf("+------+-----------+-------------+-----------+%s\n",
                con_el_viejo ? "-------------+---------------+" : "");
}

int main()
{
    std::printf("=== to_string / from_string en las bases 2..36 ===\n");
    std::printf("%zu operandos aleatorios; %zu vueltas de %.0f ms por casilla, escribir y\n"
                "leer ENTRELAZADAS y con el orden al azar. Se publica el minimo.\n",
                OPERANDOS, bench::REPETICIONES, bench::MS_POR_CASILLA);
    std::printf("\nLo esperado: las potencias de dos --2 y 16-- mas baratas (van a "
                "desplazamientos);\nel resto, parecidas entre si y bajando despacio segun "
                "crece la base.\n");
    std::printf("Seis bases, no las 35: ver BASES en el fuente. Se recorta la rejilla,\n"
                "no el protocolo, asi que siguen siendo comparables con el resto.\n");

    una_anchura<2>("uint128");
    una_anchura<4>("uint256");

    if (g_discrepancias != 0)
    {
        std::printf("\n  [OJO] %d operandos en los que el tipo nuevo y el viejo NO convierten igual.\n"
                    "        Esas bases no se midieron: compararlas no diria nada.\n",
                    g_discrepancias);
        return 2;
    }
    return 0;
}
