// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: identidad de los alias de int128_param_t y construccion en Exceso-K
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// ANTES ESTE FICHERO NO COMPROBABA NADA. Imprimia el resultado de dos
// `is_same_v` y terminaba con «[OK] Template type test passed» y `return 0`,
// pasara lo que pasara: si un alias hubiera dejado de corresponder con su
// plantilla, habria impreso un `0` y seguido diciendo que todo bien.
//
// Se descubrio el 26 ago 2026 auditando que senales del proyecto podian decir
// «bien» sin haberlo comprobado.
//
// Las dos identidades de tipo son de tiempo de compilacion, asi que ahora son
// `static_assert`: si dejan de cumplirse, esto **no compila**, que es la forma
// mas fuerte de comprobarlo. Lo que si depende de la ejecucion lleva contador.
// =============================================================================

#include "int128_parameterized.hpp"

#include <iostream>
#include <type_traits>

using namespace nstd;

// =============================================================================
// Identidades de tipo — comprobadas en compilacion
// =============================================================================

static_assert(
    std::is_same_v<uint128_t, int128_param_t<signedness::unsigned_type, representation_form::binnat>>,
    "uint128_t debe ser int128_param_t<unsigned_type, binnat>");

static_assert(std::is_same_v<int128_tc_t,
                             int128_param_t<signedness::signed_type, representation_form::twos_complement>>,
              "int128_tc_t debe ser int128_param_t<signed_type, twos_complement>");

// =============================================================================
// Comprobaciones de ejecucion
// =============================================================================

static int g_passed{0};
static int g_failed{0};

static void check(const char *que, bool ok)
{
    if (ok)
    {
        ++g_passed;
        std::cout << "  [OK]   " << que << "\n";
    }
    else
    {
        ++g_failed;
        std::cout << "  [FAIL] " << que << "\n";
    }
}

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "Identidad de alias y construccion en Exceso-K\n";
    std::cout << "====================================================================\n\n";

    std::cout << "Identidades de tipo: comprobadas en compilacion (static_assert).\n\n";

    // En Exceso-K el valor almacenado lleva un sesgo, de modo que `is_zero()`
    // no compara contra el patron de bits todo a cero sino contra el sesgo.
    // Esto comprueba justamente eso: que la construccion aplica el sesgo.
    const int128_ek_t cero{0};
    const int128_ek_t cien{100};

    check("int128_ek_t{0}.is_zero() es cierto", cero.is_zero());
    check("int128_ek_t{100}.is_zero() es falso", !cien.is_zero());
    check("int128_ek_t{0} y int128_ek_t{100} no son iguales",
          cero.low() != cien.low() || cero.high() != cien.high());

    std::cout << "\nDiagnostico (no es una comprobacion):\n";
    std::cout << "  int128_ek_t{100}.low()  = " << cien.low() << "\n";
    std::cout << "  int128_ek_t{100}.high() = " << cien.high() << "\n";

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
