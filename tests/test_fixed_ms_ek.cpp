// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: Magnitud-Signo y Exceso-K en fixed_int_t  (P1.5 tramo 3)
// Part of int128 Library
// =============================================================================
//
// LA FORMA DEL TEST LA FIJA ADR-018, Y NO ES CASUAL
// -------------------------------------------------
// [ADR-018] decide que **la representacion no es observable**: `fixed_int_t` con
// Magnitud-Signo, con Exceso-K y con complemento a dos dan **lo mismo en todo**.
//
// Eso convierte el test en algo muy simple y muy dificil de enganar: para cada
// valor y cada operacion, **el resultado en MS y en EK tiene que coincidir con el
// de complemento a dos**. Si son la misma cosa, coinciden siempre.
//
// POR QUE NO SE PRUEBAN PROPIEDADES
// ---------------------------------
// El tipo viejo llevaba MS y EK «implementados» y estaban **rotos**: `~` en MS
// daba -1,70e38 en vez de -4, y en Exceso-K los desplazamientos enteros daban
// basura. Nadie lo vio en meses porque sus tests comprueban cosas asi:
//
//     TEST("ms not changes value",      ~x != x);
//     TEST("ms demorgan ~(a&b)==~a|~b", ~(a & b) == (~a | ~b));
//     TEST("ms shl pos sign preserved", !(x << 1).is_negative());
//
// **Ninguno mira el valor.** De Morgan se cumple igual con basura, porque es una
// identidad estructural: vale sobre las magnitudes y los signos coinciden a los
// dos lados. Una comprobacion que no puede fallar no es una comprobacion.
//
// Aqui todo se cruza contra el oraculo --complemento a dos-- y se compara el
// **valor decimal**, que es lo unico que no se puede cumplir por accidente.
// =============================================================================

#include "fixed_int_traits_specializations.hpp"
#include "fixed_width_int_t.hpp"

#include <cstdio>
#include <string>

namespace
{
    int fallos = 0;
    long long casos = 0;

    using namespace nstd;

    constexpr std::size_t N = 2; // 128 bits: suficiente para cruzar limbos

    using TC = fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>;
    using MS = fixed_int_t<N, signedness::signed_type, representation_form::magnitude_sign>;
    using EK = fixed_int_t<N, signedness::signed_type, representation_form::excess_k>;

    void mal(const char *op, const char *repr, long long a, long long b, const std::string &esp,
             const std::string &obt)
    {
        std::printf("  [FALLA] %-10s %-3s  a=%lld b=%lld   C2 dice %s, %s dice %s\n", op, repr, a, b,
                    esp.c_str(), repr, obt.c_str());
        ++fallos;
    }

    /// El nucleo: una operacion, los tres tipos, y el valor decimal comparado.
    ///
    /// `Op` se instancia tres veces, una por representacion, y todas tienen que
    /// devolver el mismo numero.
    template <typename Op>
    void cruza(const char *nombre, long long a, long long b, Op op)
    {
        const std::string en_tc = op(TC{a}, TC{b}).to_string();
        const std::string en_ms = op(MS{a}, MS{b}).to_string();
        const std::string en_ek = op(EK{a}, EK{b}).to_string();
        ++casos;
        if (en_ms != en_tc)
            mal(nombre, "MS", a, b, en_tc, en_ms);
        if (en_ek != en_tc)
            mal(nombre, "EK", a, b, en_tc, en_ek);
    }

    /// Para los predicados, que devuelven `bool` y no un numero.
    template <typename Op>
    void cruza_bool(const char *nombre, long long a, long long b, Op op)
    {
        const bool en_tc = op(TC{a}, TC{b});
        const bool en_ms = op(MS{a}, MS{b});
        const bool en_ek = op(EK{a}, EK{b});
        ++casos;
        if (en_ms != en_tc)
            mal(nombre, "MS", a, b, en_tc ? "true" : "false", en_ms ? "true" : "false");
        if (en_ek != en_tc)
            mal(nombre, "EK", a, b, en_tc ? "true" : "false", en_ek ? "true" : "false");
    }

