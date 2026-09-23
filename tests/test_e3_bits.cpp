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
// @file       test_e3_bits.cpp
// @brief      E3: countl_one, countr_one, bit_floor, bit_ceil, byteswap
// @date       2026-09-23
// =============================================================================
//
// LOS ALEATORIOS NO SIRVEN AQUI, Y ESTA ESCRITO CON SANGRE
//
// `is_power_of_2` salio de un barrido de 400 valores al azar con CERO
// discrepancias estando rota para TODA potencia de dos: un numero de 128 bits al
// azar no es potencia de dos nunca. `bit_floor` y `bit_ceil` tienen exactamente
// la misma forma, asi que aqui las esquinas se CONSTRUYEN: las 128 potencias de
// dos, sus vecinas, los extremos y el cero.
//
// Y todo se cruza contra las CUATRO representaciones, porque ADR-018 dice que
// dan lo mismo y en el tramo 3 de P1.5 resulto que en seis sitios no.
//
// EL ORACULO ES UN PAR (hi, lo) DE uint64_t, NO `unsigned __int128`
//
// La primera version usaba `unsigned __int128` y **MSVC no lo tiene**: 3 errores
// de compilacion, 70/71, mientras los otros cuatro compiladores daban verde. Lo
// cazo la politica de los cinco, no la revision. Con el par a mano el oraculo
// sigue siendo independiente de la biblioteca --que es lo unico que se le pide--
// y ademas compila en todas partes.
// =============================================================================
#include "fixed_width_int_t.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
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

// --- el oraculo: 128 bits a mano, sin depender del compilador ---------------
struct o128
{
    std::uint64_t hi{0};
    std::uint64_t lo{0};
};

static bool o_bit(const o128 &v, int i) noexcept
{
    return i < 64 ? ((v.lo >> i) & 1U) != 0 : ((v.hi >> (i - 64)) & 1U) != 0;
}

static o128 o_pow2(int i) noexcept
{
    o128 r{};
    if (i < 64)
        r.lo = std::uint64_t{1} << i;
    else
        r.hi = std::uint64_t{1} << (i - 64);
    return r;
}

static o128 o_sub1(const o128 &v) noexcept
{
    o128 r = v;
    if (r.lo == 0)
    {
        --r.hi;
        r.lo = ~std::uint64_t{0};
    }
    else
        --r.lo;
    return r;
}

static o128 o_add1(const o128 &v) noexcept
{
    o128 r = v;
    ++r.lo;
    if (r.lo == 0)
        ++r.hi;
    return r;
}

static bool o_es_cero(const o128 &v) noexcept { return v.hi == 0 && v.lo == 0; }

static bool o_le(const o128 &a, const o128 &b) noexcept { return a.hi != b.hi ? a.hi < b.hi : a.lo <= b.lo; }

static U2 desde(const o128 &v) noexcept
{
    U2 r{};
    r.set_limb(0, v.lo);
    r.set_limb(1, v.hi);
    return r;
}

static unsigned oraculo_countl_one(const o128 &v) noexcept
{
    unsigned n = 0;
    for (int i = 127; i >= 0; --i)
    {
        if (!o_bit(v, i))
            break;
        ++n;
    }
    return n;
}

static unsigned oraculo_countr_one(const o128 &v) noexcept
{
    unsigned n = 0;
    for (int i = 0; i < 128; ++i)
    {
        if (!o_bit(v, i))
            break;
        ++n;
    }
    return n;
}

static o128 oraculo_bit_floor(const o128 &v) noexcept
{
    for (int i = 127; i >= 0; --i)
        if (o_bit(v, i))
            return o_pow2(i);
    return o128{};
}

// La menor potencia de dos que llega a v. Solo se usa donde no desborda.
static o128 oraculo_bit_ceil(const o128 &v) noexcept
{
    if (o_le(v, o128{0, 1}))
        return o128{0, 1};
    for (int i = 0; i < 128; ++i)
    {
        const o128 p = o_pow2(i);
        if (o_le(v, p))
            return p;
    }
    return o128{};
}

static o128 oraculo_byteswap(const o128 &v) noexcept
{
    o128 r{};
    for (int i = 0; i < 16; ++i)
    {
        const std::uint64_t fuente = (i < 8) ? v.lo : v.hi;
        const std::uint64_t b = (fuente >> (8 * (i % 8))) & 0xFFU;
        const int destino = 15 - i;
        if (destino < 8)
            r.lo |= b << (8 * destino);
        else
            r.hi |= b << (8 * (destino - 8));
    }
    return r;
}

