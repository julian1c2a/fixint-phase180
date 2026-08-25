# Referencia rápida

**v1.90.1** · documento **derivado**: no es fuente de verdad de nada. Cada cosa
enlaza a donde se decide de verdad.

## Tipos

```cpp
#include "fixed_width_int_t.hpp"          // nstd::fixed_int_t<N, Sign, Form>
```

| Alias | Bits | | Alias | Bits |
|---|---:|---|---|---:|
| `uint64_fixed_t` | 64 | | `int64_fixed_t` | 64 |
| `uint128_fixed_t` | 128 | | `int128_fixed_t` | 128 |
| `uint256_fixed_t` | 256 | | `int256_fixed_t` | 256 |
| `uint512_fixed_t` | 512 | | `int512_fixed_t` | 512 |

Genéricos: `uint_fixed_t<N>` e `int_fixed_t<N>`, con N limbos de 64 bits.

## Operaciones

| | |
|---|---|
| Aritmética | `+ - * / %`, unario `-` y `+`, `++ --`, y sus `op=` |
| Bits | `~ & \| ^ << >>`, y sus `op=` |
| Comparación | `== != < <= > >=`, `<=>` |
| Limbos | `limb(i)` · `set_limb(i,v)` · `limbs()` · `limbs_ref()` |
| Consultas | `is_zero()` `is_negative()` `is_power_of_two()` `bit_width()` `popcount()` `count_leading_zeros()` `count_trailing_zeros()` |
| Constructores | `zero()` `one()` `max()` `min()` |
| Cadena | `to_string(base = 10)` · `from_string(s, base = 10)` · `try_from_string(s, base = 10)` |
| Superiores | `mul_wide` `pow` `sqrt` `gcd` `lcm` `checked_add/sub/mul` |

**Todo es `constexpr`**, división y módulo incluidos.

## Semántica en una línea

| | |
|---|---|
| Desbordamiento | modular, 2^(64N) — como los built-in de C++ |
| División | trunca hacia cero; el resto toma el signo del dividendo |
| `min() / -1` | envuelve a `min()`, no es UB |
| División por cero | `std::domain_error` · en contexto constante, error de compilación |
| `from_string` fuera de rango | `std::out_of_range` |
| Desde `float`: NaN / +inf / −inf | `0` / `max()` / `min()` |
| Desplazamiento ≥ 64N | `0`, o relleno de signo en `>>` con signo |
| Conversiones | todas **explícitas** |

Detalle: [`docs/API_fixed_int.md`](docs/API_fixed_int.md) ·
[`docs/API_operator_semantics.md`](docs/API_operator_semantics.md)

## Biblioteca estándar

```cpp
#include "fixed_int_iostreams.hpp"   // std::cout << x, is >> x
#include "fixed_int_format.hpp"      // std::format("{:#x}", x)
#include "fixed_int_hash.hpp"        // unordered_map<uint256_fixed_t, T>
#include "fixed_int_limits.hpp"      // std::numeric_limits
#include "fixed_int_concepts.hpp"    // nstd::integral y compañía
```

Detalle: [`docs/API_fixed_int_stl.md`](docs/API_fixed_int_stl.md)

## Comandos

```bash
python make.py test gcc release-O2     # suite completa (~170 s)
python make.py test all all            # todos los compiladores y modos
python make.py bench gcc release-O2    # benchmarks
python scripts/toolchains.py           # qué compilador se va a usar de verdad
```

Antes de subir nada, los cuatro verificadores:

```bash
python make.py test gcc release-O2
python scripts/check_headers_selfcontained.py
python scripts/check_docs_consistency.py --doxygen
clang-format-21 --dry-run --Werror <ficheros>      # OJO: la versión 21
```

Detalle: [`CONTRIBUTING.md`](CONTRIBUTING.md)

## Tres cosas que muerden

1. En Windows, `g++` y `clang++` **a secas no son los del proyecto**: resuelven
   al toolchain MSYS. Usa `scripts/toolchains.py`.
2. **clang-format es la 21.** La 19 y la 22 reformatean este árbol distinto.
3. La salida de consola va en **ASCII**, sin acentos.

## Dónde está cada cosa

| Qué buscas | Dónde |
|---|---|
| Qué es el proyecto | [`README.md`](README.md) |
| Estado actual | [`PROJECT_STATUS.md`](PROJECT_STATUS.md) |
| Historia | [`CHANGELOG.md`](CHANGELOG.md) |
| Qué viene | [`ROADMAP.md`](ROADMAP.md) · [`NEXT_STEPS.md`](NEXT_STEPS.md) |
| Decisiones y su porqué | [`docs/decisions/`](docs/decisions/README.md) |
| Cómo se construye | [`CONTRIBUTING.md`](CONTRIBUTING.md) |
| API y semántica | [`docs/`](docs/) |
| Rendimiento | [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) |
| Convenciones | [`NAMING_CONVENTIONS.md`](NAMING_CONVENTIONS.md) · [`STYLE_CONVENTIONS.md`](STYLE_CONVENTIONS.md) |
| Guía de desarrollo | [`AI-GUIDE.md`](AI-GUIDE.md) |
