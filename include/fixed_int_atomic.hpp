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
// @file       fixed_int_atomic.hpp
// @brief      Acceso atomico a fixed_int_t, sin bloqueo cuando se puede
// @author     Julián Calderón Almendros
// @date       2026-09-18
// =============================================================================
//
// QUE SUSTITUYE, Y EN QUE SE APARTA
// ---------------------------------
// Es el tramo 2d de P1.5 (ver ADR-006): el reemplazo de
// `atomic_int128_param_t` de `int128_param_thread_safety.hpp`.
//
// Aquel usaba **un mutex siempre** y declaraba `is_always_lock_free() == false`
// fijo. Para un tipo de 128 bits escrito a mano eso era honesto. Con
// `fixed_int_t` ya no lo es: es **trivialmente copiable en las cuatro
// combinaciones**, y `fixed_int_t<1, ..., wrap>` mide 8 bytes, o sea lo mismo
// que un `std::uint64_t`. Cobrarle un mutex a eso es tirar rendimiento.
//
// Aquel tambien hacia `(void)order;` en cada operacion: el `memory_order` se
// aceptaba y se descartaba. Aqui se respeta.
//
// LA CONDICION PARA IR SIN BLOQUEO NO ES EL TAMANO
// ------------------------------------------------
// La tentacion es decir «si cabe en 16 bytes, `std::atomic` y listo». Es falso, y
// falla justo donde mas duele: **en el enlazado**.
//
// Medido el 18 sep 2026, compilando cada caso SIN `-latomic` y con las banderas
// del proyecto (`is_always_lock_free`, los cuatro compiladores):
//
//     tipo                  bytes   gcc   clang   intel   msvc
//     <1,u,binnat,wrap>       8     SI    SI      SI      SI
//     <1,s,c2,wrap>           8     SI    SI      SI      SI
//     <1,u,binnat,checked>   16     no    no      no      no
//     <2,u,binnat,wrap>      16     no    no      no      no
//     <2,u,binnat,checked>   24     no    no      no      no
//     <4,u,binnat,wrap>      32     no    no      no      no
//
// **Y esa tabla depende de las BANDERAS, no solo del compilador.** Con
// `-march=native` --que habilita `cmpxchg16b`-- clang pasa a decir que si en las
// dos filas de 16 bytes. Por eso aqui no se codifica ningun tamano: se pregunta.
//
// Lo que si es estable, y es lo unico que hace falta, es de donde viene el
// peligro. Compilando SIN `-latomic`:
//
//     gcc,   <1,checked> 16B ... **NO ENLAZA**: __atomic_load_16
//     gcc,   <2,wrap>    16B ... **NO ENLAZA**: __atomic_compare_exchange_16
//     gcc,   <4,wrap>    32B ... **NO ENLAZA**: __atomic_load
//     clang, los mismos      ... enlaza (libc++ lleva su propia tabla)
//
// `std::atomic<T>` de gcc llama a `__atomic_load_16` y compania en cuanto T pasa
// de 8 bytes, y eso vive en **libatomic**. Esta biblioteca es de solo cabeceras y
// no exige enlazar nada; adquirir esa dependencia sin darse cuenta seria romper
// su contrato para los usuarios de gcc, y el error saldria en el enlazador del
// usuario, no aqui.
//
// La primera version se apoyo en esta implicacion:
//
//     is_always_lock_free == true  ==>  enlaza sin libatomic
//
// medida en gcc y clang. **Con `icpx` es FALSA.** El 21 sep 2026 el CI de Linux
// murio al enlazar con
//
//     undefined reference to "__atomic_compare_exchange"
//
// para tipos que ese mismo compilador declaraba lock-free: dice que si y emite
// la llamada a la biblioteca igualmente.
//
// Por eso la condicion son ahora DOS cosas: que lo diga `is_always_lock_free`
// **y** que el tipo quepa en una palabra, que es lo unico que resuelven todos
// con una instruccion. Se pierde el caso de 16 bytes que clang hacia sin
// bloqueo con `-march=native`; gcc, MSVC e Intel ya decian que no. Vale mas no
// romperle el enlazado a nadie que ganar un caso que dependia de una bandera.
//
// La leccion, que en este proyecto ya va por la tercera: **una medida tomada en
// dos compiladores no es una ley para todos.**
//
// EL RELLENO NO ROMPE EL CAS, Y ESO TAMBIEN SE COMPROBO
// -----------------------------------------------------
// `fixed_int_t<1, ..., checked>` mide 16 bytes: 8 de limbo, 1 de marca y **7 de
// relleno**. `compare_exchange` compara la representacion de objeto, asi que con
// relleno indeterminado un bucle CAS podria no terminar NUNCA.
//
// C++20 (P0528) dice que el relleno no participa. Pero un cuelgue no se arregla
// citando el estandar, asi que se midio: cuatro hilos, 50 000 `fetch_add` por
// CAS cada uno, sobre los tipos con relleno y sin el. **Todos terminan y suman
// bien**, con 1,0 a 2,7 reintentos por exito -- el mismo orden que los tipos sin
// relleno.
//
// COMO SE COMPRUEBA DESDE FUERA
// -----------------------------
// `sin_bloqueo` es publico y `constexpr`, asi que un `static_assert` del usuario
// puede exigir que su tipo concreto vaya sin bloqueo en su plataforma. No se
// pone aqui como requisito porque depende del compilador, y fallar la
// compilacion por eso seria peor que el mutex.
// =============================================================================

