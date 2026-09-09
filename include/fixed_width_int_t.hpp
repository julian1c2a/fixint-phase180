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
// @file       fixed_width_int_t.hpp
// @brief      fixed_int_t<N, Sign, Form>: entero de N x 64 bits
// @author     Julián Calderón Almendros
// @date       2026-08-23 (last edit)
// @version    1.90.0
// =============================================================================

/**
 * @file fixed_width_int_t.hpp
 * @brief `fixed_int_t<N, Sign, Form>`: entero de anchura fija de N x 64 bits.
 *
 * @details
 * Un unico template cubre los enteros con y sin signo de cualquier anchura
 * multiplo de 64 bits. Los limbos se guardan en orden little-endian:
 * `limb(0)` es el menos significativo y `limb(N-1)` el mas significativo.
 *
 * | Alias | N | Bits |
 * |---|---|---|
 * | `uint64_fixed_t` / `int64_fixed_t`   | 1 | 64  |
 * | `uint128_fixed_t` / `int128_fixed_t` | 2 | 128 |
 * | `uint256_fixed_t` / `int256_fixed_t` | 4 | 256 |
 * | `uint512_fixed_t` / `int512_fixed_t` | 8 | 512 |
 *
 * **Semantica.** Toda la aritmetica es modular respecto a 2^(64N), igual que la
 * de los enteros built-in sin signo y la de los enteros con signo en
 * complemento a dos de C++20. La division trunca hacia cero y el resto toma el
 * signo del dividendo, como en C++. `min() / -1` envuelve a `min()` en lugar de
 * ser comportamiento indefinido.
 *
 * **constexpr.** Todas las operaciones son evaluables en tiempo de compilacion,
 * division y modulo incluidos. La division por cero lanza `std::domain_error`,
 * lo que en contexto constante se traduce en un error de compilacion, igual que
 * `1/0` con un `int`.
 *
 * **Rendimiento.** Hay caminos rapidos por plataforma (intrinsecos de MSVC,
 * `unsigned __int128` en GCC/Clang, asm en ICX-Windows), Karatsuba para N=4 y
 * N=8 en la multiplicacion, y el algoritmo D de Knuth para la division con
 * divisores de dos o mas limbos.
 *
 * @see fixed_int_iostreams.hpp  para `operator<<` y `operator>>`
 * @see fixed_int_format.hpp     para `std::format`
 * @see fixed_int_hash.hpp       para `std::hash`
 * @see fixed_int_limits.hpp     para `std::numeric_limits`
 * @see fixed_int_concepts.hpp   para `nstd::integral` y companyia
 */

// =============================================================================
// fixed_width_int_t.hpp — Fixed-width integer templates (N x 64-bit limbs)
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// fixed_int_t<N, Sign, Form>: unified signed/unsigned fixed-width integer.
// data[0] = lowest limb (LSB), data[N-1] = highest limb (MSB).
//
// uint_fixed_t<N> = fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>
// int_fixed_t<N>  = fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>
//
// N=1 -> 64-bit, N=2 -> 128-bit, N=4 -> 256-bit, N=8 -> 512-bit
//
// Operations (all mod 2^(64N)):
//   Construction: from any integral T, from limb array, from bytes, from bitset
//   Assignment:   from any integral T
//   Arithmetic:   +, -, *, /, %, unary -, ++, --
//   Bitwise:      &, |, ^, ~, <<, >>
//   Comparison:   ==, !=, <, <=, >, >=
//   Conversion:   explicit operator bool/T/bytes/bitset; to_string/from_string
//   Utilities:    zero(), max(), min(), one(), is_zero(), bit_width(), popcount()

#ifndef FIXED_WIDTH_INT_T_HPP
#define FIXED_WIDTH_INT_T_HPP

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "representation.hpp"

#if __has_include("intrinsics/arithmetic_operations.hpp")
#include "intrinsics/arithmetic_operations.hpp"
#endif

#if __has_include("intrinsics/bit_operations.hpp")
#include "intrinsics/bit_operations.hpp"
#endif

namespace nstd
{

    // =========================================================================
    // Parse error codes and result type
    // =========================================================================

/// @def NSTD_PARSE_COMMON_DEFINED
/// @brief Guarda para que `parse_error` y `parse_result` se definan una sola
///        vez: los declaran tanto esta cabecera como la de `int128_param_t`, y
///        un programa puede incluir las dos.
#ifndef NSTD_PARSE_COMMON_DEFINED
#define NSTD_PARSE_COMMON_DEFINED
    /// @brief Motivo por el que fallo una conversion desde cadena.
    ///
    /// Lo devuelve `try_from_string()`, que **no lanza**: la cadena mal formada
    /// es un resultado esperable, no un error de programacion (ver ADR-004).
    /// `from_string()`, en cambio, traduce cada uno de estos codigos a la
    /// excepcion correspondiente.
    enum class parse_error : std::uint8_t
    {
        success = 0,             ///< No hubo error.
        null_pointer,            ///< Se paso un puntero nulo.
        empty_string,            ///< La cadena estaba vacia.
        invalid_base,            ///< La base no esta en [2, 36].
        invalid_base_value,      ///< El prefijo de base (`0x`, `0b`, `0`) no cuadra con la base pedida.
        invalid_character,       ///< Un caracter que no es digito ni separador.
        digit_out_of_range,      ///< Un digito valido pero fuera de la base (una `9` en base 8).
        no_digits,               ///< Solo habia signo, prefijo o separadores.
        overflow,                ///< El valor no cabe en `64 * N` bits.
        separator_at_boundaries, ///< Un separador al principio o al final del numero.
        unknown_error            ///< Reservado; no deberia salir.
    };

    /// @brief Resultado de una conversion desde cadena que no lanza.
    ///
    /// @tparam T Tipo del valor convertido.
    ///
    /// Si `error` es `parse_error::success`, `value` es el valor convertido. En
    /// cualquier otro caso `value` no significa nada y `error_index` dice en que
    /// posicion de la cadena se detecto el problema.
    template <typename T>
    struct parse_result
    {
        parse_error error;       ///< Codigo de error; `success` si todo fue bien.
        T value;                 ///< Valor convertido. Solo valido si `success()`.
        std::size_t error_index; ///< Posicion del fallo; `std::string::npos` si no lo hubo.

        /// @brief `true` si la conversion salio bien.
        constexpr bool success() const noexcept { return error == parse_error::success; }

        /// @brief Construye un resultado correcto con el valor por defecto de `T`.
        constexpr parse_result() noexcept
            : error(parse_error::success), value(T{}), error_index(std::string::npos)
        {
        }

        /// @brief Construye un resultado explicito.
        /// @param err Codigo de error.
        /// @param val Valor convertido, o el que sea si hubo error.
        /// @param idx Posicion del fallo en la cadena.
        constexpr parse_result(parse_error err, T val, std::size_t idx) noexcept
            : error(err), value(val), error_index(idx)
        {
        }
    };
#endif // NSTD_PARSE_COMMON_DEFINED

    // =============================================================================
    // Politica de desbordamiento  (ADR-007 a ADR-011)
    // =============================================================================

    /// @brief Que hace el tipo cuando una operacion desborda.
    ///
    /// Es el **cuarto parametro** de `fixed_int_t`, y su valor por defecto es
    /// `wrap`, de modo que todo el codigo escrito antes de existir este
    /// parametro sigue compilando y significando lo mismo.
    ///
    /// See ADR-007 (la politica como parametro) y ADR-008 (el diseno).
    enum class overflow_policy : std::uint8_t
    {
        /// @brief Envuelve modulo 2^(64N). Lo que hacen los enteros del
        ///        lenguaje, y lo que hacia esta biblioteca antes de existir
        ///        este parametro. **No cuesta ni un byte ni un ciclo.**
        wrap = 0,

        /// @brief Marca el valor como invalido al desbordar, y la marca se
        ///        propaga. El equivalente entero de un NaN; se consulta al
        ///        final con `valid()`.
        checked = 1,

        /// @brief Satura en `max()` o en `min()`. **Sin implementar**; el
        ///        enumerado nace con los cuatro valores para no ampliarlo
        ///        despues y romper el ABI de la plantilla (ADR-008).
        saturate = 2,

        /// @brief Aborta al desbordar. **Sin implementar.**
        trap = 3,
    };

    // -------------------------------------------------------------------------
    // Almacenamiento de la marca: un miembro que SOLO EXISTE con `checked`
    // -------------------------------------------------------------------------
    //
    // LA CONDICION ES EL ABI, NO EL COMPILADOR. MSVC ignora el
    // `[[no_unique_address]]` estandar por compatibilidad de ABI y ofrece el
    // suyo. E Intel ICX en Windows define `_MSC_VER` **y** `__clang__` --es
    // clang por dentro y ABI de MSVC por fuera--, asi que tambien lo ignora.
    //
    // Medido en los cuatro compiladores el 5 sep 2026. Con el atributo
    // estandar a secas, MSVC e Intel dan **40 bytes donde deben dar 32**: se
    // llevan por delante la invariante que ADR-009 promete, y en silencio. De
    // ahi que el `static_assert` de tamano de mas abajo no sea decorativo.
/// @def NSTD_NO_UNIQUE_ADDRESS
/// @brief El atributo que permite que un miembro vacio no ocupe nada, en la
///        forma que entienda el ABI de destino.
///
/// La condicion es el **ABI**, no el compilador: MSVC ignora el atributo
/// estandar por compatibilidad binaria, e Intel ICX en Windows tambien --define
/// `_MSC_VER` y `__clang__` a la vez--. Sin esta distincion, los dos dan 40
/// bytes donde deben dar 32.
#if defined(_MSC_VER)
#define NSTD_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NSTD_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

/// @name Anchuras en las que `operator*` toma el camino de Karatsuba
///
/// Son macros, y no constantes, para poder BARRERLAS desde la linea de ordenes
/// sin tocar el fichero: `-DNSTD_KARATSUBA_MAX=32`. El barrido es necesario
/// porque el umbral no se puede razonar solo con la cuenta de operaciones.
///
/// La teoria dice que Karatsuba gana a partir de alguna anchura --cambia
/// N^2 por N^1.585-- pero la constante que arrastra es grande: reparte,
/// suma, resta y vuelve a juntar. Medido el 6 sep 2026 frente a un escolar
/// DESENROLLADO, en N=4 y N=8 pierde en los cuatro compiladores. Donde esta
/// el cruce de verdad es lo que hay que medir, no suponer.
///
/// Solo se aplica a anchuras potencia de dos: el reparto de Karatsuba parte
/// el operando por la mitad y necesita que la mitad vuelva a ser par hasta
/// llegar a 1.
/// @{
/// @def NSTD_KARATSUBA_MIN
/// @brief Anchura minima, en limbos, a partir de la cual `operator*` toma el
///        camino de Karatsuba. Por defecto 32.
///
/// Medido el 6 sep 2026: por debajo de 32, un escolar desenrollado es mas
/// rapido que Karatsuba en los cuatro compiladores. A partir de 32, Karatsuba
/// gana en los cuatro.
#ifndef NSTD_KARATSUBA_MIN
#define NSTD_KARATSUBA_MIN 32
#endif

/// @def NSTD_KARATSUBA_MAX
/// @brief Anchura maxima con Karatsuba. Existe para poder acotar el barrido;
///        en la practica no estorba.
#ifndef NSTD_KARATSUBA_MAX
#define NSTD_KARATSUBA_MAX 4096
#endif

/// @def NSTD_DESENROLLA_MAX
/// @brief Anchura maxima que se multiplica con el escolar DESENROLLADO por
///        construccion. Por encima se usa el bucle. Por defecto 20.
///
/// Barrido del 6 sep 2026, GCC 16.2, minimo de 5 rondas, bucle frente a
/// desenrollado:
///
///     N=12  3,50x    N=18  1,40x    N=24  1,11x
///     N=14  4,58x    N=20  1,48x    N=28  1,07x
///     N=16  2,11x    N=22  1,25x
///
/// El coste por producto se dispara pasado N=16 --de 4,4 a 7,0 ciclos, que es
/// presion de registros-- pero la ganancia no se agota hasta cerca de N=24. El
/// tope se pone en 20, el ultimo punto con ganancia clara.
///
/// El tope se puso primero en 16, comparando N=16 con N=32 y sin mirar nada en
/// medio. Eso dejaba a N=17..31 --y a N=20 en particular-- con el camino MAS
/// LENTO de los tres: no son potencia de dos, asi que Karatsuba no les aplica,
/// y quedaban por encima del tope. Lo señalo el autor el 6 sep 2026.
///
/// Subirlo mas cuesta tiempo de compilacion --son N(N+1)/2 productos en linea
/// recta-- pero solo lo paga quien INSTANCIA esa anchura: para los tipos que
/// nadie usa, el coste es cero.
#ifndef NSTD_DESENROLLA_MAX
#define NSTD_DESENROLLA_MAX 20
#endif
    /// @}

    namespace detail
    {
        /// @brief Marca de un tipo que no la necesita. Vacia a proposito: con
        ///        `NSTD_NO_UNIQUE_ADDRESS` no ocupa nada.
        struct marca_ausente
        {
        };

        /// @brief El tipo del miembro de estado, segun la politica.
        ///
        /// Con `checked` es un **limbo entero de 64 bits**, no un `bool`. Por
        /// el alineamiento los dos costarian lo mismo --40 bytes en N=4--, y el
        /// limbo deja **63 bits libres** para lo que ADR-008 dejo contemplado
        /// sin implementar (`saturate`, `trap`) sin volver a cambiar el
        /// tamano del tipo, que es un cambio que solo se puede hacer una vez.
        template <overflow_policy P>
        using marca_de = std::conditional_t<P == overflow_policy::checked, std::uint64_t, marca_ausente>;
    } // namespace detail

    // Forward declaration so cross-type constructors can reference the alias
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    class fixed_int_t;

    // =============================================================================
    // Aliases (forward-declared so cross-type constructors compile)
    // =============================================================================

    /// @brief Entero **sin signo** de `64 * N` bits.
    /// @tparam N Numero de limbos de 64 bits.
    /// Sin signo implica `binnat`, y al reves (ver ADR-011): no hay signo que
    /// codificar. Preferir los alias por anchura (`uint256_fixed_t`, ...).
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    using uint_fixed_t = fixed_int_t<N, signedness::unsigned_type, representation_form::binnat, Policy>;

    /// @brief Entero **con signo** de `64 * N` bits, en complemento a dos.
    /// @tparam N Numero de limbos de 64 bits.
    /// Preferir los alias por anchura (`int256_fixed_t`, ...).
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    using int_fixed_t = fixed_int_t<N, signedness::signed_type, representation_form::twos_complement, Policy>;

    // =============================================================================
    // fixed_int_t<N, Sign, Form> — unified signed/unsigned N-limb integer
    // =============================================================================

    template <std::size_t N, signedness Sign = signedness::unsigned_type,
              representation_form Form =
                  (Sign == signedness::unsigned_type ? representation_form::binnat
                                                     : representation_form::twos_complement),
              // Sin `= wrap` aqui: el valor por defecto ya lo da la declaracion
              // adelantada de arriba, y repetirlo es un error de compilacion.
              overflow_policy Policy>
    /**
     * @brief Entero de anchura fija de N x 64 bits, con o sin signo.
     *
     * @tparam N     Numero de limbos de 64 bits. N >= 1.
     * @tparam Sign  `signedness::unsigned_type` o `signedness::signed_type`.
     * @tparam Form  Representacion interna. Se deduce de `Sign`: `binnat` para
     *               los tipos sin signo y `twos_complement` para los que tienen
     *               signo. Son las dos unicas combinaciones implementadas.
     *
     * @details
     * Toda la aritmetica es modular respecto a 2^(64N). El tipo es trivialmente
     * copiable, asi que `std::bit_cast` y `memcpy` funcionan sobre el; no es en
     * cambio un *structural type*, de modo que no puede usarse como parametro
     * no-tipo de plantilla (los limbos son privados desde la version 1.90).
     *
     * Preferir los alias `uint_fixed_t<N>` e `int_fixed_t<N>` a escribir la
     * plantilla completa.
     *
     * @code
     * using nstd::uint256_fixed_t;
     * constexpr uint256_fixed_t a{1000000};
     * constexpr auto b = a * a;          // constexpr, modular
     * static_assert(b / a == a);
     * @endcode
     */
    class fixed_int_t
    {
        static_assert(N >= 1, "fixed_int_t requires at least 1 limb");
        // Dos condiciones, y son de naturaleza distinta. Antes iban en un solo
        // `static_assert` que las mezclaba, y asi no se veia cual era una ley y
        // cual una tarea pendiente.

        // LEY. Sin signo y `binnat` son la misma cosa, en los dos sentidos: las
        // otras tres formas SON codificaciones del signo --complemento a dos,
        // magnitud-signo y exceso-K existen para representar negativos--, y un
        // tipo sin signo no tiene signo que codificar. «Sin signo en complemento
        // a dos» no es una combinacion restrictiva: es una SIN SIGNIFICADO.
        // Esto no se relaja nunca.
        //
        // See ADR-011.
        static_assert((Sign == signedness::unsigned_type) == (Form == representation_form::binnat),
                      "Sin signo implica binnat y binnat implica sin signo (ADR-011). "
                      "Preferir los alias uint_fixed_t<N> e int_fixed_t<N>, que ya lo cumplen.");

        // TAREA PENDIENTE. De las cuatro combinaciones que la ley admite
        // --binnat sin signo, y TC / MS / EK con signo--, hoy solo hay dos
        // implementadas. Esta condicion SE RELAJA al portar Magnitud-Signo y
        // Exceso-K, que es lo que decide ADR-006.
        static_assert(Form == representation_form::binnat || Form == representation_form::twos_complement,
                      "Magnitud-Signo y Exceso-K todavia no estan implementadas en fixed_int_t; "
                      "por ahora viven en int128_param_t (ADR-006).");

        // TAREA PENDIENTE, como la de las representaciones: el enumerado nace
        // con cuatro valores para no ampliarlo despues y romper el ABI de la
        // plantilla, pero hoy solo hay dos implementados (ADR-008, decision 1).
        static_assert(Policy == overflow_policy::wrap || Policy == overflow_policy::checked,
                      "Las politicas saturate y trap estan contempladas en el enumerado "
                      "pero todavia no implementadas (ADR-008).");

        static constexpr bool is_signed = (Sign == signedness::signed_type);

        // Cualquier otra instanciacion de fixed_int_t es amiga: los constructores
        // cross-tipo y los operadores cross-N/cross-signo necesitan leer los
        // limbos del otro tipo. Antes funcionaba solo porque `data` era publico.
        template <std::size_t, signedness, representation_form, overflow_policy>
        friend class fixed_int_t;

    public:
        /// @brief Si el tipo tiene signo. Copia del parametro `Sign`, consultable
        ///        desde codigo generico sin repetir la lista de parametros.
        static constexpr signedness sign{Sign};

        /// @brief Representacion interna. Copia del parametro `Form`. Sin signo
        ///        implica `binnat`, y al reves (ADR-011).
        static constexpr representation_form form{Form};

        /// @brief La politica de desbordamiento de este tipo, consultable desde
        ///        codigo generico sin repetir la lista de parametros.
        static constexpr overflow_policy policy{Policy};

        /// @brief Si este tipo puede quedar marcado como invalido, es decir, si
        ///        lleva el limbo de estado. Falso con `wrap`, que es el caso por
        ///        defecto y no paga nada (ADR-009).
        static constexpr bool comprueba_desbordamiento{Policy == overflow_policy::checked};

        // =========================================================================
        // Acceso a los limbos
        //
        // T2.4 (auditoria 23 ago 2026). `data` era publico, a diferencia de
        // `int128_param_t` en phase-1.75, que lo tenia privado con high()/low().
        // Se recupera la encapsulacion: `data` pasa a privado (ver mas abajo) y
        // el acceso se hace por estos accesores.
        //
        // Consecuencia asumida: fixed_int_t deja de ser *structural type*, asi que
        // ya no puede usarse como parametro no-tipo de plantilla (NTTP). Decision
        // tomada el 23 ago 2026: se deja decaer el NTTP a cambio de volver al
        // comportamiento de phase-1.75. Sigue siendo trivialmente copiable, asi
        // que std::bit_cast y memcpy no se ven afectados.
        //
        // limb(i) / set_limb(i, v): un limbo suelto, i en [0, N).
        // limbs():                  el array completo (solo lectura).
        // limbs_ref():              el array completo (escritura); uso interno y
        //                           de tests que construyen patrones de bits.
        // =========================================================================

        /// @brief Limbo `i`, con `limb(0)` el menos significativo. `i` en [0, N).
        [[nodiscard]] constexpr std::uint64_t limb(std::size_t i) const noexcept { return data[i]; }

        /// @brief Fija el limbo `i`. No comprueba el rango: `i` debe estar en [0, N).
        constexpr void set_limb(std::size_t i, std::uint64_t v) noexcept { data[i] = v; }

        /// @brief Los N limbos en orden little-endian, solo lectura.
        [[nodiscard]] constexpr const std::array<std::uint64_t, N> &limbs() const noexcept { return data; }

        /// @brief Los N limbos en orden little-endian, con escritura.
        /// @warning Permite dejar el valor en cualquier estado: usar con cuidado.
        [[nodiscard]] constexpr std::array<std::uint64_t, N> &limbs_ref() noexcept { return data; }

    private:
        // data[0] = least-significant limb, data[N-1] = most-significant limb
        std::array<std::uint64_t, N> data{};

        /// @brief Marca de invalido. **Solo existe con `Policy == checked`**;
        ///        con `wrap` es un tipo vacio y no ocupa nada.
        ///
        /// Cero significa valido. Es un limbo entero y no un `bool` a
        /// proposito: por alineamiento cuestan lo mismo, y los 63 bits que
        /// sobran quedan para `saturate` y `trap` sin volver a cambiar el
        /// tamano del tipo (ADR-008, ADR-009).
        NSTD_NO_UNIQUE_ADDRESS detail::marca_de<Policy> estado{};

        // =========================================================================
        // La marca: como se hereda y como se pone  (ADR-008, ADR-010)
        // =========================================================================
        //
        // Con `wrap` todo esto se compila a nada: los `if constexpr` descartan el
        // cuerpo y no queda ni una instruccion. Es lo que hace que la politica no
        // cueste nada al caso por defecto.

        /// @brief Cero si el valor es de fiar; distinto de cero si no.
        [[nodiscard]] static constexpr std::uint64_t marca_de_valor(const fixed_int_t &x) noexcept
        {
            if constexpr (Policy == overflow_policy::checked)
                return x.estado;
            else
                return 0;
        }

