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
#include <limits>
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

    // ---- TABLA:INICIO -- generada por scripts/genera_tabla_redondeo.py ----
    // clang-format off
    // GENERADO por scripts/genera_tabla_redondeo.py -- no editar a mano.
    //
    // Oraculo de Q64.64 en aritmetica exacta de Python. El otro oraculo, el de
    // `__int128` sobre los tipos de un limbo, esta escrito a mano mas abajo: dos
    // caminos independientes sobre dos anchuras distintas.

    constexpr int kModos = 5;  // los del enum `rounding_mode`, en su orden

    struct CasoBin
    {
        const char *nombre;
        std::uint64_t a[2];
        std::uint64_t b[2];
        std::uint64_t esperado[kModos][2];
    };

    constexpr CasoBin kProductos[] = {
        {"uno por uno", {0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0x0000000000000001ULL},
         {{0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0x0000000000000001ULL}}},
        {"dos por tres", {0x0000000000000000ULL, 0x0000000000000002ULL}, {0x0000000000000000ULL, 0x0000000000000003ULL},
         {{0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}}},
        {"medio por medio", {0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0x4000000000000000ULL, 0x0000000000000000ULL}, {0x4000000000000000ULL, 0x0000000000000000ULL}, {0x4000000000000000ULL, 0x0000000000000000ULL}, {0x4000000000000000ULL, 0x0000000000000000ULL}, {0x4000000000000000ULL, 0x0000000000000000ULL}}},
        {"menos medio por medio", {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0xC000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, {0xC000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, {0xC000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, {0xC000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, {0xC000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}}},
        {"cuarto por cuarto", {0x4000000000000000ULL, 0x0000000000000000ULL}, {0x4000000000000000ULL, 0x0000000000000000ULL},
         {{0x1000000000000000ULL, 0x0000000000000000ULL}, {0x1000000000000000ULL, 0x0000000000000000ULL}, {0x1000000000000000ULL, 0x0000000000000000ULL}, {0x1000000000000000ULL, 0x0000000000000000ULL}, {0x1000000000000000ULL, 0x0000000000000000ULL}}},
        {"empate: epsilon por medio", {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}}},
        {"empate: 3 epsilon por medio", {0x0000000000000003ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0x0000000000000002ULL, 0x0000000000000000ULL}, {0x0000000000000002ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000002ULL, 0x0000000000000000ULL}}},
        {"empate negativo: -epsilon por medio", {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0x0000000000000000ULL, 0x0000000000000000ULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}}},
        {"empate negativo: -3 eps por medio", {0xFFFFFFFFFFFFFFFDULL, 0xFFFFFFFFFFFFFFFFULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}}},
        {"empate: (uno+eps) por medio", {0x0000000000000001ULL, 0x0000000000000001ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000001ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000001ULL, 0x0000000000000000ULL}}},
        {"bajo la mitad", {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x7FFFFFFFFFFFFFFFULL, 0x0000000000000000ULL},
         {{0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}}},
        {"sobre la mitad", {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x8000000000000001ULL, 0x0000000000000000ULL},
         {{0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}}},
        {"negativo bajo la mitad", {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x7FFFFFFFFFFFFFFFULL, 0x0000000000000000ULL},
         {{0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}}},
        {"negativo sobre la mitad", {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x8000000000000001ULL, 0x0000000000000000ULL},
         {{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}}},
        {"empate con suelo par", {0x0000000000000002ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}}},
        {"empate con suelo impar", {0x0000000000000003ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0x0000000000000002ULL, 0x0000000000000000ULL}, {0x0000000000000002ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000002ULL, 0x0000000000000000ULL}}},
        {"empate suelo par negativo", {0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}}},
        {"empate suelo impar negativo", {0xFFFFFFFFFFFFFFFDULL, 0xFFFFFFFFFFFFFFFFULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {{0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}}},
        {"1.5 por 1.5", {0x8000000000000000ULL, 0x0000000000000001ULL}, {0x8000000000000000ULL, 0x0000000000000001ULL},
         {{0x4000000000000000ULL, 0x0000000000000002ULL}, {0x4000000000000000ULL, 0x0000000000000002ULL}, {0x4000000000000000ULL, 0x0000000000000002ULL}, {0x4000000000000000ULL, 0x0000000000000002ULL}, {0x4000000000000000ULL, 0x0000000000000002ULL}}},
        {"-1.5 por 2.25", {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFEULL}, {0x4000000000000000ULL, 0x0000000000000002ULL},
         {{0xA000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0xA000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0xA000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0xA000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0xA000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}}},
        {"7.25 por -3.75", {0x4000000000000000ULL, 0x0000000000000007ULL}, {0x4000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL},
         {{0xD000000000000000ULL, 0xFFFFFFFFFFFFFFE4ULL}, {0xD000000000000000ULL, 0xFFFFFFFFFFFFFFE4ULL}, {0xD000000000000000ULL, 0xFFFFFFFFFFFFFFE4ULL}, {0xD000000000000000ULL, 0xFFFFFFFFFFFFFFE4ULL}, {0xD000000000000000ULL, 0xFFFFFFFFFFFFFFE4ULL}}},
        {"grande por pequeno", {0x0000000000000000ULL, 0x00000000000F4240ULL}, {0x0000000000000003ULL, 0x0000000000000000ULL},
         {{0x00000000002DC6C0ULL, 0x0000000000000000ULL}, {0x00000000002DC6C0ULL, 0x0000000000000000ULL}, {0x00000000002DC6C0ULL, 0x0000000000000000ULL}, {0x00000000002DC6C0ULL, 0x0000000000000000ULL}, {0x00000000002DC6C0ULL, 0x0000000000000000ULL}}},
    };

    constexpr CasoBin kDivisiones[] = {
        {"uno entre tres", {0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0x0000000000000003ULL},
         {{0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555556ULL, 0x0000000000000000ULL}}},
        {"menos uno entre tres", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000003ULL},
         {{0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAAAULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}}},
        {"uno entre menos tres", {0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL},
         {{0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAAAULL, 0xFFFFFFFFFFFFFFFFULL}, {0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}}},
        {"menos uno entre menos tres", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL},
         {{0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555555ULL, 0x0000000000000000ULL}, {0x5555555555555556ULL, 0x0000000000000000ULL}}},
        {"dos entre uno", {0x0000000000000000ULL, 0x0000000000000002ULL}, {0x0000000000000000ULL, 0x0000000000000001ULL},
         {{0x0000000000000000ULL, 0x0000000000000002ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL}}},
        {"uno entre dos", {0x0000000000000000ULL, 0x0000000000000001ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {{0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL}}},
        {"epsilon entre dos", {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {{0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}}},
        {"menos epsilon entre dos", {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {{0x0000000000000000ULL, 0x0000000000000000ULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}, {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, {0x0000000000000000ULL, 0x0000000000000000ULL}}},
        {"tres epsilon entre dos", {0x0000000000000003ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {{0x0000000000000002ULL, 0x0000000000000000ULL}, {0x0000000000000002ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000002ULL, 0x0000000000000000ULL}}},
        {"siete entre dos", {0x0000000000000000ULL, 0x0000000000000007ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {{0x8000000000000000ULL, 0x0000000000000003ULL}, {0x8000000000000000ULL, 0x0000000000000003ULL}, {0x8000000000000000ULL, 0x0000000000000003ULL}, {0x8000000000000000ULL, 0x0000000000000003ULL}, {0x8000000000000000ULL, 0x0000000000000003ULL}}},
        {"menos siete entre dos", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFF9ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {{0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}}},
        {"10 entre 4", {0x0000000000000000ULL, 0x000000000000000AULL}, {0x0000000000000000ULL, 0x0000000000000004ULL},
         {{0x8000000000000000ULL, 0x0000000000000002ULL}, {0x8000000000000000ULL, 0x0000000000000002ULL}, {0x8000000000000000ULL, 0x0000000000000002ULL}, {0x8000000000000000ULL, 0x0000000000000002ULL}, {0x8000000000000000ULL, 0x0000000000000002ULL}}},
        {"-10 entre 4", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFF6ULL}, {0x0000000000000000ULL, 0x0000000000000004ULL},
         {{0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL}, {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL}}},
        {"1.5 entre 0.25", {0x8000000000000000ULL, 0x0000000000000001ULL}, {0x4000000000000000ULL, 0x0000000000000000ULL},
         {{0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}, {0x0000000000000000ULL, 0x0000000000000006ULL}}},
        {"cien entre siete", {0x0000000000000000ULL, 0x0000000000000064ULL}, {0x0000000000000000ULL, 0x0000000000000007ULL},
         {{0x4924924924924925ULL, 0x000000000000000EULL}, {0x4924924924924925ULL, 0x000000000000000EULL}, {0x4924924924924924ULL, 0x000000000000000EULL}, {0x4924924924924924ULL, 0x000000000000000EULL}, {0x4924924924924925ULL, 0x000000000000000EULL}}},
        {"-cien entre siete", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFF9CULL}, {0x0000000000000000ULL, 0x0000000000000007ULL},
         {{0xB6DB6DB6DB6DB6DBULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DBULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DCULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DBULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DCULL, 0xFFFFFFFFFFFFFFF1ULL}}},
        {"cien entre menos siete", {0x0000000000000000ULL, 0x0000000000000064ULL}, {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFF9ULL},
         {{0xB6DB6DB6DB6DB6DBULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DBULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DCULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DBULL, 0xFFFFFFFFFFFFFFF1ULL}, {0xB6DB6DB6DB6DB6DCULL, 0xFFFFFFFFFFFFFFF1ULL}}},
    };

    // El resto es EXACTO, asi que lleva UN valor y no cinco (ADR-020).
    struct CasoResto
    {
        const char *nombre;
        std::uint64_t a[2];
        std::uint64_t b[2];
        std::uint64_t esperado[2];
    };

    constexpr CasoResto kRestos[] = {
        {"7 mod 2", {0x0000000000000000ULL, 0x0000000000000007ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {0x0000000000000000ULL, 0x0000000000000001ULL}},
        {"-7 mod 2", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFF9ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}},
        {"7 mod -2", {0x0000000000000000ULL, 0x0000000000000007ULL}, {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFEULL},
         {0x0000000000000000ULL, 0x0000000000000001ULL}},
        {"-7 mod -2", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFF9ULL}, {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFEULL},
         {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}},
        {"5.5 mod 2", {0x8000000000000000ULL, 0x0000000000000005ULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {0x8000000000000000ULL, 0x0000000000000001ULL}},
        {"-5.5 mod 2", {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFAULL}, {0x0000000000000000ULL, 0x0000000000000002ULL},
         {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFEULL}},
        {"0.75 mod 0.5", {0xC000000000000000ULL, 0x0000000000000000ULL}, {0x8000000000000000ULL, 0x0000000000000000ULL},
         {0x4000000000000000ULL, 0x0000000000000000ULL}},
        {"epsilon mod uno", {0x0000000000000001ULL, 0x0000000000000000ULL}, {0x0000000000000000ULL, 0x0000000000000001ULL},
         {0x0000000000000001ULL, 0x0000000000000000ULL}},
    };

    struct CasoCadena
    {
        const char *nombre;
        std::uint64_t a[2];
        unsigned decimales;
        const char *esperado[kModos];
    };

    constexpr CasoCadena kCadenas[] = {
        {"cero, dos cifras", {0x0000000000000000ULL, 0x0000000000000000ULL}, 2,
         {"0.00", "0.00", "0.00", "0.00", "0.00"}},
        {"siete sin cifras", {0x0000000000000000ULL, 0x0000000000000007ULL}, 0,
         {"7", "7", "7", "7", "7"}},
        {"menos siete sin cifras", {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFF9ULL}, 0,
         {"-7", "-7", "-7", "-7", "-7"}},
        {"un medio, una cifra", {0x8000000000000000ULL, 0x0000000000000000ULL}, 1,
         {"0.5", "0.5", "0.5", "0.5", "0.5"}},
        {"dos y medio", {0x8000000000000000ULL, 0x0000000000000002ULL}, 2,
         {"2.50", "2.50", "2.50", "2.50", "2.50"}},
        {"menos dos y medio", {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL}, 2,
         {"-2.50", "-2.50", "-2.50", "-2.50", "-2.50"}},
        {"empate 2,5 sin cifras", {0x8000000000000000ULL, 0x0000000000000002ULL}, 0,
         {"2", "3", "2", "2", "3"}},
        {"empate 3,5 sin cifras", {0x8000000000000000ULL, 0x0000000000000003ULL}, 0,
         {"4", "4", "3", "3", "4"}},
        {"empate -2,5 sin cifras", {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFDULL}, 0,
         {"-2", "-3", "-2", "-3", "-2"}},
        {"empate -3,5 sin cifras", {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFCULL}, 0,
         {"-4", "-4", "-3", "-4", "-3"}},
        {"empate 0,5 sin cifras", {0x8000000000000000ULL, 0x0000000000000000ULL}, 0,
         {"0", "1", "0", "0", "1"}},
        {"empate -0,5 sin cifras", {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, 0,
         {"-0", "-1", "-0", "-1", "-0"}},
        {"empate 0,25 con una cifra", {0x4000000000000000ULL, 0x0000000000000000ULL}, 1,
         {"0.2", "0.3", "0.2", "0.2", "0.3"}},
        {"empate -0,25 con una cifra", {0xC000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, 1,
         {"-0.2", "-0.3", "-0.2", "-0.3", "-0.2"}},
        {"casi uno, dos cifras", {0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL}, 2,
         {"1.00", "1.00", "0.99", "0.99", "1.00"}},
        {"casi diez, dos cifras", {0xFFFFFFFFFFFFFFFFULL, 0x0000000000000009ULL}, 2,
         {"10.00", "10.00", "9.99", "9.99", "10.00"}},
        {"casi menos diez, dos cifras", {0x0000000000000001ULL, 0xFFFFFFFFFFFFFFF6ULL}, 2,
         {"-10.00", "-10.00", "-9.99", "-10.00", "-9.99"}},
        {"casi cien, una cifra", {0xFFFFFFFFFFFFFFFFULL, 0x0000000000000063ULL}, 1,
         {"100.0", "100.0", "99.9", "99.9", "100.0"}},
        {"un epsilon negativo, dos cifras", {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, 2,
         {"-0.00", "-0.00", "-0.00", "-0.01", "-0.00"}},
        {"un epsilon positivo, dos cifras", {0x0000000000000001ULL, 0x0000000000000000ULL}, 2,
         {"0.00", "0.00", "0.00", "0.00", "0.01"}},
        {"menos tres cuartos, una cifra", {0x4000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL}, 1,
         {"-0.8", "-0.8", "-0.7", "-0.8", "-0.7"}},
        {"un tercio aproximado, 8 cifras", {0x5555555555555555ULL, 0x0000000000000000ULL}, 8,
         {"0.33333333", "0.33333333", "0.33333333", "0.33333333", "0.33333334"}},
        {"menos un tercio, 8 cifras", {0xAAAAAAAAAAAAAAABULL, 0xFFFFFFFFFFFFFFFFULL}, 8,
         {"-0.33333333", "-0.33333333", "-0.33333333", "-0.33333334", "-0.33333333"}},
        {"mil y pico, 4 cifras", {0x4000000000000000ULL, 0x00000000000003E8ULL}, 4,
         {"1000.2500", "1000.2500", "1000.2500", "1000.2500", "1000.2500"}},
        {"epsilon, 20 cifras", {0x0000000000000001ULL, 0x0000000000000000ULL}, 20,
         {"0.00000000000000000005", "0.00000000000000000005", "0.00000000000000000005", "0.00000000000000000005", "0.00000000000000000006"}},
    };
    // clang-format on
    // ---- TABLA:FIN ----

    // =====================================================================
    // Oraculo propio: `__int128` sobre los tipos de UN limbo
    // =====================================================================
    //
    // Para Q64.64 el producto de dos crudos son 256 bits y no hay tipo nativo
    // donde calcularlo; por eso aquella tabla se genera en Python. Pero con
    // `N = F = 1` los crudos son de 64 bits, el producto cabe en 128, y el
    // oraculo se puede escribir **aqui**, sin generar nada.
    //
    // Son dos caminos independientes sobre dos anchuras. Con uno solo no habria
    // forma de separar «el redondeo esta bien» de «esta bien en la anchura que
    // mire».

#ifdef __SIZEOF_INT128__
    /// La tabla de ADR-020, escrita otra vez. Que este repetida es el punto:
    /// llamar a la del C++ seria comprobar que una funcion es igual a si misma.
    bool sube_oraculo(nstd::rounding_mode modo, bool resto_cero, bool pasa_mitad, bool empate, bool q_impar,
                      bool q_negativo)
    {
        if (resto_cero)
            return false;
        switch (modo)
        {
            case nstd::rounding_mode::to_nearest_even:
                return pasa_mitad || (empate && q_impar);
            case nstd::rounding_mode::to_nearest_away:
                return pasa_mitad || (empate && !q_negativo);
            case nstd::rounding_mode::toward_zero:
                return q_negativo;
            case nstd::rounding_mode::toward_neg_inf:
                return false;
            case nstd::rounding_mode::toward_pos_inf:
                return true;
        }
        return false;
    }

    /// Crudo esperado de `a * b` con `N = F = 1`, en aritmetica exacta de 128 bits.
    std::int64_t producto_oraculo(std::int64_t a, std::int64_t b, nstd::rounding_mode modo)
    {
        const __int128 p = static_cast<__int128>(a) * static_cast<__int128>(b);
        const __int128 q = p >> 64; // suelo
        const std::uint64_t r = static_cast<std::uint64_t>(static_cast<unsigned __int128>(p));
        const std::uint64_t mitad = std::uint64_t{1} << 63U;
        const bool inc = sube_oraculo(modo, r == 0, r > mitad, r == mitad,
                                      (static_cast<std::uint64_t>(q) & 1U) != 0U, q < 0);
        return static_cast<std::int64_t>(static_cast<std::uint64_t>(q) + (inc ? 1U : 0U));
    }

    /// Crudo esperado de `a / b` con `N = F = 1`.
    std::int64_t division_oraculo(std::int64_t a, std::int64_t b, nstd::rounding_mode modo)
    {
        const __int128 num = static_cast<__int128>(a) << 64;
        const __int128 den = static_cast<__int128>(b);

        __int128 q = num / den;  // C++ trunca hacia cero
        __int128 rt = num % den; // resto con el signo del dividendo
        if (rt != 0 && ((num < 0) != (den < 0)))
        {
            // de cociente truncado a suelo
            --q;
            rt += den;
        }
        const unsigned __int128 rm = static_cast<unsigned __int128>(rt < 0 ? -rt : rt);
        const unsigned __int128 dm = static_cast<unsigned __int128>(den < 0 ? -den : den);

        const bool inc = sube_oraculo(modo, rm == 0, 2 * rm > dm, 2 * rm == dm,
                                      (static_cast<std::uint64_t>(q) & 1U) != 0U, q < 0);
        return static_cast<std::int64_t>(static_cast<std::uint64_t>(q) + (inc ? 1U : 0U));
    }
#endif

    // --- ayudantes del redondeo, definidos tras los oraculos ---------------

    /// Instancia Q64.64 con el modo `M` y devuelve el crudo de `a op b`.
    template <nstd::rounding_mode M>
    void mira_tabla(const std::uint64_t a[2], const std::uint64_t b[2], const std::uint64_t esp[2],
                    const char *nombre, const char *op)
    {
        using T = nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, M>;
        typename T::entero ea{}, eb{};
        ea.set_limb(0, a[0]);
        ea.set_limb(1, a[1]);
        eb.set_limb(0, b[0]);
        eb.set_limb(1, b[1]);
        const T x = T::desde_crudo(ea);
        const T y = T::desde_crudo(eb);

        const T r = (op[0] == '*') ? (x * y) : ((op[0] == '/') ? (x / y) : (x % y));

        ++casos;
        if (r.crudo().limb(0) != esp[0] || r.crudo().limb(1) != esp[1])
        {
            std::printf("  [FALLA] %s %s modo %d: esperado %016llX%016llX, obtenido %016llX%016llX\n", nombre,
                        op, static_cast<int>(M), static_cast<unsigned long long>(esp[1]),
                        static_cast<unsigned long long>(esp[0]),
                        static_cast<unsigned long long>(r.crudo().limb(1)),
                        static_cast<unsigned long long>(r.crudo().limb(0)));
            ++fallos;
        }
    }

    void cruza_tabla_producto(const CasoBin &c)
    {
        mira_tabla<nstd::rounding_mode::to_nearest_even>(c.a, c.b, c.esperado[0], c.nombre, "*");
        mira_tabla<nstd::rounding_mode::to_nearest_away>(c.a, c.b, c.esperado[1], c.nombre, "*");
        mira_tabla<nstd::rounding_mode::toward_zero>(c.a, c.b, c.esperado[2], c.nombre, "*");
        mira_tabla<nstd::rounding_mode::toward_neg_inf>(c.a, c.b, c.esperado[3], c.nombre, "*");
        mira_tabla<nstd::rounding_mode::toward_pos_inf>(c.a, c.b, c.esperado[4], c.nombre, "*");
    }

    void cruza_tabla_division(const CasoBin &c)
    {
        mira_tabla<nstd::rounding_mode::to_nearest_even>(c.a, c.b, c.esperado[0], c.nombre, "/");
        mira_tabla<nstd::rounding_mode::to_nearest_away>(c.a, c.b, c.esperado[1], c.nombre, "/");
        mira_tabla<nstd::rounding_mode::toward_zero>(c.a, c.b, c.esperado[2], c.nombre, "/");
        mira_tabla<nstd::rounding_mode::toward_neg_inf>(c.a, c.b, c.esperado[3], c.nombre, "/");
        mira_tabla<nstd::rounding_mode::toward_pos_inf>(c.a, c.b, c.esperado[4], c.nombre, "/");
    }

    /// Instancia Q64.64 con el modo `M` y compara la cadena.
    template <nstd::rounding_mode M>
    void mira_cadena(const CasoCadena &c, const char *esp)
    {
        using T = nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, M>;
        typename T::entero ea{};
        ea.set_limb(0, c.a[0]);
        ea.set_limb(1, c.a[1]);

        const std::string obtenido = T::desde_crudo(ea).to_string(c.decimales);
        ++casos;
        if (obtenido != esp)
        {
            std::printf("  [FALLA] cadena %s modo %d con %u cifras: esperado %s, obtenido %s\n",
                        c.nombre, static_cast<int>(M), c.decimales, esp, obtenido.c_str());
            ++fallos;
        }
    }

    /// `x >> n` contra `x / 2^n`, en el modo `M`.
    ///
    /// Son dos caminos distintos dentro de la misma cabecera: el
    /// desplazamiento enmascara los `n` bits bajos, y la division preescala y
    /// llama a `divmod` en el tipo ancho. Si coinciden en los cinco modos y en
    /// los dos signos, es que el marco (q, r, d) esta bien puesto en los dos.
    template <nstd::rounding_mode M>
    void cruza_desplazamientos()
    {
        using T = nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, M>;

        // OJO con los valores. Construido desde un entero, el crudo es `v*2^64`
        // y lleva **64 bits bajos a cero**: desplazar diez posiciones no pierde
        // nada, los cinco modos coinciden y el cruce no comprueba el redondeo.
        // Aqui se trabaja con CRUDOS que tienen bits bajos, y para cada `n` se
        // incluyen los tres casos que decide la perilla.
        for (unsigned n = 1; n <= 10; ++n)
        {
            const std::uint64_t paso = std::uint64_t{1} << n;
            const std::uint64_t mitad = std::uint64_t{1} << (n - 1U);

            const std::uint64_t bajos[] = {
                0U,             // exacto: los cinco modos tienen que coincidir
                mitad,          // EMPATE
                mitad - 1U,     // justo por debajo
                mitad + 1U,     // justo por encima  (con n=1 coincide con paso)
                1U,
                paso - 1U,
            };
            const long long enteros[] = {0, 1, -1, 2, -2, 3, -3, 100, -100};

            // `2^n` como punto fijo, para la division.
            T dos_a_la_n = T::one();
            for (unsigned i = 0; i < n; ++i)
                dos_a_la_n = dos_a_la_n + dos_a_la_n;

            for (long long v : enteros)
                for (std::uint64_t b : bajos)
            {
                // El crudo: la parte entera `v` mas unos bits bajos. Para los
                // negativos se construye por resta, que es lo que deja el
                // complemento a dos correcto sin escribirlo a mano.
                const T base{v};
                const T x = T::desde_crudo(base.crudo() + typename T::entero{b});

                const T por_desplazamiento = x >> n;
                const T por_division = x / dos_a_la_n;

                ++casos;
                if (por_desplazamiento != por_division)
                {
                    std::printf("  [FALLA] (%lld,%llu) >> %u no es lo mismo que / 2^%u (modo %d)\n",
                                v, static_cast<unsigned long long>(b), n, n, static_cast<int>(M));
                    ++fallos;
                }

                // `<<` es exacto, asi que deshace a `>>`: los bits bajos que
                // `<<` mete son ceros, y volver no pierde nada.
                ++casos;
                if (((x << n) >> n) != x)
                {
                    std::printf("  [FALLA] ((%lld,%llu) << %u) >> %u no vuelve (modo %d)\n", v,
                                static_cast<unsigned long long>(b), n, n, static_cast<int>(M));
                    ++fallos;
                }
            }
        }
    }

    void cruza_tabla_cadena(const CasoCadena &c)
    {
        mira_cadena<nstd::rounding_mode::to_nearest_even>(c, c.esperado[0]);
        mira_cadena<nstd::rounding_mode::to_nearest_away>(c, c.esperado[1]);
        mira_cadena<nstd::rounding_mode::toward_zero>(c, c.esperado[2]);
        mira_cadena<nstd::rounding_mode::toward_neg_inf>(c, c.esperado[3]);
        mira_cadena<nstd::rounding_mode::toward_pos_inf>(c, c.esperado[4]);
    }

    void cruza_tabla_resto(const CasoResto &c)
    {
        // El resto es exacto: el mismo esperado para todos los modos, y se
        // comprueba con dos de ellos para que eso quede dicho por el test.
        mira_tabla<nstd::rounding_mode::to_nearest_even>(c.a, c.b, c.esperado, c.nombre, "%");
        mira_tabla<nstd::rounding_mode::toward_neg_inf>(c.a, c.b, c.esperado, c.nombre, "%");
    }

    /// ¿Dan lo mismo los cinco modos para un producto que sale exacto?
    bool coinciden_los_cinco(long long a, long long b)
    {
        using E =
            nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, nstd::rounding_mode::to_nearest_even>;
        using A =
            nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, nstd::rounding_mode::to_nearest_away>;
        using Z = nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, nstd::rounding_mode::toward_zero>;
        using M =
            nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, nstd::rounding_mode::toward_neg_inf>;
        using P =
            nstd::sfixed_point_t<2, 1, nstd::overflow_policy::wrap, nstd::rounding_mode::toward_pos_inf>;
        const auto ref = (E{a} * E{b}).crudo();
        return (A{a} * A{b}).crudo().limb(0) == ref.limb(0) && (A{a} * A{b}).crudo().limb(1) == ref.limb(1) &&
               (Z{a} * Z{b}).crudo().limb(0) == ref.limb(0) && (M{a} * M{b}).crudo().limb(0) == ref.limb(0) &&
               (P{a} * P{b}).crudo().limb(0) == ref.limb(0) && (P{a} * P{b}).crudo().limb(1) == ref.limb(1);
    }

#ifdef __SIZEOF_INT128__
    /// Cruza un modo concreto en el tipo de UN limbo contra el oraculo de 128 bits.
    template <nstd::rounding_mode M>
    void cruza_un_modo(const std::int64_t *crudos, std::size_t n)
    {
        using T =
            nstd::fixed_point_t<1, 1, nstd::signedness::signed_type,
                                nstd::representation_form::twos_complement, nstd::overflow_policy::wrap, M>;
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = 0; j < n; ++j)
            {
                const std::int64_t a = crudos[i], b = crudos[j];

                typename T::entero ea{}, eb{};
                ea.set_limb(0, static_cast<std::uint64_t>(a));
                eb.set_limb(0, static_cast<std::uint64_t>(b));
                const T x = T::desde_crudo(ea);
                const T y = T::desde_crudo(eb);

                ++casos;
                const auto obtenido = static_cast<std::int64_t>((x * y).crudo().limb(0));
                const std::int64_t esperado = producto_oraculo(a, b, M);
                if (obtenido != esperado)
                {
                    std::printf("  [FALLA] producto modo %d: %lld * %lld  esperado %lld, obtenido %lld\n",
                                static_cast<int>(M), static_cast<long long>(a), static_cast<long long>(b),
                                static_cast<long long>(esperado), static_cast<long long>(obtenido));
                    ++fallos;
                }

                if (b == 0)
                    continue; // dividir por cero lanza; se comprueba aparte

                ++casos;
                const auto obt_div = static_cast<std::int64_t>((x / y).crudo().limb(0));
                const std::int64_t esp_div = division_oraculo(a, b, M);
                if (obt_div != esp_div)
                {
                    std::printf("  [FALLA] division modo %d: %lld / %lld  esperado %lld, obtenido %lld\n",
                                static_cast<int>(M), static_cast<long long>(a), static_cast<long long>(b),
                                static_cast<long long>(esp_div), static_cast<long long>(obt_div));
                    ++fallos;
                }
            }
    }

    void cruza_modos_1limbo(const std::int64_t *crudos, std::size_t n)
    {
        cruza_un_modo<nstd::rounding_mode::to_nearest_even>(crudos, n);
        cruza_un_modo<nstd::rounding_mode::to_nearest_away>(crudos, n);
        cruza_un_modo<nstd::rounding_mode::toward_zero>(crudos, n);
        cruza_un_modo<nstd::rounding_mode::toward_neg_inf>(crudos, n);
        cruza_un_modo<nstd::rounding_mode::toward_pos_inf>(crudos, n);
    }
#endif

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

        // CAMBIO del 22 sep: antes `to_string(3)` de un siete daba "7" porque
        // no habia parte fraccionaria que escribir. Ahora escribe las tres
        // cifras, que es lo que hace `printf("%.3f", 7.0)`: el valor ES siete,
        // y siete con tres decimales es 7,000. `decimales` significa lo que
        // dice.
        compara(E{7}.to_string(3), "7.000", "F == 0 escribe las cifras pedidas, como printf");
        compara((-E{7}).to_string(2), "-7.00", "F == 0 con signo");
        compara(E{7}.to_string(0), "7", "y con cero cifras no hay coma");

        // El minimo: negarlo ENVUELVE, y por ahi se colaba un segundo signo.
        comprueba(-Q::min() == Q::min(), "negar el minimo envuelve y da el minimo");
        comprueba(Q::min().is_negative(), "el minimo es negativo");
        compara(Q::min().to_string(2), "-9223372036854775808.00", "el minimo lleva UN signo, no dos");
        // CAMBIO del 22 sep: `to_string` redondea. El maximo es
        // 9223372036854775807,99999999999999999995, y a dos cifras eso **sube**.
        // La cadena ya no es el valor truncado sino el valor redondeado, que es
        // lo que se pidio, y el acarreo llega hasta la parte entera.
        compara(Q::max().to_string(2), "9223372036854775808.00",
                "el maximo redondea hacia arriba y acarrea a la parte entera");
        compara(Q::max().to_string(0), "9223372036854775808", "y sin cifras igual");
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

    // ------------------------------------ incremento y decremento (ADR-020) ---
    //
    // `++x` es `x += 1`, como en `float` y `double`. NO avanza un epsilon: eso
    // es `std::nextafter`, y darle a `++` otro significado del que tiene en
    // coma flotante seria una sorpresa silenciosa en codigo generico.
    std::printf("-- ++ y -- suman UNO, no un epsilon\n");
    {
        Q x{5};
        comprueba(++x == Q{6}, "++ suma uno");
        comprueba(x == Q{6}, "y deja el valor subido");
        comprueba(--x == Q{5}, "-- resta uno");

        // Lo que distingue las dos lecturas: un epsilon NO es uno.
        Q y{0};
        ++y;
        comprueba(y == Q::one(), "++ desde cero da UNO");
        comprueba(y != Q::epsilon(), "y no un epsilon, que es la otra lectura posible");

        // Post frente a pre.
        Q z{3};
        comprueba(z++ == Q{3}, "post-incremento devuelve el valor de ANTES");
        comprueba(z == Q{4}, "y deja el de despues");
        comprueba(z-- == Q{4}, "post-decremento igual");
        comprueba(z == Q{3}, "y vuelve");

        // Con fraccion: sumar uno no toca la parte fraccionaria.
        Q f = Q{2} + Q::desde_crudo(Q::entero{std::uint64_t{1}} << 63U);
        ++f;
        compara(f.to_string(2), "3.50", "++ sobre 2,5 da 3,5: la fraccion no se toca");
    }

    // ------------------------------------- el resto es EXACTO (ADR-020) -------
    //
    // `%` sigue a `std::fmod`: el resto de truncar hacia cero, con el signo del
    // dividendo. Y NO necesita redondeo, porque el resto de dos multiplos de
    // epsilon es multiplo de epsilon.
    std::printf("-- %% es exacto y no redondea\n");
    {
        // La identidad que SI se cumple: con el cociente TRUNCADO.
        using Qt = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::toward_zero>;
        const long long dividendos[] = {0, 1, -1, 7, -7, 10, -10, 100, -100, 12345, -12345};
        const long long divisores[] = {1, -1, 2, -2, 3, -3, 7, -7, 100, -100};
        for (long long da : dividendos)
            for (long long db : divisores)
            {
                const Qt a{da}, b{db};
                const Qt r = a % b;

                // OJO: la identidad NO es `a == (a/b)*b + a%b`. Esa es la de
                // los ENTEROS, donde `/` ya devuelve el cociente entero. Aqui
                // `a/b` tiene parte fraccionaria --`-12345/7` es `-1763,571...`,
                // no `-1763`-- y escribirla asi fue un error mio que este mismo
                // bucle destapo.
                //
                // La que si vale pasa por el cociente truncado A ENTERO, y ese
                // se recupera quitando el resto: `a - r` es multiplo exacto de
                // `b`, asi que `k = (a - r)/b` es entero.
                const Qt k = (a - r) / b;
                ++casos;
                if (!k.es_entero())
                {
                    std::printf("  [FALLA] (a - a%%b)/b no es entero: %lld, %lld\n", da, db);
                    ++fallos;
                }

                // `k` es entero, asi que `k * b` es exacto y esto no depende
                // del modo de redondeo.
                ++casos;
                if (k * b + r != a)
                {
                    std::printf("  [FALLA] a != k*b + a%%b con k entero: %lld, %lld\n", da, db);
                    ++fallos;
                }

                // Y el resto es menor que el divisor en valor absoluto.
                const Qt ra = r.is_negative() ? -r : r;
                const Qt ba = b.is_negative() ? -b : b;
                ++casos;
                if (!(ra < ba))
                {
                    std::printf("  [FALLA] |resto| no es menor que |divisor|: %lld, %lld\n", da, db);
                    ++fallos;
                }
                // Y el resto lleva el signo del dividendo, como en C++.
                ++casos;
                if (!r.is_zero() && (r.is_negative() != a.is_negative()))
                {
                    std::printf("  [FALLA] el signo del resto no es el del dividendo: %lld, %lld\n", da, db);
                    ++fallos;
                }
            }

        // El resto NO depende del modo de redondeo: es exacto.
        using Qe = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::to_nearest_even>;
        using Qp = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::toward_pos_inf>;
        comprueba((Qe{7} % Qe{2}).crudo() == (Qp{7} % Qp{2}).crudo(),
                  "el resto sale igual con dos modos distintos");
    }

    // ------------------- el redondeo, oraculo 1: __int128 en un limbo ---------
#ifdef __SIZEOF_INT128__
    std::printf("-- redondeo, oraculo de __int128 sobre N = F = 1\n");
    {
        // Los crudos que hacen falta. Las esquinas a mano PRIMERO: los empates
        // exactos son lo unico que separa al-par de alejarse-del-cero, y un
        // sorteo al azar no los produce nunca.
        const std::int64_t crudos[] = {
            0,
            1,
            -1,
            2,
            -2,
            3,
            -3,
            std::int64_t{1} << 62, // un cuarto
            -(std::int64_t{1} << 62),
            std::numeric_limits<std::int64_t>::max(),
            std::numeric_limits<std::int64_t>::min(),
            std::numeric_limits<std::int64_t>::min() + 1,
            0x0123456789ABCDEFLL,
            -0x0123456789ABCDEFLL,
            0x7FFFFFFFFFFFFFFELL,
        };

        cruza_modos_1limbo(crudos, sizeof(crudos) / sizeof(crudos[0]));
    }
#else
    std::printf("-- (sin __int128: el oraculo de un limbo no corre aqui)\n");
#endif

    // ------------------- el redondeo, oraculo 2: la tabla de Q64.64 -----------
    //
    // Generada con aritmetica exacta de Python, porque el producto de dos
    // crudos de 128 bits son 256 y no hay tipo nativo donde calcularlo.
    std::printf("-- redondeo, tabla generada para Q64.64\n");
    {
        for (const CasoBin &c : kProductos)
            cruza_tabla_producto(c);
        for (const CasoBin &c : kDivisiones)
            cruza_tabla_division(c);
        for (const CasoResto &c : kRestos)
            cruza_tabla_resto(c);
    }

    // --------------------------------- lo que el redondeo NO debe tocar -------
    std::printf("-- los cinco modos coinciden cuando el resultado es exacto\n");
    {
        // Si el producto cae justo, los cinco modos tienen que dar lo mismo. Un
        // modo que «redondeara» un resultado exacto estaria mal, y este es el
        // unico sitio del test donde todos deben coincidir.
        comprueba(coinciden_los_cinco(2, 3), "2 * 3 es exacto en los cinco modos");
        comprueba(coinciden_los_cinco(-4, 5), "-4 * 5 tambien");
        comprueba(coinciden_los_cinco(7, 1), "y 7 * 1");
    }

    // ------------------------------------------- compuestos y equivalencia ----
    std::printf("-- *=, /=, %%= hacen lo mismo que *, / y %%\n");
    {
        const Q a{7}, b{3};
        Q x{a};
        x *= b;
        comprueba(x == a * b, "*= es *");
        Q y{a};
        y /= b;
        comprueba(y == a / b, "/= es /");
        Q z{a};
        z %= b;
        comprueba(z == a % b, "%= es %");

        // Y el *= por un entero, que es el exacto.
        Q w{a};
        w *= Q::entero{4};
        comprueba(w == a * Q::entero{4}, "*= por entero es * por entero");
        comprueba(w == Q{28}, "7 * 4 = 28");
    }

    // ------------------------------ to_string redondea, y acarrea ------------
    //
    // Un decimal de longitud fija se rompe por tres sitios, y ninguno sale de
    // numeros bonitos: el EMPATE en la ultima cifra, el ACARREO que se sale de
    // la parte fraccionaria, y los negativos diminutos que redondean a cero.
    std::printf("-- to_string, tabla generada para los cinco modos\n");
    {
        for (const CasoCadena &c : kCadenas)
            cruza_tabla_cadena(c);

        // El acarreo que ALARGA la cadena, que es el caso que obliga a meter la
        // coma al final y contando desde la derecha.
        const Q casi_diez = Q::desde_crudo(Q::entero{std::uint64_t{9}} *
                                               (Q::entero::one() << 64U) +
                                           (Q::entero::one() << 64U) - Q::entero::one());
        compara(casi_diez.to_string(2), "10.00", "9,9999... sube a 10,00 y la cadena crece");
        compara(casi_diez.to_string(0), "10", "y sin cifras tambien");

        // Muchos decimales: la cuenta no se desborda porque el resto se reduce
        // modulo la escala en cada vuelta.
        compara(Q::epsilon().to_string(0), "0", "un epsilon con cero cifras es cero");
        comprueba(Q::epsilon().to_string(30).size() == 32U,
                  "treinta cifras caben y salen todas");
    }

    // ------------------------------------ << y >> escalan el VALOR -----------
    //
    // El cruce fuerte: `x >> n` y `x / 2^n` tienen que dar lo mismo, y son
    // **dos implementaciones distintas** --una enmascara los n bits bajos, la
    // otra pasa por `divmod` en el tipo ancho--. Que coincidan en los cinco
    // modos no lo garantiza ninguna de las dos por separado.
    std::printf("-- << y >> escalan por potencias de dos\n");
    {
        cruza_desplazamientos<rounding_mode::to_nearest_even>();
        cruza_desplazamientos<rounding_mode::to_nearest_away>();
        cruza_desplazamientos<rounding_mode::toward_zero>();
        cruza_desplazamientos<rounding_mode::toward_neg_inf>();
        cruza_desplazamientos<rounding_mode::toward_pos_inf>();

        // `<<` es exacto: es multiplicar por una potencia de dos.
        comprueba((Q{3} << 2U) == Q{12}, "3 << 2 = 12");
        comprueba((Q{-3} << 2U) == Q{-12}, "-3 << 2 = -12");
        comprueba((Q{5} << 0U) == Q{5}, "desplazar cero no hace nada");
        comprueba((Q{5} >> 0U) == Q{5}, "ni a la derecha");

        // Un medio: `>> 1` sobre uno.
        compara((Q::one() >> 1U).to_string(1), "0.5", "1 >> 1 es un medio");

        // En sitio.
        Q x{3};
        x <<= 3U;
        comprueba(x == Q{24}, "<<= es <<");
        Q y{24};
        y >>= 3U;
        comprueba(y == Q{3}, ">>= es >>");

        // Desplazar MAS que el ancho del tipo no es indefinido: se cae todo, y
        // lo que quede decide el redondeo como en cualquier otro sitio.
        using Msuelo = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::toward_neg_inf>;
        comprueba((Msuelo{7} >> 200U).is_zero(), "un positivo se cae entero hacia el suelo");
        comprueba((Msuelo{-7} >> 200U) == Msuelo{0} - Msuelo::epsilon(),
                  "un negativo cae al escalon de abajo, no a cero");
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
