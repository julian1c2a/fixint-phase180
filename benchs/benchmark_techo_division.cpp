// =============================================================================
// El techo de la division: cuanto cuesta comparada con una multiplicacion
// =============================================================================
//
// Los algoritmos rapidos de division (Burnikel-Ziegler, Newton/Barrett)
// convierten la division en multiplicaciones. Su techo es el que publica GMP:
// «a 2N x N division is about 2 to 4 times slower than an N x N multiplication».
//
// Asi que antes de escribir ninguno de ellos, la pregunta que decide es una:
//
//     ?cuanto cuesta HOY una division (N, n=N/2) comparada con una
//      multiplicacion de N/2 x N/2 limbos?
//
// Si ya esta en 2-4x, no hay nada que ganar: un algoritmo que convierte division
// en multiplicaciones pagaria su sobrecoste sin recoger ninguna diferencia. Si
// esta en 10x o 30x, ahi esta el trabajo. Medirlo cuesta media hora y evita (o
// justifica) dias.
//
// Se mide `n = N/2` a proposito: es donde la superficie de la division tiene su
// maximo, porque el coste de Knuth D es O((N-n) * n) y eso se maximiza en N/2.
// Es exactamente el caso que atacan esos algoritmos.
//
// LO MEDIDO (18 sep 2026, clang)
// ------------------------------
//     N        8    16    32    64   128   256   512  1024  2048
//     div/mul 3,6x  2,8x  1,9x  1,6x  1,7x  1,9x  2,2x  2,8x  4,9x
//
// La razon tiene un MINIMO en N=64 y crece a los dos lados. Dentro del techo de
// GMP hasta N=1024, y solo lo pasa en N=2048 -- que es donde la multiplicacion
// entra en Toom-3 (`NSTD_TOOM3_MIN` = 1024 y el factor mide N/2) y la division
// se queda sin nada con que seguirla.
//
// Es decir: **en el rango donde esta biblioteca se usa, la division no esta mal
// servida en relacion a su propia multiplicacion.** La proyeccion escrita en
// docs/ESTUDIO_ALGORITMOS_RAPIDOS.md esperaba un umbral de ~45-50 limbos para
// Burnikel-Ziegler, copiado de `DC_DIV_QR_THRESHOLD` de GMP. Lo medido lo pone
// veinte veces mas arriba, y la razon es que GMP compara contra SU `mpn_mul`,
// con ensamblador afinado y Toom-4; aqui el rival es la multiplicacion de esta
// casa, cuyo exponente medido es 1,68.
//
// AL MEDIR ESTO: CONSUMIR EL RESULTADO ENTERO
// -------------------------------------------
// La primera version hacia `doNotOptimize(r[0])`, que consume UN limbo del
// producto y deja al compilador eliminar el calculo de los otros 2H-1. El
// exponente de la multiplicacion salia 1,23 -- imposible, mejor que Karatsuba --
// y la division parecia estar ya en el techo en todo el rango. Es el
// espantapajaros de siempre, el mismo que ya fabrico un 6,23x falso en el banco
// de Karatsuba.
#include "algorithms/div_kernels.hpp"
#include "algorithms/mul_kernels.hpp"
#include "bench_adaptativo.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <tuple>

using namespace nstd::algorithms;

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

    constexpr std::size_t OPER = 4;

    /// @tparam N anchura de la division. La multiplicacion se mide a N/2 limbos,
    ///         que es el tamano de los factores en los que B-Z descompondria.
    template <std::size_t N>
    void mide()
    {
        constexpr std::size_t H = N / 2;
        static std::array<std::array<std::uint64_t, N>, OPER> a{}, b{};
        static std::array<std::array<std::uint64_t, H>, OPER> p{}, q_{};
        static std::array<std::uint64_t, N> qq{}, rr{};

        xorshift rng{0xBEEFULL + N};
        for (std::size_t i = 0; i < OPER; ++i)
        {
            for (std::size_t k = 0; k < N; ++k)
            {
                a[i][k] = rng();
                b[i][k] = (k < H) ? rng() : 0;
            }
            if (b[i][H - 1] == 0)
                b[i][H - 1] = 1;
            for (std::size_t k = 0; k < H; ++k)
            {
                p[i][k] = rng();
                q_[i][k] = rng();
            }
        }

        // CONSUMIR EL RESULTADO ENTERO. Con `doNotOptimize(r[0])` el compilador
        // puede eliminar el calculo de los otros 2H-1 limbos del producto, y la
        // multiplicacion sale gratis: el exponente medido bajaba a 1,23, que es
        // IMPOSIBLE --mejor que Karatsuba--. Es el espantapajaros de siempre.
        auto div = [&](std::size_t i)
        {
            div_knuth_d<N>(a[i % OPER], b[i % OPER], qq, rr);
            std::uint64_t acc = 0;
            for (std::size_t k = 0; k < N; ++k)
                acc ^= qq[k] ^ rr[k];
            doNotOptimize(acc);
        };
        auto mul = [&](std::size_t i)
        {
            auto r = kmul_full_gen<H, 26>(p[i % OPER], q_[i % OPER]);
            std::uint64_t acc = 0;
            for (std::size_t k = 0; k < 2 * H; ++k)
                acc ^= r[k];
            doNotOptimize(acc);
        };

        const auto m = bench::mide_entrelazado(std::make_tuple(div, mul), 20);
        const double razon = m[0].minimo / m[1].minimo;
        std::printf("| %5zu | %5zu | %10.0f | %10.0f | %7.1fx | %s |\n", N, H, m[0].minimo, m[1].minimo,
                    razon,
                    razon > 4.0 ? "HAY SITIO" : (razon > 3.0 ? "en el techo" : "por debajo del techo"));
        std::fflush(stdout);
    }
} // namespace

int main()
{
    print_header("cuanto hay sobre la mesa para Burnikel-Ziegler");
    std::printf("\nDivision (N, n=N/2) contra multiplicacion completa de N/2 x N/2.\n"
                "El techo publicado de GMP es 2x-4x. Entrelazadas, 20 repeticiones.\n\n");
    std::printf("|     N |   N/2 |   div N/2 |    mul N/2 |   razon | veredicto |\n");
    std::printf("|------:|------:|----------:|-----------:|--------:|:----------|\n");
    mide<8>();
    mide<16>();
    mide<32>();
    mide<64>();
    mide<128>();
    mide<256>();
    mide<512>();
    mide<1024>();
    mide<2048>();
    print_footer();
    return 0;
}
