// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: std::numeric_limits de fixed_point_t  -- P4, E4 (ADR-022)
// Part of int128 Library
// =============================================================================
//
// SE CRUZA CONTRA LA ARITMETICA, NO CONTRA UNA TABLA
// ---------------------------------------------------
// Comprobar que `epsilon()` vale `2^-(64F)` comparandolo con `2^-(64F)` escrito
// a mano no puede fallar: es la misma constante dos veces. Aqui se comprueba lo
// que **significa**: que `epsilon()` es de verdad el paso entre dos valores
// consecutivos, que `max()` es de verdad el mayor, que `min()` es de verdad el
// menor.
//
// Y los CINCO modos de redondeo se instancian, porque cuatro podrian estar mal
// y el test del modo por omision pasaria igual. Es lo que ADR-022 deja escrito
// como consecuencia.
// =============================================================================

#include "fixed_int_limits.hpp"
#include "fixed_point_limits.hpp"

#include <cstdio>
#include <limits>
#include <string>

namespace
{
    int fallos = 0;
    long long casos = 0;

    using namespace nstd;

    /// @param donde El tipo del que se habla.
    ///
    /// No es adorno: sin el, los ayudantes plantilla de abajo se instancian
    /// para seis tipos y un fallo no dice **cual**. La primera version de este
    /// test no lo tenia y hubo que adivinar.
    void comprueba(bool ok, const char *que, const char *donde = "")
    {
        ++casos;
        if (!ok)
        {
            std::printf("  [FALLA] %-52s %s\n", que, donde);
            ++fallos;
        }
    }

    /// Lo que `epsilon()` TIENE que significar: el paso entre consecutivos.
    ///
    /// Y ademas que ese paso es el mismo en todo el rango, que es lo que
    /// distingue un epsilon absoluto de uno relativo.
    template <typename T>
    void epsilon_es_el_paso(const char *como)
    {
        using L = std::numeric_limits<T>;
        const T eps = L::epsilon();

        comprueba(!eps.is_zero(), "epsilon no es cero", como);

        // El paso desde cero.
        comprueba(T::zero() + eps != T::zero(), "sumar epsilon cambia el valor", como);
        comprueba((T::zero() + eps).crudo() == T::entero::one(), "epsilon es exactamente un crudo", como);

        // El MISMO paso lejos de cero: eso es ser absoluto. En coma flotante
        // esto seria falso, porque alli el ulp crece con la magnitud.
        const T lejos{1000000};
        comprueba(lejos + eps != lejos, "y tambien cambia lejos del cero", como);
        comprueba((lejos + eps).crudo() - lejos.crudo() == (T::zero() + eps).crudo(),
                  "el paso es EL MISMO cerca y lejos del cero (epsilon absoluto)", como);

        // Las tres igualdades que ADR-022 llama caracteristicas del punto fijo.
        comprueba(L::denorm_min() == eps, "denorm_min es epsilon", como);
        comprueba(eps == T::epsilon(), "numeric_limits y el tipo dicen lo mismo", como);
    }

    /// `max()` y `min()` son de verdad los extremos.
    template <typename T>
    void extremos_de_verdad(const char *como)
    {
        using L = std::numeric_limits<T>;

        // La regla de ADR-022: dos formas de preguntar dan lo mismo.
        comprueba(L::max() == T::max(), "numeric_limits::max coincide con T::max", como);
        comprueba(L::min() == T::min(), "numeric_limits::min coincide con T::min", como);
        comprueba(L::lowest() == L::min(), "lowest es min", como);

        // Y que son de verdad extremos: pasarse envuelve, o sea deja de ser
        // mayor. Con `wrap` eso es lo que significa ser el maximo.
        comprueba(!(L::max() + L::epsilon() > L::max()), "no hay nada por encima del maximo", como);
        comprueba(!(L::min() - L::epsilon() < L::min()), "ni por debajo del minimo", como);

        // El minimo es el MAS NEGATIVO, no el positivo mas pequeno. Es la
        // trampa que ADR-022 decide, y compila igual con las dos respuestas.
        if constexpr (T::sign == signedness::signed_type)
        {
            comprueba(L::min().is_negative(), "con signo, min() es NEGATIVO", como);
            comprueba(L::min() < T::zero(), "y menor que cero", como);
            comprueba(L::epsilon() > T::zero(), "mientras que epsilon es positivo", como);
        }
        else
        {
            comprueba(L::min().is_zero(), "sin signo, min() es cero", como);
        }
    }

