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
// @file       test_mul_kernels.cpp
// @brief      Los nucleos sueltos tienen que dar EXACTAMENTE lo mismo que la clase
// @date       2026-09-10
// =============================================================================
//
// `include/algorithms/mul_kernels.hpp` saca los tres caminos de `operator*` a
// funciones libres sobre `std::array`, para poder compararlos entre si en la
// misma N y en el mismo proceso -- sin eso no se puede cumplir la regla de
// rondas entrelazadas del protocolo de medicion.
//
// Mientras las dos implementaciones convivan, esto es lo que impide que se
// separen: **bit a bit, sobre operandos al azar, en todas las anchuras**. Los
// tres nucleos calculan la misma cosa --los N limbos bajos de a*b-- asi que
// cualquier discrepancia es un error, no una diferencia de estrategia.
// =============================================================================

#include "algorithms/mul_kernels.hpp"
#include "fixed_width_int_t.hpp"

#include <array>
#include <cstdint>
#include <cstdio>

using namespace nstd;

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

/// @brief Generador propio, para no depender de como implemente <random> cada
///        biblioteca estandar: los tres compiladores tienen que ver los MISMOS
///        operandos.
struct xorshift
{
    std::uint64_t s;
    constexpr std::uint64_t operator()() noexcept
    {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return s;
    }
};

// =============================================================================
// Los dos escolares, contra la clase, en todas las anchuras
// =============================================================================

template <std::size_t N>
static bool cruza_escolares(std::uint64_t semilla, int vueltas)
{
    xorshift rng{semilla};
    for (int v = 0; v < vueltas; ++v)
    {
        uint_fixed_t<N> a{}, b{};
        for (std::size_t i = 0; i < N; ++i)
        {
            a.set_limb(i, rng());
            b.set_limb(i, rng());
        }

        const uint_fixed_t<N> esperado = a * b; // lo que hace la clase hoy

        std::array<std::uint64_t, N> r_bucle{}, r_desen{};
        algorithms::mul_escolar_bucle<N>(a.limbs(), b.limbs(), r_bucle);
        algorithms::mul_escolar_desenrollado<N>(a.limbs(), b.limbs(), r_desen);

        for (std::size_t i = 0; i < N; ++i)
        {
            if (r_bucle[i] != esperado.limb(i))
                return false;
            if (r_desen[i] != esperado.limb(i))
                return false;
        }
    }
    return true;
}

/// @brief Recorre N = 1..Tope en tiempo de compilacion.
template <std::size_t N, std::size_t Tope>
static bool barre_escolares(std::uint64_t semilla, int vueltas)
{
    if constexpr (N > Tope)
    {
        (void)semilla;
        (void)vueltas;
        return true;
    }
    else
    {
        if (!cruza_escolares<N>(semilla + N, vueltas))
        {
            std::printf("       ...la discrepancia esta en N=%zu\n", N);
            return false;
        }
        return barre_escolares<N + 1, Tope>(semilla, vueltas);
    }
}

// =============================================================================
// Karatsuba, solo en las potencias de dos que admite
// =============================================================================

template <std::size_t N>
static bool cruza_karatsuba(std::uint64_t semilla, int vueltas)
{
    xorshift rng{semilla};
    for (int v = 0; v < vueltas; ++v)
    {
        uint_fixed_t<N> a{}, b{};
        for (std::size_t i = 0; i < N; ++i)
        {
            a.set_limb(i, rng());
            b.set_limb(i, rng());
        }

        const uint_fixed_t<N> esperado = a * b;

        std::array<std::uint64_t, N> r{};
        algorithms::mul_karatsuba_pot2<N>(a.limbs(), b.limbs(), r, algorithms::medio_como_hoy{});

        for (std::size_t i = 0; i < N; ++i)
            if (r[i] != esperado.limb(i))
            {
                std::printf("       ...N=%zu, limbo %zu: nucleo %llu, clase %llu\n", N, i,
                            (unsigned long long)r[i], (unsigned long long)esperado.limb(i));
                return false;
            }
    }
    return true;
}

// =============================================================================

int main()
{
    std::printf("=== test_mul_kernels: los nucleos sueltos frente a la clase ===\n\n");

    std::printf("--- los dos escolares, N = 1..40, 200 pares al azar cada uno ---\n");
    ok("escolar en bucle y desenrollado coinciden con la clase en N=1..40",
       barre_escolares<1, 40>(0x9E3779B97F4A7C15ULL, 200));

    std::printf("\n--- Karatsuba, en las potencias de dos que admite ---\n");
    ok("Karatsuba coincide con la clase en N=2", cruza_karatsuba<2>(0xD1B54A32D192ED03ULL, 200));
    ok("Karatsuba coincide con la clase en N=4", cruza_karatsuba<4>(0xD1B54A32D192ED04ULL, 200));
    ok("Karatsuba coincide con la clase en N=8", cruza_karatsuba<8>(0xD1B54A32D192ED08ULL, 200));
    ok("Karatsuba coincide con la clase en N=16", cruza_karatsuba<16>(0xD1B54A32D192ED10ULL, 200));
    ok("Karatsuba coincide con la clase en N=32", cruza_karatsuba<32>(0xD1B54A32D192ED20ULL, 200));
    ok("Karatsuba coincide con la clase en N=64", cruza_karatsuba<64>(0xD1B54A32D192ED40ULL, 100));

    std::printf("\n--- casos que no salen de un generador al azar ---\n");
    {
        // max * max: el que mas acarreos encadena.
        constexpr std::size_t N = 8;
        uint_fixed_t<N> m{};
        for (std::size_t i = 0; i < N; ++i)
            m.set_limb(i, ~std::uint64_t{0});
        const uint_fixed_t<N> esperado = m * m;

        std::array<std::uint64_t, N> r1{}, r2{}, r3{};
        algorithms::mul_escolar_bucle<N>(m.limbs(), m.limbs(), r1);
        algorithms::mul_escolar_desenrollado<N>(m.limbs(), m.limbs(), r2);
        algorithms::mul_karatsuba_pot2<N>(m.limbs(), m.limbs(), r3, algorithms::medio_como_hoy{});

        bool bien = true;
        for (std::size_t i = 0; i < N; ++i)
            bien =
                bien && r1[i] == esperado.limb(i) && r2[i] == esperado.limb(i) && r3[i] == esperado.limb(i);
        ok("max*max: los tres nucleos coinciden con la clase", bien);
    }
    {
        // Cero y uno, que es donde se cuela un `fill` olvidado.
        constexpr std::size_t N = 4;
        uint_fixed_t<N> z{}, u{};
        u.set_limb(0, 1);
        std::array<std::uint64_t, N> r{};
        r.fill(0xAAAA); // basura previa: el nucleo tiene que limpiarla
        algorithms::mul_escolar_bucle<N>(z.limbs(), u.limbs(), r);
        bool cero = true;
        for (std::size_t i = 0; i < N; ++i)
            cero = cero && r[i] == 0;
        ok("0*1 da cero aunque el destino llegara sucio", cero);

        r.fill(0xBBBB);
        algorithms::mul_escolar_desenrollado<N>(u.limbs(), u.limbs(), r);
        bool uno = r[0] == 1;
        for (std::size_t i = 1; i < N; ++i)
            uno = uno && r[i] == 0;
        ok("1*1 da uno aunque el destino llegara sucio", uno);
    }

    std::printf("\n====================================================================\n");
    std::printf("Results: %d passed, %d failed\n", g_pasan, g_fallan);
    std::printf("====================================================================\n");
    return g_fallan == 0 ? 0 : 1;
}
