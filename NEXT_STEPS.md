# 🔮 NEXT STEPS

**Last Updated:** 25 August 2026
**Versión:** **v1.90.1** · **rama** `phase-1.80` · árbol limpio · todo en `origin`

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

## Lo primero al retomar: la release rota

El workflow **Release** falló sobre el tag `v1.90.1` y **no hay ninguna release
publicada**. Llamaba a cuatro scripts de los que tres no existían. Está
arreglado en `03283af`, pero **el tag apunta a `ce71d5d`, anterior al arreglo**,
así que el arreglo no se ha ejercitado nunca.

Hay que decidir entre mover el tag `v1.90.1` a un commit que ya lleve el arreglo,
o publicar una `v1.90.2` que lo incluya. Lo segundo es más limpio: un tag ya
pusheado que se mueve rompe a quien lo tenga.

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
| `README.md` sigue llevando historial y una línea de estado que se contradice a sí misma | [PROJECT_STATUS.md](PROJECT_STATUS.md), «Deuda anotada» |
| **Medir Karatsuba**: no tiene cifras, siendo optimización de cabecera de v1.90 | [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) |
| Cobertura Doxygen de `fixed_width_int_t.hpp` a nivel de miembro | [ROADMAP.md](ROADMAP.md) |

## Cosas que conviene no redescubrir

- **clang-format: el CI usa la 21**, que es la versión de referencia declarada en
  la cabecera de `.clang-format`. No es estable entre versiones mayores.
- En Windows, **`g++` y `clang++` a secas NO son los del proyecto**: resuelven al
  toolchain MSYS. Usar `python scripts/toolchains.py` para ver cuál se usará.
- El `Makefile` es un **shim** sobre `make.py`, que es la capa canónica. Lo que
  no esté en el shim se hace con `python make.py --help`.
- Antes de cerrar sesión: `/guarda_y_sube`, que ejecuta los cuatro verificadores
  que exige el CI.
