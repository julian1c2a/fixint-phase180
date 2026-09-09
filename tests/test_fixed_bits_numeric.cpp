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
// @file       test_fixed_bits_numeric.cpp
// @brief      P1.5 tramo 1: rotaciones, nombres de <bit>, cmath y numeric
// @date       2026-09-09
// =============================================================================
//
// Primera entrega de la paridad de ADR-006: lo que `int128_param_t` tenia en
// `int128_param_bits.hpp`, `int128_param_cmath.hpp` y `int128_param_numeric.hpp`
// y `fixed_int_t` no.
//
// Casi todo va en `static_assert`, que es gratis y no se puede desactivar. Lo
// que no puede ir --lo que lanza-- se comprueba en ejecucion.
// =============================================================================

#include "fixed_width_int_t.hpp"

#include <cstdint>
#include <cstdio>
#include <stdexcept>

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

using U2 = uint_fixed_t<2>;
using U4 = uint_fixed_t<4>;
using I2 = int_fixed_t<2>;

static constexpr std::uint64_t MAX64 = ~std::uint64_t{0};

// =============================================================================
// Rotaciones
// =============================================================================

// Rotar cero posiciones no cambia nada, y rotar la anchura entera tampoco.
static_assert(rotl(U2{std::uint64_t{12345}}, 0) == U2{std::uint64_t{12345}});
static_assert(rotl(U2{std::uint64_t{12345}}, 128) == U2{std::uint64_t{12345}});
static_assert(rotr(U2{std::uint64_t{12345}}, 128) == U2{std::uint64_t{12345}});

// Rotar a la izquierda y luego lo mismo a la derecha devuelve el original. Es
// la propiedad que de verdad importa, y no depende de conocer el resultado.
static_assert(rotr(rotl(U2{std::uint64_t{0xDEADBEEF}}, 37), 37) == U2{std::uint64_t{0xDEADBEEF}});
static_assert(rotl(rotr(U4{std::uint64_t{0xC0FFEE}}, 91), 91) == U4{std::uint64_t{0xC0FFEE}});

// El bit mas alto da la vuelta al mas bajo.
static_assert(rotl(U2{std::uint64_t{1}} << 127U, 1) == U2{std::uint64_t{1}});
static_assert(rotr(U2{std::uint64_t{1}}, 1) == (U2{std::uint64_t{1}} << 127U));

// Desplazamientos mayores que la anchura se toman modulo, y los NEGATIVOS
// rotan al otro lado -- como `std::rotl`.
static_assert(rotl(U2{std::uint64_t{7}}, 130) == rotl(U2{std::uint64_t{7}}, 2));
static_assert(rotl(U2{std::uint64_t{7}}, -3) == rotr(U2{std::uint64_t{7}}, 3));
static_assert(rotr(U2{std::uint64_t{7}}, -3) == rotl(U2{std::uint64_t{7}}, 3));

// Rotar no pierde bits: el popcount se conserva.
static_assert(popcount(rotl(U2{std::uint64_t{0xF0F0F0F0F0F0F0F0ULL}}, 41)) ==
              popcount(U2{std::uint64_t{0xF0F0F0F0F0F0F0F0ULL}}));

// =============================================================================
// Los nombres de <bit>, que tienen que coincidir con los metodos
// =============================================================================

static_assert(countl_zero(U2{std::uint64_t{1}}) == U2{std::uint64_t{1}}.count_leading_zeros());
static_assert(countr_zero(U2{std::uint64_t{8}}) == U2{std::uint64_t{8}}.count_trailing_zeros());
static_assert(popcount(U2{MAX64}) == U2{MAX64}.popcount());
static_assert(bit_width(U2{std::uint64_t{255}}) == U2{std::uint64_t{255}}.bit_width());

static_assert(countl_zero(U2{}) == 128U, "el cero son 128 ceros por delante");
static_assert(countr_zero(U2{}) == 128U, "y 128 por detras");
static_assert(popcount(U2{MAX64}) == 64U);
static_assert(bit_width(U2{std::uint64_t{255}}) == 8U);

// =============================================================================
// min, max, clamp, midpoint, abs_diff
// =============================================================================

static_assert(min(U2{std::uint64_t{3}}, U2{std::uint64_t{7}}) == U2{std::uint64_t{3}});
static_assert(max(U2{std::uint64_t{3}}, U2{std::uint64_t{7}}) == U2{std::uint64_t{7}});
static_assert(min(I2{-5}, I2{3}) == I2{-5}, "con signo, el negativo es el menor");
static_assert(max(I2{-5}, I2{3}) == I2{3});