#ifndef NSTD_FIXED_INT_ATOMIC_HPP
#define NSTD_FIXED_INT_ATOMIC_HPP

#include "fixed_width_int_t.hpp"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <type_traits>

namespace nstd
{
    /// @brief Acceso atomico a un `fixed_int_t`, sin bloqueo cuando la
    ///        plataforma lo permite sin arrastrar `libatomic`.
    ///
    /// @tparam N Numero de limbos. @tparam Sign Con o sin signo.
    /// @tparam Form Representacion. @tparam Policy Politica de desbordamiento.
    ///
    /// @note No es copiable ni movible, como `std::atomic`.
    /// @note Las operaciones de lectura-modificacion-escritura (`fetch_add` y
    ///       companeras, `++`, `+=`...) se hacen con un **bucle CAS**: leen,
    ///       calculan y reintentan si otro hilo se adelanto. El calculo usa la
    ///       aritmetica normal del tipo, asi que **respeta su politica de
    ///       desbordamiento**: con `checked`, la marca viaja dentro del valor y
    ///       se publica con el.
    template <std::size_t N, signedness Sign = signedness::unsigned_type,
              representation_form Form = representation_form::binnat,
              overflow_policy Policy = overflow_policy::wrap>
    class atomic_fixed_int_t
    {
    public:
        /// @brief El tipo que se guarda y se devuelve.
        using value_type = fixed_int_t<N, Sign, Form, Policy>;

        /// @brief Si esta instancia va **sin bloqueo** en esta plataforma.
        ///
        /// Son DOS condiciones, y la segunda esta aqui porque la primera no
        /// basto. Es publica a proposito, para que el usuario pueda poner un
        /// `static_assert` sobre su tipo concreto.
        ///
        /// POR QUE NO VALE `is_always_lock_free` A SECAS
        /// ---------------------------------------------
        /// La version del 18 sep 2026 usaba solo `is_always_lock_free`, apoyada
        /// en esta premisa:
        ///
        ///     is_always_lock_free == true  ==>  enlaza sin libatomic
        ///
        /// Se comprobo en **gcc y clang** y se dio por buena para todos. **Con
        /// `icpx` es falsa**: en el CI de Linux, el 21 sep 2026, el enlazado
        /// murio con
        ///
        ///     undefined reference to "__atomic_compare_exchange"
        ///     icpx: error: linker command failed with exit code 1
        ///
        /// para tipos que ese mismo compilador declara lock-free. Reporta que si
        /// y luego emite la llamada a la biblioteca igualmente.
        ///
        /// Asi que se exige ademas que el tipo **quepa en una palabra**, que es
        /// lo unico que todos los compiladores resuelven con una instruccion, sin
        /// ayuda de libatomic. Lo que se pierde con eso son los 16 bytes que
        /// clang hacia sin bloqueo **solo con `-march=native`** (o sea `-mcx16`);
        /// gcc, MSVC e Intel ya decian que no. Se cambia un caso que dependia de
        /// una bandera por no romperle el enlazado a nadie.
        ///
        /// @note Es la tercera vez en este proyecto que una medida tomada en dos
        ///       compiladores se generaliza a todos y sale mal. Aqui el aviso lo
        ///       dio el CI, y **solo despues de arreglarlo para que imprimiera el
        ///       error**: antes decia «FAIL (compile)» y nada mas.
        static constexpr bool sin_bloqueo =
            std::atomic<value_type>::is_always_lock_free && sizeof(value_type) <= sizeof(void *);