        /// @brief Marca este valor a partir de dos operandos y de si la operacion
        ///        desbordo.
        ///
        /// La marca es **pegajosa**: se hereda de los operandos con un OR, de modo
        /// que cualquier operacion con un operando invalido produce un resultado
        /// invalido, sin importar si esta operacion concreta desbordo o no. Es la
        /// regla de propagacion de ADR-008.
        constexpr void marcar(const fixed_int_t &a, const fixed_int_t &b, bool desbordo) noexcept
        {
            if constexpr (Policy == overflow_policy::checked)
                estado = a.estado | b.estado | (desbordo ? std::uint64_t{1} : std::uint64_t{0});
        }

        /// @brief La misma, para operaciones de un solo operando.
        constexpr void marcar(const fixed_int_t &a, bool desbordo) noexcept
        {
            if constexpr (Policy == overflow_policy::checked)
                estado = a.estado | (desbordo ? std::uint64_t{1} : std::uint64_t{0});
        }

        /// @brief Si con esta politica hay que molestarse en detectar nada.
        ///        Con `wrap`, no: envolver es el comportamiento pedido.
        static constexpr bool detecta = (Policy == overflow_policy::checked);

        /// @brief Si el producto `a * b` no cabe en `64 * N` bits.
        ///
        /// Calcula el producto completo de `2N` limbos y mira lo que sobra por
        /// arriba. Es exacto, no una estimacion: sin signo desborda si algun
        /// limbo alto es distinto de cero; con signo, si los limbos altos no son
        /// la extension de signo del bit mas alto de la parte baja. Es la misma
        /// comprobacion que hace `checked_mul`.
        ///
        /// @note **Cuesta un producto de doble anchura.** Es el precio de
        ///       `checked`, y solo lo paga `checked`: con `wrap` esta funcion no
        ///       se llama nunca porque el `if constexpr` la descarta.
        [[nodiscard]] static constexpr bool producto_desborda(const fixed_int_t &a,
                                                              const fixed_int_t &b) noexcept
        {
            // Producto escolar sobre 2N limbos, sin truncar.
            std::array<std::uint64_t, 2 * N> p{};
            for (std::size_t i{0}; i < N; ++i)
            {
                std::uint64_t acarreo{0};
                for (std::size_t j{0}; j < N; ++j)
                {
                    std::uint64_t hi{0};
#if __has_include("intrinsics/arithmetic_operations.hpp")
                    const std::uint64_t lo = intrinsics::umul128(a.data[i], b.data[j], &hi);
#else
                    const std::uint64_t x0 = a.data[i] & 0xFFFFFFFFULL, x1 = a.data[i] >> 32;
                    const std::uint64_t y0 = b.data[j] & 0xFFFFFFFFULL, y1 = b.data[j] >> 32;
                    const std::uint64_t p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, p11 = x1 * y1;
                    const std::uint64_t medio = (p00 >> 32) + (p01 & 0xFFFFFFFFULL) + (p10 & 0xFFFFFFFFULL);
                    const std::uint64_t lo = (p00 & 0xFFFFFFFFULL) | (medio << 32);
                    hi = p11 + (p01 >> 32) + (p10 >> 32) + (medio >> 32);
#endif
                    std::uint64_t v = p[i + j];
                    std::uint64_t suma = v + lo;
                    std::uint64_t c1 = (suma < v) ? 1u : 0u;
                    v = suma + acarreo;
                    c1 += (v < suma) ? 1u : 0u;
                    p[i + j] = v;
                    acarreo = hi + c1;
                }
                p[i + N] += acarreo;
            }

            if constexpr (is_signed)
            {
                // CORRECCION DE SIGNO. Lo de arriba es el producto de los PATRONES
                // DE BITS, o sea el producto SIN signo. Para leerlo como producto
                // con signo hay que restar de la mitad alta cada operando cuando el
                // OTRO es negativo:
                //
                //     con_signo(a*b) = sin_signo(a*b) - (a<0 ? b<<64N : 0)
                //                                     - (b<0 ? a<<64N : 0)
                //
                // Sin esta correccion `(-1) * (-1)` decia que desbordaba: el
                // producto sin signo de 0xFF..FF por si mismo tiene la mitad alta
                // llena de unos y se leia como que no cabia. Lo cazo
                // test_fixed_signed el 5 sep 2026, con cuatro fallos identicos.
                auto restar_en_alta = [&p](const fixed_int_t &x) noexcept
                {
                    std::uint64_t prestamo{0};
                    for (std::size_t k{0}; k < N; ++k)
                    {
                        const std::uint64_t minuendo = p[N + k];
                        const std::uint64_t quito = x.data[k];
                        const std::uint64_t d1 = minuendo - quito;
                        const std::uint64_t d2 = d1 - prestamo;
                        prestamo = ((minuendo < quito) ? 1u : 0u) + ((d1 < prestamo) ? 1u : 0u);
                        p[N + k] = d2;
                    }
                };
                if (a.is_negative())
                    restar_en_alta(b);
                if (b.is_negative())
                    restar_en_alta(a);

                // Ahora si: la mitad alta tiene que ser exactamente la extension de
                // signo del bit mas alto de la parte baja.
                const std::uint64_t relleno = (p[N - 1] >> 63) != 0 ? ~std::uint64_t{0} : std::uint64_t{0};
                for (std::size_t i{N}; i < 2 * N; ++i)
                    if (p[i] != relleno)
                        return true;
                return false;
            }
            else
            {
                for (std::size_t i{N}; i < 2 * N; ++i)
                    if (p[i] != 0)
                        return true;
                return false;
            }
        }

    public:
        // =========================================================================
        // Construction
        // =========================================================================

        constexpr fixed_int_t() noexcept = default;

        /// @brief Construye desde un entero del lenguaje.
        ///
        /// Es `explicit`, como todos (ADR-001): `f(42)` no compila, hay que
        /// escribir `f(uint256_fixed_t{42})`.
        ///
        /// @tparam T Cualquier entero del lenguaje **salvo `bool`**.
        /// @param v Valor de partida. Si `T` tiene signo y `v` es negativo, se
        ///          **extiende el signo** a todos los limbos, de modo que en un
        ///          tipo sin signo `-1` se convierte en `max()`, igual que en C++.
        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        explicit constexpr fixed_int_t(T v) noexcept : data{}
        {
            data[0] = static_cast<std::uint64_t>(v);
            const std::uint64_t fill = (std::is_signed_v<T> && v < 0) ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{1}; i < N; ++i)
                data[i] = fill;
        }

        // Construct from array of limbs (data[0]=LSB, data[N-1]=MSB)
        /// @brief Construye desde los limbos en crudo.
        /// @param limbs Los N limbos en **orden little-endian**: `limbs[0]` es el
        ///        menos significativo (ADR-002). No se valida nada.
        explicit constexpr fixed_int_t(std::array<std::uint64_t, N> limbs) noexcept : data{limbs} {}

        /// @brief Construye desde su representacion en bytes.
        ///
        /// Inverso exacto de la conversion a `std::array<std::byte, N * 8>`: la
        /// ida y vuelta no pierde nada.
        ///
        /// @param bytes Los `8N` bytes en orden little-endian. La lectura se hace
        ///        con desplazamientos, no con `memcpy`, asi que **el resultado no
        ///        depende del orden de bytes de la maquina** y el formato es
        ///        portable entre plataformas.
        explicit constexpr fixed_int_t(const std::array<std::byte, N * 8> &bytes) noexcept : data{}
        {
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 8; ++j)
                    data[i] |= static_cast<std::uint64_t>(bytes[i * 8 + j]) << (j * 8);
        }

        /// @brief Construye desde un `std::bitset`.
        /// @param b Bits del valor; el bit 0 es el menos significativo.
        explicit constexpr fixed_int_t(const std::bitset<64 * N> &b) noexcept : data{}
        {
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 64; ++j)
                    if (b.test(i * 64 + j))
                        data[i] |= std::uint64_t{1} << j;
        }

#ifdef __SIZEOF_INT128__
        explicit constexpr fixed_int_t(unsigned __int128 v) noexcept : data{}
        {
            for (std::size_t i{0}; i < N && i < 2; ++i)
                data[i] = static_cast<std::uint64_t>(v >> (i * 64));
        }

        explicit constexpr fixed_int_t(__int128 v) noexcept : data{}
        {
            const unsigned __int128 uv = static_cast<unsigned __int128>(v);
            for (std::size_t i{0}; i < N && i < 2; ++i)
                data[i] = static_cast<std::uint64_t>(uv >> (i * 64));
            const std::uint64_t fill = (v < 0) ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{2}; i < N; ++i)
                data[i] = fill;
        }
#endif

        // Cross-type constructor: from any other fixed_int_t<M, S2, F2>
        // Handles: same-sign different-N, different-sign, or any combination
        /// @brief Convierte desde otro `fixed_int_t` de anchura o signo distintos.
        ///
        /// Cubre las tres mezclas a la vez: distinta N, distinto signo, o ambas.
        ///
        /// - **Al ensanchar** (`M < N`): si el origen tiene signo y es negativo se
        ///   **extiende el signo**; si no, se rellena con ceros. El valor se
        ///   conserva.
        /// - **Al estrechar** (`M > N`): se **truncan** los limbos sobrantes, es
        ///   decir se toma el valor modulo 2^(64N), igual que la conversion entre
        ///   enteros del lenguaje. Puede cambiar el valor, y no avisa.
        ///
        /// @param o Valor de partida.
        ///
        /// @note **La politica tambien puede cambiar.** Antes de P1.5 este
        ///       constructor tomaba `fixed_int_t<M, S2, F2>` --tres parametros--
        ///       es decir, solo origenes con la politica por defecto. Con eso,
        ///       `uint_fixed_t<N, checked>{int_fixed_t<N, checked>}` no
        ///       compilaba, y con ello se caia la division con signo de un tipo
        ///       `checked`, que la usa por dentro.
        ///
        /// @note Si **los dos** son `checked` y el origen esta marcado, el
        ///       destino sale marcado: una conversion no limpia un valor que ya
        ///       era invalido. Al convertir a `wrap` la marca se pierde, que es
        ///       lo que se esta pidiendo al elegir `wrap`.
        template <std::size_t M, signedness S2, representation_form F2, overflow_policy P2,
                  typename = std::enable_if_t<(M != N || S2 != Sign || F2 != Form || P2 != Policy)>>
        explicit constexpr fixed_int_t(const fixed_int_t<M, S2, F2, P2> &o) noexcept : data{}
        {
            constexpr std::size_t copy = M < N ? M : N;
            for (std::size_t i{0}; i < copy; ++i)
                data[i] = o.data[i];
            // sign-extend if source is signed and negative
            const bool src_neg = (S2 == signedness::signed_type) && ((o.data[M - 1] >> 63) != 0);
            const std::uint64_t fill = src_neg ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{M}; i < N; ++i)
                data[i] = fill;

            if constexpr (Policy == overflow_policy::checked && P2 == overflow_policy::checked)
            {
                if (!o.valid())
                    estado = std::uint64_t{1};
            }
        }

        // Construccion desde punto flotante.
        //
        // Valores no finitos (T2.2 — auditoria 23 ago 2026): antes se colaban al
        // bucle de abajo, donde std::fmod(inf, 2^64) da NaN y el
        // static_cast<uint64_t>(NaN) es comportamiento indefinido (en la practica
        // producia un valor basura cercano a 2^255 para uint_fixed_t<4>). Ahora se
        // saturan de forma definida, en linea con la conversion float->int con
        // saturacion de otros lenguajes y con std::numeric_limits:
        //
        //   NaN   -> 0
        //   +inf  -> max()
        //   -inf  -> min()   (con signo)  /  0  (sin signo)
        //
        // Los valores finitos fuera de rango siguen truncandose modulo 2^(64N),
        // igual que la conversion entre enteros built-in.
        /// @brief Construye truncando un valor de coma flotante hacia cero.
        ///
        /// @tparam F `float`, `double` o `long double`.
        /// @param v Valor de partida.
        ///
        /// Los **valores no finitos saturan** de forma definida, en vez de ser
        /// comportamiento indefinido:
        ///
        /// | `v` | resultado |
        /// |---|---|
        /// | `NaN` | 0 |
        /// | `+inf` | `max()` |
        /// | `-inf` | `min()` con signo, 0 sin signo |
        ///
        /// Los valores finitos fuera de rango se truncan modulo 2^(64N), igual
        /// que la conversion entre enteros del lenguaje.
        ///
        /// @note No es `constexpr`: usa `std::isfinite` y `std::fmod`.
        template <typename F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
        explicit fixed_int_t(F v) noexcept : data{}
        {
            if (!std::isfinite(v))
            {
                if (v != v) // NaN
                    return; // queda en cero
                if (v > F{0})
                    *this = max();
                else if constexpr (is_signed)
                    *this = min();
                // sin signo y -inf: queda en cero
                return;
            }

            if constexpr (is_signed)
            {
                if (v >= F{0})
                    *this = fixed_int_t{uint_fixed_t<N>{v}};
                else
                    *this = fixed_int_t{-uint_fixed_t<N>{-v}};
            }
            else
            {
                if (!(v >= F{1}))
                    return;
                constexpr long double base = 18446744073709551616.0L; // 2^64
                long double tmp = std::trunc(static_cast<long double>(v));
                for (std::size_t i{0}; i < N && tmp >= 1.0L; ++i)
                {
                    const long double rem = std::fmod(tmp, base);
                    data[i] = static_cast<std::uint64_t>(rem);
                    tmp = std::trunc(tmp / base);
                }
            }
        }

        // =========================================================================
        // Named constructors
        // =========================================================================

        /// @brief El valor cero.
        static constexpr fixed_int_t zero() noexcept { return fixed_int_t{}; }

        /// @brief El valor uno.
        static constexpr fixed_int_t one() noexcept { return fixed_int_t{std::uint64_t{1}}; }

        /// @brief Mayor valor representable: 2^(64N)-1 sin signo, 2^(64N-1)-1 con signo.
        static constexpr fixed_int_t max() noexcept
        {
            fixed_int_t r{};
            if constexpr (!is_signed)
            {
                for (auto &limb : r.data)
                    limb = ~std::uint64_t{0};
            }
            else
            {
                for (auto &limb : r.data)
                    limb = ~std::uint64_t{0};
                r.data[N - 1] >>= 1; // clear MSB
            }
            return r;
        }

        /// @brief Menor valor representable: 0 sin signo, -2^(64N-1) con signo.
        static constexpr fixed_int_t min() noexcept
        {
            if constexpr (!is_signed)
            {
                return fixed_int_t{};
            }
            else
            {
                fixed_int_t r{};
                r.data[N - 1] = std::uint64_t{1} << 63;
                return r;
            }
        }

        // Aliases for compatibility with old int_fixed_t
        template <bool S = is_signed, typename = std::enable_if_t<S>>
        /// @brief El mayor valor representable.
        /// @return `2^(64N) - 1` sin signo; `2^(64N-1) - 1` con signo.
        static constexpr fixed_int_t max_val() noexcept
        {
            return max();
        }

        template <bool S = is_signed, typename = std::enable_if_t<S>>
        /// @brief El menor valor representable.
        /// @return 0 sin signo; `-2^(64N-1)` con signo. Ese minimo **no tiene
        ///         opuesto** representable: `-min_val()` es `min_val()`, igual
        ///         que con los `int` del lenguaje.
        static constexpr fixed_int_t min_val() noexcept
        {
            return min();
        }

        // =========================================================================
        // Assignment
        // =========================================================================

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator=(T v) noexcept
        {
            return *this = fixed_int_t{v};
        }

#ifdef __SIZEOF_INT128__
        constexpr fixed_int_t &operator=(unsigned __int128 v) noexcept { return *this = fixed_int_t{v}; }

        constexpr fixed_int_t &operator=(__int128 v) noexcept { return *this = fixed_int_t{v}; }
#endif

        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<(M != N || S2 != Sign || F2 != Form)>>
        constexpr fixed_int_t &operator=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            return *this = fixed_int_t{o};
        }

        template <typename F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
        fixed_int_t &operator=(F v) noexcept
        {
            return *this = fixed_int_t{v};
        }

        // =========================================================================
        // Predicates
        // =========================================================================

        /// @brief `true` si todos los limbos son cero.
        constexpr bool is_zero() const noexcept
        {
            for (const auto &limb : data)
                if (limb != 0)
                    return false;
            return true;
        }

        /// @brief Un valor marcado como invalido desde el principio.
        ///
        /// Solo existe con `Policy == checked`. Hace falta para las operaciones
        /// que **no tienen resultado**, no que se salen de rango: dividir por
        /// cero es el caso claro. El valor que se guarda es el que se pase, y no
        /// significa nada mientras la marca este puesta.
        ///
        /// @param valor El numero que queda dentro; cero si no se dice otro.
        /// @return Ese valor, marcado.
        template <bool P = (Policy == overflow_policy::checked), typename = std::enable_if_t<P>>
        [[nodiscard]] static constexpr fixed_int_t invalido(const fixed_int_t &valor = fixed_int_t{}) noexcept
        {
            fixed_int_t r{valor};
            r.estado = 1;
            return r;
        }

        /// @brief Si el valor es de fiar, es decir, si no lo ha marcado un
        ///        desbordamiento.
        ///
        /// Con `Policy == wrap` devuelve **siempre `true`**: envolver no es un
        /// error, es el comportamiento pedido, igual que en los enteros del
        /// lenguaje. Con `checked`, un desbordamiento en cualquier punto de la
        /// cadena deja la marca puesta y ya no se quita:
        ///
        /// @code
        /// using u256c = uint_fixed_t<4, overflow_policy::checked>;
        /// u256c r = a * b + c;        // aritmetica normal, noexcept, constexpr
        /// if (!r.valid()) { ... }     // algo desbordo por el camino
        /// @endcode
        ///
        /// See ADR-008 (la marca pegajosa) y ADR-010 (como comparan los
        /// invalidos: se **ordenan**, no se vuelven incomparables).
        [[nodiscard]] constexpr bool valid() const noexcept
        {
            if constexpr (Policy == overflow_policy::checked)
                return estado == 0;
            else
                return true;
        }

        /// @brief Si el valor es negativo.
        /// @return El bit de signo, con signo; siempre `false` sin signo.
        constexpr bool is_negative() const noexcept
        {
            if constexpr (!is_signed)
                return false;
            else
                return (data[N - 1] >> 63) != 0;
        }

        // =========================================================================
        // Explicit conversions
        // =========================================================================

        [[nodiscard]] explicit constexpr operator bool() const noexcept { return !is_zero(); }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        [[nodiscard]] explicit constexpr operator T() const noexcept
        {
            return static_cast<T>(data[0]);
        }

        [[nodiscard]] explicit constexpr operator std::array<std::byte, N * 8>() const noexcept
        {
            std::array<std::byte, N * 8> result{};
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 8; ++j)
                    result[i * 8 + j] = static_cast<std::byte>((data[i] >> (j * 8)) & 0xFF);
            return result;
        }

        [[nodiscard]] explicit constexpr operator std::bitset<64 *N>() const noexcept
        {
            std::bitset<64 * N> result{};
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 64; ++j)
                    if ((data[i] & (std::uint64_t{1} << j)) != 0)
                        result.set(i * 64 + j);
            return result;
        }

#ifdef __SIZEOF_INT128__
        [[nodiscard]] explicit constexpr operator unsigned __int128() const noexcept
        {
            unsigned __int128 r{0};
            for (std::size_t i{0}; i < N && i < 2; ++i)
                r |= static_cast<unsigned __int128>(data[i]) << (i * 64);
            return r;
        }

        [[nodiscard]] explicit constexpr operator __int128() const noexcept
        {
            return static_cast<__int128>(static_cast<unsigned __int128>(*this));
        }
