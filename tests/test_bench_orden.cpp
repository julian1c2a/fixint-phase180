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
// @file       test_bench_orden.cpp
// @brief      El orden de las variantes del arnes de medida: al azar, repetible
//             y sin vecino fijo
// @date       2026-10-02
// =============================================================================
//
// Prueba una pieza del ARNES, no de la biblioteca: `benchs/bench_adaptativo.hpp`.
// Esta aqui porque lo que decide si una medida vale es infraestructura primaria,
// y lo que no se prueba se rompe sin que nadie lo vea -- como le paso al orden
// fijo de las variantes, que costo treinta puntos en `karatsuba` sin tocar una
// linea del codigo medido.
//
// Lo que tiene que cumplir el orden de cada ronda desde el 2 oct 2026:
//
//   1. ser una PERMUTACION de las variantes: todas, una vez cada una;
//   2. ser REPETIBLE, y el mismo en todos los compiladores: con la misma semilla,
//      los mismos ordenes. Los de referencia salen de una implementacion en
//      Python escrita aparte, no de correr esta;
//   3. no dejar VECINO FIJO: cada variante va detras de cada otra un numero
//      parecido de veces, que es justo lo que el orden rotando no cumplia;
//   4. y que `mide_entrelazado` lo USE: que las rondas cronometradas vayan en
//      ese orden y no en otro.
// =============================================================================

#include "../benchs/bench_adaptativo.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>

static int g_pasan{0};
static int g_fallan{0};

static void ok(const char *nombre, bool cond)
{
    if (cond)
    {
        std::printf("[OK]   %s\n", nombre);
        ++g_pasan;
    }
    else
    {
        std::printf("[FALLA] %s\n", nombre);
        ++g_fallan;
    }
}

template <std::size_t K>
static bool es_permutacion(const std::array<std::size_t, K> &p)
{
    std::array<int, K> visto{};
    for (const std::size_t x : p)
    {
        if (x >= K || visto[x] != 0)
            return false;
        visto[x] = 1;
    }
    return true;
}

// =============================================================================
// 1 y 2. Permutaciones, y las mismas que calcula Python
// =============================================================================

static bool coincide_con_python()
{
    // Calculado el 2 oct 2026 con una implementacion independiente en Python
    // de splitmix64 y Fisher-Yates con rechazo, semilla 0x5EED, K = 6.
    constexpr std::array<std::array<std::size_t, 6>, 4> esperado{{
        {5, 2, 1, 3, 0, 4},
        {0, 4, 5, 1, 3, 2},
        {3, 4, 2, 0, 5, 1},
        {1, 2, 4, 5, 3, 0},
    }};
    const bench::Orden o{true, 0x5EEDull};
    std::uint64_t estado = o.semilla;
    for (std::size_t r = 0; r < esperado.size(); ++r)
        if (bench::orden_de_ronda<6>(o, estado, r) != esperado[r])
            return false;
    return true;
}

static bool primer_azar_como_python()
{
    std::uint64_t e = 0x5EEDull;
    return bench::siguiente_azar(e) == 0x09F1FD9D03F0A9B4ull;
}

// =============================================================================
// 3. Sin vecino fijo, y sin posicion favorita
// =============================================================================

/// @brief Cuenta, en muchas rondas, quien va JUSTO DETRAS de quien y quien
///        ocupa cada posicion. Las dos cuentas tienen que salir parejas.
///
/// Con K = 5 y 20000 rondas, cada par ordenado se espera 4000 veces y cada
/// variante en cada posicion otras 4000. La tolerancia del 10 % son unas seis
/// desviaciones tipicas: no es un contraste estadistico fino, es una barrera
/// que el orden rotando no pasa ni de lejos -- con el, la mitad de los pares
/// no aparece NUNCA.
static bool sin_vecino_fijo(bool al_azar, std::size_t *peor_par, std::size_t *peor_pos)
{
    constexpr std::size_t K = 5;
    constexpr std::size_t RONDAS = 20000;
    std::array<std::array<std::size_t, K>, K> detras{}; // detras[i][j]: j justo detras de i
    std::array<std::array<std::size_t, K>, K> posicion{};
    const bench::Orden o{al_azar, 0xC0FFEEull};
    std::uint64_t estado = o.semilla;
    for (std::size_t r = 0; r < RONDAS; ++r)
    {
        const auto p = bench::orden_de_ronda<K>(o, estado, r);
        if (!es_permutacion(p))
            return false;
        for (std::size_t i = 0; i < K; ++i)
        {
            ++posicion[p[i]][i];
            if (i + 1 < K)
                ++detras[p[i]][p[i + 1]];
        }
    }
    const double esperado = static_cast<double>(RONDAS) / K;
    bool bien = true;
    *peor_par = static_cast<std::size_t>(esperado);
    *peor_pos = static_cast<std::size_t>(esperado);
    for (std::size_t i = 0; i < K; ++i)
        for (std::size_t j = 0; j < K; ++j)
        {
            const auto lejos = [&](std::size_t n)
            { return std::fabs(static_cast<double>(n) - esperado) > 0.10 * esperado; };
            if (i != j && lejos(detras[i][j]))
            {
                bien = false;
                *peor_par = detras[i][j];
            }
            if (lejos(posicion[i][j]))
            {
                bien = false;
                *peor_pos = posicion[i][j];
            }
        }
    return bien;
}

