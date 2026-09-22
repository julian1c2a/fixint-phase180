// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julián Calderón Almendros
// Email: julian.calderon.almendros@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
// @file       fixed_point_t.hpp
// @brief      Punto fijo de N limbos, F de ellos fraccionarios
// @author     Julián Calderón Almendros
// @date       2026-09-22
// =============================================================================
//
// UN PUNTO FIJO ES UN ENTERO CON UNA ESCALA  (ADR-019)
// -----------------------------------------------------
// Este tipo **guarda un `fixed_int_t<N, Sign, Form, Policy>`** y sabe que ese
// entero vale `crudo / 2^(64*F)`. De ahi sale todo lo demas:
//
//     +  -  comparacion  orden    las del entero, SIN TOCAR NADA: misma escala
//     *                           la del entero mas descartar F limbos
//     /                           preescalar por 2^(64*F) y dividir
//
// **No hay ni un algoritmo aritmetico aqui.** Knuth D, Karatsuba,
// Moller-Granlund, Toom-3, las cuatro representaciones y la politica de
// desbordamiento vienen ya escritos y probados desde `fixed_int_t`.
//
// Es el tercer caso del mismo patron en este proyecto, y el mas barato de los
// tres: una **capa** se paga en cada operacion, una **codificacion** son dos
// funciones (ADR-017), y una **escala** es un desplazamiento.
//
// POR QUE `N` Y `F` EN LIMBOS, Y NO `E` Y `F` EN BITS
// ---------------------------------------------------
// Con bits, si `E+F` no es multiplo de 64 sobran bits en el limbo alto, y
// entonces **cada operacion** tiene que enmascararlos y leer el signo del bit
// `E+F-1` en vez del 63. Es coste por operacion y superficie de fallo nueva.
//
// Con limbos, el almacenamiento **es exactamente** un `fixed_int_t<N>`: sin
// relleno, sin nada que enmascarar nunca. La parte entera es `E = N - F` y se
// deduce.
//
// ESTADO: EL TIPO Y LAS CONVERSIONES
// ----------------------------------
// Esta primera entrega trae el tipo, las conversiones y **las operaciones que
// son exactas** --suma, resta, negacion, comparacion y orden--, que por la
// decision 1 de ADR-019 son las del entero sin tocar nada.
//
// **El producto y la division no estan todavia**: necesitan el redondeo, que
// ADR-019 deja como perilla y que se escribe despues para poder comparar los
// modos midiendo en vez de elegirlos por analogia. Mientras tanto no se declaran
// siquiera, para que llamarlos sea un error de compilacion y no una sorpresa.
// =============================================================================

#ifndef NSTD_FIXED_POINT_T_HPP
#define NSTD_FIXED_POINT_T_HPP

