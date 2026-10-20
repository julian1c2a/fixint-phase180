# PROJECT STATUS

**Last Updated:** 20 October 2026
**Versión:** v1.90.4 · **rama** `phase-1.90` *(abierta el 27 sep)* · árbol limpio, todo en `origin`

> Instantánea **del estado actual**, y solo eso. No acumula historia: lo ya hecho
> vive en [`CHANGELOG.md`](CHANGELOG.md); lo que viene, en [`ROADMAP.md`](ROADMAP.md)
> y [`NEXT_STEPS.md`](NEXT_STEPS.md); el porqué de cada decisión, en
> [`docs/decisions/`](docs/decisions/README.md).

---

## En una línea

`fixed_int_t<N, Sign, Form, Policy>` está **terminado** como entero de N × 64
bits, **v1.90.4 está publicada**, y **el diseño de la 2.0 está cerrado**. La
**multiplicación** está cerrada (17 sep: escolar desenrollado, Karatsuba
equilibrado, cuadrado y Toom-3, cada umbral medido) y la **división** también,
por ahora: Knuth D en su capa medible, Möller–Granlund 2/1 y 3/2, y
Burnikel–Ziegler aparcado con medida
([ADR-016](docs/decisions/ADR-016-burnikel-ziegler-aparcado-por-medida.md)).

**La 1.90 está en su fase 0: afinar el banco ANTES de borrar `int128_param_*` y
renombrar** ([ADR-023](docs/decisions/ADR-023-sin-sufijo-t-en-las-plantillas-de-clase.md),
[ADR-024](docs/decisions/ADR-024-el-nombre-del-tipo-de-punto-fijo.md)). El banco
es el instrumento con el que se va a juzgar la 1.90, y no estaba calibrado.
Desde el 27 sep se han cerrado siete tareas; las tres que más cambian lo que se
puede afirmar:

- el arnés **mide lo que pasa DURANTE la toma**: un solo proceso compitiendo
  infla todas las cifras un 17 %, y la dispersión no lo ve (P2.22);
- el banco de comparación **mide lo mismo en todos los tipos**, con el orden de
  las variantes al azar en cada ronda. La división publicada medía `1/12345`
  (P2.25);
- la toma de referencia **medirá el «antes» de todo lo que el borrado se lleva**
  (P2.27).

**Falta medir, y se medirá en una máquina dedicada**: un mini PC con AMD Ryzen 7
5825U y Ubuntu Server, con el turbo apagado. Los guiones de preparación y de
sesión y los tres experimentos de validación están escritos y **probados en un
Ubuntu 24.04 limpio** (P2.24); falta instalar la máquina. Después, por este
orden: E1–E3, la toma de referencia, **P2.26** —el tipo nuevo pierde contra el
viejo en desplazamiento, comparación y lectura decimal, y borrar el viejo así
sería una regresión— y entonces el borrado y el renombrado. Ver
[NEXT_STEPS](NEXT_STEPS.md), «La plataforma de medida dedicada».

## Verificación — 20 October 2026

