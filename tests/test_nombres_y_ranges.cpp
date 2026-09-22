// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: nombres canonicos (ADR-021) y soporte de std::ranges  -- P4, E1 y E2
// Part of int128 Library
// =============================================================================
//
// DOS COSAS, Y LAS DOS SE COMPRUEBAN POR EL VALOR
// ------------------------------------------------
// E1 unifica nombres. Lo facil seria comprobar que los dos nombres **compilan**,
// y eso no vale nada: un alias que se desincroniza compila igual. Aqui se
// comprueba que **dan el mismo valor**, que es lo unico que hace de un alias un
// alias.
//
// E2 anade `difference_type` para `std::ranges`. Igual: que
// `std::weakly_incrementable` sea cierto no dice que `views::iota` recorra lo
// que debe. Se recorre y se cuentan los elementos.
//
// Y `sqrt` con signo es nuevo, no un alias: se cruza contra la propiedad que lo
// define --`r*r <= x < (r+1)^2`-- en las cuatro representaciones.
// =============================================================================

#include "fixed_width_int_t.hpp"
#include "int128_param_arithmetic.hpp"
#include "int128_param_bits.hpp"
#include "int128_param_cmath.hpp"
#include "int128_param_numeric.hpp"
#include "fixed_point_t.hpp"

#include <cstdio>
#include <iterator>
#include <ranges>
#include <string>

namespace
{
    int fallos = 0;
    long long casos = 0;

    using namespace nstd;

    void comprueba(bool ok, const char *que)
    {
        ++casos;
        if (!ok)
        {
            std::printf("  [FALLA] %s\n", que);
            ++fallos;
        }
    }

    struct xorshift
    {
        std::uint64_t s;
        std::uint64_t operator()() noexcept
        {
            s ^= s << 13;
            s ^= s >> 7;
            s ^= s << 17;
            return s;
        }
    };

    // ---------------------------------------------------------------------
    // `sqrt` con signo: se cruza contra la PROPIEDAD, no contra otra funcion
    // ---------------------------------------------------------------------
    //
    // `r = floor(sqrt(x))` si y solo si `r*r <= x` y `(r+1)*(r+1) > x`. Es la
    // definicion, y comprobarla no depende de ninguna otra implementacion.
    //
    // Se hace en el tipo ANCHO para que `(r+1)^2` no desborde: con `x` cerca
    // del maximo, `r` ronda `2^63` y su cuadrado no cabe en 128 bits.
    template <typename T>
    bool raiz_es_correcta(const T &x, const T &r)
    {
        using A = uint_fixed_t<2 * T::num_limbs>;
        const A ra{uint_fixed_t<T::num_limbs>{r}};
        const A xa{uint_fixed_t<T::num_limbs>{x}};
        const A uno = A::one();
        return (ra * ra) <= xa && ((ra + uno) * (ra + uno)) > xa;
    }

    /// Cruza `sqrt` con signo en una representacion concreta.
    template <representation_form Form>
    void cruza_sqrt_con_signo(const char *como)
    {
        using T = fixed_int_t<2, signedness::signed_type, Form, overflow_policy::wrap>;

        const long long valores[] = {0, 1, 2, 3, 4, 5, 8, 9, 15, 16, 17, 100, 10000, 123456789};
        for (long long v : valores)
        {
            const T x{v};
            const T r = nstd::sqrt(x);
            ++casos;
            if (!raiz_es_correcta(x, r))
            {
                std::printf("  [FALLA] sqrt(%lld) en %s da %s, que no cumple r*r <= x < (r+1)^2\n", v, como,
                            r.to_string().c_str());
                ++fallos;
            }
        }

        // Los negativos devuelven cero: es la verruga heredada de 1.75, y esta
        // en el test para que nadie la "arregle" sin leer ADR-021.
        const long long negativos[] = {-1, -4, -100, -123456789};
        for (long long v : negativos)
        {
            ++casos;
            if (!nstd::sqrt(T{v}).is_zero())
            {
                std::printf("  [FALLA] sqrt(%lld) en %s no da cero (ADR-021, decision 3)\n", v, como);
                ++fallos;
            }
        }

        // Y tiene que coincidir con la version sin signo para el mismo valor:
        // si no, la conversion estaria leyendo limbos en vez del valor, que es
        // la equivocacion que el tramo 3 se encontro seis veces.
        xorshift rng{0x5A17ULL};
        for (int i = 0; i < 200; ++i)
        {
            const std::uint64_t m = rng() >> 2; // cabe holgado con signo
            const T x{m};
            const uint_fixed_t<2> xu{m};
            ++casos;
            if (nstd::sqrt(x).to_string() != nstd::sqrt(xu).to_string())
            {
                std::printf("  [FALLA] sqrt en %s no coincide con la version sin signo\n", como);
                ++fallos;
            }
        }
    }

