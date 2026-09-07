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
// @file       test_config_macros.cpp
// @brief      P0.9: guardas de include y macros de configuracion
// @date       2026-09-07
// =============================================================================
//
// POR QUE EXISTE ESTE FICHERO
//
// De las once senales que han mentido en las ultimas sesiones, las que mas caras
// salieron NO eran fallos de aritmetica sino de CONFIGURACION, y ninguna la
// habria cazado un test de multiplicar:
//
//   - Una guarda `#if !FIXED_INT_USING_LIBCPP` que apagaba las primarias
//     `nstd::is_integral...` bajo libc++. El proyecto se quedaba sin ellas y
//     tres tests no compilaban. Lo peor: la guarda CAUSABA el problema que decia
//     evitar, porque apagadas las primarias el nombre sin cualificar resolvia a
//     `std::__1::is_integral`.
//   - Un `[[no_unique_address]]` que elegia la rama equivocada porque Intel
//     define `_MSC_VER` **y** `__clang__` a la vez. Daba 40 bytes donde el
//     contrato exige 32.
//   - Una regresion de P1.1: las especializaciones de traits seguian con TRES
//     parametros de plantilla despues de anadir el cuarto, asi que
//     `nstd::is_integral_v<uint_fixed_t<2, checked>>` devolvia `false`.
//
// LA VUELTA DE TUERCA. Este test **falla si no reconoce la combinacion** de
// compilador y biblioteca estandar en la que corre. Una configuracion nueva y no
// contemplada tiene que salir en ROJO, no pasar de largo: un verde que no sabe
// donde esta no significa nada.
// =============================================================================

// Las guardas se comprueban incluyendo DOS VECES y en los dos ordenes posibles.
// El orden importa: `NSTD_TRAITS_PRIMARY_DEFINED` lo comparten los dos ficheros
// de traits y quien llegue primero define las primarias.
#include "fixed_int_traits_specializations.hpp"
#include "int128_param_traits_specializations.hpp"
#include "fixed_int_traits_specializations.hpp" // a proposito: segunda vez
#include "int128_param_traits_specializations.hpp"

#include "fixed_int_hash.hpp"
#include "fixed_int_limits.hpp"
#include "fixed_width_int_t.hpp"
#include "fixed_width_int_t.hpp" // a proposito: segunda vez
#include "intrinsics/arithmetic_operations.hpp"
#include "intrinsics/compiler_detection.hpp"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <type_traits>
#include <version>

using namespace nstd;

static int g_pasan{0};
static int g_fallan{0};

