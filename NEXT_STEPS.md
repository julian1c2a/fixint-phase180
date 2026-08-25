# 🔮 NEXT STEPS

**Last Updated:** 25 August 2026
**Última versión publicada:** **v1.90.1** (tag anotado, commit `ce71d5d`)
**Rama:** `phase-1.80` · árbol limpio · todo en `origin`

> Este documento es **el puntero y lo pendiente**. No acumula historia: lo ya
> hecho vive en [`CHANGELOG.md`](CHANGELOG.md), y el plan a largo plazo en
> [`ROADMAP.md`](ROADMAP.md).

---

# 📍 POR AQUÍ VAMOS

> Puntero de continuación. Lo primero que hay que leer al retomar.

## Estado al cerrar la sesión del 24 ago 2026

| | |
|---|---|
| **v1.90.1 publicada** | tag anotado y pusheado |
| **CI** | ✅ **24/24 jobs**, tres runs seguidos en verde (35, 36, 37) |
| **Suite** | 55/55 con GCC 16.2 (Windows, 170 s), MSVC 19.5x (462 s) y todo el CI |
| **Verificadores** | headers 31/31 · armonizador 9/9 · formato conforme · 0 avisos de Doxygen desde `include/` |

Las ocho fases del plan de auditoría están cerradas. `fixed_int_t<N>` está
terminado como entero de N × 64 bits y ocupa el sitio de los tipos de 256 bits
anteriores.

## Lo siguiente, en orden

El orden **no es negociable**: lo fija [ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md).
Hacerlo al revés significa portar la API dos veces.

### 1.º — Política de desbordamiento → abre la **2.0**

Decidido en [ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md)
y diseñado en [ADR-008](docs/decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md).
`fixed_int_t<N, Sign, Form, Policy>`, con:

- `wrap` por defecto — el comportamiento de hoy y el de los built-in de C++;
- `checked` **sin excepciones**: todo sigue siendo `constexpr` y `noexcept`, así
  que el fallo viaja en una **marca pegajosa dentro del valor** (el equivalente
  entero de un NaN) que se propaga y se consulta al final con `valid()`;
- `saturate` y `trap` en el enumerado, implementadas después;
- **prohibido mezclar políticas**: error de compilación, no conversión silenciosa.

**Por dónde empezar:** la única cuestión que ADR-008 dejó abierta es de
implementación — **dónde vive la marca de inválido** sin que quien use `wrap`
pague tamaño. Hoy `fixed_int_t<N>` ocupa exactamente `8N` bytes y eso es
contrato (`std::array<std::byte, N*8>`, `bit_cast`). La opción que no cobra nada
al caso común es un miembro extra **solo cuando `Policy == checked`**, vía
especialización. Decidir eso es el primer paso.

> `std::expected` **no** es una opción: es C++23 y el suelo del proyecto es
> C++20. Las `checked_*` siguen con `std::optional`.

### 2.º — Unificación de tipos

[ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md): toda la
funcionalidad de `int128_param_t` se replica en `fixed_int_t` y el tipo viejo se
retira. El inventario de paridad está medido en el ADR. Lo de más peso: portar
**Magnitude-Sign y Excess-K** como valores de `representation_form`, lo que
obliga a generalizar el `static_assert` que hoy solo admite `binnat` y
complemento a dos.

### 3.º — Etapa 5: punto fijo

Ver [ROADMAP.md](ROADMAP.md). Depende de que 1 y 2 estén cerradas.

## Pendiente menor, sin bloquear a nadie

| Qué | Dónde está anotado |
|---|---|
| **T6.7** — consolidación de la documentación de raíz. En curso: la historia ya vive solo en el CHANGELOG; queda decidir el destino de las tres guías de desarrollo | [`CHANGELOG.md`](CHANGELOG.md), sección T6.7 |
| Desenganchar del `Makefile` y de `release.yml` los 5 scripts que aún los llaman, para poder archivarlos | [`scripts/README.md`](scripts/README.md) |
| Escribir los **ADR 001–005**, decisiones tomadas hace tiempo y nunca documentadas | [`docs/decisions/README.md`](docs/decisions/README.md) |
| Cobertura Doxygen de `fixed_width_int_t.hpp` a nivel de miembro | [ROADMAP.md](ROADMAP.md) |

## Cosas que conviene no redescubrir

- **clang-format: usar la 21.** No es estable entre versiones mayores — la 19
  rompe `a ^ T{0}` y la 22 rompe `std::bitset<64 * N>`. Está en la cabecera de
  `.clang-format` y en `toolchains.json`.
- En Windows, **`g++` y `clang++` a secas NO son los del proyecto**: resuelven al
  toolchain MSYS. Usar `python scripts/toolchains.py` para ver cuál se va a usar.
- Antes de cerrar cualquier sesión: `/guarda_y_sube`, que ejecuta los cuatro
  verificadores que exige el CI.

---

---
