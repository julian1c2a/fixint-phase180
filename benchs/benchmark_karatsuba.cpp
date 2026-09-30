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
// Karatsuba era una de las optimizaciones de cabecera de v1.90 y nunca se habia
// medido: este benchmark existe para eso.
//
// EL REPARTO DE `operator*`, leido de los umbrales y no de memoria:
//
//     N == 2                         especializado de 128 bits
//     N = 3 .. NSTD_DESENROLLA_MAX   escolar DESENROLLADO por construccion
//     N >= NSTD_KARATSUBA_MIN        Karatsuba equilibrado, CUALQUIER N
//
// Hoy esos umbrales son 21 y 22 --son «la misma frontera», lo dice su `@def`--,
// asi que de todo lo que mide este fichero **el unico que usa Karatsuba es
// N=32**.
//
// (Hasta el 30 sep 2026 aqui ponia «toma el camino de Karatsuba para N=4 y N=8».
// Eso fue cierto con unos umbrales de 4 y 8 que cambiaron el 16 sep, y la tabla
// siguio imprimiendo «<- Karatsuba» junto a N=4 y N=8 en cada ejecucion. El
// header ya se habia corregido --hay alli un comentario del 18 sep que llama a
// su propia version anterior «tres afirmaciones falsas en cinco lineas»-- y el
// benchmark se quedo atras. Por eso las etiquetas ya no se escriben a mano: las
// deduce `regimen()` de los umbrales.)
//
// Metodo: las tres variantes se miden ENTRELAZADAS y CON EL ORDEN ROTANDO, en
// vueltas de tiempo fijo, y se toma el MINIMO. Entrelazar reparte por igual la
// deriva termica y el ruido del planificador; rotar el orden quita el sesgo de
// ir siempre la primera --que era el resto que quedaba--; y el minimo se queda
// con la vuelta menos contaminada, que es la que mas se parece al coste real.
// Una media mediria sobre todo el ruido del sistema.
//
// Desde P2.17 lo hace `bench::mide_entrelazado`, que ademas guarda el ruido de
// cada casilla --dispersion, cola baja, vueltas limpias-- para que `--compare`
// pueda decidir si una diferencia de manana significa algo. Antes eran 400.000
// iteraciones fijas para todo: en N=2 la ventana duraba 4 ms y en N=32 casi un
// segundo.
//
// EL CONTROL ES EL COSTE POR PRODUCTO DE LIMBO. El escolar truncado de N limbos
// hace N(N+1)/2 productos, asi que `cyc_op / productos(N)` es lo que cuesta UNO,
// y esa cifra si deberia ser plana mientras no cambie el regimen de generacion
// de codigo. Si se mueve, hay algo que nombrar. Lo calcula `control::informa()`.
//
// (ANTES EL CONTROL ERA N=16, con el argumento de que alli la biblioteca usa el
// mismo bucle escolar que la referencia y por tanto la razon TENIA que salir
// ~1,00x. Resulto ser justo la anchura donde no sale: en N=16 la biblioteca
// cuesta 4,15 ciclos por producto y hasta N=12 costaba 1,7. El argumento era
// bueno en el fuente y falso en el binario -- la misma leccion, otra vez.)
//
// (La primera version de este benchmark uso una propagacion de acarreo
// portable, con un `while` y un salto dependiente de los datos, en vez de los
// intrinsecos de la biblioteca. El control salio 2.00x en N=16 y 6.23x en N=2:
// no se estaba midiendo Karatsuba contra el metodo escolar, sino la biblioteca
// contra un espantapajaros. De ahi que el control este aqui.)
// =============================================================================

#include "../include/fixed_width_int_t.hpp"
#include "../include/intrinsics/arithmetic_operations.hpp"
#include "bench_adaptativo.hpp"

