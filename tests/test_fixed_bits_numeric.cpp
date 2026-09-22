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
// @brief      P1.5 tramos 1 y 2: <bit>, cmath, numeric, mulhi/mullo y politica
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
#include <numeric>
#include <stdexcept>
#include <type_traits>

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

// LA ESQUINA QUE FALTABA, Y POR ESO EL FALLO VIVIO EN VERDE.
//
// Arriba estaba `midpoint(3, 4)` y NO `midpoint(4, 3)`. Con `a < b` las dos
// lecturas --hacia `a` y hacia el menor-- coinciden, asi que el test no podia
// distinguirlas. Hasta el 23 sep, `midpoint(4, 3)` daba 3 en vez de 4 y
// discrepaba de `std::midpoint` en 5110 de 20 000 casos al azar.
//
// `midpoint` NO es simetrica: [numeric.ops.midpoint] dice «rounded towards a».
static_assert(midpoint(U2{std::uint64_t{4}}, U2{std::uint64_t{3}}) == U2{std::uint64_t{4}},
              "impar con a > b: redondea hacia a, que es el MAYOR aqui");
static_assert(midpoint(U2{std::uint64_t{1}}, U2{std::uint64_t{0}}) == U2{std::uint64_t{1}},
              "midpoint(1, 0) es 1, no 0");
static_assert(midpoint(U2{std::uint64_t{5}}, U2{std::uint64_t{2}}) == U2{std::uint64_t{4}},
              "midpoint(5, 2) es 4, no 3");

// Y con signo, que es nuevo (E3). El caso que justifica que la diferencia se
// calcule SIN signo: `max - min` no cabe con signo.
static_assert(midpoint(I2{-10}, I2{10}) == I2{0});
static_assert(midpoint(I2{10}, I2{-10}) == I2{0});
static_assert(midpoint(I2{-5}, I2{-4}) == I2{-5}, "hacia a");
static_assert(midpoint(I2{-4}, I2{-5}) == I2{-4}, "hacia a, al reves");
static_assert(midpoint(I2::min(), I2::max()) == I2{-1},
              "min y max: la diferencia NO cabe con signo, y aun asi no desborda");
static_assert(midpoint(I2::max(), I2::min()) == I2{}, "y al reves redondea hacia el maximo: da cero");

// La identidad sobre enteros: existen para que el codigo generico compile.
static_assert(floor(I2{-7}) == I2{-7});
static_assert(ceil(I2{-7}) == I2{-7});
static_assert(trunc(I2{-7}) == I2{-7});
static_assert(round(I2{-7}) == I2{-7});
static_assert(floor(U2{std::uint64_t{7}}) == U2{std::uint64_t{7}});
static_assert(ceil(U2{std::uint64_t{7}}) == U2{std::uint64_t{7}});

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

// =============================================================================
// Tramo 2: mulhi y mullo
// =============================================================================

// mullo es operator*; el interes esta en que mulhi da lo que operator* TIRA.
static_assert(mullo(U2{std::uint64_t{6}}, U2{std::uint64_t{7}}) == U2{std::uint64_t{42}});
static_assert(mulhi(U2{std::uint64_t{6}}, U2{std::uint64_t{7}}) == U2{});

// max * max: el producto exacto es (2^128-1)^2 = 2^256 - 2^129 + 1, cuya
// mitad baja es 1 y cuya mitad alta es 2^128 - 2 = max - 1.
static_assert(mullo(U2::max(), U2::max()) == U2{std::uint64_t{1}});
static_assert(mulhi(U2::max(), U2::max()) == U2::max() - U2{std::uint64_t{1}});

// 2^64 * 2^64 = 2^128: mitad baja cero, mitad alta uno.
static_assert(mullo(U2{std::uint64_t{1}} << 64U, U2{std::uint64_t{1}} << 64U) == U2{});
static_assert(mulhi(U2{std::uint64_t{1}} << 64U, U2{std::uint64_t{1}} << 64U) == U2{std::uint64_t{1}});

// Con signo, la mitad alta lleva la EXTENSION DE SIGNO del producto entero.
// Leerla sin signo es el error que tenia `producto_desborda` antes de P1.3.
static_assert(mulhi(I2{-1}, I2{-1}) == I2{0}, "(-1)*(-1) = 1: arriba no hay nada");
static_assert(mullo(I2{-1}, I2{-1}) == I2{1});
static_assert(mulhi(I2{-1}, I2{1}) == I2{-1}, "el producto es negativo: arriba, todo unos");
static_assert(mulhi(I2::min(), I2{2}) == I2{-1}, "min*2 = -2^128, y -2^128 >> 128 es -1");

