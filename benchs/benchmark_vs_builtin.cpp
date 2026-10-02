// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Benchmark: los enteros de 128 bits de la biblioteca frente a los del
//            compilador y a los de Boost.Multiprecision
// License: BSL-1.0
// =============================================================================
//
// OJO, LOS #include VAN ARRIBA, antes de la explicacion larga: el guion de
// compilacion (`scripts/build_generic.py`) decide si enlaza GMP y TomMath
// buscando «boost/multiprecision» en los PRIMEROS 4000 caracteres del fichero.
// Con la explicacion delante, el enlazado fallaria sin decir por que.
//
// `int128_param_t` esta deprecado (ADR-006) y este fichero lo usa A PROPOSITO:
// mide el tipo que se retira junto al que lo sustituye. Se va en la 1.90.
#define NSTD_SILENCIA_INT128_PARAM_DEPRECADO
#define NSTD_QUIERO_INT128_PARAM

#include "fixed_width_int_t.hpp"
#include "int128_parameterized.hpp"

#include "bench_adaptativo.hpp"

#include <boost/multiprecision/cpp_int.hpp>
#if !defined(_MSC_VER) || defined(FORCE_GMP_TOMMATH)
#define BENCH_HAS_GMP_TOMMATH 1
#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/tommath.hpp>
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// =============================================================================
// LA REGLA (2 oct 2026): MISMA FUNCION, MISMOS VALORES; SOLO CAMBIA EL TIPO
// =============================================================================
//
// Una tabla que compara tipos solo dice algo si a todos se les mide LO MISMO:
// el mismo nucleo de medida, los mismos valores, y en las mismas condiciones.
// Si cambia cualquier otra cosa, la diferencia que sale en la tabla es de esa
// otra cosa y no del tipo.
//
// HASTA HOY ESTE BANCO NO LA CUMPLIA, y en tres sitios eso falseaba la tabla:
//
//  1. PRODUCTO. Nueve tipos median una CADENA (`a = a * b`: latencia) y tres --
//     `checked_uint128`, GMP y TomMath, las cifras marcadas `(*)`-- productos
//     sueltos (`auto r = a * b`: rendimiento). Y en GMP y TomMath ni eso: con
//     plantillas de expresion, `auto r = a * b` NO MULTIPLICA, guarda la
//     expresion sin evaluarla. Por eso TomMath «multiplicaba» en 0,72 ciclos,
//     mas deprisa que `uint64_t`: no se media ningun producto.
//  2. DIVISION. El bucle era `a = a / 12345 + 1`, y eso converge: a la quinta
//     vuelta `a` vale 1 y ya no se mueve. Las 10.000 de calentamiento lo
//     garantizaban, asi que el tramo cronometrado entero dividia 1 entre 12345.
//     Y en `uint64_t` el divisor era una constante, que GCC cambia por una
//     multiplicacion. De ahi salio, y se publico en PERFORMANCE.md, que
//     `nstd::uint128_t` dividia mas deprisa que el `uint64_t` nativo.
//  3. RESTA Y DESPLAZAMIENTO. GMP y TomMath restaban con un `if (a < 0) a +=
//     2^128` dentro del bucle, y `checked_uint128` desplazaba por una cantidad
//     variable donde los demas rotaban por 3. Otro algoritmo, otra cifra.
//
// Y LAS VARIANTES SE MEDIAN UNA DETRAS DE OTRA, en orden fijo y una sola vez,
// con lo que lo que hiciera la maquina en cada momento se lo llevaba un tipo
// concreto.
//
// LO QUE MIDE AHORA. Para cada operacion, UN SOLO NUCLEO para todos los tipos:
//
//     r = a[i] OP b[i];      i = k % 8, ocho parejas de operandos
//     escapa(r);             el resultado entero, a memoria
//
// - Es RENDIMIENTO: operaciones independientes, como en la comparacion desde
//   P2.19. Medir latencia pide realimentar el resultado, y eso no se puede hacer
//   igual en todos: un producto encadenado se desborda, y entonces
//   `checked_uint128` lanza, `__int128` incurre en comportamiento indefinido y
//   GMP crece sin limite. Habria que darle a cada uno un bucle distinto, que es
//   justo lo que se esta arreglando.
// - LOS VALORES SON LOS MISMOS en todos los tipos de 128 bits, como numeros, y
//   estan elegidos para que NINGUN resultado se salga de 127 bits: asi el tipo
//   con signo, el comprobado y los de precision arbitraria calculan exactamente
//   lo mismo que los modulares, sin envolver, sin lanzar y sin crecer. Por eso
//   ya no hacen falta las filas «[128]» de GMP y TomMath con mascara: aqui
//   nunca pasan de 128 bits. `uint64_t`, la base, recibe la misma receta
//   escalada a 63 bits.
// - ANTES DE MEDIR SE COMPRUEBA que todos los tipos dan los MISMOS resultados en
//   las ocho parejas de cada operacion. Si no, el programa se para: una tabla en
//   la que un tipo calcula otra cosa no mide lo que dice.
// - Las variantes de cada operacion se miden ENTRELAZADAS, con el orden al azar
//   en cada ronda (`bench::mide_entrelazado`), y con el arnes adaptativo:
//   25 vueltas, suelo y dispersion. Cada operacion es una ventana.
// - Y DESPUES SE COMPRUEBA QUE LAS CIFRAS SON POSIBLES: ver «Verosimilitud».
//
// LO QUE NO MIDE, y conviene saberlo al leer la tabla: la latencia (una
// operacion cuyo resultado hace falta YA cuesta mas que una de estas), los
// numeros negativos, ni los productos cuyos dos factores pasan de 64 bits -- un
// producto exacto que cabe en 127 bits tiene siempre un factor de un limbo.
//
// COSTE: diez operaciones x doce tipos x 25 vueltas x 200 ms, unos 11 minutos.
// =============================================================================

