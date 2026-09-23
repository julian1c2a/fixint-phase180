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
// @file       test_e3_charconv.cpp
// @brief      E3: nstd::to_chars y nstd::from_chars
// @date       2026-09-23
// =============================================================================
//
// EL ORACULO ES `std::` DE VERDAD, NO LO QUE YO CREA QUE HACE
//
// Donde el valor cabe en `std::uint64_t` se compara contra `std::to_chars` y
// `std::from_chars` reales, caracter a caracter y codigo de error a codigo de
// error. Comprobar contra lo que uno recuerda del estandar es como comprobar un
// algoritmo contra su propia implementacion: siempre sale bien.
//
// Y las esquinas se construyen: el hueco JUSTO, el hueco de uno menos, el rango
// vacio, el que no tiene digitos, el que desborda por un digito.
// =============================================================================
#include "fixed_width_int_t.hpp"

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <system_error>
#include <vector>

using namespace nstd;

static int g_pasan = 0;
static int g_fallan = 0;

static void ok(const char *que, bool cond)
{
    if (cond)
    {
        ++g_pasan;
        std::printf("[ OK ] %s\n", que);
    }
    else
    {
        ++g_fallan;
        std::printf("[FAIL] %s\n", que);
    }
}

using U2 = uint_fixed_t<2>;
using I2 = int_fixed_t<2>;
using MS = fixed_int_t<2, signedness::signed_type, representation_form::magnitude_sign>;
using EK = fixed_int_t<2, signedness::signed_type, representation_form::excess_k>;

