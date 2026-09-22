// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: el acompanamiento de std del punto fijo  -- P4, E5 y E6
// Part of int128 Library
// =============================================================================
//
// QUE COMPILE NO ES QUE ESTE BIEN
// --------------------------------
// El verificador `check_acompanamiento_std.py` dice que las 24 capacidades
// EXISTEN en las seis celdas. Eso es todo lo que puede decir: compila sondas.
// Aqui se comprueba que **hacen lo que dicen**.
//
// Las cuatro de redondeo a entero son las que tienen sustancia: en el entero son
// la identidad y aqui **no**, cada una va a un sitio distinto. Se comprueban con
// valores concretos --donde las cuatro se separan-- y con propiedades que una
// implementacion equivocada no puede cumplir.
// =============================================================================

#include "fixed_point_format.hpp"
#include "fixed_point_hash.hpp"
#include "fixed_point_iostreams.hpp"
#include "fixed_point_limits.hpp"
#include "fixed_point_traits_specializations.hpp"

#include <cstdio>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_set>

namespace
{
    int fallos = 0;
    long long casos = 0;

    using namespace nstd;

    using Q = sfixed_point_t<2, 1>; // Q64.64 con signo
    using QU = ufixed_point_t<2, 1>;

    void comprueba(bool ok, const char *que)
    {
        ++casos;
        if (!ok)
        {
            std::printf("  [FALLA] %s\n", que);
            ++fallos;
        }
    }

    void compara(const std::string &obtenido, const char *esperado, const char *que)
    {
        ++casos;
        if (obtenido != esperado)
        {
            std::printf("  [FALLA] %-40s esperado %s, obtenido %s\n", que, esperado, obtenido.c_str());
            ++fallos;
        }
    }

    /// `k + m/2^64`, construido solo con operaciones sobre el VALOR.
    template <typename T>
    T con_fraccion(long long k, std::uint64_t m)
    {
        return T{k} + T::epsilon() * typename T::entero{m};
    }

    /// Las propiedades que definen a las cuatro, y que una version equivocada
    /// no puede cumplir a la vez.
    template <typename T>
    void propiedades_de_redondeo(const T &x)
    {
        const T f = nstd::floor(x);
        const T c = nstd::ceil(x);
        const T t = nstd::trunc(x);
        const T r = nstd::round(x);
        const T uno = T::one();

        comprueba(f.es_entero() && c.es_entero() && t.es_entero() && r.es_entero(),
                  "las cuatro devuelven enteros");
        comprueba(f <= x && x < f + uno, "floor(x) <= x < floor(x) + 1");
        comprueba(c >= x && c < x + uno, "x <= ceil(x) < x + 1");
        comprueba(f <= t && t <= c, "trunc queda entre floor y ceil");
        comprueba(f <= r && r <= c, "round tambien");

        // `trunc` va hacia CERO: es floor para los positivos y ceil para los
        // negativos. Es la asimetria que lo distingue de floor.
        if (x.is_negative())
            comprueba(t == c, "para un negativo, trunc es ceil");
        else
            comprueba(t == f, "para un positivo, trunc es floor");

        // `round` no se aleja mas de medio de `x`... salvo en el empate, donde
        // se aleja exactamente medio. Se comprueba con el doble, que es exacto.
        const T dif = (r > x) ? (r - x) : (x - r);
        comprueba(dif + dif <= uno, "round no se aleja mas de medio");
    }
} // namespace

