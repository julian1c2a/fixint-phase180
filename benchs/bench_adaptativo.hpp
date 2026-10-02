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
// ronda, para que ni la posicion ni el vecino favorezcan a ninguna. Desde el 2
// oct 2026 ese orden es AL AZAR en cada ronda; ver «El orden de las variantes».
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
#include <cstdlib>
#include <random>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
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

        /// @brief CUANDO se tomaron las muestras: segundos desde la epoca, del reloj
        ///        de pared. -1 si no se sabe.
        ///
        /// Marca el tramo de las vueltas cronometradas --no la calibracion ni el
        /// calentamiento--, que es durante el que una perturbacion de la maquina
        /// puede ensuciar la cola baja. Sin el no hay forma de saber que ventanas
        /// coincidieron con que (P2.22). Todas las variantes de una ventana
        /// comparten sello, porque se miden entrelazadas en el mismo tramo.
        double t_inicio{-1};
        double t_fin{-1};

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
    /// @brief Techo de la calibracion: 2^36 iteraciones, o 2^30 donde `size_t`
    ///        tiene 32 bits.
    ///
    /// Gastar 200 ms en 2^36 vueltas son 3 picosegundos por vuelta, lo que no
    /// cuesta NADA que haga trabajo de verdad. Si la calibracion llega aqui, el
    /// compilador se ha llevado la operacion. En 32 bits, 2^30 vueltas en 200 ms
    /// son 0,19 ns: menos de un ciclo de cualquier procesador de esa clase.
    ///
    /// EL ANCHO IMPORTA, y se aprendio en el CI el mismo dia: el techo era un
    /// `size_t{1} << 36` fijo, que en armhf --`size_t` de 32 bits-- no compila.
    ///
    /// ANTES NO HABIA TECHO, y el fallo era silencioso. Con una operacion que no
    /// cuesta nada el tiempo no crece con `n`, y `n` se multiplicaba por 4
    /// cuarenta veces: en la 32 llega a 2^64, que en `std::size_t` es CERO. La
    /// medida dividia entonces por cero y publicaba un NaN. Lo destapo el 2 oct
    /// 2026 la prueba del orden (`tests/test_bench_orden.cpp`), con unas
    /// variantes que GCC, clang y MSVC reducian a una sola llamada.
    inline constexpr std::size_t MAX_ITERACIONES = std::size_t{1} << (sizeof(std::size_t) >= 8 ? 36 : 30);

    /// @brief Calibra: cuantas iteraciones hacen falta para gastar `objetivo_ms`.
    ///
    /// Dobla desde 1 hasta pasarse, midiendo con el reloj de pared --no con
    /// RDTSC-- porque lo que se quiere acotar es el TIEMPO que dura la sesion,
    /// y RDTSC cuenta a frecuencia invariante, no a la real.
    ///
    /// @param op Lo que se mide. Recibe el indice de la vuelta.
    /// @param objetivo_ms Milisegundos que se quieren gastar.
    /// @return Numero de iteraciones, entre 1 y `MAX_ITERACIONES`. Si toca el
    ///         techo, avisa: lo que se mide no cuesta nada.
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
            if (n >= MAX_ITERACIONES)
                break;

            // Si la medida es demasiado corta para fiarse del reloj, dobla a
            // ciegas; si ya se puede extrapolar, salta directamente.
            if (ms < 1.0)
                n *= 4;
            else
            {
                const double factor = objetivo_ms / ms;
                // En `double` y recortado ANTES de convertir: pasar a `size_t`
                // un `double` mayor que su maximo es comportamiento indefinido.
                const double deseado = static_cast<double>(n) * factor * 1.1 + 1.0;
                const std::size_t siguiente = deseado >= static_cast<double>(MAX_ITERACIONES)
                                                  ? MAX_ITERACIONES
                                                  : static_cast<std::size_t>(deseado);
                n = siguiente > n ? siguiente : n * 2;
            }
            if (n > MAX_ITERACIONES)
                n = MAX_ITERACIONES;
        }
        std::printf("  [OJO] la calibracion llego a %zu vueltas sin gastar %.3g ms: lo que se mide NO\n"
                    "        CUESTA NADA. El compilador se ha llevado la operacion; la cifra no vale.\n",
                    n, objetivo_ms);
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

    /// @brief Segundos desde la epoca, del reloj de PARED.
    ///
    /// `system_clock` y no `steady_clock`, a proposito: el sello lo compara otro
    /// proceso (`bench_history.py`, con `time.time()`), y la epoca de
    /// `steady_clock` no esta especificada -- dos procesos pueden no compartirla.
    [[nodiscard]] inline double ahora_epoca() noexcept
    {
        return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    /// @brief Mide UNA variante, con calibracion y repeticiones.
    ///
    /// @param op Lo que se mide.
    /// @param reps Repeticiones; el protocolo pide diez como minimo.
    /// @param objetivo_ms Tiempo por repeticion.
    template <typename F>
    Medida mide_una(F op, std::size_t reps = REPETICIONES, double objetivo_ms = MS_POR_CASILLA)
    {
        // `op` ya es una copia local, que es lo que `mide_entrelazado` tiene que
        // hacer a mano (ver alli). Se exige lo mismo, para que las dos digan lo
        // mismo de una variante.
        static_assert(std::is_invocable_v<const F &, std::size_t>,
                      "mide_una: la variante no puede ser `mutable`");
        const std::size_t n = calibra(op, objetivo_ms);

        // Calentamiento que se descarta: la primera vuelta paga los fallos de
        // cache y la subida de frecuencia.
        for (std::size_t k = 0; k < n / 4 + 1; ++k)
            op(k);

        std::vector<double> v;
        v.reserve(reps);
        const double t_inicio = ahora_epoca(); // el tramo cronometrado, y solo el
        for (std::size_t r = 0; r < reps; ++r)
        {
            CycleTimer t;
            for (std::size_t k = 0; k < n; ++k)
                op(k);
            v.push_back(static_cast<double>(t.elapsed_cycles()) / static_cast<double>(n));
        }
        const double t_fin = ahora_epoca();
        Medida m = resume(std::move(v), n);
        m.t_inicio = t_inicio;
        m.t_fin = t_fin;
        return m;
    }

    // =========================================================================
    // El orden de las variantes en cada ronda (2 oct 2026)
    // =========================================================================
    //
    // HASTA EL 2 OCT ROTABA una posicion por ronda: en la ronda r, el orden era
    // r, r+1, ..., r-1. Eso equilibra la POSICION --cada variante pasa por cada
    // sitio las mismas veces-- pero deja fijo el VECINO: la variante j va detras
    // de la j-1 en K-1 de cada K rondas, siempre. Si una variante deja la
    // maquina peor de lo que la encontro --el monton de GMP fragmentado, el
    // nucleo mas caliente, el predictor entrenado en otra cosa--, quien la sigue
    // lo paga SIEMPRE, y ninguna estadistica de la tanda lo puede ver: lo llevan
    // todas sus vueltas, tambien las de la cola baja.
    //
    // AHORA ES UNA PERMUTACION AL AZAR EN CADA RONDA. Lo que dependa del vecino
    // deja de ser un sesgo y pasa a ser ruido; y el ruido si lo ve la tanda,
    // porque unas vueltas lo llevan y otras no, y el suelo se queda con las que
    // no.
    //
    // CUANTO PESABA, NO SE SABE. Con 200 ms por vuelta, lo que se recupera en
    // microsegundos --la cache, el predictor-- se diluye; lo que dura mas --la
    // temperatura, el estado del monton-- no. Se mide en la plataforma dedicada:
    // el mismo binario con los dos ordenes, el mismo minuto (NEXT_STEPS, E3).
    //
    // EL AZAR SE PUEDE REPETIR. La semilla de la toma se anuncia al empezar y
    // queda en el registro (`#orden`); `BENCH_SEMILLA=<semilla>` repite los
    // mismos ordenes. El generador y el barajado son PROPIOS --splitmix64 y
    // Fisher-Yates con rechazo--, no los de la biblioteca estandar: lo que hace
    // `std::shuffle` con una semilla no esta especificado, y la misma daria
    // ordenes distintos en libstdc++, libc++ y la de MSVC.
    //
    // `BENCH_ORDEN=rotando` devuelve el orden de antes. Existe para MEDIR la
    // diferencia, no para medir con el; igual que `BENCH_REPETICIONES`.

    /// @brief Un paso de splitmix64: avanza `estado` y devuelve 64 bits.
    [[nodiscard]] inline std::uint64_t siguiente_azar(std::uint64_t &estado) noexcept
    {
        std::uint64_t z = (estado += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    /// @brief Entero uniforme en `[0, n)`, con `n >= 1`.
    ///
    /// SIN SESGO: `x % n` a secas favorece los restos pequenos cuando 2^64 no es
    /// multiplo de `n`. Se rechaza la cola que no completa un multiplo.
    [[nodiscard]] inline std::size_t azar_menor_que(std::uint64_t &estado, std::size_t n) noexcept
    {
        const std::uint64_t m = static_cast<std::uint64_t>(n);
        const std::uint64_t tope = UINT64_MAX - UINT64_MAX % m; // multiplo de m
        std::uint64_t x = siguiente_azar(estado);
        while (x >= tope)
            x = siguiente_azar(estado);
        return static_cast<std::size_t>(x % m);
    }

    /// @brief Como se ordenan las variantes dentro de cada ronda de UNA ventana.
    struct Orden
    {
        bool al_azar{true};       ///< false: rota, el protocolo de antes del 2 oct.
        std::uint64_t semilla{0}; ///< la de ESTA ventana, sacada de la de la toma.
    };

    /// @brief El orden de la ronda `r`. Funcion pura salvo por `estado`, que
    ///        avanza: las rondas de una ventana se piden en orden, una tras otra.
    template <std::size_t K>
    [[nodiscard]] std::array<std::size_t, K> orden_de_ronda(const Orden &o, std::uint64_t &estado,
                                                            std::size_t r)
    {
        std::array<std::size_t, K> p{};
        for (std::size_t i = 0; i < K; ++i)
            p[i] = o.al_azar ? i : (i + r) % K;
        if (o.al_azar)
            for (std::size_t i = K; i > 1; --i) // Fisher-Yates
                std::swap(p[i - 1], p[azar_menor_que(estado, i)]);
        return p;
    }

    namespace detalle
    {
        struct OrdenDeLaToma
        {
            bool al_azar{true};
            std::uint64_t semilla{0}; ///< la de la toma, la que se anuncia.
            std::uint64_t estado{0};  ///< de aqui sale la semilla de cada ventana.
        };

        /// @brief Se decide UNA vez por proceso, al pedir la primera ventana.
        inline OrdenDeLaToma &orden_de_la_toma()
        {
            static OrdenDeLaToma t = []
            {
                OrdenDeLaToma r{};
                const char *modo = std::getenv("BENCH_ORDEN");
                r.al_azar = !(modo != nullptr && std::string_view(modo) == "rotando");
                const char *s = std::getenv("BENCH_SEMILLA");
                if (s != nullptr && *s != '\0')
                    r.semilla = std::strtoull(s, nullptr, 0);
                else
                {
                    std::random_device rd;
                    r.semilla = (static_cast<std::uint64_t>(rd()) << 32) ^ static_cast<std::uint64_t>(rd()) ^
                                static_cast<std::uint64_t>(
                                    std::chrono::steady_clock::now().time_since_epoch().count());
                }
                r.estado = r.semilla;

                char texto[64];
                if (r.al_azar)
                {
                    std::snprintf(texto, sizeof texto, "al_azar 0x%016llx",
                                  static_cast<unsigned long long>(r.semilla));
                    std::printf("  [orden] al azar en cada ronda, semilla 0x%016llx "
                                "(BENCH_SEMILLA=0x%016llx lo repite)\n",
                                static_cast<unsigned long long>(r.semilla),
                                static_cast<unsigned long long>(r.semilla));
                }
                else
                {
                    std::snprintf(texto, sizeof texto, "rotando");
                    std::printf("  [orden] ROTANDO (BENCH_ORDEN=rotando): el protocolo de antes del "
                                "2 oct 2026, solo para medir la diferencia\n");
                }
                bench_record_meta("orden", texto);
                return r;
            }();
            return t;
        }
    } // namespace detalle

    /// @brief El orden de la PROXIMA ventana. Cada llamada saca una semilla
    ///        nueva de la de la toma, asi que dos ventanas no repiten la misma
    ///        sucesion de ordenes, y con la misma semilla de toma se repiten
    ///        todas.
    [[nodiscard]] inline Orden orden_de_la_ventana()
    {
        auto &t = detalle::orden_de_la_toma();
        return Orden{t.al_azar, siguiente_azar(t.estado)};
    }

    /// @brief Mide varias variantes **entrelazadas y en orden cambiante**.
    ///
    /// En cada ronda se ejecutan todas, **en un orden al azar distinto cada
    /// ronda** (ver «El orden de las variantes»). Asi ninguna ocupa siempre el
    /// mismo sitio ni va siempre detras de la misma, y la deriva termica y el
    /// escalado de frecuencia se reparten por igual entre ellas.
    ///
    /// Es la regla 4 del protocolo, y hasta ahora era imposible de cumplir:
    /// requiere que las variantes existan A LA VEZ en el mismo binario.
    ///
    /// @param ops `std::tuple` de invocables, cada uno recibe el indice de la
    ///        vuelta.
    /// @param reps Repeticiones por variante; diez es el minimo del protocolo.
    /// @param objetivo_ms Tiempo por repeticion y variante.
    /// @param orden Como se ordenan las rondas. Por omision, el de la toma; las
    ///        pruebas pasan uno fijo para poder comprobarlo.
    /// @return Una `Medida` por variante, en el orden en que se pasaron.
    template <typename... Fs>
    auto mide_entrelazado(std::tuple<Fs...> ops, std::size_t reps = REPETICIONES,
                          double objetivo_ms = MS_POR_CASILLA, Orden orden = orden_de_la_ventana())
    {
        constexpr std::size_t K = sizeof...(Fs);
        static_assert(K >= 1, "hace falta al menos una variante");
        // El bucle cronometrado trabaja sobre una COPIA de cada variante (ver
        // alli por que). Eso solo es lo mismo que usar la original si la
        // variante no guarda estado propio entre vueltas: se exige aqui, en vez
        // de suponerlo.
        static_assert((std::is_invocable_v<const Fs &, std::size_t> && ...),
                      "mide_entrelazado: las variantes no pueden ser `mutable` (se copian)");
        static_assert((std::is_copy_constructible_v<Fs> && ...),
                      "mide_entrelazado: las variantes tienen que poder copiarse");

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

        // EL SELLO VA AQUI, ni antes ni despues: justo alrededor de las vueltas
        // cronometradas, que son las que forman la cola baja.
        std::uint64_t estado = orden.semilla;
        const double t_inicio = ahora_epoca();
        for (std::size_t r = 0; r < reps; ++r)
        {
            const std::array<std::size_t, K> turno = orden_de_ronda<K>(orden, estado, r);
            for (std::size_t paso = 0; paso < K; ++paso)
            {
                const std::size_t cual = turno[paso];
                [&]<std::size_t... I>(std::index_sequence<I...>)
                {
                    (
                        [&]
                        {
                            if (I != cual)
                                return;
                            // COPIAS LOCALES de la variante y del numero de
                            // vueltas, a proposito (2 oct 2026). Una barrera con
                            // `memory` --la de `escapa`, la de `doNotOptimize`
                            // para tipos que no son de 16 bytes-- obliga al
                            // compilador a suponer que cualquier memoria a la
                            // que se llega ha podido cambiar, y la tupla y `n`
                            // estaban en esa memoria: GCC volvia a leer EN CADA
                            // VUELTA el puntero que la variante lleva capturado
                            // y el limite del bucle. Medido con
                            // `benchmark_vs_builtin`: copiar un `uint64_t`
                            // costaba 2,2 tics dentro del arnes y 1,2 fuera. Un
                            // tic de mas en todas las variantes no las ordena
                            // distinto, pero aplasta las razones entre ellas.
                            // Unas copias locales, que no se dejan ver desde
                            // fuera, pueden vivir en registros.
                            const auto op = std::get<I>(ops);
                            const std::size_t vueltas = n[I];
                            CycleTimer t;
                            for (std::size_t k = 0; k < vueltas; ++k)
                                op(k);
                            v[I].push_back(static_cast<double>(t.elapsed_cycles()) /
                                           static_cast<double>(vueltas));
                        }(),
                        ...);
                }(std::make_index_sequence<K>{});
            }
        }

        const double t_fin = ahora_epoca();

        std::array<Medida, K> salida{};
        for (std::size_t i = 0; i < K; ++i)
        {
            salida[i] = resume(std::move(v[i]), n[i]);
            salida[i].t_inicio = t_inicio;
            salida[i].t_fin = t_fin;
        }
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
                     m.suelo, m.dispersion_baja, m.limpias, m.k_suelo, m.t_inicio, m.t_fin);
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