    /// Los cinco modos: `round_style` y `round_error()`.
    template <rounding_mode M>
    void mira_modo(std::float_round_style esperado, bool al_mas_cercano, const char *como)
    {
        using T = sfixed_point_t<2, 1, overflow_policy::wrap, M>;
        using L = std::numeric_limits<T>;

        ++casos;
        if (L::round_style != esperado)
        {
            std::printf("  [FALLA] round_style de %s no es el esperado\n", como);
            ++fallos;
        }

        // `round_error()` se mide **en ULPs**: 0,5 al mas cercano y 1 con los
        // dirigidos, igual que `numeric_limits<float>::round_error()` vale 0,5.
        //
        // La primera version de este test esperaba `epsilon()/2` y fallo, que es
        // como se descubrio que medio ulp **no es representable**: el ulp es el
        // valor positivo mas pequeno. El ADR era el equivocado, no el codigo.
        const T err = L::round_error();
        ++casos;
        if (al_mas_cercano)
        {
            // Medio: sumado consigo mismo da uno.
            if (!(err + err == T{1}))
            {
                std::printf("  [FALLA] round_error de %s no es 0,5 ULP\n", como);
                ++fallos;
            }
        }
        else if (!(err == T{1}))
        {
            std::printf("  [FALLA] round_error de %s no es 1 ULP\n", como);
            ++fallos;
        }
    }
} // namespace

