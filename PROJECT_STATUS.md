# PROJECT STATUS

**Last Updated:** 25 August 2026
**Versión:** v1.90.4 · **rama** `phase-1.80` · árbol limpio, todo en `origin`

> Instantánea **del estado actual**, y solo eso. No acumula historia: lo ya hecho
> vive en [`CHANGELOG.md`](CHANGELOG.md); lo que viene, en [`ROADMAP.md`](ROADMAP.md)
> y [`NEXT_STEPS.md`](NEXT_STEPS.md); el porqué de cada decisión, en
> [`docs/decisions/`](docs/decisions/README.md).

---

## En una línea

`fixed_int_t<N, Sign, Form>` está **terminado** como entero de N × 64 bits, la
suite está en verde en los 24 jobs del CI, y **el diseño de la 2.0 está cerrado
sin cuestiones abiertas**: falta escribirlo.

## Verificación — 25 August 2026

| Comprobación | Resultado |
|---|---|
| CI sobre `HEAD` (`4d3ab66`) | ✅ **24/24 jobs** |
| `python make.py test gcc release-O2` | **55/55 ficheros** (161,3 s, GCC 16.2 ucrt64) |
| `scripts/check_headers_selfcontained.py` | **31/31** headers compilan aislados |
| Clang **sin flags no estándar** | 31/31 headers + suite ✅ |
| `clang-format --dry-run --Werror`, 97 ficheros | 0 sin formatear |
| `scripts/check_docs_consistency.py` | **7/7** (9/9 con `--doxygen`) |
| Avisos de Doxygen desde `include/` | **0** |

Compiladores en verde: GCC 13–16, Clang 18–22, MSVC 19.5x, Intel ICX. Arcos:
x86-64, x86-32, ARM64, ARM32 y RISC-V 64.

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
| Headers | 31 (`include/`, con `algorithms/` e `intrinsics/`) |
| Tests | 55 ficheros |
| Scripts vivos | 16 (llegaron a ser 47) |
| Documentos de raíz | 12, 9.834 renglones — de los que 6.271 son el `CHANGELOG` |
| ADR | **14**, ninguna decisión tomada sin documentar |

## Cifras de la suite

| Test | Asserts |
|---|---|
| `test_fixed_differential` | **46.800** comprobaciones contra oráculo independiente |
| `test_fixed_signed` | 966 |
| `test_fixed_basic` | 843 |
| `test_fixed_vs_param` | 804 |
| `test_fixed_divmod` | 218 + 30 `static_assert` de constexpr |
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
| Knuth D, Karatsuba (N=4/8, **medido: 1,65× y 1,48×**), Granlund-Montgomery | ✅ |
| Magnitud-Signo y Exceso-K **en `int128_param_t`** | ✅ |
| Magnitud-Signo y Exceso-K **en `fixed_int_t`** | ❌ pendiente — [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md) |
| Política de desbordamiento como parámetro | ❌ diseñada, sin escribir — ADR-007 a 010 |
| `checked_div` y las tres `saturating_*` en `fixed_int_t` | ❌ pendiente — [ADR-009](docs/decisions/ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md) |

## Deuda anotada

- **La release de v1.90.1 no está publicada** (arriba). Es lo único que está roto.
- `intrinsics/compiler_detection.hpp` (29 avisos) y
  `algorithms/karatsuba.hpp` (3) están fuera del ámbito de ADR-014 pero no de
  `int128_param_*`: habrá que decidir si entran.
- **`python make.py build` devuelve 0 aunque falle el enlazado.** Comprobado
  con `benchmark_vs_builtin`, que necesita GMP: el enlazador falla, make.py
  imprime «Build complete» y sale con 0. Significa que **`make.py build` no
  sirve como puerta** en ningún guion ni workflow. Es lo más serio de esta lista.
- **Documentación de `int128_param_*`: 349 avisos**, exentos a propósito hasta
  que ADR-006 retire el tipo.
- **Karatsuba solo está medido en GCC.** Faltan Clang, MSVC e Intel.
- **El bucle escolar de `operator*` es un 14 % más lento que una copia idéntica
  suya escrita como función libre**, en N=3. Lo destapó el control del benchmark
  de Karatsuba y no está explicado.
- Falta la especialización de `representation_traits<binnat>`. Su contenido ya
  está determinado por
  [ADR-011](docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md); es escribirla.