    // Declarado aqui porque `todas_las_operaciones` lo llama y se define abajo,
    // junto al resto de ayudantes del recuento de bits.
    void cruza_bits(long long v);

    void todas_las_operaciones(long long a, long long b)
    {
        // Construir el mismo numero en las tres tiene que dar el mismo numero.
        // Si esto falla, lo demas no significa nada.
        cruza("construir", a, b, [](auto x, auto) { return x; });

        cruza("+", a, b, [](auto x, auto y) { return x + y; });
        cruza("-", a, b, [](auto x, auto y) { return x - y; });
        cruza("*", a, b, [](auto x, auto y) { return x * y; });
        cruza("negar", a, b, [](auto x, auto) { return -x; });

        if (b != 0)
        {
            cruza("/", a, b, [](auto x, auto y) { return x / y; });
            cruza("%", a, b, [](auto x, auto y) { return x % y; });
        }

        // Los que ADR-018 desvia del comportamiento del tipo viejo.
        cruza("~", a, b, [](auto x, auto) { return ~x; });
        cruza("&", a, b, [](auto x, auto y) { return x & y; });
        cruza("|", a, b, [](auto x, auto y) { return x | y; });
        cruza("^", a, b, [](auto x, auto y) { return x ^ y; });
        cruza("<<1", a, b, [](auto x, auto) { return x << 1U; });
        cruza(">>1", a, b, [](auto x, auto) { return x >> 1U; });
        cruza(">>3", a, b, [](auto x, auto) { return x >> 3U; });

        // Las funciones LIBRES. Estaban escritas sobre los alias `int_fixed_t`
        // y `uint_fixed_t`, que fijan `Form` a complemento a dos, asi que ni
        // compilaban para MS/EK. Lo destapo la matriz de paridad al abrir las
        // columnas; se generalizaron, y esto comprueba que ademas **dan el valor
        // correcto**, que es otra cosa.
        cruza("mulhi", a, b, [](auto x, auto y) { return nstd::mulhi(x, y); });
        cruza("mullo", a, b, [](auto x, auto y) { return nstd::mullo(x, y); });
        cruza("abs", a, b, [](auto x, auto) { return nstd::abs(x); });

        // `mul_wide` devuelve el DOBLE de ancho, asi que se compara aparte.
        {
            ++casos;
            const std::string w_tc = nstd::mul_wide(TC{a}, TC{b}).to_string();
            const std::string w_ms = nstd::mul_wide(MS{a}, MS{b}).to_string();
            const std::string w_ek = nstd::mul_wide(EK{a}, EK{b}).to_string();
            if (w_ms != w_tc)
                mal("mul_wide", "MS", a, b, w_tc, w_ms);
            if (w_ek != w_tc)
                mal("mul_wide", "EK", a, b, w_tc, w_ek);
        }

        // `pow` y `gcd`/`lcm` piden operandos no negativos o exponente sin signo.
        if (a >= 0 && b >= 0)
        {
            ++casos;
            const auto e = uint_fixed_t<N>{std::uint64_t{3}};
            const std::string p_tc = nstd::pow(TC{a}, e).to_string();
            const std::string p_ms = nstd::pow(MS{a}, e).to_string();
            const std::string p_ek = nstd::pow(EK{a}, e).to_string();
            if (p_ms != p_tc)
                mal("pow", "MS", a, b, p_tc, p_ms);
            if (p_ek != p_tc)
                mal("pow", "EK", a, b, p_tc, p_ek);

            if (a != 0 && b != 0)
            {
                ++casos;
                const std::string g_tc = nstd::gcd(TC{a}, TC{b}).to_string();
                const std::string g_ms = nstd::gcd(MS{a}, MS{b}).to_string();
                const std::string g_ek = nstd::gcd(EK{a}, EK{b}).to_string();
                if (g_ms != g_tc)
                    mal("gcd", "MS", a, b, g_tc, g_ms);
                if (g_ek != g_tc)
                    mal("gcd", "EK", a, b, g_tc, g_ek);
            }
        }

        // El RECUENTO DE BITS, que es lo que a este test le faltaba.
        //
        // El tramo 3 cruzo la aritmetica, los desplazamientos, los bitwise, el
        // orden y `to_string`. No cruzo esto, y ahi habia un fallo de verdad:
        // `popcount` contaba los limbos guardados, no el valor, asi que en
        // Exceso-K contaba ademas el bit del sesgo y en Magnitud-Signo el del
        // signo. Discrepaba en 194 de 400 valores en MS y en los 400 en EK.
        cruza_bits(a);
        cruza_bits(b);

        // El orden, que es la decision 3.
        cruza_bool("<", a, b, [](auto x, auto y) { return x < y; });
        cruza_bool("==", a, b, [](auto x, auto y) { return x == y; });
        cruza_bool("is_negative", a, b, [](auto x, auto) { return x.is_negative(); });
        cruza_bool("is_zero", a, b, [](auto x, auto) { return x.is_zero(); });
    }

