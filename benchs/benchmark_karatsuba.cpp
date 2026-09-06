// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julian Calderon Almendros
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
// @file       benchmark_karatsuba.cpp
// @brief      Karatsuba frente a la multiplicacion escolar, para N=2,3,4,8,16
// @date       2026-08-25
// =============================================================================
//
// `fixed_int_t::operator*` toma el camino de Karatsuba para N=4 y N=8, y el
// escolar O(N^2) para el resto. Karatsuba era una de las optimizaciones de
// cabecera de v1.90 y nunca se habia medido: este benchmark existe para eso.
//
// Metodo: las dos variantes se miden INTERCALADAS dentro de cada ronda, y se
// toma el MINIMO de las rondas para cada caso. Intercalar reparte por igual la
// deriva termica y el ruido del planificador; el minimo se queda con la ronda
// menos contaminada, que es la que mas se parece al coste real. Una media
// mediria sobre todo el ruido del sistema.
//
// EL CONTROL ES N=16. Ahi la biblioteca usa el mismo bucle escolar que la
// implementacion de referencia de abajo, asi que la razon TIENE que salir
// ~1.00x. Si no sale, la referencia no es fiel y ninguna otra cifra vale.
//
// (La primera version de este benchmark uso una propagacion de acarreo
// portable, con un `while` y un salto dependiente de los datos, en vez de los
// intrinsecos de la biblioteca. El control salio 2.00x en N=16 y 6.23x en N=2:
// no se estaba midiendo Karatsuba contra el metodo escolar, sino la biblioteca
// contra un espantapajaros. De ahi que el control este aqui.)
// =============================================================================

#include "../include/fixed_width_int_t.hpp"
#include "../include/intrinsics/arithmetic_operations.hpp"
#include "bench_common.hpp"

#include <string>
#include <vector>

using namespace nstd;

// ============================================================================
// Multiplicacion escolar O(N^2), el camino que Karatsuba pretende batir
// ============================================================================
//
// HAY DOS REFERENCIAS, Y LA RAZON ES EL FALLO QUE ESTO DESTAPO.
//
// La primera, `schoolbook_mul`, es copia fiel del bucle general de
// `fixed_int_t::operator*`, con sus mismas primitivas. Fiel EN EL FUENTE. Pero
// medido el 6 sep 2026 sobre el ensamblador que emite GCC 16.2 en N=4:
//
//     Karatsuba (mul_sin_marca)   119 instrucciones,  9 `mul`  -> DESENROLLADO
//     escolar   (este de abajo)    61 instrucciones,  1 `mul`  -> BUCLE
//
// Un `mul` ejecutado diez veces contra nueve `mul` en linea recta. Eso no
// compara algoritmos: compara desenrollado. Y explica la mayor parte del
// "1,65x" que se venia publicando -- con `-funroll-loops`, la razon en N=4
// cae de 1,75x a 0,88x, o sea Karatsuba PIERDE.
//
// Es la segunda vez que esta comparacion mide algo que no es. La primera fue el
// "6,23x" contra un espantapajaros, que se arreglo poniendo esta copia fiel. La
// leccion: fiel en el fuente no es equivalente en el binario.
//
// Por eso la segunda referencia, `schoolbook_mul_desenrollado`: el MISMO
// algoritmo y las MISMAS primitivas, pero desenrollado por construccion --
// recursion de plantilla con indices de compilacion--, igual que lo esta el
// Karatsuba de la biblioteca. Contra esa es contra la que hay que comparar.
//
// Las dos se miden y se publican las dos razones. La diferencia entre ellas ES
// la aportacion del desenrollado, separada de la del algoritmo.

namespace ref
{
    inline unsigned char add_limb(std::uint64_t &limb, std::uint64_t v) noexcept
    {
        return intrinsics::addcarry_u64(0, limb, v, &limb);
    }

    inline unsigned char add_limb_carry(std::uint64_t &limb, std::uint64_t v, unsigned char c) noexcept
    {
        return intrinsics::addcarry_u64(c, limb, v, &limb);
    }
} // namespace ref