namespace bmp = boost::multiprecision;

using boost_cpp_u128 = bmp::uint128_t;
using boost_cpp_i128 = bmp::int128_t;
using boost_checked_u128 = bmp::checked_uint128_t;
#ifdef BENCH_HAS_GMP_TOMMATH
using boost_gmp_int = bmp::mpz_int;
using boost_tom_int = bmp::tom_int;
#endif

#ifdef __SIZEOF_INT128__
#define HAS_BUILTIN_INT128 1
// `__extension__` para que `-pedantic` no avise en cada uso.
__extension__ typedef unsigned __int128 u128_nativo;
__extension__ typedef __int128 i128_nativo;
#endif

// =============================================================================
// Los valores: numeros de hasta 127 bits, como pareja de limbos
// =============================================================================

/// @brief Un numero de hasta 128 bits, sin tipo: `alto * 2^64 + bajo`.
struct Valor
{
    std::uint64_t alto{0};
    std::uint64_t bajo{0};

    friend bool operator==(const Valor &, const Valor &) = default;
    friend bool operator<(const Valor &x, const Valor &y)
    {
        return x.alto != y.alto ? x.alto < y.alto : x.bajo < y.bajo;
    }
};

/// @brief Una pareja de operandos y, para los desplazamientos, la distancia.
struct Pareja
{
    Valor a;
    Valor b;
    unsigned s{0};
};

/// @brief Cuantas parejas entran en el ciclo. Ocho caben de sobra en la cache
///        y `k % 8` se compila a un `and`.
static constexpr std::size_t VALORES = 8;

enum class Seccion
{
    copia,
    suma,
    resta,
    producto,
    division_corta,
    division_larga,
    desplaza_izq,
    desplaza_der,
    o_exclusivo,
    menor,
};

/// @brief El nombre de la operacion en la tabla y en el historico.
static constexpr const char *nombre(Seccion s)
{
    switch (s)
    {
        case Seccion::copia:
            return "copia (=)";
        case Seccion::suma:
            return "suma (+)";
        case Seccion::resta:
            return "resta (-)";
        case Seccion::producto:
            return "producto (*)";
        case Seccion::division_corta:
            return "division (/), divisor corto";
        case Seccion::division_larga:
            return "division (/), divisor largo";
        case Seccion::desplaza_izq:
            return "desplazamiento (<<)";
        case Seccion::desplaza_der:
            return "desplazamiento (>>)";
        case Seccion::o_exclusivo:
            return "o exclusivo (^)";
        case Seccion::menor:
            return "comparacion (<)";
    }
    return "?";
}