    /// Cruza el recuento de bits contra complemento a dos.
    ///
    /// Se comparan NUMEROS, no propiedades. `popcount(x) == popcount(x)` seria
    /// una identidad estructural de las que este fichero rechaza en su
    /// encabezado: se cumple con basura.
    void cruza_bits(long long v)
    {
        const TC t{v};
        const MS m{v};
        const EK e{v};

        struct Op
        {
            const char *nombre;
            unsigned (*f)(const TC &);
            unsigned (*g)(const MS &);
            unsigned (*h)(const EK &);
        };

        ++casos;
        if (m.popcount() != t.popcount() || e.popcount() != t.popcount())
            mal("popcount", "MS/EK", v, 0, std::to_string(t.popcount()),
                std::to_string(m.popcount()) + "/" + std::to_string(e.popcount()));

        ++casos;
        if (m.bit_width() != t.bit_width() || e.bit_width() != t.bit_width())
            mal("bit_width", "MS/EK", v, 0, std::to_string(t.bit_width()),
                std::to_string(m.bit_width()) + "/" + std::to_string(e.bit_width()));

        ++casos;
        if (m.count_leading_zeros() != t.count_leading_zeros() ||
            e.count_leading_zeros() != t.count_leading_zeros())
            mal("count_leading_zeros", "MS/EK", v, 0, std::to_string(t.count_leading_zeros()),
                std::to_string(m.count_leading_zeros()));

        ++casos;
        if (m.count_trailing_zeros() != t.count_trailing_zeros() ||
            e.count_trailing_zeros() != t.count_trailing_zeros())
            mal("count_trailing_zeros", "MS/EK", v, 0, std::to_string(t.count_trailing_zeros()),
                std::to_string(m.count_trailing_zeros()));

        // Y `is_power_of_2`, que es donde se vio. OJO: con valores al azar esta
        // comprobacion NO PUEDE FALLAR --un numero de 128 bits al azar no es
        // potencia de dos nunca-- asi que las potencias se construyen aparte,
        // abajo, en `potencias_de_dos()`.
        ++casos;
        const bool pt = nstd::is_power_of_2(t);
        if (nstd::is_power_of_2(m) != pt || nstd::is_power_of_2(e) != pt)
            mal("is_power_of_2", "MS/EK", v, 0, pt ? "1" : "0", "distinto");
    }