| Comprobación | Resultado |
|---|---|
| `python make.py test gcc release-O2` | **73/73 ficheros** (GCC 16.2 ucrt64, libstdc++), **medido el 20 oct** |
| `python make.py test clang release-O2` | **73/73 ficheros** (clang 23.1.3 clang64, **libc++**), 20 oct |
| `python make.py test clang-libstdcxx release-O2` | **73/73 ficheros** (clang 23.1.3 ucrt64, **libstdc++**), 20 oct |
| `python make.py test msvc release-O2` | **73/73 ficheros**, 20 oct |
| `python make.py test intel release-O2` | **73/73 ficheros** (oneAPI), 20 oct |
| CI sobre `88d23f4` | **25/25 jobs, cero fallos** (contados con `gh run view`, no supuestos), matriz completa: gcc-13/14/15/16, clang-18/19/20/21/22, ARM64, arm32, riscv64, i686, **Intel ICX**, los benchmarks compilando y las pruebas de los guiones de Python |
| `scripts/check_headers_selfcontained.py` | **40/40** headers aislados con GCC 16.2, 20 oct (el 23 sep, también con clang y clang-libstdcxx) |
| `clang-format --dry-run --Werror`, **146** ficheros | 0 sin formatear, con la **21** (la del CI, en WSL) **y** con la **23.1.3** (la de MSYS2 hoy), 20 oct |
| `scripts/check_docs_consistency.py --doxygen` | **14/14** — es la orden que corre el CI |
| `python make.py wsl` | **73/73 en las tres familias**: g++, clang e **Intel oneAPI**, sin avisos, 20 oct |
| Pruebas de los guiones (`scripts/tests`) | **81** en Linux; en Windows se saltan las 13 que necesitan bash POSIX |
| Preparación de la máquina de medida (`scripts/condiciones_benchmark/contenedor/prueba.sh`) | **32/32** en un Ubuntu 24.04 limpio, 20 oct |
| `scripts/check_matriz_paridad.py` | **348/348 celdas** (47 capacidades × **6** columnas, más 2 políticas reservadas y **13 sondas del punto fijo**, ocho que tienen que compilar y cinco que no), todas como declaran. **No está en el CI**; ver [MATRIZ_DE_PARIDAD](docs/MATRIZ_DE_PARIDAD.md) |
| Avisos de cobertura de Doxygen desde `include/` | **257** (local, doxygen 1.18.0), y el techo de ADR-014 baja a 257 con ellos (23 sep, P3.7). **Son todos de `int128_param_*`**: el código que se queda está a **cero**, headers internos incluidos. Total de doxygen **525 → 270**, y los 13 restantes son enlaces válidos en el repo que el sitio generado no resuelve |

> **La cifra de Doxygen no ha empeorado: antes no se medía.** Hasta el 25 ago el
> `Doxyfile` tenía `EXTRACT_ALL = YES` y `WARN_IF_UNDOCUMENTED = NO`, con lo que
> era **imposible** que apareciera un aviso de cobertura, y el «0» que se
> publicaba no significaba nada. Ver
> [ADR-014](docs/decisions/ADR-014-cobertura-de-doxygen.md).

Compiladores: GCC 13–16, Clang 18–23, MSVC 19.5x e **Intel ICX 2026.1**
(73/73 en local sobre Windows, y **Intel oneAPI 2026.0 también en WSL**). Arcos: x86-64, x86-32, ARM64, ARM32 y RISC-V 64.

> **ARM ya no lleva reservas** (26 ago 2026). Los cuatro fallos que las
> tolerancias del CI tapaban eran un bug de portabilidad real —`_umul128` en la
> rama `#else` de `test_fixed_karatsuba.cpp`, duplicado— y **tres tests
> correctos a los que no les daba tiempo**. Reproducido bajo QEMU en WSL:
> `test_param_divmod` tarda 202 s en arm32 y el límite estaba en 180.
> Arreglado el bug, subidos los límites a 600 s y **retiradas las dos
> tolerancias**: `cross-arm32` pasa de 54/55 compilando a **55/55 compilando y
> 55/55 pasando**.