#include <algorithm> // std::sort, que usa el control de escalado
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
// compara algoritmos: compara desenrollado, y explica la mayor parte del "1,65x"
// que se venia publicando.
//
// Es la segunda vez que esta comparacion mide algo que no es. La primera fue el
// "6,23x" contra un espantapajaros, que se arreglo poniendo esta copia fiel. La
// leccion: fiel en el fuente no es equivalente en el binario.
//
// LA CIFRA CON LA QUE SE CERRO AQUEL HALLAZGO ERA A SU VEZ OTRO ARTEFACTO.
//
// Aquel 6 sep se concluyo que «con `-funroll-loops` la razon en N=4 cae de 1,75x
// a 0,88x, o sea Karatsuba PIERDE». Medido el 30 sep 2026 al migrar a este
// arnes, ese 0,88x no vale: el arnes viejo media las tres variantes en ORDEN
// FIJO dentro de cada ronda, con la biblioteca SIEMPRE LA PRIMERA. Ir primero se
// paga --cachés y predictor frios--, asi que `mejor_k` salia inflado y la razon
// `d/k` deflactada. Rotando el orden, SEIS anchuras cruzan el 1,00x:
//
//     N        arnes viejo   arnes nuevo
//     4            0,880        1,000
//     5            0,940        1,120
//     6            0,930        1,220
//     8            0,830        1,110
//     10           0,900        1,350
//     12           0,930        1,310
//
// No es deriva de la maquina: el arnes viejo, sacado de git y corrido ESE MISMO
// DIA, reproduce el historico del 27 sep dentro de 0,02 --0,878 a 0,880 en N=4,
// 1,776 a 1,800 en N=32--. Lo que cambia es el arnes.
//
// Asi que la lectura buena, hoy, es: en las anchuras que NO usan Karatsuba la
// biblioteca va por delante del escolar desenrollado, no por detras. Lo cual no
// dice nada a favor de Karatsuba --ahi no se usa--, sino que el bucle escolar de
// la biblioteca esta mejor generado de lo que se creia.
//
// Lo que aquel hallazgo DESCUBRIO sigue intacto: el desenrollado se colaba dentro
// de la razon, y por eso existe `schoolbook_mul_desenrollado`. Lo que CONCLUYO se
// apoyaba en numeros de orden fijo.
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
//
// (Y hubo una cuarta, el 30 sep: el orden fijo de las variantes. Esa tampoco
// saltaba sola -- hizo falta correr los dos arneses el mismo minuto. De las
// cuatro, solo esta de aqui avisa por su cuenta, que es el argumento para poner
// mas comprobaciones como ella y no mas parrafos como este.)
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

// ============================================================================
// Coste teorico declarado, y su distancia con lo medido
// ============================================================================
//
// Cuarta pieza del desguace (P2.7). Cada algoritmo declara SU CUENTA de
// productos de limbo, y el benchmark publica la razon ESPERADA al lado de la
// MEDIDA. La distancia entre las dos es el resultado interesante: ahi viven el
// desenrollado, la presion de registros, la planificacion y los fallos de
// medida.
//
// POR QUE. La razon medida decia 1,65x a favor de Karatsuba en N=4. La cuenta
// teorica dice 10/9 = 1,11x. Un 1,65x medido contra un 1,11x esperado es medio
// factor sin explicar, y ESO era la senal -- pero nadie la calculaba, asi que
// el 1,65x se publico durante meses. Sale de una resta.
//
// Que la distancia no sea 1,00x no significa que la medida este mal: significa
// que hay algo que la cuenta de multiplicaciones no captura, y que hay que
// nombrarlo antes de publicar. En N=4 resulto ser el desenrollado.

/// @brief Productos de limbo del escolar truncado de N limbos.
static constexpr double productos_escolar(std::size_t N) noexcept
{
    return static_cast<double>(N * (N + 1) / 2);
}

