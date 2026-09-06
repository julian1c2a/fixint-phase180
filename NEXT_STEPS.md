# 🔮 NEXT STEPS

**Last Updated:** 6 September 2026
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

## Estado al 6 sep 2026

| | |
|---|---|
| **Release** | ✅ **v1.90.4 publicada**, la primera del proyecto: tres zips (gcc, clang, msvc) |
| **Suite local** | ✅ **58/58 con los cuatro**: GCC (libstdc++), clang (**libc++**), MSVC e Intel |
| **CI** | ✅ **24/24 jobs** sobre `f959f53`, ya con la matriz compilando de verdad: no había fallos tapados |
| **Diseño de la 2.0** | ✅ cerrado. **P1.1 a P1.4 escritos**; quedan P1.5 y P1.6 |
| **ADR** | 15 registros, ninguna decisión sin documentar |

**Lo primero al retomar: `python scripts/check_docs_consistency.py --doxygen`.**
Con `--doxygen`, que es la orden que corre el CI; sin el flag son 7
comprobaciones en vez de 9 y no sirve de nada.

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
| ~~P0.2~~ | ~~Confirmar que el CI se pone verde~~ | ✅ **confirmado** en `314543e` |
| ~~P0.3~~ | ~~47 `assert()` que `-DNDEBUG` borra~~ | ✅ **hecho**, y comprobado que ahora saltan |
| ~~P0.4~~ | ~~`test_template_type.cpp` no comprueba nada~~ | ✅ **hecho**: las identidades de tipo son `static_assert` |
| ~~P0.5~~ | ~~`cross-arm32` y `aarch64` tapan fallos reales~~ | ✅ **hecho**: un bug de compilacion y tres timeouts |
| ~~P0.6~~ | ~~Intel oneAPI en Windows~~ | ✅ **en local**: 55/55 con Intel 2026.1. Queda **solo el runner del CI** |
| ~~P0.7~~ | ~~El clang del proyecto: CLANG64 (libc++) o UCRT64~~ | ✅ **cerrado a favor de las DOS**: `nstd::is_integral...` se define para libstdc++ y para libc++. Eran tres causas: la guarda que se causaba a sí misma, una regresión de P1.1 en el 4.º parámetro, y `-latomic` a ciegas |
| ~~P0.8~~ | ~~Qué saca la matriz del CI ahora que compila de verdad~~ | ✅ **nada tapado**: 24/24 jobs verdes sobre `f959f53`, matriz completa |
| **P0.9** | **Tests de guardas de include y de macros de configuración.** Que la suite *afirme* en qué combinación compilador/biblioteca corre, y falle si no la reconoce. Ver el detalle abajo | — |

### P1 — Camino crítico (el orden lo fija ADR-007)

| | Qué | Depende de |
|---|---|---|
| ~~P1.1~~ | ~~Almacenamiento de la política~~ | ✅ **hecho** en `6ea1ae4`: 32 bytes con `wrap` en los cuatro compiladores |
| ~~P1.2~~ | ~~Propagación de la marca, `valid()`, comparación~~ | ✅ **hecho** en `5ad2bad`: `+ - * << - ++ --` y sus `op=`, orden total, `to_string` |
| ~~P1.3~~ | ~~`checked_div` y las tres `saturating_*`~~ | ✅ **hecho** en `d684bb6`, y las `checked_*` dejan `std::optional`. Destapó el producto con signo, que se leía sin signo |
| ~~P1.4~~ | ~~`representation_traits<binnat>` y el `static_assert` al bicondicional de [ADR-011](docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md)~~ | ✅ **hecho** en `c73e55a` |
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
| **P2.6** | **La tercera combinación: `clang + libstdc++`** (el clang de UCRT64). Ya está instalado y no se usaba | Barato: amplía el recubrimiento sin tocar código |
| ~~P2.7~~ | ~~Desguace de benchmarks de algoritmo~~ | ✅ **las cuatro piezas hechas**: algoritmo/desenrollado, verosimilitud, código emitido (`scripts/bench_asm.py`) y coste teórico declarado |

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

### P0.6 (lo que queda) — Intel en el runner del CI

**En local ya funciona**: 55/55 con Intel 2026.1 (581,9 s). Lo que se arregló:

- `compiler_env.py` tenía la versión **cableada a `2025.3`** teniendo instaladas
  2025.3, 2026.0 y 2026.1. No era una decisión, era un descuido: cogía la más
  vieja de las tres. Ahora `INTEL_VERSION = "auto"` descubre y coge la más alta.
- Las dos versiones de 2026 **fallaban hasta para responder a `--version`** con
  `icpx: error #10026: error generating temporary file`, por culpa del `TMP`.
  Con `C:\msys64\tmp` —lo que hay en esta máquina, por MSYS2— revientan; con
  `%LOCALAPPDATA%\Temp` van bien. La 2025.3 no se ve afectada, que es por lo
  que el problema pasó desapercibido mientras la versión estuvo cableada a la
  vieja. El entorno aislado de Intel ahora lleva un `TMP` que Intel acepta.

