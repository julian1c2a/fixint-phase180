# 🔮 NEXT STEPS

**Last Updated:** 26 August 2026
**Versión:** **v1.90.4** publicada · **rama** `phase-1.80` · árbol limpio · todo en `origin`

> Este documento es **el puntero y lo pendiente a corto**. No acumula historia:
> lo ya hecho vive en [`CHANGELOG.md`](CHANGELOG.md), el plan largo en
> [`ROADMAP.md`](ROADMAP.md) y el estado en [`PROJECT_STATUS.md`](PROJECT_STATUS.md).
>
> **Y no acumula tareas hechas.** Llegó a tener 405 renglones con dos secciones
> «para mañana» y trabajo ya terminado dentro. Si al abrirlo hay algo hecho, se
> borra en el momento: es la única forma de que la lista sirva para decidir.

---

# 📍 POR AQUÍ VAMOS

## Estado al 26 ago 2026

| | |
|---|---|
| **Release** | ✅ **v1.90.4 publicada**, la primera del proyecto: tres zips (gcc, clang, msvc) |
| **Suite local** | ✅ 55/55, GCC 16.2 |
| **CI** | ⚠️ **en rojo desde el 25 ago**, por el techo de avisos de doxygen. Arreglado en `314543e`, **pendiente de confirmar** |
| **Diseño de la 2.0** | ✅ cerrado, sin cuestiones abiertas. Falta escribirlo |
| **ADR** | 14 registros, ninguna decisión sin documentar |

**Lo primero al retomar: mirar si el CI se puso verde con `314543e`.** Si no,
leer el log del job `format-and-docs` antes que nada.

---

## Prioridades

Cómo se decide qué va antes. **Tres preguntas, en este orden**; la primera que
dé «sí» fija el nivel. Cada criterio sale de un tropiezo concreto, no de una
máxima general.

### Criterio 1 — ¿Miente?

**Todo lo que hace creer al proyecto algo que no es cierto va primero.** No por
pulcritud: las demás prioridades **se deciden leyendo esas señales**. Si mienten,
se prioriza mal y ni siquiera se sabe.

Lo que ha aparecido en dos días:

| Señal | Decía | Pasaba |
|---|---|---|
| `make.py build` | «Build complete», salida 0 | el enlazado había fallado |
| «0 avisos de Doxygen» | cobertura perfecta | la comprobación estaba apagada |
| Job de Intel en la release | `success` en 2,3 min | cero artefactos |
| Benchmark de Karatsuba | 6,23× de mejora | medía contra un espantapájaros |
| CI en verde durante meses | todo bien | el workflow de release llevaba roto |
| «armonizador 7/7» | todo conforme | se corría **sin** `--doxygen`; el CI lo corre **con** |
| `cross-arm32`, `aarch64` | jobs en verde | 1 y 2 tests fallando, tapados |

Siete, y **ninguno se detectó mirando**: todos salieron de comprobar un resultado
que ya se daba por bueno.

### Criterio 2 — ¿Bloquea?

El orden del camino crítico **no es negociable**, lo fija
[ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md):
política de desbordamiento → unificación de tipos → punto fijo. Al revés
significa **portar la API dos veces**. No hay nada que decidir aquí: está
decidido, hay que escribirlo.

### Criterio 3 — ¿Caduca, y hacia qué lado?

De dos tareas igual de secundarias, la que caduca va antes o después según en qué
dirección lo haga:

- **Medir caduca hacia adelante.** Una cifra tomada *después* de reescribir el
  código **no se puede comparar** con las de hoy. Los benchmarks pendientes van
  **antes** de la 2.0, aunque parezcan menos importantes.
- **Documentar `int128_param_*` caduca hacia atrás.** Ese tipo lo retira
  [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md): la
  cuenta **baja sola** conforme se alcanza la paridad, porque cada pieza portada
  permite **borrar** la vieja. El trabajo no es documentar, es borrar.

### Desempate

Entre iguales, **primero lo que se reproduce en local**. Las cuatro releases que
costó publicar v1.90.4 se fueron en probar cosas que solo podían probarse en el
CI; reproducir el fallo de v1.90.2 en local costaba dos segundos.

---

## Los siguientes pasos, ordenados

### P0 — Señales que mienten

| | Qué | Estado |
|---|---|---|
| ~~P0.1~~ | ~~`make.py` y el enlazado~~ | ✅ **hecho** (`05ba169`) |
| **P0.2** | **Confirmar que el CI se pone verde** con el techo por versión de doxygen | pendiente, `314543e` en marcha |
| **P0.3** | **47 `assert()` que `-DNDEBUG` borra**, en 3 ficheros de test | decidido, sin hacer |
| **P0.4** | **`tests/test_template_type.cpp` no comprueba nada** | decidido, sin hacer |
| **P0.5** | **`cross-arm32` y `aarch64` tapan fallos reales** | decidido, sin hacer |
| **P0.6** | **Intel oneAPI en Windows**: el proyecto dice «validado en 4 compiladores» y en Windows solo lo están 3 | pendiente |