    /// Las potencias de dos, a mano.
    ///
    /// Es la parte que los aleatorios no dan: `is_power_of_2` salio con **cero
    /// discrepancias sobre 400 valores al azar** estando rota para todas las
    /// potencias de dos.
    void potencias_de_dos()
    {
        for (unsigned b = 0; b < 126; ++b)
        {
            const TC t = TC::one() << b;
            const MS m = MS::one() << b;
            const EK e = EK::one() << b;

            ++casos;
            if (!nstd::is_power_of_2(t) || !nstd::is_power_of_2(m) || !nstd::is_power_of_2(e))
            {
                std::printf("  [FALLA] 2^%u no se reconoce como potencia de dos en las tres\n", b);
                ++fallos;
            }

            ++casos;
            if (m.popcount() != 1U || e.popcount() != 1U || t.popcount() != 1U)
            {
                std::printf("  [FALLA] popcount(2^%u) no es 1: TC=%u MS=%u EK=%u\n", b, t.popcount(),
                            m.popcount(), e.popcount());
                ++fallos;
            }

            ++casos;
            if (m.bit_width() != t.bit_width() || e.bit_width() != t.bit_width())
            {
                std::printf("  [FALLA] bit_width(2^%u) discrepa: TC=%u MS=%u EK=%u\n", b, t.bit_width(),
                            m.bit_width(), e.bit_width());
                ++fallos;
            }
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
} // namespace

int main()
{
    std::printf("=== Magnitud-Signo y Exceso-K contra complemento a dos ===\n\n");

    // Las esquinas a mano: el cero y sus vecinos, los signos cruzados, y los
    // valores donde `>>` distingue truncar-a-cero de redondear-a--inf.
    const long long esquinas[] = {0, 1,  -1, 2,  -2, 3,   -3,  4,    -4,   5,    -5,
                                  7, -7, 8,  -8, 10, -10, 100, -100, 1023, -1023};
    std::printf("-- esquinas (%zu x %zu combinaciones)\n", sizeof(esquinas) / sizeof(esquinas[0]),
                sizeof(esquinas) / sizeof(esquinas[0]));
    for (long long a : esquinas)
        for (long long b : esquinas)
            todas_las_operaciones(a, b);

    // Las potencias de dos, que los aleatorios no dan nunca.
    std::printf("-- potencias de dos (lo que los aleatorios no generan)\n");
    potencias_de_dos();

    // Y aleatorios, que cubren el caso comun.
    std::printf("-- aleatorios\n");
    xorshift rng{0xA5A5ULL};
    for (int v = 0; v < 300; ++v)
    {
        const long long a = static_cast<long long>(rng() % 200001) - 100000;
        const long long b = static_cast<long long>(rng() % 200001) - 100000;
        todas_las_operaciones(a, b);
    }

    // El caso que el tipo viejo tenia ROTO, como prueba nombrada: `~3`.
    {
        ++casos;
        const std::string tc = (~TC{3}).to_string();
        const std::string ms = (~MS{3}).to_string();
        const std::string ek = (~EK{3}).to_string();
        std::printf("-- el caso que el tipo viejo tenia roto: ~3 = %s (C2), %s (MS), %s (EK)\n", tc.c_str(),
                    ms.c_str(), ek.c_str());
        if (tc != "-4" || ms != "-4" || ek != "-4")
        {
            std::printf("  [FALLA] ~3 deberia ser -4 en las tres\n");
            ++fallos;
        }
    }

    // Y el que el tipo viejo resolvia AL REVES a proposito: `-3 >> 1`.
    {
        ++casos;
        const std::string ms = (MS{-3} >> 1U).to_string();
        std::printf("-- el que el tipo viejo truncaba: -3 >> 1 = %s en MS (el viejo daba -1)\n", ms.c_str());
        if (ms != "-2")
        {
            std::printf("  [FALLA] -3 >> 1 deberia ser -2, no %s\n", ms.c_str());
            ++fallos;
        }
    }

    // Los dos ceros de Magnitud-Signo: distinto patron, mismo numero.
    {
        ++casos;
        MS mas_cero{0};
        MS menos_cero = -MS{0};
        if (!(mas_cero == menos_cero))
        {
            std::printf("  [FALLA] en MS, +0 y -0 deberian ser iguales\n");
            ++fallos;
        }
        if (!menos_cero.is_zero())
        {
            std::printf("  [FALLA] -0 deberia ser cero\n");
            ++fallos;
        }
    }

    std::printf("\n%lld comprobaciones cruzadas contra complemento a dos, %d fallos\n", casos, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