/// @brief Un numero al azar de EXACTAMENTE `bits` bits: el de arriba a uno.
///        `bits` entre 1 y 127.
static Valor con_bits(std::uint64_t &estado, unsigned bits)
{
    Valor v{bench::siguiente_azar(estado), bench::siguiente_azar(estado)};
    if (bits <= 64)
    {
        v.alto = 0;
        if (bits < 64)
            v.bajo &= (std::uint64_t{1} << bits) - 1;
        v.bajo |= std::uint64_t{1} << (bits - 1);
    }
    else
    {
        const unsigned b = bits - 64;
        v.alto &= (std::uint64_t{1} << b) - 1;
        v.alto |= std::uint64_t{1} << (b - 1);
    }
    return v;
}

/// @brief `round(ancho * f)`, metido en `[1, ancho - 1]`.
static unsigned parte(unsigned ancho, double f)
{
    const auto n = static_cast<unsigned>(static_cast<double>(ancho) * f + 0.5);
    return n < 1 ? 1 : (n > ancho - 1 ? ancho - 1 : n);
}

/// @brief Las ocho parejas de una operacion, para `ancho` bits utiles: 127 en
///        los tipos de 128 bits, 63 en `uint64_t`.
///
/// Salen de un generador con semilla FIJA por operacion, asi que son las mismas
/// en todas las ejecuciones y en todos los compiladores. Y cada receta garantiza
/// que el resultado EXACTO cabe en `ancho` bits.
static std::array<Pareja, VALORES> parejas(Seccion sec, unsigned ancho)
{
    std::uint64_t e = 0x5EED0000ull + static_cast<std::uint64_t>(sec);
    const unsigned mitad = (ancho + 1) / 2; // 64 o 32: lo que cabe en «un limbo»
    // Fracciones con las que se reparten los bits: variadas, para no medir un
    // solo tamano, y con periodo 8, que el predictor de saltos aprende.
    constexpr std::array<double, VALORES> f{0.75, 0.5, 0.25, 0.6, 0.4, 0.85, 0.15, 0.5};
    constexpr std::array<double, VALORES> g{1.0, 0.9, 0.75, 0.6, 0.5, 0.35, 0.2, 0.1};
    constexpr std::array<double, VALORES> h{0.02, 0.2, 0.45, 0.5, 0.55, 0.7, 0.9, 0.98};

    std::array<Pareja, VALORES> p{};
    for (std::size_t k = 0; k < VALORES; ++k)
    {
        Pareja &q = p[k];
        const auto kk = static_cast<unsigned>(k);
        switch (sec)
        {
            case Seccion::copia:
                q.a = con_bits(e, ancho - kk % 3);
                q.b = q.a;
                break;
            case Seccion::suma: // los dos por debajo de 2^(ancho-1): la suma cabe
                q.a = con_bits(e, ancho - 1 - kk % 3);
                q.b = con_bits(e, ancho - 1 - (kk * 5) % 7);
                break;
            case Seccion::resta: // el mayor menos el menor: nunca negativo
            {
                const Valor x = con_bits(e, ancho - kk % 3);
                const Valor y = con_bits(e, ancho - (kk * 5) % 7);
                q.a = x < y ? y : x;
                q.b = x < y ? x : y;
                break;
            }
            case Seccion::producto: // bits(a) + bits(b) = ancho: el producto cabe
            {
                const unsigned ba = parte(ancho, f[k]);
                q.a = con_bits(e, ba);
                q.b = con_bits(e, ancho - ba);
                break;
            }
            case Seccion::division_corta: // divisor de media anchura o menos
                q.a = con_bits(e, ancho);
                q.b = con_bits(e, parte(mitad + 1, g[k]));
                break;
            case Seccion::division_larga: // divisor de mas de media anchura
                q.a = con_bits(e, ancho);
                q.b = con_bits(e, mitad + 1 + parte(ancho - 1 - mitad, g[k]));
                break;
            case Seccion::desplaza_izq: // a con sitio para desplazarse s bits
                q.s = parte(ancho, h[k]);
                q.a = con_bits(e, ancho - q.s);
                break;
            case Seccion::desplaza_der:
                q.s = parte(ancho, h[k]);
                q.a = con_bits(e, ancho);
                break;
            case Seccion::o_exclusivo:
                q.a = con_bits(e, ancho - kk % 2);
                q.b = con_bits(e, ancho - 1 - kk % 3);
                break;
            case Seccion::menor:
            {
                // La MITAD sale cierta, por construccion, como en P2.19. Y la mitad
                // de las parejas solo se distingue en la mitad baja, para que el
                // limbo alto empate y decida el bajo.
                const Valor x = con_bits(e, ancho - 1 - kk % 2);
                Valor y = con_bits(e, ancho - 1 - kk % 3);
                if (k % 4 < 2)
                {
                    const std::uint64_t mascara =
                        mitad >= 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << mitad) - 1;
                    std::uint64_t d = bench::siguiente_azar(e) & mascara;
                    y = Valor{x.alto, x.bajo ^ (d == 0 ? 1 : d)};
                }
                const bool cierta = (k % 2 == 0);
                q.a = (x < y) == cierta ? x : y;
                q.b = (x < y) == cierta ? y : x;
                // Y UNA PAREJA IGUAL, la ultima (de las falsas). Sin ella, un tipo
                // que hiciera `<=` en vez de `<` daba lo mismo en las ocho y pasaba
                // la comprobacion: lo destapo una averia hecha a proposito el 2 oct.
                if (k == VALORES - 1)
                    q.b = q.a;
                break;
            }
        }
    }
    return p;
}