template <std::size_t N>
[[nodiscard]] uint_fixed_t<N> schoolbook_mul(const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) noexcept
{
    uint_fixed_t<N> out{};
    auto &r = out.limbs_ref();
    const auto &x = a.limbs();
    const auto &y = b.limbs();

    for (std::size_t i{0}; i < N; ++i)
    {
        for (std::size_t j{0}; i + j < N; ++j)
        {
            std::uint64_t hi{0};
            const std::uint64_t lo = intrinsics::umul128(x[i], y[j], &hi);
            unsigned char c = ref::add_limb(r[i + j], lo);
            const std::size_t next = i + j + 1;
            if (next < N)
            {
                c = ref::add_limb_carry(r[next], hi, c);
                for (std::size_t k{next + 1}; k < N && c; ++k)
                    c = ref::add_limb(r[k], std::uint64_t{c});
            }
        }
    }
    return out;
}

// ----------------------------------------------------------------------------
// La misma, desenrollada por construccion
// ----------------------------------------------------------------------------
//
// Identica en aritmetica y en primitivas a la de arriba. Lo unico que cambia es
// que los indices son de compilacion, asi que no depende de que el compilador
// se anime a desenrollar: sale en linea recta en los cuatro.

namespace ref
{
    /// Propaga el acarreo desde el limbo K en adelante. Conserva la salida
    /// temprana del bucle original (`&& c`), que aqui es un `if`.
    template <std::size_t N, std::size_t K>
    inline void propaga(std::array<std::uint64_t, N> &r, unsigned char c) noexcept
    {
        if constexpr (K < N)
        {
            if (c)
                propaga<N, K + 1>(r, ref::add_limb(r[K], std::uint64_t{c}));
        }
    }

    /// Una fila del escolar: los productos x[I]*y[J] para J creciente.
    template <std::size_t N, std::size_t I, std::size_t J>
    inline void fila(std::array<std::uint64_t, N> &r, const std::array<std::uint64_t, N> &x,
                     const std::array<std::uint64_t, N> &y) noexcept
    {
        if constexpr (I + J < N)
        {
            std::uint64_t hi{0};
            const std::uint64_t lo = intrinsics::umul128(x[I], y[J], &hi);
            unsigned char c = ref::add_limb(r[I + J], lo);
            if constexpr (I + J + 1 < N)
            {
                c = ref::add_limb_carry(r[I + J + 1], hi, c);
                propaga<N, I + J + 2>(r, c);
            }
            fila<N, I, J + 1>(r, x, y);
        }
    }

    template <std::size_t N, std::size_t I>
    inline void filas(std::array<std::uint64_t, N> &r, const std::array<std::uint64_t, N> &x,
                      const std::array<std::uint64_t, N> &y) noexcept
    {
        if constexpr (I < N)
        {
            fila<N, I, 0>(r, x, y);
            filas<N, I + 1>(r, x, y);
        }
    }
} // namespace ref

template <std::size_t N>
[[nodiscard]] uint_fixed_t<N> schoolbook_mul_desenrollado(const uint_fixed_t<N> &a,
                                                          const uint_fixed_t<N> &b) noexcept
{
    uint_fixed_t<N> out{};
    ref::filas<N, 0>(out.limbs_ref(), a.limbs(), b.limbs());
    return out;
}

// ============================================================================
// Operandos
// ============================================================================
//
// xorshift64* con semilla fija: reproducible entre ejecuciones y entre
// compiladores, y sin depender de <random>, cuya distribucion no esta
// especificada de forma portable.

static std::uint64_t seed_state{0x9E3779B97F4A7C15ull};

static std::uint64_t next_u64() noexcept
{
    seed_state ^= seed_state >> 12;
    seed_state ^= seed_state << 25;
    seed_state ^= seed_state >> 27;
    return seed_state * 0x2545F4914F6CDD1Dull;
}

template <std::size_t N>
static std::vector<uint_fixed_t<N>> make_operands(std::size_t count)
{
    std::vector<uint_fixed_t<N>> v;
    v.reserve(count);
    for (std::size_t k{0}; k < count; ++k)
    {
        uint_fixed_t<N> x{};
        for (std::size_t i{0}; i < N; ++i)
            x.set_limb(i, next_u64());
        v.push_back(x);
    }
    return v;
}

