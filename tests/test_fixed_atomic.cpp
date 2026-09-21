// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: fixed_int_atomic.hpp -- acceso atomico a fixed_int_t
// Part of int128 Library
// =============================================================================
//
// Cubre tres cosas distintas, y la tercera es la que el envoltorio viejo no
// podia cubrir:
//
//   1. **Funcional, un hilo.** Que cada operacion devuelva lo que manda el
//      contrato de `<atomic>`: `fetch_*` devuelve el valor ANTERIOR, el `++`
//      prefijo el nuevo y el sufijo el viejo.
//   2. **Concurrente.** Cuatro hilos sumando y restando a la vez; al final la
//      cuenta tiene que cuadrar exactamente. Si el CAS estuviera mal, o si el
//      relleno del tipo `checked` rompiera la comparacion, saldria aqui.
//   3. **Que `is_lock_free()` diga la verdad.** El envoltorio viejo devolvia
//      `false` fijo. Este no. Ojo: la relacion con `std::atomic` es una
//      IMPLICACION, no una igualdad --ver el apartado 3-- y se IMPRIME la tabla
//      por plataforma, porque depende del compilador Y de las banderas.
//
// El apartado 2 usa hilos, pero **este fichero NO necesita la excepcion** que
// lleva `test_param_thread_safety` para MSVC e Intel: se comprobo, y da 8/8 en
// los cuatro compiladores por los dos modos.
//
// Sobre `-latomic`: aqui decia que tampoco hacia falta, «que es justo lo que el
// diseno va a buscar». **Era verdad en los cuatro compiladores de Windows y
// falsa en `icpx` sobre Linux**, donde el CI murio al enlazar con
// `undefined reference to __atomic_compare_exchange` el 21 sep 2026. El diseno
// se corrigio --ahora exige ademas que el tipo quepa en una palabra-- y esta
// frase se queda como recordatorio de que la comprobacion se habia hecho en
// media plataforma.
// =============================================================================

#include "fixed_int_atomic.hpp"

#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

namespace
{
    int fallos = 0;
    int comprobaciones = 0;

    void comprueba(bool condicion, const char *que)
    {
        ++comprobaciones;
        if (!condicion)
        {
            std::printf("  [FALLA] %s\n", que);
            ++fallos;
        }
    }

    using namespace nstd;