// =============================================================================
// De un Valor a cada tipo, y vuelta
// =============================================================================

/// @brief Vale uno, pero el compilador no puede saberlo: sin esto, los
///        operandos son constantes de compilacion y no queda nada que medir.
///        Se lee al MONTAR los operandos, nunca dentro de la medida (P2.19).
static volatile std::uint64_t g_uno{1};

template <typename T>
static constexpr bool es_de_64 = std::is_same_v<T, std::uint64_t>;

template <typename T>
static T a_tipo(const Valor &v)
{
    const std::uint64_t uno = g_uno;
    if constexpr (es_de_64<T>)
        return static_cast<T>(v.bajo * uno);
    else
    {
        T x{v.alto * uno};
        x <<= 64u;
        x += T{v.bajo * uno};
        return x;
    }
}

template <typename T>
static Valor a_valor(const T &x)
{
    if constexpr (std::is_same_v<T, bool>)
        return Valor{0, x ? 1u : 0u};
    else if constexpr (es_de_64<T>)
        return Valor{0, x};
    else
    {
        const T alto = x >> 64u;
        const T bajo = x - (alto << 64u);
        return Valor{static_cast<std::uint64_t>(alto), static_cast<std::uint64_t>(bajo)};
    }
}

// =============================================================================
// EL NUCLEO: uno solo, para todos los tipos
// =============================================================================

/// @brief Los operandos y el resultado de un tipo en una operacion.
///
/// EN EL MISMO BLOQUE A PROPOSITO. `escapa(r)` le da al ensamblador la direccion
/// de `r` y le dice que puede leer cualquier memoria a la que se llegue; como los
/// operandos viven en el mismo bloque, el compilador los tiene que volver a leer
/// en cada vuelta. Sin eso, podria dejar los dieciseis de `uint64_t` en
/// registros y no los treinta y dos de un tipo de 128 bits, y la diferencia
/// saldria en la tabla como si fuera del tipo.
template <Seccion S, typename T>
struct Banco
{
    using R = std::conditional_t<S == Seccion::menor, bool, T>;
    std::array<T, VALORES> a{};
    std::array<T, VALORES> b{};
    std::array<unsigned, VALORES> s{};
    R r{};
};

