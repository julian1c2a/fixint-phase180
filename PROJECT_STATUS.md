# PROJECT STATUS

**Last Updated:** 18 September 2026
**Versión:** v1.90.4 · **rama** `phase-1.80` · árbol limpio, todo en `origin`

> Instantánea **del estado actual**, y solo eso. No acumula historia: lo ya hecho
> vive en [`CHANGELOG.md`](CHANGELOG.md); lo que viene, en [`ROADMAP.md`](ROADMAP.md)
> y [`NEXT_STEPS.md`](NEXT_STEPS.md); el porqué de cada decisión, en
> [`docs/decisions/`](docs/decisions/README.md).

---

## En una línea

`fixed_int_t<N, Sign, Form, Policy>` está **terminado** como entero de N × 64
bits, **v1.90.4 está publicada**, y **el diseño de la 2.0 está cerrado sin
cuestiones abiertas**: se está escribiendo. De P1 van cerradas P1.1 a P1.4 y
**los tramos 1 y 2 de P1.5** salvo lo anotado; del inventario de retirada de
`int128_param_t`
([ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md)) van
**7 filas de 11**, y dos más esperan una decisión escrita, no trabajo.

**El frente de la división está abierto, pero Möller–Granlund queda cerrado**
(P2.10, 18 sep). Knuth D vive en su propia capa medible, con la estimación del
dígito del cociente como perilla. La **2/1** hace que el coste por limbo del
divisor de un limbo caiga de **~84 a ~17 ciclos** (hasta 4,97×); la **3/2** da
hasta **3,33×** en la estimación de q̂ con divisores cortos. Queda
**Burnikel–Ziegler**.

Las dos mitades trajeron el mismo aviso, y el proyecto lo tenía escrito al revés:
Möller–Granlund **sí tiene umbral**, porque el recíproco es un coste fijo por
llamada y el ahorro es por iteración. Sin iteraciones sobre las que repartirlo es
coste puro: la 2/1 pierde 4× en `N=1`, y la 3/2 pierde el 47% cuando el divisor
ocupa la anchura entera —que es justo lo que dan dos operandos aleatorios—.
**Los dos casos malos vivían fuera del barrido que se había hecho.**

**Y el 18 sep se recogieron dos ganancias que no eran algoritmos**: `mul_wide`
ensanchaba antes de multiplicar (**2,3×–2,8×**) y `checked_mul` multiplicaba dos
veces, la primera con un escolar cuadrático (**1,4×–1,9×, creciendo con N**).

**El frente de la multiplicación está cerrado** (17 sep 2026): cuatro algoritmos
en `include/algorithms/mul_kernels.hpp` —escolar desenrollado, Karatsuba con
reparto equilibrado, cuadrado y Toom-3— **con cada umbral medido**, y la mitad
alta del rango admitido (N = 64…4096) cubierta por primera vez.

## Verificación — 18 September 2026

| Comprobación | Resultado |
|---|---|
| `python make.py test gcc release-O2` | **64/64 ficheros** (GCC 16.2 ucrt64, libstdc++) |
| `python make.py test clang release-O2` | **64/64 ficheros** (clang 22.1.8 clang64, **libc++**) |
| `python make.py test clang-libstdcxx release-O2` | **64/64 ficheros** (clang 22.1.8 ucrt64, **libstdc++**) |
| `python make.py test msvc release-O2` | **64/64 ficheros** |
| `python make.py test intel release-O2` | **64/64 ficheros** (oneAPI 2026.1) |
| CI sobre `f959f53` | **24/24 jobs**, matriz completa: gcc-13/14/15/16, clang-18/19/20/21/22, ARM64, arm32, riscv64, i686 |
| `scripts/check_headers_selfcontained.py` | **34/34** headers aislados, en los **tres** de MinGW: gcc, clang y clang-libstdcxx |
| `clang-format --dry-run --Werror`, 104 ficheros | 0 sin formatear, con la 21.1.8 **y** con la 22.1.8 |
| `scripts/check_docs_consistency.py --doxygen` | **9/9** — es la orden que corre el CI |
| `scripts/check_matriz_paridad.py` | **192/192 celdas**, todas como declaran. **No está en el CI**; ver [MATRIZ_DE_PARIDAD](docs/MATRIZ_DE_PARIDAD.md) |
| Avisos de cobertura de Doxygen desde `include/` | **499** (local, doxygen 1.18.0) · **518** (CI, doxygen 1.9.8) |

> **La cifra de Doxygen no ha empeorado: antes no se medía.** Hasta el 25 ago el
> `Doxyfile` tenía `EXTRACT_ALL = YES` y `WARN_IF_UNDOCUMENTED = NO`, con lo que
> era **imposible** que apareciera un aviso de cobertura, y el «0» que se
> publicaba no significaba nada. Ver
> [ADR-014](docs/decisions/ADR-014-cobertura-de-doxygen.md).

Compiladores: GCC 13–16, Clang 18–22, MSVC 19.5x e **Intel ICX 2026.1**
(55/55 en local sobre Windows; en el runner del CI sigue sin instalarse, ver la
deuda). Arcos: x86-64, x86-32, ARM64, ARM32 y RISC-V 64.

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
| Headers | 34 (`include/`, con `algorithms/` e `intrinsics/`) |
| Tests | 64 ficheros |
| Scripts vivos | 20 (llegaron a ser 47) |
| Documentos de raíz | 12, 9.834 renglones — de los que 6.271 son el `CHANGELOG` |
| ADR | **17**, ninguna decisión tomada sin documentar |

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
| Magnitud-Signo y Exceso-K **en `int128_param_t`** | ✅ |
| Magnitud-Signo y Exceso-K **en `fixed_int_t`** | ❌ pendiente — [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md) |
| Política de desbordamiento como parámetro | ✅ **escrita** (P1.1–P1.3): almacenamiento, propagación, orden y las `checked_*`/`saturating_*` |
| `checked_div` y las tres `saturating_*` en `fixed_int_t` | ✅ (5 sep 2026) — desbloquea [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md) |

## Deuda anotada

- `intrinsics/compiler_detection.hpp` (29 avisos) y
  `algorithms/karatsuba.hpp` (3) están fuera del ámbito de ADR-014 pero no de
  `int128_param_*`: habrá que decidir si entran.
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
- **Documentación de `int128_param_*`: 349 avisos**, exentos a propósito hasta
  que ADR-006 retire el tipo.
- **Karatsuba solo está medido en GCC.** Faltan Clang, MSVC e Intel.
- **El bucle escolar de `operator*` es un 14 % más lento que una copia idéntica
  suya escrita como función libre**, en N=3. Lo destapó el control del benchmark
  de Karatsuba y no está explicado. Falta barrer N impares y N pares que no usen
  Karatsuba, para saber si es cosa de la paridad o del tamaño.
- Falta la especialización de `representation_traits<binnat>`. Su contenido ya
  está determinado por
  [ADR-011](docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md); es escribirla.