#endif

        template <typename F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
        [[nodiscard]] explicit operator F() const noexcept
        {
            if constexpr (is_signed)
            {
                if (is_negative())
                    return -static_cast<F>(uint_fixed_t<N>{-(*this)});
                return static_cast<F>(uint_fixed_t<N, Policy>{*this});
            }
            else
            {
                constexpr long double base = 18446744073709551616.0L; // 2^64
                long double result{0};
                for (std::size_t i{N}; i-- > 0;)
                    result = result * base + static_cast<long double>(data[i]);
                return static_cast<F>(result);
            }
        }

        // =========================================================================
        // Comparison
        // =========================================================================

        /// @brief Igualdad.
        ///
        /// Con `checked` compara **primero la marca y despues el valor**: dos
        /// invalidos distintos NO son iguales, porque ADR-009 conserva el valor al
        /// desbordar y dos desbordamientos con resultados distintos son valores
        /// distintos.
        ///
        /// Lo importante es que **`x == x` es siempre cierto**, tambien para un
        /// invalido. Es lo que separa esto del NaN, y lo que hace que
        /// `unordered_map` y `unordered_set` sigan siendo correctos: la igualdad
        /// es una relacion de equivalencia. Ver ADR-010.
        constexpr bool operator==(const fixed_int_t &o) const noexcept
        {
            if constexpr (Policy == overflow_policy::checked)
                return estado == o.estado && data == o.data;
            else
                return data == o.data;
        }

        constexpr bool operator!=(const fixed_int_t &o) const noexcept { return !(*this == o); }

        /// @brief Menor que, con **orden total** aunque haya invalidos.
        ///
        /// Comparacion lexicografica sobre `(valido?, valor)`: un invalido es
        /// mayor que cualquier valido, y entre dos del mismo estado deciden los
        /// limbos. Ver ADR-010.
        ///
        /// La logica de la marca esta AQUI y no en `operator<=>` a proposito: en
        /// C++20 los relacionales se sintetizan desde `<=>` **salvo que exista un
        /// `operator<` propio**, y este existe. Ponerla solo en `<=>` dejaba a
        /// `>`, `std::sort` y `std::max_element` usando el orden de siempre.
        constexpr bool operator<(const fixed_int_t &o) const noexcept
        {
            if constexpr (Policy == overflow_policy::checked)
            {
                const bool a_mal = (estado != 0);
                const bool b_mal = (o.estado != 0);
                if (a_mal != b_mal)
                    return b_mal; // el invalido es el mayor: a < b si b es el malo
            }
            if constexpr (is_signed)
            {
                const bool a_neg = is_negative();
                const bool b_neg = o.is_negative();
                if (a_neg != b_neg)
                    return a_neg;
                // same sign: unsigned limb comparison
            }
            for (std::size_t i{N}; i-- > 0;)
            {
                if (data[i] != o.data[i])
                    return data[i] < o.data[i];
            }
            return false;
        }

        constexpr bool operator<=(const fixed_int_t &o) const noexcept { return !(o < *this); }

        constexpr bool operator>(const fixed_int_t &o) const noexcept { return o < *this; }

        constexpr bool operator>=(const fixed_int_t &o) const noexcept { return !(*this < o); }

        // Three-way comparison (C++20). Coexists with the 6 manual comparators
        // above — overload resolution prefers the explicit ones, so existing
        // call sites are unchanged. New code can use `a <=> b` directly, and
        // generic algorithms / containers that require <=> can now use
        // fixed_int_t. T2 — Fase MS-INTEROP.
        /// @brief Orden **total**, tambien con valores invalidos.
        ///
        /// Comparacion lexicografica sobre `(valido?, valor)`: si uno es invalido
        /// y el otro no, **el invalido es el mayor**; si los dos tienen el mismo
        /// estado, se comparan los valores.
        ///
        /// Devuelve `std::strong_ordering` con cualquier politica, a proposito.
        /// Imitar al NaN --comparaciones falsas salvo `!=`-- habria roto el orden
        /// debil estricto y convertido en comportamiento indefinido meter un
        /// invalido en un `std::map`, un `std::set` o un `std::sort`. Se descarto
        /// por eso: la propagacion por la aritmetica, que es lo que de verdad se
        /// queria, no dependia de aquello. Ver ADR-010.
        ///
        /// El invalido va **arriba** para que el veneno salga a la superficie: al
        /// ordenar queda al final, y `std::max_element` sobre un rango
        /// contaminado devuelve el invalido en vez de esconderlo.
        constexpr std::strong_ordering operator<=>(const fixed_int_t &o) const noexcept
        {
            // Delega en `operator<`, que es donde vive el criterio de orden --marca
            // incluida-- para que no haya dos sitios que puedan discrepar.
            if (*this < o)
                return std::strong_ordering::less;
            if (o < *this)
                return std::strong_ordering::greater;
            return std::strong_ordering::equal;
        }

        // =========================================================================
        // Bitwise
        // =========================================================================

        constexpr fixed_int_t operator~() const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = ~data[i];
            return r;
        }

        constexpr fixed_int_t operator&(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] & o.data[i];
            return r;
        }

        constexpr fixed_int_t operator|(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] | o.data[i];
            return r;
        }

        constexpr fixed_int_t operator^(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] ^ o.data[i];
            return r;
        }

        constexpr fixed_int_t &operator&=(const fixed_int_t &o) noexcept
        {
            *this = *this & o;
            return *this;
        }

        constexpr fixed_int_t &operator|=(const fixed_int_t &o) noexcept
        {
            *this = *this | o;
            return *this;
        }

        constexpr fixed_int_t &operator^=(const fixed_int_t &o) noexcept
        {
            *this = *this ^ o;
            return *this;
        }

        // Left shift (logical for both signed and unsigned)
        /// @brief Desplazamiento a la izquierda.
        ///
        /// @note Con `checked` marca si se **pierden bits por arriba**, que es lo
        ///       que un desplazamiento a la izquierda hace cuando desborda. El
        ///       contador saturado a `64*N` cuenta como desbordamiento salvo que
        ///       el valor sea cero.
        constexpr fixed_int_t operator<<(unsigned shift) const noexcept
        {
            if constexpr (detecta)
            {
                // Desborda si algun bit distinto de cero sale por arriba, es
                // decir si el valor tiene mas de `64*N - shift` bits utiles.
                const unsigned ancho = 64u * static_cast<unsigned>(N);
                const bool desbordo = !is_zero() && (shift >= ancho || bit_width() + shift > ancho);
                fixed_int_t r = this->shl_sin_marca(shift);
                r.marcar(*this, desbordo);
                return r;
            }
            return this->shl_sin_marca(shift);
        }

        /// @brief El desplazamiento de siempre, sin tocar la marca.
        constexpr fixed_int_t shl_sin_marca(unsigned shift) const noexcept
        {
            fixed_int_t r{};
            if (shift >= 64U * N)
                return r;
            const std::size_t ls = shift / 64U;
            const unsigned bs = shift % 64U;
            if (bs == 0)
            {
                for (std::size_t i{ls}; i < N; ++i)
                    r.data[i] = data[i - ls];
            }
            else
            {
                for (std::size_t i{ls}; i < N; ++i)
                {
                    r.data[i] = data[i - ls] << bs;
                    if (i > ls)
                        r.data[i] |= data[i - ls - 1] >> (64U - bs);
                }
            }
            return r;
        }

        // Right shift: logical for unsigned, arithmetic for signed
        constexpr fixed_int_t operator>>(unsigned shift) const noexcept
        {
            if constexpr (!is_signed)
            {
                fixed_int_t r{};
                if (shift >= 64U * N)
                    return r;
                const std::size_t ls = shift / 64U;
                const unsigned bs = shift % 64U;
                if (bs == 0)
                {
                    for (std::size_t i{0}; i + ls < N; ++i)
                        r.data[i] = data[i + ls];
                }
                else
                {
                    for (std::size_t i{0}; i + ls < N; ++i)
                    {
                        r.data[i] = data[i + ls] >> bs;
                        if (i + ls + 1 < N)
                            r.data[i] |= data[i + ls + 1] << (64U - bs);
                    }
                }
                return r;
            }
            else
            {
                // arithmetic right shift
                if (shift == 0)
                    return *this;
                if (shift >= 64U * N)
                    return is_negative() ? fixed_int_t{std::int64_t{-1}} : zero();
                if (!is_negative())
                    return fixed_int_t{uint_fixed_t<N, Policy>{*this} >> shift};
                const uint_fixed_t<N, Policy> fill = uint_fixed_t<N, Policy>::max() << (64U * N - shift);
                return fixed_int_t{(uint_fixed_t<N, Policy>{*this} >> shift) | fill};
            }
        }

        constexpr fixed_int_t &operator<<=(unsigned shift) noexcept
        {
            if constexpr (detecta)
            {
                *this = *this << shift;
                return *this;
            }
            *this = *this << shift;
            return *this;
        }

        constexpr fixed_int_t &operator>>=(unsigned shift) noexcept
        {
            *this = *this >> shift;
            return *this;
        }

        // =========================================================================
        // Shift overloads with fixed_int_t<M, S2, F2> count  (T1 — Fase MS-INTEROP)
        //
        // Built-in semantics: `x << n` accepts any integral n; negative or
        // out-of-range counts yield UB. Aqui NO hay UB: el resultado esta siempre
        // definido y coincide con el de la sobrecarga `unsigned`.
        //
        // T2.3 (auditoria 23 ago 2026). Antes se hacia
        // `static_cast<unsigned>(shift.data[0])`, que truncaba los limbos altos:
        // `x << u256{2^64}` devolvia `x` en vez de 0, porque data[0] valia 0. Y
        // `x << u256{2^32}` devolvia `x << 0` por el truncado a 32 bits de
        // `unsigned`. Ahora cualquier contador que no quepa en el rango util
        // [0, 64N) satura a 64N, que es justo el camino de "desplazamiento
        // completo" de la sobrecarga `unsigned`:
        //
        //   contador >= 64N  ->  0  (o relleno de signo en >> con signo)
        //   contador < 0     ->  idem (un contador negativo es, en complemento a
        //                        dos, un valor enorme: satura igual)
        // =========================================================================

        // Reduce un contador de desplazamiento fixed_int_t<M,S2,F2> a `unsigned`,
        // saturando a 64*N cuando no cabe o es negativo.
        /// @brief Reduce un contador de desplazamiento a `unsigned`, **saturando**.
        ///
        /// Permite escribir `x << y` con `y` de otro `fixed_int_t`. El contador
        /// se satura a `64 * N` —lo que hace que el desplazamiento de un valor
        /// completo— cuando no cabe en un `unsigned` o cuando es negativo.
        ///
        /// Saturar en vez de truncar es deliberado: truncando, `x << 2^64` daria
        /// `x << 0`, es decir `x`, que es justo lo contrario de lo que cabe
        /// esperar. Era un fallo real, corregido en v1.90.1.
        ///
        /// @param shift Contador, de cualquier anchura y signo.
        /// @return El contador, o `64 * N` si no cabe o es negativo.
        template <std::size_t M, signedness S2, representation_form F2>
        static constexpr unsigned shift_count_of(const fixed_int_t<M, S2, F2> &shift) noexcept
        {
            constexpr unsigned saturated = 64U * static_cast<unsigned>(N);

            // Contador negativo (solo posible si el tipo del contador tiene signo).
            if constexpr (S2 == signedness::signed_type)
            {
                if ((shift.data[M - 1] >> 63) != 0)
                    return saturated;
            }

            // Cualquier limbo por encima del bajo distinto de cero => enorme.
            for (std::size_t i{1}; i < M; ++i)
                if (shift.data[i] != 0)
                    return saturated;

            const std::uint64_t low = shift.data[0];
            if (low >= static_cast<std::uint64_t>(saturated))
                return saturated;

            return static_cast<unsigned>(low);
        }

        template <std::size_t M, signedness S2, representation_form F2>
        constexpr fixed_int_t operator<<(const fixed_int_t<M, S2, F2> &shift) const noexcept
        {
            return *this << shift_count_of(shift);
        }

        template <std::size_t M, signedness S2, representation_form F2>
        constexpr fixed_int_t operator>>(const fixed_int_t<M, S2, F2> &shift) const noexcept
        {
            return *this >> shift_count_of(shift);
        }

        template <std::size_t M, signedness S2, representation_form F2>
        constexpr fixed_int_t &operator<<=(const fixed_int_t<M, S2, F2> &shift) noexcept
        {
            return *this <<= shift_count_of(shift);
        }

        template <std::size_t M, signedness S2, representation_form F2>
        constexpr fixed_int_t &operator>>=(const fixed_int_t<M, S2, F2> &shift) noexcept
        {
            return *this >>= shift_count_of(shift);
        }

        // =========================================================================
        // Arithmetic — addition/subtraction (ripple-carry via intrinsics or portable)
        // =========================================================================

        constexpr fixed_int_t operator+(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            unsigned char carry{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                carry = intrinsics::addcarry_u64(carry, data[i], o.data[i], &r.data[i]);
#else
                const std::uint64_t s = data[i] + o.data[i] + carry;
                carry = static_cast<unsigned char>((s < data[i]) || (carry != 0 && s == data[i]) ? 1 : 0);
                r.data[i] = s;
#endif
            }
            if constexpr (detecta)
            {
                // Sin signo: desborda si sale acarreo del limbo mas alto.
                // Con signo: desborda si los dos sumandos tienen el mismo signo y
                // el resultado tiene otro. Es la regla clasica del complemento a
                // dos, la misma que ya usaba `checked_add`.
                bool desbordo;
                if constexpr (is_signed)
                {
                    const bool sa = is_negative();
                    desbordo = (sa == o.is_negative()) && (r.is_negative() != sa);
                }
                else
                {
                    desbordo = (carry != 0);
                }
                r.marcar(*this, o, desbordo);
            }
            return r;
        }

        constexpr fixed_int_t operator-(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            unsigned char borrow{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                borrow = intrinsics::subborrow_u64(borrow, data[i], o.data[i], &r.data[i]);
#else
                const std::uint64_t d = data[i] - o.data[i] - borrow;
                borrow = static_cast<unsigned char>(
                    (data[i] < o.data[i]) || (borrow != 0 && data[i] == o.data[i]) ? 1 : 0);
                r.data[i] = d;
#endif
            }
            if constexpr (detecta)
            {
                // Sin signo: desborda por abajo si sale prestamo, es decir si el
                // sustraendo era mayor. Con signo: si los operandos tienen signos
                // distintos y el resultado no tiene el del minuendo.
                bool desbordo;
                if constexpr (is_signed)
                {
                    const bool sa = is_negative();
                    desbordo = (sa != o.is_negative()) && (r.is_negative() != sa);
                }
                else
                {
                    desbordo = (borrow != 0);
                }
                r.marcar(*this, o, desbordo);
            }
            return r;
        }

        /// @brief Opuesto.
        ///
        /// @note Con `checked` y **sin signo**, `-x` marca para todo `x` distinto
        ///       de cero: el resultado matematico es negativo y no cabe. Es
        ///       coherente con `checked_sub(0, x)`, que devuelve `nullopt`.
        ///       Con signo, solo marca en `min()`, cuyo opuesto no es
        ///       representable.
        constexpr fixed_int_t operator-() const noexcept
        {
            fixed_int_t r = ~(*this) + one();
            if constexpr (detecta)
            {
                // `if constexpr` y no un ternario: `min_val()` solo existe para
                // tipos con signo, y un ternario de ejecucion instanciaria sus
                // dos brazos aunque solo se ejecute uno.
                if constexpr (is_signed)
                    r.marcar(*this, *this == min_val());
                else
                    r.marcar(*this, !is_zero());
            }
            return r;
        }

        /// Unary plus — returns a copy. Mirrors built-in `+x` semantics.
        constexpr fixed_int_t operator+() const noexcept { return *this; }

        constexpr fixed_int_t &operator+=(const fixed_int_t &o) noexcept
        {
            // Con `checked` se delega en `operator+`, que ya detecta y marca: la
            // deteccion vive en un solo sitio. Con `wrap` sigue el camino rapido
            // de siempre, sin tocar ni una instruccion.
            if constexpr (detecta)
            {
                *this = *this + o;
                return *this;
            }
            unsigned char carry{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                carry = intrinsics::addcarry_u64(carry, data[i], o.data[i], &data[i]);
#else
                const std::uint64_t a = data[i];
                const std::uint64_t s = a + o.data[i] + carry;
                carry = static_cast<unsigned char>((s < a) || (carry != 0 && s == a) ? 1 : 0);
                data[i] = s;
#endif
            }
            return *this;
        }

        constexpr fixed_int_t &operator-=(const fixed_int_t &o) noexcept
        {
            if constexpr (detecta)
            {
                *this = *this - o;
                return *this;
            }
            unsigned char borrow{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                borrow = intrinsics::subborrow_u64(borrow, data[i], o.data[i], &data[i]);
#else
                const std::uint64_t a = data[i];
                const std::uint64_t d = a - o.data[i] - borrow;
                borrow =
                    static_cast<unsigned char>((a < o.data[i]) || (borrow != 0 && a == o.data[i]) ? 1 : 0);
                data[i] = d;
#endif
            }
            return *this;
        }

        constexpr fixed_int_t &operator++() noexcept
        {
            *this += one();
            return *this;
        }

        constexpr fixed_int_t operator++(int) noexcept
        {
            fixed_int_t tmp{*this};
            ++(*this);
            return tmp;
        }

        constexpr fixed_int_t &operator--() noexcept
        {
            *this -= one();
            return *this;
        }

        constexpr fixed_int_t operator--(int) noexcept
        {
            fixed_int_t tmp{*this};
            --(*this);
            return tmp;
        }

        // =========================================================================
        // Arithmetic — multiplication (mod 2^(64N))
        //
        // N=2 fast path:
        //   GCC/Clang/ICX (has __uint128_t): single __uint128_t multiply (constexpr-safe)
        //   MSVC x64 (no __uint128_t):       _umul128 + two 64-bit muls (runtime only)
        // Karatsuba (runtime only): T(N)=3·T(N/2)+O(N), T(2)=3 umul128
        //   anchuras segun NSTD_KARATSUBA_MIN/MAX (por defecto 4 y 8)
        //   kmul_full<N/2> for the full lower product; half-width operator* for middle terms
        //   N=4: 9 umul128+0 (vs 10 schoolbook); N=8: 19 umul128+8 muls (vs 36)
        // Fallback: schoolbook O(N^2) fuera de esas anchuras, o constexpr en MSVC
        // =========================================================================

        constexpr fixed_int_t operator*(const fixed_int_t &o) const noexcept
        {
            // Con `checked`, la deteccion se hace ANTES y el resultado se calcula
            // por el camino de siempre. Separarlo asi evita tener que marcar en
            // los tres puntos de salida distintos que tiene este operador --el
            // camino rapido de N=2, el de Karatsuba y el escolar-- y evita que
            // alguno se quede sin marcar al tocarlo en el futuro.
            if constexpr (detecta)
            {
                const bool desbordo = producto_desborda(*this, o);
                fixed_int_t r = this->mul_sin_marca(o);
                r.marcar(*this, o, desbordo);
                return r;
            }
            return this->mul_sin_marca(o);
        }

        /// @brief El producto de siempre, modular y sin tocar la marca.
        ///        Es el cuerpo que tenia `operator*` antes de existir la politica.
        constexpr fixed_int_t mul_sin_marca(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};

#ifdef __SIZEOF_INT128__
            if constexpr (N == 2)
            {
                const std::uint64_t a0 = data[0], a1 = data[1];
                const std::uint64_t b0 = o.data[0], b1 = o.data[1];
                const unsigned __int128 p = static_cast<unsigned __int128>(a0) * b0;
                r.data[0] = static_cast<std::uint64_t>(p);
                r.data[1] = static_cast<std::uint64_t>(p >> 64) + a0 * b1 + a1 * b0;
                return r;
            }
#elif defined(_MSC_VER) && defined(_M_X64)
            if constexpr (N == 2)
            {
                if (!std::is_constant_evaluated())
                {
                    // 128x128 -> 128 (low): need only 3 of the 4 partial products
                    // a0*b0: full 128-bit product (both halves used)
                    // a0*b1, a1*b0: only low 64 bits (upper half -> result[2], discarded)
                    // a1*b1: entirely discarded (-> result[2+], mod 2^128)
                    std::uint64_t hi00;
                    const std::uint64_t lo00 = _umul128(data[0], o.data[0], &hi00);
                    r.data[0] = lo00;
                    r.data[1] = hi00 + data[0] * o.data[1] + data[1] * o.data[0];
                    return r;
                }
            }