/// @brief Productos de limbo de Karatsuba tal como estaba escrito EN 2026-09.
///
/// @warning **Este modelo ya no corresponde a la implementacion.** Cuenta el
///          reparto por potencias de dos --`3^log2(N/2)` mas dos terminos del
///          medio-- y la biblioteca usa `mul_karatsuba_equilibrado` desde el
///          16 sep 2026, que reparte para cualquier N. Lo que sale de aqui
///          alimenta `razon esperada` y `sin explicar`, asi que esas dos
///          columnas NO se pueden leer hoy para N >= NSTD_KARATSUBA_MIN.
///          Anotado en NEXT_STEPS; rehacerlo pide derivar la cuenta del reparto
///          equilibrado, que no es un ajuste de una linea.
static constexpr double productos_karatsuba(std::size_t N) noexcept
{
    if (N <= 1)
        return 1.0;
    double kf = 1.0;
    for (std::size_t m = N / 2; m > 1; m /= 2)
        kf *= 3.0;
    return kf + 2.0 * productos_escolar(N / 2);
}

static int g_medidas_descartadas{0};

// ============================================================================
// EL CONTROL: coste por producto de limbo
// ============================================================================
//
// QUE SUSTITUYE, Y POR QUE HABIA QUE SUSTITUIRLO. Hasta el 30 sep 2026 el
// control era una banda: «si los N de este barrido no salen entre 0,95x y 1,05x,
// hay algo que explicar». Pedia un +-5 % sobre una cantidad cuyo recorrido real
// es del 134 % --de 0,759x a 1,776x--, asi que llevaba meses incumplida y el
// aviso se leia por encima.
//
// No estaba mal calibrada: pedia que fuera constante algo que NO PUEDE serlo.
// Los dos lados de esa razon son un bucle y una version desenrollada por
// construccion, y desenrollar 528 productos en linea recta (N=32) no cuesta lo
// mismo por producto que desenrollar 6 (N=3).
//
// LO QUE SI DEBERIA SER PLANO. El fichero ya tenia su modelo de coste
// --`productos_escolar`-- y no lo usaba para controlar nada. Medido sobre el
// historico del 27 sep, ciclos por producto de limbo:
//
//     N        biblioteca   escolar   desenrollado
//     3           1,559       4,046      1,595
//     7           1,551       5,403      1,565
//     12          1,729       7,951      1,559
//     16          4,152       8,877      3,150   <- x2,40
//     32          5,171       9,530      9,184
//
// El bucle escolar escala liso de punta a punta. La biblioteca y el desenrollado
// van a la par hasta N=12 y caen por un escalon en N=16, que es donde el
// compilador deja de desenrollar; en N=32 el desenrollado ya cuesta por producto
// lo mismo que el bucle (9,18 contra 9,53), o sea que desenrollar ha dejado de
// pagar del todo.
//
// ESTO NO ABORTA, Y ES DELIBERADO. El escalon es una propiedad reproducible de
// la biblioteca, no un fallo de medida: un control que lo convirtiera en error
// estaria rojo siempre, y a la semana nadie lo miraria. Lo que hace es NOMBRAR
// donde esta y cuanto vale, que es lo que la banda pretendia y no lograba.
// Abortar se reserva para lo que es imposible, que es `verosimil`.

/// @brief ¿Toma `operator*` el camino de Karatsuba para esta anchura?
///
/// OJO CON LA POTENCIA DE DOS. Esta condicion llevaba un `(N & (N - 1)) == 0`
/// copiado de cuando Karatsuba exigia que N fuera potencia de dos. Ya no: el
/// reparto EQUILIBRADO entro el 16 sep 2026 y vale para cualquier N -- esa
/// exigencia era justamente la causa del acantilado que se quito. Sobre las
/// anchuras que mide este fichero daba la respuesta correcta por casualidad
/// (ninguna que no sea potencia de dos llega a 22), pero habria mentido en
/// cuanto se midiera N=24.
[[nodiscard]] static constexpr bool usa_karatsuba(std::size_t N) noexcept
{
    return N >= NSTD_KARATSUBA_MIN && N <= NSTD_KARATSUBA_MAX;
}

