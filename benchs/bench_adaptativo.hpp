// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julian Calderon Almendros
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
// @file       bench_adaptativo.hpp
// @brief      Arnes de medicion con iteraciones adaptativas y dispersion
// @date       2026-09-10
// =============================================================================
//
// DOS PROBLEMAS DEL ARNES DE ANTES, Y LOS DOS SE ARREGLAN AQUI.
//
// 1. EL NUMERO DE ITERACIONES ERA FIJO. Con 50.000 iteraciones x 5 rondas, una
//    casilla de N=64 tarda un instante y una de N=4096 tarda 17 minutos, porque
//    el coste de la operacion crece con N y el numero de repeticiones no baja.
//    La rejilla de la sesion de medicion se iria a dias.
//
//    Aqui se fija el TIEMPO por casilla --200 ms por defecto-- y se deduce el
//    numero de iteraciones con una pasada de calibracion que dobla hasta llegar.
//    Una casilla cuesta lo mismo en N=8 que en N=2048.
//
// 2. SE MEDIA UNA VEZ Y SE PUBLICABA EL NUMERO. De ahi salieron dos cifras
//    contradictorias para lo mismo --1,11x y 2,44x para el desenrollado en
//    N=24-- sin forma de saber cual era. Aqui **cada casilla se mide diez veces
//    como minimo y se publica la dispersion**: sin ella no se puede saber que
//    diferencias son reales.
//
// Y UNA COSA QUE ANTES NO SE PODIA HACER: RONDAS ENTRELAZADAS.
//
// Comparar dos variantes midiendo primero todas las vueltas de una y luego
// todas las de la otra deja que la deriva termica, el escalado de frecuencia y
// la ocupacion de la cache se repartan de forma desigual entre ellas. Lo
// correcto es alternar: una vuelta de cada, y ademas EN ORDEN DISTINTO cada
// ronda, para que la posicion tampoco favorezca a ninguna.
//
// Requiere que las variantes existan a la vez en el mismo binario, que es lo
// que `include/algorithms/mul_kernels.hpp` vino a permitir.
// =============================================================================

#ifndef BENCH_ADAPTATIVO_HPP
#define BENCH_ADAPTATIVO_HPP

#include "bench_common.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <utility>
#include <vector>

namespace bench
{

    /// @brief Cuanto tiempo se quiere gastar en cada casilla. 200 ms es
    ///        suficiente para que el reloj sea estable y poco para que la
    ///        rejilla entera quepa en minutos.
    inline constexpr double MS_POR_CASILLA = 200.0;

    /// @brief Repeticiones por casilla. **Diez es el minimo del protocolo**, no
    ///        una sugerencia: una casilla sin dispersion no es una medida.
    ///
    /// **25 desde el 27 sep 2026**, para las tomas profundas del historico. Diez
    /// sigue siendo el minimo aceptable; veinticinco es lo que se paga cuando la
    /// medida se va a guardar y comparar durante meses.
    ///
    /// @warning **Cambiar esto cambia el REGIMEN de medida, no solo su coste**, y
    ///          esta MEDIDO: el mismo binario de `cuadrado` compilado con 10 y con
    ///          25 vueltas, alternando las tandas, da
    ///
    ///              minimo(25) / minimo(10)   mediana +3,4 %  (rango -6,8 a +8,1)
    ///              suelo (25) / suelo (10)   mediana +5,5 %  (rango -3,9 a +11,6)
    ///              vueltas limpias           15 % con 10, 8 % con 25
    ///
    ///          O sea que la cifra **SUBE**, al contrario de lo que dice la teoria
    ///          del estadistico de orden: una tanda de 25 vueltas dura 2,5 veces
    ///          mas y la maquina mide mas caliente, y el efecto termico se come al
    ///          del muestreo. Dos tomas con distinto numero de repeticiones **no
    ///          son comparables**, y `bench_history.py --compare` avisa cuando
    ///          difieren.
    ///
    /// @note El coste crece lineal: cada casilla son `REPETICIONES` x
    ///       `MS_POR_CASILLA` por variante. De 10 a 25 la sesion se multiplica
    ///       por 2,5 en los doce benchmarks de este arnes.
    /// @note Anulable con `-DBENCH_REPETICIONES=N`, igual que `BENCH_ITERATIONS`.
    ///       Existe para poder **medir** el sesgo por regimen --el mismo binario
    ///       con 10 vueltas y con 25-- en vez de suponerlo.
#ifndef BENCH_REPETICIONES
#define BENCH_REPETICIONES 25
#endif
    inline constexpr std::size_t REPETICIONES = BENCH_REPETICIONES;