#endif

            // Karatsuba: producto bajo completo con kmul_full<N/2>, terminos del
            // medio con el operator* de media anchura, que recurre solo.
            // Las anchuras las fijan NSTD_KARATSUBA_MIN/MAX, ver arriba.
            if constexpr ((N & (N - 1)) == 0 && N >= NSTD_KARATSUBA_MIN && N <= NSTD_KARATSUBA_MAX)
            {
                if (!std::is_constant_evaluated())
                {
                    constexpr std::size_t HH = N / 2;
                    using half_t = uint_fixed_t<HH>;

                    half_t a_lo{}, a_hi{}, b_lo{}, b_hi{};
                    for (std::size_t i = 0; i < HH; ++i)
                    {
                        a_lo.data[i] = data[i];
                        a_hi.data[i] = data[HH + i];
                        b_lo.data[i] = o.data[i];
                        b_hi.data[i] = o.data[HH + i];
                    }

                    const auto z0 = kmul_full<HH>(a_lo.data, b_lo.data);
                    const half_t mid = a_lo * b_hi + a_hi * b_lo;

                    for (std::size_t i = 0; i < N; ++i)
                        r.data[i] = z0[i];
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = add_limb_carry(r.data[HH + i], mid.data[i], c);
                    return r;
                }
            }

            // Escolar desenrollado, para las anchuras donde compensa.
            if constexpr (N <= NSTD_DESENROLLA_MAX)
            {
                filas_desenrolladas<0>(r.data, data, o.data);
                return r;
            }

            // Escolar O(N^2) en bucle: por encima del tope de desenrollado, o en
            // evaluacion constante con MSVC.
            for (std::size_t i{0}; i < N; ++i)
            {
                for (std::size_t j{0}; i + j < N; ++j)
                {
                    std::uint64_t hi{0};
                    const std::uint64_t lo = producto64(data[i], o.data[j], hi);
                    unsigned char c = add_limb(r.data[i + j], lo);
                    const std::size_t next = i + j + 1;
                    if (next < N)
                    {
                        c = add_limb_carry(r.data[next], hi, c);
                        for (std::size_t k{next + 1}; k < N && c; ++k)
                            c = add_limb(r.data[k], std::uint64_t{c});
                    }
                }
            }
            return r;
        }

        constexpr fixed_int_t &operator*=(const fixed_int_t &o) noexcept
        {
            if constexpr (detecta)
            {
                *this = *this * o;
                return *this;
            }
#ifdef __SIZEOF_INT128__
            if constexpr (N == 2)
            {
                // Schoolbook 128x128->128 with explicit 64-bit variables.
                // GCC/Clang generate better register allocation than a128*b128.
                const std::uint64_t a0 = data[0], a1 = data[1];
                const std::uint64_t b0 = o.data[0], b1 = o.data[1];
                const unsigned __int128 p = static_cast<unsigned __int128>(a0) * b0;
                data[0] = static_cast<std::uint64_t>(p);
                data[1] = static_cast<std::uint64_t>(p >> 64) + a0 * b1 + a1 * b0;
                return *this;
            }
#elif defined(_MSC_VER) && defined(_M_X64)
            if constexpr (N == 2)
            {
                if (!std::is_constant_evaluated())
                {
                    // Los CUATRO limbos se leen antes de escribir nada. Es
                    // imprescindible: si `o` es este mismo objeto (`x *= x`, que
                    // es justo lo que hace `pow` en su bucle de cuadrados),
                    // escribir data[0] antes de leer o.data[0] y o.data[1]
                    // corrompe el resultado. La rama de GCC/Clang de arriba ya
                    // cacheaba b0 y b1; esta no, y por eso `pow(-2, 3)` daba un
                    // valor equivocado solo con MSVC.
                    const std::uint64_t a0 = data[0], a1 = data[1];
                    const std::uint64_t b0 = o.data[0], b1 = o.data[1];
                    std::uint64_t hi00;
                    const std::uint64_t lo00 = _umul128(a0, b0, &hi00);
                    data[0] = lo00;
                    data[1] = hi00 + a0 * b1 + a1 * b0;
                    return *this;
                }
            }
#endif
            *this = *this * o;
            return *this;
        }

        // =========================================================================
        // Arithmetic — division and modulo
        //
        // Unsigned N=2 fast paths (in order):
        //   [0]  a < b                  → {0, a}  (early-out, all N)
        //   [1]  __uint128_t available  → 128/64 or 128/128 via hardware  (GCC/Clang/ICX)
        //   [2]  MSVC x64               → 128/64 via _udiv128; 128/128 → binary long div
        //   [3]  fallback               → binary long division O(64N^2)
        //
        // Signed: reduce to unsigned, apply sign rules.
        // Throws std::domain_error on division by zero.
        // =========================================================================

        /// @brief Cociente y resto en una sola operacion.
        ///
        /// Calcula los dos a la vez, que cuesta lo mismo que uno solo: preferirla
        /// a hacer `a / b` y `a % b` por separado.
        ///
        /// Sin signo usa el algoritmo D de Knuth. Con signo reduce a la version
        /// sin signo y aplica las reglas de signo de C++: el cociente **trunca
        /// hacia cero** y el resto lleva **el signo del dividendo**.
        ///
        /// @param a Dividendo.
        /// @param b Divisor.
        /// @return `{cociente, resto}`, cumpliendo `a == q * b + r`.
        /// @throws std::domain_error si `b` es cero. Es la unica operacion
        ///         aritmetica que puede lanzar (ADR-004), y por eso ni ella ni
        ///         `/` ni `%` son `noexcept`. En contexto constante el `throw`
        ///         convierte la expresion en no-constante, es decir en un error
        ///         de compilacion, igual que `1 / 0` con un `int`.
        static constexpr std::pair<fixed_int_t, fixed_int_t> divmod(const fixed_int_t &a,
                                                                    const fixed_int_t &b)
        {
            if (b.is_zero())
                throw std::domain_error("fixed_int_t::divmod: division by zero");

            if constexpr (!is_signed)
            {
                if (a < b)
                    return {fixed_int_t{}, a};

// Intel ICX on Windows (MSVC ABI) defines __SIZEOF_INT128__ but ships without
// __udivti3/__umodti3 in its runtime — those calls would fail to link.
// We fall through to the _udiv128 path (same as pure MSVC) instead.
#if defined(__SIZEOF_INT128__) && !(defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)))
                if constexpr (N == 2)
                {
                    const unsigned __int128 u = (static_cast<unsigned __int128>(a.data[1]) << 64) | a.data[0];

                    if (b.data[1] == 0)
                    {
                        // Fast path: 64-bit divisor — single 128/64 hardware division
                        const std::uint64_t d = b.data[0];
                        const unsigned __int128 q128 = u / d;
                        const std::uint64_t rem = static_cast<std::uint64_t>(u % d);
                        fixed_int_t q{};
                        q.data[0] = static_cast<std::uint64_t>(q128);
                        q.data[1] = static_cast<std::uint64_t>(q128 >> 64);
                        return {q, fixed_int_t{rem}};
                    }

                    // General 128/128: compiler emits __udivti3 (Knuth D internally)
                    const unsigned __int128 v = (static_cast<unsigned __int128>(b.data[1]) << 64) | b.data[0];
                    const unsigned __int128 q128 = u / v;
                    const unsigned __int128 r128 = u % v;
                    fixed_int_t q{}, r{};
                    q.data[0] = static_cast<std::uint64_t>(q128);
                    q.data[1] = static_cast<std::uint64_t>(q128 >> 64);
                    r.data[0] = static_cast<std::uint64_t>(r128);
                    r.data[1] = static_cast<std::uint64_t>(r128 >> 64);
                    return {q, r};
                }
#else
                // MSVC e ICX-Windows: 128/64 en dos pasos. div_128_64 usa el
                // intrinseco (_udiv128) o el asm `divq` en ejecucion, y la version
                // portable en contexto constante (T3.1).
                if constexpr (N == 2)
                {
                    if (b.data[1] == 0)
                    {
                        const std::uint64_t d = b.data[0];
                        const std::uint64_t q_hi = a.data[1] / d;
                        const std::uint64_t r_hi = a.data[1] % d;
                        std::uint64_t rem = 0;
                        const std::uint64_t q_lo = div_128_64(r_hi, a.data[0], d, rem);
                        fixed_int_t q{};
                        q.data[0] = q_lo;
                        q.data[1] = q_hi;
                        return {q, fixed_int_t{rem}};
                    }
                    // 128/128: cae a la division larga binaria de mas abajo.
                }
#endif

                // ─────────────────────────────────────────────────────────────────
                // Single-limb divisor fast path — b fits entirely in data[0].
                //
                // Replaces O(64N²) binary long div with O(N) hardware DIV instructions:
                // iterate from MSL to LSL, each step divides (rem:a[i]) by d where
                // rem < d is a loop invariant (guaranteed: each remainder < divisor).
                //
                // With rem < d, libgcc's __udivti3 / _udiv128 emit a SINGLE divq.
                // Total cost: N divq instructions instead of 64N² bit-loop iterations.
                //
                // (For N=2 on GCC/Clang, the __uint128_t block above already handled
                //  the b.data[1]==0 case and returned; this is the critical path for N≥3.
                //  For N=2 on MSVC/ICX-Win, the check below finds single_limb=false
                //  since the 128/128 fallback only reaches here when b.data[1]!=0.)
                // ─────────────────────────────────────────────────────────────────
                {
                    bool single_limb_b = true;
                    for (std::size_t k = 1; k < N; ++k)
                        if (b.data[k] != 0)
                        {
                            single_limb_b = false;
                            break;
                        }

                    if (single_limb_b)
                    {
                        const std::uint64_t d = b.data[0];
                        fixed_int_t q{};
                        std::uint64_t rem = 0;
                        for (std::size_t i = N; i-- > 0;)
                        {
                            // rem < d es invariante del bucle, asi que esto es una
                            // sola instruccion DIV en hardware. div_128_64 encapsula
                            // intrinseco (ejecucion) vs. portable (constexpr) -- T3.1.
                            q.data[i] = div_128_64(rem, a.data[i], d, rem);
                        }
                        return {q, fixed_int_t{rem}};
                    }
                }

                // ─────────────────────────────────────────────────────────────────
                // Knuth Algorithm D — general N-limb ÷ M-limb (M ≥ 2)
                // TAOCP Vol. 2 §4.3.1. Base B = 2^64.
                // Reached only when b has ≥ 2 significant limbs.
                // ─────────────────────────────────────────────────────────────────
                {
                    // Count significant limbs of b (guaranteed ≥ 2 here)
                    std::size_t n = N;
                    while (b.data[n - 1] == 0)
                        --n;

                    const std::size_t m_quot = N - n; // quotient digits: q.data[0..m_quot]

                    // D1. Normalize: find shift s so that v[n-1] has its MSB set.
#if __has_include("intrinsics/bit_operations.hpp")
                    const int s = intrinsics::clz64(b.data[n - 1]);
#else
                    int s = 0;
                    {
                        std::uint64_t tmp = b.data[n - 1];
                        while ((tmp & (std::uint64_t{1} << 63)) == 0)
                        {
                            ++s;
                            tmp <<= 1;
                        }
                    }
#endif

                    std::array<std::uint64_t, N> v{};     // normalized divisor  [0..n-1]
                    std::array<std::uint64_t, N + 1> u{}; // normalized dividend [0..N]

                    if (s == 0)
                    {
                        for (std::size_t i = 0; i < n; ++i)
                            v[i] = b.data[i];
                        for (std::size_t i = 0; i < N; ++i)
                            u[i] = a.data[i];
                        // u[N] stays 0
                    }
                    else
                    {
                        for (std::size_t i = n - 1; i > 0; --i)
                            v[i] = (b.data[i] << s) | (b.data[i - 1] >> (64 - s));
                        v[0] = b.data[0] << s;
                        u[N] = a.data[N - 1] >> (64 - s);
                        for (std::size_t i = N - 1; i > 0; --i)
                            u[i] = (a.data[i] << s) | (a.data[i - 1] >> (64 - s));
                        u[0] = a.data[0] << s;
                    }

                    const std::uint64_t v1 = v[n - 1];
                    const std::uint64_t v2 = v[n - 2]; // safe: n ≥ 2

                    fixed_int_t q{};

                    // D2–D7. Main loop: j = m_quot down to 0
                    for (std::size_t j = m_quot + 1; j-- > 0;)
                    {
                        const std::uint64_t u0 = u[j + n];     // top window limb
                        const std::uint64_t u1 = u[j + n - 1]; // next limb
                        const std::uint64_t u2 = u[j + n - 2]; // limb below (n≥2,j≥0)

                        // D3. Estimate trial quotient q̂
                        std::uint64_t q_hat, r_hat = 0;
                        bool skip_refine = false;

                        if (u0 > v1)
                        {
                            // r̂ ≥ B for sure — skip refinement entirely
                            q_hat = ~std::uint64_t{0};
                            skip_refine = true;
                        }
                        else if (u0 == v1)
                        {
                            q_hat = ~std::uint64_t{0};
                            r_hat = u1 + v1;
                            skip_refine = (r_hat < u1); // overflow ⟹ r̂ ≥ B
                        }
                        else
                        {
                            // 0 <= u0 < v1: division 128/64 exacta (T3.1).
                            q_hat = div_128_64(u0, u1, v1, r_hat);
                        }

                        // D3. Refinement: while q̂·v2 > r̂·B + u2, do q̂--, r̂ += v1
                        if (!skip_refine)
                        {
                            while (true)
                            {
                                // Comparacion exacta q_hat*v2 <= r_hat*B + u2.
                                // Antes, en plataformas sin __int128 ni _umul128 se
                                // hacia `break` sin comparar y se dejaba que el
                                // add-back de D5 corrigiese; con mul_64x64 la
                                // comparacion es exacta en todas (T3.1).
                                std::uint64_t lhs_hi = 0;
                                const std::uint64_t lhs_lo = mul_64x64(q_hat, v2, lhs_hi);
                                if (lhs_hi < r_hat || (lhs_hi == r_hat && lhs_lo <= u2))
                                    break;
                                --q_hat;
                                const std::uint64_t r_new = r_hat + v1;
                                if (r_new < r_hat)
                                    break; // r̂ overflowed B
                                r_hat = r_new;
                            }
                        }

                        // D4. Multiply-subtract: u[j..j+n] -= q̂ × v[0..n-1]
                        std::uint64_t borrow = 0;
                        for (std::size_t i = 0; i < n; ++i)
                        {
                            std::uint64_t prod_hi = 0;
                            const std::uint64_t prod_lo = mul_64x64(q_hat, v[i], prod_hi);
                            const std::uint64_t sub = prod_lo + borrow;
                            borrow = prod_hi + (sub < prod_lo ? 1U : 0U);
                            if (u[j + i] < sub)
                                ++borrow;
                            u[j + i] -= sub;
                        }

                        // D5. Add-back if underflow (happens with prob ~2/B per step)
                        if (u[j + n] < borrow)
                        {
                            u[j + n] -= borrow; // intentional wrap
                            std::uint64_t carry = 0;
                            for (std::size_t i = 0; i < n; ++i)
                            {
                                const std::uint64_t s1 = u[j + i] + v[i];
                                const std::uint64_t s2 = s1 + carry;
                                carry = (s1 < u[j + i]) + (s2 < s1);
                                u[j + i] = s2;
                            }
                            u[j + n] += carry;
                            --q_hat;
                        }
                        else
                        {
                            u[j + n] -= borrow;
                        }

                        // D6. Store quotient digit
                        q.data[j] = q_hat;
                    }

                    // D8. Unnormalize: remainder is u[0..n-1] right-shifted by s
                    fixed_int_t r{};
                    if (s == 0)
                    {
                        for (std::size_t i = 0; i < n; ++i)
                            r.data[i] = u[i];
                    }
                    else
                    {
                        for (std::size_t i = 0; i < n - 1; ++i)
                            r.data[i] = (u[i] >> s) | (u[i + 1] << (64 - s));
                        r.data[n - 1] = u[n - 1] >> s;
                    }

                    return {q, r};
                }

                // Fallback: binary long division O(64N^2)
                // (dead code when Knuth D is compiled — kept as safety net)
                fixed_int_t q{};
                fixed_int_t r{};

                for (std::size_t i{64U * N}; i-- > 0;)
                {
                    r <<= 1;
                    r.data[0] |= (a.data[i / 64U] >> (i % 64U)) & std::uint64_t{1};
                    if (!(r < b))
                    {
                        r -= b;
                        q.data[i / 64U] |= std::uint64_t{1} << (i % 64U);
                    }
                }
                return {q, r};
            }
            else
            {
                const bool a_neg = a.is_negative();
                const bool b_neg = b.is_negative();
                using U = uint_fixed_t<N, Policy>;
                const U ua = a_neg ? U{-a} : U{a};
                const U ub = b_neg ? U{-b} : U{b};
                const auto [uq, ur] = U::divmod(ua, ub);
                const bool q_neg = (a_neg != b_neg);
                fixed_int_t q{q_neg ? -fixed_int_t{uq} : fixed_int_t{uq}};
                fixed_int_t r{a_neg ? -fixed_int_t{ur} : fixed_int_t{ur}};
                return {q, r};
            }
        }

        constexpr fixed_int_t operator/(const fixed_int_t &o) const { return divmod(*this, o).first; }

        constexpr fixed_int_t operator%(const fixed_int_t &o) const { return divmod(*this, o).second; }

        constexpr fixed_int_t &operator/=(const fixed_int_t &o)
        {
            *this = *this / o;
            return *this;
        }

        constexpr fixed_int_t &operator%=(const fixed_int_t &o)
        {
            *this = *this % o;
            return *this;
        }

        // =========================================================================
        // Compound assignments — mixed integral types
        // =========================================================================

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator+=(T v) noexcept
        {
            *this += fixed_int_t{v};
            return *this;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator-=(T v) noexcept
        {
            *this -= fixed_int_t{v};
            return *this;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator*=(T v) noexcept
        {
            *this *= fixed_int_t{v};
            return *this;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator/=(T v)
        {
            *this /= fixed_int_t{v};
            return *this;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator%=(T v)
        {
            *this %= fixed_int_t{v};
            return *this;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator&=(T v) noexcept
        {
            *this &= fixed_int_t{v};
            return *this;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator|=(T v) noexcept
        {
            *this |= fixed_int_t{v};
            return *this;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> &&
                                                          !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator^=(T v) noexcept
        {
            *this ^= fixed_int_t{v};
            return *this;
        }

#ifdef __SIZEOF_INT128__
        constexpr fixed_int_t &operator+=(unsigned __int128 v) noexcept
        {
            *this += fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator-=(unsigned __int128 v) noexcept
        {
            *this -= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator*=(unsigned __int128 v) noexcept
        {
            *this *= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator/=(unsigned __int128 v)
        {
            *this /= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator%=(unsigned __int128 v)
        {
            *this %= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator&=(unsigned __int128 v) noexcept
        {
            *this &= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator|=(unsigned __int128 v) noexcept
        {
            *this |= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator^=(unsigned __int128 v) noexcept
        {
            *this ^= fixed_int_t{v};
            return *this;
        }

        constexpr fixed_int_t &operator+=(__int128 v) noexcept
        {
            *this += fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator-=(__int128 v) noexcept
        {
            *this -= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator*=(__int128 v) noexcept
        {
            *this *= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator/=(__int128 v)
        {
            *this /= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator%=(__int128 v)
        {
            *this %= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator&=(__int128 v) noexcept
        {
            *this &= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator|=(__int128 v) noexcept
        {
            *this |= fixed_int_t{v};
            return *this;
        }
        constexpr fixed_int_t &operator^=(__int128 v) noexcept
        {
            *this ^= fixed_int_t{v};
            return *this;
        }
#endif

        // =========================================================================
        // Cross-N same-sign compound assignments — fixed_int_t<M, Sign, Form> (M != N)
        // Both operands promoted to the wider type; result truncated to N.
        // =========================================================================

        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator+=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} + fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator-=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} - fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator*=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} * fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator/=(const fixed_int_t<M, Sign, Form> &o)
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} / fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator%=(const fixed_int_t<M, Sign, Form> &o)
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} % fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator&=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} & fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator|=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} | fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator^=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} ^ fixed_int_t<R, Sign, Form>{o}};
        }

        // =========================================================================
        // Mixed-sign compound assignments
        // C++ usual arithmetic conversions:
        //   uint op= int: N >= M -> uint wins; N < M -> int wins
        //   int  op= uint: N > M -> int wins; N <= M -> uint wins
        // =========================================================================

        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator+=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} + R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} + R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator-=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} - R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} - R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator*=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} * R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} * R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator/=(const fixed_int_t<M, S2, F2> &o)
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} / R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} / R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator%=(const fixed_int_t<M, S2, F2> &o)
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} % R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} % R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator&=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} & R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} & R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator|=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} | R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} | R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator^=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<
                    (N >= M), fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type, representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} ^ R{o}};
            }
            else
            {
                using R = std::conditional_t<
                    (N > M), fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} ^ R{o}};
            }
        }

        // =========================================================================
        // Utility
        // =========================================================================

        // Number of significant bits (floor(log2(x))+1), returns 0 for zero
        /// @brief Numero de bits significativos, floor(log2(x))+1. Devuelve 0 para el cero.
        constexpr unsigned bit_width() const noexcept
        {
            for (std::size_t i{N}; i-- > 0;)
            {
                if (data[i] != 0)
                {
                    unsigned w{0};
                    std::uint64_t v = data[i];
                    while (v != 0)
                    {
                        v >>= 1;
                        ++w;
                    }
                    return static_cast<unsigned>(i * 64U) + w;
                }
            }
            return 0;
        }

        // Population count (number of set bits)
        /// @brief Numero de bits a uno.
        constexpr unsigned popcount() const noexcept
        {
            unsigned total{0};
            for (const auto &limb : data)
            {
                std::uint64_t v = limb;
                while (v != 0)
                {
                    total += static_cast<unsigned>(v & 1U);
                    v >>= 1;
                }
            }
            return total;
        }

        // Count leading zeros (MSB end); returns 64*N for zero
        /// @brief Ceros a la izquierda (por el lado del MSB). Devuelve 64*N para el cero.
        constexpr unsigned count_leading_zeros() const noexcept
        {
            return 64U * static_cast<unsigned>(N) - bit_width();
        }

        // Count trailing zeros (LSB end); returns 64*N for zero
        /// @brief Ceros a la derecha (por el lado del LSB). Devuelve 64*N para el cero.
        constexpr unsigned count_trailing_zeros() const noexcept
        {
            for (std::size_t i{0}; i < N; ++i)
            {
                if (data[i] != 0)
                {
#if __has_include("intrinsics/bit_operations.hpp")
                    return static_cast<unsigned>(i * 64U) + intrinsics::ctz64(data[i]);
#else
                    std::uint64_t v = data[i];
                    unsigned n{0};
                    while ((v & 1U) == 0)
                    {
                        v >>= 1;
                        ++n;
                    }
                    return static_cast<unsigned>(i * 64U) + n;
#endif
                }
            }
            return 64U * static_cast<unsigned>(N);
        }

        /// @brief `true` si el valor es una potencia de dos exacta. El cero no lo es.
        [[nodiscard]] constexpr bool is_power_of_two() const noexcept
        {
            return !is_zero() && (*this & (*this - one())).is_zero();
        }

        // Signed-only utilities (guarded by enable_if)
        /// @name Utilidades solo para tipos con signo
        /// @{

        /// @brief Valor absoluto.
        /// @return `|x|`. **Salvo para `min_val()`**, cuyo opuesto no es
        ///         representable: devuelve `min_val()`, que es negativo. Es el
        ///         mismo agujero que tiene `std::abs` con `INT_MIN`.
        template <bool S = is_signed, typename = std::enable_if_t<S>>
        constexpr fixed_int_t abs() const noexcept
        {
            return is_negative() ? -(*this) : *this;
        }

        /// @brief Si el valor es estrictamente mayor que cero.
        /// @return `true` si no es cero ni negativo.
        template <bool S = is_signed, typename = std::enable_if_t<S>>
        [[nodiscard]] constexpr bool is_positive() const noexcept
        {
            return !is_zero() && !is_negative();
        }

        /// @brief Signo del valor.
        /// @return `-1`, `0` o `1`.
        template <bool S = is_signed, typename = std::enable_if_t<S>>
        [[nodiscard]] constexpr int signum() const noexcept
        {
            if (is_zero())
                return 0;
            return is_negative() ? -1 : 1;
        }

        /// @}

        // =========================================================================
        // String conversion — base 10
        // =========================================================================

        // Conversion a cadena en base 2..36 (T4.4 — auditoria 23 ago 2026).
        //
        // base 10  : camino rapido existente, chunking por 10^19.
        // base 2^k : extraccion directa de bits, sin divisiones.
        // resto    : chunking por la mayor potencia de `base` que cabe en 64 bits.
        //
        // Los digitos por encima de 9 se escriben en MAYUSCULAS ('A'..'Z'), como
        // hace int128_param_t; operator<< las pasa a minusculas salvo que el flujo
        // tenga puesto std::uppercase.
        //
        // Una base fuera de [2, 36] lanza std::invalid_argument.
        /// @brief Convierte a cadena en la base pedida.
        /// @param base Base de 2 a 36. Los digitos por encima de 9 son letras
        ///        minusculas.
        /// @return La representacion, con `-` delante si el valor es negativo.
        /// @throws std::invalid_argument si la base esta fuera de [2, 36].
        [[nodiscard]] std::string to_string(int base) const
        {
            // Un valor marcado no imprime su numero a secas: el numero esta ahi
            // --ADR-009 lo conserva-- pero no significa nada, y devolverlo tal
            // cual seria dar basura con aspecto de resultado. ADR-008 lo pide asi.
            if constexpr (Policy == overflow_policy::checked)
            {
                if (!valid())
                    return "invalido";
            }
            if (base < 2 || base > 36)
                throw std::invalid_argument("fixed_int_t::to_string: base out of range [2, 36]");
            if (base == 10)
                return to_string();
            if (is_zero())
                return "0";

            if constexpr (is_signed)
            {
                if (is_negative())
                    return "-" + uint_fixed_t<N>{-(*this)}.to_string(base);
            }

            const uint_fixed_t<N, Policy> mag{*this};

            // El peor caso de longitud es la base 2: 64*N digitos.
            std::string out;
            out.reserve(64U * N + 1U);

            const bool pow2_base = (base & (base - 1)) == 0;
            if (pow2_base)
            {
                // Extraccion directa: log2(base) bits por digito, sin dividir.
                unsigned bits = 0;
                for (int t = base; t > 1; t >>= 1)
                    ++bits;
                const std::uint64_t mask = (std::uint64_t{1} << bits) - 1U;

                unsigned start = mag.bit_width();
                start -= start % bits; // primer digito parcial
                for (unsigned shift = start;; shift -= bits)
                {
                    const uint_fixed_t<N, Policy> piece = mag >> shift;
                    const std::uint64_t digit = piece.limb(0) & mask;
                    if (!out.empty() || digit != 0)
                        out.push_back(digit_char_(digit));
                    if (shift < bits)
                        break;
                }
                if (out.empty())
                    out.push_back('0');
                return out;
            }

            // Bases no potencia de dos: chunking por base^k con base^k < 2^64.
            std::uint64_t chunk_base = 1;
            unsigned digits_per_chunk = 0;
            const std::uint64_t limit = ~std::uint64_t{0} / static_cast<std::uint64_t>(base);
            while (chunk_base <= limit)
            {
                chunk_base *= static_cast<std::uint64_t>(base);
                ++digits_per_chunk;
            }

            const uint_fixed_t<N, Policy> cb{chunk_base};
            uint_fixed_t<N, Policy> tmp{mag};
            std::string rev; // digitos en orden inverso
            rev.reserve(64U * N + 1U);

            while (!tmp.is_zero())
            {
                const auto [q, r] = uint_fixed_t<N, Policy>::divmod(tmp, cb);
                std::uint64_t chunk = r.limb(0);
                const bool last = q.is_zero();
                for (unsigned d = 0; d < digits_per_chunk; ++d)
                {
                    rev.push_back(digit_char_(chunk % static_cast<std::uint64_t>(base)));
                    chunk /= static_cast<std::uint64_t>(base);
                    if (last && chunk == 0)
                        break;
                }
                tmp = q;
            }

            out.assign(rev.rbegin(), rev.rend());
            return out;
        }

        /// @brief Convierte a cadena en base 10.
        /// @return La representacion decimal, con `-` delante si es negativo.
        std::string to_string() const
        {
            if constexpr (Policy == overflow_policy::checked)
            {
                if (!valid())
                    return "invalido";
            }
            if (is_zero())
                return "0";
            if constexpr (is_signed)
            {
                if (is_negative())
                    return "-" + uint_fixed_t<N>{-(*this)}.to_string();
            }
            // unsigned path: chunk-based base-10 (divides by 10^19 per iteration)
            constexpr std::size_t max_digits = N * 20 + 1;
            char buf[max_digits];
            int pos = static_cast<int>(max_digits);

            const uint_fixed_t<N, Policy> chunk_base{std::uint64_t{10000000000000000000ULL}};
            uint_fixed_t<N, Policy> tmp{*this};

            while (!tmp.is_zero())
            {
                const auto [q, r] = uint_fixed_t<N, Policy>::divmod(tmp, chunk_base);
                const std::uint64_t chunk = r.data[0];
                if (q.is_zero())
                    write_u64_digits(buf, pos, chunk);
                else
                    write_19_padded_digits(buf, pos, chunk);
                tmp = q;
            }

            return std::string(buf + pos, buf + max_digits);
        }

        // Parseo base 10 sin excepciones.
        //
        // T2.1 (auditoria 23 ago 2026). Antes, la acumulacion
        // `result = result * 10 + digito` no comprobaba nada: parsear 2^256 en un
        // uint_fixed_t<4> devolvia 0 EN SILENCIO, con el valor truncado modulo
        // 2^(64N). Los codigos `parse_error` y el tipo `parse_result<T>` llevaban
        // declarados desde el principio de este fichero sin usarse; aqui es donde
        // se cablean.
        //
        // Gramatica aceptada (estricta, sin espacios ni separadores):
        //   con signo:  [+-]? digito+
        //   sin signo:  digito+          (el signo NO se acepta, ni '+' ni '-')
        //
        // Errores devueltos: invalid_base, null_pointer, empty_string, no_digits,
        // invalid_character, digit_out_of_range, overflow. `error_index` apunta al
        // caracter culpable (en overflow, al digito que se sale de rango).
        //
        // T4.4: `base` en [2, 36], o 0 para deducirla del prefijo. Se aceptan los
        // prefijos 0x/0X (16), 0b/0B (2) y 0o/0O (8), tanto con base 0 como cuando
        // coinciden con la base pedida. Un '0' suelto NO se interpreta como octal:
        // ese es un pie de plomo heredado de strtoul que aqui no se replica.
        /// @brief Convierte desde cadena **sin lanzar**.
        ///
        /// La version que hay que preferir cuando la cadena viene de fuera. Acepta
        /// signo, los prefijos `0x`, `0b` y `0`, y separadores.
        ///
        /// @param s Cadena a convertir.
        /// @param base Base de 2 a 36; 10 por defecto.
        /// @return Un `parse_result`: si `success()`, el valor esta en `.value`;
        ///         si no, `.error` dice que paso y `.error_index` donde.
        ///         **Detecta el desbordamiento** y devuelve
        ///         `parse_error::overflow`: antes de v1.90.1 un numero demasiado
        ///         grande se truncaba en silencio.
        [[nodiscard]] static parse_result<fixed_int_t> try_from_string(const char *s, int base = 10) noexcept
        {
            using U = uint_fixed_t<N, Policy>;

            if (base != 0 && (base < 2 || base > 36))
                return {parse_error::invalid_base, fixed_int_t{}, 0};

            if (!s)
                return {parse_error::null_pointer, fixed_int_t{}, std::string::npos};
            if (*s == '\0')
                return {parse_error::empty_string, fixed_int_t{}, 0};

            const char *p = s;
            bool negative = false;

            if constexpr (is_signed)
            {
                if (*p == '-' || *p == '+')
                {
                    negative = (*p == '-');
                    ++p;
                    if (*p == '\0')
                        return {parse_error::no_digits, fixed_int_t{}, static_cast<std::size_t>(p - s)};
                }
            }

            // Prefijo de base, si lo hay.
            if (p[0] == '0' && p[1] != '\0')
            {
                const char k = p[1];
                int prefix_base = 0;
                if (k == 'x' || k == 'X')
                    prefix_base = 16;
                else if (k == 'b' || k == 'B')
                    prefix_base = 2;
                else if (k == 'o' || k == 'O')
                    prefix_base = 8;

                if (prefix_base != 0 && (base == 0 || base == prefix_base))
                {
                    base = prefix_base;
                    p += 2;
                    if (*p == '\0')
                        return {parse_error::no_digits, fixed_int_t{}, static_cast<std::size_t>(p - s)};
                }
            }
            if (base == 0)
                base = 10;

            // Cotas exactas para detectar el desbordamiento digito a digito:
            // acc cabe tras `acc*base + d` si y solo si
            //   acc < max/base, o bien acc == max/base y d <= max%base.
            const U ubase{static_cast<std::uint64_t>(base)};
            const U u_max = U::max();
            const auto [u_max_div, u_max_mod] = U::divmod(u_max, ubase);
            const std::uint64_t max_last_digit = u_max_mod.limb(0);

            U mag{};
            bool any = false;

            for (; *p != '\0'; ++p)
            {
                const unsigned dv = digit_value_(*p);
                if (dv >= static_cast<unsigned>(base))
                    return {dv == 255U ? parse_error::invalid_character : parse_error::digit_out_of_range,
                            fixed_int_t{}, static_cast<std::size_t>(p - s)};

                const std::uint64_t digit = dv;

                if (mag > u_max_div || (mag == u_max_div && digit > max_last_digit))
                    return {parse_error::overflow, fixed_int_t{}, static_cast<std::size_t>(p - s)};

                mag = mag * ubase + U{digit};
                any = true;
            }

            if (!any)
                return {parse_error::no_digits, fixed_int_t{}, 0};

            if constexpr (is_signed)
            {
                // Rango representable: [-2^(64N-1), 2^(64N-1)-1].
                const U limit_pos = u_max >> 1;                                        // 2^(64N-1) - 1
                const U limit_neg = U::one() << (64U * static_cast<unsigned>(N) - 1U); // 2^(64N-1)
                if (negative ? (mag > limit_neg) : (mag > limit_pos))
                    return {parse_error::overflow, fixed_int_t{}, static_cast<std::size_t>(p - s - 1)};

                const fixed_int_t value{mag};
                return {parse_error::success, negative ? -value : value, std::string::npos};
            }
            else
            {
                return {parse_error::success, fixed_int_t{mag}, std::string::npos};
            }
        }

        // Version que lanza. Mensajes de error compatibles con las versiones
        // anteriores; el desbordamiento es std::out_of_range (como std::stoull),
        // no std::invalid_argument.
        /// @brief Convierte desde cadena, **lanzando** si falla.
        ///
        /// Envoltorio de `try_from_string()` que traduce cada codigo de error a
        /// su excepcion. Preferir `try_from_string()` si la cadena no es de fiar.
        ///
        /// @param s Cadena a convertir.
        /// @param base Base de 2 a 36; 10 por defecto.
        /// @return El valor convertido.
        /// @throws std::invalid_argument si la cadena o la base no son validas.
        /// @throws std::out_of_range si el valor no cabe en `64 * N` bits. Es
        ///         `out_of_range` y no `invalid_argument`, por coherencia con
        ///         `std::stoull`.
        static fixed_int_t from_string(const char *s, int base = 10)
        {
            const parse_result<fixed_int_t> r = try_from_string(s, base);
            switch (r.error)
            {
                case parse_error::success:
                    return r.value;
                case parse_error::null_pointer:
                case parse_error::empty_string:
                    throw std::invalid_argument("fixed_int_t::from_string: empty string");
                case parse_error::no_digits:
                    throw std::invalid_argument("fixed_int_t::from_string: no digits");
                case parse_error::invalid_character:
                    throw std::invalid_argument("fixed_int_t::from_string: invalid character");
                case parse_error::digit_out_of_range:
                    throw std::invalid_argument("fixed_int_t::from_string: digit out of range for base");
                case parse_error::invalid_base:
                    throw std::invalid_argument("fixed_int_t::from_string: base out of range [2, 36]");
                case parse_error::overflow:
                    throw std::out_of_range("fixed_int_t::from_string: value out of range");
                default:
                    throw std::invalid_argument("fixed_int_t::from_string: parse error");
            }
        }

    private:
        // =========================================================================
        // Primitivas 64x64 -> 128 y 128/64, constexpr en todas las plataformas
        //
        // T3.1 (auditoria 23 ago 2026). Antes, cada uno de estos calculos estaba
        // escrito en linea dentro de divmod con una cadena #if/#elif/#else por
        // plataforma; el resultado era que en MSVC e ICX-Windows la unica version
        // compilada era la del intrinseco, que no es evaluable en tiempo de
        // compilacion. Eso impedia marcar divmod (y por tanto / y %) como
        // constexpr.
        //
        // Al centralizarlas aqui:
        //   - el camino de ejecucion no cambia: en GCC/Clang siguen siendo
        //     operaciones sobre unsigned __int128, y en MSVC/ICX los mismos
        //     intrinsecos, bajo `if (!std::is_constant_evaluated())`;
        //   - el camino constexpr existe siempre (version portable);
        //   - desaparecen 4 copias del multiply 32x32 portable y 2 del bucle de
        //     division bit a bit que habia repartidas por divmod.
        // =========================================================================

        // Producto completo x*y: devuelve los 64 bits bajos y deja los altos en hi.
        [[nodiscard]] static constexpr std::uint64_t mul_64x64(std::uint64_t x, std::uint64_t y,
                                                               std::uint64_t &hi) noexcept
        {
#if defined(__SIZEOF_INT128__)
            // Constexpr-friendly en GCC/Clang/ICX-Linux: sin rama.
            const unsigned __int128 p = static_cast<unsigned __int128>(x) * y;
            hi = static_cast<std::uint64_t>(p >> 64);
            return static_cast<std::uint64_t>(p);
#else
            if (!std::is_constant_evaluated())
            {
#if defined(_MSC_VER) && defined(_M_X64)
                return _umul128(x, y, &hi);
#endif
            }
            // Portable: escuela 32x32 -> 64.
            const std::uint64_t xl = x & 0xFFFFFFFFU;
            const std::uint64_t xh = x >> 32;
            const std::uint64_t yl = y & 0xFFFFFFFFU;
            const std::uint64_t yh = y >> 32;

            const std::uint64_t p0 = xl * yl;
            const std::uint64_t p1 = xl * yh;
            const std::uint64_t p2 = xh * yl;
            const std::uint64_t p3 = xh * yh;

            const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFFFFFFU) + (p2 & 0xFFFFFFFFU);
            hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
            return (p0 & 0xFFFFFFFFU) | (mid << 32);