    // -------------------------------------------------------- 1. funcional ---
    template <typename A>
    void funcional(const char *nombre)
    {
        using V = typename A::value_type;
        std::printf("-- funcional: %s\n", nombre);

        A a{V{10}};
        comprueba(a.load() == V{10}, "load tras construir");

        a.store(V{20});
        comprueba(a.load() == V{20}, "store");

        comprueba(a.exchange(V{30}) == V{20}, "exchange devuelve el anterior");
        comprueba(a.load() == V{30}, "exchange deja el nuevo");

        // `fetch_*` devuelven el ANTERIOR. Es el error clasico al escribir esto.
        comprueba(a.fetch_add(V{5}) == V{30}, "fetch_add devuelve el anterior");
        comprueba(a.load() == V{35}, "fetch_add suma");
        comprueba(a.fetch_sub(V{5}) == V{35}, "fetch_sub devuelve el anterior");
        comprueba(a.load() == V{30}, "fetch_sub resta");

        a.store(V{0b1100});
        comprueba(a.fetch_and(V{0b1010}) == V{0b1100}, "fetch_and devuelve el anterior");
        comprueba(a.load() == V{0b1000}, "fetch_and");
        comprueba(a.fetch_or(V{0b0011}) == V{0b1000}, "fetch_or devuelve el anterior");
        comprueba(a.load() == V{0b1011}, "fetch_or");
        comprueba(a.fetch_xor(V{0b1111}) == V{0b1011}, "fetch_xor devuelve el anterior");
        comprueba(a.load() == V{0b0100}, "fetch_xor");

        // El prefijo da el NUEVO, el sufijo el VIEJO.
        a.store(V{7});
        comprueba(++a == V{8}, "++a da el nuevo");
        comprueba(a++ == V{8}, "a++ da el viejo");
        comprueba(a.load() == V{9}, "a++ incrementa");
        comprueba(--a == V{8}, "--a da el nuevo");
        comprueba(a-- == V{8}, "a-- da el viejo");
        comprueba(a.load() == V{7}, "a-- decrementa");

        comprueba((a += V{3}) == V{10}, "+= da el nuevo");
        comprueba((a -= V{4}) == V{6}, "-= da el nuevo");

        // CAS: acierta cuando coincide, y cuando no, deja lo que habia.
        a.store(V{100});
        V esperado = V{100};
        comprueba(a.compare_exchange_strong(esperado, V{200}), "CAS acierta");
        comprueba(a.load() == V{200}, "CAS escribe");

        esperado = V{999};
        const bool fallo_esperado = a.compare_exchange_strong(esperado, V{300});
        comprueba(!fallo_esperado, "CAS falla si no coincide");
        comprueba(esperado == V{200}, "CAS deja en `esperado` lo que habia");
        comprueba(a.load() == V{200}, "CAS fallido no escribe");

        // La conversion implicita y la asignacion.
        a = V{42};
        const V leido = a;
        comprueba(leido == V{42}, "asignacion y conversion implicita");

        // Las funciones libres apuntan al mismo sitio.
        atomic_store(&a, V{55});
        comprueba(atomic_load(&a) == V{55}, "atomic_store / atomic_load libres");
        comprueba(atomic_fetch_add(&a, V{5}) == V{55}, "atomic_fetch_add libre");
        comprueba(atomic_load(&a) == V{60}, "atomic_fetch_add libre suma");
        comprueba(atomic_exchange(&a, V{1}) == V{60}, "atomic_exchange libre");
    }

    // ------------------------------------------------------ 2. concurrente ---
    template <typename A>
    void concurrente(const char *nombre)
    {
        using V = typename A::value_type;
        constexpr int HILOS = 4;
        constexpr int VUELTAS = 20000;

        std::printf("-- concurrente: %s (%d hilos x %d)\n", nombre, HILOS, VUELTAS);

        // Suma: cada hilo suma uno, VUELTAS veces. Al final, HILOS*VUELTAS.
        {
            A a{V{0}};
            std::vector<std::thread> hilos;
            hilos.reserve(HILOS);
            for (int h = 0; h < HILOS; ++h)
                hilos.emplace_back(
                    [&a]
                    {
                        for (int v = 0; v < VUELTAS; ++v)
                            (void)a.fetch_add(V{1}, std::memory_order_relaxed);
                    });
            for (auto &h : hilos)
                h.join();
            comprueba(a.load() == V{HILOS * VUELTAS}, "fetch_add concurrente cuadra");
        }

        // Suma y resta a la vez: la mitad suman, la mitad restan. Debe volver al
        // valor de partida.
        //
        // **Se parte de un valor alto a proposito, no de cero.** Con `checked`,
        // una resta que cruza el cero es un desbordamiento legitimo y deja la
        // marca, que es pegajosa: el resultado ya no vuelve nunca al valor de
        // partida. Arrancando en cero, que este test pase depende de si los
        // hilos se entrelazaron de forma que la cuenta bajara de cero o no --o
        // sea, **pasa por suerte**. Se vio: en clang fallaba y en gcc pasaba, y
        // la diferencia no era el CAS sino el entrelazado.
        {
            const V partida{HILOS * VUELTAS * 3};
            A a{partida};
            std::vector<std::thread> hilos;
            hilos.reserve(HILOS);
            for (int h = 0; h < HILOS; ++h)
                hilos.emplace_back(
                    [&a, h]
                    {
                        for (int v = 0; v < VUELTAS; ++v)
                        {
                            if (h % 2 == 0)
                                (void)a.fetch_add(V{3}, std::memory_order_relaxed);
                            else
                                (void)a.fetch_sub(V{3}, std::memory_order_relaxed);
                        }
                    });
            for (auto &h : hilos)
                h.join();
            comprueba(a.load() == partida, "sumas y restas concurrentes vuelven al valor de partida");
        }

        // CAS a pelo: cada hilo publica su propio valor; al final tiene que ser
        // uno de los publicados, nunca una mezcla de dos.
        {
            A a{V{0}};
            std::vector<std::thread> hilos;
            hilos.reserve(HILOS);
            for (int h = 0; h < HILOS; ++h)
                hilos.emplace_back(
                    [&a, h]
                    {
                        for (int v = 0; v < VUELTAS / 10; ++v)
                        {
                            V esperado = a.load(std::memory_order_relaxed);
                            while (!a.compare_exchange_weak(esperado, V{h + 1}, std::memory_order_acq_rel,
                                                            std::memory_order_relaxed))
                            {
                            }
                        }
                    });
            for (auto &h : hilos)
                h.join();
            const V fin = a.load();
            comprueba(fin >= V{1} && fin <= V{HILOS}, "el CAS deja un valor publicado, no una mezcla");
        }
    }