> **`v1.90.4` es la primera release publicada del proyecto**: tres zips —gcc,
> clang y msvc— en
> [releases/tag/v1.90.4](https://github.com/julian1c2a/fixint-phase180/releases/tag/v1.90.4).
> Hicieron falta cuatro intentos; `v1.90.1`, `.2` y `.3` se quedan sin release y
> sin mover, por [ADR-012](docs/decisions/ADR-012-no-se-mueve-un-tag-publicado.md).
>
> **Intel no se publica**: no funciona en Windows en este repositorio y su job
> sale en rojo a propósito. Queda por decidir si se retira de la matriz.

## Volumen

| | |
|---|---|
| Headers | 40 (`include/`, con `algorithms/` e `intrinsics/`) |
| Tests | 73 ficheros |
| Scripts vivos | **41** guiones (`.py`, `.sh`, `.bash`, `.ps1`, `.bat`) rastreados en `scripts/`, sin contar `scripts/archive` (32) ni `scripts/tests`; contados el 20 oct con `git ls-files`. Decia 25 con un criterio que no estaba escrito del todo, y no se podia recontar igual: este si |
| Documentos de raíz | 12, **13.974** renglones — de los que la mayoría son el `CHANGELOG` |
| ADR | **24**, ninguna decisión tomada sin documentar |

## Cifras de la suite

| Test | Asserts |
|---|---|
| `test_fixed_differential` | **46.800** comprobaciones contra oráculo independiente |
| `test_fixed_signed` | 966 |
| `test_fixed_basic` | 843 |
| `test_fixed_vs_param` | 804 |
| `test_fixed_divmod` | 218 + 30 `static_assert` de constexpr |
| `test_fixed_bits_numeric` | 85 `static_assert` + 10 en ejecución, de las que 2 cruzan **400.000 pares al azar** |
| `test_cross_operators` | 206 |
| `test_fixed_string_io` | 104 |
| `test_fixed_stl_integration` | 95 |
| `test_fixed_karatsuba` | 49 |

Los `test_sweep_*` añaden del orden de 588 millones de comprobaciones de
propiedades sobre `int128_param_t`.

## Qué está terminado y qué no

| | Estado |
|---|---|
| Aritmética modular completa y `constexpr`, división y módulo incluidos | ✅ |
| Interop signed/unsigned al estilo de los built-in | ✅ |
| iostreams, `std::format`, `std::hash`, cadena en bases 2..36 | ✅ |
| Knuth D, Karatsuba (N≥32), escolar desenrollado (N=3..16), Granlund-Montgomery | ✅ · **2× a 4,9× más rápido** desde el 6 sep 2026, ver [PERFORMANCE](docs/PERFORMANCE.md) |
| Magnitud-Signo y Exceso-K **en `int128_param_t`** | ⚠️ **incompleto, y el ✅ era falso** (medido 21 sep): en Exceso-K los **desplazamientos** dan basura —no hay rama para esa representación— y en Magnitud-Signo **`~` está roto** (invierte la magnitud y el `~` pone a uno el bit de signo). Los tests no lo cazaban porque comprueban propiedades estructurales, no valores. **No se arregla**: lo retira [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md); ver [ADR-018](docs/decisions/ADR-018-la-representacion-no-es-observable.md) |
| Magnitud-Signo y Exceso-K **en `fixed_int_t`** | ✅ (22 sep 2026, P1.5 tramo 3) — y a diferencia del tipo viejo, **comprobado por valor**: 16 624 comprobaciones cruzadas contra complemento a dos. Se comportan igual que él en todo ([ADR-018](docs/decisions/ADR-018-la-representacion-no-es-observable.md)) |
| Política de desbordamiento como parámetro | ✅ **escrita** (P1.1–P1.3): almacenamiento, propagación, orden y las `checked_*`/`saturating_*` |
| `checked_div` y las tres `saturating_*` en `fixed_int_t` | ✅ (5 sep 2026) — desbloquea [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md) |

## Deuda anotada

- ✅ `intrinsics/compiler_detection.hpp` (29 avisos) y `algorithms/karatsuba.hpp`
  (3) estaban fuera del ámbito de ADR-014 pero no eran de `int128_param_*`. **Se
  documentaron igual el 23 sep**, junto con `div_kernels` (9), `mul_kernels` (3)
  y `arithmetic_operations` (1): 45 avisos que no contaban contra el techo pero
  son el código que de verdad ejecutan los núcleos. La decisión de si entran al
  ámbito ya no urge, porque están a cero.
- ✅ **Los 466 avisos del ámbito público no bajaban solos a cero con P1.5**, y
  por eso existió P3.7. Contado el 21 sep: 257 de `int128_param_*` —esos sí los
  retira P1.5— y **209 del tipo nuevo** (177 en `fixed_width_int_t.hpp`, 32 en
  `fixed_int_limits.hpp`), que era trabajo propio sin apuntar en ninguna lista.
  **Escritos el 23 sep**: el techo queda en 257 y P3.2 (`WARN_AS_ERROR = YES`)
  ya solo espera a la retirada, no a trabajo propio.
- ✅ **`make.py build` ya detecta el fallo de enlazado** (25 ago 2026). Eran dos
  fallos en `scripts/build_generic.py`: el código de salida no se propagaba, y el
  criterio era `returncode == 0 **o** existe el binario`, con un `or` que dejaba
  que un binario viejo disfrazase un fallo nuevo. Ahora exige **las dos cosas** y
  `main()` sale con 1. Comprobado con los tres casos: el que fallaba, uno bueno,
  y uno roto a propósito con binario previo presente.
### Lo que destapó la auditoría de señales (25 ago 2026)

Se revisaron los 11 guiones de `scripts/`, los 55 tests y los 9 jobs del CI,
buscando lo mismo: **quién puede decir «bien» sin haberlo comprobado.**

- ✅ **Los 47 `assert()` ya no son inertes** (26 ago 2026). `-DNDEBUG` los
  borraba en los modos release, que son los que se ejecutan, así que
  `test_param_iostreams` (35), `test_representation_conversions` (8) y
  `test_phase5_operators` (4) pasaban sin verificar nada. Ahora llevan
  `#undef NDEBUG` antes de `<cassert>`, **comprobado rompiendo un `assert` en
  cada uno**: los tres abortan. Queda pendiente convertirlos a la macro `TEST()`
  de los otros 51 ficheros, que sí da recuento y no se para en el primer fallo.
- ✅ **`tests/test_template_type.cpp` ya comprueba** (26 ago 2026). Las dos
  identidades de tipo pasan a `static_assert` —si dejan de cumplirse, **no
  compila**— y lo que depende de la ejecución lleva contador. Antes imprimía
  «[OK] … passed» pasara lo que pasara.

- ✅ **Los jobs cruzados ya pueden fallar** (26 ago 2026). `cross-arm32` no
  tenía ninguna puerta —ni en compilación ni en ejecución— y encima llevaba
  `continue-on-error: true`; `aarch64` y `riscv64` toleraban un 10 % de fallos,
  que con 2 sobre 55 nunca saltaba. Retiradas las tres tolerancias y subidos los
  límites de QEMU de 120/180 a **600 s**, con las medidas escritas en el
  workflow. No era inestabilidad: era un límite mal calibrado.

- Los otros dos `continue-on-error` del CI (`static-cppcheck`, `static-clang-tidy`)
  **sí están documentados** como no bloqueantes. Correcto.

- `scripts/init_project.py` no devuelve código de error nunca. Riesgo bajo: no
  se usa como puerta.

**Los 51 tests restantes están bien**: devuelven código distinto de cero cuando
fallan, comprobado uno a uno.

- ✅ **Intel funciona en local** (26 ago 2026): 55/55 con **2026.1**. La versión
  estaba **cableada a `2025.3`** teniendo tres instaladas, y las dos de 2026
  fallaban por el `TMP` —`error #10026`— con el `C:\msys64\tmp` de MSYS2.
  Corregidas las dos cosas en `compiler_env.py`; el porqué de coger la más
  nueva en vez de fijar una,
  en [ADR-015](docs/decisions/ADR-015-version-de-intel-oneapi.md).
- **Intel sigue sin funcionar en el runner del CI**, que es otro problema:
  `ONEAPI_ROOT` viene vacío y la acción de terceros deja un `CMAKE_PREFIX_PATH`
  de Linux en un runner de Windows.
- **Documentación de `int128_param_*`: 257 avisos** (decía 349: es la cifra de
  antes del 23 sep), exentos a propósito hasta que ADR-006 retire el tipo.
- ✅ **Karatsuba ya está medido en los cuatro compiladores** (6 sep 2026, ver
  [PERFORMANCE](docs/PERFORMANCE.md), «Los cuatro compiladores»). Esta línea
  decía que sólo en GCC hasta el 20 oct, seis semanas después.
- **El bucle escolar de `operator*` es un 14 % más lento que una copia idéntica
  suya escrita como función libre**, en N=3. Lo destapó el control del benchmark
  de Karatsuba y no está explicado. Falta barrer N impares y N pares que no usen
  Karatsuba, para saber si es cosa de la paridad o del tamaño.
- Falta la especialización de `representation_traits<binnat>`. Su contenido ya
  está determinado por
  [ADR-011](docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md); es escribirla.