static void ok(const char *nombre, bool condicion)
{
    if (condicion)
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

// =============================================================================
// 1. Que hay debajo, dicho en voz alta
// =============================================================================
//
// Que el test IMPRIMA en que combinacion corre es media batalla: sin esto, un
// "58/58" no dice si se probo con libstdc++, con libc++ o con la STL de
// Microsoft.

#if defined(__clang__)
#define ESTE_COMPILADOR "clang"
#define ESTE_COMPILADOR_CLANG 1
#else
#define ESTE_COMPILADOR_CLANG 0
#endif
#if defined(__GNUC__) && !defined(__clang__)
#define ESTE_COMPILADOR "gcc"
#define ESTE_COMPILADOR_GCC 1
#else
#define ESTE_COMPILADOR_GCC 0
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#define ESTE_COMPILADOR "msvc"
#define ESTE_COMPILADOR_MSVC 1
#else
#define ESTE_COMPILADOR_MSVC 0
#endif
#if defined(__INTEL_LLVM_COMPILER)
#undef ESTE_COMPILADOR
#define ESTE_COMPILADOR "intel-icx"
#define ESTE_COMPILADOR_INTEL 1
#else
#define ESTE_COMPILADOR_INTEL 0
#endif
#ifndef ESTE_COMPILADOR
#define ESTE_COMPILADOR "DESCONOCIDO"
#endif

#if defined(_LIBCPP_VERSION)
#define ESTA_BIBLIOTECA "libc++"
#define ESTA_BIBLIOTECA_LIBCPP 1
#else
#define ESTA_BIBLIOTECA_LIBCPP 0
#endif
#if defined(__GLIBCXX__)
#define ESTA_BIBLIOTECA "libstdc++"
#define ESTA_BIBLIOTECA_LIBSTDCPP 1
#else
#define ESTA_BIBLIOTECA_LIBSTDCPP 0
#endif
#if defined(_MSVC_STL_VERSION)
#define ESTA_BIBLIOTECA "MS STL"
#define ESTA_BIBLIOTECA_MSSTL 1
#else
#define ESTA_BIBLIOTECA_MSSTL 0
#endif
#ifndef ESTA_BIBLIOTECA
#define ESTA_BIBLIOTECA "DESCONOCIDA"
#endif

// EL TEST FALLA SI NO SABE DONDE ESTA. Si algun dia se compila con un
// compilador o una biblioteca que nadie contemplo, esto sale en rojo y obliga a
// mirar en vez de dar por bueno un verde ciego.
static_assert(ESTE_COMPILADOR_GCC + ESTE_COMPILADOR_CLANG + ESTE_COMPILADOR_MSVC >= 1,
              "combinacion no reconocida: hay que anadir este compilador a "
              "test_config_macros.cpp antes de darlo por probado");
static_assert(ESTA_BIBLIOTECA_LIBCPP + ESTA_BIBLIOTECA_LIBSTDCPP + ESTA_BIBLIOTECA_MSSTL == 1,
              "combinacion no reconocida: hay que anadir esta biblioteca estandar a "
              "test_config_macros.cpp antes de darla por probada");

// =============================================================================
// 2. Las macros propias resuelven a lo que deben
// =============================================================================

// El ABI es lo que decide, NO el compilador. Intel ICX en Windows define
// `_MSC_VER` y `__clang__` a la vez; tratarlo como "clang" fue lo que dio 40
// bytes donde el contrato exige 32.
static_assert(INTRINSICS_USES_MSVC_ABI + INTRINSICS_USES_GNU_ABI == 1,
              "exactamente uno de los dos ABI tiene que estar activo");

#if defined(_MSC_VER)
static_assert(INTRINSICS_USES_MSVC_ABI == 1,
              "con _MSC_VER definido el ABI es el de MSVC, aunque el compilador sea clang");
#else
static_assert(INTRINSICS_USES_GNU_ABI == 1, "sin _MSC_VER el ABI es el de GNU");
#endif

// `NSTD_NO_UNIQUE_ADDRESS` sigue la MISMA regla, y por el mismo motivo.
// No se puede leer el texto de una macro desde el codigo, asi que lo que se
// comprueba es su EFECTO, que es lo que de verdad importa: ver la seccion 4.

// El detector de evaluacion constante tiene que funcionar de verdad, no ser el
// `false` de reserva: si lo fuera, las ramas constexpr de los intrinsecos nunca
// se tomarian y `static_assert` sobre aritmetica dejaria de compilar.
static_assert(INTRINSICS_IS_CONSTANT_EVALUATED(),
              "en un static_assert siempre se esta en evaluacion constante; si esto "
              "falla, la macro es el `false` de reserva y las ramas constexpr no se toman");

// Las primarias de traits las define UNO de los dos ficheros, no los dos.
#if !defined(NSTD_TRAITS_PRIMARY_DEFINED)
#error "nadie ha definido las primarias nstd::is_integral... -- vuelve la guarda de libc++"
#endif
#if !defined(NSTD_MAKE_SIGNED_PRIMARY_DEFINED)
#error "nadie ha definido nstd::make_signed / make_unsigned"
#endif

// Los umbrales de `operator*` tienen que ser coherentes entre si: si el
// desenrollado llegara mas arriba que el arranque de Karatsuba, habria anchuras
// con dos caminos posibles y el reparto dejaria de estar definido.
static_assert(NSTD_DESENROLLA_MAX < NSTD_KARATSUBA_MIN,
              "los umbrales se solapan: habria anchuras con dos caminos");
static_assert(NSTD_KARATSUBA_MIN <= NSTD_KARATSUBA_MAX, "el rango de Karatsuba esta al reves");
static_assert((NSTD_KARATSUBA_MIN & (NSTD_KARATSUBA_MIN - 1)) == 0,
              "Karatsuba parte por la mitad: su umbral tiene que ser potencia de dos");

// =============================================================================
// 3. Lo que existe, existe en TODAS las combinaciones
// =============================================================================
//
// Es el agujero del 6 sep 2026, en sus dos mitades: bajo libc++ no existian, y
// con el cuarto parametro de plantilla no encajaban. Puesto como static_assert,
// no se puede volver a abrir en silencio.

using U2 = uint_fixed_t<2>;
using I2 = int_fixed_t<2>;
using U2c = uint_fixed_t<2, overflow_policy::checked>;
using I2c = int_fixed_t<2, overflow_policy::checked>;

static_assert(nstd::is_integral_v<U2>, "nstd::is_integral no existe o no reconoce wrap");
static_assert(nstd::is_integral_v<U2c>, "...ni checked: es la regresion de P1.1");
static_assert(nstd::is_integral_v<I2> && nstd::is_integral_v<I2c>);
static_assert(nstd::is_integral_v<int> && !nstd::is_integral_v<float>,
              "y tiene que seguir delegando en std:: para los tipos del lenguaje");

static_assert(nstd::is_arithmetic_v<U2> && nstd::is_arithmetic_v<U2c>);
static_assert(nstd::is_unsigned_v<U2> && nstd::is_unsigned_v<U2c>);
static_assert(nstd::is_signed_v<I2> && nstd::is_signed_v<I2c>);
static_assert(!nstd::is_signed_v<U2> && !nstd::is_unsigned_v<I2>);

// make_signed / make_unsigned CONSERVAN la politica: mezclarlas esta prohibido
// por ADR-008, asi que un `make_signed` que devolviera `wrap` desde `checked`
// abriria por la puerta de atras lo que la de delante cierra.
static_assert(std::is_same_v<typename nstd::make_signed<U2>::type, I2>);
static_assert(std::is_same_v<typename nstd::make_signed<U2c>::type, I2c>,
              "make_signed tiene que conservar la politica");
static_assert(std::is_same_v<typename nstd::make_unsigned<I2c>::type, U2c>,
              "make_unsigned tiene que conservar la politica");

// numeric_limits y hash, para las dos politicas.
// Para `fixed_int_t` es `std::numeric_limits`, que SI es especializable para
// tipos de usuario; `nstd::numeric_limits` es el de `int128_param_t`.
static_assert(std::numeric_limits<U2>::is_specialized);
static_assert(std::numeric_limits<U2c>::is_specialized, "numeric_limits falta para checked");
static_assert(std::numeric_limits<U2>::digits == 128);
static_assert(std::is_invocable_r_v<std::size_t, std::hash<U2>, const U2 &>);
static_assert(std::is_invocable_r_v<std::size_t, std::hash<U2c>, const U2c &>,
              "std::hash falta para checked");

// std::common_type, en los dos sentidos y con la politica.
static_assert(std::is_same_v<std::common_type_t<U2, std::uint32_t>, U2>);
static_assert(std::is_same_v<std::common_type_t<std::uint32_t, U2>, U2>);
static_assert(std::is_same_v<std::common_type_t<U2c, std::uint32_t>, U2c>,
              "common_type con un entero del lenguaje tiene que conservar la politica");

// =============================================================================
// 4. Los contratos que dependen del ABI
// =============================================================================
//
// Aqui es donde se ve el EFECTO de `NSTD_NO_UNIQUE_ADDRESS`. Si eligiera la rama
// equivocada --lo que paso con Intel-- `wrap` dejaria de ocupar 8*N bytes.

static_assert(sizeof(uint_fixed_t<1>) == 8);
static_assert(sizeof(uint_fixed_t<2>) == 16);
static_assert(sizeof(uint_fixed_t<4>) == 32, "wrap tiene que seguir ocupando 8*N exactos");
static_assert(sizeof(uint_fixed_t<8>) == 64);
static_assert(sizeof(uint_fixed_t<4, overflow_policy::checked>) == 40,
              "checked son 8*N mas el limbo de la marca");

static_assert(std::is_standard_layout_v<uint_fixed_t<4>>,
              "el miembro condicional se eligio precisamente para conservar esto");
static_assert(std::is_standard_layout_v<uint_fixed_t<4, overflow_policy::checked>>,
              "y con checked tambien: era el motivo de descartar la clase base vacia");
static_assert(std::is_trivially_copyable_v<uint_fixed_t<4>>);
static_assert(std::is_trivially_copyable_v<uint_fixed_t<4, overflow_policy::checked>>);

// =============================================================================
// 5. Que la aritmetica funciona en evaluacion constante
// =============================================================================
//
// No es un test de aritmetica: es la comprobacion de que las ramas
// `INTRINSICS_IS_CONSTANT_EVALUATED()` de los intrinsecos existen y se toman. Si
// la macro fuera el `false` de reserva, estos static_assert no compilarian.

static_assert((uint_fixed_t<4>{7} * uint_fixed_t<4>{6}) == uint_fixed_t<4>{42});
static_assert((uint_fixed_t<4>{100} + uint_fixed_t<4>{23}) == uint_fixed_t<4>{123});
static_assert((uint_fixed_t<4>{100} - uint_fixed_t<4>{58}) == uint_fixed_t<4>{42});
// Una anchura de cada camino del reparto de `operator*`, para que ninguno se
// quede sin comprobar en constexpr.
static_assert((uint_fixed_t<2>{123456789} * uint_fixed_t<2>{987654321}) ==
              uint_fixed_t<2>{121932631112635269ULL});
static_assert((uint_fixed_t<8>{65536} * uint_fixed_t<8>{65536}) == uint_fixed_t<8>{4294967296ULL});

int main()
{
    std::printf("=== test_config_macros ===\n\n");
    std::printf("compilador : %s\n", ESTE_COMPILADOR);
    std::printf("biblioteca : %s\n", ESTA_BIBLIOTECA);
    std::printf("ABI        : %s\n", INTRINSICS_USES_MSVC_ABI ? "MSVC" : "GNU");
    std::printf("__int128   : %s\n", INTRINSICS_HAS_INT128 ? "si" : "no");
    std::printf("operator*  : desenrollado hasta N=%d, Karatsuba desde N=%d\n\n", (int)NSTD_DESENROLLA_MAX,
                (int)NSTD_KARATSUBA_MIN);

    // Lo de arriba ya esta comprobado en compilacion. Lo de aqui abajo es lo que
    // NO se puede comprobar con static_assert.
    std::printf("--- lo que solo se ve en ejecucion ---\n");

    // El reparto de `operator*` se toma en ejecucion --las ramas de Karatsuba
    // estan bajo `!is_constant_evaluated()`-- asi que hay que ejecutarlo para
    // saber que el camino de ejecucion da lo mismo que el de compilacion.
    {
        constexpr auto en_compilacion = uint_fixed_t<4>{123456789} * uint_fixed_t<4>{987654321};
        volatile std::uint64_t a = 123456789, b = 987654321;
        const auto en_ejecucion = uint_fixed_t<4>{a} * uint_fixed_t<4>{b};
        ok("operator* da lo mismo en compilacion que en ejecucion (N=4)", en_compilacion == en_ejecucion);
    }
    {
        constexpr auto en_compilacion = uint_fixed_t<8>{0xFFFFFFFFULL} * uint_fixed_t<8>{0xFFFFFFFFULL};
        volatile std::uint64_t a = 0xFFFFFFFFULL;
        const auto en_ejecucion = uint_fixed_t<8>{a} * uint_fixed_t<8>{a};
        ok("operator* da lo mismo en compilacion que en ejecucion (N=8)", en_compilacion == en_ejecucion);
    }
    {
        // N=32 toma el camino de Karatsuba, que solo existe en ejecucion.
        constexpr auto en_compilacion = uint_fixed_t<32>{0xDEADBEEFULL} * uint_fixed_t<32>{0xC0FFEEULL};
        volatile std::uint64_t a = 0xDEADBEEFULL, b = 0xC0FFEEULL;
        const auto en_ejecucion = uint_fixed_t<32>{a} * uint_fixed_t<32>{b};
        ok("Karatsuba (N=32) da lo mismo que el escolar de compilacion", en_compilacion == en_ejecucion);
    }
    {
        // Y una anchura del tramo que se quedaba con el bucle antes del 7 sep.
        constexpr auto en_compilacion = uint_fixed_t<20>{0x123456789ULL} * uint_fixed_t<20>{0xABCDEFULL};
        volatile std::uint64_t a = 0x123456789ULL, b = 0xABCDEFULL;
        const auto en_ejecucion = uint_fixed_t<20>{a} * uint_fixed_t<20>{b};
        ok("N=20 (desenrollado) da lo mismo en las dos evaluaciones", en_compilacion == en_ejecucion);
    }

    // `std::hash` tiene que dar lo mismo para el mismo valor. No se puede
    // comprobar en compilacion porque no es constexpr.
    {
        const U2 x{std::uint64_t{12345}};
        const U2 y{std::uint64_t{12345}};
        ok("std::hash es consistente con ==", std::hash<U2>{}(x) == std::hash<U2>{}(y));
        const U2c xc = nstd::con_comprobacion(x);
        ok("std::hash existe y funciona para checked", std::hash<U2c>{}(xc) == std::hash<U2c>{}(xc));
    }

    std::printf("\n====================================================================\n");
    std::printf("Results: %d passed, %d failed\n", g_pasan, g_fallan);
    std::printf("====================================================================\n");
    return g_fallan == 0 ? 0 : 1;
}