// ============================================================================
// Medida
// ============================================================================

// ============================================================================
// Verosimilitud: el suelo fisico de la maquina
// ============================================================================
//
// Un `mul` de 64x64 no baja de ~1 ciclo de RENDIMIENTO en ningun x86-64 actual
// (la latencia es mayor, pero se solapa). El escolar truncado de N limbos hace
// N(N+1)/2 productos, asi que su coste NO PUEDE bajar de esa cuenta.
//
// PERO EL SUELO NO ES 1,0, Y LA RAZON IMPORTA. Lo que mide `CycleTimer` es
// RDTSC, que en los procesadores actuales es TSC invariante: cuenta a una
// frecuencia de referencia fija, no a la del nucleo. Con turbo, el nucleo va
// mas rapido que el TSC, asi que un ciclo real MIDE MENOS DE UN "ciclo" TSC.
// Esta escrito en la cabecera de bench_common.hpp y hay que tenerlo en cuenta
// aqui: poner el suelo en 1,0 haria saltar el aviso sobre medidas legitimas.
//
// Con una relacion turbo/base de hasta ~2x, un producto que cuesta 1 ciclo de
// nucleo puede medir 0,5. El suelo se pone en 0,35 para dejar margen de sobra:
// no pretende ser ajustado, pretende no dar falsos positivos y aun asi cazar lo
// que es imposible por goleada.
//
// Si una medida se salta ese suelo, no es que el codigo sea rapido: es que el
// compilador se ha llevado el trabajo. Medido el 6 sep 2026, Intel daba 2,49
// cyc/op en N=4 --0,25 ciclos por producto-- mientras que de N=5 en adelante
// daba 2,3 a 3,2, que si es creible. Sin este aviso, ese 2,49 se habria
// publicado como una razon de 0,09x.
//
// Es la tercera vez que esta comparacion mide algo que no es: primero el
// espantapajaros del 6,23x, luego el escolar en bucle contra el Karatsuba
// desenrollado, y ahora esto. La diferencia es que esto salta solo.
static constexpr double CICLOS_MINIMOS_POR_PRODUCTO{0.35};

/// @brief Avisa si una medida se ha saltado el suelo fisico.
/// @return true si la cifra es creible.
static bool verosimil(std::size_t N, const char *que, double cyc_op)
{
    // N=2 se queda fuera: la biblioteca toma ahi un camino especializado de 128
    // bits que no hace N(N+1)/2 productos, asi que el modelo no le aplica.
    if (N < 3)
        return true;
    const double productos = static_cast<double>(N * (N + 1) / 2);
    const double por_producto = cyc_op / productos;
    if (por_producto >= CICLOS_MINIMOS_POR_PRODUCTO)
        return true;
    std::cout << "  [OJO] N=" << N << ' ' << que << ": " << cyc_op << " cyc/op son " << por_producto
              << " ciclos por producto, y el suelo fisico es " << CICLOS_MINIMOS_POR_PRODUCTO << ".\n"
              << "        El compilador se ha llevado el trabajo. La cifra NO VALE.\n";
    return false;
}

static int g_medidas_descartadas{0};

static constexpr std::size_t OPERANDS{256};
static constexpr std::size_t ROUNDS{7};
static constexpr std::size_t ITERS{400000};

template <std::size_t N, typename F>
static double measure(const std::vector<uint_fixed_t<N>> &xs, F op)
{
    uint_fixed_t<N> sink{};

    // Calentamiento: se descarta.
    for (std::size_t k{0}; k < WARMUP; ++k)
    {
        sink = op(xs[k % OPERANDS], xs[(k + 1) % OPERANDS]);
        doNotOptimize(sink);
    }

    CycleTimer t;
    for (std::size_t k{0}; k < ITERS; ++k)
    {
        sink = op(xs[k % OPERANDS], xs[(k + 1) % OPERANDS]);
        doNotOptimize(sink);
    }
    return static_cast<double>(t.elapsed_cycles()) / static_cast<double>(ITERS);
}