    private:
        /// El camino sin bloqueo: `std::atomic` directo.
        struct almacen_atomico
        {
            std::atomic<value_type> v;

            constexpr explicit almacen_atomico(const value_type &x) noexcept : v{x} {}

            [[nodiscard]] value_type carga(std::memory_order o) const noexcept { return v.load(o); }
            void guarda(const value_type &x, std::memory_order o) noexcept { v.store(x, o); }
            [[nodiscard]] value_type intercambia(const value_type &x, std::memory_order o) noexcept
            {
                return v.exchange(x, o);
            }
            [[nodiscard]] bool cas_fuerte(value_type &esperado, const value_type &deseado,
                                          std::memory_order exito, std::memory_order fallo) noexcept
            {
                return v.compare_exchange_strong(esperado, deseado, exito, fallo);
            }
            [[nodiscard]] bool cas_debil(value_type &esperado, const value_type &deseado,
                                         std::memory_order exito, std::memory_order fallo) noexcept
            {
                return v.compare_exchange_weak(esperado, deseado, exito, fallo);
            }
            [[nodiscard]] bool sin_bloqueo_ahora() const noexcept { return v.is_lock_free(); }
        };

        /// El camino con bloqueo, para los tipos que no caben en una instruccion.
        ///
        /// El `memory_order` se ignora **y eso es correcto**: tomar y soltar el
        /// mutex da adquisicion y liberacion, que es mas fuerte que cualquier
        /// orden que se pida. Lo que no seria correcto es aceptarlo y no dar ni
        /// eso, que es lo que hacia el envoltorio viejo al no tener mutex en
        /// algunas rutas.
        struct almacen_con_mutex
        {
            mutable std::mutex m;
            value_type v;

            constexpr explicit almacen_con_mutex(const value_type &x) noexcept : m{}, v{x} {}

            [[nodiscard]] value_type carga(std::memory_order) const noexcept
            {
                const std::lock_guard<std::mutex> cerrojo{m};
                return v;
            }
            void guarda(const value_type &x, std::memory_order) noexcept
            {
                const std::lock_guard<std::mutex> cerrojo{m};
                v = x;
            }
            [[nodiscard]] value_type intercambia(const value_type &x, std::memory_order) noexcept
            {
                const std::lock_guard<std::mutex> cerrojo{m};
                const value_type viejo = v;
                v = x;
                return viejo;
            }
            [[nodiscard]] bool cas_fuerte(value_type &esperado, const value_type &deseado, std::memory_order,
                                          std::memory_order) noexcept
            {
                const std::lock_guard<std::mutex> cerrojo{m};
                if (v == esperado)
                {
                    v = deseado;
                    return true;
                }
                esperado = v;
                return false;
            }
            [[nodiscard]] bool cas_debil(value_type &esperado, const value_type &deseado,
                                         std::memory_order exito, std::memory_order fallo) noexcept
            {
                // Con mutex no hay fallo espurio que ahorrar: el debil ES el fuerte.
                return cas_fuerte(esperado, deseado, exito, fallo);
            }
            [[nodiscard]] bool sin_bloqueo_ahora() const noexcept { return false; }
        };

        using almacen_t = std::conditional_t<sin_bloqueo, almacen_atomico, almacen_con_mutex>;
        almacen_t a_;

        /// @brief El bucle CAS que comparten todas las lecturas-modificaciones.
        ///
        /// @param op Funcion que recibe el valor leido y devuelve el que se quiere
        ///        dejar. Puede llamarse **mas de una vez** si hay contienda, asi
        ///        que tiene que ser pura.
        /// @return El valor **anterior**, como manda `fetch_*`.
        template <typename Op>
        [[nodiscard]] value_type lee_modifica_escribe(Op op, std::memory_order orden) noexcept
        {
            const std::memory_order orden_fallo = orden_de_fallo(orden);
            value_type esperado = a_.carga(std::memory_order_relaxed);
            while (!a_.cas_debil(esperado, op(esperado), orden, orden_fallo))
            {
                // `cas_debil` deja en `esperado` lo que habia: se reintenta con eso.
            }
            return esperado;
        }