#endif
        }

        // Division (hi:lo) / d con hi < d (precondicion del llamante).
        // Devuelve el cociente de 64 bits y deja el resto en rem.
        [[nodiscard]] static constexpr std::uint64_t div_128_64(std::uint64_t hi, std::uint64_t lo,
                                                                std::uint64_t d, std::uint64_t &rem) noexcept
        {
#if defined(__SIZEOF_INT128__) && !(defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)))
            // GCC/Clang/ICX-Linux: __udivti3 detecta hi < d y emite un solo divq.
            // Constexpr-friendly, sin rama.
            const unsigned __int128 u = (static_cast<unsigned __int128>(hi) << 64) | lo;
            rem = static_cast<std::uint64_t>(u % d);
            return static_cast<std::uint64_t>(u / d);
#else
            if (!std::is_constant_evaluated())
            {
#if defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)) && defined(_M_X64)
                // ICX en Windows define __SIZEOF_INT128__ pero su runtime no trae
                // __udivti3/__umodti3: enlazaria mal. Usa asm estilo GCC (frontend
                // Clang/LLVM).
                std::uint64_t q, r;
                __asm__("divq %4" : "=a"(q), "=d"(r) : "0"(lo), "1"(hi), "rm"(d));
                rem = r;
                return q;
#elif defined(_MSC_VER) && defined(_M_X64)
                return _udiv128(hi, lo, d, &rem);
#endif
            }

            // Portable. Caso rapido hi == 0 y, si no, division larga bit a bit.
            if (hi == 0)
            {
                rem = lo % d;
                return lo / d;
            }
            std::uint64_t r = hi;
            std::uint64_t q = 0;
            for (int bit = 63; bit >= 0; --bit)
            {
                const bool ovf = (r >> 63) != 0;
                r = (r << 1) | ((lo >> bit) & 1U);
                if (ovf || r >= d)
                {
                    r -= d;
                    q |= std::uint64_t{1} << bit;
                }
            }
            rem = r;
            return q;
#endif
        }

        // Add v to limb, return carry (0 or 1)
        // =====================================================================
        // Escolar desenrollado POR CONSTRUCCION
        // =====================================================================
        //
        // Mismo algoritmo y mismas primitivas que el bucle de mas abajo. Lo
        // unico que cambia es que los indices son de compilacion, asi que no
        // depende de que el compilador se anime a desenrollar.
        //
        // POR QUE. Medido el 6 sep 2026 sobre el ensamblador de GCC 16.2 en N=4,
        // el bucle salia con 61 instrucciones y UN `mul` --o sea, rodando-- y
        // Karatsuba con 119 y NUEVE. Escrito asi, el escolar pasa de 79,8 a
        // 26,7 cyc/op en GCC: 3x. Y no es un caso aislado, va de 1,5x a 3,8x en
        // los cuatro compiladores. Es la mejora mas grande que ha aparecido
        // midiendo, y afecta a TODAS las anchuras, no solo a las de Karatsuba.
        //
        // El tope es NSTD_DESENROLLA_MAX: desenrollar cuesta codigo --N(N+1)/2
        // productos en linea recta-- y por encima de cierta anchura deja de
        // compensar por presion de registros y cache de instrucciones. Por
        // encima del tope se sigue usando el bucle.

        /// @brief Propaga el acarreo desde el limbo K. Conserva la salida
        ///        temprana del bucle original, que aqui es un `if`.
        template <std::size_t K>
        static constexpr void propaga_desde(std::array<std::uint64_t, N> &r, unsigned char c) noexcept
        {
            if constexpr (K < N)
            {
                if (c)
                    propaga_desde<K + 1>(r, add_limb(r[K], std::uint64_t{c}));
            }
        }

        /// @brief Una fila del escolar: los productos x[I]*y[J] con J creciente.
        template <std::size_t I, std::size_t J>
        static constexpr void fila_desenrollada(std::array<std::uint64_t, N> &r,
                                                const std::array<std::uint64_t, N> &x,
                                                const std::array<std::uint64_t, N> &y) noexcept
        {
            if constexpr (I + J < N)
            {
                std::uint64_t hi{0};
                const std::uint64_t lo = producto64(x[I], y[J], hi);
                unsigned char c = add_limb(r[I + J], lo);
                if constexpr (I + J + 1 < N)
                {
                    c = add_limb_carry(r[I + J + 1], hi, c);
                    propaga_desde<I + J + 2>(r, c);
                }
                fila_desenrollada<I, J + 1>(r, x, y);
            }
        }

        /// @brief Todas las filas.
        template <std::size_t I>
        static constexpr void filas_desenrolladas(std::array<std::uint64_t, N> &r,
                                                  const std::array<std::uint64_t, N> &x,
                                                  const std::array<std::uint64_t, N> &y) noexcept
        {
            if constexpr (I < N)
            {
                fila_desenrollada<I, 0>(r, x, y);
                filas_desenrolladas<I + 1>(r, x, y);
            }
        }

        /// @brief Producto 64x64 -> 128. La parte baja se devuelve, la alta va a
        ///        `hi`. Estaba copiado dentro del bucle escolar; ahora lo usan
        ///        el bucle y la version desenrollada, para que no puedan
        ///        separarse por descuido.
        [[nodiscard]] static constexpr std::uint64_t producto64(std::uint64_t a, std::uint64_t b,
                                                                std::uint64_t &hi) noexcept
        {
#if __has_include("intrinsics/arithmetic_operations.hpp")
            return intrinsics::umul128(a, b, &hi);
#else
            const std::uint64_t a_lo = a & 0xFFFFFFFFULL;
            const std::uint64_t a_hi = a >> 32;
            const std::uint64_t b_lo = b & 0xFFFFFFFFULL;
            const std::uint64_t b_hi = b >> 32;
            const std::uint64_t p0 = a_lo * b_lo;
            const std::uint64_t p1 = a_lo * b_hi;
            const std::uint64_t p2 = a_hi * b_lo;
            const std::uint64_t p3 = a_hi * b_hi;
            const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL);
            hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
            return (p0 & 0xFFFFFFFFULL) | (mid << 32);
