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
    /// @brief Los modos de redondeo del punto fijo (ADR-019, ADR-020).
    ///
    /// Es un parametro de plantilla propio y **no** parte de `overflow_policy`:
    /// son dos politicas distintas --el desbordamiento es *no cabe por arriba*,
    /// el redondeo es *no cabe por abajo*-- y juntarlas multiplicaria las
    /// combinaciones que habria que verificar.
    enum class rounding_mode : std::uint8_t
    {
        /// Al mas cercano, y en el empate al **par**. Es el de IEEE-754 y el de
        /// esta biblioteca por omision. Se elige por el **sesgo**, no por el
        /// coste: lo que se hace con punto fijo es acumular, y un redondeo
        /// sesgado deriva tanto mas cuanto mas larga es la cadena.
        to_nearest_even,

        /// Al mas cercano, y en el empate **alejandose del cero**. Es el
        /// «redondeo de toda la vida» que se ensena en la escuela.
        to_nearest_away,

        /// Truncar hacia cero. El mas barato, y el que ya tiene `operator/` del
        /// entero. **Sesga** siempre hacia cero.
        toward_zero,

        /// Hacia `-infinito`: el suelo. Es **gratis**, porque es el
        /// desplazamiento aritmetico tal cual.
        toward_neg_inf,

        /// Hacia `+infinito`: el techo.
        toward_pos_inf,
    };

    namespace detalle_redondeo
    {
        /// @brief ¿Hay que sumar uno al suelo?
        ///
        /// El valor exacto es `q + r/d`, con `q` entero, `0 <= r < d` y `d > 0`.
        /// `q` es el **suelo** y `r` **nunca es negativo**, que es lo que hace
        /// que la misma formula valga para negativos sin un caso aparte.
        ///
        /// Toda operacion con redondeo acaba aqui, con tres datos y nada mas:
        /// como queda `2r` contra `d`, y la paridad y el signo de `q`.
        ///
        /// @param modo        El modo pedido.
        /// @param resto_cero  `r == 0`: el resultado ya era exacto.
        /// @param pasa_mitad  `2r > d`.
        /// @param empate      `2r == d`.
        /// @param q_impar     La paridad del suelo, para el desempate al par.
        /// @param q_negativo  `q < 0`, que con `r != 0` equivale a valor < 0.
        [[nodiscard]] constexpr bool sube(rounding_mode modo, bool resto_cero, bool pasa_mitad, bool empate,
                                          bool q_impar, bool q_negativo) noexcept
        {
            if (resto_cero)
                return false; // exacto: no hay nada que decidir

            switch (modo)
            {
                case rounding_mode::to_nearest_even:
                    return pasa_mitad || (empate && q_impar);
                case rounding_mode::to_nearest_away:
                    // El empate se aleja del cero. El valor es `q + 1/2`, asi que es
                    // positivo exactamente cuando `q >= 0`.
                    return pasa_mitad || (empate && !q_negativo);
                case rounding_mode::toward_zero:
                    // Truncar hacia cero desde el SUELO: para un valor negativo hay
                    // que subir, porque el suelo se paso de largo.
                    return q_negativo;
                case rounding_mode::toward_neg_inf:
                    return false;
                case rounding_mode::toward_pos_inf:
                    return true;
            }
            return false;
        }
    } // namespace detalle_redondeo

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
    /// @tparam Redondeo Que hacer con lo que no cabe por abajo. Va **el ultimo**
    ///         porque es el que menos gente toca, de modo que anadirlo no cambio
    ///         ni un uso existente (ADR-020).
    template <std::size_t N, std::size_t F, signedness Sign = signedness::unsigned_type,
              representation_form Form = representation_form::binnat,
              overflow_policy Policy = overflow_policy::wrap,
              rounding_mode Redondeo = rounding_mode::to_nearest_even>
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
        /// @brief El modo de redondeo, publicado para poder reconstruir el tipo
        ///        desde codigo generico.
        static constexpr rounding_mode redondeo{Redondeo};

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

        // --- incremento y decremento: suman UNO, no un epsilon ---------------
        //
        // `++x` es `x += 1`, que es lo que hacen `float` y `double` en C++.
        //
        // Avanzar al siguiente valor representable --un `epsilon`-- es tentador
        // porque aqui ese valor EXISTE, cosa que en un entero no pasa. Pero esa
        // operacion ya tiene nombre en el estandar y no es `++`: es
        // `std::nextafter`. Darle a `++` otro significado del que tiene en
        // `float` seria una sorpresa silenciosa en codigo generico (ADR-020).
        //
        // Son exactos: sumar uno no cambia la escala.

        /// @brief Pre-incremento: suma **uno**, no un `epsilon`.
        constexpr fixed_point_t &operator++() noexcept
        {
            static_assert(F < N, "fixed_point_t: con F == N el uno no es representable, "
                                 "asi que ++ no tiene a que sumar");
            bruto_ += one().bruto_;
            return *this;
        }

        /// @brief Pre-decremento: resta **uno**.
        constexpr fixed_point_t &operator--() noexcept
        {
            static_assert(F < N, "fixed_point_t: con F == N el uno no es representable, "
                                 "asi que -- no tiene que restar");
            bruto_ -= one().bruto_;
            return *this;
        }

        /// @brief Post-incremento.
        constexpr fixed_point_t operator++(int) noexcept
        {
            const fixed_point_t antes{*this};
            ++(*this);
            return antes;
        }

        /// @brief Post-decremento.
        constexpr fixed_point_t operator--(int) noexcept
        {
            const fixed_point_t antes{*this};
            --(*this);
            return antes;
        }

        /// @brief Multiplicar por un entero **no** cambia la escala, asi que es
        ///        exacto y no necesita redondeo.
        ///
        /// Es `a * k`, no `a * b` entre dos puntos fijos: eso ultimo duplica los
        /// limbos fraccionarios y hay que redondear.
        constexpr fixed_point_t operator*(const entero &k) const noexcept { return desde_crudo(bruto_ * k); }

        // =====================================================================
        // Lo que si redondea: producto y division  (ADR-020)
        // =====================================================================
        //
        // Ni una es un algoritmo nuevo. `*` es `mul_wide` mas un desplazamiento,
        // `/` es un preescalado mas `divmod`, y `%` es `divmod` a secas.

        /// @brief Producto de dos puntos fijos. **Redondea** segun `Redondeo`.
        ///
        /// `a*b = (A*B)/2^(2k)`, y el crudo del resultado es `(A*B)/2^k`: sobran
        /// justo los `k` bits bajos del producto.
        ///
        /// `mul_wide` devuelve el producto **exacto** en `2N` limbos, asi que
        /// **todos los bits que se descartan estan ya calculados** y no hace
        /// falta guardar ni uno de mas. Los bits de guarda son de la coma
        /// flotante, donde el producto se trunca al calcularlo (ADR-019).
        [[nodiscard]] constexpr fixed_point_t operator*(const fixed_point_t &o) const noexcept
        {
            if constexpr (F == 0)
                return desde_crudo(bruto_ * o.bruto_); // sin escala no hay nada que descartar
            else
            {
                const ancho producto = mul_wide(bruto_, o.bruto_);
                return desde_crudo(redondea_desplazando(producto));
            }
        }

        /// @brief Division de dos puntos fijos. **Redondea** segun `Redondeo`.
        ///
        /// `a/b = A/B`, y el crudo del resultado es `A*2^k/B`: el dividendo se
        /// preescala a `2N` limbos y se divide. Lo que decide el redondeo es el
        /// resto de esa division.
        ///
        /// @throws std::domain_error si `o` es cero, igual que en el entero
        ///         (ADR-004). Por eso no es `noexcept`.
        [[nodiscard]] constexpr fixed_point_t operator/(const fixed_point_t &o) const
        {
            const ancho num = desplaza_ancho(ensancha(bruto_));
            const ancho den = ensancha(o.bruto_);
            return desde_crudo(redondea_dividiendo(num, den));
        }

        /// @brief Resto de la division. **Es exacto: no redondea** (ADR-020).
        ///
        /// Sigue a `std::fmod`: el resto de truncar el cociente hacia cero, con
        /// el signo del dividendo. Y resulta ser el `%` de los crudos sin mas,
        /// porque **el resto siempre es representable**: `a` y `b` son multiplos
        /// de `epsilon`, el cociente truncado es entero, luego `q*b` es multiplo
        /// de `epsilon` y `r = a - q*b` tambien. No hay nada que descartar.
        ///
        /// @warning La identidad `a == (a/b)*b + a%b` **no se cumple** cuando
        ///          `/` redondea, porque entonces `a/b` ya no es el cociente
        ///          truncado. Vale entre `%` y el cociente **truncado**, no
        ///          entre `%` y `operator/`.
        ///
        /// @throws std::domain_error si `o` es cero.
        [[nodiscard]] constexpr fixed_point_t operator%(const fixed_point_t &o) const
        {
            return desde_crudo(bruto_ % o.bruto_);
        }

        /// @brief Producto en sitio.
        constexpr fixed_point_t &operator*=(const fixed_point_t &o) noexcept
        {
            *this = *this * o;
            return *this;
        }

        /// @brief Producto en sitio por un entero (exacto).
        constexpr fixed_point_t &operator*=(const entero &k) noexcept
        {
            bruto_ *= k;
            return *this;
        }

        /// @brief Division en sitio.
        constexpr fixed_point_t &operator/=(const fixed_point_t &o)
        {
            *this = *this / o;
            return *this;
        }

        /// @brief Resto en sitio.
        constexpr fixed_point_t &operator%=(const fixed_point_t &o)
        {
            bruto_ %= o.bruto_;
            return *this;
        }

        // =====================================================================
        // Desplazamientos: escalar por potencias de dos
        // =====================================================================
        //
        // En punto fijo `<<` y `>>` escalan el VALOR, no reacomodan bits: es lo
        // que hacen los tipos de coma fija del TR 18037, y lo que hace que
        // `x >> 1` valga `x / 2`.
        //
        // No hay `&`, `|`, `^` ni `~`. El entero los tiene, pero ahi operan
        // sobre el valor de un entero; aqui no significarian nada util --ni
        // `float` ni los tipos del TR 18037 los ofrecen-- y anadirlos seria
        // inventar semantica en vez de seguir el estandar.

        /// @brief `x << n` es `x * 2^n`. **Exacto** salvo desbordamiento, que lo
        ///        decide `overflow_policy`.
        [[nodiscard]] constexpr fixed_point_t operator<<(unsigned n) const noexcept
        {
            return desde_crudo(bruto_ << n);
        }

        /// @brief `x >> n` es `x / 2^n`, y **redondea** segun `Redondeo`.
        ///
        /// No es un desplazamiento de bits disfrazado: al bajar `n` posiciones
        /// se caen `n` bits por abajo, y que se caigan es exactamente el caso
        /// que la perilla decide. Con `toward_neg_inf` sale el desplazamiento
        /// aritmetico tal cual, que es gratis.
        [[nodiscard]] constexpr fixed_point_t operator>>(unsigned n) const noexcept
        {
            if (n == 0U)
                return *this;

            using U = fixed_int_t<N, signedness::unsigned_type, representation_form::binnat, Policy>;
            constexpr unsigned bits_tipo = 64U * static_cast<unsigned>(N);

            const entero suelo = bruto_ >> n;
            const U trozos{bruto_};

            bool resto_cero{}, empate{}, pasa_mitad{};
            if (n >= bits_tipo)
            {
                // Se cae TODO: el resto es el valor entero. Y `2^n` es tan
                // grande que la mitad solo puede alcanzarse justo en `n` igual
                // al ancho; por encima, `2r < 2^n` siempre.
                resto_cero = trozos.is_zero();
                const U mitad = U::one() << (bits_tipo - 1U);
                empate = (n == bits_tipo) && (trozos == mitad);
                pasa_mitad = (n == bits_tipo) && (mitad < trozos);
            }
            else
            {
                const U mascara = (U::one() << n) - U::one();
                const U r = trozos & mascara;
                const U mitad = U::one() << (n - 1U);
                resto_cero = r.is_zero();
                empate = (r == mitad);
                pasa_mitad = (mitad < r);
            }

            const bool sube = decide(resto_cero, pasa_mitad, empate, es_impar(suelo), suelo.is_negative());
            return desde_crudo(sumar_si(suelo, sube));
        }

        /// @brief Desplazamiento a la izquierda en sitio.
        constexpr fixed_point_t &operator<<=(unsigned n) noexcept
        {
            bruto_ <<= n;
            return *this;
        }

        /// @brief Desplazamiento a la derecha en sitio. **Redondea.**
        constexpr fixed_point_t &operator>>=(unsigned n) noexcept
        {
            *this = *this >> n;
            return *this;
        }

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

        /// @brief Decimal con `decimales` cifras tras la coma, **redondeando**
        ///        segun `Redondeo`.
        ///
        /// Se redondea el VALOR, no la magnitud, que es la unica forma de que
        /// los modos dirigidos salgan bien: `toward_pos_inf` sobre `-2,55` con
        /// una cifra da `-2,5`, no `-2,6`. Redondear la magnitud y pegar el
        /// signo despues daria lo segundo.
        ///
        /// Para llegar ahi se escriben las cifras de la magnitud y luego se pasa
        /// al marco `(q, r, d)` de ADR-020:
        ///
        ///     positivo             q = Dmag,       r = resto
        ///     negativo, resto > 0  q = -(Dmag+1),  r = 2^k - resto
        ///
        /// de modo que en los negativos **subir el valor es NO subir la
        /// magnitud**, y al reves. La paridad que mira el desempate al par es la
        /// de `q`, que en los negativos es la de `Dmag+1`.
        ///
        /// @param decimales Cuantas cifras tras la coma. Con `0` no se escribe ni
        ///        la coma. Con `F == 0` se escriben igual, todas cero: el valor
        ///        es entero, y `to_string(3)` de un siete es `7.000`, como
        ///        `printf("%.3f", 7.0)`.
        ///
        /// @note Un negativo que redondea a cero sale con signo --`-0.00`--,
        ///       igual que `printf`. Este tipo no tiene cero negativo; lo que
        ///       dice esa cadena es «un negativo pequeno», que es informacion.
        [[nodiscard]] std::string to_string(unsigned decimales = 6) const
        {
            const bool negativo = is_negative();
            // Se trabaja con la magnitud para escribir las cifras, y el marco
            // (q, r, d) devuelve el signo al final.
            const fixed_point_t mag = negativo ? -(*this) : *this;

            // ...salvo en el minimo, donde NO hay magnitud a la que pasarse:
            // `-min()` envuelve y vuelve a dar `min()`. Sale bien igualmente
            // porque el minimo **no tiene parte fraccionaria** --sus limbos
            // bajos son todos cero-- asi que no hay nada que redondear y el
            // suelo ES el valor, con su propio signo.
            const bool envolvio = mag.is_negative();

            // --- las cifras de la magnitud, sin coma todavia -----------------
            std::string cifras = mag.suelo().to_string();

            // El producto por diez va al doble de ancho a proposito: con
            // `F == N` no queda parte entera donde recoger la cifra, `resto*10`
            // desborda y `resto >> 64*N` desplaza el ancho completo, con lo que
            // `0,5` se imprimia como «0.0».
            using U2 = u_ancho;
            U2 resto{mag.parte_fraccionaria()};
            const U2 diez{std::uint64_t{10}};
            const U2 escala = U2::one() << static_cast<unsigned>(escala_bits);

            for (unsigned i = 0; i < decimales; ++i)
            {
                resto = resto * diez;
                const U2 cifra = resto / escala;
                cifras += static_cast<char>('0' + static_cast<char>(cifra.limb(0) % 10U));
                resto = resto - (cifra * escala);
            }

            // --- el redondeo de la ultima cifra ------------------------------
            //
            // Lo que queda mas alla de la ultima cifra es `resto/escala`, en
            // [0,1) y en unidades de esa ultima cifra. De ahi salen las tres
            // senales de siempre.
            if (!resto.is_zero() && !envolvio)
            {
                // `resto` es lo que queda de la MAGNITUD. El marco (q, r, d)
                // es del VALOR, y en los negativos no son lo mismo: si a la
                // magnitud le sobra poquito, al valor le falta casi todo.
                //
                //     positivo             q = Dmag,       r = resto
                //     negativo, resto > 0  q = -(Dmag+1),  r = escala - resto
                const U2 r = negativo ? (escala - resto) : resto;

                const U2 media = escala >> 1U;
                const bool empate = (r == media);
                const bool pasa_mitad = (media < r);

                // La paridad de `q`. En los negativos `q = -(Dmag+1)`, asi que
                // es la CONTRARIA de la ultima cifra escrita.
                const bool ultima_impar = ((cifras.back() - '0') % 2) != 0;
                const bool q_impar = negativo ? !ultima_impar : ultima_impar;

                const bool sube_el_valor = decide(false, pasa_mitad, empate, q_impar, negativo);

                // Y aqui el espejo: en los negativos, subir el VALOR es no subir
                // la MAGNITUD, porque la magnitud crece hacia abajo.
                if (negativo ? !sube_el_valor : sube_el_valor)
                    incrementa_decimal(cifras);
            }

            // --- la coma, que va `decimales` cifras desde la derecha ----------
            if (decimales > 0)
                cifras.insert(cifras.size() - decimales, 1, '.');

            return (negativo && !envolvio) ? ("-" + cifras) : cifras;
        }

    private:
        /// El entero, ya escalado: el valor es `bruto_ / 2^(64*F)`.
        entero bruto_{};

        // =====================================================================
        // La maquinaria del redondeo  (ADR-020, decision 3)
        // =====================================================================

        /// El doble de ancho, donde caben el producto exacto y el dividendo
        /// preescalado.
        using ancho = fixed_int_t<2 * N, Sign, Form, Policy>;

        /// Sube un crudo de `N` limbos al tipo ancho, conservando el VALOR.
        [[nodiscard]] static constexpr ancho ensancha(const entero &x) noexcept { return ancho{x}; }

        /// Multiplica por `2^k` dentro del tipo ancho.
        [[nodiscard]] static constexpr ancho desplaza_ancho(const ancho &x) noexcept
        {
            if constexpr (F == 0)
                return x;
            else
                return x << static_cast<unsigned>(escala_bits);
        }

        /// Baja del tipo ancho al crudo. Si no cabe, manda `overflow_policy`.
        [[nodiscard]] static constexpr entero estrecha(const ancho &x) noexcept { return entero{x}; }

        /// @brief `p / 2^k`, redondeado: el camino del producto.
        ///
        /// `q` es el desplazamiento **aritmetico**, que es el suelo, y `r` son
        /// los `k` bits bajos, que nunca son negativos. Con `d = 2^k`, el empate
        /// es `r == 2^(k-1)` y pasarse de la mitad es `r > 2^(k-1)`.
        [[nodiscard]] static constexpr entero redondea_desplazando(const ancho &p) noexcept
        {
            const unsigned k = static_cast<unsigned>(escala_bits);

            const ancho suelo = p >> k;

            // Los k bits bajos, leidos SIN signo: son el `r`, y `r >= 0` es lo
            // que hace que la formula valga igual para negativos.
            using UAncho = u_ancho;
            const UAncho bits{p};
            const UAncho mascara = (UAncho::one() << k) - UAncho::one();
            const UAncho r = bits & mascara;
            const UAncho mitad = UAncho::one() << (k - 1U);

            const bool resto_cero = r.is_zero();
            const bool empate = (r == mitad);
            const bool pasa_mitad = (mitad < r);

            const bool sube = decide(resto_cero, pasa_mitad, empate, es_impar(suelo), suelo.is_negative());
            return sumar_si(estrecha(suelo), sube);
        }

        /// @brief `num / den`, redondeado: el camino de la division.
        ///
        /// `divmod` da el cociente **truncado** y un resto con el signo del
        /// dividendo. Para llegar al suelo hay que bajar uno cuando el resto no
        /// es cero y los signos difieren; es el unico ajuste de signo de todo
        /// esto, y son tres lineas.
        [[nodiscard]] static constexpr entero redondea_dividiendo(const ancho &num, const ancho &den)
        {
            auto [q, r] = ancho::divmod(num, den);

            if constexpr (Sign == signedness::signed_type)
            {
                // De cociente truncado a suelo.
                if (!r.is_zero() && (num.is_negative() != den.is_negative()))
                {
                    q -= ancho::one();
                    r += den;
                }
            }

            // `r/den` es ahora la fraccion no negativa que queda. Se compara
            // `2|r|` con `|den|`, que es el `d` de la formula.
            const ancho rm = valor_absoluto(r);
            const ancho dm = valor_absoluto(den);

            const bool resto_cero = rm.is_zero();
            // `2*rm` puede no caber, asi que se compara `rm` con `dm/2` y se
            // corrige con la paridad de `dm`: `2rm > dm` <=> `rm > dm/2` o
            // (`rm == dm/2` y `dm` impar).
            const ancho media = dm >> 1U; // suelo de d/2
            //
            // `2r > d` sale igual con `d` par y con `d` impar, y es solo
            // `media < r`:
            //   d = 2m    ->  2r > 2m    <=>  r > m
            //   d = 2m+1  ->  2r > 2m+1  <=>  2r >= 2m+2  <=>  r > m
            //
            // El empate, en cambio, SOLO puede darse con `d` par: con `d`
            // impar, `2r` nunca lo iguala. Y ahi estuvo el fallo -- escribi
            // `|| (dm_impar && rm == media)` creyendo que ese caso se pasaba de
            // la mitad, cuando `r = m` con `d = 2m+1` da `2r = 2m < d`: esta
            // por DEBAJO. El efecto era que cuatro de los cinco modos se
            // comportaban como `toward_pos_inf` siempre que el divisor era
            // impar, que es justo lo que saco el oraculo con `1/3`.
            const bool empate = (!es_impar(dm) && rm == media);
            const bool pasa_mitad = (media < rm);

            const bool sube = decide(resto_cero, pasa_mitad, empate, es_impar(q), q.is_negative());
            return sumar_si(estrecha(q), sube);
        }

        /// El valor absoluto dentro del tipo ancho. Nunca desborda aqui, porque
        /// los dos usos vienen de valores de `N` limbos ya ensanchados.
        template <typename T>
        [[nodiscard]] static constexpr T valor_absoluto(const T &x) noexcept
        {
            if constexpr (Sign == signedness::signed_type)
                return x.is_negative() ? -x : x;
            else
                return x;
        }

        /// La paridad del VALOR, no de los bits: en Exceso-K no son lo mismo.
        template <typename T>
        [[nodiscard]] static constexpr bool es_impar(const T &x) noexcept
        {
            return !(x & T::one()).is_zero();
        }

        [[nodiscard]] static constexpr entero sumar_si(const entero &q, bool sube) noexcept
        {
            return sube ? (q + entero::one()) : q;
        }

        /// @brief `detalle_redondeo::sube` con la perilla ya puesta.
        ///
        /// Ahorra repetir `Redondeo` en los cuatro sitios que deciden, y de paso
        /// deja las llamadas lo bastante cortas como para que **clang-format 21
        /// y 22 las formateen igual**: una llamada larga admite varios repartos
        /// y cada version elige el suyo (ADR-013).
        [[nodiscard]] static constexpr bool decide(bool resto_cero, bool pasa_mitad, bool empate,
                                                   bool q_impar, bool q_negativo) noexcept
        {
            return detalle_redondeo::sube(Redondeo, resto_cero, pasa_mitad, empate, q_impar, q_negativo);
        }

        /// @brief El doble de ancho, sin signo: donde se leen los bits que se
        ///        descartan y donde se hacen las cuentas de `to_string`.
        using u_ancho = fixed_int_t<2 * N, signedness::unsigned_type, representation_form::binnat, Policy>;

        /// @brief Suma uno a una cadena de cifras decimales, con acarreo.
        ///
        /// Es lo que obliga a redondear un decimal de longitud fija: el acarreo
        /// no se queda en la parte fraccionaria. `0.99` con dos cifras sube a
        /// `1.00`, y `9.99` a `10.00`, que ademas **alarga la cadena**. Por eso
        /// la coma se mete al final y contando desde la derecha.
        static void incrementa_decimal(std::string &cifras)
        {
            for (std::size_t i = cifras.size(); i > 0;)
            {
                --i;
                if (cifras[i] != '9')
                {
                    ++cifras[i];
                    return;
                }
                cifras[i] = '0';
            }
            cifras.insert(cifras.begin(), '1');
        }

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
    template <std::size_t N, std::size_t F, overflow_policy Policy = overflow_policy::wrap,
              rounding_mode Redondeo = rounding_mode::to_nearest_even>
    using ufixed_point_t =
        fixed_point_t<N, F, signedness::unsigned_type, representation_form::binnat, Policy, Redondeo>;

    /// @brief Punto fijo con signo, en complemento a dos.
    template <std::size_t N, std::size_t F, overflow_policy Policy = overflow_policy::wrap,
              rounding_mode Redondeo = rounding_mode::to_nearest_even>
    using sfixed_point_t =
        fixed_point_t<N, F, signedness::signed_type, representation_form::twos_complement, Policy, Redondeo>;

    /// @brief 64 enteros y 64 fraccionarios, sin signo. El «Q64.64» de toda la vida.
    using ufixed_64_64_t = ufixed_point_t<2, 1>;
    /// @brief 64 enteros y 64 fraccionarios, con signo.
    using sfixed_64_64_t = sfixed_point_t<2, 1>;

} // namespace nstd

#endif // NSTD_FIXED_POINT_T_HPP