### P1 — Camino crítico (el orden lo fija ADR-007)

| | Qué | Depende de |
|---|---|---|
| **P1.1** | **Almacenamiento de la política**: miembro extra solo con `checked`, con `static_assert` de que `sizeof(fixed_int_t<4, …, wrap>)` sigue siendo 32 | — |
| **P1.2** | Propagación de la marca, `valid()`, comparación lexicográfica sobre `(válido?, valor)` | P1.1 |
| **P1.3** | **`checked_div` y las tres `saturating_*`**, devolviendo el tipo con política `checked` | P1.1 |
| **P1.4** | `representation_traits<binnat>` y generalizar el `static_assert` al bicondicional de [ADR-011](docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md) | — (pequeño, se puede colar antes) |
| **P1.5** | **Portar Magnitud-Signo y Exceso-K** a `fixed_int_t` y retirar `int128_param_t` | P1.3 |
| **P1.6** | Etapa 5: punto fijo | P1.5 |

### P2 — Medir, antes de que el código cambie

| | Qué | Por qué antes |
|---|---|---|
| **P2.1** | **Barrido de `operator*`**: N=3,5,7,9 frente a N=6,10,12. ¿Paridad o tamaño? | La 2.0 toca `operator*`; después no habría con qué comparar |
| **P2.2** | Karatsuba en **Clang, MSVC e Intel** | Ídem. P0.6 lo desbloquea para Intel |
| **P2.3** | **Re-medir las tablas heredadas** de Knuth D y de comparación con built-in | Hoy **incumplen la regla del propio `docs/PERFORMANCE.md`**: sin fecha, compilador ni máquina |
| **P2.4** | Coste de las conversiones a y desde cadena, bases 2..36 | API nueva de v1.90.1, sin medir |
| **P2.5** | **Montar el histórico de benchmarks** (ver abajo) | Da sitio donde guardar P2.1–P2.4 |

### P3 — Lo que se abarata o desaparece esperando

| | Qué | Nota |
|---|---|---|
| **P3.1** | Decidir si `intrinsics/compiler_detection.hpp` (29 avisos) y `algorithms/karatsuba.hpp` (3) entran en el ámbito de [ADR-014](docs/decisions/ADR-014-cobertura-de-doxygen.md) | Decisión, no trabajo |
| **P3.2** | Cerrar la puerta: `WARN_AS_ERROR = YES` cuando el ámbito llegue a cero | Depende de P3.1 |
| **P3.3** | Los 6 headers sin `API_*.md` propio que señala el armonizador | — |
| **P3.4** | `benchmark_vs_builtin` no enlaza sin GMP | Ahora **sí se ve**, desde P0.1 |
| **P3.5** | Documentar `int128_param_*` (505 avisos) | **Caduca hacia atrás**: baja sola con P1.5 |
| **P3.6** | Decidir si Intel sale de la matriz de release | Se cae solo si P0.6 sale bien |

---

## El detalle de lo inmediato

### P0.3 — Activar los 47 `assert()`

`tests/test_param_iostreams.cpp` (35), `tests/test_representation_conversions.cpp`
(8) y `tests/test_phase5_operators.cpp` (4). En release —el modo que se ejecuta—
no comprueban nada y los tres cuentan como aprobados.

La vía directa es `#undef NDEBUG` antes de `#include <cassert>`. Pega a tener en
cuenta: **`assert` aborta en el primer fallo**, con `SIGABRT`. El código de
salida es distinto de cero, que es lo que el arnés necesita, pero **no da
recuento** ni sigue con los demás casos, al revés que el `TEST()` de los otros 51.

| | Coste | Qué se obtiene |
|---|---|---|
| `#undef NDEBUG` | tres líneas | Comprueban de verdad. Se paran en el primer fallo |
| Convertir a `TEST()` | 47 sustituciones | Recuento, sigue tras el fallo, suite homogénea |

**Cerrar el agujero ya con la primera; convertir después.**

### P0.4 — `test_template_type.cpp`

Imprime el resultado de dos `is_same_v` y termina con
`«[OK] Template type test passed»` y `return 0`, pase lo que pase. Pasar esas dos
comprobaciones a `static_assert` —son de tiempo de compilación— y quitar el
mensaje que afirma lo que no se ha verificado.

### P0.5 — arm32 y aarch64: no es inestabilidad, es que nadie ha mirado

Datos del run 33121568464, no suposiciones:

| Arco | Compilación | Ejecución bajo QEMU |
|---|---|---|
| **arm32** | **54/55** — 1 no compila | **53/54** — 1 falla |
| **aarch64** | 55/55 | **53/55 — 2 fallan** |
| riscv64 | 55/55 | 55/55 |

Tapado por dos mecanismos distintos: **arm32** no hace `exit 1` y además lleva
`continue-on-error: true`; **aarch64** sí hace `exit 1`, pero con umbral
`FAIL > TOTAL/10 + 1` — con 2 sobre 55 el umbral es 6 y **pasa igual**.