/// @brief La operacion, la misma linea para todos los tipos.
template <Seccion S, typename T>
static inline void opera(Banco<S, T> &k, std::size_t i)
{
    const T &x = k.a[i];
    const T &y = k.b[i];
    if constexpr (S == Seccion::copia)
        k.r = x;
    else if constexpr (S == Seccion::suma)
        k.r = x + y;
    else if constexpr (S == Seccion::resta)
        k.r = x - y;
    else if constexpr (S == Seccion::producto)
        k.r = x * y;
    else if constexpr (S == Seccion::division_corta || S == Seccion::division_larga)
        k.r = x / y;
    else if constexpr (S == Seccion::desplaza_izq)
        k.r = x << k.s[i];
    else if constexpr (S == Seccion::desplaza_der)
        k.r = x >> k.s[i];
    else if constexpr (S == Seccion::o_exclusivo)
        k.r = x ^ y;
    else
        k.r = x < y;
}

template <Seccion S, typename T>
static std::unique_ptr<Banco<S, T>> prepara(unsigned ancho)
{
    auto k = std::make_unique<Banco<S, T>>();
    const auto p = parejas(S, ancho);
    for (std::size_t i = 0; i < VALORES; ++i)
    {
        k->a[i] = a_tipo<T>(p[i].a);
        k->b[i] = a_tipo<T>(p[i].b);
        k->s[i] = p[i].s;
    }
    return k;
}

/// @brief Lo que se cronometra: la operacion y la barrera, nada mas.
template <Seccion S, typename T>
static auto vuelta(Banco<S, T> *k)
{
    return [k](std::size_t n)
    {
        opera<S, T>(*k, n % VALORES);
        escapa(k->r);
    };
}

// =============================================================================
// Los tipos que se comparan
// =============================================================================

template <typename T>
struct Tipo
{
    using tipo = T;
    const char *nombre;
    unsigned ancho; ///< bits utiles: 63 en `uint64_t`, 127 en los de 128.
};

static auto tipos()
{
    return std::tuple_cat(
        std::make_tuple(Tipo<std::uint64_t>{"uint64_t", 63}, Tipo<nstd::uint128_t>{"nstd::uint128_t", 127},
                        Tipo<nstd::int128_t>{"nstd::int128_t (TC)", 127},
                        Tipo<nstd::uint128_fixed_t>{"nstd::uint128_fixed_t", 127},
                        Tipo<nstd::int128_fixed_t>{"nstd::int128_fixed_t", 127}),
#ifdef HAS_BUILTIN_INT128
        std::make_tuple(Tipo<u128_nativo>{"unsigned __int128", 127}, Tipo<i128_nativo>{"__int128", 127}),
#endif
        std::make_tuple(Tipo<boost_cpp_u128>{"boost::cpp_int u128", 127},
                        Tipo<boost_cpp_i128>{"boost::cpp_int i128", 127},
                        Tipo<boost_checked_u128>{"boost::checked_uint128", 127})
#ifdef BENCH_HAS_GMP_TOMMATH
            ,
        std::make_tuple(Tipo<boost_gmp_int>{"boost::gmp_int", 127},
                        Tipo<boost_tom_int>{"boost::tom_int", 127})
#endif
    );
}

static constexpr std::size_t NUM_TIPOS = std::tuple_size_v<decltype(tipos())>;

/// @brief El tipo contra el que se comprueban los demas. De Boost a proposito:
///        no es ninguno de los que la biblioteca pone a prueba.
using Referencia = boost_cpp_u128;

// =============================================================================
// ANTES DE MEDIR: todos calculan lo mismo
// =============================================================================

template <Seccion S, typename T>
static std::array<Valor, VALORES> resultados(unsigned ancho)
{
    auto k = prepara<S, T>(ancho);
    std::array<Valor, VALORES> v{};
    for (std::size_t i = 0; i < VALORES; ++i)
    {
        opera<S, T>(*k, i);
        v[i] = a_valor(k->r);
    }
    return v;
}