#include "fixed_width_int_t.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace nstd
{
    /// @brief Punto fijo binario de `N` limbos, de los que `F` son fraccionarios.
    ///
    /// El valor representado es `crudo() / 2^(64*F)`, donde `crudo()` es un
    /// `fixed_int_t<N, Sign, Form, Policy>`.
    ///
    /// @tparam N Limbos **totales**. La parte entera es `E = N - F`.
    /// @tparam F Limbos **fraccionarios**. `F == 0` da un entero con otro nombre;
    ///         `F == N` da un tipo puramente fraccionario, en `[0, 1)` sin signo.
    /// @tparam Sign Con o sin signo, igual que en el entero.
    /// @tparam Form Representacion. Las cuatro del entero valen: `binnat` sin
    ///         signo, y complemento a dos, Magnitud-Signo o Exceso-K con signo.
    ///         El bit de signo es el mas significativo de la parte entera, **no
    ///         un bit anadido**, asi que todas ocupan lo mismo (ADR-019).
    /// @tparam Policy Politica de desbordamiento, igual que en el entero.
    template <std::size_t N, std::size_t F, signedness Sign = signedness::unsigned_type,
              representation_form Form = representation_form::binnat,
              overflow_policy Policy = overflow_policy::wrap>
    class fixed_point_t
    {
        static_assert(F <= N, "fixed_point_t: la parte fraccionaria no puede pasar del total");

    public:
        /// @brief El entero que hay debajo. **Es el almacenamiento entero**, no
        ///        la parte entera del valor.
        using entero = fixed_int_t<N, Sign, Form, Policy>;

        /// @brief Limbos **totales** del almacenamiento, enteros y fraccionarios.
        static constexpr std::size_t num_limbs{N};
        /// @brief Limbos **fraccionarios**: los que quedan por debajo de la coma.
        static constexpr std::size_t limbos_fraccionarios{F};
        /// @brief Limbos de la parte entera, `N - F`.
        static constexpr std::size_t limbos_enteros{N - F};

        /// @brief Bits fraccionarios: la escala es `2^escala_bits`.
        static constexpr std::size_t escala_bits{64U * F};

        /// @brief Con o sin signo, tal como se instancio.
        static constexpr signedness sign{Sign};
        /// @brief La representacion: `binnat`, complemento a dos, Magnitud-Signo
        ///        o Exceso-K. Por ADR-018 **no se observa desde el
        ///        comportamiento**: solo cambia lo que devuelve `crudo().limb()`.
        static constexpr representation_form form{Form};
        /// @brief La politica de desbordamiento, que se hereda del entero: aqui
        ///        no hay nada que anadirle, porque la suma y la resta son las
        ///        suyas sin tocar nada.
        static constexpr overflow_policy policy{Policy};

        // =====================================================================
        // Construccion
        // =====================================================================

        /// @brief El cero.
        constexpr fixed_point_t() noexcept = default;

        /// @brief Construye desde un entero del lenguaje, **como parte entera**.
        ///
        /// `fixed_point_t<2,1>{3}` vale tres, no `3 / 2^64`. Para lo segundo esta
        /// `desde_crudo`.
        ///
        /// @note Es `explicit` como todo en esta biblioteca (ADR-001), y ademas
        ///       aqui hace mas falta: sin el, `x + 1` compilaria con dos
        ///       significados posibles.
        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        explicit constexpr fixed_point_t(T v) noexcept : bruto_{entero{v}}
        {
            if constexpr (F > 0)
                bruto_ = desplaza_a_escala(entero{v});
        }

        /// @brief Construye desde el entero de abajo **ya escalado**.
        ///
        /// Es lo contrario del constructor de arriba: aqui `x` se toma tal cual,
        /// de modo que el valor es `x / 2^(64*F)`. Se llama asi y no es un
        /// constructor a secas porque los dos tienen la misma firma y confundirlos
        /// es un error silencioso de un factor `2^(64*F)`.
        [[nodiscard]] static constexpr fixed_point_t desde_crudo(const entero &x) noexcept
        {
            fixed_point_t r{};
            r.bruto_ = x;
            return r;
        }

        /// @brief El entero de abajo, tal cual se guarda.
        [[nodiscard]] constexpr const entero &crudo() const noexcept { return bruto_; }

        // =====================================================================
        // Constantes
        // =====================================================================

        /// @brief El cero, que es el mismo en las cuatro representaciones
        ///        aunque en Exceso-K sus limbos **no** sean todos ceros.
        [[nodiscard]] static constexpr fixed_point_t zero() noexcept { return fixed_point_t{}; }

        /// @brief El uno. Con `F == N` **no existe**: ese tipo solo representa
        ///        `[0, 1)`, y el `static_assert` lo dice en vez de devolver cero.
        [[nodiscard]] static constexpr fixed_point_t one() noexcept
        {
            static_assert(F < N, "fixed_point_t: con F == N el uno no es representable");
            return fixed_point_t{1};
        }

        /// @brief El valor positivo mas pequeno que no es cero: `2^-(64*F)`.
        ///
        /// Es el «ulp» del tipo, y el paso entre dos valores consecutivos.
        [[nodiscard]] static constexpr fixed_point_t epsilon() noexcept { return desde_crudo(entero::one()); }

        /// @brief El mayor valor representable, que es `entero::max() / 2^(64*F)`.
        [[nodiscard]] static constexpr fixed_point_t max() noexcept { return desde_crudo(entero::max()); }

        /// @brief El menor valor representable. Sin signo es el cero.
        [[nodiscard]] static constexpr fixed_point_t min() noexcept { return desde_crudo(entero::min()); }

        // =====================================================================
        // Lo exacto: no hay redondeo que hacer  (ADR-019, decision 1)
        // =====================================================================
        //
        // Dos valores de este tipo comparten escala, asi que sumarlos es sumar
        // los enteros de abajo. No hay nada que ajustar ni que redondear, y la
        // politica de desbordamiento funciona sola porque la hereda el entero.

        /// @brief Suma **exacta**: misma escala, asi que es la del entero.
        constexpr fixed_point_t operator+(const fixed_point_t &o) const noexcept
        {
            return desde_crudo(bruto_ + o.bruto_);
        }

        /// @brief Resta **exacta**, por el mismo motivo que la suma.
        constexpr fixed_point_t operator-(const fixed_point_t &o) const noexcept
        {
            return desde_crudo(bruto_ - o.bruto_);
        }

        /// @brief Negacion. Sin signo desborda segun la politica, igual que en el entero.
        constexpr fixed_point_t operator-() const noexcept { return desde_crudo(-bruto_); }
        /// @brief Mas unario, que no hace nada. Esta por simetria con el menos.
        constexpr fixed_point_t operator+() const noexcept { return *this; }

        /// @brief Suma en sitio.
        constexpr fixed_point_t &operator+=(const fixed_point_t &o) noexcept
        {
            bruto_ += o.bruto_;
            return *this;
        }

        /// @brief Resta en sitio.
        constexpr fixed_point_t &operator-=(const fixed_point_t &o) noexcept
        {
            bruto_ -= o.bruto_;
            return *this;
        }

        /// @brief Multiplicar por un entero **no** cambia la escala, asi que es
        ///        exacto y no necesita redondeo.
        ///
        /// Es `a * k`, no `a * b` entre dos puntos fijos: eso ultimo duplica los
        /// limbos fraccionarios y hay que redondear.
        constexpr fixed_point_t operator*(const entero &k) const noexcept { return desde_crudo(bruto_ * k); }

        // --- comparacion: la escala es comun, asi que basta el entero ---------

        /// @brief Igualdad. Dos valores del mismo tipo son iguales si lo son
        ///        sus crudos: la escala es comun y no hay dos crudos distintos
        ///        que den el mismo valor.
        constexpr bool operator==(const fixed_point_t &o) const noexcept { return bruto_ == o.bruto_; }
        /// @brief Desigualdad.
        constexpr bool operator!=(const fixed_point_t &o) const noexcept { return !(*this == o); }
        /// @brief Menor que. Ordenar por el crudo ordena por el valor porque la
        ///        escala es un factor positivo y comun.
        constexpr bool operator<(const fixed_point_t &o) const noexcept { return bruto_ < o.bruto_; }
        /// @brief Mayor que.
        constexpr bool operator>(const fixed_point_t &o) const noexcept { return o < *this; }
        /// @brief Menor o igual.
        constexpr bool operator<=(const fixed_point_t &o) const noexcept { return !(o < *this); }
        /// @brief Mayor o igual.
        constexpr bool operator>=(const fixed_point_t &o) const noexcept { return !(*this < o); }

        /// @brief Comparacion de tres vias. Es **total** --`strong_ordering`--
        ///        porque aqui no hay NaN ni dos ceros: eso es de la coma
        ///        flotante, no del punto fijo.
        [[nodiscard]] constexpr std::strong_ordering operator<=>(const fixed_point_t &o) const noexcept
        {
            return bruto_ <=> o.bruto_;
        }

        // =====================================================================
        // Preguntas
        // =====================================================================

        /// @brief Si el valor **no arrastra una marca de desbordamiento**.
        ///
        /// Con `overflow_policy::checked` la marca la pone y la propaga el
        /// entero de abajo; aqui solo se reexpone, porque si no habria que
        /// preguntarsela a `crudo()` y la politica seria un parametro que el
        /// tipo declara y no deja consultar.
        ///
        /// Con las demas politicas devuelve siempre `true`, igual que en el
        /// entero.
        ///
        /// @note La marca es **pegajosa**: saturar o volver al rango no la
        ///       limpia, porque un valor marcado guarda dentro el resultado ya
        ///       envuelto (P1.5 tramo 2f).
        [[nodiscard]] constexpr bool valid() const noexcept { return bruto_.valid(); }

        /// @brief Si el valor es cero.
        [[nodiscard]] constexpr bool is_zero() const noexcept { return bruto_.is_zero(); }
        /// @brief Si el valor es negativo. Sin signo siempre es `false`.
        [[nodiscard]] constexpr bool is_negative() const noexcept { return bruto_.is_negative(); }

        /// @brief Si el valor no tiene parte fraccionaria.
        [[nodiscard]] constexpr bool es_entero() const noexcept
        {
            if constexpr (F == 0)
                return true;
            else
                return parte_fraccionaria().is_zero();
        }

        // =====================================================================
        // Las dos mitades
        // =====================================================================

        /// @brief La parte entera, **truncada hacia -infinito**.
        ///
        /// @warning Es el suelo, no el truncamiento hacia cero: `-1.5` da `-2`.
        ///          Sale de que la parte entera son los limbos altos del valor
        ///          con signo, que es un desplazamiento aritmetico. Es la misma
        ///          asimetria que tiene `operator>>` frente a `operator/` en el
        ///          entero, y no se disimula aqui.
        [[nodiscard]] constexpr entero suelo() const noexcept
        {
            if constexpr (F == 0)
                return bruto_;
            else
                return bruto_ >> static_cast<unsigned>(escala_bits);
        }

        /// @brief La parte fraccionaria, como el entero de los `F` limbos bajos.
        ///
        /// Siempre es **no negativa**: es el `r` de `valor = suelo() + r/2^(64F)`
        /// con `0 <= r < 2^(64F)`, que es lo que hace que la formula del redondeo
        /// valga igual para negativos (ADR-019).
        [[nodiscard]] constexpr fixed_int_t<N, signedness::unsigned_type, representation_form::binnat, Policy>
        parte_fraccionaria() const noexcept
        {
            using U = fixed_int_t<N, signedness::unsigned_type, representation_form::binnat, Policy>;
            if constexpr (F == 0)
                return U{};
            else
            {
                // En complemento a dos los F limbos bajos ya son ese `r`. Para
                // las otras representaciones hay que pasar por ahi, que es lo que
                // hace la conversion (ADR-018).
                U u{bruto_};
                U mascara = U::max();
                if constexpr (F < N)
                    mascara = (U::one() << static_cast<unsigned>(escala_bits)) - U::one();
                return u & mascara;
            }
        }

        // =====================================================================
        // Cadena
        // =====================================================================

        /// @brief Decimal con `decimales` cifras tras la coma, **truncando**.
        ///
        /// @warning Trunca, no redondea, y es a proposito mientras el redondeo no
        ///          este escrito: mas vale que corte de forma evidente a que
        ///          redondee de una manera que luego haya que cambiar. Cuando
        ///          entre la perilla, esta funcion la usara.
        ///
        /// @param decimales Cuantas cifras tras la coma. Con `0` no se escribe ni
        ///        la coma.
        [[nodiscard]] std::string to_string(unsigned decimales = 6) const
        {
            const bool negativo = is_negative();
            // Se trabaja con la magnitud para que la coma no tenga que saber de
            // signos: el `-` se pega al final.
            const fixed_point_t mag = negativo ? -(*this) : *this;

            // ...salvo en el minimo, donde NO hay magnitud a la que pasarse:
            // `-min()` envuelve y vuelve a dar `min()`. Imprimirlo por el camino
            // de arriba pegaba un segundo signo --«--9223372036854775808.00»--.
            //
            // En ese unico caso se imprime el valor tal cual, y sale bien porque
            // el minimo **no tiene parte fraccionaria**: sus limbos bajos son
            // todos cero, asi que el suelo ES el valor y ya trae su signo. Para
            // cualquier otro negativo esto no valdria --el suelo va hacia -inf y
            // daria «-3.5» para -2,5-- y por eso el rodeo por la magnitud.
            const bool envolvio = mag.is_negative();

            std::string s = mag.suelo().to_string();
            if (decimales > 0 && F > 0)
            {
                s += '.';
                // El producto por diez se hace ANCHO a proposito. Con `F == N`
                // --el tipo puramente fraccionario, que solo representa [0,1)--
                // no queda ni un bit de parte entera donde recoger la cifra:
                // `resto * 10` desborda y `resto >> 64*N` da cero, con lo que
                // `0,5` se imprimia como «0.0». Con el doble de ancho la cifra
                // tiene sitio, y el caso general no paga nada por ello porque el
                // producto se descarta enseguida.
                using U2 = fixed_int_t<2 * N, signedness::unsigned_type, representation_form::binnat, Policy>;
                U2 resto{mag.parte_fraccionaria()};
                const U2 diez{std::uint64_t{10}};
                const U2 escala = U2::one() << static_cast<unsigned>(escala_bits);
                for (unsigned i = 0; i < decimales; ++i)
                {
                    resto = resto * diez;
                    const U2 cifra = resto / escala;
                    s += static_cast<char>('0' + static_cast<char>(cifra.limb(0) % 10U));
                    resto = resto - (cifra * escala);
                }
            }
            return (negativo && !envolvio) ? ("-" + s) : s;
        }

    private:
        /// El entero, ya escalado: el valor es `bruto_ / 2^(64*F)`.
        entero bruto_{};

        /// Sube un entero a la escala del tipo.
        [[nodiscard]] static constexpr entero desplaza_a_escala(const entero &v) noexcept
        {
            if constexpr (F == 0)
                return v;
            else
                return v << static_cast<unsigned>(escala_bits);
        }
    };

    // =========================================================================
    // Alias
    // =========================================================================

    /// @brief Punto fijo sin signo: `N` limbos, `F` fraccionarios.
    template <std::size_t N, std::size_t F, overflow_policy Policy = overflow_policy::wrap>
    using ufixed_point_t =
        fixed_point_t<N, F, signedness::unsigned_type, representation_form::binnat, Policy>;

    /// @brief Punto fijo con signo, en complemento a dos.
    template <std::size_t N, std::size_t F, overflow_policy Policy = overflow_policy::wrap>
    using sfixed_point_t =
        fixed_point_t<N, F, signedness::signed_type, representation_form::twos_complement, Policy>;

    /// @brief 64 enteros y 64 fraccionarios, sin signo. El «Q64.64» de toda la vida.
    using ufixed_64_64_t = ufixed_point_t<2, 1>;
    /// @brief 64 enteros y 64 fraccionarios, con signo.
    using sfixed_64_64_t = sfixed_point_t<2, 1>;

} // namespace nstd

#endif // NSTD_FIXED_POINT_T_HPP