static bool azar_sin_sesgo()
{
    // Tres no divide a 2^64: `x % 3` a secas daria un sesgo, invisible aqui
    // pero real. Lo que se comprueba es que el rechazo no rompa nada.
    std::array<std::size_t, 3> cuenta{};
    std::uint64_t e = 0xABCDEFull;
    for (int i = 0; i < 30000; ++i)
        ++cuenta[bench::azar_menor_que(e, 3)];
    for (const std::size_t c : cuenta)
        if (c < 9500 || c > 10500)
            return false;
    std::uint64_t e1 = 1;
    return bench::azar_menor_que(e1, 1) == 0;
}

static bool rotando_conserva_el_orden_de_antes()
{
    const bench::Orden o{false, 0};
    std::uint64_t estado = 0;
    for (std::size_t r = 0; r < 7; ++r)
    {
        const auto p = bench::orden_de_ronda<4>(o, estado, r);
        for (std::size_t paso = 0; paso < 4; ++paso)
            if (p[paso] != (paso + r) % 4)
                return false;
    }
    return true;
}

// =============================================================================
// 4. `mide_entrelazado` mide en ese orden
// =============================================================================

/// @brief Una escritura que el compilador no puede quitar, para que las
///        variantes de prueba cuesten algo: sin ella, GCC, clang y MSVC reducen
///        el bucle entero a una llamada y la calibracion no tiene que medir.
static volatile std::size_t g_sumidero{0};

/// @brief Cada variante apunta su numero cuando empieza un tramo (k == 0).
///
/// Tambien empiezan tramos en la calibracion y en el calentamiento, pero esos
/// van ANTES; los `REPS x K` ultimos apuntes son las rondas cronometradas, y
/// tienen que ir exactamente en el orden que da `orden_de_ronda` con la misma
/// semilla.
static bool mide_en_ese_orden(bool al_azar)
{
    constexpr std::size_t K = 3;
    constexpr std::size_t REPS = 30;
    std::vector<int> apuntes;
    auto variante = [&apuntes](int id)
    {
        return [&apuntes, id](std::size_t k)
        {
            g_sumidero = k;
            if (k == 0)
                apuntes.push_back(id);
        };
    };
    const bench::Orden o{al_azar, 0xFEEDull};
    const auto m =
        bench::mide_entrelazado(std::make_tuple(variante(0), variante(1), variante(2)), REPS, 0.05, o);
    for (const auto &x : m)
        if (x.repeticiones != REPS)
            return false;
    if (apuntes.size() < REPS * K)
        return false;

    const std::size_t desde = apuntes.size() - REPS * K;
    std::uint64_t estado = o.semilla;
    for (std::size_t r = 0; r < REPS; ++r)
    {
        const auto p = bench::orden_de_ronda<K>(o, estado, r);
        for (std::size_t paso = 0; paso < K; ++paso)
            if (apuntes[desde + r * K + paso] != static_cast<int>(p[paso]))
                return false;
    }
    return true;
}

/// @brief Lo que destapo esta prueba el 2 oct 2026: con una operacion que no
///        cuesta nada, `calibra` desbordaba `n` hasta CERO y la medida dividia
///        por cero. Ahora se para en el techo.
///
/// La operacion vacia se queda en cero pasos con cualquier optimizacion; si un
/// compilador no la quitara, la calibracion llegaria a su objetivo y devolveria
/// algo menor que el techo -- tambien valido. Lo que no puede salir es 0.
static bool calibra_no_desborda()
{
    const std::size_t n = bench::calibra([](std::size_t) {}, 1.0);
    return n >= 1 && n <= bench::MAX_ITERACIONES;
}

int main()
{
    std::printf("=== test_bench_orden: el orden de las variantes del arnes ===\n\n");

    std::printf("--- repetible, y el mismo que calcula Python ---\n");
    ok("splitmix64 da el primer valor que da Python", primer_azar_como_python());
    ok("semilla 0x5EED, K = 6: las cuatro primeras rondas son las de Python", coincide_con_python());

    std::printf("\n--- sin vecino fijo ---\n");
    std::size_t par = 0;
    std::size_t pos = 0;
    const bool azar = sin_vecino_fijo(true, &par, &pos);
    ok("al azar: cada variante va detras de cada otra y en cada sitio unas 4000 de 20000 veces", azar);
    if (!azar)
        std::printf("        peor par %zu, peor posicion %zu (se esperaban 4000)\n", par, pos);
    ok("rotando NO lo cumple: es el defecto que se arreglo", !sin_vecino_fijo(false, &par, &pos));
    ok("azar_menor_que(3) reparte por igual, y azar_menor_que(1) da 0", azar_sin_sesgo());
    ok("BENCH_ORDEN=rotando conserva el orden de antes del 2 oct", rotando_conserva_el_orden_de_antes());

    std::printf("\n--- mide_entrelazado mide en ese orden ---\n");
    ok("al azar: las rondas cronometradas van en el orden de orden_de_ronda", mide_en_ese_orden(true));
    ok("rotando: tambien", mide_en_ese_orden(false));

    std::printf("\n--- la calibracion (el [OJO] que sale aqui es el esperado) ---\n");
    ok("una operacion que no cuesta nada no desborda n a cero", calibra_no_desborda());

    std::printf("\n%d pasan, %d fallan\n", g_pasan, g_fallan);
    return g_fallan == 0 ? 0 : 1;
}
