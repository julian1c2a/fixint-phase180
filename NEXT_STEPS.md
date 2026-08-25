# 🔮 NEXT STEPS

**Last Updated:** 25 August 2026
**Versión:** **v1.90.4** · **rama** `phase-1.80` · árbol limpio · todo en `origin`

> Este documento es **el puntero y lo pendiente a corto**. No acumula historia:
> lo ya hecho vive en [`CHANGELOG.md`](CHANGELOG.md), el plan largo en
> [`ROADMAP.md`](ROADMAP.md) y el estado en [`PROJECT_STATUS.md`](PROJECT_STATUS.md).

---

# 📍 POR AQUÍ VAMOS

## Al cerrar el 25 ago 2026

**El diseño de la 2.0 está cerrado. No quedan cuestiones abiertas: falta
escribirlo.**

Hoy se decidieron las tres cosas que faltaban —dónde vive la marca de inválido,
qué devuelven las `checked_*` y cómo comparan los valores inválidos— y se
escribieron los cinco ADR fundacionales que estaban pendientes desde siempre.
`docs/decisions/` tiene **10 registros** y ninguna decisión sin documentar.

| | |
|---|---|
| **CI** | ✅ 24/24 jobs sobre `HEAD` (`4d3ab66`) |
| **Suite** | 55/55 |
| **Release de v1.90.1** | ❌ **no publicada** — ver abajo |

## La release ya está: queda una decisión sobre Intel

**`v1.90.4` publicada**, con tres zips (gcc, clang, msvc). Esa deuda se cierra.

Lo que queda es decidir qué hacer con **Intel en Windows, que no funciona**:
`ONEAPI_ROOT` viene vacío y la acción `rscohn2/setup-oneapi` deja un
`CMAKE_PREFIX_PATH` que apunta a `/opt/intel/...`, una ruta de Linux en un runner
de Windows. El CI sí cubre Intel, pero sobre Ubuntu.

- **Retirarlo de la matriz de release.** Las cabeceras son idénticas en los
  cuatro compiladores, así que el contenido útil del zip no cambia. Las releases
  saldrían limpias en verde.
- **Hacerlo funcionar.** Requiere una acción que instale oneAPI en Windows de
  verdad, y varios ciclos sobre el CI: no se puede reproducir en local.

Mientras no se decida, su job sale **en rojo a propósito** y el run entero figura
como `failure` aunque la release se publique.

## Después: escribir la 2.0

El orden **no es negociable**, lo fija [ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md):
hacerlo al revés significa portar la API dos veces.

### 1.º — Política de desbordamiento → abre la **2.0**

`fixed_int_t<N, Sign, Form, Policy>`. Todo decidido:

| Qué | Dónde |
|---|---|
| La política es parámetro de plantilla, el cuarto | [ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md) |
| `wrap` por defecto; `checked` sin excepciones, con marca pegajosa; nada de mezclar políticas | [ADR-008](docs/decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md) |
| La marca vive en un miembro extra **solo con `checked`**: `wrap` conserva sus `8N` bytes exactos | [ADR-009](docs/decisions/ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md) |
| Las `checked_*` se completan **además** de la política, y devuelven el propio tipo con política `checked` | [ADR-009](docs/decisions/ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md) |
| Los inválidos **se ordenan**: `operator<=>` sigue devolviendo `std::strong_ordering`, comparación lexicográfica sobre `(válido?, valor)` | [ADR-010](docs/decisions/ADR-010-orden-total-con-valores-invalidos.md) |

**Por dónde empezar:** el almacenamiento parametrizado por política —clase base
con `[[no_unique_address]]` o especialización—, comprobando con un
`static_assert` que `sizeof(fixed_int_t<4, ..., wrap>) == 32` no se mueve.

> `std::expected` **no** es una opción: es C++23 y el suelo del proyecto es C++20.

### 2.º — Unificación de tipos

[ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md): toda la
funcionalidad de `int128_param_t` se replica en `fixed_int_t` y el tipo viejo se
retira. Lo de más peso: portar **Magnitud-Signo y Exceso-K** como valores de
`representation_form`, generalizando el `static_assert` que hoy solo admite
`binnat` y complemento a dos.

[ADR-011](docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md) fija las
combinaciones válidas: **sin signo ⟺ `binnat`**, y con signo una de las tres
codificaciones de signo. Son **cuatro combinaciones, no ocho**, así que el
`static_assert` pasa a expresar ese bicondicional en vez de enumerar las dos
implementadas. Queda por escribir la especialización de
`representation_traits<binnat>`, cuyo contenido ADR-011 deja determinado.

### 3.º — Etapa 5: punto fijo

Ver [ROADMAP.md](ROADMAP.md). Depende de que 1 y 2 estén cerradas.

## Pendiente menor, sin bloquear a nadie

| Qué | Dónde |
|---|---|
| **`make.py build` devuelve 0 aunque el enlazado falle.** No sirve como puerta; hay que arreglarlo antes de fiarse de él en ningún sitio | [PROJECT_STATUS.md](PROJECT_STATUS.md) |
| Medir Karatsuba en **Clang, MSVC e Intel**: hoy solo hay cifras de GCC | [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) |
| Averiguar por qué el bucle escolar de `operator*` es un 14 % más lento que una copia suya escrita como función libre, en N=3 | [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) |

## Cosas que conviene no redescubrir

- **clang-format: en local la 22.1.8, en el CI la 21.** El árbol es punto fijo
  de las dos, medido; la regla es que lo siga siendo. La serie 19 sí lo rompe.
- En Windows, **`g++` y `clang++` a secas NO son los del proyecto**: resuelven al
  toolchain MSYS. Usar `python scripts/toolchains.py` para ver cuál se usará.
- El `Makefile` es un **shim** sobre `make.py`, que es la capa canónica. Lo que
  no esté en el shim se hace con `python make.py --help`.
- Antes de cerrar sesión: `/guarda_y_sube`, que ejecuta los cuatro verificadores
  que exige el CI.