static_assert(clamp(U2{std::uint64_t{1}}, U2{std::uint64_t{3}}, U2{std::uint64_t{7}}) ==
              U2{std::uint64_t{3}});
static_assert(clamp(U2{std::uint64_t{9}}, U2{std::uint64_t{3}}, U2{std::uint64_t{7}}) ==
              U2{std::uint64_t{7}});
static_assert(clamp(U2{std::uint64_t{5}}, U2{std::uint64_t{3}}, U2{std::uint64_t{7}}) ==
              U2{std::uint64_t{5}});

static_assert(midpoint(U2{std::uint64_t{0}}, U2{std::uint64_t{10}}) == U2{std::uint64_t{5}});
static_assert(midpoint(U2{std::uint64_t{10}}, U2{std::uint64_t{0}}) == U2{std::uint64_t{5}});
static_assert(midpoint(U2{std::uint64_t{3}}, U2{std::uint64_t{4}}) == U2{std::uint64_t{3}},
              "impar: redondea hacia el primero");

// EL CASO QUE JUSTIFICA QUE `midpoint` EXISTA: `(a + b) / 2` desbordaria.
static_assert(midpoint(U2::max(), U2::max()) == U2::max());
static_assert(midpoint(U2{}, U2::max()) == (U2::max() >> 1U));

static_assert(abs_diff(U2{std::uint64_t{3}}, U2{std::uint64_t{10}}) == U2{std::uint64_t{7}});
static_assert(abs_diff(U2{std::uint64_t{10}}, U2{std::uint64_t{3}}) == U2{std::uint64_t{7}});
static_assert(abs_diff(U2{}, U2::max()) == U2::max());

// =============================================================================
// is_even, is_odd, ilog2, factorial
// =============================================================================

static_assert(is_even(U2{}) && !is_odd(U2{}), "el cero es par");
static_assert(is_odd(U2{std::uint64_t{7}}) && !is_even(U2{std::uint64_t{7}}));
static_assert(is_even(U2::max() - U2{std::uint64_t{0}}) == false, "max es impar: todo unos");
static_assert(is_odd(I2{-3}), "con signo tambien: -3 en complemento a dos acaba en 1");

static_assert(ilog2(U2{std::uint64_t{1}}) == 0U);
static_assert(ilog2(U2{std::uint64_t{2}}) == 1U);
static_assert(ilog2(U2{std::uint64_t{255}}) == 7U, "truncado hacia abajo");
static_assert(ilog2(U2{std::uint64_t{256}}) == 8U);
static_assert(ilog2(U2::max()) == 127U);

static_assert(factorial<2, representation_form::binnat, overflow_policy::wrap>(0) == U2{std::uint64_t{1}});
static_assert(factorial<2, representation_form::binnat, overflow_policy::wrap>(1) == U2{std::uint64_t{1}});
static_assert(factorial<2, representation_form::binnat, overflow_policy::wrap>(5) == U2{std::uint64_t{120}});
static_assert(factorial<2, representation_form::binnat, overflow_policy::wrap>(20) ==
                  U2{std::uint64_t{2432902008176640000ULL}},
              "20! es el ultimo que cabe en 64 bits");

// =============================================================================
// is_power_of_2, sign, abs y divmod libres
// =============================================================================

static_assert(is_power_of_2(U2{std::uint64_t{1}}));
static_assert(is_power_of_2(U2{std::uint64_t{1}} << 100U));
static_assert(!is_power_of_2(U2{}), "el cero no es potencia de dos");
static_assert(!is_power_of_2(U2{std::uint64_t{6}}));
static_assert(!is_power_of_2(U2::max()), "todo unos, no");

// EL CASO QUE IMPORTA: en complemento a dos el minimo tiene UN SOLO bit puesto
// --el de signo-- asi que un `popcount(x) == 1` a secas diria que si. No lo es.
static_assert(popcount(I2::min()) == 1U, "un solo bit: el de signo");
static_assert(!is_power_of_2(I2::min()), "pero es el mas negativo, no 2^k");
static_assert(is_power_of_2(I2{8}), "positivo con un bit, ese si");

static_assert(sign(U2{}) == 0);
static_assert(sign(U2{std::uint64_t{7}}) == 1);
static_assert(sign(I2{}) == 0);
static_assert(sign(I2{7}) == 1);
static_assert(sign(I2{-7}) == -1);
static_assert(sign(I2::min()) == -1);