    /// @brief Que fraccion de las vueltas forma «la cola baja».
    ///
    /// **Un cuantil fijo, no un numero fijo de vueltas, y esa es la clave.** El
    /// minimo de `n` muestras estima el cuantil `1/(n+1)`: con 10 vueltas apunta
    /// al 9 % y con 25 al 3,8 %, asi que **cambiar las repeticiones cambia la
    /// cifra publicada** sin que nada haya mejorado. La media del 20 % mas bajo
    /// apunta al mismo sitio con 10 vueltas y con 25, y por tanto **si se puede
    /// comparar entre regimenes**.
    inline constexpr double FRACCION_SUELO = 0.20;

    /// @brief Cuanto puede separarse una vuelta del minimo y seguir contando
    ///        como «limpia». Provisional: hay que calibrarlo como se calibro el
    ///        suelo de `bench_history.py`, con dos tomas del mismo codigo.
    inline constexpr double TOLERANCIA_LIMPIA = 0.02;

    /// @brief Lo que sale de medir una casilla.
    struct Medida
    {
        double minimo{0};           ///< cyc/op. Es la cifra que se publica.
        double suelo{0};            ///< cyc/op. Media de la cola baja; **comparable entre n**.
        double dispersion_baja{0};  ///< (suelo - minimo)/minimo: lo que se mueve la cola BAJA.
        double limpias{0};          ///< fraccion de vueltas dentro de `TOLERANCIA_LIMPIA` del minimo.
        std::size_t k_suelo{0};     ///< cuantas vueltas entraron en la cola baja.
        double mediana{0};          ///< cyc/op.
        double media{0};            ///< cyc/op.
        double desviacion{0};       ///< cyc/op, tipica muestral.
        double dispersion{0};       ///< desviacion/media, en tanto por uno.
        double techo{0};            ///< el peor de los repetidos.
        std::size_t iteraciones{0}; ///< las que decidio la calibracion.
        std::size_t repeticiones{0};

        /// @brief Cuanto se separa el maximo del minimo, en tanto por uno.
        ///        Es la cifra honesta para decir "este banco tiene X% de ruido".
        [[nodiscard]] double recorrido() const noexcept
        {
            return minimo > 0 ? (techo - minimo) / minimo : 0.0;
        }
    };

    /// @brief Calibra: cuantas iteraciones hacen falta para gastar `objetivo_ms`.
    ///
    /// Dobla desde 1 hasta pasarse, midiendo con el reloj de pared --no con
    /// RDTSC-- porque lo que se quiere acotar es el TIEMPO que dura la sesion,
    /// y RDTSC cuenta a frecuencia invariante, no a la real.
    ///
    /// @param op Lo que se mide. Recibe el indice de la vuelta.
    /// @param objetivo_ms Milisegundos que se quieren gastar.
    /// @return Numero de iteraciones, al menos 1.
    template <typename F>
    std::size_t calibra(F op, double objetivo_ms = MS_POR_CASILLA)
    {
        using reloj = std::chrono::steady_clock;
        std::size_t n = 1;
        for (int intento = 0; intento < 40; ++intento)
        {
            const auto t0 = reloj::now();
            for (std::size_t k = 0; k < n; ++k)
                op(k);
            const double ms = std::chrono::duration<double, std::milli>(reloj::now() - t0).count();

            if (ms >= objetivo_ms)
                return n;

            // Si la medida es demasiado corta para fiarse del reloj, dobla a
            // ciegas; si ya se puede extrapolar, salta directamente.
            if (ms < 1.0)
                n *= 4;
            else
            {
                const double factor = objetivo_ms / ms;
                const auto siguiente = static_cast<std::size_t>(static_cast<double>(n) * factor * 1.1) + 1;
                n = siguiente > n ? siguiente : n * 2;
            }
        }
        return n;
    }