**Queda el runner**, que es un problema distinto: allí `ONEAPI_ROOT` viene
vacío y `rscohn2/setup-oneapi` deja un `CMAKE_PREFIX_PATH` apuntando a
`/opt/intel/oneapi/...` —una ruta de Linux en un runner de Windows—. Con el
camino local resuelto, ahora se sabe qué hay que exigirle: una instalación real
bajo `C:\Program Files (x86)\Intel\oneAPI\compiler\<ver>\bin\icpx.exe`.

Y sigue en pie la decisión de **si Intel se queda en la matriz de release**
(P3.6): las cabeceras son idénticas en los cuatro compiladores, así que el
contenido útil del zip no cambia.

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
- **Los `run:` de GitHub Actions van con `bash -e`.** Un comando que devuelve
  distinto de cero **fuera de un `if`** mata el paso en el acto. Sacar un
  `timeout` de su `if` para guardar `$?` dejó el job de aarch64 muerto a mitad
  del bucle. La forma correcta es `code=0` y luego `cmd || code=$?`, que deja el
  comando comprobado y a la vez guarda el código.
- **Intel: `icpx` usa flags estilo GCC**, no de MSVC — los `/Qstd:c++20` y
  `/EHsc` son de `icx-cl`. Y necesita el entorno que monta `compiler_env.py`;
  invocarlo a pelo da «`'cstddef' file not found`» y no significa nada.
- **No se toca `compiler_env.py` con una compilación en marcha.** Cada test se
  compila en un proceso nuevo que relee el módulo, así que el cambio se cuela a
  mitad de la suite y el resultado sale mezclado sin que nada lo diga.
- **QEMU está en WSL** (`qemu-arm`, `qemu-aarch64`, y las toolchains cruzadas).
  Reproducir un fallo de ARM en local cuesta minutos; por el CI, media hora.
- **Para probar que una comprobación salta, primero se commitea el arreglo.**
  Al validar los `assert()` se rompió un test a propósito y se restauró con
  `git checkout --`, que deshizo la rotura **y también el arreglo**, porque
  todavía no estaba commiteado. El fichero volvió a quedar inerte y la prueba
  dijo «siguen inertes» cuando lo que fallaba era el procedimiento.
- Antes de cerrar sesión: `/guarda_y_sube`, que ejecuta los cuatro verificadores
  que exige el CI.


### P0.9 — Tests de guardas de include y de macros de configuración

**De dónde sale.** De que en tres sesiones han aparecido once señales que
mentían, y las que más caras han salido no eran fallos del código sino de la
**configuración**: una guarda de libc++ que causaba el problema que decía evitar
y dejaba al proyecto sin `nstd::is_integral` durante meses, un
`[[no_unique_address]]` que elegía la rama equivocada porque Intel define
`_MSC_VER` **y** `__clang__` a la vez, y un `-latomic` que se añadía a ciegas.
Ninguna de las tres la habría cazado un test de aritmética.

**Qué tiene que comprobar**, y todo en `static_assert` donde se pueda:

1. **Qué hay debajo, dicho en voz alta.** Compilador, versión y biblioteca
   estándar (`__GLIBCXX__` / `_LIBCPP_VERSION` / `_MSVC_STL_VERSION`). Que el
   test *imprima* en qué combinación corre es media batalla.
2. **Las macros propias resuelven a lo que deben** en esa combinación:
   `INTRINSICS_USES_MSVC_ABI` frente a `INTRINSICS_USES_GNU_ABI`,
   `NSTD_NO_UNIQUE_ADDRESS`, `INTRINSICS_IS_CONSTANT_EVALUATED`,
   `NSTD_TRAITS_PRIMARY_DEFINED`.
3. **Lo que existe, existe en todas las combinaciones**: `nstd::is_integral` y
   sus hermanas, `nstd::hash`, `nstd::numeric_limits`, para `wrap` **y** para
   `checked`. Es el agujero del 6 sep 2026, y como `static_assert` no se puede
   volver a abrir en silencio.
4. **Los contratos que dependen de la ABI**: `sizeof(fixed_int_t<4,…,wrap>) ==
   32`, `== 40` con `checked`, `is_standard_layout` en las dos. Hoy viven
   sueltos en la cabecera del tipo.
5. **Las guardas son idempotentes**: incluir dos veces cada header no cambia
   nada, y **el orden entre los dos ficheros de traits tampoco** — que es donde
   vive `NSTD_TRAITS_PRIMARY_DEFINED` y donde estaba el fallo.

**La vuelta de tuerca que lo hace útil:** el test debe **fallar si no reconoce la
combinación**, no pasar de largo. Una configuración nueva y no contemplada tiene
que salir en rojo. Si no, es otro verde que no significa nada.

### P2.6 — La tercera combinación: `clang + libstdc++`

Hay **tres** combinaciones compilador/biblioteca disponibles en esta máquina, y
solo se usaban dos. Medido el 6 sep 2026 con una sonda que imprime los macros:

| toolchain | compilador | biblioteca |
|---|---|---|
| `ucrt64/g++` | gcc 16.2.0 | libstdc++ 20260807 |
| `ucrt64/clang++` | clang 22.1.8 | **libstdc++ 20260807** ← la que faltaba |
| `clang64/clang++` | clang 22.1.8 | libc++ 220108 |

