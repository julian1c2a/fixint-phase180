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

// =============================================================================
// Karatsuba con reparto EQUILIBRADO: la prueba que importa es en TODA N
// =============================================================================
//
// `mul_karatsuba_pot2` solo admite potencias de dos, y por eso toda anchura
// mayor que el tope de desenrollado que no lo sea cae al bucle escolar --el
// acantilado de docs/PERFORMANCE.md--. El equilibrado quita esa limitacion.
//
// Aqui lo que hay que comprobar NO son las potencias de dos, que ya
// funcionaban, sino las que no lo son y sobre todo **las impares**, que llevan
// el camino del relleno de un limbo.

template <std::size_t N>
static bool cruza_equilibrado(std::uint64_t semilla, int vueltas)
{
    xorshift rng{semilla};
    for (int v = 0; v < vueltas; ++v)
    {
        std::array<std::uint64_t, N> a{}, b{}, ref{}, eq{};
        for (std::size_t i = 0; i < N; ++i)
        {
            a[i] = rng();
            b[i] = rng();
        }
        // Casos que un generador no da: todo unos --el que mas acarreos
        // encadena-- y una potencia de dos en el limbo mas alto.
        if (v == 0)
            for (std::size_t i = 0; i < N; ++i)
                a[i] = b[i] = ~std::uint64_t{0};
        if (v == 1)
        {
            a.fill(0);
            b.fill(0);
            a[N - 1] = 1;
            b[0] = 3;
        }

        algorithms::mul_escolar_bucle<N>(a, b, ref);
        algorithms::mul_karatsuba_equilibrado<N>(a, b, eq);

        for (std::size_t i = 0; i < N; ++i)
            if (ref[i] != eq[i])
            {
                std::printf("       ...N=%zu vuelta %d limbo %zu: escolar %llu, equilibrado %llu\n", N, v, i,
                            (unsigned long long)ref[i], (unsigned long long)eq[i]);
                return false;
            }
    }
    return true;
}

template <std::size_t N, std::size_t Tope>
static bool barre_equilibrado(std::uint64_t semilla, int vueltas)
{
    if constexpr (N > Tope)
    {
        (void)semilla;
        (void)vueltas;
        return true;
    }
    else
    {
        if (!cruza_equilibrado<N>(semilla + N * 2654435761ULL, vueltas))
        {
            std::printf("       ...la discrepancia esta en N=%zu\n", N);
            return false;
        }
        return barre_equilibrado<N + 1, Tope>(semilla, vueltas);
    }
}

// =============================================================================
// El CUADRADO: a*a tiene la mitad de trabajo, y ahora tiene camino propio
// =============================================================================
//
// Un cuadrado mal escrito falla justo donde la simetria se rompe: al doblar los
// productos cruzados y al sumar la diagonal, que NO se dobla. Por eso se prueba
// en todas las anchuras y no en unas cuantas, y con `max` --el valor que mas
// acarreos encadena al doblar-- en cada una.

template <std::size_t N>
static bool cruza_cuadrado(std::uint64_t semilla, int vueltas)
{
    xorshift rng{semilla};
    for (int v = 0; v < vueltas; ++v)
    {
        std::array<std::uint64_t, N> a{}, ref{}, sq{};
        for (std::size_t i = 0; i < N; ++i)
            a[i] = rng();
        if (v == 0)
            a.fill(~std::uint64_t{0}); // todo unos
        if (v == 1)
        {
            a.fill(0);
            a[N - 1] = 1; // una potencia de dos en el limbo mas alto
        }

        algorithms::mul_escolar_bucle<N>(a, a, ref);
        algorithms::sqr_karatsuba_equilibrado<N>(a, sq);
        for (std::size_t i = 0; i < N; ++i)
            if (ref[i] != sq[i])
            {
                std::printf("       ...modular N=%zu v=%d limbo %zu: a*a %llu, sqr %llu\n", N, v, i,
                            (unsigned long long)ref[i], (unsigned long long)sq[i]);
                return false;
            }

        // Y el cuadrado COMPLETO, contra el producto completo.
        std::array<std::uint64_t, 2 * N> anchoRef{};
        algorithms::detail::mul_full_escolar<N>(a, a, anchoRef);
        const auto anchoSqr = algorithms::sqr_full_gen<N, 8>(a);
        for (std::size_t i = 0; i < 2 * N; ++i)
            if (anchoRef[i] != anchoSqr[i])
            {
                std::printf("       ...completo N=%zu v=%d limbo %zu: a*a %llu, sqr %llu\n", N, v, i,
                            (unsigned long long)anchoRef[i], (unsigned long long)anchoSqr[i]);
                return false;
            }
    }
    return true;
}

template <std::size_t N, std::size_t Tope>
static bool barre_cuadrado(std::uint64_t semilla, int vueltas)
{
    if constexpr (N > Tope)
    {
        (void)semilla;
        (void)vueltas;
        return true;
    }
    else
    {
        if (!cruza_cuadrado<N>(semilla + N * 2654435761ULL, vueltas))
        {
            std::printf("       ...la discrepancia esta en N=%zu\n", N);
            return false;
        }
        return barre_cuadrado<N + 1, Tope>(semilla, vueltas);
    }
}