        /// El orden de fallo no puede ser mas fuerte que el de exito, ni de
        /// liberacion: el estandar lo prohibe y algunas implementaciones abortan.
        [[nodiscard]] static constexpr std::memory_order orden_de_fallo(std::memory_order o) noexcept
        {
            switch (o)
            {
                case std::memory_order_release:
                    return std::memory_order_relaxed;
                case std::memory_order_acq_rel:
                    return std::memory_order_acquire;
                default:
                    return o;
            }
        }

    public:
        // ------------------------------------------------------ construccion ---

        constexpr atomic_fixed_int_t() noexcept : a_{value_type{}} {}
        /// @brief Construye con un valor inicial.
        /// @param val El valor de partida.
        constexpr explicit atomic_fixed_int_t(const value_type &val) noexcept : a_{val} {}

        atomic_fixed_int_t(const atomic_fixed_int_t &) = delete;
        atomic_fixed_int_t &operator=(const atomic_fixed_int_t &) = delete;
        atomic_fixed_int_t(atomic_fixed_int_t &&) = delete;
        atomic_fixed_int_t &operator=(atomic_fixed_int_t &&) = delete;

        // ------------------------------------------------------- lo esencial ---

        /// @brief Lee el valor.
        /// @param orden Orden de memoria. @return El valor actual.
        [[nodiscard]] value_type load(std::memory_order orden = std::memory_order_seq_cst) const noexcept
        {
            return a_.carga(orden);
        }

        /// @brief Escribe el valor.
        /// @param val El valor nuevo. @param orden Orden de memoria.
        void store(const value_type &val, std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            a_.guarda(val, orden);
        }

        /// @brief Escribe y devuelve lo que habia, en un solo paso.
        /// @param val El valor nuevo. @param orden Orden de memoria.
        /// @return El valor **anterior**.
        [[nodiscard]] value_type exchange(const value_type &val,
                                          std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return a_.intercambia(val, orden);
        }

        /// @brief Escribe `deseado` **solo si** el valor actual es `esperado`.
        /// @param esperado Lo que se cree que hay. Si no coincide, se le deja
        ///        dentro lo que habia de verdad, listo para reintentar.
        /// @param deseado Lo que se quiere dejar.
        /// @param exito Orden de memoria si acierta.
        /// @param fallo Orden si falla. **No puede ser mas fuerte que `exito`.**
        /// @return `true` si escribio.
        [[nodiscard]] bool compare_exchange_strong(value_type &esperado, const value_type &deseado,
                                                   std::memory_order exito, std::memory_order fallo) noexcept
        {
            return a_.cas_fuerte(esperado, deseado, exito, fallo);
        }

        /// @brief Escribe `deseado` **solo si** el valor actual es `esperado`.
        /// @param esperado Lo que se cree que hay. Si no coincide, se le deja
        ///        dentro lo que habia de verdad, listo para reintentar.
        /// @param deseado Lo que se quiere dejar.
        /// @param orden Orden de memoria; la de fallo se deriva de ella.
        /// @return `true` si escribio.
        [[nodiscard]] bool
        compare_exchange_strong(value_type &esperado, const value_type &deseado,
                                std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return a_.cas_fuerte(esperado, deseado, orden, orden_de_fallo(orden));
        }

        /// @brief Escribe `deseado` **solo si** el valor actual es `esperado`.
        /// @param esperado Lo que se cree que hay. Si no coincide, se le deja
        ///        dentro lo que habia de verdad, listo para reintentar.
        /// @param deseado Lo que se quiere dejar.
        /// @note Puede fallar **espuriamente**: usar siempre dentro de un bucle.
        /// @param exito Orden de memoria si acierta.
        /// @param fallo Orden si falla. **No puede ser mas fuerte que `exito`.**
        /// @return `true` si escribio.
        [[nodiscard]] bool compare_exchange_weak(value_type &esperado, const value_type &deseado,
                                                 std::memory_order exito, std::memory_order fallo) noexcept
        {
            return a_.cas_debil(esperado, deseado, exito, fallo);
        }

        /// @brief Escribe `deseado` **solo si** el valor actual es `esperado`.
        /// @param esperado Lo que se cree que hay. Si no coincide, se le deja
        ///        dentro lo que habia de verdad, listo para reintentar.
        /// @param deseado Lo que se quiere dejar.
        /// @note Puede fallar **espuriamente**: usar siempre dentro de un bucle.
        /// @param orden Orden de memoria; la de fallo se deriva de ella.
        /// @return `true` si escribio.
        [[nodiscard]] bool compare_exchange_weak(value_type &esperado, const value_type &deseado,
                                                 std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return a_.cas_debil(esperado, deseado, orden, orden_de_fallo(orden));
        }

