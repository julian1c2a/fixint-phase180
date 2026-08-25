# int128 / `fixed_int_t<N>`

**Enteros de anchura fija de N × 64 bits para C++20, solo cabeceras.**

[![BSL-1.0](https://img.shields.io/badge/license-BSL--1.0-blue.svg)](LICENSE.txt)

```cpp
#include "fixed_width_int_t.hpp"
using namespace nstd;

constexpr uint256_fixed_t a{"115792089237316195423570985008687907853269984665640564039457584007913129639935"};
constexpr auto b = a / 7;              // constexpr, division incluida
static_assert(b * 7 + a % 7 == a);
```

---

## Qué es

Una biblioteca de enteros de anchura fija mayor que la que ofrece el lenguaje:
`fixed_int_t<N, Sign, Form>` es un entero de **N limbos de 64 bits**, con o sin
signo, cuya aritmética imita la de los enteros built-in de C++ —modular, sin
excepciones, `constexpr` de principio a fin— y llega hasta donde no llegan ni
`__int128` ni las bibliotecas de precisión arbitraria: **anchura fija, coste
predecible, cero asignaciones de memoria**.

Los tamaños habituales tienen nombre propio:

| Alias | Bits | | Alias | Bits |
|---|---:|---|---|---:|
| `uint64_fixed_t` | 64 | | `int64_fixed_t` | 64 |
| `uint128_fixed_t` | 128 | | `int128_fixed_t` | 128 |
| `uint256_fixed_t` | 256 | | `int256_fixed_t` | 256 |
| `uint512_fixed_t` | 512 | | `int512_fixed_t` | 512 |
| `uint1024_fixed_t` | 1024 | | `int1024_fixed_t` | 1024 |

## Para qué se hizo

No para competir con `__int128`. El proyecto nació como **sustrato para
investigar codificaciones de cara a implementar coma flotante**, y de ahí viene
lo que lo distingue: la representación interna es un **parámetro de plantilla**,
no una decisión fija.

| Forma | Qué es | Para qué |
|---|---|---|
| `binnat` | binario natural | el caso sin signo |
| `twos_complement` | complemento a dos | lo que hace el hardware |
| `magnitude_sign` | bit de signo + magnitud | **la mantisa** de IEEE-754 |
| `excess_k` | valor sesgado, real = almacenado − k | **el exponente** de IEEE-754 |

Las dos representaciones «raras» no son un capricho académico: son las dos
piezas de un `float`. El porqué está en
[ADR-005](docs/decisions/ADR-005-representacion-como-parametro-de-plantilla.md).

## Qué sabe hacer

- **Aritmética modular completa y `constexpr`** — división y módulo incluidos,
  comprobado con `static_assert`.
- **Interoperabilidad signed/unsigned al estilo de los built-in**, trampas
  incluidas: `i<N>::min() > u<N>{0}` da `true`, igual que en C++.
- **Anchuras mezcladas**: `uint_fixed_t<4> + uint_fixed_t<8>` compila y gana la
  anchura mayor, sin perder bits.
- **Algoritmos**: división por Knuth D, multiplicación de Karatsuba en N=4 y N=8
  (1,5×–1,7× sobre el método escolar,
  [medido](docs/PERFORMANCE.md#multiplicación--karatsuba-frente-al-método-escolar)),
  división por constante Granlund-Montgomery.
- **Integración con la STL**: `iostreams`, `std::format`, `std::hash`,
  `std::numeric_limits`, `<=>`.
- **Cadenas en bases 2..36**, con `try_from_string()` que no lanza.
- **Sin asignaciones de memoria** y trivialmente copiable: `sizeof` es
  exactamente `8N` bytes, y `std::bit_cast` funciona.

## Decisiones de diseño

Las que explican por qué la biblioteca es como es están en
**[`docs/decisions/`](docs/decisions/README.md)**. Las cinco fundacionales:

| | |
|---|---|
| [ADR-001](docs/decisions/ADR-001-constructores-y-conversiones-explicitos.md) | Todos los constructores y conversiones son `explicit` |
| [ADR-002](docs/decisions/ADR-002-almacenamiento-little-endian-de-limbos.md) | Los limbos se almacenan en orden little-endian |
| [ADR-003](docs/decisions/ADR-003-std-byte-para-buffers.md) | Los buffers de bytes son `std::byte` |
| [ADR-004](docs/decisions/ADR-004-sin-excepciones-en-el-nucleo.md) | La aritmética no lanza |
| [ADR-005](docs/decisions/ADR-005-representacion-como-parametro-de-plantilla.md) | La representación es un parámetro de plantilla |

## Cómo se usa

Es **solo cabeceras**. No hay que compilar nada: basta con añadir `include/` a
la ruta de inclusión.

```bash
g++ -std=c++20 -I include mi_programa.cpp
```

Requisitos: **C++20**. Probada en GCC 13–16, Clang 18–22, MSVC 19.5x e Intel
ICX, sobre x86-64, x86-32, ARM64, ARM32 y RISC-V 64.

## Cómo se construye el proyecto

La capa canónica es `make.py`. El `Makefile` es un envoltorio sobre ella.

```bash
python make.py test                    # suite completa, todos los compiladores
python make.py test gcc release-O2     # un compilador y un modo
python make.py bench                   # benchmarks
python make.py --help                  # el resto

make test COMPILER=gcc MODE=release-O2 # lo mismo, via Makefile
```

## El otro tipo, `int128_param_t`

La biblioteca contiene todavía un segundo tipo, anterior, fijo a 128 bits, que
es donde hoy viven Magnitud-Signo y Exceso-K. **Está en retirada**: toda su
funcionalidad se replica en `fixed_int_t` y después desaparece. El plan y el
inventario de paridad están en
[ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md).

Para código nuevo, usar `fixed_int_t` y sus alias.

## Dónde está cada cosa

| Busca | Está en |
|---|---|
| Estado actual del proyecto | [`PROJECT_STATUS.md`](PROJECT_STATUS.md) |
| Qué viene, a largo y a corto | [`ROADMAP.md`](ROADMAP.md) · [`NEXT_STEPS.md`](NEXT_STEPS.md) |
| Historia de todo lo hecho | [`CHANGELOG.md`](CHANGELOG.md) |
| Decisiones y su porqué | [`docs/decisions/`](docs/decisions/README.md) |
| Referencia de la API | [`docs/`](docs/) — 19 documentos estilo cppreference |
| Cifras de rendimiento | [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) |
| Convenciones de código | [`STYLE_CONVENTIONS.md`](STYLE_CONVENTIONS.md) · [`NAMING_CONVENTIONS.md`](NAMING_CONVENTIONS.md) |
| Guía de desarrollo completa | [`AI-GUIDE.md`](AI-GUIDE.md) |
| Chuleta de una pantalla | [`QUICK_REFERENCE.md`](QUICK_REFERENCE.md) |

## Estructura

```
include/          31 cabeceras; fixed_width_int_t.hpp es el tipo insignia
tests/            55 ficheros de test
benchs/           benchmarks (RDTSC, rondas intercaladas, minimo)
demos/            ejemplos de uso
docs/             referencia de la API y decisiones
scripts/          16 herramientas de construccion y verificacion
make.py           capa canonica de construccion
```

## Contribuir

Ver [`CONTRIBUTING.md`](CONTRIBUTING.md). Para vulnerabilidades,
[`SECURITY.md`](SECURITY.md).

## Licencia

[Boost Software License 1.0](LICENSE.txt). Uso libre, comercial incluido, sin
obligación de atribuir en los binarios.

---

**Autor:** Julián Calderón Almendros ·
[julian.calderon.almendros@gmail.com](mailto:julian.calderon.almendros@gmail.com)