    // ------------------------------- 2b. la politica viaja con el valor ---
    //
    // El bucle CAS calcula con la aritmetica normal del tipo, asi que una
    // operacion que desborda deja la marca DENTRO del valor y se publica con el.
    // Esto no es un efecto colateral: es lo que hace que `checked` siga
    // significando lo mismo a traves del atomico.
    void la_marca_viaja()
    {
        using V =
            fixed_int_t<1, signedness::unsigned_type, representation_form::binnat, overflow_policy::checked>;
        using A = atomic_fixed_int_t<1, signedness::unsigned_type, representation_form::binnat,
                                     overflow_policy::checked>;

        A a{V{0}};
        comprueba(a.load().valid(), "arranca valido");

        // 0 - 3 sin signo: desborda.
        const V anterior = a.fetch_sub(V{3});
        comprueba(anterior.valid(), "el valor devuelto es el de ANTES, y era valido");
        comprueba(!a.load().valid(), "tras desbordar, el atomico publica el valor marcado");

        // Y la marca es pegajosa: sumar lo restado no la limpia.
        (void)a.fetch_add(V{3});
        comprueba(!a.load().valid(), "la marca es pegajosa tambien a traves del atomico");

        // Un store limpio si repone un valor valido.
        a.store(V{5});
        comprueba(a.load().valid(), "store repone un valor valido");
    }

    // ----------------------------------------- 3. que is_lock_free no mienta ---
    template <typename A>
    void dice_la_verdad(const char *nombre)
    {
        using V = typename A::value_type;
        A a{V{0}};

        // La relacion con `std::atomic` es una IMPLICACION, no una igualdad, y
        // eso es deliberado desde el 21 sep 2026.
        //
        // La version anterior exigia igualdad, apoyada en que
        // `is_always_lock_free` bastaba para no necesitar libatomic. **Con
        // `icpx` esa premisa es falsa**: declara lock-free y aun asi emite
        // `__atomic_compare_exchange`, y el CI de Linux murio al enlazar. Asi
        // que la clase exige ademas que el tipo quepa en una palabra.
        //
        // Lo que hay que garantizar, entonces, es solo una direccion:
        comprueba(!A::is_always_lock_free || std::atomic<V>::is_always_lock_free,
                  "si la clase dice sin bloqueo, std::atomic tambien lo dice");

        // Y que la condicion es exactamente la que se quiso poner, ni mas ni
        // menos: si esto falla, alguien cambio la regla sin actualizar el test.
        comprueba(A::is_always_lock_free ==
                      (std::atomic<V>::is_always_lock_free && sizeof(V) <= sizeof(void *)),
                  "la condicion es is_always_lock_free Y caber en una palabra");

        // Y si dice que SIEMPRE, la instancia concreta tambien tiene que decirlo.
        if (A::is_always_lock_free)
            comprueba(a.is_lock_free(), "si is_always_lock_free, la instancia tambien");

        std::printf("   %-38s size=%3zu  is_always_lock_free=%s\n", nombre, sizeof(V),
                    A::is_always_lock_free ? "SI" : "no");
    }