/// @brief Por donde va `a * b` para esta anchura, en una palabra.
///
/// SE DEDUCE, NO SE ESCRIBE A MANO. Las etiquetas de la tabla estuvieron
/// escritas a mano y se quedaron dos semanas anunciando «Karatsuba» junto a N=4
/// y N=8, que van por el escolar desenrollado desde que los umbrales se movieron.
/// Deducirlas de las macros cuesta lo mismo y no envejece.
[[nodiscard]] static const char *regimen(std::size_t N) noexcept
{
    if (N == 2)
        return "camino especializado de 128 bits";
    if (usa_karatsuba(N))
        return "Karatsuba equilibrado";
    if (N <= NSTD_DESENROLLA_MAX)
        return "escolar desenrollado";
    return "escolar en bucle";
}

namespace control
{
    struct Casilla
    {
        std::size_t N;
        double biblioteca;
        double escolar;
        double desenrollado;
    };

    static std::vector<Casilla> casillas;

    // CUANTO PUEDE SALTAR EL COSTE POR PRODUCTO ENTRE DOS N CONSECUTIVOS SIN QUE
    // sea un cambio de regimen. CALIBRADO, no elegido: en el tramo liso del
    // historico del 27 sep (N=3..12) el mayor salto consecutivo de la biblioteca
    // es x1,06, y el escalon de N=12 a N=16 vale x2,40. Un 1,5 deja ~40 % de
    // margen por los dos lados. No se copia de ningun sitio: sale de esta
    // maquina y este compilador, y si alguna vez se muda, se vuelve a medir.
    static constexpr double SALTO_DE_REGIMEN{1.5};

    /// @brief Ciclos por producto de limbo: la cifra que si deberia ser plana.
    [[nodiscard]] static double por_producto(double cyc, std::size_t N) noexcept
    {
        return cyc / productos_escolar(N);
    }

    static void anota(std::size_t N, double k, double e, double d)
    {
        casillas.push_back(Casilla{N, k, e, d});
    }

    static void informa()
    {
        // Solo los que NO usan Karatsuba: ahi las tres columnas hacen la misma
        // cuenta de productos y son comparables. Y sin N=2, que toma un camino
        // especializado de 128 bits que no hace N(N+1)/2 productos -- la misma
        // exclusion que `verosimil`.
        std::vector<Casilla> serie;
        for (const auto &c : casillas)
            if (c.N >= 3 && !usa_karatsuba(c.N))
                serie.push_back(c);
        std::sort(serie.begin(), serie.end(), [](const Casilla &a, const Casilla &b) { return a.N < b.N; });
        if (serie.size() < 2)
            return;

        std::cout << "\n[control] ciclos por PRODUCTO DE LIMBO, que es lo que deberia ser plano\n";
        std::cout << "+------+------------+------------+--------------+---------+\n";
        std::cout << "|   N  | biblioteca |  escolar   | desenrollado |  salto  |\n";
        std::cout << "+------+------------+------------+--------------+---------+\n";

        std::size_t escalones{0};
        double previo{0.0};
        std::size_t previo_n{0};
        for (const auto &c : serie)
        {
            const double pk = por_producto(c.biblioteca, c.N);
            std::cout << "| " << std::right << std::setw(4) << c.N << " | " << std::fixed
                      << std::setprecision(3) << std::setw(10) << pk << " | " << std::setw(10)
                      << por_producto(c.escolar, c.N) << " | " << std::setw(12)
                      << por_producto(c.desenrollado, c.N) << " | ";
            if (previo > 0.0)
            {
                const double salto = pk / previo;
                std::cout << std::setw(6) << std::setprecision(2) << salto << "x |";
                if (salto >= SALTO_DE_REGIMEN || salto <= 1.0 / SALTO_DE_REGIMEN)
                {
                    std::cout << "  <- CAMBIO DE REGIMEN (N=" << previo_n << " -> " << c.N << ")";
                    ++escalones;
                }
            }
            else
            {
                std::cout << "      -- |";
            }
            std::cout << "\n";
            previo = pk;
            previo_n = c.N;
        }
        std::cout << "+------+------------+------------+--------------+---------+\n";

        if (escalones == 0)
        {
            std::cout << "El coste por producto es plano en todo el barrido: una sola forma de\n"
                         "generar el codigo, y las razones de arriba se pueden comparar entre si.\n";
        }
        else
        {
            std::cout << escalones
                      << " cambio(s) de regimen. NO es un fallo de medida, y NO es el despacho:\n"
                         "el reparto de `operator*` no cambia hasta N="
                      << (NSTD_DESENROLLA_MAX + 1)
                      << ". Lo que cambia es el CODIGO\n"
                         "GENERADO -- desenrollar N(N+1)/2 productos deja de caber--, y se ve en que\n"
                         "la referencia desenrollada salta igual mientras el bucle sigue liso.\n"
                         "Las razones a un lado y otro del escalon no son comparables entre si.\n";
        }
    }
} // namespace control

