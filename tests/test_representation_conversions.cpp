// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: representation.hpp - Conversiones entre representaciones 128 bits
// Part of int128 Library - https://github.com/[repo]
// License: BSL-1.0
// =============================================================================

#include "representation.hpp"
#include <iostream>

// -----------------------------------------------------------------------------
// ESTE TEST COMPRUEBA CON assert(), Y LA SUITE SE COMPILA CON -DNDEBUG.
//
// Sin el `#undef` de aqui abajo, `assert()` se expande a nada en todos los modos
// release --que son los que se ejecutan-- y este fichero pasaria SIEMPRE, sin
// verificar nada. Se descubrio el 26 ago 2026 auditando que senales podian decir
// "bien" sin haberlo comprobado: eran 8 assert() inertes.
//
// El `#undef` va DESPUES de los demas includes a proposito: `assert` es una
// macro que <cassert> redefine cada vez que se incluye, segun como este NDEBUG
// en ese momento. Poniendolo aqui, gana este.
//
// OJO: assert() aborta en el primer fallo y no da recuento, al reves que la
// macro TEST() que usan los otros 51 ficheros de la suite. El codigo de salida
// es distinto de cero, que es lo que el arnes necesita, pero convertirlos a
// TEST() sigue pendiente (P0.3 en NEXT_STEPS.md).
// -----------------------------------------------------------------------------
#undef NDEBUG
#include <cassert>

using namespace nstd;

int main()
{
    std::cout << "Test: conversiones TC <-> MS <-> EK (128 bits)\n";

    // Caso base: valor positivo
    {
        uint64_t tc_high{0x123456789ABCDEF0}, tc_low{0x0FEDCBA987654321};
        uint64_t ms_high, ms_low, ek_high, ek_low;
        uint64_t out_high, out_low;

        // TC -> MS -> TC
        twos_complement128_to_ms(tc_high, tc_low, ms_high, ms_low);
        ms128_to_twos_complement(ms_high, ms_low, out_high, out_low);
        assert(tc_high == out_high && tc_low == out_low);

        // TC -> EK -> TC
        twos_complement128_to_excess_k(tc_high, tc_low, ek_high, ek_low);
        excess_k128_to_twos_complement(ek_high, ek_low, out_high, out_low);
        assert(tc_high == out_high && tc_low == out_low);

        // MS -> EK -> MS
        ms128_to_excess_k(ms_high, ms_low, ek_high, ek_low);
        excess_k128_to_ms(ek_high, ek_low, out_high, out_low);
        assert(ms_high == out_high && ms_low == out_low);
    }

    // Caso negativo: -42 en TC
    {
        uint64_t tc_high{~0ULL}, tc_low{~41ULL}; // -42 en TC
        uint64_t ms_high, ms_low, ek_high, ek_low;
        uint64_t out_high, out_low;

        // TC -> MS -> TC
        twos_complement128_to_ms(tc_high, tc_low, ms_high, ms_low);
        ms128_to_twos_complement(ms_high, ms_low, out_high, out_low);
        assert(tc_high == out_high && tc_low == out_low);

        // TC -> EK -> TC
        twos_complement128_to_excess_k(tc_high, tc_low, ek_high, ek_low);
        excess_k128_to_twos_complement(ek_high, ek_low, out_high, out_low);
        assert(tc_high == out_high && tc_low == out_low);

        // MS -> EK -> MS
        ms128_to_excess_k(ms_high, ms_low, ek_high, ek_low);
        excess_k128_to_ms(ek_high, ek_low, out_high, out_low);
        assert(ms_high == out_high && ms_low == out_low);
    }

    // Caso cero
    {
        uint64_t tc_high{0}, tc_low{0};
        uint64_t ms_high, ms_low, ek_high, ek_low;
        uint64_t out_high, out_low;

        twos_complement128_to_ms(tc_high, tc_low, ms_high, ms_low);
        ms128_to_twos_complement(ms_high, ms_low, out_high, out_low);
        assert(tc_high == out_high && tc_low == out_low);

        twos_complement128_to_excess_k(tc_high, tc_low, ek_high, ek_low);
        excess_k128_to_twos_complement(ek_high, ek_low, out_high, out_low);
        assert(tc_high == out_high && tc_low == out_low);
    }

    std::cout << "Todos los tests de conversión 128 bits PASAN.\n";
    return 0;
}
