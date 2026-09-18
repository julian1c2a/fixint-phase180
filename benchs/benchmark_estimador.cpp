// =============================================================================
// La perilla `Estimador` de Knuth D: Knuth contra Moller-Granlund 3/2
// =============================================================================
//
// QUE MIDE
// --------
// El paso D3 de Knuth --estimar el digito del cociente-- es el bucle interno de
// toda la division. `div_knuth_d` lo tiene como parametro de plantilla, asi que
// las dos estimaciones existen **a la vez en este binario** y se pueden
// entrelazar, que es lo que pide docs/PLAN_SESION_MEDICION.md.
//
// LOS DOS EJES, Y POR QUE HACEN FALTA LOS DOS
// -------------------------------------------
// La division no es una curva, es una **superficie**: el coste depende de la
// anchura `N` y de los limbos significativos del divisor `n`, por separado.
//
// Un barrido que solo cruce `n = 2` da de 2x a 3,3x y parece que la 3/2 gana
// siempre. Es falso. Con `n = N` --que es lo que dan dos operandos aleatorios de
// la misma anchura, o sea **el caso mas frecuente**-- solo hay UN digito de
// cociente, el inverso de Moller-Granlund no tiene sobre que amortizarse, y la
// 3/2 pura **pierde hasta un 47%**.
//
// Por eso la tabla cruza `n = 2`, `n = N/2` y `n = N`, y la segunda parte barre
// directamente el numero de digitos de cociente, que es de lo que depende el
// umbral `NSTD_MG_3POR2_MIN`.
//
// COMO SE LEE
// -----------
// Razon > 1 significa que la variante es mas rapida que la estimacion de Knuth.
// `auto` es lo que usa la biblioteca por omision: Knuth por debajo del umbral,
// Moller-Granlund por encima.
#include "algorithms/div_kernels.hpp"
#include "bench_adaptativo.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <tuple>

using namespace nstd::algorithms;

namespace
{
    /// xorshift: los operandos deben ser los mismos para todas las variantes, y
    /// deben fijarse ANTES de medir.
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

    constexpr std::size_t OPERANDOS = 4;

    /// @tparam N Anchura. @tparam SIG Limbos significativos del divisor.
    template <std::size_t N, std::size_t SIG>
    void mide(const char *forma)
    {
        static_assert(SIG >= 2 && SIG <= N, "div_knuth_d pide un divisor de 2 limbos o mas");

        static std::array<std::array<std::uint64_t, N>, OPERANDOS> a{}, b{};
        static std::array<std::uint64_t, N> q{}, r{};
        static bool iniciado = false;
        if (!iniciado)
        {
            xorshift rng{0xD00DULL + N * 31 + SIG};
            for (std::size_t i = 0; i < OPERANDOS; ++i)
            {
                for (std::size_t k = 0; k < N; ++k)
                {
                    a[i][k] = rng();
                    b[i][k] = (k < SIG) ? rng() : 0;
                }
                if (b[i][SIG - 1] == 0)
                    b[i][SIG - 1] = 1; // o no tendria SIG limbos
            }
            iniciado = true;
        }

        auto knuth = [&](std::size_t i)
        {
            div_knuth_d<N, true, estimador_knuth>(a[i % OPERANDOS], b[i % OPERANDOS], q, r);
            doNotOptimize(q[0]);
        };
        auto mg = [&](std::size_t i)
        {
            div_knuth_d<N, true, estimador_moller_granlund>(a[i % OPERANDOS], b[i % OPERANDOS], q, r);
            doNotOptimize(q[0]);
        };
        auto autom = [&](std::size_t i)
        {
            div_knuth_d<N, true, estimador_auto<N>>(a[i % OPERANDOS], b[i % OPERANDOS], q, r);
            doNotOptimize(q[0]);
        };

        const auto m = bench::mide_entrelazado(std::make_tuple(knuth, mg, autom), 20);
        std::printf("| %5zu | %-7s | %7zu | %10.0f | %10.0f | %6.2fx | %9.0f | %6.2fx |\n", N, forma,
                    N - SIG + 1, m[0].minimo, m[1].minimo, m[0].minimo / m[1].minimo, m[2].minimo,
                    m[0].minimo / m[2].minimo);
        std::fflush(stdout);
    }

    void cabecera_tabla()
    {
        std::printf(
            "|     N | divisor | digitos |      knuth |     MG 3/2 |  razon |      auto |  razon |\n");
        std::printf(
            "|------:|:--------|--------:|-----------:|-----------:|-------:|----------:|-------:|\n");
        std::fflush(stdout);
    }
} // namespace

int main()
{
    std::printf("Umbral vivo: NSTD_MG_3POR2_MIN = %d digitos de cociente\n", NSTD_MG_3POR2_MIN);

    print_header("la perilla Estimador: los dos ejes");
    std::printf("\nEl borde de abajo se mide EN SERIO: N = 2..8 con n = N es donde la\n"
                "3/2 pura pierde, y es el caso que dan los operandos aleatorios.\n\n");
    cabecera_tabla();
    mide<2, 2>("n=N");
    mide<3, 2>("n=2");
    mide<3, 3>("n=N");
    mide<4, 2>("n=2");
    mide<4, 4>("n=N");
    mide<5, 5>("n=N");
    mide<6, 3>("n=N/2");
    mide<6, 6>("n=N");
    mide<7, 7>("n=N");
    mide<8, 2>("n=2");
    mide<8, 4>("n=N/2");
    mide<8, 8>("n=N");
    mide<16, 2>("n=2");
    mide<16, 8>("n=N/2");
    mide<16, 16>("n=N");
    mide<32, 2>("n=2");
    mide<32, 16>("n=N/2");
    mide<32, 32>("n=N");
    mide<64, 2>("n=2");
    mide<64, 32>("n=N/2");
    mide<64, 64>("n=N");
    mide<128, 2>("n=2");
    mide<128, 64>("n=N/2");
    mide<128, 128>("n=N");
    print_footer();

    print_header("de que depende el umbral: los digitos de cociente");
    std::printf("\nEl inverso de Moller-Granlund se paga UNA vez por llamada y ahorra por\n"
                "digito. Asi que lo que decide no es N, son los N-n+1 digitos.\n\n");
    cabecera_tabla();
    mide<16, 16>("n=N");
    mide<16, 15>("");
    mide<16, 14>("");
    mide<16, 13>("");
    mide<16, 12>("");
    mide<16, 10>("");
    mide<64, 64>("n=N");
    mide<64, 63>("");
    mide<64, 62>("");
    mide<64, 61>("");
    mide<64, 60>("");
    mide<64, 56>("");
    print_footer();
    return 0;
}
