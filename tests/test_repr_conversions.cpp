// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: representation.hpp -- conversiones genericas por N (P1.5 tramo 3)
// Part of int128 Library
// =============================================================================
//
// Magnitud-Signo y Exceso-K son **codificaciones**, no aritmeticas: la suma en MS
// se hace decodificando a complemento a dos, sumando con el codigo que ya existe
// y recodificando. Asi que todo el porte del tramo 3 se apoya en que estas
// conversiones sean exactas, y eso es lo que se prueba aqui.
//
// LAS DOS ASIMETRIAS QUE HAY QUE APRETAR
// --------------------------------------
// 1. **MS tiene dos ceros** (`+0` y `-0`) y complemento a dos uno. La ida no es
//    inyectiva, asi que la vuelta no puede devolver siempre el mismo patron.
// 2. **El minimo de complemento a dos no existe en MS**: su magnitud es
//    `2^(64N-1)` y pisaria el bit de signo. `c2_a_ms` satura, y eso pierde
//    informacion.
//
// Un test que solo probara ida y vuelta con valores comodos no veria ninguna de
// las dos. Por eso se barren **las esquinas a mano** ademas de aleatorios.
// =============================================================================

#include "representation.hpp"

#include <array>
#include <cstdint>
#include <cstdio>

namespace
{
    int fallos = 0;
    long long casos = 0;

    using nstd::representation_form;
    namespace repr = nstd::repr;

    template <std::size_t N>
    void di(const char *que, const std::array<std::uint64_t, N> &x)
    {
        std::printf("  [FALLA] %s  N=%zu  limbos:", que, N);
        for (std::size_t i = N; i-- > 0;)
            std::printf(" %016llx", static_cast<unsigned long long>(x[i]));
        std::printf("\n");
        ++fallos;
    }

    // ---------------------------------------------------- Exceso-K: biyeccion ---
    //
    // Con sesgo 2^(64N-1) la conversion es invertir el bit alto, asi que tiene
    // que ser **una involucion exacta**: aplicarla dos veces devuelve el original,
    // sin excepciones ni perdida.
    template <std::size_t N>
    void ek_es_involucion(const std::array<std::uint64_t, N> &original)
    {
        std::array<std::uint64_t, N> x = original;
        repr::c2_ek_ida_y_vuelta<N>(x);
        repr::c2_ek_ida_y_vuelta<N>(x);
        ++casos;
        if (x != original)
            di<N>("EK no es involucion", original);
    }

    // Y que de verdad aplica el sesgo: el cero de complemento a dos tiene que
    // caer en el medio del rango sin signo.
    template <std::size_t N>
    void ek_cero_va_al_medio()
    {
        std::array<std::uint64_t, N> x{};
        repr::desde_c2<representation_form::excess_k, N>(x);
        ++casos;
        bool ok = (x[N - 1] == (std::uint64_t{1} << 63));
        for (std::size_t i = 0; i + 1 < N; ++i)
            ok = ok && (x[i] == 0);
        if (!ok)
            di<N>("el cero no cae en el medio del rango", x);
    }

    // ------------------------------------------------ MS: ida y vuelta exacta ---
    //
    // Para todo valor de complemento a dos que NO sea el minimo, ida y vuelta
    // tiene que devolver exactamente el mismo patron.
    template <std::size_t N>
    void ms_ida_y_vuelta(const std::array<std::uint64_t, N> &original)
    {
        // El minimo se trata aparte: ahi la perdida es conocida y deliberada.
        bool es_minimo = (original[N - 1] == (std::uint64_t{1} << 63));
        for (std::size_t i = 0; i + 1 < N && es_minimo; ++i)
            es_minimo = (original[i] == 0);
        if (es_minimo)
            return;

        std::array<std::uint64_t, N> x = original;
        repr::c2_a_ms<N>(x);
        repr::ms_a_c2<N>(x);
        ++casos;
        if (x != original)
            di<N>("MS: ida y vuelta no devuelve el original", original);
    }

    /// El minimo de complemento a dos satura, y hay que comprobar **a cuanto**.
    template <std::size_t N>
    void ms_satura_el_minimo()
    {
        std::array<std::uint64_t, N> x{};
        x[N - 1] = std::uint64_t{1} << 63; // el minimo
        repr::c2_a_ms<N>(x);

        // Esperado: signo puesto y magnitud = 2^(64N-1) - 1, o sea todo unos
        // menos el bit de signo.
        ++casos;
        bool ok = (x[N - 1] == ~std::uint64_t{0});
        for (std::size_t i = 0; i + 1 < N; ++i)
            ok = ok && (x[i] == ~std::uint64_t{0});
        if (!ok)
            di<N>("el minimo no satura a -(2^(64N-1) - 1)", x);

        // Y al volver tiene que dar el minimo mas uno, no el minimo.
        repr::ms_a_c2<N>(x);
        ++casos;
        bool vuelve = (x[N - 1] == (std::uint64_t{1} << 63));
        for (std::size_t i = 0; i + 1 < N; ++i)
            vuelve = vuelve && (x[i] == 0);
        if (vuelve)
            di<N>("el minimo saturado vuelve al minimo, y no deberia", x);
    }

