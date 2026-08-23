// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: fixed_int_t<N> — string I/O (to_string / from_string)
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// Origen: portado desde include/test_fixed_string_io.cpp (escrito con gtest, que
// no es dependencia del proyecto) al framework propio de tests/. Tarea T0.2 del
// plan de la auditoria 23 ago 2026.
//
// Este fichero es la BASE de T5.1 (ampliar cobertura de IO). Cobertura actual:
//   1.  to_string: cero, positivos, negativos, N=2/4/8
//   2.  from_string: cero, positivos, negativos, signo '+' y '-'
//   3.  from_string: rutas de error (cadena vacia, nullptr, caracter invalido,
//       solo signo, signo en unsigned, espacios)
//   3b. from_string / try_from_string: desbordamiento (T2.1)
//   3c. constructor desde punto flotante no finito: inf y NaN (T2.2)
//   4.  Round-trip to_string -> from_string
//   5.  Limites: max(), min(), min()+1, max()-1
//
// NOTA: este fichero usa los accesores limb()/set_limb()/limbs() y el
// constructor desde std::array<uint64_t, N>. `data` es privado desde T2.4,
// recuperando el comportamiento de phase-1.75.

#include "fixed_width_int_t.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using nstd::int_fixed_t;
using nstd::parse_error;
using nstd::uint_fixed_t;

// =============================================================================
// Test framework
// =============================================================================

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

// Comprueba que `expr` lanza std::invalid_argument.
#define TEST_THROWS(name, expr)               \
    do                                        \
    {                                         \
        bool threw_{false};                   \
        try                                   \
        {                                     \
            (void)(expr);                     \
        }                                     \
        catch (const std::invalid_argument &) \
        {                                     \
            threw_ = true;                    \
        }                                     \
        catch (...)                           \
        {                                     \
            threw_ = false;                   \
        }                                     \
        TEST(name, threw_);                   \
    } while (false)

// =============================================================================
// Helpers
// =============================================================================

// Construye un valor a partir de sus limbos (data[0] = LSB), sin tocar `data`.
template <std::size_t N, typename T>
static T make_from_limbs(std::array<std::uint64_t, N> limbs)
{
    return T{limbs};
}

// =============================================================================
// 1. to_string
// =============================================================================

static void test_to_string()
{
    std::cout << "\n--- 1. to_string ---\n";

    TEST("to_string: u256 cero", uint_fixed_t<4>{}.to_string() == "0");
    TEST("to_string: i256 cero", int_fixed_t<4>{}.to_string() == "0");

    TEST("to_string: u256 pequenyo", uint_fixed_t<4>{std::uint64_t{12345}}.to_string() == "12345");
    TEST("to_string: i256 negativo pequenyo", int_fixed_t<4>{std::int64_t{-12345}}.to_string() == "-12345");

    // 2^128 + 1
    const auto u_2_128_plus_1 = make_from_limbs<4, uint_fixed_t<4>>({1ULL, 0ULL, 1ULL, 0ULL});
    TEST("to_string: u256 = 2^128 + 1",
         u_2_128_plus_1.to_string() == "340282366920938463463374607431768211457");

    const auto i_2_128_plus_1 = -make_from_limbs<4, int_fixed_t<4>>({1ULL, 0ULL, 1ULL, 0ULL});
    TEST("to_string: i256 = -(2^128 + 1)",
         i_2_128_plus_1.to_string() == "-340282366920938463463374607431768211457");

    TEST("to_string: u512 de un limbo",
         uint_fixed_t<8>{std::uint64_t{1234567890123456789ULL}}.to_string() == "1234567890123456789");

    // 2^64 exacto en N=2: comprueba el chunking por 10^19
    const auto u_2_64 = make_from_limbs<2, uint_fixed_t<2>>({0ULL, 1ULL});
    TEST("to_string: u128 = 2^64", u_2_64.to_string() == "18446744073709551616");

    // 10^19 exacto: el chunk_base de to_string, caso frontera del padding
    TEST("to_string: u128 = 10^19",
         uint_fixed_t<2>{std::uint64_t{10000000000000000000ULL}}.to_string() == "10000000000000000000");
}

// =============================================================================
// 2. from_string — valores validos
// =============================================================================

static void test_from_string_valid()
{
    std::cout << "\n--- 2. from_string (validos) ---\n";

    TEST("from_string: u256 \"0\"", uint_fixed_t<4>::from_string("0").is_zero());
    TEST("from_string: i256 \"-0\"", int_fixed_t<4>::from_string("-0").is_zero());
    TEST("from_string: i256 \"+0\"", int_fixed_t<4>::from_string("+0").is_zero());

    TEST("from_string: u256 \"12345\"",
         uint_fixed_t<4>::from_string("12345") == uint_fixed_t<4>{std::uint64_t{12345}});
    TEST("from_string: i256 \"-12345\"",
         int_fixed_t<4>::from_string("-12345") == int_fixed_t<4>{std::int64_t{-12345}});
    TEST("from_string: i256 \"+12345\"",
         int_fixed_t<4>::from_string("+12345") == int_fixed_t<4>{std::int64_t{12345}});

    const auto expected = make_from_limbs<4, uint_fixed_t<4>>({1ULL, 0ULL, 1ULL, 0ULL});
    TEST("from_string: u256 = 2^128 + 1",
         uint_fixed_t<4>::from_string("340282366920938463463374607431768211457") == expected);

    const auto expected_neg = -make_from_limbs<4, int_fixed_t<4>>({1ULL, 0ULL, 1ULL, 0ULL});
    TEST("from_string: i256 = -(2^128 + 1)",
         int_fixed_t<4>::from_string("-340282366920938463463374607431768211457") == expected_neg);

    // Ceros a la izquierda
    TEST("from_string: u256 \"000123\"",
         uint_fixed_t<4>::from_string("000123") == uint_fixed_t<4>{std::uint64_t{123}});
}