// --- las esquinas, CONSTRUIDAS ----------------------------------------------
static std::vector<o128> esquinas()
{
    std::vector<o128> v;
    v.push_back(o128{0, 0});
    v.push_back(o128{0, 1});
    v.push_back(o128{0, 2});
    v.push_back(o128{0, 3});
    for (int i = 0; i < 128; ++i)
    {
        const o128 p = o_pow2(i);
        v.push_back(p); // la potencia exacta
        if (i > 0)
        {
            v.push_back(o_sub1(p)); // la de debajo: todos unos
            v.push_back(o_add1(p)); // la de encima
        }
    }
    v.push_back(o128{~std::uint64_t{0}, ~std::uint64_t{0}}); // todos unos
    v.push_back(o128{~std::uint64_t{0}, ~std::uint64_t{0} - 1});
    v.push_back(o_pow2(127)); // solo el bit alto
    // y unos cuantos con patrones mezclados, que no son potencias
    v.push_back(o128{0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL});
    v.push_back(o128{0xFF00FF00FF00FF00ULL, 0x00FF00FF00FF00FFULL});
    v.push_back(o128{0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL});
    return v;
}

// El UNICO valor de los 2^128 que Magnitud-Signo no puede guardar: -2^127. Su
// minimo es uno mas alto, porque el cero negativo ocupa el hueco (ADR-017).
//
// ADR-018 dice que las cuatro representaciones dan lo mismo, y es cierto **para
// todo valor que las cuatro puedan representar**. Este queda fuera por
// construccion. No es un fallo: es el precio de la codificacion.
static bool fuera_de_MS(const I2 &v) noexcept { return v == I2::min(); }