    /// Los dos ceros de MS van los dos a cero en complemento a dos.
    template <std::size_t N>
    void ms_dos_ceros()
    {
        std::array<std::uint64_t, N> mas_cero{};
        std::array<std::uint64_t, N> menos_cero{};
        menos_cero[N - 1] = std::uint64_t{1} << 63; // signo puesto, magnitud 0

        repr::ms_a_c2<N>(mas_cero);
        repr::ms_a_c2<N>(menos_cero);
        ++casos;
        if (mas_cero != menos_cero)
            di<N>("+0 y -0 de MS no dan el mismo cero", menos_cero);

        std::array<std::uint64_t, N> nada{};
        ++casos;
        if (menos_cero != nada)
            di<N>("-0 de MS no da cero", menos_cero);
    }

    /// MS conserva el orden de las magnitudes en los positivos.
    template <std::size_t N>
    void ms_positivos_intactos(const std::array<std::uint64_t, N> &original)
    {
        if (repr::bit_de_signo<N>(original))
            return;
        std::array<std::uint64_t, N> x = original;
        repr::c2_a_ms<N>(x);
        ++casos;
        if (x != original)
            di<N>("un positivo no deberia cambiar al pasar a MS", original);
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

    template <std::size_t N>
    void barre(const char *nombre)
    {
        std::printf("-- N=%zu (%s)\n", N, nombre);
        const std::uint64_t U = ~std::uint64_t{0};
        const std::uint64_t H = std::uint64_t{1} << 63;

        ek_cero_va_al_medio<N>();
        ms_satura_el_minimo<N>();
        ms_dos_ceros<N>();

        // Las esquinas, a mano.
        const std::uint64_t altos[] = {0, 1, H - 1, H, H + 1, U - 1, U};
        for (std::uint64_t alto : altos)
            for (std::uint64_t bajo : {std::uint64_t{0}, std::uint64_t{1}, U - 1, U})
            {
                std::array<std::uint64_t, N> x{};
                x[N - 1] = alto;
                x[0] = bajo;
                ek_es_involucion<N>(x);
                ms_ida_y_vuelta<N>(x);
                ms_positivos_intactos<N>(x);
            }

        // Y aleatorios.
        xorshift rng{0xC0DE5EEDULL + N};
        for (int v = 0; v < 200; ++v)
        {
            std::array<std::uint64_t, N> x{};
            for (std::size_t i = 0; i < N; ++i)
                x[i] = rng();
            ek_es_involucion<N>(x);
            ms_ida_y_vuelta<N>(x);
            ms_positivos_intactos<N>(x);
        }
    }
} // namespace

int main()
{
    std::printf("=== conversiones genericas de representacion (P1.5 tramo 3) ===\n\n");

    barre<1>("un limbo");
    barre<2>("128 bits");
    barre<3>("impar");
    barre<4>("256 bits");
    barre<8>("512 bits");

    // Y que valen en evaluacion constante: el tipo es `constexpr` de arriba abajo.
    {
        constexpr auto ek_ida_vuelta = []
        {
            std::array<std::uint64_t, 2> x{7, 0};
            nstd::repr::c2_ek_ida_y_vuelta<2>(x);
            nstd::repr::c2_ek_ida_y_vuelta<2>(x);
            return x;
        }();
        static_assert(ek_ida_vuelta[0] == 7 && ek_ida_vuelta[1] == 0,
                      "las conversiones tienen que valer en evaluacion constante");

        constexpr auto ms_de_menos_uno = []
        {
            std::array<std::uint64_t, 2> x{~std::uint64_t{0}, ~std::uint64_t{0}}; // -1 en C2
            nstd::repr::c2_a_ms<2>(x);
            return x;
        }();
        // -1 en MS: signo puesto y magnitud 1.
        static_assert(ms_de_menos_uno[0] == 1, "la magnitud de -1 es 1");
        static_assert(ms_de_menos_uno[1] == (std::uint64_t{1} << 63), "y el signo puesto");
    }

    std::printf("\n%lld comprobaciones, %d fallos\n", casos, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