    /// @brief Estadistica de una tanda de medidas en cyc/op.
    inline Medida resume(std::vector<double> v, std::size_t iteraciones)
    {
        Medida m{};
        if (v.empty())
            return m;
        std::sort(v.begin(), v.end());
        m.minimo = v.front();
        m.techo = v.back();
        m.mediana = v[v.size() / 2];
        double suma = 0.0;
        for (const double x : v)
            suma += x;
        m.media = suma / static_cast<double>(v.size());
        double s2 = 0.0;
        for (const double x : v)
            s2 += (x - m.media) * (x - m.media);
        m.desviacion = v.size() > 1 ? std::sqrt(s2 / static_cast<double>(v.size() - 1)) : 0.0;
        m.dispersion = m.media > 0 ? m.desviacion / m.media : 0.0;
        m.iteraciones = iteraciones;
        m.repeticiones = v.size();

        // LA COLA BAJA. `v` ya esta ordenado.
        //
        // Dos como minimo aunque el cuantil pida menos: con una sola vuelta
        // `suelo == minimo` y el campo no aportaria nada.
        std::size_t k = static_cast<std::size_t>(static_cast<double>(v.size()) * FRACCION_SUELO + 0.5);
        if (k < 2)
            k = 2;
        if (k > v.size())
            k = v.size();
        double suma_baja = 0.0;
        for (std::size_t i = 0; i < k; ++i)
            suma_baja += v[i];
        m.k_suelo = k;
        m.suelo = suma_baja / static_cast<double>(k);
        m.dispersion_baja = m.minimo > 0 ? (m.suelo - m.minimo) / m.minimo : 0.0;

        // Vueltas «limpias»: las que caen a un pelo del minimo. Es el indicador
        // de si el suelo esta BIEN DETERMINADO -- veinte de veinticinco pegadas
        // al minimo dicen que la maquina dejo medir; una sola dice que ese
        // minimo fue un golpe de suerte y que no conviene fiarse de el.
        const double techo_limpio = m.minimo * (1.0 + TOLERANCIA_LIMPIA);
        std::size_t n_limpias = 0;
        for (const double x : v)
            if (x <= techo_limpio)
                ++n_limpias;
        m.limpias = static_cast<double>(n_limpias) / static_cast<double>(v.size());

        return m;
    }

    /// @brief Mide UNA variante, con calibracion y repeticiones.
    ///
    /// @param op Lo que se mide.
    /// @param reps Repeticiones; el protocolo pide diez como minimo.
    /// @param objetivo_ms Tiempo por repeticion.
    template <typename F>
    Medida mide_una(F op, std::size_t reps = REPETICIONES, double objetivo_ms = MS_POR_CASILLA)
    {
        const std::size_t n = calibra(op, objetivo_ms);

        // Calentamiento que se descarta: la primera vuelta paga los fallos de
        // cache y la subida de frecuencia.
        for (std::size_t k = 0; k < n / 4 + 1; ++k)
            op(k);

        std::vector<double> v;
        v.reserve(reps);
        for (std::size_t r = 0; r < reps; ++r)
        {
            CycleTimer t;
            for (std::size_t k = 0; k < n; ++k)
                op(k);
            v.push_back(static_cast<double>(t.elapsed_cycles()) / static_cast<double>(n));
        }
        return resume(std::move(v), n);
    }

    /// @brief Mide varias variantes **entrelazadas y en orden cambiante**.
    ///
    /// En cada ronda se ejecutan todas, y el orden **rota una posicion por
    /// ronda**. Asi ninguna ocupa siempre el mismo sitio, y la deriva termica y
    /// el escalado de frecuencia se reparten por igual entre ellas.
    ///
    /// Es la regla 4 del protocolo, y hasta ahora era imposible de cumplir:
    /// requiere que las variantes existan A LA VEZ en el mismo binario.
    ///
    /// @param ops `std::tuple` de invocables, cada uno recibe el indice de la
    ///        vuelta.
    /// @param reps Repeticiones por variante; diez es el minimo del protocolo.
    /// @param objetivo_ms Tiempo por repeticion y variante.
    /// @return Una `Medida` por variante, en el orden en que se pasaron.
    template <typename... Fs>
    auto mide_entrelazado(std::tuple<Fs...> ops, std::size_t reps = REPETICIONES,
                          double objetivo_ms = MS_POR_CASILLA)
    {
        constexpr std::size_t K = sizeof...(Fs);
        static_assert(K >= 1, "hace falta al menos una variante");

        // La calibracion se hace UNA vez por variante y se queda fija: si el
        // numero de iteraciones cambiara entre rondas, las rondas no serian
        // comparables entre si.
        std::array<std::size_t, K> n{};
        [&]<std::size_t... I>(std::index_sequence<I...>)
        { ((n[I] = calibra(std::get<I>(ops), objetivo_ms)), ...); }(std::make_index_sequence<K>{});

        std::array<std::vector<double>, K> v{};
        for (std::size_t i = 0; i < K; ++i)
            v[i].reserve(reps);

        // Calentamiento de todas antes de contar nada: la primera vuelta paga
        // los fallos de cache y la subida de frecuencia.
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (
                [&]
                {
                    for (std::size_t k = 0; k < n[I] / 4 + 1; ++k)
                        std::get<I>(ops)(k);
                }(),
                ...);
        }(std::make_index_sequence<K>{});