#endif
        }

        static constexpr unsigned char add_limb(std::uint64_t &limb, std::uint64_t v) noexcept
        {
#if __has_include("intrinsics/arithmetic_operations.hpp")
            return intrinsics::addcarry_u64(0, limb, v, &limb);
#else
            const std::uint64_t old{limb};
            limb += v;
            return static_cast<unsigned char>(limb < old ? 1 : 0);
#endif
        }

        // Add v + carry_in to limb, return carry_out (0 or 1)
        static constexpr unsigned char add_limb_carry(std::uint64_t &limb, std::uint64_t v,
                                                      unsigned char c) noexcept
        {
#if __has_include("intrinsics/arithmetic_operations.hpp")
            return intrinsics::addcarry_u64(c, limb, v, &limb);
#else
            const std::uint64_t old{limb};
            limb += v + c;
            return static_cast<unsigned char>((limb < old || (c && limb == old)) ? 1 : 0);
#endif
        }

        // =========================================================================
        // Karatsuba full multiply: M×M → 2M limb product (unsigned, limb arrays).
        // Recurrence T(1)=1, T(M)=3·T(M/2)+O(M):  T(2)=3, T(4)=9 umul128 calls.
        // =========================================================================

        template <std::size_t M>
        [[nodiscard]] static std::array<std::uint64_t, 2 * M>
        kmul_full(const std::array<std::uint64_t, M> &a, const std::array<std::uint64_t, M> &b) noexcept
        {
            std::array<std::uint64_t, 2 * M> r{};

            if constexpr (M == 1)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                r[0] = intrinsics::umul128(a[0], b[0], &r[1]);
#else
                const std::uint64_t al = a[0] & 0xFFFF'FFFFull;
                const std::uint64_t ah = a[0] >> 32;
                const std::uint64_t bl = b[0] & 0xFFFF'FFFFull;
                const std::uint64_t bh = b[0] >> 32;
                const std::uint64_t p0 = al * bl, p1 = al * bh;
                const std::uint64_t p2 = ah * bl, p3 = ah * bh;
                const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFF'FFFFull) + (p2 & 0xFFFF'FFFFull);
                r[0] = (p0 & 0xFFFF'FFFFull) | (mid << 32);
                r[1] = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
#endif
                return r;
            }
            else
            {
                static_assert(M % 2 == 0, "kmul_full: M must be even");
                constexpr std::size_t HH = M / 2;

                // ── Split into low/high halves ────────────────────────────────────
                std::array<std::uint64_t, HH> a_lo{}, a_hi{}, b_lo{}, b_hi{};
                for (std::size_t i = 0; i < HH; ++i)
                {
                    a_lo[i] = a[i];
                    a_hi[i] = a[HH + i];
                    b_lo[i] = b[i];
                    b_hi[i] = b[HH + i];
                }

                // ── z0 = a_lo·b_lo,  z2 = a_hi·b_hi  (each 2HH limbs) ───────────
                const auto z0 = kmul_full<HH>(a_lo, b_lo);
                const auto z2 = kmul_full<HH>(a_hi, b_hi);

                // ── sum_a = a_lo + a_hi,  sum_b = b_lo + b_hi  (+carry ca, cb) ───
                std::array<std::uint64_t, HH> sum_a{}, sum_b{};
                unsigned char ca = 0, cb = 0;
                for (std::size_t i = 0; i < HH; ++i)
                {
                    sum_a[i] = a_hi[i];
                    ca = add_limb_carry(sum_a[i], a_lo[i], ca);
                    sum_b[i] = b_hi[i];
                    cb = add_limb_carry(sum_b[i], b_lo[i], cb);
                }

                // ── p = (sum_a + ca·B^HH) · (sum_b + cb·B^HH)  (2HH+1 limbs) ───
                // = sum_a·sum_b + ca·sum_b·B^HH + cb·sum_a·B^HH + ca·cb·B^(2HH)
                std::array<std::uint64_t, 2 * HH + 1> p{};
                {
                    const auto pp = kmul_full<HH>(sum_a, sum_b);
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                        p[i] = pp[i];
                }
                if (ca)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = add_limb_carry(p[HH + i], sum_b[i], c);
                    p[2 * HH] += c;
                }
                if (cb)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = add_limb_carry(p[HH + i], sum_a[i], c);
                    p[2 * HH] += c;
                }
                if (ca & cb)
                    ++p[2 * HH];

                // ── z1 = p − z0 − z2  (guaranteed ≥ 0, fits in 2HH+1 limbs) ─────
                std::array<std::uint64_t, 2 * HH + 1> z1 = p;
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z0[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z0[i] - borrow;
                        borrow = static_cast<unsigned char>((av < z0[i]) || (borrow && av == z0[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z2[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z2[i] - borrow;
                        borrow = static_cast<unsigned char>((av < z2[i]) || (borrow && av == z2[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }

                // ── Combine: r = z0 + z1·B^HH + z2·B^(2HH) ─────────────────────
                for (std::size_t i = 0; i < 2 * HH; ++i)
                    r[i] = z0[i];
                {
                    unsigned char c = 0;
                    std::size_t i = 0;
                    for (; i <= 2 * HH; ++i)
                        c = add_limb_carry(r[HH + i], z1[i], c);
                    for (; c && HH + i < 2 * M; ++i)
                        c = add_limb(r[HH + i], std::uint64_t{1});
                }
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                        c = add_limb_carry(r[2 * HH + i], z2[i], c);
                }

                return r;
            }
        }

        // Two-digit lookup: "00", "01", ..., "99"
        static constexpr char DIGIT_PAIRS_[201] = "00010203040506070809"
                                                  "10111213141516171819"
                                                  "20212223242526272829"
                                                  "30313233343536373839"
                                                  "40414243444546474849"
                                                  "50515253545556575859"
                                                  "60616263646566676869"
                                                  "70717273747576777879"
                                                  "80818283848586878889"
                                                  "90919293949596979899";

        // Write val as decimal digits into buf[..pos-1] (no zero-padding)
        static inline void write_u64_digits(char *buf, int &pos, std::uint64_t val) noexcept
        {
            while (val >= 100)
            {
                const std::uint64_t q = val / 100;
                const std::uint64_t r = val % 100;
                buf[--pos] = DIGIT_PAIRS_[r * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[r * 2];
                val = q;
            }
            if (val >= 10)
            {
                buf[--pos] = DIGIT_PAIRS_[val * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[val * 2];
            }
            else
            {
                buf[--pos] = static_cast<char>('0' + val);
            }
        }

        // Digito -> caracter, mayusculas por encima de 9 (T4.4).
        [[nodiscard]] static constexpr char digit_char_(std::uint64_t d) noexcept
        {
            return d < 10 ? static_cast<char>('0' + d) : static_cast<char>('A' + (d - 10));
        }

        // Caracter -> digito, o 255 si no es un digito valido. Acepta ambas cajas.
        [[nodiscard]] static constexpr unsigned digit_value_(char c) noexcept
        {
            if (c >= '0' && c <= '9')
                return static_cast<unsigned>(c - '0');
            if (c >= 'a' && c <= 'z')
                return static_cast<unsigned>(c - 'a') + 10U;
            if (c >= 'A' && c <= 'Z')
                return static_cast<unsigned>(c - 'A') + 10U;
            return 255U;
        }

        // Write exactly 19 decimal digits from val into buf[..pos-1] (zero-padded)
        // Precondition: val < 10^19
        static inline void write_19_padded_digits(char *buf, int &pos, std::uint64_t val) noexcept
        {
            for (int i{0}; i < 9; ++i)
            {
                const std::uint64_t q = val / 100;
                const std::uint64_t r = val % 100;
                buf[--pos] = DIGIT_PAIRS_[r * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[r * 2];
                val = q;
            }
            buf[--pos] = static_cast<char>('0' + val);
        }
    };

    // =============================================================================
    // El contrato de tamano y disposicion  (ADR-009)
    // =============================================================================
    //
    // POR QUE ESTAN AQUI Y NO DENTRO DE LA CLASE. Dentro no se puede: en el
    // cuerpo de la propia plantilla el tipo esta INCOMPLETO, y `sizeof` de un
    // tipo incompleto es un error. Los `static_assert` que si dependen solo de
    // los parametros --la ley de ADR-011, las formas y politicas implementadas--
    // si estan dentro, que es donde valen mas.
    //
    // Estos se comprueban al leer la cabecera, sin ejecutar nada y sin que nadie
    // tenga que acordarse de lanzar un test.
    //
    // QUE PROTEGEN. Que `wrap` no pague NADA por la existencia de la politica.
    // No es una formalidad: con el `[[no_unique_address]]` estandar a secas,
    // MSVC e Intel dan 40 bytes donde deben dar 32 --medido el 5 sep 2026 en los
    // cuatro compiladores-- y se cargan la invariante en silencio. Aqui deja de
    // ser silencio.

    namespace detail
    {
        /// @brief Comprueba de una vez todo lo que ADR-009 promete de `wrap`:
        ///        que ocupe `8N` bytes exactos, que este alineado como un limbo,
        ///        y que conserve el standard layout y la copia trivial.
        ///
        /// Se usa desde los `static_assert` de mas abajo. Es una funcion y no
        /// una expresion suelta para poder repetirla sobre varios N sin escribir
        /// la lista de condiciones cinco veces.
        ///
        /// @tparam N Numero de limbos a comprobar.
        /// @return `true` si el contrato se cumple.
        template <std::size_t N>
        inline constexpr bool contrato_de_wrap() noexcept
        {
            using u =
                fixed_int_t<N, signedness::unsigned_type, representation_form::binnat, overflow_policy::wrap>;
            using i = fixed_int_t<N, signedness::signed_type, representation_form::twos_complement,
                                  overflow_policy::wrap>;
            return sizeof(u) == 8 * N && sizeof(i) == 8 * N && alignof(u) == alignof(std::uint64_t) &&
                   std::is_standard_layout_v<u> && std::is_standard_layout_v<i> &&
                   std::is_trivially_copyable_v<u> && std::is_trivially_copyable_v<i>;
        }
    } // namespace detail

    static_assert(detail::contrato_de_wrap<1>(), "wrap debe ocupar 8*N bytes y ser standard layout");
    static_assert(detail::contrato_de_wrap<2>(), "wrap debe ocupar 8*N bytes y ser standard layout");
    static_assert(detail::contrato_de_wrap<4>(), "wrap debe ocupar 8*N bytes y ser standard layout");
    static_assert(detail::contrato_de_wrap<8>(), "wrap debe ocupar 8*N bytes y ser standard layout");
    static_assert(detail::contrato_de_wrap<16>(), "wrap debe ocupar 8*N bytes y ser standard layout");

    // Y `checked` paga lo que ADR-009 dijo que pagaria: un limbo mas. Ni mas ni
    // menos. Que siga siendo standard layout es lo que hace que el miembro
    // condicional fuera mejor que una clase base, que lo perdia en los cuatro
    // compiladores.
    static_assert(sizeof(fixed_int_t<4, signedness::unsigned_type, representation_form::binnat,
                                     overflow_policy::checked>) == 40,
                  "checked debe ocupar exactamente un limbo mas que wrap");
    static_assert(
        std::is_standard_layout_v<
            fixed_int_t<4, signedness::unsigned_type, representation_form::binnat, overflow_policy::checked>>,
        "checked tiene que conservar el standard layout");
    static_assert(
        std::is_trivially_copyable_v<
            fixed_int_t<4, signedness::unsigned_type, representation_form::binnat, overflow_policy::checked>>,
        "checked tiene que seguir siendo trivialmente copiable");

    // =============================================================================
    // Cruzar de una politica a otra: solo con nombre
    // =============================================================================
    //
    // NO HAY NINGUNA CONVERSION ENTRE POLITICAS QUE NO SE LEA EN EL PUNTO DE
    // LLAMADA. Ni implicita, ni por `static_cast`, ni por constructor: las dos
    // funciones de aqui abajo son el unico camino.
    //
    // ADR-008 prohibe MEZCLAR politicas en una operacion --`a + b` con politicas
    // distintas es error de compilacion-- pero no decia nada de convertir. Un
    // `static_cast` habria bastado tecnicamente, y se descarto por un motivo
    // concreto: dice COMO convertir pero no QUE PASA CON LA MARCA. Con un nombre,
    // el contrato va en el nombre y se ve al leer la linea.

    /// @brief De `checked` a `wrap`, **tirando la marca**.
    ///
    /// El valor numerico se conserva tal cual --es el resultado envuelto, que
    /// siempre esta ahi--; lo que se pierde es el saber si era de fiar.
    ///
    /// @warning Si `x` estaba marcado como invalido, **esa informacion
    ///          desaparece aqui y no hay forma de recuperarla**. Por eso la
    ///          funcion se llama como se llama: para que la linea diga lo que
    ///          hace. Lo normal es consultar `valid()` ANTES.
    ///
    /// @param x Valor con politica `checked`.
    /// @return El mismo numero, con politica `wrap`.
    template <std::size_t N, signedness Sign, representation_form Form>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, overflow_policy::wrap>
    descartar_marca(const fixed_int_t<N, Sign, Form, overflow_policy::checked> &x) noexcept
    {
        fixed_int_t<N, Sign, Form, overflow_policy::wrap> r{};
        for (std::size_t i{0}; i < N; ++i)
            r.set_limb(i, x.limb(i));
        return r;
    }

    namespace detail
    {
        /// @brief Devuelve `v` con la politica `Policy`, **arrastrando la marca
        ///        de `a` y de `b`**.
        ///
        /// Es la pieza que hace pegajosa la saturacion (P1.5 tramo 2f). El valor
        /// numerico es `v` y no se toca; lo unico que se hereda es el saber si
        /// alguno de los operandos ya era invalido.
        ///
        /// @note El `(a - a) + (b - b)` no es un truco gratuito: es un CERO que
        ///       lleva la marca de los dos operandos. ADR-008 dice que la marca
        ///       se hereda con un OR en cada operacion, asi que sumarlo conserva
        ///       el valor y une las dos marcas. Restar un valor de si mismo no
        ///       puede desbordar --ni siquiera en el minimo con signo-- asi que
        ///       no introduce una marca que no estuviera ya.
        ///
        /// @note Con `wrap` no hay nada que heredar y se devuelve `v` tal cual.
        ///
        /// @param v Valor numerico del resultado, ya calculado y sin marca.
        /// @param a Primer operando de la operacion original.
        /// @param b Segundo operando.
        /// @return `v` con la politica de los operandos y su marca heredada.
        template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
        [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
        con_marca_heredada(const fixed_int_t<N, Sign, Form, overflow_policy::wrap> &v,
                           const fixed_int_t<N, Sign, Form, Policy> &a,
                           const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
        {
            if constexpr (Policy == overflow_policy::wrap)
            {
                (void)a;
                (void)b;
                return v;
            }
            else
            {
                using T = fixed_int_t<N, Sign, Form, Policy>;
                return T{v} + ((a - a) + (b - b));
            }
        }
    } // namespace detail

    /// @brief De `wrap` a `checked`, marcando el resultado como valido.
    ///
    /// Es la direccion segura: un valor de `wrap` no lleva marca que pueda
    /// contradecir nada, asi que el resultado nace valido.
    ///
    /// @note Que nazca valido **no dice que no haya desbordado antes**. Si `x`
    ///       viene de una cadena con `wrap`, cualquier desbordamiento de esa
    ///       cadena ya se perdio sin dejar rastro, que es lo que `wrap`
    ///       significa. Esta funcion empieza a comprobar desde aqui, no
    ///       reconstruye el pasado.
    ///
    /// @param x Valor con politica `wrap`.
    /// @return El mismo numero, con politica `checked` y marcado como valido.
    template <std::size_t N, signedness Sign, representation_form Form>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, overflow_policy::checked>
    con_comprobacion(const fixed_int_t<N, Sign, Form, overflow_policy::wrap> &x) noexcept
    {
        fixed_int_t<N, Sign, Form, overflow_policy::checked> r{};
        for (std::size_t i{0}; i < N; ++i)
            r.set_limb(i, x.limb(i));
        return r;
    }

    // =============================================================================
    // Type aliases — unsigned
    // =============================================================================

    /// @name Alias sin signo por anchura en bits
    /// Los nombres que se usan en la practica. Cada uno fija el numero de
    /// limbos: la anchura en bits es `64 * N`.
    /// @{
    using uint64_fixed_t = uint_fixed_t<1>;    ///< 64 bits sin signo (1 limbo).
    using uint128_fixed_t = uint_fixed_t<2>;   ///< 128 bits sin signo (2 limbos).
    using uint256_fixed_t = uint_fixed_t<4>;   ///< 256 bits sin signo (4 limbos).
    using uint512_fixed_t = uint_fixed_t<8>;   ///< 512 bits sin signo (8 limbos).
    using uint1024_fixed_t = uint_fixed_t<16>; ///< 1024 bits sin signo (16 limbos).
    /// @}

    // =============================================================================
    // Type aliases — signed
    // =============================================================================

    /// @name Alias con signo por anchura en bits
    /// Complemento a dos, como los enteros con signo del lenguaje.
    /// @{
    using int64_fixed_t = int_fixed_t<1>;    ///< 64 bits con signo (1 limbo).
    using int128_fixed_t = int_fixed_t<2>;   ///< 128 bits con signo (2 limbos).
    using int256_fixed_t = int_fixed_t<4>;   ///< 256 bits con signo (4 limbos).
    using int512_fixed_t = int_fixed_t<8>;   ///< 512 bits con signo (8 limbos).
    using int1024_fixed_t = int_fixed_t<16>; ///< 1024 bits con signo (16 limbos).
    /// @}

    /// @name Sucesor y predecesor
    /// `succ(x)` y `pred(x)` son `x + 1` y `x - 1` sin modificar `x`, para
    /// usarlos en contextos donde `++`/`--` no encajan (algoritmos, `constexpr`
    /// sobre un valor constante). Como toda la aritmetica de la biblioteca son
    /// **modulares**: `succ(max())` da `min()` y `pred(min())` da `max()`.
    /// @{

    /// @brief Sucesor sin signo: `x + 1` modulo 2^(64N).
    /// @param x Valor de partida, que no se modifica.
    /// @return `x + 1`; `0` si `x` era `max()`.
    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<N> succ(const uint_fixed_t<N> &x) noexcept
    {
        return x + uint_fixed_t<N>::one();
    }

    /// @brief Predecesor sin signo: `x - 1` modulo 2^(64N).
    /// @param x Valor de partida, que no se modifica.
    /// @return `x - 1`; `max()` si `x` era cero.
    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<N> pred(const uint_fixed_t<N> &x) noexcept
    {
        return x - uint_fixed_t<N>::one();
    }

    /// @brief Sucesor con signo: `x + 1` modulo 2^(64N).
    /// @param x Valor de partida, que no se modifica.
    /// @return `x + 1`; `min()` si `x` era `max()`.
    template <std::size_t N>
    [[nodiscard]] constexpr int_fixed_t<N> succ(const int_fixed_t<N> &x) noexcept
    {
        return x + int_fixed_t<N>::one();
    }

    /// @brief Predecesor con signo: `x - 1` modulo 2^(64N).
    /// @param x Valor de partida, que no se modifica.
    /// @return `x - 1`; `max()` si `x` era `min()`.
    template <std::size_t N>
    [[nodiscard]] constexpr int_fixed_t<N> pred(const int_fixed_t<N> &x) noexcept
    {
        return x - int_fixed_t<N>::one();
    }
    /// @}

    // =========================================================================
    // Public trait: mixed_iu_t (C++ usual arithmetic conversions for fixed_int_t)
    // Promoted from nstd::detail::mixed_iu_t to nstd::mixed_iu_t — Fase MS-INTEROP.
    // =========================================================================

    /// @brief Result type of `int_fixed_t<N> op uint_fixed_t<M>` per C++ UAC.
    ///
    /// Rule (mirrors built-in `signed op unsigned`):
    ///   - N > M  -> int_fixed_t<N>  (signed wider rank wins, unsigned zero-extends)
    ///   - N <= M -> uint_fixed_t<M> (unsigned rank >= signed → signed converts to unsigned)
    ///
    /// Both orientations (int op uint, uint op int) yield the same type. The order
    /// of template parameters here is conventionally (N = signed side, M = unsigned side).
    /// @tparam Policy Politica de desbordamiento, la MISMA en los dos lados:
    ///         mezclarlas esta prohibido por ADR-008. Por defecto `wrap`, para
    ///         que el codigo escrito antes del cuarto parametro siga valiendo.
    template <std::size_t N, std::size_t M, overflow_policy Policy = overflow_policy::wrap>
    using mixed_iu_t = std::conditional_t<(N > M), int_fixed_t<N, Policy>, uint_fixed_t<M, Policy>>;

    // =========================================================================
    // Detection traits for fixed_int_t — Fase MS-INTEROP
    // =========================================================================

    namespace detail
    {
        template <typename T>
        struct is_fixed_int_impl : std::false_type
        {
        };

        template <std::size_t N, signedness S, representation_form F, overflow_policy P>
        struct is_fixed_int_impl<fixed_int_t<N, S, F, P>> : std::true_type
        {
        };
    } // namespace detail

    /// @brief Trait class: true iff T is some `fixed_int_t<N, Sign, Form>`.
    template <typename T>
    struct is_fixed_int : detail::is_fixed_int_impl<std::remove_cv_t<T>>
    {
    };

    /// @brief Atajo de `is_fixed_int<T>::value`.
    /// @tparam T Tipo a comprobar.
    template <typename T>
    inline constexpr bool is_fixed_int_v = is_fixed_int<T>::value;

    /// @brief Trait class: true iff T is a signed `fixed_int_t<N>` instance.
    template <typename T>
    struct is_signed_fixed_int : std::false_type
    {
    };

    template <std::size_t N, representation_form F, overflow_policy P>
    struct is_signed_fixed_int<fixed_int_t<N, signedness::signed_type, F, P>> : std::true_type
    {
    };

    /// @brief Atajo de `is_signed_fixed_int<T>::value`.
    /// @tparam T Tipo a comprobar.
    template <typename T>
    inline constexpr bool is_signed_fixed_int_v = is_signed_fixed_int<std::remove_cv_t<T>>::value;

    /// @brief Trait class: true iff T is an unsigned `fixed_int_t<N>` instance.
    template <typename T>
    struct is_unsigned_fixed_int : std::false_type
    {
    };

    template <std::size_t N, representation_form F, overflow_policy P>
    struct is_unsigned_fixed_int<fixed_int_t<N, signedness::unsigned_type, F, P>> : std::true_type
    {
    };

    /// @brief Atajo de `is_unsigned_fixed_int<T>::value`.
    /// @tparam T Tipo a comprobar.
    template <typename T>
    inline constexpr bool is_unsigned_fixed_int_v = is_unsigned_fixed_int<std::remove_cv_t<T>>::value;

    // =========================================================================
    // detail — SFINAE helpers
    // =========================================================================

    namespace detail
    {
        template <typename T>
        /// @brief SFINAE: solo participa si `T` es un entero del lenguaje
        ///        distinto de `bool`. Es lo que hace que las sobrecargas mixtas
        ///        no se traguen cualquier tipo.
        using if_integral =
            std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>;

        // Compatibility alias: kept so internal code referencing detail::mixed_iu_t
        // continues to work after the trait was promoted to nstd::mixed_iu_t.
        template <std::size_t N, std::size_t M>
        /// @brief Alias interno de `nstd::mixed_iu_t`, conservado por
        ///        compatibilidad con el codigo anterior a la Fase MS-INTEROP.
        using mixed_iu_t = nstd::mixed_iu_t<N, M>;
    } // namespace detail

    // =========================================================================
    // Free-function binary operators — uint_fixed_t<N> mixed with integral T
    // =========================================================================

    /// @name Operadores mixtos: `uint_fixed_t<N>` con un entero del lenguaje
    ///
    /// Existen porque los constructores son `explicit` (ADR-001): sin conversion
    /// implicita, `a + 42` solo funciona si hay una sobrecarga que lo acepte. Se
    /// proveen en las dos orientaciones, `fixed op T` y `T op fixed`.
    ///
    /// **Semantica comun a toda la familia:**
    /// - El entero del lenguaje se convierte a `uint_fixed_t<N>` y la operacion
    ///   se hace en esa anchura; el **resultado es siempre `uint_fixed_t<N>`**,
    ///   nunca el tipo del operando pequeno.
    /// - La aritmetica es **modular** respecto a 2^(64N), como la de los enteros
    ///   sin signo del lenguaje.
    /// - Un `T` con signo y valor negativo se convierte igual que lo haria el
    ///   lenguaje: por complemento a dos, de modo que `-1` es `max()`.
    /// - Todas son `constexpr`. Todas son `noexcept` **salvo `/` y `%`**, que
    ///   lanzan `std::domain_error` si el divisor es cero (ADR-004).
    /// @{

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator+(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a + uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator+(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} + b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator-(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a - uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator-(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} - b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator*(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a * uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator*(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} * b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator/(const uint_fixed_t<N> &a, T b)
    {
        return a / uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator/(T a, const uint_fixed_t<N> &b)
    {
        return uint_fixed_t<N>{a} / b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator%(const uint_fixed_t<N> &a, T b)
    {
        return a % uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator%(T a, const uint_fixed_t<N> &b)
    {
        return uint_fixed_t<N>{a} % b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator&(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a & uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator&(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} & b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator|(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a | uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator|(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} | b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator^(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a ^ uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator^(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} ^ b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a == uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} == b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a != uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} != b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a < uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} < b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a <= uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} <= b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a > uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} > b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(const uint_fixed_t<N> &a, T b) noexcept
    {
        return a >= uint_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(T a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} >= b;
    }

#ifdef __SIZEOF_INT128__
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a + uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} + b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a + uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} + b;
    }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a - uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} - b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a - uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} - b;
    }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a * uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} * b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a * uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} * b;
    }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator/(const uint_fixed_t<N> &a, unsigned __int128 b)
    {
        return a / uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator/(unsigned __int128 a, const uint_fixed_t<N> &b)
    {
        return uint_fixed_t<N>{a} / b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator/(const uint_fixed_t<N> &a, __int128 b)
    {
        return a / uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator/(__int128 a, const uint_fixed_t<N> &b)
    {
        return uint_fixed_t<N>{a} / b;
    }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator%(const uint_fixed_t<N> &a, unsigned __int128 b)
    {
        return a % uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator%(unsigned __int128 a, const uint_fixed_t<N> &b)
    {
        return uint_fixed_t<N>{a} % b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator%(const uint_fixed_t<N> &a, __int128 b)
    {
        return a % uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator%(__int128 a, const uint_fixed_t<N> &b)
    {
        return uint_fixed_t<N>{a} % b;
    }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a & uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} & b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a & uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} & b;
    }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a | uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} | b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a | uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} | b;
    }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a ^ uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} ^ b;
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a ^ uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} ^ b;
    }

    template <std::size_t N>
    constexpr bool operator==(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a == uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator==(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} == b;
    }
    template <std::size_t N>
    constexpr bool operator==(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a == uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator==(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} == b;
    }

    template <std::size_t N>
    constexpr bool operator!=(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a != uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator!=(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} != b;
    }
    template <std::size_t N>
    constexpr bool operator!=(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a != uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator!=(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} != b;
    }

    template <std::size_t N>
    constexpr bool operator<(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a < uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} < b;
    }
    template <std::size_t N>
    constexpr bool operator<(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a < uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} < b;
    }

    template <std::size_t N>
    constexpr bool operator<=(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a <= uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<=(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} <= b;
    }
    template <std::size_t N>
    constexpr bool operator<=(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a <= uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<=(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} <= b;
    }

    template <std::size_t N>
    constexpr bool operator>(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a > uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} > b;
    }
    template <std::size_t N>
    constexpr bool operator>(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a > uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} > b;
    }

    template <std::size_t N>
    constexpr bool operator>=(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a >= uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>=(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} >= b;
    }
    template <std::size_t N>
    constexpr bool operator>=(const uint_fixed_t<N> &a, __int128 b) noexcept
    {
        return a >= uint_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>=(__int128 a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<N>{a} >= b;
    }
#endif

    /// @}

    // =========================================================================
    // Free-function binary operators — int_fixed_t<N> mixed with integral T
    // =========================================================================

    /// @name Operadores mixtos: `int_fixed_t<N>` con un entero del lenguaje
    ///
    /// La misma familia que la anterior, para el tipo con signo. El resultado es
    /// siempre `int_fixed_t<N>`, la aritmetica es modular en complemento a dos y
    /// el desbordamiento **envuelve** en vez de ser comportamiento indefinido,
    /// que es la unica diferencia deliberada con los `int` del lenguaje.
    ///
    /// `constexpr` todas; `noexcept` todas salvo `/` y `%`.
    /// @{

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator+(const int_fixed_t<N> &a, T b) noexcept
    {
        return a + int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator+(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} + b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator-(const int_fixed_t<N> &a, T b) noexcept
    {
        return a - int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator-(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} - b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator*(const int_fixed_t<N> &a, T b) noexcept
    {
        return a * int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator*(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} * b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator/(const int_fixed_t<N> &a, T b)
    {
        return a / int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator/(T a, const int_fixed_t<N> &b)
    {
        return int_fixed_t<N>{a} / b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator%(const int_fixed_t<N> &a, T b)
    {
        return a % int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator%(T a, const int_fixed_t<N> &b)
    {
        return int_fixed_t<N>{a} % b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator&(const int_fixed_t<N> &a, T b) noexcept
    {
        return a & int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator&(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} & b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator|(const int_fixed_t<N> &a, T b) noexcept
    {
        return a | int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator|(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} | b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator^(const int_fixed_t<N> &a, T b) noexcept
    {
        return a ^ int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator^(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} ^ b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(const int_fixed_t<N> &a, T b) noexcept
    {
        return a == int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} == b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(const int_fixed_t<N> &a, T b) noexcept
    {
        return a != int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} != b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(const int_fixed_t<N> &a, T b) noexcept
    {
        return a < int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} < b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(const int_fixed_t<N> &a, T b) noexcept
    {
        return a <= int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} <= b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(const int_fixed_t<N> &a, T b) noexcept
    {
        return a > int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} > b;
    }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(const int_fixed_t<N> &a, T b) noexcept
    {
        return a >= int_fixed_t<N>{b};
    }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(T a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} >= b;
    }

#ifdef __SIZEOF_INT128__
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a + int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} + b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a + int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} + b;
    }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a - int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} - b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a - int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} - b;
    }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a * int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} * b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a * int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} * b;
    }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator/(const int_fixed_t<N> &a, unsigned __int128 b)
    {
        return a / int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator/(unsigned __int128 a, const int_fixed_t<N> &b)
    {
        return int_fixed_t<N>{a} / b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator/(const int_fixed_t<N> &a, __int128 b)
    {
        return a / int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator/(__int128 a, const int_fixed_t<N> &b)
    {
        return int_fixed_t<N>{a} / b;
    }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator%(const int_fixed_t<N> &a, unsigned __int128 b)
    {
        return a % int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator%(unsigned __int128 a, const int_fixed_t<N> &b)
    {
        return int_fixed_t<N>{a} % b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator%(const int_fixed_t<N> &a, __int128 b)
    {
        return a % int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator%(__int128 a, const int_fixed_t<N> &b)
    {
        return int_fixed_t<N>{a} % b;
    }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a & int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} & b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a & int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} & b;
    }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a | int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} | b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a | int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} | b;
    }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a ^ int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} ^ b;
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a ^ int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} ^ b;
    }

    template <std::size_t N>
    constexpr bool operator==(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a == int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator==(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} == b;
    }
    template <std::size_t N>
    constexpr bool operator==(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a == int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator==(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} == b;
    }

    template <std::size_t N>
    constexpr bool operator!=(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a != int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator!=(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} != b;
    }
    template <std::size_t N>
    constexpr bool operator!=(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a != int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator!=(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} != b;
    }

    template <std::size_t N>
    constexpr bool operator<(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a < int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} < b;
    }
    template <std::size_t N>
    constexpr bool operator<(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a < int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} < b;
    }

    template <std::size_t N>
    constexpr bool operator<=(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a <= int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<=(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} <= b;
    }
    template <std::size_t N>
    constexpr bool operator<=(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a <= int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator<=(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} <= b;
    }

    template <std::size_t N>
    constexpr bool operator>(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a > int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} > b;
    }
    template <std::size_t N>
    constexpr bool operator>(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a > int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} > b;
    }

    template <std::size_t N>
    constexpr bool operator>=(const int_fixed_t<N> &a, unsigned __int128 b) noexcept
    {
        return a >= int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>=(unsigned __int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} >= b;
    }
    template <std::size_t N>
    constexpr bool operator>=(const int_fixed_t<N> &a, __int128 b) noexcept
    {
        return a >= int_fixed_t<N>{b};
    }
    template <std::size_t N>
    constexpr bool operator>=(__int128 a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<N>{a} >= b;
    }