int main()
{
    std::printf("====================================================================\n");
    std::printf("E3: los nombres de <bit> que faltaban\n");
    std::printf("====================================================================\n\n");

    const std::vector<o128> casos = esquinas();
    std::printf("esquinas construidas: %zu\n\n", casos.size());

    const o128 dos_127 = o_pow2(127);

    // --- 1. countl_one / countr_one contra el oraculo ------------------------
    {
        int mal_cl = 0, mal_cr = 0;
        for (const o128 &v : casos)
        {
            const U2 x = desde(v);
            if (countl_one(x) != oraculo_countl_one(v))
                ++mal_cl;
            if (countr_one(x) != oraculo_countr_one(v))
                ++mal_cr;
        }
        ok("countl_one coincide con el oraculo en todas las esquinas", mal_cl == 0);
        ok("countr_one coincide con el oraculo en todas las esquinas", mal_cr == 0);
    }

    // --- 2. bit_floor / bit_ceil contra el oraculo ---------------------------
    {
        int mal_f = 0, mal_c = 0;
        for (const o128 &v : casos)
        {
            const U2 x = desde(v);
            if (bit_floor(x) != desde(oraculo_bit_floor(v)))
                ++mal_f;

            // bit_ceil solo se compara donde NO desborda: por encima de 2^127 el
            // resultado no cabe y manda la politica, que se prueba aparte.
            if (o_le(v, dos_127) && bit_ceil(x) != desde(oraculo_bit_ceil(v)))
                ++mal_c;
        }
        ok("bit_floor coincide con el oraculo en todas las esquinas", mal_f == 0);
        ok("bit_ceil coincide con el oraculo donde no desborda", mal_c == 0);
    }

    // --- 3. las identidades que definen bit_floor/bit_ceil -------------------
    {
        int mal = 0;
        for (const o128 &v : casos)
        {
            if (o_es_cero(v))
                continue;
            const U2 x = desde(v);
            const U2 f = bit_floor(x);
            if (!has_single_bit(f) || !(f <= x))
                ++mal;
        }
        ok("bit_floor(x) es potencia de dos y no pasa de x", mal == 0);
    }
    {
        int mal = 0;
        for (const o128 &v : casos)
        {
            if (o_es_cero(v) || !o_le(v, dos_127))
                continue;
            const U2 x = desde(v);
            const U2 c = bit_ceil(x);
            if (!has_single_bit(c) || !(x <= c))
                ++mal;
        }
        ok("bit_ceil(x) es potencia de dos y llega a x", mal == 0);
    }

    // --- 4. la potencia de dos es punto fijo de las dos ----------------------
    {
        int mal = 0;
        for (int i = 0; i < 128; ++i)
        {
            const U2 p = desde(o_pow2(i));
            if (bit_floor(p) != p || bit_ceil(p) != p)
                ++mal;
        }
        ok("las 128 potencias de dos son punto fijo de bit_floor y bit_ceil", mal == 0);
    }

    // --- 5. byteswap: involucion ---------------------------------------------
    {
        int mal_u = 0, mal_s = 0, mal_ms = 0, mal_ek = 0;
        for (const o128 &v : casos)
        {
            const U2 x = desde(v);
            if (byteswap(byteswap(x)) != x)
                ++mal_u;

            // Los sin signo llegan a 2^128-1 y con signo solo a 2^127-1: los que
            // no caben se saltan en vez de lanzar. La primera version de este
            // test se caia aqui, y el fallo era del TEST, no del codigo.
            const std::string s = x.to_string();
            const auto pi = I2::try_from_string(s.c_str());
            if (!pi.success())
                continue;
            const I2 xi = pi.value;
            if (byteswap(byteswap(xi)) != xi)
                ++mal_s;

            // Se salta tambien cuando el RESULTADO se sale: `byteswap(128)` da
            // exactamente -2^127, que MS no puede guardar, asi que la vuelta
            // parte de otro numero y la involucion no puede cumplirse. Es la
            // misma causa, vista por el otro lado.
            const MS xm = MS::from_string(s.c_str());
            if (xm.to_string() == xi.to_string() && !fuera_de_MS(byteswap(xi)) &&
                byteswap(byteswap(xm)) != xm)
                ++mal_ms;

            const EK xe = EK::from_string(s.c_str());
            if (byteswap(byteswap(xe)) != xe)
                ++mal_ek;
        }
        ok("byteswap es involucion sin signo", mal_u == 0);
        ok("byteswap es involucion con signo", mal_s == 0);
        ok("byteswap es involucion en Magnitud-Signo (ADR-018)", mal_ms == 0);
        ok("byteswap es involucion en Exceso-K (ADR-018)", mal_ek == 0);
    }

    // --- 6. byteswap contra el oraculo ---------------------------------------
    {
        int mal = 0;
        for (const o128 &v : casos)
            if (byteswap(desde(v)) != desde(oraculo_byteswap(v)))
                ++mal;
        ok("byteswap coincide con invertir los 16 bytes a mano", mal == 0);
    }

    // --- 7. LAS CUATRO REPRESENTACIONES DAN LO MISMO (ADR-018) ---------------
    //
    // Este es el cruce que en el tramo 3 de P1.5 saco seis huecos. Se hace sobre
    // valores CON SIGNO, positivos y negativos, porque es donde MS y EK guardan
    // algo distinto del valor.
    {
        int mal_cl = 0, mal_cr = 0, mal_f = 0, mal_c = 0;
        int saltados_ms = 0;
        for (const o128 &v : casos)
        {
            for (int signo = 0; signo < 2; ++signo)
            {
                std::string s = desde(v).to_string();
                if (signo && s != "0")
                    s.insert(s.begin(), '-');
                const auto pr = I2::try_from_string(s.c_str());
                if (!pr.success())
                    continue;
                const I2 xi = pr.value;
                const MS xm = MS::from_string(s.c_str());
                const EK xe = EK::from_string(s.c_str());

                // ADR-018 vale PARA EL MISMO VALOR. Si MS guarda otro numero
                // --porque el pedido no cabe-- discrepar es lo correcto. Se
                // salta por VALOR, no por el caso concreto: si apareciera otra
                // saturacion, esto la saltaria tambien y el test seguiria
                // diciendo la verdad.
                if (xm.to_string() != xi.to_string())
                {
                    ++saltados_ms;
                    continue;
                }

                if (countl_one(xi) != countl_one(xm) || countl_one(xi) != countl_one(xe))
                    ++mal_cl;
                if (countr_one(xi) != countr_one(xm) || countr_one(xi) != countr_one(xe))
                    ++mal_cr;
                if (bit_floor(xi).to_string() != bit_floor(xm).to_string() ||
                    bit_floor(xi).to_string() != bit_floor(xe).to_string())
                    ++mal_f;
                // `bit_ceil` puede DESBORDAR, y al desbordar cae justo en
                // -2^127: C2 y EK envuelven hasta ese valor, MS satura a uno mas
                // alto porque no lo tiene.
                if (!fuera_de_MS(bit_ceil(xi)) && (bit_ceil(xi).to_string() != bit_ceil(xm).to_string() ||
                                                   bit_ceil(xi).to_string() != bit_ceil(xe).to_string()))
                    ++mal_c;
            }
        }
        ok("countl_one: las cuatro representaciones coinciden (ADR-018)", mal_cl == 0);
        ok("countr_one: las cuatro representaciones coinciden (ADR-018)", mal_cr == 0);
        ok("bit_floor: las cuatro representaciones coinciden (ADR-018)", mal_f == 0);
        ok("bit_ceil: las cuatro representaciones coinciden (ADR-018)", mal_c == 0);
        std::printf("       (%d casos saltados: MS no puede representarlos)\n", saltados_ms);
        // Que se salte ALGO es parte del contrato: si no se saltara ninguno,
        // seria que MS representa -2^127, y entonces ADR-017 estaria mal.
        ok("el cruce salta exactamente los que MS no puede representar", saltados_ms > 0);
    }

    // --- 7b. Y QUE MS SATURE BIEN JUSTO AHI ---------------------------------
    //
    // Es el complemento del salto de arriba: lo que no se puede exigir igual, se
    // exige correcto. Este caso es UNO entre 2^128 y ningun valor al azar lo
    // encuentra; `from_string` saturaba al lado contrario --daba `+max` en vez de
    // `min`, PERDIENDO EL SIGNO-- y lo destapo el cruce de las cuatro.
    {
        const char *MIN_C2 = "-170141183460469231731687303715884105728";
        const MS m = MS::from_string(MIN_C2);
        ok("MS satura -2^127 a su propio min(), no a max()", m == MS::min());
        ok("MS satura -2^127 conservando el SIGNO", m.to_string()[0] == '-');
        ok("EK si representa -2^127 exacto", EK::from_string(MIN_C2).to_string() == MIN_C2);
        ok("I2 si representa -2^127 exacto", I2::from_string(MIN_C2).to_string() == MIN_C2);
        ok("from_string y desde_c2 coinciden en el limite",
           MS::from_string(MIN_C2) == MS::desde_c2(I2::min()));

        // Lo que SI se puede exigir donde el resultado se sale: que MS caiga en
        // su propio minimo, no en cualquier sitio. Sin esto, saltar seria tapar.
        const I2 ciento_veintiocho = I2::from_string("128");
        const MS ms_128 = MS::from_string("128");
        ok("byteswap(128) en I2 es exactamente -2^127", byteswap(ciento_veintiocho).to_string() == MIN_C2);
        ok("byteswap(128) en MS cae en MS::min(), no en otro sitio", byteswap(ms_128) == MS::min());

        const I2 casi = I2::from_string("85070591730234615865843651857942052865"); // 2^126+1
        const MS casi_ms = MS::from_string("85070591730234615865843651857942052865");
        ok("bit_ceil(2^126+1) desborda a -2^127 en complemento a dos", bit_ceil(casi).to_string() == MIN_C2);
        ok("bit_ceil(2^126+1) satura a MS::min() en Magnitud-Signo", bit_ceil(casi_ms) == MS::min());
    }

    // --- 8. los negativos, que es la extension propia -------------------------
    {
        const I2 neg = I2::from_string("-12345678901234567890");
        ok("bit_floor de un negativo es cero", bit_floor(neg).is_zero());
        ok("bit_ceil de un negativo es uno", bit_ceil(neg) == I2::one());
        ok("bit_floor(0) es cero", bit_floor(U2::zero()).is_zero());
        ok("bit_ceil(0) es uno", bit_ceil(U2::zero()) == U2::one());
        // -1 tiene los 128 bits a uno en complemento a dos
        const I2 menos_uno = I2::from_string("-1");
        ok("countl_one(-1) es 128", countl_one(menos_uno) == 128U);
        ok("countr_one(-1) es 128", countr_one(menos_uno) == 128U);
        ok("countl_one(0) es 0", countl_one(U2::zero()) == 0U);
        ok("countr_one(0) es 0", countr_one(U2::zero()) == 0U);
        ok("countl_one(max sin signo) es 128", countl_one(U2::max()) == 128U);
    }

    std::printf("\n====================================================================\n");
    std::printf("Results: %d passed, %d failed\n", g_pasan, g_fallan);
    std::printf("====================================================================\n");
    return g_fallan == 0 ? 0 : 1;
}