        // ------------------------------------- lectura-modificacion-escritura ---

        /// @brief Suma `val`, por bucle CAS.
        /// @param val El operando. @param orden Orden de memoria.
        /// @return El valor **anterior**, como manda `<atomic>`.
        value_type fetch_add(const value_type &val,
                             std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return lee_modifica_escribe([&](const value_type &x) { return x + val; }, orden);
        }

        /// @brief Resta `val`, por bucle CAS.
        /// @param val El operando. @param orden Orden de memoria.
        /// @return El valor **anterior**, como manda `<atomic>`.
        value_type fetch_sub(const value_type &val,
                             std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return lee_modifica_escribe([&](const value_type &x) { return x - val; }, orden);
        }

        /// @brief Hace AND bit a bit con `val`, por bucle CAS.
        /// @param val El operando. @param orden Orden de memoria.
        /// @return El valor **anterior**, como manda `<atomic>`.
        value_type fetch_and(const value_type &val,
                             std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return lee_modifica_escribe([&](const value_type &x) { return x & val; }, orden);
        }

        /// @brief Hace OR bit a bit con `val`, por bucle CAS.
        /// @param val El operando. @param orden Orden de memoria.
        /// @return El valor **anterior**, como manda `<atomic>`.
        value_type fetch_or(const value_type &val,
                            std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return lee_modifica_escribe([&](const value_type &x) { return x | val; }, orden);
        }

        /// @brief Hace XOR bit a bit con `val`, por bucle CAS.
        /// @param val El operando. @param orden Orden de memoria.
        /// @return El valor **anterior**, como manda `<atomic>`.
        value_type fetch_xor(const value_type &val,
                             std::memory_order orden = std::memory_order_seq_cst) noexcept
        {
            return lee_modifica_escribe([&](const value_type &x) { return x ^ val; }, orden);
        }

        // ----------------------------------------------------------- azucar ---
        //
        // Devuelven valores, no referencias, porque un `atomic` no puede prestar
        // su interior: entre devolver la referencia y usarla, otro hilo escribe.

        /// @brief Escribe el valor. @param val Lo que se escribe.
        /// @return El valor escrito, **no una referencia**: ver la nota de abajo.
        value_type operator=(const value_type &val) noexcept
        {
            store(val);
            return val;
        }

        /// @brief Lee el valor, como `load()` con el orden por omision.
        [[nodiscard]] operator value_type() const noexcept { return load(); }

        /// @brief Incrementa. @return El valor **nuevo**.
        value_type operator++() noexcept { return fetch_add(value_type{1}) + value_type{1}; }
        /// @brief Incrementa. @return El valor **anterior**.
        value_type operator++(int) noexcept { return fetch_add(value_type{1}); }
        /// @brief Decrementa. @return El valor **nuevo**.
        value_type operator--() noexcept { return fetch_sub(value_type{1}) - value_type{1}; }
        /// @brief Decrementa. @return El valor **anterior**.
        value_type operator--(int) noexcept { return fetch_sub(value_type{1}); }

        /// @brief Suma en su sitio. @param val El operando.
        /// @return El valor **nuevo**.
        value_type operator+=(const value_type &val) noexcept { return fetch_add(val) + val; }
        /// @brief Resta en su sitio. @param val El operando.
        /// @return El valor **nuevo**.
        value_type operator-=(const value_type &val) noexcept { return fetch_sub(val) - val; }
        /// @brief AND bit a bit en su sitio. @param val El operando.
        /// @return El valor **nuevo**.
        value_type operator&=(const value_type &val) noexcept { return fetch_and(val) & val; }
        /// @brief OR bit a bit en su sitio. @param val El operando.
        /// @return El valor **nuevo**.
        value_type operator|=(const value_type &val) noexcept { return fetch_or(val) | val; }
        /// @brief XOR bit a bit en su sitio. @param val El operando.
        /// @return El valor **nuevo**.
        value_type operator^=(const value_type &val) noexcept { return fetch_xor(val) ^ val; }

        // ------------------------------------------------------ que soy yo ---

        /// @brief Si **esta** instancia va sin bloqueo. Lo dice de verdad: no es
        ///        un `return false` fijo como en el envoltorio viejo.
        [[nodiscard]] bool is_lock_free() const noexcept { return a_.sin_bloqueo_ahora(); }