int main()
{
    std::printf("=== numeric_limits de fixed_point_t (ADR-022) ===\n\n");

    using QU = ufixed_point_t<2, 1>;
    using QS = sfixed_point_t<2, 1>;

    // ------------------------------------------ lo que lo separa del entero ---
    std::printf("-- is_integer es falso, is_exact es cierto\n");
    {
        static_assert(std::numeric_limits<QS>::is_specialized);
        static_assert(!std::numeric_limits<QS>::is_integer,
                      "un punto fijo NO es un entero: hay valores entre dos consecutivos");
        static_assert(std::numeric_limits<QS>::is_exact,
                      "pero SI es exacto: cada valor es k/2^(64F) sin aproximacion");
        static_assert(std::numeric_limits<QS>::is_bounded);
        static_assert(std::numeric_limits<QS>::radix == 2);
        static_assert(!std::numeric_limits<QS>::has_infinity);
        static_assert(!std::numeric_limits<QS>::has_quiet_NaN);
        static_assert(std::numeric_limits<QS>::is_signed);
        static_assert(!std::numeric_limits<QU>::is_signed);

        // El entero de al lado dice lo contrario en `is_integer`, que es
        // exactamente lo que tiene que pasar.
        static_assert(std::numeric_limits<int_fixed_t<2>>::is_integer);
        comprueba(true, "los static_assert de arriba compilan");
    }

    std::printf("-- digits cuenta TODOS los bits de valor\n");
    {
        // 2 limbos = 128 bits; con signo, uno se va en el signo.
        static_assert(std::numeric_limits<QU>::digits == 128);
        static_assert(std::numeric_limits<QS>::digits == 127);
        // No los fraccionarios: esos son 64 y estan en el tipo.
        static_assert(QS::escala_bits == 64U);
        static_assert(std::numeric_limits<QS>::digits != static_cast<int>(QS::escala_bits),
                      "digits NO son los bits fraccionarios (ADR-022, decision 4)");
        static_assert(std::numeric_limits<QS>::digits10 == (127 * 30103) / 100000);
        static_assert(std::numeric_limits<QS>::max_digits10 == std::numeric_limits<QS>::digits10 + 2);
        // Sin exponente: ese es el punto del punto fijo.
        static_assert(std::numeric_limits<QS>::min_exponent == 0);
        static_assert(std::numeric_limits<QS>::max_exponent == 0);
        comprueba(true, "los static_assert de digits compilan");
    }

    // --------------------------------------------- epsilon es EL PASO --------
    std::printf("-- epsilon es el paso, y es el mismo en todo el rango\n");
    {
        epsilon_es_el_paso<QS>("con signo");
        epsilon_es_el_paso<QU>("sin signo");
        epsilon_es_el_paso<fixed_point_t<3, 1, signedness::signed_type, representation_form::magnitude_sign>>(
            "MS");
        epsilon_es_el_paso<fixed_point_t<3, 1, signedness::signed_type, representation_form::excess_k>>("EK");
    }

    std::printf("-- max y min son de verdad los extremos\n");
    {
        extremos_de_verdad<QS>("con signo");
        extremos_de_verdad<QU>("sin signo");
        extremos_de_verdad<fixed_point_t<3, 1, signedness::signed_type, representation_form::excess_k>>("EK");
    }

    // ------------------------------------------- round_style lee la perilla ---
    std::printf("-- round_style sale de la perilla, los cinco modos\n");
    {
        mira_modo<rounding_mode::to_nearest_even>(std::round_to_nearest, true, "al par");
        mira_modo<rounding_mode::to_nearest_away>(std::round_to_nearest, true, "alejarse");
        mira_modo<rounding_mode::toward_zero>(std::round_toward_zero, false, "hacia cero");
        mira_modo<rounding_mode::toward_neg_inf>(std::round_toward_neg_infinity, false, "suelo");
        mira_modo<rounding_mode::toward_pos_inf>(std::round_toward_infinity, false, "techo");

        // Y lo que el estandar NO puede decir: su enum no distingue los dos
        // modos «al mas cercano». Esta escrito aqui para que nadie deduzca el
        // desempate de `round_style` (ADR-022, decision 5).
        using Par = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::to_nearest_even>;
        using Lej = sfixed_point_t<2, 1, overflow_policy::wrap, rounding_mode::to_nearest_away>;
        static_assert(std::numeric_limits<Par>::round_style == std::numeric_limits<Lej>::round_style,
                      "el enum del estandar no los distingue, y eso es del estandar");
        // Pero los tipos SI se comportan distinto, que es lo que importa.
        const Par a{2};
        const Lej b{2};
        const Par medio_par = Par::desde_crudo(Par::entero::one() << 63U);
        const Lej medio_lej = Lej::desde_crudo(Lej::entero::one() << 63U);
        comprueba((a + medio_par).to_string(0) != (b + medio_lej).to_string(0),
                  "2,5 se redondea distinto en los dos, aunque round_style coincida");
    }

    // ----------------------------------------- is_modulo sale de la politica --
    std::printf("-- is_modulo sale de la politica, no del signo\n");
    {
        using W = sfixed_point_t<2, 1, overflow_policy::wrap>;
        using C = sfixed_point_t<2, 1, overflow_policy::checked>;
        static_assert(std::numeric_limits<W>::is_modulo, "wrap envuelve, con signo tambien");
        static_assert(!std::numeric_limits<C>::is_modulo, "checked no envuelve: marca");
        static_assert(std::numeric_limits<ufixed_point_t<2, 1, overflow_policy::checked>>::is_modulo == false,
                      "y sin signo con checked tampoco");

        // Y que `is_modulo` dice la verdad: con wrap, pasarse del maximo vuelve
        // por abajo; con checked, marca.
        comprueba((W::max() + W::epsilon()) == W::min(), "con wrap, el maximo envuelve al minimo");
        comprueba(!(C::max() + C::epsilon()).valid(), "con checked, marca en vez de envolver");
    }

    std::printf("\n%lld comprobaciones, %d fallos\n", casos, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