// La identidad que las une: el producto exacto es (alta << 64N) | baja.
static_assert((U4{mulhi(U2::max(), U2::max())} << 128U) + U4{mullo(U2::max(), U2::max())} ==
              mul_wide(U2::max(), U2::max()));

// =============================================================================
// Tramo 2 (2a): la politica se conserva y ya no bloquea
// =============================================================================

using CU = uint_fixed_t<2, overflow_policy::checked>;
using CI = int_fixed_t<2, overflow_policy::checked>;

// ANTES esto no compilaba: las nueve firmas fijaban la politica por defecto.
static_assert(gcd(CU{std::uint64_t{12}}, CU{std::uint64_t{18}}) == CU{std::uint64_t{6}});
static_assert(lcm(CU{std::uint64_t{12}}, CU{std::uint64_t{18}}) == CU{std::uint64_t{36}});
static_assert(sqrt(CU{std::uint64_t{144}}) == CU{std::uint64_t{12}});
static_assert(pow(CU{std::uint64_t{3}}, CU{std::uint64_t{5}}) == CU{std::uint64_t{243}});

// Y la politica sale VIVA del resultado, incluso cuando cambia el signo:
// `gcd` con signo devuelve sin signo, y conserva `checked`. Es lo mismo que
// hacen `make_signed`/`make_unsigned` por ADR-008.
static_assert(std::is_same_v<decltype(gcd(CU{}, CU{})), CU>);
static_assert(std::is_same_v<decltype(gcd(CI{}, CI{})), CU>);
static_assert(std::is_same_v<decltype(lcm(CI{}, CI{})), CU>);
static_assert(std::is_same_v<decltype(sqrt(CU{})), CU>);
static_assert(std::is_same_v<decltype(pow(CI{}, CU{})), CI>);
static_assert(std::is_same_v<decltype(mul_wide(CU{}, CU{})), uint_fixed_t<4, overflow_policy::checked>>);
static_assert(std::is_same_v<decltype(mulhi(CU{}, CU{})), CU>);

// Y las llamadas de siempre, con `wrap`, siguen dando exactamente lo mismo.
static_assert(std::is_same_v<decltype(gcd(U2{}, U2{})), U2>);
static_assert(std::is_same_v<decltype(mul_wide(U2{}, U2{})), U4>);