#endif

    /// @}

    // =========================================================================
    // Cross-N binary operators — uint_fixed_t<N> op uint_fixed_t<M> (N != M)
    // =========================================================================

    /// @name Operadores entre anchuras distintas, sin signo
    ///
    /// `uint_fixed_t<N> op uint_fixed_t<M>` con `N != M`. **Gana la anchura
    /// mayor**: el operando estrecho se extiende con ceros y el resultado es
    /// `uint_fixed_t<max(N,M)>`. Es el equivalente de las promociones del
    /// lenguaje, pero sin perder bits nunca.
    ///
    /// `constexpr` todas; `noexcept` todas salvo `/` y `%`.
    /// @{

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator+(const uint_fixed_t<N> &a,
                                                      const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} + uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator-(const uint_fixed_t<N> &a,
                                                      const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} - uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator*(const uint_fixed_t<N> &a,
                                                      const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} * uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator/(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} / uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator%(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} % uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator&(const uint_fixed_t<N> &a,
                                                      const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} & uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator|(const uint_fixed_t<N> &a,
                                                      const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} | uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator^(const uint_fixed_t<N> &a,
                                                      const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} ^ uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator==(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} == uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator!=(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} != uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} < uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<=(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} <= uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} > uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>=(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} >= uint_fixed_t<R>{b};
    }

    /// @}

    // =========================================================================
    // Cross-N binary operators — int_fixed_t<N> op int_fixed_t<M> (N != M)
    // =========================================================================

    /// @name Operadores entre anchuras distintas, con signo
    ///
    /// `int_fixed_t<N> op int_fixed_t<M>` con `N != M`. Gana la anchura mayor y
    /// el operando estrecho se extiende **con su signo**, de modo que un valor
    /// negativo sigue siendo el mismo negativo en la anchura grande.
    ///
    /// `constexpr` todas; `noexcept` todas salvo `/` y `%`.
    /// @{

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator+(const int_fixed_t<N> &a,
                                                     const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} + int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator-(const int_fixed_t<N> &a,
                                                     const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} - int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator*(const int_fixed_t<N> &a,
                                                     const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} * int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator/(const int_fixed_t<N> &a, const int_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} / int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator%(const int_fixed_t<N> &a, const int_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} % int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator&(const int_fixed_t<N> &a,
                                                     const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} & int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator|(const int_fixed_t<N> &a,
                                                     const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} | int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator^(const int_fixed_t<N> &a,
                                                     const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} ^ int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator==(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} == int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator!=(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} != int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} < int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<=(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} <= int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} > int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>=(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} >= int_fixed_t<R>{b};
    }

    /// @}

    // =========================================================================
    // Mixed-sign free operators — int_fixed_t<N> op uint_fixed_t<M>
    // C++ usual arithmetic conversions: N > M -> int_fixed_t<N>; N <= M -> uint_fixed_t<M>.
    // Both orientations (int op uint, uint op int) produce the same result type.
    // =========================================================================

    /// @name Operadores entre signo y sin signo
    ///
    /// `int_fixed_t<N> op uint_fixed_t<M>` y la orientacion contraria, que dan
    /// **el mismo tipo de resultado**. La regla es la de las conversiones
    /// aritmeticas usuales de C++, con la misma sorpresa incluida:
    ///
    /// - `N > M`  -> `int_fixed_t<N>`: el con signo es mas ancho y gana.
    /// - `N <= M` -> `uint_fixed_t<M>`: **gana el sin signo**, y el operando con
    ///   signo se convierte, de modo que un valor negativo pasa a ser un valor
    ///   grande. Es exactamente lo que hace `-1 < 0u` en C++, que es `false`.
    ///
    /// Se imita a proposito: la biblioteca reproduce la aritmetica del lenguaje,
    /// trampas incluidas, para que trasladar codigo no cambie de significado.
    ///
    /// `constexpr` todas; `noexcept` todas salvo `/` y `%`.
    /// @{

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator+(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} + R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator+(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} + R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator-(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} - R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator-(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} - R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator*(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} * R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator*(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} * R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator/(const int_fixed_t<N> &a, const uint_fixed_t<M> &b)
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} / R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator/(const uint_fixed_t<M> &a, const int_fixed_t<N> &b)
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} / R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator%(const int_fixed_t<N> &a, const uint_fixed_t<M> &b)
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} % R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator%(const uint_fixed_t<M> &a, const int_fixed_t<N> &b)
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} % R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator&(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} & R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator&(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} & R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator|(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} | R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator|(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} | R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator^(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} ^ R { b };
    }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator^(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} ^ R { b };
    }

    // Free three-way comparison for cross-N and/or cross-sign fixed_int_t.
    // Coexists with the 12 manual cross-sign comparators below (T2 — MS-INTEROP):
    // explicit comparators win during overload resolution; this <=> kicks in
    // when user code writes `a <=> b` directly, or when generic code routes
    // comparisons through <=> (std::strong_order, std::compare_three_way).
    //
    // Constrained so it does NOT match same-type same-N (which is handled by the
    // member <=>); ambiguity would otherwise arise.
    template <std::size_t N1, signedness S1, representation_form F1, std::size_t N2, signedness S2,
              representation_form F2>
        requires(N1 != N2 || S1 != S2)
    constexpr std::strong_ordering operator<=>(const fixed_int_t<N1, S1, F1> &a,
                                               const fixed_int_t<N2, S2, F2> &b) noexcept
    {
        if constexpr (S1 == S2)
        {
            // Same-sign cross-N: promote to wider, same Sign and Form (default alias).
            using R = fixed_int_t<(N1 > N2 ? N1 : N2), S1, F1>;
            return R{a} <=> R{b};
        }
        else if constexpr (S1 == signedness::signed_type)
        {
            // a is signed (rank N1), b is unsigned (rank N2) → mixed_iu_t<N1, N2>.
            using R = mixed_iu_t<N1, N2>;
            return R{a} <=> R{b};
        }
        else
        {
            // a is unsigned (rank N1), b is signed (rank N2) → mixed_iu_t<N2, N1>.
            using R = mixed_iu_t<N2, N1>;
            return R{a} <=> R{b};
        }
    }

    template <std::size_t N, std::size_t M>
    constexpr bool operator==(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} == R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr bool operator==(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} == R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr bool operator!=(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} != R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr bool operator!=(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} != R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr bool operator<(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} < R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr bool operator<(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} < R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr bool operator<=(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} <= R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr bool operator<=(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} <= R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr bool operator>(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} > R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr bool operator>(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} > R{b};
    }

    template <std::size_t N, std::size_t M>
    constexpr bool operator>=(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} >= R{b};
    }
    template <std::size_t N, std::size_t M>
    constexpr bool operator>=(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    {
        using R = detail::mixed_iu_t<N, M>;
        return R{a} >= R{b};
    }

    /// @}

    // =========================================================================
    // Higher arithmetic — mul_wide, pow, sqrt, gcd, lcm, checked_*
    // =========================================================================
    //
    // P1.5 tramo 2 (2a): estas nueve firmas estaban escritas sobre
    // `uint_fixed_t<N>` / `int_fixed_t<N>`, es decir, con la politica FIJADA a
    // la de por defecto. Con un tipo `checked` no compilaban: `gcd(a, b)` sobre
    // `uint_fixed_t<2, overflow_policy::checked>` daba "no matching function".
    //
    // Ahora llevan `Policy`, que es DEDUCIBLE del argumento --los alias de
    // plantilla son transparentes-- asi que ninguna llamada existente cambia y
    // el valor por defecto solo actua cuando se dan los parametros a mano.
    //
    // La politica se CONSERVA en el resultado, incluso cuando cambia el signo
    // (`gcd` y `lcm` con signo devuelven sin signo), que es lo mismo que hacen
    // `make_signed`/`make_unsigned` por ADR-008.

    /// @brief Producto **sin perder bits**: `N x N -> 2N` limbos.
    ///
    /// A diferencia de `operator*`, que es modular respecto a 2^(64N), aqui el
    /// resultado tiene el doble de anchura y **nunca desborda**. Es la operacion
    /// sobre la que se construyen `checked_mul()` y la division por constante.
    ///
    /// @param a Primer factor.
    /// @param b Segundo factor.
    /// @return El producto exacto, en `uint_fixed_t<2 * N>`.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<2 * N, Policy> mul_wide(const uint_fixed_t<N, Policy> &a,
                                                                 const uint_fixed_t<N, Policy> &b) noexcept
    {
        return uint_fixed_t<2 * N, Policy>{a} * uint_fixed_t<2 * N, Policy>{b};
    }

    /// @brief Producto con signo sin perder bits: `N x N -> 2N` limbos.
    /// @param a Primer factor.
    /// @param b Segundo factor.
    /// @return El producto exacto, en `int_fixed_t<2 * N>`.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr int_fixed_t<2 * N, Policy> mul_wide(const int_fixed_t<N, Policy> &a,
                                                                const int_fixed_t<N, Policy> &b) noexcept
    {
        return int_fixed_t<2 * N, Policy>{a} * int_fixed_t<2 * N, Policy>{b};
    }

    /// @brief Potencia por cuadrados repetidos, **modular**.
    ///
    /// @param base Base.
    /// @param exp  Exponente, sin signo.
    /// @return `base^exp` modulo 2^(64N). **No avisa si desborda**, igual que
    ///         `operator*`: si el resultado no cabe, se envuelve. Para saberlo,
    ///         acotar el exponente antes o usar `mul_wide()` a mano.
    ///
    /// `pow(x, 0)` es 1, incluido `pow(0, 0)`, que es la convencion habitual.
    /// El coste es logaritmico en el exponente, no lineal.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<N, Policy> pow(uint_fixed_t<N, Policy> base,
                                                        uint_fixed_t<N, Policy> exp) noexcept
    {
        uint_fixed_t<N, Policy> result = uint_fixed_t<N, Policy>::one();
        while (!exp.is_zero())
        {
            if (exp.limb(0) & std::uint64_t{1})
                result *= base;
            base *= base;
            exp >>= 1;
        }
        return result;
    }

    /// @brief Potencia con base con signo y exponente sin signo, modular.
    ///
    /// El exponente es **sin signo a proposito**: un exponente negativo daria un
    /// racional, que no es representable aqui.
    ///
    /// @param base Base, que puede ser negativa.
    /// @param exp  Exponente, sin signo.
    /// @return `base^exp` modulo 2^(64N), con el signo que corresponda.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr int_fixed_t<N, Policy> pow(int_fixed_t<N, Policy> base,
                                                       uint_fixed_t<N, Policy> exp) noexcept
    {
        int_fixed_t<N, Policy> result = int_fixed_t<N, Policy>::one();
        while (!exp.is_zero())
        {
            if (exp.limb(0) & std::uint64_t{1})
                result *= base;
            base *= base;
            exp >>= 1;
        }
        return result;
    }

    /// @brief Raiz cuadrada entera: el mayor `r` tal que `r * r <= x`.
    ///
    /// Metodo de Newton sobre enteros, partiendo de una potencia de dos deducida
    /// de `bit_width()`. Converge cuadraticamente.
    ///
    /// @param x Radicando.
    /// @return `floor(sqrt(x))`. Para `x = 0` devuelve 0.
    /// @warning **No es `noexcept`**: usa `operator/`, que lanza
    ///          `std::domain_error` si el divisor es cero. No puede ocurrir con
    ///          la iteracion de aqui, pero la firma lo arrastra.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<N, Policy> sqrt(const uint_fixed_t<N, Policy> &x)
    {
        if (x.is_zero())
            return uint_fixed_t<N, Policy>{};
        const unsigned bw = x.bit_width();
        uint_fixed_t<N, Policy> r = uint_fixed_t<N, Policy>::one() << ((bw + 1) / 2);
        for (;;)
        {
            const uint_fixed_t<N, Policy> nr = (r + x / r) >> 1;
            if (nr >= r)
                break;
            r = nr;
        }
        return r;
    }

    /// @brief Maximo comun divisor, por el **algoritmo binario de Stein**.
    ///
    /// Se usa Stein y no Euclides porque solo necesita restas y desplazamientos:
    /// evita la division, que en anchuras grandes es con diferencia la operacion
    /// mas cara.
    ///
    /// @param a Primer operando.
    /// @param b Segundo operando.
    /// @return `gcd(a, b)`; `gcd(x, 0)` es `x`, y `gcd(0, 0)` es 0.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<N, Policy> gcd(uint_fixed_t<N, Policy> a,
                                                        uint_fixed_t<N, Policy> b) noexcept
    {
        if (a.is_zero())
            return b;
        if (b.is_zero())
            return a;
        const unsigned ka = a.count_trailing_zeros();
        const unsigned kb = b.count_trailing_zeros();
        const unsigned k = ka < kb ? ka : kb;
        a >>= ka;
        b >>= kb;
        while (!b.is_zero())
        {
            if (a < b)
            {
                uint_fixed_t<N, Policy> t = a;
                a = b;
                b = t;
            }
            a -= b;
            if (!a.is_zero())
                a >>= a.count_trailing_zeros();
        }
        return a << k;
    }

    /// @brief Maximo comun divisor de dos enteros con signo.
    ///
    /// @param a Primer operando, con signo.
    /// @param b Segundo operando, con signo.
    /// @return `gcd(|a|, |b|)`, **sin signo**: el mcd se define sobre los valores
    ///         absolutos y siempre es no negativo.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<N, Policy> gcd(const int_fixed_t<N, Policy> &a,
                                                        const int_fixed_t<N, Policy> &b) noexcept
    {
        return gcd(a.is_negative() ? uint_fixed_t<N, Policy>{-a} : uint_fixed_t<N, Policy>{a},
                   b.is_negative() ? uint_fixed_t<N, Policy>{-b} : uint_fixed_t<N, Policy>{b});
    }

    /// @brief Minimo comun multiplo.
    ///
    /// Calculado como `a / gcd(a, b) * b`, en ese orden: dividir **antes** de
    /// multiplicar evita desbordar en el paso intermedio siempre que el
    /// resultado quepa.
    ///
    /// @param a Primer operando.
    /// @param b Segundo operando.
    /// @return `lcm(a, b)`; 0 si alguno de los dos es 0. Si el resultado no cabe
    ///         en `64 * N` bits **se envuelve**, sin aviso.
    /// @warning No es `noexcept`: usa `operator/`.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<N, Policy> lcm(const uint_fixed_t<N, Policy> &a,
                                                        const uint_fixed_t<N, Policy> &b)
    {
        if (a.is_zero() || b.is_zero())
            return uint_fixed_t<N, Policy>{};
        return a / gcd(a, b) * b;
    }

    /// @brief Minimo comun multiplo de dos enteros con signo.
    /// @param a Primer operando, con signo.
    /// @param b Segundo operando, con signo.
    /// @return `lcm(|a|, |b|)`, sin signo.
    /// @warning No es `noexcept`: usa `operator/`.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<N, Policy> lcm(const int_fixed_t<N, Policy> &a,
                                                        const int_fixed_t<N, Policy> &b)
    {
        const uint_fixed_t<N, Policy> ua =
            a.is_negative() ? uint_fixed_t<N, Policy>{-a} : uint_fixed_t<N, Policy>{a};
        const uint_fixed_t<N, Policy> ub =
            b.is_negative() ? uint_fixed_t<N, Policy>{-b} : uint_fixed_t<N, Policy>{b};
        return lcm(ua, ub);
    }

    // =========================================================================
    // P1.5, tramo 2: `int128_param_arithmetic.hpp`
    // =========================================================================
    //
    // De las tres que tenia ese header solo falta UNA de verdad:
    //
    // - `widening_mul(a, b)` es **exactamente** `mul_wide(a, b)`, que ya existe.
    //   No se porta el nombre, por lo mismo que no se porto `power`: un segundo
    //   nombre para la misma operacion es deuda recien estrenada. Quien venga de
    //   `int128_param_t` busca `mul_wide`.
    // - `mulhi(a, b)` NO existia: es la mitad ALTA del producto de doble
    //   anchura, y no se puede sacar de `operator*`, que da la baja. Se porta.
    // - `mullo(a, b)` es `operator*`. El header viejo ya decia que existia "por
    //   simetria con mulhi", y esa razon se sostiene: en codigo generico las dos
    //   van juntas, y escribir `mulhi(a,b)` al lado de `a*b` se lee peor que
    //   `mulhi(a,b)` al lado de `mullo(a,b)`. Se porta.

    /// @name Mitades alta y baja del producto
    /// @{

    /// @brief Mitad **alta** del producto de doble anchura.
    /// @param a Primer factor. @param b Segundo factor.
    /// @return Los `64*N` bits altos de `a * b`, o sea `(a * b) >> (64*N)`.
    ///
    /// @note Es lo que `operator*` **tira**. Junto a `mullo` reconstruye el
    ///       producto exacto sin pasar por un tipo de doble anchura.
    /// @note No lleva marca de desbordamiento aunque la politica sea `checked`:
    ///       no hay tal cosa que desbordar, porque el resultado exacto se
    ///       calcula en `2N` limbos y aqui solo se elige que mitad devolver.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr uint_fixed_t<N, Policy> mulhi(const uint_fixed_t<N, Policy> &a,
                                                          const uint_fixed_t<N, Policy> &b) noexcept
    {
        const uint_fixed_t<2 * N, Policy> ancho = mul_wide(a, b);
        uint_fixed_t<N, Policy> r{};
        for (std::size_t i = 0; i < N; ++i)
            r.set_limb(i, ancho.limb(N + i));
        return r;
    }

    /// @brief Mitad alta del producto con signo.
    /// @param a Primer factor. @param b Segundo factor.
    /// @return Los `64*N` bits altos del producto exacto con signo.
    ///
    /// @note El resultado se devuelve **con signo**: la mitad alta de un
    ///       producto con signo lleva la extension de signo del producto
    ///       completo, y leerla sin signo es justo el error que tenia
    ///       `producto_desborda` antes de P1.3.
    template <std::size_t N, overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr int_fixed_t<N, Policy> mulhi(const int_fixed_t<N, Policy> &a,
                                                         const int_fixed_t<N, Policy> &b) noexcept
    {
        const int_fixed_t<2 * N, Policy> ancho = mul_wide(a, b);
        int_fixed_t<N, Policy> r{};
        for (std::size_t i = 0; i < N; ++i)
            r.set_limb(i, ancho.limb(N + i));
        return r;
    }

    /// @brief Mitad **baja** del producto. Es `a * b`.
    /// @param a Primer factor. @param b Segundo factor.
    /// @return `a * b` modulo `2^(64*N)`.
    ///
    /// @note Existe por simetria con `mulhi`, no porque anada nada a
    ///       `operator*`. Con la politica `checked`, y a diferencia de `mulhi`,
    ///       **si marca** si el producto no cabe: es `operator*`.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
    mullo(const fixed_int_t<N, Sign, Form, Policy> &a, const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        return a * b;
    }
    /// @}

    // =========================================================================
    // P1.5, tramo 1: bits, cmath y numeric de `int128_param_t`
    // =========================================================================
    //
    // Primera entrega de la paridad que pide ADR-006. Son las tres familias mas
    // independientes: no tocan el nucleo del tipo ni la representacion, asi que
    // se pueden portar y comprobar por separado.
    //
    // Lo que NO se porta, porque ya existe con otro nombre: `countl_zero` y
    // `countr_zero` son `count_leading_zeros` y `count_trailing_zeros`;
    // `isqrt` es `sqrt`. Se anaden los nombres de `<bit>` como alias, porque el
    // codigo que venga de tipos del lenguaje los buscara asi.

    /// @name Rotaciones y nombres de `<bit>`
    /// @{

    /// @brief Rotacion a la izquierda sobre los `64*N` bits.
    /// @param x Valor a rotar.
    /// @param s Numero de posiciones. Se toma modulo la anchura, y **acepta
    ///          negativos**, que rotan al otro lado -- igual que `std::rotl`.
    /// @return El valor rotado.
    ///
    /// @note Rota el patron de bits completo, sin tratar el signo de forma
    ///       especial. Es lo que hace `std::rotl` con los enteros del lenguaje.
    ///       El `rotl` de `int128_param_t` SI trataba el signo aparte en
    ///       Magnitud-Signo, pero eso es propio de esa representacion y aqui no
    ///       aplica mientras `fixed_int_t` solo admita binnat y complemento a
    ///       dos (ADR-011).
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
    rotl(const fixed_int_t<N, Sign, Form, Policy> &x, int s) noexcept
    {
        constexpr int ancho = static_cast<int>(64U * N);
        int k = s % ancho;
        if (k < 0)
            k += ancho;
        if (k == 0)
            return x;
        return (x << static_cast<unsigned>(k)) | (x >> static_cast<unsigned>(ancho - k));
    }

    /// @brief Rotacion a la derecha. Es `rotl(x, -s)`.
    /// @param x Valor a rotar.
    /// @param s Numero de posiciones; acepta negativos.
    /// @return El valor rotado.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
    rotr(const fixed_int_t<N, Sign, Form, Policy> &x, int s) noexcept
    {
        return rotl(x, -s);
    }

    /// @brief Ceros por delante. Nombre de `<bit>` para `count_leading_zeros`.
    /// @param x Valor a examinar.
    /// @return Cuantos bits cero hay antes del primer uno, o `64*N` si es cero.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr unsigned countl_zero(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        return x.count_leading_zeros();
    }

    /// @brief Ceros por detras. Nombre de `<bit>` para `count_trailing_zeros`.
    /// @param x Valor a examinar.
    /// @return Cuantos bits cero hay tras el ultimo uno, o `64*N` si es cero.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr unsigned countr_zero(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        return x.count_trailing_zeros();
    }

    /// @brief Numero de bits a uno. Nombre de `<bit>` para `popcount`.
    /// @param x Valor a examinar.
    /// @return Cuantos bits valen uno.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr unsigned popcount(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        return x.popcount();
    }

    /// @brief Anchura en bits del valor. Nombre de `<bit>` para `bit_width`.
    /// @param x Valor a examinar.
    /// @return Bits necesarios para representarlo; 0 si es cero.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr unsigned bit_width(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        return x.bit_width();
    }
    /// @}

    /// @name Comparacion y mezcla (`<algorithm>` y `<numeric>`)
    /// @{

    /// @brief El menor de dos.
    /// @param a Primer valor. @param b Segundo valor.
    /// @return El menor; `a` si son iguales.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr const fixed_int_t<N, Sign, Form, Policy> &
    min(const fixed_int_t<N, Sign, Form, Policy> &a, const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        return (b < a) ? b : a;
    }

    /// @brief El mayor de dos.
    /// @param a Primer valor. @param b Segundo valor.
    /// @return El mayor; `a` si son iguales.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr const fixed_int_t<N, Sign, Form, Policy> &
    max(const fixed_int_t<N, Sign, Form, Policy> &a, const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        return (a < b) ? b : a;
    }

    /// @brief Encaja `x` en el intervalo `[lo, hi]`.
    /// @param x Valor a encajar. @param lo Minimo. @param hi Maximo.
    /// @return `lo` si `x < lo`, `hi` si `x > hi`, y `x` en otro caso.
    /// @pre `lo <= hi`. Si no, se devuelve `hi`, igual que hace `std::clamp`
    ///      con un intervalo invertido -- que es comportamiento indefinido
    ///      alli, y aqui simplemente no se promete nada util.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr const fixed_int_t<N, Sign, Form, Policy> &
    clamp(const fixed_int_t<N, Sign, Form, Policy> &x, const fixed_int_t<N, Sign, Form, Policy> &lo,
          const fixed_int_t<N, Sign, Form, Policy> &hi) noexcept
    {
        return (x < lo) ? lo : ((hi < x) ? hi : x);
    }

    /// @brief Punto medio de dos valores, SIN desbordar por el camino.
    /// @param a Primer valor. @param b Segundo valor.
    /// @return `(a + b) / 2`, redondeado hacia `a`.
    ///
    /// @note No se calcula como `(a + b) / 2`, que desborda cuando la suma no
    ///       cabe -- justo el caso en que hace falta. Se usa
    ///       `a + (b - a) / 2` sobre la diferencia, que siempre cabe. Es la
    ///       misma razon por la que existe `std::midpoint`.
    template <std::size_t N, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr fixed_int_t<N, signedness::unsigned_type, Form, Policy>
    midpoint(const fixed_int_t<N, signedness::unsigned_type, Form, Policy> &a,
             const fixed_int_t<N, signedness::unsigned_type, Form, Policy> &b) noexcept
    {
        using U = fixed_int_t<N, signedness::unsigned_type, Form, Policy>;
        return (a < b) ? U{a + ((b - a) >> 1U)} : U{b + ((a - b) >> 1U)};
    }

    /// @brief Diferencia en valor absoluto, sin signo y sin desbordar.
    /// @param a Primer valor. @param b Segundo valor.
    /// @return `|a - b|`.
    template <std::size_t N, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr fixed_int_t<N, signedness::unsigned_type, Form, Policy>
    abs_diff(const fixed_int_t<N, signedness::unsigned_type, Form, Policy> &a,
             const fixed_int_t<N, signedness::unsigned_type, Form, Policy> &b) noexcept
    {
        return (a < b) ? (b - a) : (a - b);
    }
    /// @}

    /// @name Predicados y funciones enteras
    /// @{

    /// @brief Si es par.
    /// @param x Valor a examinar.
    /// @return `true` si el bit mas bajo es cero.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr bool is_even(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        return (x.limb(0) & std::uint64_t{1}) == 0;
    }

    /// @brief Si es impar.
    /// @param x Valor a examinar.
    /// @return `true` si el bit mas bajo es uno.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr bool is_odd(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        return !is_even(x);
    }

    /// @brief Logaritmo en base dos, truncado.
    /// @param x Valor, que **no puede ser cero**.
    /// @return El mayor `k` con `2^k <= x`, o sea `bit_width(x) - 1`.
    /// @throws std::domain_error si `x` es cero: `log2(0)` no existe, y
    ///         devolver 0 o -1 seria dar un numero donde no lo hay.
    template <std::size_t N, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr unsigned ilog2(const fixed_int_t<N, signedness::unsigned_type, Form, Policy> &x)
    {
        if (x.is_zero())
            throw std::domain_error("nstd::ilog2: el logaritmo de cero no existe");
        return x.bit_width() - 1U;
    }

    /// @brief Factorial.
    /// @param n Cuantos factores; con `n <= 1` da uno.
    /// @return `n!` truncado a `64*N` bits.
    ///
    /// @note **Desborda muy pronto**: 34! ya no cabe en 128 bits. Con la
    ///       politica `wrap` el resultado envuelve en silencio, que es lo que
    ///       hacen los enteros del lenguaje; con `checked` queda marcado. No se
    ///       pone un tope artificial porque el que cabe depende de N.
    template <std::size_t N, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr fixed_int_t<N, signedness::unsigned_type, Form, Policy>
    factorial(unsigned n) noexcept
    {
        using U = fixed_int_t<N, signedness::unsigned_type, Form, Policy>;
        U r{std::uint64_t{1}};
        for (unsigned k = 2; k <= n; ++k)
            r = r * U{static_cast<std::uint64_t>(k)};
        return r;
    }
    /// @}

    /// @name Lo que el inventario de ADR-006 no listaba
    /// @{
    //
    // Al contrastar el tramo 1 contra los headers viejos aparecieron cuatro
    // funciones publicas que las filas del ADR no mencionaban: `is_power_of_2`
    // (bits), `sign` y `divmod` libres (numeric) y `abs` libre (cmath). El
    // inventario estaba incompleto, no el codigo; se cierran aqui y se corrige
    // la tabla.
    //
    // La que NO se porta es `power`, que en `int128_param_numeric.hpp` era un
    // alias de `pow` "por consistencia con phase166". Esa consistencia es
    // justo la capa que ADR-006 retira, asi que un segundo nombre para lo
    // mismo seria deuda recien estrenada. `pow` se queda solo.

    /// @brief Si el valor es una potencia exacta de dos.
    /// @param x Valor a examinar.
    /// @return `true` si tiene exactamente un bit a uno **y no es negativo**.
    ///
    /// @note El cero da `false`: no es potencia de dos de nada.
    /// @note Con signo, los negativos dan `false` aunque el patron tenga un
    ///       solo bit. El caso es real: en complemento a dos el minimo
    ///       (`-2^(64N-1)`) tiene justo el bit de signo puesto, y `popcount`
    ///       diria 1. No es una potencia de dos; es el numero mas negativo.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr bool is_power_of_2(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        if (x.is_zero())
            return false;
        if constexpr (Sign == signedness::signed_type)
        {
            if (x.is_negative())
                return false;
        }
        return x.popcount() == 1U;
    }

    /// @brief Signo del valor.
    /// @param x Valor a examinar.
    /// @return `-1` si es negativo, `0` si es cero, `+1` si es positivo.
    ///
    /// @note Sin signo solo puede devolver `0` o `+1`.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr int sign(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        if (x.is_zero())
            return 0;
        if constexpr (Sign == signedness::signed_type)
            return x.is_negative() ? -1 : 1;
        else
            return 1;
    }

    /// @brief Valor absoluto, como funcion libre.
    /// @param x Valor.
    /// @return `|x|`.
    ///
    /// @note **Acepta tambien sin signo**, donde es la identidad. El metodo
    ///       `abs()` de la clase solo existe con signo, y con razon: pedirle el
    ///       absoluto a un `uint` suele ser un error de quien escribe. Pero la
    ///       funcion libre la llama codigo generico que vale para los dos, y
    ///       ahi negar el caso obliga a un `if constexpr` en cada sitio que la
    ///       use. Se resuelve una vez aqui.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
    abs(const fixed_int_t<N, Sign, Form, Policy> &x) noexcept
    {
        if constexpr (Sign == signedness::signed_type)
            return x.abs();
        else
            return x;
    }

    /// @brief Cociente y resto de una vez.
    /// @param a Dividendo. @param b Divisor.
    /// @return `{cociente, resto}`.
    /// @throws std::domain_error si `b` es cero.
    ///
    /// @note Cuesta lo mismo que una sola division: el algoritmo de Knuth
    ///       produce los dos a la vez, y pedirlos por separado con `/` y `%`
    ///       lo ejecuta dos veces.
    /// @note El `divmod` de `int128_param_numeric.hpp` devolvia `{0, 0}` al
    ///       dividir por cero y lo documentaba como comportamiento indefinido.
    ///       Aqui lanza, como el resto de la division de `fixed_int_t`:
    ///       devolver un cero que parece un resultado es peor que parar.
    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] constexpr std::pair<fixed_int_t<N, Sign, Form, Policy>, fixed_int_t<N, Sign, Form, Policy>>
    divmod(const fixed_int_t<N, Sign, Form, Policy> &a, const fixed_int_t<N, Sign, Form, Policy> &b)
    {
        return fixed_int_t<N, Sign, Form, Policy>::divmod(a, b);
    }
    /// @}

    /// @name Aritmetica comprobada y saturada
    ///
    /// Dos formas de no tragarse un desbordamiento en silencio, para quien no
    /// quiera cambiar el tipo de sus variables:
    ///
    /// - Las `checked_*` **devuelven el propio tipo con politica `checked`**.
    ///   La funcion libre y la politica dejan de ser dos mecanismos y pasan a
    ///   ser uno con dos puertas de entrada: `valid()` es la consulta en los dos
    ///   casos, y el resultado se puede seguir encadenando.
    /// - Las `saturating_*` devuelven el tipo de siempre, pegado a `max()` o a
    ///   `min()` cuando no cabe.
    ///
    /// **CAMBIO QUE ROMPE (5 sep 2026).** Las `checked_*` devolvian
    /// `std::optional<...>`. Era la unica de las tres formas candidatas que
    /// **tiraba el valor** al desbordar, y rompia el encadenado: `checked_add(a,b)`
    /// no se podia volver a sumar sin desenvolverlo. La migracion es directa:
    ///
    /// | antes | ahora |
    /// |---|---|
    /// | `r.has_value()` | `r.valid()` |
    /// | `*r` o `r.value()` | `r` (el valor siempre esta ahi) |
    /// | `r == std::optional<U>{U{3}}` | `r == checked_of<U>{3}` |
    ///
    /// See ADR-009.
    /// @{

    // P1.5 tramo 2f (10 sep 2026): LAS SIETE ACEPTAN CUALQUIER POLITICA.
    //
    // Antes tomaban solo operandos `wrap`, asi que `checked_add(a, b)` sobre un
    // `uint_fixed_t<2, checked>` no compilaba. Lo encontro la matriz de paridad
    // en su primera pasada. Codigo generico que llamase a `saturating_add` dejaba
    // de compilar en cuanto alguien cambiaba la politica del tipo, que es justo
    // lo que un parametro de plantilla no deberia provocar.
    //
    // LA MARCA ES PEGAJOSA, y esa fue la decision de verdad. `saturating_add`
    // sobre un valor YA MARCADO satura, pero **no limpia la marca**. El motivo
    // es concreto y no estetico: un valor marcado guarda dentro el resultado
    // ENVUELTO, no el verdadero --`max() + 1` con `checked` deja un cero
    // dentro--, asi que saturar a partir de el no da el valor saturado correcto.
    //
    //     x = max() + 1        -> marcado, y dentro lleva 0
    //     saturating_add(x, 5) -> 5, cuando lo saturado de verdad seria max()
    //
    // Devolver ese 5 SIN marca seria afirmar que esta bien un numero que no lo
    // esta, y despues ya no habria forma de saberlo: convierte un error
    // detectado en una respuesta equivocada indetectable. Con la marca puesta
    // sigue siendo un 5 equivocado, pero `valid()` lo dice.
    //
    // La alternativa --limpiar-- SERIA correcta si el valor marcado conservara
    // lo necesario para saturar de verdad. Con `checked` no lo conserva: guarda
    // el envuelto mas un bit. Con la politica `saturate`, declarada y todavia
    // sin escribir (ADR-009), el contenido SI seria el valor saturado, y ahi
    // limpiar tendria sentido. Es una diferencia entre las dos politicas que
    // conviene tener presente el dia que se escriba `saturate`.

    /// @brief Suma comprobada.
    /// @param a Primer sumando.
    /// @param b Segundo sumando.
    /// @return La suma, con politica `checked`. Si desbordo, queda marcada y
    ///         `valid()` devuelve `false`; **el valor envuelto sigue ahi**.
    /// @note Acepta operandos de **cualquier** politica. Si ya venian marcados,
    ///       el resultado hereda la marca (ADR-008).
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, overflow_policy::checked>
    checked_add(const fixed_int_t<N, Sign, Form, Policy> &a,
                const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        using C = fixed_int_t<N, Sign, Form, overflow_policy::checked>;
        return C{a} + C{b};
    }

    /// @brief Resta comprobada.
    /// @param a Minuendo.
    /// @param b Sustraendo.
    /// @return La diferencia, con politica `checked`.
    /// @note Acepta operandos de cualquier politica; la marca se hereda.
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, overflow_policy::checked>
    checked_sub(const fixed_int_t<N, Sign, Form, Policy> &a,
                const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        using C = fixed_int_t<N, Sign, Form, overflow_policy::checked>;
        return C{a} - C{b};
    }

    /// @brief Producto comprobado.
    /// @param a Primer factor.
    /// @param b Segundo factor.
    /// @return El producto, con politica `checked`.
    /// @note Acepta operandos de cualquier politica; la marca se hereda.
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, overflow_policy::checked>
    checked_mul(const fixed_int_t<N, Sign, Form, Policy> &a,
                const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        using C = fixed_int_t<N, Sign, Form, overflow_policy::checked>;
        return C{a} * C{b};
    }

    /// @brief Division comprobada. **Faltaba**, y sin ella `int128_param_t` no
    ///        podia retirarse (ADR-006).
    ///
    /// @param a Dividendo.
    /// @param b Divisor.
    /// @return El cociente, con politica `checked`.
    /// @note Acepta operandos de cualquier politica; la marca se hereda.
    /// @note **No lanza** aunque `b` sea cero: marca. Es la diferencia con
    ///       `operator/`, y la razon de que exista.
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, overflow_policy::checked>
    checked_div(const fixed_int_t<N, Sign, Form, Policy> &a,
                const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        using W = fixed_int_t<N, Sign, Form, overflow_policy::wrap>;
        using C = fixed_int_t<N, Sign, Form, overflow_policy::checked>;

        // El calculo se hace sin marca, para que un operando ya marcado no se
        // confunda con un desbordamiento de ESTA division; la marca previa se
        // vuelve a unir al final.
        const W wa{a};
        const W wb{b};
        const C heredada = (C{a} - C{a}) + (C{b} - C{b}); // cero, con las marcas

        // Dividir por cero no desborda: NO TIENE RESULTADO. Se marca, y el valor
        // que queda dentro es cero porque cualquier otro seria igual de
        // arbitrario.
        if (wb.is_zero())
            return C::invalido();

        if constexpr (Sign == signedness::signed_type)
        {
            // min() / -1 es el unico desbordamiento de la division entera: el
            // cociente seria -min(), que no es representable. Se marca dejando
            // dentro el valor envuelto, que es lo que ADR-009 pide conservar.
            if (wa == W::min() && wb == -W::one())
                return C::invalido(con_comprobacion(W::divmod(wa, wb).first));
        }
        return con_comprobacion(W::divmod(wa, wb).first) + heredada;
    }

    /// @brief Suma saturada: se pega a `max()` o a `min()` en vez de envolver.
    /// @param a Primer sumando.
    /// @param b Segundo sumando.
    /// @return La suma, o el extremo hacia el que se desbordo, **con la misma
    ///         politica que los operandos**.
    /// @note Con `checked`, saturar **no limpia** una marca previa. Ver la nota
    ///       larga al principio de esta seccion.
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
    saturating_add(const fixed_int_t<N, Sign, Form, Policy> &a,
                   const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        using W = fixed_int_t<N, Sign, Form, overflow_policy::wrap>;
        const W wa{a};
        const W wb{b};
        const auto r = checked_add(wa, wb);
        W valor{};
        if (r.valid())
            valor = descartar_marca(r);
        else if constexpr (Sign == signedness::unsigned_type)
            valor = W::max(); // sin signo solo se puede desbordar por arriba
        else
            valor = wa.is_negative() ? W::min() : W::max();
        return detail::con_marca_heredada(valor, a, b);
    }

    /// @brief Resta saturada.
    /// @param a Minuendo.
    /// @param b Sustraendo.
    /// @return La diferencia, o el extremo hacia el que se desbordo.
    /// @note Con `checked`, saturar no limpia una marca previa.
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
    saturating_sub(const fixed_int_t<N, Sign, Form, Policy> &a,
                   const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        using W = fixed_int_t<N, Sign, Form, overflow_policy::wrap>;
        const W wa{a};
        const W wb{b};
        const auto r = checked_sub(wa, wb);
        W valor{};
        if (r.valid())
            valor = descartar_marca(r);
        else if constexpr (Sign == signedness::unsigned_type)
            valor = W{}; // sin signo solo se puede desbordar por abajo: cero
        else
            valor = wa.is_negative() ? W::min() : W::max();
        return detail::con_marca_heredada(valor, a, b);
    }

    /// @brief Producto saturado.
    /// @param a Primer factor.
    /// @param b Segundo factor.
    /// @return El producto, o el extremo hacia el que se desbordo.
    /// @note Con `checked`, saturar no limpia una marca previa.
    template <std::size_t N, signedness Sign, representation_form Form,
              overflow_policy Policy = overflow_policy::wrap>
    [[nodiscard]] constexpr fixed_int_t<N, Sign, Form, Policy>
    saturating_mul(const fixed_int_t<N, Sign, Form, Policy> &a,
                   const fixed_int_t<N, Sign, Form, Policy> &b) noexcept
    {
        using W = fixed_int_t<N, Sign, Form, overflow_policy::wrap>;
        const W wa{a};
        const W wb{b};
        const auto r = checked_mul(wa, wb);
        W valor{};
        if (r.valid())
            valor = descartar_marca(r);
        else if constexpr (Sign == signedness::unsigned_type)
            valor = W::max();
        else
            valor = (wa.is_negative() != wb.is_negative()) ? W::min() : W::max();
        return detail::con_marca_heredada(valor, a, b);
    }

    /// @}

} // namespace nstd

#endif // FIXED_WIDTH_INT_T_HPP
