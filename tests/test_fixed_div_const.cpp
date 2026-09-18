// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: div<D>() / mod<D>() / divmod_const<D>() -- divisor constante
// Part of int128 Library
// =============================================================================
//
// P1.5 tramo 2e. Lo unico que importa de verdad es una cosa:
//
//   **`div<D>()` tiene que dar EXACTAMENTE lo mismo que `operator/`.**
//
// Si no, da igual lo rapido que sea. Y como el ahorro consiste en resolver la
// normalizacion y el reciproco en compilacion, los casos que hay que apretar son
// los que dependen de eso: divisores con y sin bit alto puesto (`s == 0` frente a
// `s != 0`), potencias de dos, el divisor maximo de un limbo, y el 1.
//
// Se cruza contra `operator/` con **valores construidos, no solo aleatorios**:
// los aleatorios no llegan a las esquinas (ver el barrido de la 3/2).
// =============================================================================

#include "fixed_width_int_t.hpp"

#include <cstdio>

namespace
{
    int fallos = 0;
    long long casos = 0;

    using namespace nstd;

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

    /// Cruza `div<D>`, `mod<D>` y `divmod_const<D>` contra `/` y `%` para un
    /// valor concreto.
    template <std::uint64_t D, typename T>
    void cruza(const T &a, const char *donde)
    {
        const T d{std::uint64_t{D}};
        const T q_ref = a / d;
        const T r_ref = a % d;

        const T q = a.template div<D>();
        const T r = a.template mod<D>();
        const auto par = a.template divmod_const<D>();

        ++casos;
        if (q != q_ref || r != r_ref || par.first != q_ref || par.second != r_ref)
        {
            std::printf("  [FALLA] D=%llu %s\n", static_cast<unsigned long long>(D), donde);
            ++fallos;
        }
    }

    /// Todos los divisores interesantes para una anchura.
    ///
    /// La lista no es caprichosa: `1` es el caso degenerado, `2` y `1<<63` son
    /// potencias de dos (la segunda con `s == 0`), `3` y `10` son los divisores
    /// que mas se usan de verdad, `1e19` es el mayor redondo que cabe en un
    /// limbo, y `~0` es el maximo, donde `s == 0` y el reciproco es minimo.
    template <typename T>
    void todos_los_divisores(const T &a, const char *donde)
    {
        cruza<1>(a, donde);
        cruza<2>(a, donde);
        cruza<3>(a, donde);
        cruza<10>(a, donde);
        cruza<7>(a, donde);
        cruza<1000000007ULL>(a, donde);
        cruza<std::uint64_t{1} << 63>(a, donde);       // s == 0, potencia de dos
        cruza<(std::uint64_t{1} << 63) + 1>(a, donde); // s == 0, impar
        cruza<10000000000000000000ULL>(a, donde);      // 1e19: SI cabe en 64 bits
        cruza<~std::uint64_t{0}>(a, donde);            // el maximo de un limbo
    }

    template <std::size_t N>
    void barre(const char *nombre)
    {
        using T = uint_fixed_t<N>;
        std::printf("-- N=%zu (%s)\n", N, nombre);

        // Las esquinas del DIVIDENDO.
        todos_los_divisores(T{}, "cero");
        todos_los_divisores(T{std::uint64_t{1}}, "uno");
        todos_los_divisores(T::max(), "max");
        todos_los_divisores(T::max() - T{std::uint64_t{1}}, "max-1");

        // Un limbo alto solo, que es donde el arrastre del desplazamiento manda.
        T solo_alto{};
        solo_alto.set_limb(N - 1, ~std::uint64_t{0});
        todos_los_divisores(solo_alto, "solo el limbo alto");

        T solo_bajo{std::uint64_t{~std::uint64_t{0}}};
        todos_los_divisores(solo_bajo, "solo el limbo bajo");

        // Y aleatorios, que cubren el caso comun.
        xorshift rng{0x5EEDULL + N};
        for (int v = 0; v < 40; ++v)
        {
            T x{};
            for (std::size_t i = 0; i < N; ++i)
                x.set_limb(i, rng());
            todos_los_divisores(x, "aleatorio");
        }
    }
} // namespace

int main()
{
    std::printf("=== div<D> / mod<D> / divmod_const<D> contra / y %% ===\n\n");

    barre<1>("un limbo");
    barre<2>("128 bits");
    barre<3>("impar");
    barre<4>("256 bits");
    barre<8>("512 bits");
    barre<16>("1024 bits");

    // Y que de verdad es `constexpr`: si no lo fuera, esto no compila.
    {
        constexpr uint_fixed_t<2> x{std::uint64_t{1000000}};
        static_assert(x.div<7>() == uint_fixed_t<2>{std::uint64_t{142857}},
                      "div<D> tiene que valer en evaluacion constante");
        static_assert(x.mod<7>() == uint_fixed_t<2>{std::uint64_t{1}},
                      "mod<D> tiene que valer en evaluacion constante");
        static_assert(x.divmod_const<7>().first == uint_fixed_t<2>{std::uint64_t{142857}},
                      "divmod_const<D> tiene que valer en evaluacion constante");
    }

    std::printf("\n%lld casos cruzados contra / y %%, %d fallos\n", casos, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