int main()
{
    std::printf("====================================================================\n");
    std::printf("E3: nstd::to_chars / nstd::from_chars\n");
    std::printf("====================================================================\n\n");

    // Valores que caben en uint64_t, para poder cruzar contra `std::`.
    const std::vector<std::uint64_t> chicos = {0,
                                               1,
                                               2,
                                               7,
                                               8,
                                               9,
                                               10,
                                               15,
                                               16,
                                               17,
                                               35,
                                               36,
                                               37,
                                               99,
                                               100,
                                               255,
                                               256,
                                               1000,
                                               65535,
                                               65536,
                                               1000000007ULL,
                                               0x7FFFFFFFFFFFFFFFULL,
                                               0x8000000000000000ULL,
                                               0xFFFFFFFFFFFFFFFFULL,
                                               0xFFFFFFFFFFFFFFFEULL};

    // --- 1. to_chars contra std::to_chars, en todas las bases ---------------
    {
        int mal = 0;
        for (std::uint64_t v : chicos)
        {
            for (int base = 2; base <= 36; ++base)
            {
                char mio[256] = {};
                char suyo[256] = {};
                const auto rm = nstd::to_chars(mio, mio + sizeof(mio), U2{v}, base);
                const auto rs = std::to_chars(suyo, suyo + sizeof(suyo), v, base);
                if (rm.ec != std::errc{} || rs.ec != std::errc{})
                {
                    ++mal;
                    continue;
                }
                const std::string sm(mio, rm.ptr);
                const std::string ss(suyo, rs.ptr);
                if (sm != ss)
                    ++mal;
            }
        }
        ok("to_chars coincide con std::to_chars en las 35 bases", mal == 0);
    }

    // --- 2. from_chars contra std::from_chars -------------------------------
    {
        int mal = 0;
        for (std::uint64_t v : chicos)
        {
            for (int base = 2; base <= 36; ++base)
            {
                char buf[256] = {};
                const auto w = std::to_chars(buf, buf + sizeof(buf), v, base);

                U2 mio{};
                std::uint64_t suyo = 0;
                const auto rm = nstd::from_chars(buf, w.ptr, mio, base);
                const auto rs = std::from_chars(buf, w.ptr, suyo, base);

                if (rm.ec != rs.ec || rm.ptr != rs.ptr)
                    ++mal;
                else if (rm.ec == std::errc{} && mio != U2{suyo})
                    ++mal;
            }
        }
        ok("from_chars coincide con std::from_chars: valor, ptr y ec", mal == 0);
    }

    // --- 3. el viaje de ida y vuelta, con 128 bits de verdad ----------------
    {
        int mal_u = 0, mal_s = 0, mal_ms = 0, mal_ek = 0;
        const std::vector<std::string> grandes = {
            "0",
            "1",
            "-1",
            "170141183460469231731687303715884105727", // 2^127 - 1
            "-170141183460469231731687303715884105727",
            "340282366920938463463374607431768211455", // 2^128 - 1, solo sin signo
            "85070591730234615865843651857942052864",  // 2^126
            "123456789012345678901234567890"};
        for (const std::string &s : grandes)
        {
            for (int base = 2; base <= 36; ++base)
            {
                // sin signo
                {
                    const auto pr = U2::try_from_string(s.c_str());
                    if (pr.success())
                    {
                        char buf[600] = {};
                        const auto w = nstd::to_chars(buf, buf + sizeof(buf), pr.value, base);
                        U2 vuelta{};
                        const auto rd = nstd::from_chars(buf, w.ptr, vuelta, base);
                        if (w.ec != std::errc{} || rd.ec != std::errc{} || rd.ptr != w.ptr ||
                            vuelta != pr.value)
                            ++mal_u;
                    }
                }
                // con signo, en las tres representaciones
                {
                    const auto pr = I2::try_from_string(s.c_str());
                    if (!pr.success())
                        continue;
                    const I2 x = pr.value;
                    char buf[600] = {};
                    const auto w = nstd::to_chars(buf, buf + sizeof(buf), x, base);
                    I2 vuelta{};
                    const auto rd = nstd::from_chars(buf, w.ptr, vuelta, base);
                    if (w.ec != std::errc{} || rd.ec != std::errc{} || vuelta != x)
                        ++mal_s;

                    const MS xm = MS::from_string(s.c_str());
                    // se salta si MS no puede representarlo (ADR-017)
                    if (xm.to_string() == x.to_string())
                    {
                        char b2[600] = {};
                        const auto w2 = nstd::to_chars(b2, b2 + sizeof(b2), xm, base);
                        MS v2{};
                        const auto r2 = nstd::from_chars(b2, w2.ptr, v2, base);
                        if (w2.ec != std::errc{} || r2.ec != std::errc{} || v2 != xm)
                            ++mal_ms;
                    }

                    const EK xe = EK::from_string(s.c_str());
                    char b3[600] = {};
                    const auto w3 = nstd::to_chars(b3, b3 + sizeof(b3), xe, base);
                    EK v3{};
                    const auto r3 = nstd::from_chars(b3, w3.ptr, v3, base);
                    if (w3.ec != std::errc{} || r3.ec != std::errc{} || v3 != xe)
                        ++mal_ek;
                }
            }
        }
        ok("ida y vuelta sin signo, 35 bases", mal_u == 0);
        ok("ida y vuelta con signo, 35 bases", mal_s == 0);
        ok("ida y vuelta en Magnitud-Signo (ADR-018)", mal_ms == 0);
        ok("ida y vuelta en Exceso-K (ADR-018)", mal_ek == 0);
    }

    // --- 4. EL HUECO JUSTO, y uno menos --------------------------------------
    //
    // Es la esquina que separa `to_chars` de `to_string`: el contrato dice que
    // si no cabe NO se escribe nada util y `ptr == last`.
    {
        const U2 v{std::uint64_t{123456789}};
        const std::string esperado = v.to_string();
        const std::size_t n = esperado.size();

        char justo[64] = {};
        const auto r1 = nstd::to_chars(justo, justo + n, v);
        ok("con el hueco JUSTO, escribe y ec vale {}",
           r1.ec == std::errc{} && std::string(justo, r1.ptr) == esperado);
        ok("con el hueco justo, ptr queda al final del hueco", r1.ptr == justo + n);

        char corto[64] = {};
        const auto r2 = nstd::to_chars(corto, corto + n - 1, v);
        ok("con UN hueco menos, da value_too_large", r2.ec == std::errc::value_too_large);
        ok("y con un hueco menos, ptr == last", r2.ptr == corto + n - 1);

        char cero[4] = {};
        const auto r3 = nstd::to_chars(cero, cero, v);
        ok("con hueco de tamaño cero, da value_too_large", r3.ec == std::errc::value_too_large);
    }

    // --- 5. las esquinas de from_chars ---------------------------------------
    {
        U2 v{std::uint64_t{7}};
        const U2 intacto = v;

        const char vacio[] = "";
        const auto r1 = nstd::from_chars(vacio, vacio, v, 10);
        ok("rango vacio: invalid_argument y ptr == first",
           r1.ec == std::errc::invalid_argument && r1.ptr == vacio);
        ok("rango vacio: no toca el valor", v == intacto);

        const char letras[] = "xyz";
        const auto r2 = nstd::from_chars(letras, letras + 3, v, 10);
        ok("sin digitos validos: invalid_argument y ptr == first",
           r2.ec == std::errc::invalid_argument && r2.ptr == letras);
        ok("sin digitos validos: no toca el valor", v == intacto);

        // para en el primer caracter que no vale en esta base
        const char mezcla[] = "1234abc";
        U2 w{};
        const auto r3 = nstd::from_chars(mezcla, mezcla + 7, w, 10);
        ok("para en el primer caracter que no vale en la base",
           r3.ec == std::errc{} && r3.ptr == mezcla + 4 && w == U2{std::uint64_t{1234}});

        // ... y en base 16 esos mismos caracteres SI valen
        U2 w16{};
        const auto r4 = nstd::from_chars(mezcla, mezcla + 7, w16, 16);
        ok("en base 16 consume 'abc' tambien",
           r4.ec == std::errc{} && r4.ptr == mezcla + 7 && w16 == U2{std::uint64_t{0x1234abcULL}});

        // NO hace falta terminador nulo: se le da un trozo de una cadena mayor
        const char largo[] = "999998888877777";
        U2 trozo{};
        const auto r5 = nstd::from_chars(largo, largo + 5, trozo, 10);
        ok("no necesita terminador nulo: respeta `last`",
           r5.ec == std::errc{} && r5.ptr == largo + 5 && trozo == U2{std::uint64_t{99999}});
    }

    // --- 6. el desbordamiento: por UN digito ---------------------------------
    {
        const std::string max = U2::max().to_string();
        U2 v{std::uint64_t{7}};
        const U2 intacto = v;

        const auto r1 = nstd::from_chars(max.data(), max.data() + max.size(), v, 10);
        ok("el maximo justo SI entra", r1.ec == std::errc{} && v == U2::max());

        const std::string pasa = max + "0"; // un digito mas: x10
        U2 w{std::uint64_t{7}};
        const U2 w_intacto = w;
        const auto r2 = nstd::from_chars(pasa.data(), pasa.data() + pasa.size(), w, 10);
        ok("un digito mas da result_out_of_range", r2.ec == std::errc::result_out_of_range);
        ok("y al desbordar NO toca el valor", w == w_intacto);
        (void)intacto;

        // ceros por delante: no deben contar para el desbordamiento
        const std::string ceros = std::string(300, '0') + "42";
        U2 z{};
        const auto r3 = nstd::from_chars(ceros.data(), ceros.data() + ceros.size(), z, 10);
        ok("300 ceros por delante no provocan desbordamiento",
           r3.ec == std::errc{} && z == U2{std::uint64_t{42}});
    }

    // --- 7. base invalida ----------------------------------------------------
    {
        char b[32] = {};
        U2 v{};
        ok("to_chars con base 1 da invalid_argument",
           nstd::to_chars(b, b + 32, U2{std::uint64_t{5}}, 1).ec == std::errc::invalid_argument);
        ok("to_chars con base 37 da invalid_argument",
           nstd::to_chars(b, b + 32, U2{std::uint64_t{5}}, 37).ec == std::errc::invalid_argument);
        const char s[] = "101";
        ok("from_chars con base 1 da invalid_argument",
           nstd::from_chars(s, s + 3, v, 1).ec == std::errc::invalid_argument);
        ok("from_chars con base 37 da invalid_argument",
           nstd::from_chars(s, s + 3, v, 37).ec == std::errc::invalid_argument);
    }

    // --- 8. el negativo, y que sin signo no se lo trague ----------------------
    {
        const char neg[] = "-42";
        I2 s{};
        const auto r1 = nstd::from_chars(neg, neg + 3, s, 10);
        ok("con signo lee el negativo",
           r1.ec == std::errc{} && r1.ptr == neg + 3 && s == I2::from_string("-42"));

        U2 u{};
        const auto r2 = nstd::from_chars(neg, neg + 3, u, 10);
        ok("sin signo NO acepta el '-'", r2.ec == std::errc::invalid_argument);

        // `std::from_chars` tampoco acepta '+', y esto sigue esa regla
        const char mas[] = "+42";
        I2 s2{};
        const auto r3 = nstd::from_chars(mas, mas + 3, s2, 10);
        ok("no acepta '+', como std::from_chars", r3.ec == std::errc::invalid_argument && r3.ptr == mas);
    }

    std::printf("\n====================================================================\n");
    std::printf("Results: %d passed, %d failed\n", g_pasan, g_fallan);
    std::printf("====================================================================\n");
    return g_fallan == 0 ? 0 : 1;
}