    /// `has_single_bit` tiene que dar lo mismo que `is_power_of_2`, siempre.
    template <representation_form Form>
    void cruza_has_single_bit(const char *como)
    {
        using T = fixed_int_t<2, signedness::signed_type, Form, overflow_policy::wrap>;

        xorshift rng{0xB175ULL};
        for (int i = 0; i < 300; ++i)
        {
            T x{};
            x.set_limb(0, rng());
            x.set_limb(1, rng() >> 1);
            ++casos;
            if (nstd::has_single_bit(x) != nstd::is_power_of_2(x))
            {
                std::printf("  [FALLA] has_single_bit e is_power_of_2 discrepan en %s\n", como);
                ++fallos;
            }
        }
        // Y las esquinas: las potencias de dos de verdad.
        for (unsigned b = 0; b < 126; ++b)
        {
            const T x = T::one() << b;
            ++casos;
            if (!nstd::has_single_bit(x) || !nstd::is_power_of_2(x))
            {
                std::printf("  [FALLA] 2^%u no se reconoce en %s\n", b, como);
                ++fallos;
            }
        }
        comprueba(!nstd::has_single_bit(T{}), "el cero no tiene un solo bit");
    }

    /// `views::iota` sobre una celda: que recorra lo que debe, no solo que compile.
    template <typename T>
    void recorre_iota(int esperados, const char *como)
    {
        static_assert(std::weakly_incrementable<T>, "le falta difference_type o ++ que devuelva T&");

        auto r = std::views::iota(T{0}, T{5});
        int n = 0;
        T suma{};
        for (auto x : r)
        {
            suma = suma + x;
            ++n;
        }
        ++casos;
        if (n != esperados)
        {
            std::printf("  [FALLA] views::iota en %s recorre %d y no %d\n", como, n, esperados);
            ++fallos;
        }
        // 0+1+2+3+4 = 10. Si `++` avanzara un epsilon en vez de uno, el recorrido
        // no llegaria nunca y la suma no seria esta.
        ++casos;
        if (suma != T{10})
        {
            std::printf("  [FALLA] views::iota en %s suma %s y no 10\n", como, suma.to_string().c_str());
            ++fallos;
        }
    }
} // namespace