/// @brief Compara cada tipo con la referencia en las ocho parejas: en 127 bits
///        los de 128, en 63 bits `uint64_t`.
/// @return Cuantos tipos discrepan.
template <Seccion S>
static int discrepan()
{
    int malos = 0;
    std::apply(
        [&](const auto &...t)
        {
            (
                [&](const auto &tipo)
                {
                    using T = typename std::remove_cvref_t<decltype(tipo)>::tipo;
                    const auto esperado = resultados<S, Referencia>(tipo.ancho);
                    const auto obtenido = resultados<S, T>(tipo.ancho);
                    for (std::size_t i = 0; i < VALORES; ++i)
                        if (obtenido[i] != esperado[i])
                        {
                            std::printf("  [MAL] %-22s %-24s pareja %zu: da %016llx:%016llx, "
                                        "deberia %016llx:%016llx\n",
                                        nombre(S), tipo.nombre, i,
                                        static_cast<unsigned long long>(obtenido[i].alto),
                                        static_cast<unsigned long long>(obtenido[i].bajo),
                                        static_cast<unsigned long long>(esperado[i].alto),
                                        static_cast<unsigned long long>(esperado[i].bajo));
                            ++malos;
                            return;
                        }
                }(t),
                ...);
        },
        tipos());
    return malos;
}

// =============================================================================
// Medir una operacion
// =============================================================================

template <Seccion S>
static std::array<bench::Medida, NUM_TIPOS> mide()
{
    return std::apply(
        [](const auto &...t)
        {
            // Los bloques viven hasta el final de la medida; las variantes solo
            // guardan un puntero.
            auto bancos =
                std::make_tuple(prepara<S, typename std::remove_cvref_t<decltype(t)>::tipo>(t.ancho)...);
            return std::apply([](auto &...k)
                              { return bench::mide_entrelazado(std::make_tuple(vuelta(k.get())...)); },
                              bancos);
        },
        tipos());
}

// =============================================================================
// Verosimilitud
// =============================================================================
//
// DOS SUELOS, y una cifra que salte cualquiera de los dos no vale:
//
// - EL DEL MISMO TIPO: ninguna operacion aritmetica puede costar menos de la
//   mitad que la COPIA del mismo tipo (`r = a[i]`), porque hace todo lo que hace
//   la copia --leer un operando, escribir el resultado-- y algo mas. La mitad
//   deja margen de sobra para el ruido; lo que pretende cazar es lo que paso con
//   las cifras `(*)`: TomMath en 0,72 ciclos, por debajo de lo que cuesta
//   escribir el resultado. La comparacion queda fuera: lee dos operandos y
//   escribe un `bool`, y eso no es mas que una copia de 128 bits.
// - EL FISICO, solo en x86-64: cada vuelta escribe en memoria, ningun nucleo
//   actual hace mas de dos escrituras por ciclo, y con el turbo un ciclo del
//   nucleo puede medir medio tic del TSC. Menos de 0,25 no es posible. En ARM el
//   contador va a decenas de MHz y las cifras estan en otra escala.
static constexpr double FRACCION_DE_LA_COPIA = 0.5;
#if defined(__x86_64__) || defined(_M_X64)
static constexpr double SUELO_FISICO = 0.25;
#else
static constexpr double SUELO_FISICO = 0.0;
#endif

static int g_inverosimiles = 0;

// =============================================================================
// La tabla
// =============================================================================

template <Seccion S>
static void imprime_y_registra(const std::array<bench::Medida, NUM_TIPOS> &m,
                               const std::array<bench::Medida, NUM_TIPOS> &copia)
{
    std::printf("\n[%s]\n", nombre(S));
    std::printf("  %-26s %9s %9s %7s %8s %8s\n", "tipo", "minimo", "suelo", "(+%)", "limpias", "vs u64");
    std::printf("  %s\n", "--------------------------------------------------------------------------");
    std::size_t i = 0;
    std::apply(
        [&](const auto &...t)
        {
            (
                [&](const auto &tipo)
                {
                    const bench::Medida &x = m[i];
                    const bool bajo_fisico = x.minimo < SUELO_FISICO;
                    const bool bajo_copia = S != Seccion::copia && S != Seccion::menor &&
                                            x.minimo < FRACCION_DE_LA_COPIA * copia[i].minimo;
                    if (bajo_fisico || bajo_copia)
                        ++g_inverosimiles;
                    std::printf("  %-26s %9.2f %9.2f %+6.1f%% %7.0f%% %7.2fx%s\n", tipo.nombre, x.minimo,
                                x.suelo, x.dispersion_baja * 100.0, x.limpias * 100.0,
                                m[0].minimo > 0 ? x.minimo / m[0].minimo : 0.0,
                                bajo_fisico  ? "  <- INVEROSIMIL: bajo el suelo fisico"
                                : bajo_copia ? "  <- INVEROSIMIL: menos que copiar"
                                             : "");
                    bench::registra((std::string(nombre(S)) + " / " + tipo.nombre).c_str(), x);
                    ++i;
                }(t),
                ...);
        },
        tipos());
}