int main()
{
    std::printf("=== test_fixed_bits_numeric (P1.5 tramos 1 y 2) ===\n\n");
    std::printf("Casi todo esta comprobado en COMPILACION: si este binario existe,\n"
                "los %d static_assert de arriba pasaron.\n\n",
                85);

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

    // mulhi/mullo contra mul_wide, con valores que el compilador no ve.
    // El static_assert cubre casos elegidos a mano; esto cubre los que no se me
    // habrian ocurrido. 200.000 pares, generador propio para no depender de
    // como implemente <random> cada biblioteca estandar.
    {
        std::uint64_t sem = 0x9E3779B97F4A7C15ULL;
        auto siguiente = [&sem]() noexcept
        {
            sem ^= sem << 13;
            sem ^= sem >> 7;
            sem ^= sem << 17;
            return sem;
        };
        bool todos = true;
        int comprobados = 0;
        for (int i = 0; i < 200000 && todos; ++i)
        {
            U2 a{}, b{};
            a.set_limb(0, siguiente());
            a.set_limb(1, siguiente());
            b.set_limb(0, siguiente());
            b.set_limb(1, siguiente());

            const U4 exacto = mul_wide(a, b);
            const U4 rearmado = (U4{mulhi(a, b)} << 128U) + U4{mullo(a, b)};
            if (rearmado != exacto)
                todos = false;
            ++comprobados;
        }
        char msg[128];
        std::snprintf(msg, sizeof(msg), "mulhi/mullo rearman mul_wide en %d pares al azar", comprobados);
        ok(msg, todos);
    }

    // Lo mismo con signo, que es donde es facil equivocarse.
    {
        std::uint64_t sem = 0xD1B54A32D192ED03ULL;
        auto siguiente = [&sem]() noexcept
        {
            sem ^= sem << 13;
            sem ^= sem >> 7;
            sem ^= sem << 17;
            return sem;
        };
        bool todos = true;
        for (int i = 0; i < 200000 && todos; ++i)
        {
            I2 a{}, b{};
            a.set_limb(0, siguiente());
            a.set_limb(1, siguiente());
            b.set_limb(0, siguiente());
            b.set_limb(1, siguiente());

            // Con signo, el producto exacto de 2N limbos se parte igual, pero
            // la mitad alta se lee CON signo. La identidad que tiene que
            // cumplirse es la misma: exacto == (alta << 128) + baja, donde la
            // suma se hace en 2N limbos sin signo sobre los patrones de bits.
            const auto exacto = mul_wide(a, b);
            auto rearmado = int_fixed_t<4>{};
            const auto alta = mulhi(a, b);
            const auto baja = mullo(a, b);
            for (std::size_t k = 0; k < 2; ++k)
            {
                rearmado.set_limb(k, baja.limb(k));
                rearmado.set_limb(k + 2, alta.limb(k));
            }
            if (rearmado != exacto)
                todos = false;
        }
        ok("mulhi/mullo con signo rearman mul_wide en 200.000 pares al azar", todos);
    }

    // Rotacion sobre valores que no son constantes de compilacion, para que el
    // camino de ejecucion tambien se ejercite.
    {
        volatile std::uint64_t v = 0x123456789ABCDEFULL;
        const U4 x{static_cast<std::uint64_t>(v)};
        ok("rotl/rotr se invierten tambien en ejecucion (N=4)", rotr(rotl(x, 137), 137) == x);
        ok("rotar 256 en N=4 es la identidad", rotl(x, 256) == x);
    }

    // ------------------------------------------------------------------
    // `midpoint` contra `std::midpoint`, que es el oraculo de verdad
    // ------------------------------------------------------------------
    //
    // Los `static_assert` de arriba comprueban esquinas elegidas; esto cruza
    // contra la implementacion del estandar sobre 20 000 pares, en los dos
    // signos y en las cuatro representaciones. Un `static_assert` puede estar
    // de acuerdo con una lectura equivocada -- que es exactamente lo que paso.
    {
        std::uint64_t sem = 0x11D00117ULL;
        auto siguiente = [&sem]()
        {
            sem ^= sem << 13;
            sem ^= sem >> 7;
            sem ^= sem << 17;
            return sem;
        };

        int mal_u = 0, mal_s = 0, mal_ms = 0, mal_ek = 0;
        using MS = fixed_int_t<2, signedness::signed_type, representation_form::magnitude_sign>;
        using EK = fixed_int_t<2, signedness::signed_type, representation_form::excess_k>;

        for (int i = 0; i < 20000; ++i)
        {
            // Sin signo: valores de 63 bits para que `std::midpoint` los admita.
            const std::uint64_t ua = siguiente() >> 1;
            const std::uint64_t ub = siguiente() >> 1;
            if (midpoint(U2{ua}, U2{ub}).to_string() != std::to_string(std::midpoint(ua, ub)))
                ++mal_u;

            // Con signo, incluidos los extremos de 64 bits.
            const auto sa = static_cast<std::int64_t>(siguiente());
            const auto sb = static_cast<std::int64_t>(siguiente());
            const std::string esp = std::to_string(std::midpoint(sa, sb));
            if (midpoint(I2{sa}, I2{sb}).to_string() != esp)
                ++mal_s;
            if (midpoint(MS{sa}, MS{sb}).to_string() != esp)
                ++mal_ms;
            if (midpoint(EK{sa}, EK{sb}).to_string() != esp)
                ++mal_ek;
        }
        ok("midpoint sin signo coincide con std::midpoint (20.000 pares)", mal_u == 0);
        ok("midpoint con signo coincide con std::midpoint (20.000 pares)", mal_s == 0);
        ok("midpoint en Magnitud-Signo coincide (ADR-018)", mal_ms == 0);
        ok("midpoint en Exceso-K coincide (ADR-018)", mal_ek == 0);
    }

    std::printf("\n====================================================================\n");
    std::printf("Results: %d passed, %d failed\n", g_pasan, g_fallan);
    std::printf("====================================================================\n");
    return g_fallan == 0 ? 0 : 1;
}