        for (std::size_t r = 0; r < reps; ++r)
        {
            for (std::size_t paso = 0; paso < K; ++paso)
            {
                const std::size_t cual = (paso + r) % K; // el orden rota
                [&]<std::size_t... I>(std::index_sequence<I...>)
                {
                    (
                        [&]
                        {
                            if (I != cual)
                                return;
                            CycleTimer t;
                            for (std::size_t k = 0; k < n[I]; ++k)
                                std::get<I>(ops)(k);
                            v[I].push_back(static_cast<double>(t.elapsed_cycles()) /
                                           static_cast<double>(n[I]));
                        }(),
                        ...);
                }(std::make_index_sequence<K>{});
            }
        }

        std::array<Medida, K> salida{};
        for (std::size_t i = 0; i < K; ++i)
            salida[i] = resume(std::move(v[i]), n[i]);
        return salida;
    }

    /// @brief A partir de que recorrido se marca una casilla como ruidosa.
    ///
    /// **Calibrado midiendo, el 10 sep 2026**, no elegido a ojo. En esta
    /// maquina el recorrido dentro de una ejecucion cae entre el 9 % y el 58 %
    /// **en todas las casillas**, asi que un umbral del 10 % marca todo y no
    /// informa de nada. El 25 % deja pasar lo normal y senala lo que se sale.
    ///
    /// @note Lo que de verdad decide si una comparacion vale no es este numero
    ///       sino si la DIFERENCIA entre dos variantes supera la suma de sus
    ///       recorridos. Eso lo comprueba quien compara, no esta funcion.
    inline constexpr double RECORRIDO_RUIDOSO = 0.25;

    /// @brief Imprime una medida con su dispersion.
    ///
    /// La dispersion **no es decorativa**: es lo que dice si una diferencia
    /// entre dos casillas significa algo.
    inline void imprime(const char *etiqueta, const Medida &m)
    {
        const double rec = m.recorrido() * 100.0;
        // `limpias` dice si el minimo esta bien determinado o fue suerte, y es
        // mas util de un vistazo que la desviacion sobre la media, que se la
        // come la cola de arriba.
        std::printf("  %-34s %10.1f cyc/op   suelo %8.1f (+%4.1f%%)  limpias %3.0f%%  "
                    "recorrido %5.1f%%%s  (%zu it x %zu)\n",
                    etiqueta, m.minimo, m.suelo, m.dispersion_baja * 100.0, m.limpias * 100.0, rec,
                    rec > RECORRIDO_RUIDOSO * 100.0 ? " <-RUIDOSA" : "  ", m.iteraciones, m.repeticiones);
    }

    /// @brief Deja la medida en el historico **con su ruido**.
    ///
    /// La alternativa --`bench_record(etiqueta, m.minimo)`-- guarda el numero y
    /// tira lo unico que permite saber si manana significa algo. Quien compara
    /// dos ejecuciones necesita el recorrido de las dos: es la comprobacion que
    /// menciona el @note de `RECORRIDO_RUIDOSO` y que nadie podia hacer porque
    /// el dato no llegaba al fichero.
    ///
    /// @param caso Etiqueta. Que diga la OPERACION y no solo el tipo: siete
    ///        filas `uint64_t` indistinguibles no se comparan con nada.
    /// @param m La medida entera, no solo su minimo.
    /// @param unidad Por defecto cyc/op; las razones van en "x".
    inline void registra(const char *caso, const Medida &m, const char *unidad = "cyc/op")
    {
        bench_record(caso, m.minimo, unidad, m.dispersion, m.recorrido(), m.iteraciones, m.repeticiones,
                     m.suelo, m.dispersion_baja, m.limpias, m.k_suelo);
    }

    /// @brief Cabecera de la tabla que imprime `imprime`.
    inline void imprime_cabecera()
    {
        std::printf("  %-34s %10s   %8s %8s  %8s  %13s  %s\n", "variante", "minimo", "suelo", "(+%)",
                    "limpias", "max-min", "calibracion");
        std::printf("  %s\n", "----------------------------------------------------------------"
                              "----------------------------------");
    }

} // namespace bench

#endif // BENCH_ADAPTATIVO_HPP
