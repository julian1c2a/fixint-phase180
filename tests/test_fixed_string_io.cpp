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
// 3b. from_string / try_from_string — deteccion de desbordamiento (T2.1)
//
// Antes de T2.1 esto NO se detectaba: from_string("2^256") sobre un
// uint_fixed_t<4> devolvia 0 en silencio, con el valor truncado modulo 2^256.
// =============================================================================

// Comprueba que `expr` lanza std::out_of_range.
#define TEST_THROWS_RANGE(name, expr)     \
    do                                    \
    {                                     \
        bool threw_{false};               \
        try                               \
        {                                 \
            (void)(expr);                 \
        }                                 \
        catch (const std::out_of_range &) \
        {                                 \
            threw_ = true;                \
        }                                 \
        catch (...)                       \
        {                                 \
            threw_ = false;               \
        }                                 \
        TEST(name, threw_);               \
    } while (false)

// Devuelve la representacion decimal de `v` con `delta` sumado, usando el propio
// to_string sobre un tipo mas ancho para no depender de aritmetica externa.
template <std::size_t N>
static std::string decimal_of(const uint_fixed_t<N> &v)
{
    return v.to_string();
}

static void test_from_string_overflow()
{
    std::cout << "\n--- 3b. from_string: desbordamiento (T2.1) ---\n";

    using U = uint_fixed_t<4>;
    using I = int_fixed_t<4>;

    // Cotas exactas calculadas con el propio to_string, para no hardcodear.
    const std::string u_max_str = decimal_of(U::max()); // 2^256 - 1
    const std::string u_over_str = decimal_of(uint_fixed_t<8>{U::max()} + uint_fixed_t<8>::one());

    // El maximo justo cabe.
    const auto ok = U::try_from_string(u_max_str.c_str());
    TEST("T2.1 u256 max() cabe", ok.success() && ok.value == U::max());

    // max()+1 = 2^256 desborda (antes devolvia 0 en silencio).
    const auto over = U::try_from_string(u_over_str.c_str());
    TEST("T2.1 u256 2^256 -> parse_error::overflow", over.error == parse_error::overflow);
    TEST("T2.1 u256 2^256 -> valor no corrupto", over.value.is_zero());
    TEST_THROWS_RANGE("T2.1 u256 2^256 -> from_string lanza out_of_range",
                      U::from_string(u_over_str.c_str()));

    // Un numero absurdamente largo tambien.
    TEST_THROWS_RANGE("T2.1 u256 200 digitos -> out_of_range", U::from_string(std::string(200, '9').c_str()));

    // Con signo: los limites son asimetricos, [-2^255, 2^255-1].
    const std::string i_max_str = decimal_of(uint_fixed_t<4>{I::max()}); // 2^255 - 1
    const std::string i_min_mag = decimal_of(uint_fixed_t<4>{I::min()}); // 2^255 (magnitud)

    const auto imax = I::try_from_string(i_max_str.c_str());
    TEST("T2.1 i256 max() cabe", imax.success() && imax.value == I::max());

    const auto imin = I::try_from_string(("-" + i_min_mag).c_str());
    TEST("T2.1 i256 min() cabe", imin.success() && imin.value == I::min());

    // +2^255 NO cabe (max() es 2^255 - 1), aunque su magnitud sí quepa en unsigned.
    const auto ipos_over = I::try_from_string(i_min_mag.c_str());
    TEST("T2.1 i256 +2^255 -> overflow", ipos_over.error == parse_error::overflow);

    // -(2^255 + 1) tampoco.
    const std::string i_below_min = decimal_of(uint_fixed_t<4>{I::min()} + U::one());
    const auto ineg_over = I::try_from_string(("-" + i_below_min).c_str());
    TEST("T2.1 i256 -(2^255+1) -> overflow", ineg_over.error == parse_error::overflow);

    // Ceros a la izquierda no cuentan para el desbordamiento.
    const auto padded = U::try_from_string(("000000" + u_max_str).c_str());
    TEST("T2.1 ceros a la izquierda no desbordan", padded.success() && padded.value == U::max());

    // --- try_from_string: codigos y posiciones ---
    const auto e_null = U::try_from_string(nullptr);
    TEST("T2.1 try_from_string(nullptr) -> null_pointer", e_null.error == parse_error::null_pointer);

    const auto e_empty = U::try_from_string("");
    TEST("T2.1 try_from_string(\"\") -> empty_string", e_empty.error == parse_error::empty_string);

    const auto e_char = U::try_from_string("12x45");
    TEST("T2.1 try_from_string(\"12x45\") -> invalid_character",
         e_char.error == parse_error::invalid_character);
    TEST("T2.1 error_index apunta al caracter culpable", e_char.error_index == 2);

    const auto e_sign = I::try_from_string("-");
    TEST("T2.1 try_from_string(\"-\") -> no_digits", e_sign.error == parse_error::no_digits);

    const auto e_ok = U::try_from_string("42");
    TEST("T2.1 try_from_string valido -> success", e_ok.success());
    TEST("T2.1 try_from_string valido -> valor", e_ok.value == U{std::uint64_t{42}});
}

// =============================================================================
// 3c. Constructor desde punto flotante: valores no finitos (T2.2)
//
// Antes, std::fmod(inf, 2^64) daba NaN y el static_cast<uint64_t>(NaN) era UB:
// uint_fixed_t<4>{inf} producia un valor basura cercano a 2^255.
// =============================================================================

static void test_float_non_finite()
{
    std::cout << "\n--- 3c. constructor desde float no finito (T2.2) ---\n";

    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();

    TEST("T2.2 u256(+inf) == max()", uint_fixed_t<4>{inf} == uint_fixed_t<4>::max());
    TEST("T2.2 u256(-inf) == 0", uint_fixed_t<4>{-inf}.is_zero());
    TEST("T2.2 u256(NaN)  == 0", uint_fixed_t<4>{nan}.is_zero());

    TEST("T2.2 i256(+inf) == max()", int_fixed_t<4>{inf} == int_fixed_t<4>::max());
    TEST("T2.2 i256(-inf) == min()", int_fixed_t<4>{-inf} == int_fixed_t<4>::min());
    TEST("T2.2 i256(NaN)  == 0", int_fixed_t<4>{nan}.is_zero());

    // float y long double toman el mismo camino.
    TEST("T2.2 u128(+inf f) == max()",
         uint_fixed_t<2>{std::numeric_limits<float>::infinity()} == uint_fixed_t<2>::max());
    TEST("T2.2 u128(NaN long double) == 0",
         uint_fixed_t<2>{std::numeric_limits<long double>::quiet_NaN()}.is_zero());

    // Los finitos siguen comportandose igual que antes.
    TEST("T2.2 u256(3.9) == 3", uint_fixed_t<4>{3.9} == uint_fixed_t<4>{std::uint64_t{3}});
    TEST("T2.2 i256(-3.9) == -3", int_fixed_t<4>{-3.9} == int_fixed_t<4>{std::int64_t{-3}});
    TEST("T2.2 u256(-3.9) == 0 (negativo a unsigned)", uint_fixed_t<4>{-3.9}.is_zero());
    TEST("T2.2 u256(0.0) == 0", uint_fixed_t<4>{0.0}.is_zero());
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
    test_from_string_overflow();
    test_float_non_finite();

    test_round_trip<1>("N=1 (64-bit)");
    test_round_trip<2>("N=2 (128-bit)");
    test_round_trip<4>("N=4 (256-bit)");
    test_round_trip<8>("N=8 (512-bit)");

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