// `nota`: "" para los casos medidos, un texto para los que son control o
// camino especializado.
template <std::size_t N>
static void bench_one(const char *etiqueta, const char *nota)
{
    const auto xs = make_operands<N>(OPERANDS);

    double mejor_k{1e300};
    double mejor_e{1e300};
    double mejor_d{1e300}; // escolar desenrollado por construccion

    for (std::size_t r{0}; r < ROUNDS; ++r)
    {
        // Intercaladas dentro de la ronda: el ruido cae por igual en las tres.
        const double ck =
            measure<N>(xs, [](const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) { return a * b; });
        const double ce = measure<N>(xs, [](const uint_fixed_t<N> &a, const uint_fixed_t<N> &b)
                                     { return schoolbook_mul<N>(a, b); });
        const double cd = measure<N>(xs, [](const uint_fixed_t<N> &a, const uint_fixed_t<N> &b)
                                     { return schoolbook_mul_desenrollado<N>(a, b); });
        if (ck < mejor_k)
            mejor_k = ck;
        if (ce < mejor_e)
            mejor_e = ce;
        if (cd < mejor_d)
            mejor_d = cd;
    }

    // Antes de registrar nada, comprobar que las tres cifras son fisicamente
    // posibles. Una medida imposible contamina el historico y, peor, se compara
    // con las de manana como si valiera.
    const bool ok_k = verosimil(N, "biblioteca", mejor_k);
    const bool ok_e = verosimil(N, "escolar", mejor_e);
    const bool ok_d = verosimil(N, "escolar desenrollado", mejor_d);
    if (!(ok_k && ok_e && ok_d))
        ++g_medidas_descartadas;

    bench_record((std::string("N=") + std::to_string(N) + " biblioteca").c_str(), mejor_k);
    bench_record((std::string("N=") + std::to_string(N) + " escolar").c_str(), mejor_e);
    bench_record((std::string("N=") + std::to_string(N) + " escolar desenrollado").c_str(), mejor_d);
    // `razon` se conserva con el mismo nombre para no romper el historico ya
    // guardado, PERO es la que enganaba: compara contra el escolar en bucle.
    bench_record((std::string("N=") + std::to_string(N) + " razon").c_str(), mejor_e / mejor_k, "x");
    // Esta es la buena: los dos lados desenrollados por construccion.
    bench_record((std::string("N=") + std::to_string(N) + " razon justa").c_str(), mejor_d / mejor_k, "x");
    // Y esta separa lo que aporta el desenrollado, que era lo que se colaba
    // dentro de la razon de arriba.
    bench_record((std::string("N=") + std::to_string(N) + " aporte del desenrollado").c_str(),
                 mejor_e / mejor_d, "x");

    std::cout << "| " << std::left << std::setw(29) << etiqueta << " | " << std::right << std::fixed
              << std::setprecision(2) << std::setw(12) << mejor_k << " | " << std::setw(6)
              << (mejor_d / mejor_k) << "x   |";
    if (nota[0] != '\0')
        std::cout << "   <- " << nota;
    std::cout << "\n";

    std::cout << "| " << std::left << std::setw(29) << "   escolar O(N^2)" << " | " << std::right
              << std::fixed << std::setprecision(2) << std::setw(12) << mejor_e << " |        "
              << " |\n";
}

// ============================================================================
// Correccion antes que velocidad
// ============================================================================
//
// Un benchmark de dos implementaciones que no calculan lo mismo no mide nada.

template <std::size_t N>
static bool check_equal()
{
    const auto xs = make_operands<N>(64);
    for (std::size_t i{0}; i < xs.size(); ++i)
        for (std::size_t j{0}; j < xs.size(); ++j)
            if (xs[i] * xs[j] != schoolbook_mul<N>(xs[i], xs[j]))
                return false;
    return true;
}