La tercera **ya está instalada**: es el clang de UCRT64, con el que se trabajó
media sesión sin caer en que era una configuración distinta. Añadirla es
cuestión de que `toolchains.json` la nombre y `make.py test` la acepte.

**La cuarta, `gcc + libc++`, no es viable en MinGW y se descarta.** No por culpa
del proyecto: un hola-mundo compila (`gcc 16.2.0 / libc++ 220108`, con
`-nostdinc++ -isystem …/include/c++/v1 -lc++`), pero con código real revienta
dentro de la propia libc++ —`__algorithm/equal.h` usa un `static` en función
`constexpr`, que es de C++23, y GCC lo rechaza en modo C++20; forzando C++23 cae
en `cstdlib`, porque UCRT no tiene el `aligned_alloc` de C11—. Con las cabeceras
de UCRT64 y con las de CLANG64, igual. Queda como job experimental del CI en
Linux si algún día interesa, nunca como requisito.


### P2.7 — Desguace de benchmarks de algoritmo

**De dónde sale.** De que la comparación Karatsuba/escolar ha medido tres cosas
distintas de lo que decía medir, en tres ocasiones:

| Cuándo | Decía | Medía |
|---|---|---|
| ago 2026 | Karatsuba gana 6,23× | contra un espantapájaros |
| ago–sep 2026 | Karatsuba gana 1,65× | un desenrollado contra un bucle |
| 6 sep 2026 | escolar desenrollado, 2,1 cyc/op en N=4 con Intel | el compilador se había llevado el trabajo |

Las tres se destaparon **mirando**, no por una alarma. Un número solo, sin nada
con qué contrastarlo, no puede delatarse a sí mismo. El desguace consiste en no
publicar nunca un número solo.

**Las cuatro piezas, y cuáles ya existen.**

**1. Aportación del algoritmo, aislada del desenrollado.** ✅ *hecho el 6 sep
2026*. Se miden **tres** implementaciones y no dos: la de la biblioteca, la
referencia escrita como bucle, y la misma referencia desenrollada por
construcción. Se publican `razon justa` (los dos lados desenrollados) y `aporte
del desenrollado` (bucle frente a desenrollado). Sin esto, el «1,65×» de
Karatsuba resultó ser 0,59×.

**2. Verosimilitud: el suelo físico.** ✅ *hecho el 6 sep 2026*. Se compara la
cifra medida con lo que la máquina no puede bajar: para el escolar de N limbos,
N(N+1)/2 productos por su coste mínimo. Por debajo del suelo, la medida **no se
publica y el benchmark sale con error**. Ojo con el suelo: `CycleTimer` usa
RDTSC, que cuenta a la frecuencia invariante del TSC y no a la del núcleo, así
que con turbo un ciclo real mide menos de un «ciclo» TSC.

**3. Aportación del compilador: contar el código emitido.** ✅ *hecho el 6 sep 2026*, en `scripts/bench_asm.py`. Lo
que destapó el fallo del desenrollado no fue el cronómetro sino el
`-S`: en GCC, N=4, Karatsuba salía con 119 instrucciones y **9** `mul`
—desenrollado— y la referencia con 61 y **1** `mul` —bucle—. Esa cuenta debe
tomarse automáticamente y publicarse junto al tiempo:

- compilar la sonda con `-S` (o `/FA` en MSVC), trocear por función y contar
  instrucciones, `mul`, `adc`/`sbb`, `mov` y `call`;
- **la señal es la discordancia**: si el tiempo dice 1,7× y el número de `mul`
  dice 0,9×, hay una tercera variable, y hay que nombrarla antes de publicar.
- `call` dentro de un núcleo numérico es aviso por sí solo: MSVC dejaba
  `kmul_full` fuera de línea.

**4. Coste teórico declarado, y su distancia con lo medido.** ✅ *hecho el 6 sep 2026*.
Cada algoritmo declara su cuenta de operaciones —escolar N(N+1)/2 productos;
Karatsuba T(N)=3·T(N/2)+O(N)— y el benchmark publica la razón **esperada** al
lado de la **medida**. La distancia entre ambas es el resultado interesante: es
donde viven el desenrollado, la presión de registros, la planificación y los
fallos de medida. Hoy esa distancia se calcula a mano y sólo cuando alguien
sospecha.

**Forma que propongo.** Un `benchs/bench_desguace.hpp` con el arnés —las tres
medidas, la verosimilitud y el coste declarado— y un `scripts/bench_asm.py` que
haga la parte 3, porque necesita invocar al compilador y trocear el ensamblador.
El benchmark de Karatsuba sería el primer cliente; `divmod` y las conversiones a
cadena, los siguientes.

**Regla que sale de esto, y que vale aunque no se escriba una línea de código:**
un benchmark de algoritmo no publica una razón sin decir **contra qué** y
**compilada cómo**. Las tres veces que esto falló, la razón estaba bien
calculada; lo que estaba mal era el otro lado de la división.