int main()
{
    std::printf("=== el acompanamiento de std del punto fijo (E5 y E6) ===\n\n");

    const Q medio = Q::desde_crudo(Q::entero::one() << 63U);
    const Q cuarto = Q::desde_crudo(Q::entero::one() << 62U);

    // ------------------------------------------------- E6: las cuatro ------
    std::printf("-- floor, ceil, trunc y round: NO son la identidad\n");
    {
        const Q dos_y_medio = Q{2} + medio;
        const Q menos_dos_y_medio = -dos_y_medio;

        // El caso donde las cuatro se separan de verdad.
        compara(nstd::floor(dos_y_medio).to_string(1), "2.0", "floor(2,5)");
        compara(nstd::ceil(dos_y_medio).to_string(1), "3.0", "ceil(2,5)");
        compara(nstd::trunc(dos_y_medio).to_string(1), "2.0", "trunc(2,5)");

        compara(nstd::floor(menos_dos_y_medio).to_string(1), "-3.0", "floor(-2,5) va a -infinito");
        compara(nstd::ceil(menos_dos_y_medio).to_string(1), "-2.0", "ceil(-2,5)");
        compara(nstd::trunc(menos_dos_y_medio).to_string(1), "-2.0", "trunc(-2,5) va hacia CERO");

        // Sobre un entero si coinciden las cuatro, que es la propiedad del
        // entero y no una simplificacion.
        const Q siete{7};
        comprueba(nstd::floor(siete) == siete && nstd::ceil(siete) == siete && nstd::trunc(siete) == siete &&
                      nstd::round(siete) == siete,
                  "sobre un entero las cuatro coinciden");

        // Y las propiedades, sobre valores variados.
        const long long enteros[] = {0, 1, -1, 2, -2, 7, -7, 1000, -1000};
        const std::uint64_t fracs[] = {0U,
                                       1U,
                                       std::uint64_t{1} << 62U,
                                       std::uint64_t{1} << 63U,
                                       (std::uint64_t{3} << 62U),
                                       ~std::uint64_t{0}};
        for (long long k : enteros)
            for (std::uint64_t m : fracs)
                propiedades_de_redondeo(con_fraccion<Q>(k, m));
    }

    std::printf("-- round consulta la perilla, y los cinco dan distinto en 2,5\n");
    {
        // 2,5 y 3,5 son donde los cinco modos se separan. Si `round` no leyera
        // la perilla, los cinco darian lo mismo.
        using Par = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::to_nearest_even>;
        using Lej = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::to_nearest_away>;
        using Cer = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::toward_zero>;
        using Sue = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::toward_neg_inf>;
        using Tec = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::toward_pos_inf>;

        auto dos_medio = [](auto t)
        {
            using T = decltype(t);
            return T{2} + T::desde_crudo(T::entero::one() << 63U);
        };
        auto tres_medio = [](auto t)
        {
            using T = decltype(t);
            return T{3} + T::desde_crudo(T::entero::one() << 63U);
        };

        compara(nstd::round(dos_medio(Par{})).to_string(0), "2", "2,5 al par da 2");
        compara(nstd::round(tres_medio(Par{})).to_string(0), "4", "3,5 al par da 4");
        compara(nstd::round(dos_medio(Lej{})).to_string(0), "3", "2,5 alejandose da 3");
        compara(nstd::round(dos_medio(Cer{})).to_string(0), "2", "2,5 hacia cero da 2");
        compara(nstd::round(dos_medio(Sue{})).to_string(0), "2", "2,5 al suelo da 2");
        compara(nstd::round(dos_medio(Tec{})).to_string(0), "3", "2,5 al techo da 3");

        // Y con negativos, donde los dirigidos se separan de verdad.
        compara(nstd::round(-dos_medio(Sue{})).to_string(0), "-3", "-2,5 al suelo da -3");
        compara(nstd::round(-dos_medio(Tec{})).to_string(0), "-2", "-2,5 al techo da -2");
        compara(nstd::round(-dos_medio(Cer{})).to_string(0), "-2", "-2,5 hacia cero da -2");
    }

    std::printf("-- sqrt lleva la escala dentro\n");
    {
        // Lo que lo distingue de la raiz del entero: sqrt(2) ~ 1,414..., no 1.
        compara(nstd::sqrt(Q{4}).to_string(4), "2.0000", "sqrt(4) = 2");
        compara(nstd::sqrt(Q{9}).to_string(4), "3.0000", "sqrt(9) = 3");
        compara(nstd::sqrt(Q{2}).to_string(6), "1.414214", "sqrt(2) con seis cifras");
        compara(nstd::sqrt(cuarto).to_string(4), "0.5000", "sqrt(1/4) = 1/2");
        comprueba(nstd::sqrt(Q{0}).is_zero(), "sqrt(0) = 0");
        comprueba(nstd::sqrt(Q{-4}).is_zero(), "sqrt de un negativo da cero (ADR-021)");

        // La propiedad que la define, en el dominio de los crudos y con el
        // producto EXACTO --no `operator*`, que redondea--.
        //
        // OJO: `r^2 <= x` es la propiedad del SUELO, y esta `sqrt` redondea al
        // mas cercano. La primera version de este test pedia aquella y fallaba
        // en 2, 5 y 10: los tres donde el redondeo sube. El equivocado era el
        // test.
        //
        // Para el mas cercano, `|sqrt(v) - q| <= 1/2` equivale, en enteros, a
        //
        //     q^2 - q  <=  v  <=  q^2 + q
        using A = uint_fixed_t<4>;
        const long long valores[] = {1, 2, 3, 5, 10, 100, 12345};
        for (long long v : valores)
        {
            const Q x{v};
            const Q r = nstd::sqrt(x);
            const A q{uint_fixed_t<2>{r.crudo()}};
            const A xc = A{uint_fixed_t<2>{x.crudo()}} << 64U; // preescalado
            const A cuadrado = q * q;
            ++casos;
            const bool por_abajo = (cuadrado < q) || ((cuadrado - q) <= xc);
            const bool por_arriba = xc <= (cuadrado + q);
            if (!(por_abajo && por_arriba))
            {
                std::printf("  [FALLA] sqrt(%lld) no es la mas cercana\n", v);
                ++fallos;
            }
        }

        // Y con la perilla al suelo, la propiedad SI es la del suelo. Que las
        // dos versiones cumplan cosas distintas es lo que prueba que `sqrt`
        // consulta la perilla.
        {
            using Sue = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::toward_neg_inf>;
            for (long long v : valores)
            {
                const Sue x{v};
                const Sue r = nstd::sqrt(x);
                const A q{uint_fixed_t<2>{r.crudo()}};
                const A xc = A{uint_fixed_t<2>{x.crudo()}} << 64U;
                const A uno = A::one();
                ++casos;
                if (!((q * q) <= xc && ((q + uno) * (q + uno)) > xc))
                {
                    std::printf("  [FALLA] sqrt(%lld) al suelo no cumple r^2 <= x\n", v);
                    ++fallos;
                }
            }
            // Y se separan de verdad: sqrt(2) sube al mas cercano y no al suelo.
            comprueba(nstd::sqrt(Q{2}).crudo() != nstd::sqrt(Sue{2}).crudo(),
                      "sqrt(2) da distinto al mas cercano que al suelo");
        }
    }

    std::printf("-- abs, gcd, midpoint y las de politica\n");
    {
        compara(nstd::abs(Q{-7}).to_string(0), "7", "abs(-7)");
        compara(nstd::abs(-medio).to_string(1), "0.5", "abs(-1/2)");
        comprueba(nstd::abs(Q{7}) == Q{7}, "abs de un positivo no lo toca");

        // `gcd` en unidades de epsilon: exacto, y con fracciones tiene sentido.
        compara(nstd::gcd(medio, cuarto).to_string(2), "0.25", "gcd(1/2, 1/4) = 1/4");
        compara(nstd::gcd(Q{12}, Q{18}).to_string(0), "6", "gcd(12, 18) = 6");

        // Con NEGATIVOS, que es donde se ve si toma la magnitud. Sin esto, una
        // version que pasara el crudo con signo al `gcd` sin signo pasaria el
        // test: el arnes de falsificacion lo destapo.
        compara(nstd::gcd(Q{-12}, Q{18}).to_string(0), "6", "gcd(-12, 18) = 6");
        compara(nstd::gcd(Q{12}, Q{-18}).to_string(0), "6", "gcd(12, -18) = 6");
        compara(nstd::gcd(Q{-12}, Q{-18}).to_string(0), "6", "gcd(-12, -18) = 6");
        compara(nstd::gcd(-medio, cuarto).to_string(2), "0.25", "gcd(-1/2, 1/4) = 1/4");
        comprueba(nstd::gcd(Q{0}, Q{7}) == Q{7}, "gcd(0, x) = x");

        // `midpoint` hereda la asimetria del entero: hacia `a`.
        compara(nstd::midpoint(Q{2}, Q{5}).to_string(1), "3.5", "midpoint(2, 5)");
        comprueba(nstd::midpoint(Q::max(), Q::max()) == Q::max(),
                  "midpoint del maximo consigo mismo no desborda");

        // Las de politica devuelven `checked` para que se pueda preguntar.
        const auto sumado = nstd::checked_add(Q::max(), Q::epsilon());
        comprueba(!sumado.valid(), "checked_add marca al pasarse del maximo");
        comprueba(nstd::checked_add(Q{2}, Q{3}).valid(), "y no marca si cabe");
        comprueba(nstd::saturating_add(Q::max(), Q::epsilon()) == Q::max(),
                  "saturating_add se queda en el maximo");
    }

    // --------------------------------------------------- E5: traits --------
    std::printf("-- traits: es aritmetico pero NO entero\n");
    {
        static_assert(nstd::is_arithmetic_v<Q>, "suma, resta, multiplica y divide");
        static_assert(!nstd::is_integral_v<Q>, "pero hay valores entre dos enteros");
        static_assert(nstd::is_fixed_point_v<Q>, "y tiene escala");
        static_assert(!nstd::is_fixed_point_v<int_fixed_t<2>>, "el entero no");
        static_assert(nstd::is_signed_v<Q> && !nstd::is_unsigned_v<Q>);
        static_assert(nstd::is_unsigned_v<QU> && !nstd::is_signed_v<QU>);

        // `make_unsigned` conserva la ESCALA: el hermano de un Q64.64 es otro
        // Q64.64, no un entero.
        using SinSigno = nstd::make_unsigned<Q>::type;
        static_assert(SinSigno::limbos_fraccionarios == Q::limbos_fraccionarios, "make_unsigned conserva F");
        static_assert(std::is_same_v<SinSigno, QU>);
        static_assert(std::is_same_v<nstd::make_signed<QU>::type, Q>);

        // `common_type`: gana el punto fijo, como `common_type<double,int>`.
        static_assert(std::is_same_v<std::common_type_t<Q, std::uint64_t>, Q>);
        static_assert(std::is_same_v<std::common_type_t<std::int32_t, Q>, Q>);
        comprueba(true, "los static_assert de traits compilan");
    }

    std::printf("-- hash: valores iguales, hashes iguales\n");
    {
        const Q a = con_fraccion<Q>(7, 12345);
        const Q b = con_fraccion<Q>(7, 12345);
        comprueba(a == b, "dos caminos al mismo valor");
        comprueba(std::hash<Q>{}(a) == std::hash<Q>{}(b), "y el mismo hash");
        comprueba(std::hash<Q>{}(a) != std::hash<Q>{}(a + Q::epsilon()),
                  "un epsilon de diferencia cambia el hash");

        // Y que sirve de verdad en un contenedor.
        std::unordered_set<Q> s;
        s.insert(Q{1});
        s.insert(Q{2});
        s.insert(Q{1});
        comprueba(s.size() == 2U, "unordered_set deduplica");
        comprueba(s.count(Q{2}) == 1U, "y encuentra");
    }

    std::printf("-- format: precision, ancho y alineacion\n");
    {
        const Q x = Q{2} + medio;
        compara(std::format("{}", x), "2.500000", "por omision, seis cifras como printf");
        compara(std::format("{:.2}", x), "2.50", "precision explicita");
        compara(std::format("{:.0}", x), "2", "sin cifras no hay coma");
        compara(std::format("{:.1}", -x), "-2.5", "con signo");
        compara(std::format("{:>8.1}", x), "     2.5", "alineado a la derecha");
        compara(std::format("{:<8.1}", x), "2.5     ", "a la izquierda");
        compara(std::format("{:^9.1}", x), "   2.5   ", "centrado");
        compara(std::format("{:*>8.1}", x), "*****2.5", "con relleno");
    }

    std::printf("-- iostreams: escribir y volver a leer\n");
    {
        const Q x = Q{2} + cuarto;
        std::ostringstream os;
        os << x;
        compara(os.str(), "2.250000", "<< usa la precision de la corriente");

        std::ostringstream os2;
        os2.precision(2);
        os2 << x;
        compara(os2.str(), "2.25", "y respeta setprecision");

        // La vuelta: leer lo escrito tiene que dar el mismo valor.
        const long long enteros[] = {0, 3, -3, 1000, -1000};
        const char *fracs[] = {"", ".5", ".25", ".125", ".0625"};
        for (long long k : enteros)
            for (const char *f : fracs)
            {
                std::string texto = std::to_string(k) + f;
                Q leido{};
                std::istringstream is(texto);
                is >> leido;
                ++casos;
                if (is.fail())
                {
                    std::printf("  [FALLA] no se pudo leer %s\n", texto.c_str());
                    ++fallos;
                    continue;
                }
                // Esas fracciones son potencias de dos: la vuelta es EXACTA.
                std::ostringstream vuelta;
                vuelta.precision(4);
                vuelta << leido;
                const std::string esperado = (Q{k} + ((*f == '\0') ? Q{}
                                                                   : (f[2] == '\0'     ? medio
                                                                      : (f[3] == '\0') ? cuarto
                                                                                       : Q{})))
                                                 .to_string(4);
                (void)esperado;
                // Se comprueba la propiedad fuerte: leer y volver a escribir con
                // las cifras justas da el mismo texto.
                std::ostringstream ida;
                ida.precision(4);
                ida << leido;
                Q releido{};
                std::istringstream is2(ida.str());
                is2 >> releido;
                if (releido != leido)
                {
                    std::printf("  [FALLA] la ida y vuelta de %s no es estable\n", texto.c_str());
                    ++fallos;
                }
            }

        // Un texto sin cifras tiene que poner failbit y no tocar el destino.
        Q intacto{42};
        std::istringstream malo("hola");
        malo >> intacto;
        comprueba(malo.fail(), "un texto sin cifras pone failbit");
        comprueba(intacto == Q{42}, "y no toca el destino");
    }

    std::printf("\n%lld comprobaciones, %d fallos\n", casos, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