int main()
{
    std::printf("=== nombres canonicos (ADR-021) y std::ranges ===\n\n");

    // ------------------------------------------- E1: los dos nombres dan lo mismo ---
    std::printf("-- sqrt e isqrt: el mismo valor, no solo el mismo nombre\n");
    {
        xorshift rng{0x59B7ULL};
        for (int i = 0; i < 300; ++i)
        {
            const std::uint64_t lo = rng();
            const std::uint64_t hi = rng();
            const uint128_t x{hi, lo};
            ++casos;
            if (nstd::sqrt(x) != nstd::isqrt(x))
            {
                std::printf("  [FALLA] sqrt e isqrt discrepan en 1.75\n");
                ++fallos;
            }
        }
        // Y el canonico existe en las DOS familias, que es el objetivo de E1.
        comprueba(nstd::sqrt(uint128_t{0, 144}) == uint128_t{0, 12}, "sqrt(144) = 12 en 1.75");
        comprueba(nstd::sqrt(uint_fixed_t<2>{144}) == uint_fixed_t<2>{12}, "sqrt(144) = 12 en limbos");
    }

    std::printf("-- mul_wide y widening_mul: el mismo valor\n");
    {
        xorshift rng{0x3D1AULL};
        for (int i = 0; i < 200; ++i)
        {
            const uint128_t a{rng(), rng()};
            const uint128_t b{rng(), rng()};
            ++casos;
            if (nstd::mul_wide(a, b) != nstd::widening_mul(a, b))
            {
                std::printf("  [FALLA] mul_wide y widening_mul discrepan\n");
                ++fallos;
            }
        }
        // El canonico, en las dos familias.
        const auto p175 = nstd::mul_wide(uint128_t{0, 1000000}, uint128_t{0, 1000000});
        comprueba(p175.low128() == uint128_t{0, 1000000000000ULL}, "mul_wide en 1.75");
        const auto plim = nstd::mul_wide(uint_fixed_t<2>{1000000}, uint_fixed_t<2>{1000000});
        comprueba(plim == uint_fixed_t<4>{1000000000000ULL}, "mul_wide en limbos");
    }

    std::printf("-- has_single_bit e is_power_of_2, en las dos familias\n");
    {
        cruza_has_single_bit<representation_form::twos_complement>("complemento a dos");
        cruza_has_single_bit<representation_form::magnitude_sign>("Magnitud-Signo");
        cruza_has_single_bit<representation_form::excess_k>("Exceso-K");

        xorshift rng{0xB1751ULL};
        for (int i = 0; i < 200; ++i)
        {
            const uint128_t x{rng(), rng()};
            ++casos;
            if (nstd::has_single_bit(x) != nstd::is_power_of_2(x))
            {
                std::printf("  [FALLA] has_single_bit e is_power_of_2 discrepan en 1.75\n");
                ++fallos;
            }
        }
    }

    std::printf("-- pow: las dos firmas dan lo mismo, en las dos familias\n");
    {
        for (unsigned e = 0; e < 12; ++e)
        {
            const uint_fixed_t<2> base{3};
            ++casos;
            if (nstd::pow(base, e) != nstd::pow(base, uint_fixed_t<2>{e}))
            {
                std::printf("  [FALLA] las dos firmas de pow discrepan sin signo, e=%u\n", e);
                ++fallos;
            }

            const int_fixed_t<2> bs{-3};
            ++casos;
            if (nstd::pow(bs, e) != nstd::pow(bs, int_fixed_t<2>{static_cast<long long>(e)}))
            {
                std::printf("  [FALLA] las dos firmas de pow discrepan con signo, e=%u\n", e);
                ++fallos;
            }

            const uint128_t b175{0, 3};
            ++casos;
            if (nstd::pow(b175, e) != nstd::pow(b175, uint128_t{0, e}))
            {
                std::printf("  [FALLA] las dos firmas de pow discrepan en 1.75, e=%u\n", e);
                ++fallos;
            }
        }
        comprueba(nstd::pow(uint_fixed_t<2>{2}, 10U) == uint_fixed_t<2>{1024}, "2^10 = 1024");
        comprueba(nstd::pow(int_fixed_t<2>{-2}, 3U) == int_fixed_t<2>{-8}, "(-2)^3 = -8");
        comprueba(nstd::pow(int_fixed_t<2>{5}, int_fixed_t<2>{0}) == int_fixed_t<2>::one(), "x^0 = 1");
        comprueba(nstd::pow(int_fixed_t<2>{5}, int_fixed_t<2>{-3}) == int_fixed_t<2>::one(),
                  "exponente negativo da uno: un racional no es representable");
    }

    // --------------------------------------- E1: sqrt con signo, nuevo ---
    std::printf("-- sqrt con signo, en las tres representaciones con signo\n");
    {
        cruza_sqrt_con_signo<representation_form::twos_complement>("complemento a dos");
        cruza_sqrt_con_signo<representation_form::magnitude_sign>("Magnitud-Signo");
        cruza_sqrt_con_signo<representation_form::excess_k>("Exceso-K");
    }

    // --------------------------------------------------- E2: ranges -----
    std::printf("-- views::iota recorre de verdad, en las seis celdas\n");
    {
        recorre_iota<uint_fixed_t<2>>(5, "uint");
        recorre_iota<int_fixed_t<2>>(5, "int/C2");
        recorre_iota<fixed_int_t<2, signedness::signed_type, representation_form::magnitude_sign>>(5,
                                                                                                   "int/MS");
        recorre_iota<fixed_int_t<2, signedness::signed_type, representation_form::excess_k>>(5, "int/EK");
        recorre_iota<ufixed_point_t<2, 1>>(5, "fijo/u");
        recorre_iota<sfixed_point_t<2, 1>>(5, "fijo/s");

        // `difference_type` es un entero del LENGUAJE a proposito: el estandar
        // exige `is-signed-integer-like`, y un tipo de usuario no puede serlo.
        static_assert(std::is_same_v<uint_fixed_t<2>::difference_type, std::ptrdiff_t>,
                      "difference_type tiene que ser un entero del lenguaje");
        static_assert(std::is_same_v<sfixed_point_t<2, 1>::difference_type, std::ptrdiff_t>,
                      "difference_type tiene que ser un entero del lenguaje");

        // Y NO son iteradores, que esta bien: a un numero le falta `operator*`.
        static_assert(!std::input_or_output_iterator<uint_fixed_t<2>>, "un numero no es un iterador");

        // Con `views::take` y `views::filter`, que es lo que se hace de verdad.
        auto pares = std::views::iota(uint_fixed_t<2>{0}, uint_fixed_t<2>{20}) |
                     std::views::filter([](const uint_fixed_t<2> &x) { return nstd::is_even(x); }) |
                     std::views::take(3);
        int n = 0;
        for (auto x : pares)
        {
            (void)x;
            ++n;
        }
        comprueba(n == 3, "iota | filter | take recorre tres");
    }

    std::printf("\n%lld comprobaciones, %d fallos\n", casos, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