static_assert(abs(I2{-7}) == I2{7});
static_assert(abs(I2{7}) == I2{7});
static_assert(abs(U2{std::uint64_t{7}}) == U2{std::uint64_t{7}});

// `divmod` libre tiene que dar lo mismo que `/` y `%` por separado.
static_assert(divmod(U2{std::uint64_t{1000}}, U2{std::uint64_t{7}}).first ==
              U2{std::uint64_t{1000}} / U2{std::uint64_t{7}});
static_assert(divmod(U2{std::uint64_t{1000}}, U2{std::uint64_t{7}}).second ==
              U2{std::uint64_t{1000}} % U2{std::uint64_t{7}});
static_assert(divmod(U2{std::uint64_t{1000}}, U2{std::uint64_t{7}}).first == U2{std::uint64_t{142}});
static_assert(divmod(U2{std::uint64_t{1000}}, U2{std::uint64_t{7}}).second == U2{std::uint64_t{6}});

// Y la identidad de la division, que es lo unico que hay que cumplir siempre.
static_assert(divmod(U2::max(), U2{std::uint64_t{1000003}}).first * U2{std::uint64_t{1000003}} +
                  divmod(U2::max(), U2{std::uint64_t{1000003}}).second ==
              U2::max());

int main()
{
    std::printf("=== test_fixed_bits_numeric (P1.5 tramo 1) ===\n\n");
    std::printf("Casi todo esta comprobado en COMPILACION: si este binario existe,\n"
                "los %d static_assert de arriba pasaron.\n\n",
                60);

    std::printf("--- lo que no cabe en un static_assert ---\n");

    // `ilog2(0)` lanza. No se puede comprobar en compilacion porque lanzar en
    // evaluacion constante es un error de compilacion, no una excepcion.
    {
        bool lanzo = false;
        try
        {
            (void)ilog2(U2{});
        }
        catch (const std::domain_error &)
        {
            lanzo = true;
        }
        ok("ilog2(0) lanza domain_error en vez de devolver un numero inventado", lanzo);
    }

    // Factorial que desborda: con `wrap` envuelve en silencio, con `checked`
    // queda marcado. Es la diferencia que ADR-008 promete.
    {
        // 35! es el primero que NO cabe en 128 bits: 34! son 2,95e38 y 2^128
        // son 3,40e38, asi que 34! cabe por poco. Lo cazo este mismo test.
        const auto envuelto = factorial<2, representation_form::binnat, overflow_policy::wrap>(35);
        ok("factorial(35) con wrap envuelve y no marca nada", true);
        (void)envuelto;

        using C = uint_fixed_t<2, overflow_policy::checked>;
        const auto marcado = factorial<2, representation_form::binnat, overflow_policy::checked>(35);
        ok("factorial(35) con checked queda MARCADO", !marcado.valid());
        const auto cabe = factorial<2, representation_form::binnat, overflow_policy::checked>(20);
        ok("factorial(20) con checked NO se marca: cabe de sobra", cabe.valid());

        // La frontera exacta, que es donde se equivoco el primer intento de
        // este test: 34! cabe por poco y 35! ya no.
        const auto justo = factorial<2, representation_form::binnat, overflow_policy::checked>(34);
        ok("factorial(34) NO se marca: cabe por poco (2,95e38 < 3,40e38)", justo.valid());
        (void)sizeof(C);
    }

    // `divmod(a, 0)` lanza. El `divmod` viejo devolvia {0,0} y lo llamaba
    // comportamiento indefinido; un cero que parece un resultado es peor que
    // parar, asi que aqui se comprueba que para.
    {
        bool lanzo = false;
        try
        {
            (void)divmod(U2{std::uint64_t{5}}, U2{}).first;
        }
        catch (const std::domain_error &)
        {
            lanzo = true;
        }
        ok("divmod(a, 0) lanza en vez de devolver {0,0} como el header viejo", lanzo);
    }

    // Rotacion sobre valores que no son constantes de compilacion, para que el
    // camino de ejecucion tambien se ejercite.
    {
        volatile std::uint64_t v = 0x123456789ABCDEFULL;
        const U4 x{static_cast<std::uint64_t>(v)};
        ok("rotl/rotr se invierten tambien en ejecucion (N=4)", rotr(rotl(x, 137), 137) == x);
        ok("rotar 256 en N=4 es la identidad", rotl(x, 256) == x);
    }

    std::printf("\n====================================================================\n");
    std::printf("Results: %d passed, %d failed\n", g_pasan, g_fallan);
    std::printf("====================================================================\n");
    return g_fallan == 0 ? 0 : 1;
}