1. **Averiguar qué tests son.** Sin eso no se decide nada. Los logs se bajan con
   el token del gestor de credenciales de git.
2. Si son fallos reales en ARM, **son bugs y van a P0**: el README afirma que la
   biblioteca está probada ahí.
3. Si son cosa de QEMU, **excluir ese test por nombre y con motivo**, y quitar
   las dos tolerancias. Una exclusión nombrada es honesta; un umbral que se traga
   cualquier cosa, no.

### P0.6 — Intel oneAPI

**Está instalado en esta máquina**, así que se reproduce y arregla en local:

```
C:\Program Files (x86)\Intel\oneAPI\setvars.bat
C:\Program Files (x86)\Intel\oneAPI\2026.1\bin\   icx.exe, icpx.exe, icx-cc.exe, icx-cl.exe
                                    2026.0\bin\   y 2025.3\bin\
```

**Medio camino hecho, y una pista.** `scripts/env_setup/compiler_env.py` ya
conoce `INTEL_ROOT` y `setvars.bat` (línea 99), pero busca el compilador en
`compiler/2025.3/bin/icpx.exe` — **la versión más vieja de las tres
instaladas**. Y `toolchains.py` resuelve `intel: icx` **a secas**, dependiendo
del `PATH`, que es justo lo que ya se corrigió para los demás compiladores.

Orden: (1) que funcione en local; (2) que `toolchains.py` y `toolchains.json` lo
resuelvan por ruta real; (3) **después** el CI, que es otro problema — allí
`ONEAPI_ROOT` viene vacío y `rscohn2/setup-oneapi` deja un `CMAKE_PREFIX_PATH`
que apunta a `/opt/intel/...`, una ruta de Linux en un runner de Windows.

Decisión pendiente: **cuál de las tres versiones se fija**, y anotarlo como se
hizo con clang-format en [ADR-013](docs/decisions/ADR-013-clang-format-local-22-ci-21.md).

### P2.1 — La penalización de `operator*`

El control del benchmark de Karatsuba destapó que el bucle escolar de
`operator*` es un **14 % más lento que una copia idéntica suya escrita como
función libre**, con las mismas primitivas y el mismo compilador. Estable en dos
ejecuciones (0,86× y 0,87×). En N=16 la razón sale 1,02×.

Barrido: **N=3, 5, 7, 9** (impares) frente a **N=6, 10, 12** (pares que tampoco
usan Karatsuba). Eso separa «es cosa de la paridad» de «es cosa de no ser 2, 4
u 8». Si sale lo segundo, apunta a la generación de código del `if constexpr`
encadenado; si lo primero, al bucle de acarreo. Después, el ensamblador del N que
peor salga.

### P2.5 — Histórico de benchmarks

**Más frecuente no es mejor si las medidas no son comparables.** Una cifra de un
runner compartido y otra de esta máquina no van en la misma serie, y una serie
con medidas incomparables es peor que no tener serie: invita a leer tendencias
que no existen.

1. Que `benchs/bench_common.hpp` emita, además de la tabla legible, una línea
   **legible por máquina** con lo que exige la regla de
   [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md): fecha, commit, compilador, modo,
   máquina y cifras.
2. Un `benchs/history/` con un fichero por ejecución, **indexado por máquina**.
3. Un guion que compare contra las N anteriores **de esa misma máquina** y avise
   de lo que salga del ruido. El umbral hay que **calibrarlo** con ejecuciones
   repetidas sin cambios: sin eso, avisaría de todo.
4. Frecuencia: **en local, a menudo**. En el CI solo lo barato y estable, que
   sirve para detectar un 10×, no un 5 %.

---

## Cosas que conviene no redescubrir

- **El armonizador se ejecuta con `--doxygen`.** Sin ese flag son 7
  comprobaciones; el CI hace 9. Correrlo sin él y cantar «7/7» fue lo que dejó el
  CI en rojo cinco commits sin que nadie lo viera.
- **doxygen no cuenta igual según la versión.** 1.9.8 (CI) da 518 avisos y 1.18.0
  (local) da 505 sobre el mismo árbol. Por eso el techo de
  `check_docs_consistency.py` es **una cifra por versión**.
- **clang-format: en local la 22.1.8, en el CI la 21.** El árbol es punto fijo de
  las dos, medido; la regla es que lo siga siendo. La serie 19 sí lo rompe.
- En Windows, **`g++` y `clang++` a secas NO son los del proyecto**: resuelven al
  toolchain MSYS. Usar `python scripts/toolchains.py`.
- El `Makefile` es un **shim** sobre `make.py`, que es la capa canónica.
- **`make.py build` ya sí detecta el fallo de enlazado** (desde `05ba169`). Antes
  no, y todo lo construido encima heredaba la mentira.
- Antes de cerrar sesión: `/guarda_y_sube`, que ejecuta los cuatro verificadores
  que exige el CI.