// =============================================================================
// TOOM-3: donde el signo se rompe
// =============================================================================
//
// Toom-3 evalua en x = -1, asi que calcula `a0 - a1 + a2`, que puede ser
// NEGATIVO sobre arrays de limbos sin signo. Ahi es donde falla un Toom-3 mal
// escrito, y no con operandos al azar uniformes: hace falta que a1 domine a
// a0+a2, cosa que el azar da pocas veces. Por eso se fuerzan los casos:
//
//   v=0  todo unos               (el que mas acarreos encadena)
//   v=1  a2 = 1 y el resto cero
//   v=2  a1 = max, a0 = a2 = 0   ->  el punto -1 muy negativo EN LOS DOS
//   v=3  a0 = a2 = max, a1 = 0
//   v=4  SIGNOS MEZCLADOS: un factor negativo en -1 y el otro no
//
// El v=4 es el que distingue un Toom-3 correcto de uno que "parece funcionar":
// si falta la correccion de signo del producto de doble anchura, la mitad baja
// del resultado sigue saliendo bien y solo se estropea la alta.

template <std::size_t M>
static bool cruza_toom3(int vueltas)
{
    constexpr std::size_t k = (M + 2) / 3;
    constexpr std::uint64_t MAXL = ~std::uint64_t{0};
    xorshift rng{0x700333ULL + M * 2654435761ULL};

    for (int v = 0; v < vueltas; ++v)
    {
        std::array<std::uint64_t, M> a{}, b{};
        for (std::size_t i = 0; i < M; ++i)
        {
            a[i] = rng();
            b[i] = rng();
        }
        if (v == 0)
        {
            a.fill(MAXL);
            b.fill(MAXL);
        }
        else if (v == 1)
        {
            a.fill(0);
            b.fill(0);
            a[M - 1] = 1;
            b[M - 1] = 1;
        }
        else if (v == 2)
        {
            a.fill(0);
            b.fill(0);
            for (std::size_t i = 0; i < k && k + i < M; ++i)
            {
                a[k + i] = MAXL;
                b[k + i] = MAXL;
            }
        }
        else if (v == 3)
        {
            a.fill(MAXL);
            b.fill(MAXL);
            for (std::size_t i = 0; i < k && k + i < M; ++i)
            {
                a[k + i] = 0;
                b[k + i] = 0;
            }
        }
        else if (v == 4)
        {
            a.fill(0);
            b.fill(MAXL);
            for (std::size_t i = 0; i < k && k + i < M; ++i)
            {
                a[k + i] = MAXL;
                b[k + i] = 0;
            }
        }

        std::array<std::uint64_t, 2 * M> ref{};
        algorithms::detail::mul_full_escolar<M>(a, b, ref);

        // (a) Toom-3 directo, recurriendo HASTA EL FONDO: la prueba mas dura,
        //     porque encadena todos los niveles y los errores de signo se
        //     acumulan en vez de cancelarse.
        const auto hondo = algorithms::toom3_full<M, 8, 3>(a, b);

        // (b) Por la puerta de arriba: `kmul_full_gen` con el umbral bajado a
        //     mano. Es EL MISMO camino que toma la biblioteca en M >= 1024,
        //     pero en anchuras que no tardan un minuto en instanciar.
        const auto porArriba = algorithms::kmul_full_gen<M, 8, 24>(a, b);

        for (std::size_t i = 0; i < 2 * M; ++i)
        {
            if (ref[i] != hondo[i])
            {
                std::printf("       ...toom3 hondo M=%zu v=%d limbo %zu: ref %llu, got %llu\n", M, v, i,
                            (unsigned long long)ref[i], (unsigned long long)hondo[i]);
                return false;
            }
            if (ref[i] != porArriba[i])
            {
                std::printf("       ...kmul->toom3 M=%zu v=%d limbo %zu: ref %llu, got %llu\n", M, v, i,
                            (unsigned long long)ref[i], (unsigned long long)porArriba[i]);
                return false;
            }
        }
    }
    return true;
}

template <std::size_t M, std::size_t Tope>
static bool barre_toom3(int vueltas)
{
    if constexpr (M > Tope)
    {
        (void)vueltas;
        return true;
    }
    else
    {
        if (!cruza_toom3<M>(vueltas))
        {
            std::printf("       ...la discrepancia esta en M=%zu\n", M);
            return false;
        }
        return barre_toom3<M + 1, Tope>(vueltas);
    }
}

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

    std::printf("\n--- Karatsuba EQUILIBRADO, en toda N: 2..64, impares incluidas ---\n");
    ok("el equilibrado coincide con el escolar en las 63 anchuras de 2 a 64",
       barre_equilibrado<2, 64>(0x9E3779B97F4A7C15ULL, 60));

    std::printf("\n--- el CUADRADO, modular y completo, en N = 2..48 ---\n");
    ok("el cuadrado coincide con a*a en las 47 anchuras de 2 a 48", barre_cuadrado<2, 48>(0xC0FFEEULL, 50));

    // Y por la puerta de arriba: `operator*` tiene que desviar `x * x` al
    // cuadrado y seguir dando lo mismo. Se comprueba con el TIPO, no con los
    // nucleos, porque la deteccion es por direccion y vive en `operator*`.
    {
        using U = uint_fixed_t<8>;
        xorshift rng{0x5EED5EEDULL};
        bool bien = true;
        for (int v = 0; v < 500 && bien; ++v)
        {
            U x{};
            for (std::size_t i = 0; i < 8; ++i)
                x.set_limb(i, rng());
            const U copia = x; // otro objeto, mismo valor
            bien = (x * x) == (x * copia);
        }
        ok("operator* desvia x*x al cuadrado y da lo mismo que x*copia", bien);
    }

    std::printf("\n--- TOOM-3, contra el escolar completo, en M = 24..64 ---\n");
    ok("toom3 y kmul_full_gen->toom3 coinciden con el escolar en las 41 anchuras de 24 a 64",
       barre_toom3<24, 64>(40));

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