int main()
{
    std::cout << "\n=== Karatsuba frente a multiplicacion escolar ===\n";
    std::cout << "operandos: " << OPERANDS << " pseudoaleatorios, " << ITERS << " iteraciones x " << ROUNDS
              << " rondas intercaladas, minimo por caso\n";

    std::cout << "\n[correccion]\n";
    const bool ok = check_equal<2>() && check_equal<3>() && check_equal<4>() && check_equal<5>() &&
                    check_equal<6>() && check_equal<7>() && check_equal<8>() && check_equal<9>() &&
                    check_equal<10>() && check_equal<12>() && check_equal<16>() && check_equal<32>();
    std::cout << "  las dos implementaciones coinciden en N=2,3,4,5,6,7,8,9,10,12,16,32: "
              << (ok ? "SI" : "NO") << "\n";
    if (!ok)
    {
        std::cout << "  ABORTADO: no tiene sentido medir dos cosas que no calculan lo mismo.\n";
        return 1;
    }

    print_header("multiplicacion, ciclos por operacion");
    std::cout << "|   razon = escolar / camino de la biblioteca;  >1.00x = la biblioteca gana\n";
    print_separator();
    bench_one<4>("N=4  (256 bits)", "Karatsuba");
    bench_one<8>("N=8  (512 bits)", "Karatsuba");
    bench_one<2>("N=2  (128 bits)", "camino especializado de 128 bits");
    print_footer();

    // ------------------------------------------------------------------
    // Barrido: la penalizacion de N=3, es de los impares o de los pequenos?
    // ------------------------------------------------------------------
    //
    // El control de este mismo benchmark destapo el 25 ago 2026 que en N=3 el
    // bucle escolar de la biblioteca es un 14 % MAS LENTO que una copia
    // identica suya escrita como funcion libre (razon 0,86x, estable). En N=16
    // la razon sale 1,02x, o sea que ahi no pasa.
    //
    // Ninguno de estos N usa Karatsuba --solo lo usan 4 y 8--, asi que en todos
    // deberia salir ~1.00x. Los impares y los pares se separan a proposito: si
    // el efecto sigue la paridad, apunta al bucle de acarreo; si sigue al
    // tamano, a la generacion de codigo del `if constexpr` encadenado.
    std::cout << "\n";
    print_header("barrido: N que NO usan Karatsuba (todos deberian dar ~1.00x)");
    print_separator();
    std::cout << "|   impares\n";
    bench_one<3>("N=3  (192 bits)", "");
    bench_one<5>("N=5  (320 bits)", "");
    bench_one<7>("N=7  (448 bits)", "");
    bench_one<9>("N=9  (576 bits)", "");
    print_separator();
    std::cout << "|   pares que tampoco usan Karatsuba\n";
    bench_one<6>("N=6  (384 bits)", "");
    bench_one<10>("N=10 (640 bits)", "");
    bench_one<12>("N=12 (768 bits)", "");
    bench_one<16>("N=16 (1024 bits)", "");
    bench_one<32>("N=32 (2048 bits)", "");
    print_footer();

    std::cout << "\n"
              << "N=16 y N=32 son las anchuras donde la teoria dice que Karatsuba\n"
              << "deberia empezar a ganar: cambia N^2 por N^1.585, pero arrastra una\n"
              << "constante grande. Con NSTD_KARATSUBA_MAX=8 (el defecto) `biblioteca`\n"
              << "es el escolar en bucle; con =32 es Karatsuba. Comparar las dos\n"
              << "construcciones da el efecto del algoritmo, con el escolar\n"
              << "desenrollado de testigo, que no depende de la macro.\n";

    std::cout << "\nSi los N de este barrido no salen entre 0.95x y 1.05x, hay algo que\n"
              << "explicar: la implementacion de referencia es la misma en todos.\n";

    // Una medida imposible no es un detalle: si se cuela, contamina el historico
    // y manana se compara con ella como si valiera. Se sale con error.
    if (g_medidas_descartadas > 0)
    {
        std::cout << g_medidas_descartadas
                  << " anchura(s) con medidas por debajo del suelo fisico."
                     " Ver los [OJO] de arriba: esas cifras NO se pueden usar.";
        std::cout << std::endl;
        return 1;
    }

    return 0;
}