        /// @brief Si **todas** las instancias de este tipo van sin bloqueo.
        static constexpr bool is_always_lock_free = sin_bloqueo;
    };

    // =====================================================================
    // Las funciones libres, al estilo de <atomic>
    // =====================================================================

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] inline fixed_int_t<N, Sign, Form, Policy>
    atomic_load(const atomic_fixed_int_t<N, Sign, Form, Policy> *obj) noexcept
    {
        return obj->load();
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] inline fixed_int_t<N, Sign, Form, Policy>
    atomic_load_explicit(const atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                         std::memory_order orden) noexcept
    {
        return obj->load(orden);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    inline void atomic_store(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                             const fixed_int_t<N, Sign, Form, Policy> &val) noexcept
    {
        obj->store(val);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    inline void atomic_store_explicit(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                                      const fixed_int_t<N, Sign, Form, Policy> &val,
                                      std::memory_order orden) noexcept
    {
        obj->store(val, orden);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] inline fixed_int_t<N, Sign, Form, Policy>
    atomic_exchange(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                    const fixed_int_t<N, Sign, Form, Policy> &val) noexcept
    {
        return obj->exchange(val);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] inline bool
    atomic_compare_exchange_strong(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                                   fixed_int_t<N, Sign, Form, Policy> *esperado,
                                   const fixed_int_t<N, Sign, Form, Policy> &deseado) noexcept
    {
        return obj->compare_exchange_strong(*esperado, deseado);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    [[nodiscard]] inline bool
    atomic_compare_exchange_weak(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                                 fixed_int_t<N, Sign, Form, Policy> *esperado,
                                 const fixed_int_t<N, Sign, Form, Policy> &deseado) noexcept
    {
        return obj->compare_exchange_weak(*esperado, deseado);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    inline fixed_int_t<N, Sign, Form, Policy>
    atomic_fetch_add(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                     const fixed_int_t<N, Sign, Form, Policy> &val) noexcept
    {
        return obj->fetch_add(val);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    inline fixed_int_t<N, Sign, Form, Policy>
    atomic_fetch_sub(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                     const fixed_int_t<N, Sign, Form, Policy> &val) noexcept
    {
        return obj->fetch_sub(val);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    inline fixed_int_t<N, Sign, Form, Policy>
    atomic_fetch_and(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                     const fixed_int_t<N, Sign, Form, Policy> &val) noexcept
    {
        return obj->fetch_and(val);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    inline fixed_int_t<N, Sign, Form, Policy>
    atomic_fetch_or(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                    const fixed_int_t<N, Sign, Form, Policy> &val) noexcept
    {
        return obj->fetch_or(val);
    }

    template <std::size_t N, signedness Sign, representation_form Form, overflow_policy Policy>
    inline fixed_int_t<N, Sign, Form, Policy>
    atomic_fetch_xor(atomic_fixed_int_t<N, Sign, Form, Policy> *obj,
                     const fixed_int_t<N, Sign, Form, Policy> &val) noexcept
    {
        return obj->fetch_xor(val);
    }

    // =====================================================================
    // Alias de conveniencia, en paralelo a los de `fixed_width_int_t.hpp`
    // =====================================================================

    /// @brief El unico que va **sin bloqueo en los cuatro compiladores**, y con
    ///        las banderas del proyecto: mide 8 bytes, igual que `std::uint64_t`.
    using atomic_uint64_fixed_t =
        atomic_fixed_int_t<1, signedness::unsigned_type, representation_form::binnat, overflow_policy::wrap>;

    using atomic_int64_fixed_t =
        atomic_fixed_int_t<1, signedness::signed_type, representation_form::twos_complement,
                           overflow_policy::wrap>;

    /// @note Mide 16 bytes: **con mutex en los cuatro compiladores** salvo que se
    ///       compile con `-march=native` (o `-mcx16`), y entonces clang lo hace
    ///       sin bloqueo con `cmpxchg16b`. `sin_bloqueo` lo dice para la
    ///       compilacion concreta; no hay que adivinarlo.
    using atomic_uint128_fixed_t =
        atomic_fixed_int_t<2, signedness::unsigned_type, representation_form::binnat, overflow_policy::wrap>;

    using atomic_int128_fixed_t =
        atomic_fixed_int_t<2, signedness::signed_type, representation_form::twos_complement,
                           overflow_policy::wrap>;

} // namespace nstd

#endif // NSTD_FIXED_INT_ATOMIC_HPP
