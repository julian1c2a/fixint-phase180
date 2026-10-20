// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// =============================================================================
// @file       experimento_carga.cpp
// @brief      La misma casilla en muchas ventanas seguidas, para los experimentos
//             de condiciones de medida (E2 y E3 de la plataforma dedicada)
// @date       2026-10-20
// =============================================================================
//
// NO ES UNA SUITE DEL HISTORICO: no empieza por `benchmark_`, y `bench_history`
// no lo lanza. Vive en `benchs/` para que el job del CI que compila los bancos
// lo compile tambien y no se pudra entre experimento y experimento.
//
// Lo usan `scripts/condiciones_benchmark/experimentos/`: E2 (que le hace una
// carga conocida a la medida, con el turbo encendido y apagado) y el calor de
// P2.23. Nacio en la sonda de E2 de P2.22, el 1 oct 2026, y se trajo aqui el 20
// oct para poder repetir el experimento en Linux tal cual se hizo en Windows.
//
// TODO FIJO MENOS LA PERTURBACION: mismos operandos, misma operacion, mismo
// protocolo (las vueltas y los ms del arnes, dos variantes entrelazadas). Las
// ventanas que no tocan ninguna carga son la referencia de las demas DENTRO de la
// misma ejecucion, y no hace falta comparar contra otra toma.
//
// Las dos variantes son una multiplicacion y una suma de 256 bits: una usa el
// multiplicador y la otra solo la ALU, por si la carga las toca distinto.
//
// `VENTANAS=<n>` en el entorno fija cuantas ventanas se miden (42 por omision:
// unos siete minutos con 25 vueltas de 200 ms).
// =============================================================================

#include "fixed_width_int_t.hpp"

#include "bench_adaptativo.hpp"

#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace nstd;

static constexpr std::size_t OPERANDOS{64};

static int ventanas()
{
    const char *v = std::getenv("VENTANAS");
    const int n = (v != nullptr && *v != '\0') ? std::atoi(v) : 42;
    return n > 0 ? n : 42;
}

int main()
{
    std::mt19937_64 rng{20261001};
    std::vector<uint_fixed_t<4>> a, b;
    for (std::size_t i = 0; i < OPERANDOS; ++i)
    {
        uint_fixed_t<4> x{}, y{};
        for (std::size_t k = 0; k < 4; ++k)
        {
            x.set_limb(k, rng());
            y.set_limb(k, rng());
        }
        a.push_back(x);
        b.push_back(y);
    }

    uint_fixed_t<4> sumidero{};
    auto f_mul = [&](std::size_t k)
    {
        sumidero = a[k % OPERANDOS] * b[k % OPERANDOS];
        doNotOptimize(sumidero);
    };
    auto f_add = [&](std::size_t k)
    {
        sumidero = a[k % OPERANDOS] + b[k % OPERANDOS];
        doNotOptimize(sumidero);
    };

    const int n = ventanas();
    for (int w = 0; w < n; ++w)
    {
        const auto m = bench::mide_entrelazado(std::make_tuple(f_mul, f_add));
        char nombre[32];
        std::snprintf(nombre, sizeof(nombre), "v%02d / mul", w);
        bench::registra(nombre, m[0]);
        std::snprintf(nombre, sizeof(nombre), "v%02d / add", w);
        bench::registra(nombre, m[1]);
        std::printf("v%02d  mul %8.2f  add %8.2f\n", w, m[0].minimo, m[1].minimo);
        std::fflush(stdout);
    }
    return 0;
}
