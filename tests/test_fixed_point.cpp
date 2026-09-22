// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: fixed_point_t -- el tipo y las conversiones  (P1.6)
// Part of int128 Library
// =============================================================================
//
// EL ORACULO ES `__int128` ESCALADO
// ---------------------------------
// ADR-019 pide cruzar contra un oraculo y **prohibe expresamente comprobar
// propiedades** --asociatividad, distributividad--, porque con redondeo no se
// cumplen, y las que si se cumplen lo harian tambien con el redondeo mal puesto.
//
// Aqui el oraculo es aritmetica exacta en `__int128` sobre el valor **escalado**:
// un `fixed_point_t<N,F>` vale `crudo / 2^(64F)`, asi que el entero `crudo` es un
// numero exacto y se puede reproducir fuera del tipo. Se usa `F=1` y `N=2`, o sea
// Q64.64, que cabe holgado en `__int128`.
//
// Esta entrega cubre **lo exacto**: construccion, conversiones, suma, resta,
// negacion, comparacion, las dos mitades y `to_string`. El producto y la division
// no existen todavia --necesitan el redondeo-- y por eso no se prueban.
// =============================================================================

#include "fixed_point_t.hpp"

#include <cstdio>
#include <string>

namespace
{
    int fallos = 0;
    long long casos = 0;

    using namespace nstd;

    // Q64.64 con signo: 2 limbos, 1 fraccionario.
    using Q = sfixed_point_t<2, 1>;
    using QU = ufixed_point_t<2, 1>;

    // Las mismas Q64.64, pero codificadas en Magnitud-Signo y en Exceso-K.
    // Por ADR-018 la representacion **no se observa desde el comportamiento**,
    // asi que estas dos tienen que dar lo mismo que `Q` en todo.
    using Qms = fixed_point_t<2, 1, signedness::signed_type, representation_form::magnitude_sign>;
    using Qek = fixed_point_t<2, 1, signedness::signed_type, representation_form::excess_k>;

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
            std::printf("  [FALLA] %-34s esperado %s, obtenido %s\n", que, esperado, obtenido.c_str());
            ++fallos;
        }
    }

#ifdef __SIZEOF_INT128__
    /// El oraculo: el valor escalado, en aritmetica exacta de 128 bits.
    ///
    /// `Q{k}` guarda `k * 2^64`, asi que el crudo cabe en `__int128` mientras
    /// `|k| < 2^63`.
    __int128 crudo_de(const Q &x)
    {
        const auto e = x.crudo();
        return (static_cast<__int128>(static_cast<std::int64_t>(e.limb(1))) << 64) |
               static_cast<__int128>(e.limb(0));
    }

    void cruza_suma(long long a, long long b)
    {
        const __int128 esperado = (static_cast<__int128>(a) << 64) + (static_cast<__int128>(b) << 64);
        ++casos;
        if (crudo_de(Q{a} + Q{b}) != esperado)
        {
            std::printf("  [FALLA] suma escalada  a=%lld b=%lld\n", a, b);
            ++fallos;
        }
        const __int128 esp_resta = (static_cast<__int128>(a) << 64) - (static_cast<__int128>(b) << 64);
        ++casos;
        if (crudo_de(Q{a} - Q{b}) != esp_resta)
        {
            std::printf("  [FALLA] resta escalada  a=%lld b=%lld\n", a, b);
            ++fallos;
        }
    }