static constexpr std::size_t OPERANDS{256};

// ROUNDS, ITERS y `measure` se fueron con el arnes viejo (P2.17). Fijaban
// 400.000 iteraciones para las doce anchuras: en N=2 eso es una ventana de 4 ms
// y en N=32 de casi un segundo, con el mismo calentamiento de WARMUP vueltas
// para las dos. `bench::mide_entrelazado` fija el TIEMPO (200 ms) y deduce las
// iteraciones, calienta segun la casilla y rota el orden de las variantes.

// `nota` ya no la pasa quien llama: la deduce `regimen(N)` de los umbrales. Se
// conserva el parametro para lo que NO se puede deducir -- por ahora nada, y por
// eso todas las llamadas pasan "".
template <std::size_t N>
static void bench_one(const char *etiqueta, const char *nota)
{
    const auto xs = make_operands<N>(OPERANDS);

    uint_fixed_t<N> sink{};

    auto f_biblioteca = [&](std::size_t k)
    {
        sink = xs[k % OPERANDS] * xs[(k + 1) % OPERANDS];
        doNotOptimize(sink);
    };
    auto f_escolar = [&](std::size_t k)
    {
        sink = schoolbook_mul<N>(xs[k % OPERANDS], xs[(k + 1) % OPERANDS]);
        doNotOptimize(sink);
    };
    auto f_desenrollado = [&](std::size_t k)
    {
        sink = schoolbook_mul_desenrollado<N>(xs[k % OPERANDS], xs[(k + 1) % OPERANDS]);
        doNotOptimize(sink);
    };

    // El orden de la tupla es el orden de `m`, no el orden en que se miden: eso
    // lo rota el arnes en cada vuelta, que es justo lo que hace comparables las
    // tres cifras de abajo.
    const auto m = bench::mide_entrelazado(std::make_tuple(f_biblioteca, f_escolar, f_desenrollado));

    const double mejor_k = m[0].minimo;
    const double mejor_e = m[1].minimo;
    const double mejor_d = m[2].minimo; // escolar desenrollado por construccion

    // Antes de registrar nada, comprobar que las tres cifras son fisicamente
    // posibles. Una medida imposible contamina el historico y, peor, se compara
    // con las de manana como si valiera.
    const bool ok_k = verosimil(N, "biblioteca", mejor_k);
    const bool ok_e = verosimil(N, "escolar", mejor_e);
    const bool ok_d = verosimil(N, "escolar desenrollado", mejor_d);
    if (!(ok_k && ok_e && ok_d))
        ++g_medidas_descartadas;

    control::anota(N, mejor_k, mejor_e, mejor_d);

    // Las tres medidas, con su ruido. Las razones de debajo NO: son cocientes de
    // estas, no medidas, y no tienen dispersion propia que guardar.
    bench::registra((std::string("N=") + std::to_string(N) + " biblioteca").c_str(), m[0]);
    bench::registra((std::string("N=") + std::to_string(N) + " escolar").c_str(), m[1]);
    bench::registra((std::string("N=") + std::to_string(N) + " escolar desenrollado").c_str(), m[2]);
    // `razon` se conserva con el mismo nombre para no romper el historico ya
    // guardado, PERO es la que enganaba: compara contra el escolar en bucle.
    bench_record((std::string("N=") + std::to_string(N) + " razon").c_str(), mejor_e / mejor_k, "x");
    // Esta es la buena: los dos lados desenrollados por construccion.
    bench_record((std::string("N=") + std::to_string(N) + " razon justa").c_str(), mejor_d / mejor_k, "x");

    // Y la que dice si hay algo sin explicar: lo que la cuenta de productos
    // predice, frente a lo que dice el cronometro. Si se separan, hay una
    // variable en juego que la cuenta no captura, y hay que nombrarla.
    {
        const double esperada = usa_karatsuba(N) ? productos_escolar(N) / productos_karatsuba(N) : 1.0;
        bench_record((std::string("N=") + std::to_string(N) + " razon esperada").c_str(), esperada, "x");
        bench_record((std::string("N=") + std::to_string(N) + " sin explicar").c_str(),
                     (mejor_d / mejor_k) / esperada, "x");
    }
    // Y esta separa lo que aporta el desenrollado, que era lo que se colaba
    // dentro de la razon de arriba.
    bench_record((std::string("N=") + std::to_string(N) + " aporte del desenrollado").c_str(),
                 mejor_e / mejor_d, "x");

    std::cout << "| " << std::left << std::setw(29) << etiqueta << " | " << std::right << std::fixed
              << std::setprecision(2) << std::setw(12) << mejor_k << " | " << std::setw(6)
              << (mejor_d / mejor_k) << "x   |";
    std::cout << "   <- " << regimen(N);
    if (nota[0] != '\0')
        std::cout << ", " << nota;
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
    std::cout << "operandos: " << OPERANDS << " pseudoaleatorios; " << bench::REPETICIONES << " vueltas de "
              << bench::MS_POR_CASILLA
              << " ms por casilla, las TRES variantes entrelazadas y con el orden rotando;"
                 " minimo por caso\n";

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

    print_header("las tres anchuras con camino propio (2) o historicas (4 y 8)");
    std::cout << "|   razon = escolar / camino de la biblioteca;  >1.00x = la biblioteca gana\n";
    print_separator();
    bench_one<4>("N=4  (256 bits)", "");
    bench_one<8>("N=8  (512 bits)", "");
    bench_one<2>("N=2  (128 bits)", "");
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
    print_header("barrido de anchuras; el unico que usa Karatsuba hoy es N=32");
    print_separator();
    std::cout << "|   impares\n";
    bench_one<3>("N=3  (192 bits)", "");
    bench_one<5>("N=5  (320 bits)", "");
    bench_one<7>("N=7  (448 bits)", "");
    bench_one<9>("N=9  (576 bits)", "");
    print_separator();
    std::cout << "|   pares\n";
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

    // AQUI ESTABA LA BANDA DE 0,95x-1,05x, retirada el 30 sep 2026.
    //
    // Decia: «si los N de este barrido no salen entre 0,95x y 1,05x, hay algo que
    // explicar, porque la implementacion de referencia es la misma en todos».
    // Pedia un +-5 % sobre una cantidad cuyo recorrido real es del 134 %, asi que
    // llevaba meses incumplida -- con el arnes viejo daba 0,76x en N=16 y 1,78x
    // en N=32, y con el nuevo da de 1,07x a 1,48x. Un aviso que nunca se cumple
    // se deja de leer, y este se leia por encima.
    //
    // El fallo no era el ancho de la banda: era pedir que fuera constante una
    // razon entre un bucle y una version desenrollada por construccion. Lo
    // sustituye `control::informa()`, que mira el coste por producto de limbo
    // --que si deberia ser plano-- y dice DONDE cambia el regimen en vez de
    // limitarse a decir que algo no cuadra.
    control::informa();

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