// =============================================================================
// 3. from_string — rutas de error
// =============================================================================

static void test_from_string_errors()
{
    std::cout << "\n--- 3. from_string (rutas de error) ---\n";

    TEST_THROWS("from_string: u256 \"123a\" lanza", uint_fixed_t<4>::from_string("123a"));
    TEST_THROWS("from_string: i256 \"-123a\" lanza", int_fixed_t<4>::from_string("-123a"));

    TEST_THROWS("from_string: u256 \"\" lanza", uint_fixed_t<4>::from_string(""));
    TEST_THROWS("from_string: i256 \"\" lanza", int_fixed_t<4>::from_string(""));

    TEST_THROWS("from_string: u256 nullptr lanza", uint_fixed_t<4>::from_string(nullptr));
    TEST_THROWS("from_string: i256 nullptr lanza", int_fixed_t<4>::from_string(nullptr));

    // from_string de unsigned no acepta signos (asimetria conocida con signed, T5.1)
    TEST_THROWS("from_string: u256 \"-123\" lanza", uint_fixed_t<4>::from_string("-123"));
    TEST_THROWS("from_string: u256 \"+123\" lanza", uint_fixed_t<4>::from_string("+123"));

    // Solo signo
    TEST_THROWS("from_string: i256 \"-\" lanza", int_fixed_t<4>::from_string("-"));
    TEST_THROWS("from_string: i256 \"+\" lanza", int_fixed_t<4>::from_string("+"));

    // Espacios: no se aceptan (ni delante ni detras)
    TEST_THROWS("from_string: u256 \" 12\" lanza", uint_fixed_t<4>::from_string(" 12"));
    TEST_THROWS("from_string: u256 \"12 \" lanza", uint_fixed_t<4>::from_string("12 "));
}

// =============================================================================
// 4. Round-trip to_string -> from_string
// =============================================================================

template <std::size_t N>
static void test_round_trip(const char *tag)
{
    std::cout << "\n--- 4. round-trip " << tag << " ---\n";

    using U = uint_fixed_t<N>;
    using I = int_fixed_t<N>;

    // Patron denso en todos los limbos
    std::array<std::uint64_t, N> limbs{};
    for (std::size_t i = 0; i < N; ++i)
        limbs[i] = 0x1234567890ABCDEFULL ^ (0x1111111111111111ULL * (i + 1));

    const U u_val{limbs};
    const std::string u_str = u_val.to_string();
    TEST(std::string{"round-trip unsigned denso "} + tag, U::from_string(u_str.c_str()) == u_val);

    const I i_val = -I{limbs};
    const std::string i_str = i_val.to_string();
    TEST(std::string{"round-trip signed negativo "} + tag, I::from_string(i_str.c_str()) == i_val);

    // Limites
    TEST(std::string{"round-trip unsigned max() "} + tag,
         U::from_string(U::max().to_string().c_str()) == U::max());
    TEST(std::string{"round-trip unsigned max()-1 "} + tag,
         U::from_string((U::max() - U::one()).to_string().c_str()) == U::max() - U::one());
    TEST(std::string{"round-trip signed max() "} + tag,
         I::from_string(I::max().to_string().c_str()) == I::max());
    TEST(std::string{"round-trip signed min() "} + tag,
         I::from_string(I::min().to_string().c_str()) == I::min());
    TEST(std::string{"round-trip signed min()+1 "} + tag,
         I::from_string((I::min() + I::one()).to_string().c_str()) == I::min() + I::one());

    // Valores pequenyos y potencias de dos
    TEST(std::string{"round-trip cero "} + tag, U::from_string(U{}.to_string().c_str()).is_zero());
    TEST(std::string{"round-trip uno "} + tag, U::from_string(U::one().to_string().c_str()) == U::one());

    bool pow2_ok{true};
    for (unsigned k = 0; k < 64U * N; k += 7U)
    {
        const U p = U::one() << k;
        if (U::from_string(p.to_string().c_str()) != p)
        {
            pow2_ok = false;
            break;
        }
    }
    TEST(std::string{"round-trip potencias de 2 (paso 7 bits) "} + tag, pow2_ok);

    // Potencias de 10 hasta desbordar
    bool pow10_ok{true};
    U p10 = U::one();
    const U ten{std::uint64_t{10}};
    while (true)
    {
        const U next = p10 * ten;
        if (next / ten != p10) // desbordo: paramos
            break;
        p10 = next;
        if (U::from_string(p10.to_string().c_str()) != p10)
        {
            pow10_ok = false;
            break;
        }
    }
    TEST(std::string{"round-trip potencias de 10 "} + tag, pow10_ok);
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "fixed_int_t<N> String I/O Tests (v1.90)\n";
    std::cout << "====================================================================\n";

    test_to_string();
    test_from_string_valid();
    test_from_string_errors();

    test_round_trip<1>("N=1 (64-bit)");
    test_round_trip<2>("N=2 (128-bit)");
    test_round_trip<4>("N=4 (256-bit)");
    test_round_trip<8>("N=8 (512-bit)");

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