#endif

    /// Construye `k + m/2^64` en el tipo que sea.
    ///
    /// Solo usa operaciones sobre el VALOR --construir desde un entero, sumar y
    /// multiplicar por un entero--, que por ADR-018 dan lo mismo en las cuatro
    /// representaciones. Escribir la codificacion a mano seria justo lo que el
    /// test tiene que comprobar, no lo que puede dar por hecho.
    template <typename T>
    T con_fraccion(long long k, std::uint64_t m)
    {
        return T{k} + T::epsilon() * typename T::entero{m};
    }

    /// Cruza una codificacion contra complemento a dos, en TODO lo observable.
    ///
    /// No comprueba propiedades --ADR-019 lo prohibe expresamente-- sino
    /// igualdad termino a termino contra la referencia, que es la forma que
    /// fijo ADR-018 y la unica que puede fallar.
    template <typename Otro>
    void cruza_codificacion(long long k, const char *como)
    {
        const Q ref{k};
        const Otro otro{k};

        ++casos;
        if (ref.to_string(4) != otro.to_string(4))
        {
            std::printf("  [FALLA] %s: to_string de %lld  c2=%s otro=%s\n", como, k, ref.to_string(4).c_str(),
                        otro.to_string(4).c_str());
            ++fallos;
        }

        ++casos;
        if (ref.suelo().to_string() != otro.suelo().to_string())
        {
            std::printf("  [FALLA] %s: suelo de %lld  c2=%s otro=%s\n", como, k,
                        ref.suelo().to_string().c_str(), otro.suelo().to_string().c_str());
            ++fallos;
        }

        // La parte fraccionaria sale de enmascarar los F limbos BAJOS, y en
        // tramo 3 la equivocacion que se repitio seis veces fue justo esa: dar
        // por hecho que los limbos son el valor. En Exceso-K no lo son.
        ++casos;
        if (ref.parte_fraccionaria().to_string() != otro.parte_fraccionaria().to_string())
        {
            std::printf("  [FALLA] %s: parte fraccionaria de %lld  c2=%s otro=%s\n", como, k,
                        ref.parte_fraccionaria().to_string().c_str(),
                        otro.parte_fraccionaria().to_string().c_str());
            ++fallos;
        }

        ++casos;
        if (ref.is_negative() != otro.is_negative() || ref.is_zero() != otro.is_zero() ||
            ref.es_entero() != otro.es_entero())
        {
            std::printf("  [FALLA] %s: las preguntas de %lld no coinciden\n", como, k);
            ++fallos;
        }
    }

    /// Y lo mismo con dos operandos: suma, resta y el orden.
    template <typename Otro>
    void cruza_binaria(long long a, long long b, const char *como)
    {
        const Q ra{a}, rb{b};
        const Otro oa{a}, ob{b};

        ++casos;
        if ((ra + rb).to_string(4) != (oa + ob).to_string(4))
        {
            std::printf("  [FALLA] %s: %lld + %lld  c2=%s otro=%s\n", como, a, b,
                        (ra + rb).to_string(4).c_str(), (oa + ob).to_string(4).c_str());
            ++fallos;
        }

        ++casos;
        if ((ra - rb).to_string(4) != (oa - ob).to_string(4))
        {
            std::printf("  [FALLA] %s: %lld - %lld  c2=%s otro=%s\n", como, a, b,
                        (ra - rb).to_string(4).c_str(), (oa - ob).to_string(4).c_str());
            ++fallos;
        }

        ++casos;
        if ((ra < rb) != (oa < ob) || (ra == rb) != (oa == ob))
        {
            std::printf("  [FALLA] %s: el orden de %lld y %lld no coincide\n", como, a, b);
            ++fallos;
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
    std::printf("=== fixed_point_t: el tipo y las conversiones ===\n\n");

    // ---------------------------------------------------- la escala existe ---
    //
    // Lo primero y mas importante: que construir desde un entero SUBA a la
    // escala. Si esto falla, todo lo demas es ruido.
    std::printf("-- la escala\n");
    {
        const Q uno{1};
        comprueba(uno.crudo().limb(1) == 1U && uno.crudo().limb(0) == 0U, "Q{1} guarda 2^64, no 1");
        comprueba(Q::one() == uno, "one() es Q{1}");
        comprueba(Q::zero().is_zero(), "zero() es cero");
        comprueba(Q::epsilon().crudo() == Q::entero::one(), "epsilon() es el crudo 1");
        comprueba(Q::escala_bits == 64U, "Q64.64 escala 64 bits");
        comprueba(Q::limbos_enteros == 1U && Q::limbos_fraccionarios == 1U, "reparto de limbos");
    }

    // --------------------------------------------- crudo contra parte entera ---
    //
    // Los dos constructores tienen la misma firma y significan cosas distintas
    // por un factor 2^64. Que se noten distintos es el test.
    std::printf("-- desde_crudo no es lo mismo que el constructor\n");
    {
        const Q por_valor{3};
        const Q por_crudo = Q::desde_crudo(Q::entero{3});
        comprueba(por_valor != por_crudo, "Q{3} y desde_crudo(3) son distintos");
        comprueba(por_crudo == Q::epsilon() * Q::entero{3}, "desde_crudo(3) son tres epsilon");
    }

    // ------------------------------------------------------- las dos mitades ---
    std::printf("-- suelo y parte fraccionaria\n");
    {
        // 2.5 = crudo 2*2^64 + 2^63
        Q dos_y_medio = Q{2} + Q::desde_crudo(Q::entero{std::uint64_t{1}} << 63U);
        compara(dos_y_medio.suelo().to_string(), "2", "suelo de 2,5");
        comprueba(!dos_y_medio.es_entero(), "2,5 no es entero");
        comprueba(Q{7}.es_entero(), "7 si es entero");
        compara(Q{7}.suelo().to_string(), "7", "suelo de 7");

        // Y la asimetria declarada: el suelo va hacia -infinito, no hacia cero.
        const Q menos_dos_y_medio = -dos_y_medio;
        compara(menos_dos_y_medio.suelo().to_string(), "-3",
                "suelo de -2,5 es -3 (hacia -inf, como dice el @warning)");

        // La parte fraccionaria nunca es negativa: es el `r` de la formula del
        // redondeo, y de ahi que valga igual para negativos (ADR-019).
        comprueba(!menos_dos_y_medio.parte_fraccionaria().is_zero(),
                  "la parte fraccionaria de -2,5 no es cero");
    }

    // -------------------------------------------------------------- exactas ---
    std::printf("-- suma, resta y negacion (exactas: misma escala)\n");
    {
        comprueba(Q{2} + Q{3} == Q{5}, "2 + 3 = 5");
        comprueba(Q{5} - Q{3} == Q{2}, "5 - 3 = 2");
        comprueba(-Q{4} + Q{4} == Q::zero(), "-4 + 4 = 0");
        comprueba(Q{3} * Q::entero{4} == Q{12}, "3 * 4 = 12 (por entero, sin redondeo)");

        // Media mas media es uno: lo que el tipo aporta sobre el entero.
        const Q medio = Q::desde_crudo(Q::entero{std::uint64_t{1}} << 63U);
        comprueba(medio + medio == Q::one(), "1/2 + 1/2 = 1");
        comprueba(medio < Q::one() && Q::zero() < medio, "0 < 1/2 < 1");
    }

    // ------------------------------------------------------------- el orden ---
    std::printf("-- orden\n");
    {
        comprueba(Q{-5} < Q{-4}, "-5 < -4");
        comprueba(Q{-1} < Q::zero(), "-1 < 0");
        comprueba(Q::zero() < Q{1}, "0 < 1");
        comprueba(Q{3} <= Q{3} && Q{3} >= Q{3}, "3 <= 3 <= 3");
        comprueba((Q{2} <=> Q{7}) == std::strong_ordering::less, "2 <=> 7 es less");
    }

    // ----------------------------------------------------------- to_string ---
    std::printf("-- to_string\n");
    {
        compara(Q{0}.to_string(2), "0.00", "cero con dos decimales");
        compara(Q{7}.to_string(0), "7", "siete sin decimales");
        compara(Q{-7}.to_string(0), "-7", "menos siete");

        const Q medio = Q::desde_crudo(Q::entero{std::uint64_t{1}} << 63U);
        compara(medio.to_string(1), "0.5", "un medio");
        compara((Q{2} + medio).to_string(2), "2.50", "dos y medio");
        compara((-(Q{2} + medio)).to_string(2), "-2.50", "menos dos y medio");

        // Un cuarto y tres cuartos, que ejercitan el arrastre de cifras.
        const Q cuarto = Q::desde_crudo(Q::entero{std::uint64_t{1}} << 62U);
        compara(cuarto.to_string(2), "0.25", "un cuarto");
        compara((cuarto + medio).to_string(2), "0.75", "tres cuartos");
        compara((cuarto * Q::entero{3}).to_string(2), "0.75", "tres cuartos por producto entero");
    }

    // ----------------------------------------------- sin signo, y F == N ---
    std::printf("-- sin signo, y el tipo puramente fraccionario\n");
    {
        comprueba(QU{3} + QU{4} == QU{7}, "sin signo: 3 + 4 = 7");
        comprueba(!QU{5}.is_negative(), "sin signo nunca es negativo");

        // F == N: solo [0, 1). `one()` no existe alli, y el `static_assert` lo
        // dice; aqui se comprueba lo que si vale.
        using Frac = ufixed_point_t<1, 1>;
        const Frac medio = Frac::desde_crudo(Frac::entero{std::uint64_t{1}} << 63U);
        comprueba(Frac::limbos_enteros == 0U, "F == N deja cero limbos enteros");
        comprueba(medio.suelo().is_zero(), "en [0,1) el suelo siempre es cero");
        compara(medio.to_string(1), "0.5", "un medio en el tipo fraccionario");
    }

    // ------------------------------------------- las esquinas del reparto ---
    //
    // `F == N` ya esta arriba. Faltaban las otras dos, y **ninguna de las dos
    // la habrian encontrado los aleatorios**: hay que construirlas a mano.
    std::printf("-- las esquinas: F == 0, y el minimo\n");
    {
        // F == 0: "un entero con otro nombre", como dice el encabezado. La coma
        // no se imprime porque no hay nada detras.
        using E = sfixed_point_t<2, 0>;
        comprueba(E::escala_bits == 0U, "F == 0 no tiene escala");
        comprueba(E{7}.es_entero(), "con F == 0 todo es entero");
        compara(E{7}.to_string(3), "7", "F == 0 no pone coma aunque se pidan decimales");
        compara((-E{7}).to_string(2), "-7", "F == 0 con signo");

        // El minimo: negarlo ENVUELVE, y por ahi se colaba un segundo signo.
        comprueba(-Q::min() == Q::min(), "negar el minimo envuelve y da el minimo");
        comprueba(Q::min().is_negative(), "el minimo es negativo");
        compara(Q::min().to_string(2), "-9223372036854775808.00", "el minimo lleva UN signo, no dos");
        compara(Q::max().to_string(2), "9223372036854775807.99", "el maximo");
        compara(QU::min().to_string(2), "0.00", "sin signo el minimo es cero");
    }

    // ------------------------ las CUATRO representaciones, no dos (ADR-018) ---
    //
    // El resto del test usa `binnat` y complemento a dos. Magnitud-Signo y
    // Exceso-K son las otras dos que ADR-019 mete en el punto fijo, y aqui se
    // cruzan contra complemento a dos: por ADR-018 tienen que dar **lo mismo en
    // todo**, porque son codificaciones y no aritmeticas.
    //
    // Importa sobre todo `parte_fraccionaria()`, que enmascara los F limbos
    // bajos: en Exceso-K los limbos **no son** el valor, y esa es exactamente la
    // equivocacion que el tramo 3 se encontro seis veces.
    std::printf("-- Magnitud-Signo y Exceso-K, cruzados contra complemento a dos\n");
    {
        // Las esquinas a mano. Los aleatorios vienen despues, pero no bastan.
        const long long valores[] = {0,   1,    -1,  2,    -2,   3,     -3,    7,      -7,      8,       -8,
                                     255, -255, 256, -256, 1000, -1000, 65535, -65535, 1000000, -1000000};

        for (long long k : valores)
        {
            cruza_codificacion<Qms>(k, "MS");
            cruza_codificacion<Qek>(k, "EK");
        }

        for (long long a : valores)
            for (long long b : valores)
            {
                cruza_binaria<Qms>(a, b, "MS");
                cruza_binaria<Qek>(a, b, "EK");
            }

        // El cero, que en Exceso-K NO tiene los limbos a cero. Es el fallo que
        // el tramo 3 llamo el mas instructivo: `T x{}` daba -2^127.
        comprueba(Qek{}.is_zero(), "en Exceso-K el punto fijo por defecto es cero");
        comprueba(Qms{}.is_zero(), "en Magnitud-Signo tambien");
        comprueba(Qek::zero() == Qek{}, "zero() y el constructor por defecto coinciden (EK)");

        // Y las medias, que es lo que el tipo aporta sobre el entero: el valor
        // no esta en los limbos, hay que construirlo por la escala.
        const Qms medio_ms = Qms::desde_crudo(Qms::entero::one() << 63U);
        const Qek medio_ek = Qek::desde_crudo(Qek::entero::one() << 63U);
        compara(medio_ms.to_string(1), "0.5", "un medio en Magnitud-Signo");
        compara(medio_ek.to_string(1), "0.5", "un medio en Exceso-K");
        comprueba(medio_ms + medio_ms == Qms::one(), "1/2 + 1/2 = 1 en Magnitud-Signo");
        comprueba(medio_ek + medio_ek == Qek::one(), "1/2 + 1/2 = 1 en Exceso-K");
        compara((-(Qms{2} + medio_ms)).to_string(2), "-2.50", "menos dos y medio en MS");
        compara((-(Qek{2} + medio_ek)).to_string(2), "-2.50", "menos dos y medio en EK");
    }

    // --------------------- y ahora con parte fraccionaria DE VERDAD ---------
    //
    // Lo de arriba, construido desde enteros, dejaba el limbo bajo a cero en las
    // tres representaciones, asi que `parte_fraccionaria()` --que enmascara los
    // F limbos bajos-- no se ejercitaba. Se comprobo **rompiendo a proposito**
    // la conversion --copiar limbos en vez de convertir el valor, que es la
    // equivocacion que el tramo 3 se encontro seis veces-- y viendo que las 4009
    // comprobaciones seguian pasando enteras.
    //
    // Y no vale cualquier fraccion: un medio es 2^63, que **es su propio
    // complemento a dos**, asi que en Magnitud-Signo el limbo bajo de -2,5 sale
    // igual que en complemento a dos por casualidad. Hace falta algo como un
    // cuarto, donde 2^62 y su complemento 0xC000... son distintos.
    std::printf("-- MS y EK con parte fraccionaria, que es donde se nota\n");
    {
        const std::uint64_t fracciones[] = {
            std::uint64_t{1},               // el epsilon
            std::uint64_t{1} << 62U,        // un cuarto: NO es su complemento
            std::uint64_t{3} << 62U,        // tres cuartos
            std::uint64_t{1} << 63U,        // un medio: el caso que enganaba
            (std::uint64_t{1} << 63U) | 1U, // medio mas un epsilon
            ~std::uint64_t{0},              // el mayor: 1 - epsilon
            std::uint64_t{0x0123456789ABCDEFULL},
        };
        const long long enteros[] = {0, 1, -1, 2, -2, 7, -7, 1000, -1000};

        for (long long k : enteros)
            for (std::uint64_t m : fracciones)
            {
                const Q ref = con_fraccion<Q>(k, m);
                const Qms ms = con_fraccion<Qms>(k, m);
                const Qek ek = con_fraccion<Qek>(k, m);

                ++casos;
                if (ref.parte_fraccionaria().to_string() != ms.parte_fraccionaria().to_string())
                {
                    std::printf("  [FALLA] MS: fraccion de %lld + %llu/2^64  c2=%s ms=%s\n", k,
                                static_cast<unsigned long long>(m),
                                ref.parte_fraccionaria().to_string().c_str(),
                                ms.parte_fraccionaria().to_string().c_str());
                    ++fallos;
                }

                ++casos;
                if (ref.parte_fraccionaria().to_string() != ek.parte_fraccionaria().to_string())
                {
                    std::printf("  [FALLA] EK: fraccion de %lld + %llu/2^64  c2=%s ek=%s\n", k,
                                static_cast<unsigned long long>(m),
                                ref.parte_fraccionaria().to_string().c_str(),
                                ek.parte_fraccionaria().to_string().c_str());
                    ++fallos;
                }

                ++casos;
                if (ref.to_string(6) != ms.to_string(6) || ref.to_string(6) != ek.to_string(6))
                {
                    std::printf("  [FALLA] to_string de %lld + %llu/2^64  c2=%s ms=%s ek=%s\n", k,
                                static_cast<unsigned long long>(m), ref.to_string(6).c_str(),
                                ms.to_string(6).c_str(), ek.to_string(6).c_str());
                    ++fallos;
                }

                ++casos;
                if (ref.suelo().to_string() != ms.suelo().to_string() ||
                    ref.suelo().to_string() != ek.suelo().to_string())
                {
                    std::printf("  [FALLA] suelo de %lld + %llu/2^64  c2=%s ms=%s ek=%s\n", k,
                                static_cast<unsigned long long>(m), ref.suelo().to_string().c_str(),
                                ms.suelo().to_string().c_str(), ek.suelo().to_string().c_str());
                    ++fallos;
                }

                ++casos;
                if (ref.es_entero() != ms.es_entero() || ref.es_entero() != ek.es_entero())
                {
                    std::printf("  [FALLA] es_entero de %lld + %llu/2^64 no coincide\n", k,
                                static_cast<unsigned long long>(m));
                    ++fallos;
                }
            }

        // Y los aleatorios, DESPUES de las esquinas y no en su lugar.
        xorshift rng{0xFACC10ULL};
        for (int v = 0; v < 300; ++v)
        {
            const long long k = static_cast<long long>(rng() % 200001) - 100000;
            const std::uint64_t m = rng();
            const Q ref = con_fraccion<Q>(k, m);
            const Qms ms = con_fraccion<Qms>(k, m);
            const Qek ek = con_fraccion<Qek>(k, m);
            ++casos;
            if (ref.to_string(8) != ms.to_string(8) || ref.to_string(8) != ek.to_string(8) ||
                ref.parte_fraccionaria().to_string() != ms.parte_fraccionaria().to_string() ||
                ref.parte_fraccionaria().to_string() != ek.parte_fraccionaria().to_string())
            {
                std::printf("  [FALLA] aleatorio %lld + %llu/2^64\n", k, static_cast<unsigned long long>(m));
                ++fallos;
            }
        }
    }

    // ------------------------------ el quinto eje: la politica ------------
    //
    // El tipo tiene cinco parametros --N, F, Sign, Form, Policy-- y el test
    // cruzaba cuatro. `Policy` se hereda del entero, pero «se hereda» es
    // exactamente la clase de afirmacion que hay que comprobar: es la que fallo
    // seis veces en el tramo 3.
    std::printf("-- la politica: checked marca, y la marca se ve desde aqui\n");
    {
        using Qc = fixed_point_t<2, 1, signedness::signed_type, representation_form::twos_complement,
                                 overflow_policy::checked>;

        comprueba(Qc::policy == overflow_policy::checked, "la politica se publica");
        comprueba(Qc{7}.valid(), "un valor recien construido es valido");
        comprueba(Q{7}.valid(), "con wrap, valid() siempre es true");

        // Desbordar por arriba tiene que marcar, y la marca tiene que verse
        // desde el punto fijo y no solo desde el entero de abajo.
        const Qc desbordado = Qc::max() + Qc::epsilon();
        comprueba(!desbordado.valid(), "pasarse del maximo marca");
        comprueba(!desbordado.crudo().valid(), "y la marca esta en el entero de abajo");

        // Y es pegajosa: volver al rango no la limpia.
        comprueba(!(desbordado - Qc::epsilon()).valid(), "la marca no se limpia al volver");

        // Por abajo igual.
        const Qc hundido = Qc::min() - Qc::epsilon();
        comprueba(!hundido.valid(), "pasarse del minimo marca");

        // Lo que NO debe marcar: una suma que cabe.
        comprueba((Qc{1000} + Qc{2000}).valid(), "una suma que cabe no marca");
        comprueba((-Qc{5}).valid(), "negar un valor corriente no marca");

        // Sin signo, restar por debajo de cero es el caso clasico.
        using QUc = ufixed_point_t<2, 1, overflow_policy::checked>;
        comprueba(!(QUc{3} - QUc{4}).valid(), "sin signo, 3 - 4 marca");
        comprueba((QUc{4} - QUc{3}).valid(), "y 4 - 3 no");
    }

    // -------------------------------------------- contra el oraculo exacto ---
#ifdef __SIZEOF_INT128__
    std::printf("-- contra el oraculo: aritmetica exacta en __int128\n");
    {
        const long long esquinas[] = {0, 1, -1, 2, -2, 3, -3, 7, -7, 100, -100, 12345, -12345};
        for (long long a : esquinas)
            for (long long b : esquinas)
                cruza_suma(a, b);

        xorshift rng{0xF1EDULL};
        for (int v = 0; v < 400; ++v)
        {
            const long long a = static_cast<long long>(rng() % 2000001) - 1000000;
            const long long b = static_cast<long long>(rng() % 2000001) - 1000000;
            cruza_suma(a, b);
        }
    }
#else
    std::printf("-- (sin __int128: el oraculo no corre en este compilador)\n");
#endif

    // ------------------------------------------------ vale en compilacion ---
    {
        constexpr Q a{2};
        constexpr Q b{3};
        static_assert(a + b == Q{5}, "la suma tiene que valer en evaluacion constante");
        static_assert(Q::one().crudo().limb(1) == 1U, "y la escala tambien");
    }

    std::printf("\n%lld comprobaciones, %d fallos\n", casos, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
