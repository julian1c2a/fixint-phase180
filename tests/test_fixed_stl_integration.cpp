// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: fixed_int_t<N> — integracion con la libreria estandar
// Part of int128 Library - Phase 1.90
// SPDX-License-Identifier: BSL-1.0
// =============================================================================
//
// Fase 4 del plan de auditoria (23 ago 2026). `int128_param_t` ya tenia
// iostreams, std::format y std::hash; fixed_int_t no tenia ninguno de los tres,
// asi que no se podia ni imprimir. Como el objetivo de la rama es que
// fixed_int_t ocupe el sitio de los tipos de 256 bits anteriores, esto es
// paridad, no adorno.
//
// Secciones:
//   1. operator<< : base, showbase, uppercase, showpos, width/fill/alineacion
//   2. operator>> : lectura con y sin base, prefijos, errores (failbit)
//   3. std::format : la especificacion completa [[fill]align][sign][#][0][width][type]
//   4. std::hash  : uso real en unordered_set/unordered_map, dispersion
//   5. to_string(base) / from_string(base) y sus round-trips (T4.4)

#include "fixed_int_format.hpp"
#include "fixed_int_hash.hpp"
#include "fixed_int_iostreams.hpp"
#include "fixed_width_int_t.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#if __has_include(<format>)
#include <format>
#define HAS_FORMAT 1
#else
#define HAS_FORMAT 0
#endif

using nstd::int_fixed_t;
using nstd::uint_fixed_t;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                              \
    do                                                \
    {                                                 \
        if (cond)                                     \
        {                                             \
            std::cout << "[OK]   " << (name) << "\n"; \
            ++g_passed;                               \
        }                                             \
        else                                          \
        {                                             \
            std::cout << "[FAIL] " << (name) << "\n"; \
            ++g_failed;                               \
        }                                             \
    } while (false)

using u2 = uint_fixed_t<2>;
using u4 = uint_fixed_t<4>;
using i4 = int_fixed_t<4>;

// Serializa `v` a traves de un ostringstream aplicando `setup` al flujo.
template <typename T, typename Setup>
static std::string streamed(const T &v, Setup setup)
{
    std::ostringstream os;
    setup(os);
    os << v;
    return os.str();
}

// =============================================================================
// 1. operator<<
// =============================================================================

