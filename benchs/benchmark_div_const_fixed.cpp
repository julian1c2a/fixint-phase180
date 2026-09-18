// =============================================================================
// `div<D>()` contra `operator/` -- lo que gana tener el divisor en compilacion
// =============================================================================
//
// P1.5 tramo 2e. `div<D>()` **no usa otro algoritmo**: es el mismo bucle de
// `div_un_limbo`, con el preambulo --normalizar el divisor y calcular su
// reciproco, que cuesta un `divq`-- resuelto en compilacion por ser `D` una
// constante de plantilla.
//
// Asi que lo que se mide aqui es exactamente cuanto pesa ese preambulo, y la
// respuesta depende de `N`: es un **coste fijo** por llamada contra un bucle que
// crece con N. Mismo patron que `NSTD_MG_3POR2_MIN`.
//
// POR QUE HAY QUE VOLVER A MEDIRLO
// --------------------------------
// `PERFORMANCE.md` traia una tabla del 9 sep 2026 que decia que este truco «solo
// paga con divisores grandes» y que con los pequenos PIERDE (0,52x-0,92x). Esa
// tabla es de la implementacion vieja sobre `int128_param_t`, y desde entonces
// **su rival ha cambiado dos veces**: Moller-Granlund 2/1 (hasta 4,97x) y el
// `divq` en linea (1,28x-1,39x). Un umbral es relativo a su rival.
//
// Ademas cruzaba el eje equivocado: medir divisores grandes contra pequenos con
// N fijo esconde que lo que manda es N.
#include "fixed_width_int_t.hpp"
#include "bench_adaptativo.hpp"

#include <cstdint>
#include <cstdio>
#include <tuple>

using namespace nstd;

namespace
{
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

    /// @tparam N Anchura. @tparam D El divisor, constante de plantilla.
    template <std::size_t N, std::uint64_t D>
    void mide(const char *etiqueta)
    {
        using T = uint_fixed_t<N>;
        static std::array<T, OPERANDOS> a{};
        static bool iniciado = false;
        if (!iniciado)
        {
            xorshift rng{0xDEC0DEULL + N + D};
            for (auto &x : a)
                for (std::size_t i = 0; i < N; ++i)
                    x.set_limb(i, rng());
            iniciado = true;
        }
        const T d{std::uint64_t{D}};

        auto normal = [&](std::size_t i)
        {
            T q = a[i % OPERANDOS] / d;
            doNotOptimize(q);
        };
        auto constante = [&](std::size_t i)
        {
            T q = a[i % OPERANDOS].template div<D>();
            doNotOptimize(q);
        };

        const auto m = bench::mide_entrelazado(std::make_tuple(normal, constante), 20);
        std::printf("| %5zu | %-14s | %10.0f | %10.0f | %6.2fx |\n", N, etiqueta, m[0].minimo, m[1].minimo,
                    m[0].minimo / m[1].minimo);
        std::fflush(stdout);
    }
} // namespace

int main()
{
    print_header("div<D>() contra operator/");
    std::printf("\nMismo bucle; lo unico que cambia es que el preambulo se resuelve en\n"
                "compilacion. Entrelazadas, 20 repeticiones, minimo.\n\n");
    std::printf("|     N | divisor        | operator/  |   div<D>() |  razon |\n");
    std::printf("|------:|:---------------|-----------:|-----------:|-------:|\n");

    // El eje que manda: N, con el divisor fijo.
    mide<1, 10>("10");
    mide<2, 10>("10");
    mide<3, 10>("10");
    mide<4, 10>("10");
    mide<8, 10>("10");
    mide<16, 10>("10");
    mide<32, 10>("10");
    mide<64, 10>("10");
    mide<128, 10>("10");

    // Y el eje que la tabla vieja creia que mandaba: el divisor, con N fijo.
    mide<4, 3>("3");
    mide<4, 7>("7");
    mide<4, 1000000007ULL>("1e9+7");
    mide<4, 10000000000000000000ULL>("1e19");
    mide<4, (std::uint64_t{1} << 63)>("2^63  (s=0)");
    mide<4, ~std::uint64_t{0}>("2^64-1 (s=0)");
    print_footer();
    return 0;
}