template <Seccion S>
static void seccion(const std::array<bench::Medida, NUM_TIPOS> &copia)
{
    imprime_y_registra<S>(mide<S>(), copia);
}

int main()
{
    std::printf("================================================================\n");
    std::printf("  BENCHMARK: los enteros de 128 bits, frente al compilador y a Boost\n");
    std::printf("================================================================\n");
    std::printf("  Un solo nucleo para todos los tipos (r = a[i] OP b[i]), los mismos\n");
    std::printf("  valores, variantes entrelazadas en orden al azar. RENDIMIENTO, no\n");
    std::printf("  latencia. Ver la cabecera del fuente.\n");
#ifdef HAS_BUILTIN_INT128
    std::printf("  __int128:   si\n");
#else
    std::printf("  __int128:   NO\n");
#endif
#ifdef BENCH_HAS_GMP_TOMMATH
    std::printf("  Boost:      cpp_int, checked, GMP, TomMath\n");
#else
    std::printf("  Boost:      cpp_int, checked\n");
#endif
    std::printf("  %zu tipos x 10 operaciones x %zu vueltas x %.0f ms\n", NUM_TIPOS, bench::REPETICIONES,
                bench::MS_POR_CASILLA);
    std::printf("================================================================\n");

    // ANTES DE MEDIR NADA: que todos calculen lo mismo.
    const int malos = discrepan<Seccion::copia>() + discrepan<Seccion::suma>() + discrepan<Seccion::resta>() +
                      discrepan<Seccion::producto>() + discrepan<Seccion::division_corta>() +
                      discrepan<Seccion::division_larga>() + discrepan<Seccion::desplaza_izq>() +
                      discrepan<Seccion::desplaza_der>() + discrepan<Seccion::o_exclusivo>() +
                      discrepan<Seccion::menor>();
    if (malos != 0)
    {
        std::printf("\n  %d discrepancias. ESTE BANCO NO MIDE LO MISMO EN TODOS LOS TIPOS:\n"
                    "  no se mide nada.\n",
                    malos);
        return 2;
    }
    std::printf("\n  Comprobado: los %zu tipos dan los mismos resultados en las 10 operaciones.\n",
                NUM_TIPOS);

    // La copia primero: es el suelo de las demas.
    const auto copia = mide<Seccion::copia>();
    imprime_y_registra<Seccion::copia>(copia, copia);
    seccion<Seccion::suma>(copia);
    seccion<Seccion::resta>(copia);
    seccion<Seccion::producto>(copia);
    seccion<Seccion::division_corta>(copia);
    seccion<Seccion::division_larga>(copia);
    seccion<Seccion::desplaza_izq>(copia);
    seccion<Seccion::desplaza_der>(copia);
    seccion<Seccion::o_exclusivo>(copia);
    seccion<Seccion::menor>(copia);

    std::printf("\n================================================================\n");
    std::printf("  minimo y suelo en ciclos (tics del TSC) por operacion; vs u64 =\n");
    std::printf("  minimo / minimo de uint64_t, que recibe los mismos valores a 63 bits.\n");
    std::printf("  divisor corto: de media anchura o menos (un limbo en 128 bits);\n");
    std::printf("  divisor largo: de mas de media anchura.\n");
    std::printf("================================================================\n");

    if (g_inverosimiles != 0)
    {
        std::printf("\n  [OJO] %d cifras INVEROSIMILES: el compilador se ha llevado trabajo,\n"
                    "        o el banco mide otra cosa. Esta toma no vale.\n",
                    g_inverosimiles);
        return 1;
    }
    return 0;
}