static void test_ostream()
{
    std::cout << "\n--- 1. operator<< ---\n";

    TEST("<< decimal", streamed(u4{255}, [](std::ostream &) {}) == "255");
    TEST("<< cero", streamed(u4{}, [](std::ostream &) {}) == "0");
    TEST("<< negativo", streamed(i4{-255}, [](std::ostream &) {}) == "-255");

    TEST("<< hex", streamed(u4{255}, [](std::ostream &o) { o << std::hex; }) == "ff");
    TEST("<< hex uppercase",
         streamed(u4{255}, [](std::ostream &o) { o << std::hex << std::uppercase; }) == "FF");
    TEST("<< oct", streamed(u4{255}, [](std::ostream &o) { o << std::oct; }) == "377");

    TEST("<< hex showbase",
         streamed(u4{255}, [](std::ostream &o) { o << std::hex << std::showbase; }) == "0xff");
    TEST("<< hex showbase uppercase",
         streamed(u4{255}, [](std::ostream &o) { o << std::hex << std::showbase << std::uppercase; }) ==
             "0XFF");
    TEST("<< oct showbase",
         streamed(u4{255}, [](std::ostream &o) { o << std::oct << std::showbase; }) == "0377");
    TEST("<< oct showbase con cero",
         streamed(u4{}, [](std::ostream &o) { o << std::oct << std::showbase; }) == "0");

    TEST("<< showpos", streamed(u4{42}, [](std::ostream &o) { o << std::showpos; }) == "+42");
    TEST("<< showpos con cero", streamed(u4{}, [](std::ostream &o) { o << std::showpos; }) == "0");
    TEST("<< showpos con negativo", streamed(i4{-42}, [](std::ostream &o) { o << std::showpos; }) == "-42");

    TEST("<< width derecha (por defecto)",
         streamed(u4{42}, [](std::ostream &o) { o << std::setw(6); }) == "    42");
    TEST("<< width izquierda",
         streamed(u4{42}, [](std::ostream &o) { o << std::left << std::setw(6); }) == "42    ");
    TEST("<< width con fill",
         streamed(u4{42}, [](std::ostream &o) { o << std::setfill('*') << std::setw(6); }) == "****42");
    TEST("<< internal deja el signo delante",
         streamed(i4{-42}, [](std::ostream &o)
                  { o << std::internal << std::setfill('0') << std::setw(6); }) == "-00042");
    TEST(
        "<< internal con prefijo hex",
        streamed(u4{255}, [](std::ostream &o)
                 { o << std::hex << std::showbase << std::internal << std::setfill('0') << std::setw(8); }) ==
            "0x0000ff");

    // width se consume tras un solo uso, como en la libreria estandar.
    {
        std::ostringstream os;
        os << std::setw(6) << u4{42} << u4{7};
        TEST("<< width se consume", os.str() == "    427");
    }

    // Con hex/oct, un valor con signo se imprime como patron de bits, igual que
    // `std::cout << std::hex << -1`.
    TEST("<< hex de negativo = patron de bits",
         streamed(i4{-1}, [](std::ostream &o) { o << std::hex; }) ==
             "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

    TEST("<< valor grande", streamed(u4::max(), [](std::ostream &) {}) == u4::max().to_string());
}

// =============================================================================
// 2. operator>>
// =============================================================================

static void test_istream()
{
    std::cout << "\n--- 2. operator>> ---\n";

    {
        std::istringstream is{"12345"};
        u4 v{};
        is >> v;
        TEST(">> decimal", !is.fail() && v == u4{12345});
    }
    {
        std::istringstream is{"-12345"};
        i4 v{};
        is >> v;
        TEST(">> decimal negativo", !is.fail() && v == i4{-12345});
    }
    {
        std::istringstream is{"ff"};
        u4 v{};
        is >> std::hex >> v;
        TEST(">> hex", !is.fail() && v == u4{255});
    }
    {
        std::istringstream is{"0xFF"};
        u4 v{};
        is >> std::hex >> v;
        TEST(">> hex con prefijo", !is.fail() && v == u4{255});
    }
    {
        std::istringstream is{"377"};
        u4 v{};
        is >> std::oct >> v;
        TEST(">> oct", !is.fail() && v == u4{255});
    }
    {
        // Sin basefield, la base se deduce del prefijo (como los built-in).
        std::istringstream is{"0x10"};
        u4 v{};
        is.unsetf(std::ios_base::basefield);
        is >> v;
        TEST(">> base deducida del prefijo", !is.fail() && v == u4{16});
    }
    {
        std::istringstream is{"   42"};
        u4 v{};
        is >> v;
        TEST(">> salta espacios (skipws)", !is.fail() && v == u4{42});
    }
    {
        std::istringstream is{"abc"};
        u4 v{u4{7}};
        is >> v;
        TEST(">> entrada invalida activa failbit", is.fail());
    }
    {
        std::istringstream is{""};
        u4 v{};
        is >> v;
        TEST(">> cadena vacia activa failbit", is.fail());
    }
    {
        // Desbordamiento: 2^256 no cabe en u4.
        std::istringstream is{
            "115792089237316195423570985008687907853269984665640564039457584007913129639936"};
        u4 v{};
        is >> v;
        TEST(">> desbordamiento activa failbit", is.fail());
    }
    {
        // Round-trip por el flujo.
        const u4 original = u4::max() / u4{3};
        std::stringstream ss;
        ss << original;
        u4 back{};
        ss >> back;
        TEST(">> round-trip por el flujo", !ss.fail() && back == original);
    }
    {
        std::istringstream is{"42 7"};
        u4 a{}, b{};
        is >> a >> b;
        TEST(">> dos valores seguidos", !is.fail() && a == u4{42} && b == u4{7});
    }
}

// =============================================================================
// 3. std::format
// =============================================================================

static void test_format()
{
#if HAS_FORMAT
    std::cout << "\n--- 3. std::format ---\n";

    TEST("format por defecto", std::format("{}", u4{255}) == "255");
    TEST("format negativo", std::format("{}", i4{-255}) == "-255");

    TEST("format hex", std::format("{:x}", u4{255}) == "ff");
    TEST("format HEX", std::format("{:X}", u4{255}) == "FF");
    TEST("format binario", std::format("{:b}", u4{5}) == "101");
    TEST("format octal", std::format("{:o}", u4{255}) == "377");

    TEST("format hex con #", std::format("{:#x}", u4{255}) == "0xff");
    TEST("format HEX con #", std::format("{:#X}", u4{255}) == "0XFF");
    TEST("format binario con #", std::format("{:#b}", u4{5}) == "0b101");
    TEST("format octal con #", std::format("{:#o}", u4{8}) == "010");

    TEST("format ancho derecha", std::format("{:>8}", u4{42}) == "      42");
    TEST("format ancho izquierda", std::format("{:<8}", u4{42}) == "42      ");
    TEST("format centrado", std::format("{:^8}", u4{42}) == "   42   ");
    TEST("format relleno con caracter", std::format("{:*>8}", u4{42}) == "******42");
    TEST("format ancho por defecto = derecha", std::format("{:8}", u4{42}) == "      42");

    TEST("format relleno con ceros", std::format("{:08}", u4{42}) == "00000042");
    TEST("format ceros con negativo", std::format("{:08}", i4{-42}) == "-0000042");
    TEST("format ceros con # y hex", std::format("{:#010x}", u4{255}) == "0x000000ff");

    TEST("format signo +", std::format("{:+}", u4{42}) == "+42");
    TEST("format signo espacio", std::format("{: }", u4{42}) == " 42");
    TEST("format signo + con negativo", std::format("{:+}", i4{-42}) == "-42");

    TEST("format en cadena mayor", std::format("valor={:>6}!", u4{42}) == "valor=    42!");
    TEST("format de max()", std::format("{}", u4::max()) == u4::max().to_string());
    TEST("format hex de max()", std::format("{:x}", u4::max()) == std::string(64, 'f'));

    // Con signo y base != 10, std::format imprime signo + magnitud, como los
    // enteros built-in con signo (a diferencia de iostreams).
    TEST("format hex de negativo = signo + magnitud", std::format("{:x}", i4{-255}) == "-ff");

    TEST("format N=2", std::format("{:#x}", u2{4095}) == "0xfff");
#else
    std::cout << "\n--- 3. std::format (no disponible en este compilador) ---\n";
#endif
}

// =============================================================================
// 4. std::hash
// =============================================================================

static void test_hash()
{
    std::cout << "\n--- 4. std::hash ---\n";

    const std::hash<u4> h{};

    TEST("hash es determinista", h(u4{42}) == h(u4{42}));
    TEST("hash distingue valores", h(u4{42}) != h(u4{43}));

    // Un limbo alto distinto tiene que cambiar el hash: es el fallo tipico de
    // una implementacion que solo mira data[0].
    {
        u4 a{}, b{};
        a.set_limb(3, 1);
        b.set_limb(3, 2);
        TEST("hash mira todos los limbos", h(a) != h(b));
    }

    // Uso real en contenedores.
    {
        std::unordered_set<u4> s;
        s.insert(u4{1});
        s.insert(u4{2});
        s.insert(u4{1});
        TEST("unordered_set deduplica", s.size() == 2);
        TEST("unordered_set encuentra", s.find(u4{2}) != s.end());
        TEST("unordered_set no encuentra ausentes", s.find(u4{3}) == s.end());
    }
    {
        std::unordered_map<u4, const char *> m;
        m[u4::max()] = "max";
        m[u4{}] = "cero";
        TEST("unordered_map con clave max()", std::string{m[u4::max()]} == "max");
        TEST("unordered_map con clave cero", std::string{m[u4{}]} == "cero");
    }
    {
        std::unordered_set<i4> s;
        s.insert(i4{-1});
        s.insert(i4{1});
        TEST("unordered_set con tipo con signo", s.size() == 2);
    }

    // Dispersion: 4096 valores consecutivos y desplazados no deben colisionar
    // apenas. Con una mezcla decente, cero colisiones es lo esperable.
    {
        std::unordered_set<std::size_t> hashes;
        for (unsigned k = 0; k < 4096; ++k)
        {
            u4 v{static_cast<std::uint64_t>(k)};
            v.set_limb(2, k * 7U);
            hashes.insert(h(v));
        }
        TEST("dispersion: < 1% de colisiones en 4096 valores", hashes.size() >= 4055);
    }
}

// =============================================================================
// 5. to_string(base) / from_string(base)  (T4.4)
// =============================================================================

static void test_bases()
{
    std::cout << "\n--- 5. to_string(base) / from_string(base) ---\n";

    TEST("to_string base 16", u4{255}.to_string(16) == "FF");
    TEST("to_string base 2", u4{255}.to_string(2) == "11111111");
    TEST("to_string base 8", u4{255}.to_string(8) == "377");
    TEST("to_string base 36", u4{255}.to_string(36) == "73");
    TEST("to_string base 3", u4{255}.to_string(3) == "100110");
    TEST("to_string base 16 de cero", u4{}.to_string(16) == "0");
    TEST("to_string base 16 de negativo", i4{-255}.to_string(16) == "-FF");
    TEST("to_string base 16 de max()", u4::max().to_string(16) == std::string(64, 'F'));

    {
        bool threw{false};
        try
        {
            (void)u4{1}.to_string(37);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }
        TEST("to_string base 37 lanza", threw);
    }

    TEST("from_string base 16", u4::from_string("FF", 16) == u4{255});
    TEST("from_string base 16 minusculas", u4::from_string("ff", 16) == u4{255});
    TEST("from_string prefijo 0x con base 16", u4::from_string("0xFF", 16) == u4{255});
    TEST("from_string prefijo 0x con base 0", u4::from_string("0xFF", 0) == u4{255});
    TEST("from_string prefijo 0b con base 0", u4::from_string("0b1010", 0) == u4{10});
    TEST("from_string prefijo 0o con base 0", u4::from_string("0o777", 0) == u4{511});
    TEST("from_string base 0 sin prefijo es decimal", u4::from_string("123", 0) == u4{123});
    TEST("from_string base 36", u4::from_string("ZZ", 36) == u4{35 * 36 + 35});
    TEST("from_string negativo en base 16", i4::from_string("-FF", 16) == i4{-255});
    TEST("from_string negativo con prefijo", i4::from_string("-0xFF", 0) == i4{-255});

    // Un '0' suelto NO se lee como octal (a diferencia de strtoul).
    TEST("'077' en base 0 es 77 decimal", u4::from_string("077", 0) == u4{77});

    // Errores especificos de base.
    {
        const auto r = u4::try_from_string("12", 2);
        TEST("digito fuera de base -> digit_out_of_range",
             r.error == nstd::parse_error::digit_out_of_range && r.error_index == 1);
    }
    {
        const auto r = u4::try_from_string("1", 37);
        TEST("base 37 -> invalid_base", r.error == nstd::parse_error::invalid_base);
    }
    {
        const auto r = u4::try_from_string("0x", 16);
        TEST("solo prefijo -> no_digits", r.error == nstd::parse_error::no_digits);
    }
    {
        const std::string over = "1" + std::string(64, '0'); // 2^256 en hex
        const auto r = u4::try_from_string(over.c_str(), 16);
        TEST("desbordamiento en base 16", r.error == nstd::parse_error::overflow);
    }

    // Round-trips en todas las bases, con valores frontera.
    {
        bool all_ok{true};
        const u4 values[] = {u4{}, u4::one(), u4{255}, u4::max(), u4::max() / u4{3}, u4::one() << 200};
        for (int base = 2; base <= 36 && all_ok; ++base)
        {
            for (const u4 &v : values)
            {
                const std::string s = v.to_string(base);
                if (u4::from_string(s.c_str(), base) != v)
                {
                    all_ok = false;
                    std::cout << "       falla base " << base << " con " << v.to_string() << "\n";
                    break;
                }
            }
        }
        TEST("round-trip en todas las bases 2..36", all_ok);
    }
    {
        bool all_ok{true};
        const i4 values[] = {i4{}, i4{-1}, i4{-255}, i4::min(), i4::max()};
        for (int base = 2; base <= 36 && all_ok; ++base)
        {
            for (const i4 &v : values)
            {
                const std::string s = v.to_string(base);
                if (i4::from_string(s.c_str(), base) != v)
                {
                    all_ok = false;
                    std::cout << "       falla base " << base << " con " << v.to_string() << "\n";
                    break;
                }
            }
        }
        TEST("round-trip con signo en todas las bases 2..36", all_ok);
    }
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "fixed_int_t<N> — integracion con la libreria estandar (v1.90)\n";
    std::cout << "====================================================================\n";

    test_ostream();
    test_istream();
    test_format();
    test_hash();
    test_bases();

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