    using u1_wrap =
        atomic_fixed_int_t<1, signedness::unsigned_type, representation_form::binnat, overflow_policy::wrap>;
    using u1_checked = atomic_fixed_int_t<1, signedness::unsigned_type, representation_form::binnat,
                                          overflow_policy::checked>;
    using s1_wrap = atomic_fixed_int_t<1, signedness::signed_type, representation_form::twos_complement,
                                       overflow_policy::wrap>;
    using u2_wrap =
        atomic_fixed_int_t<2, signedness::unsigned_type, representation_form::binnat, overflow_policy::wrap>;
    using u2_checked = atomic_fixed_int_t<2, signedness::unsigned_type, representation_form::binnat,
                                          overflow_policy::checked>;
    using u4_wrap =
        atomic_fixed_int_t<4, signedness::unsigned_type, representation_form::binnat, overflow_policy::wrap>;
} // namespace

int main()
{
    std::printf("=== fixed_int_atomic: acceso atomico a fixed_int_t ===\n\n");

    // No copiable ni movible, como `std::atomic`.
    static_assert(!std::is_copy_constructible_v<u1_wrap>, "un atomico no se copia");
    static_assert(!std::is_move_constructible_v<u1_wrap>, "un atomico no se mueve");
    static_assert(!std::is_copy_assignable_v<u1_wrap>, "un atomico no se asigna copiando");

    // Los alias apuntan a lo que dicen.
    static_assert(std::is_same_v<nstd::atomic_uint64_fixed_t::value_type, nstd::uint64_fixed_t>,
                  "atomic_uint64_fixed_t envuelve uint64_fixed_t");

    std::printf("[1] Funcional, un hilo\n");
    funcional<u1_wrap>("<1,u,binnat,wrap>");
    funcional<u1_checked>("<1,u,binnat,checked>  (7 bytes de relleno)");
    funcional<s1_wrap>("<1,s,c2,wrap>");
    funcional<u2_wrap>("<2,u,binnat,wrap>");
    funcional<u2_checked>("<2,u,binnat,checked>");
    funcional<u4_wrap>("<4,u,binnat,wrap>");

    std::printf("\n[2] Concurrente\n");
    concurrente<u1_wrap>("<1,u,binnat,wrap>");
    concurrente<u1_checked>("<1,u,binnat,checked>  (7B relleno)");
    concurrente<u2_wrap>("<2,u,binnat,wrap>");
    concurrente<u4_wrap>("<4,u,binnat,wrap>");

    std::printf("\n[2b] La marca de `checked` viaja a traves del atomico\n");
    la_marca_viaja();

    std::printf("\n[3] is_lock_free dice la verdad (depende del compilador)\n");
    dice_la_verdad<u1_wrap>("<1,u,binnat,wrap>");
    dice_la_verdad<u1_checked>("<1,u,binnat,checked>");
    dice_la_verdad<s1_wrap>("<1,s,c2,wrap>");
    dice_la_verdad<u2_wrap>("<2,u,binnat,wrap>");
    dice_la_verdad<u2_checked>("<2,u,binnat,checked>");
    dice_la_verdad<u4_wrap>("<4,u,binnat,wrap>");

    std::printf("\n%d comprobaciones, %d fallos\n", comprobaciones, fallos);
    if (fallos == 0)
        std::printf("TODOS LOS TESTS PASARON\n");
    return fallos == 0 ? 0 : 1;
}
