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

## Para mañana — tres cosas, decididas el 25 ago 2026

Las tres salieron de la sesión de hoy y están por delante de la 2.0.

### 1. Intel oneAPI tiene que funcionar, y **está instalado en esta máquina**

Esto cambia el planteamiento por completo: **se puede reproducir y arreglar en
local**, sin gastar ciclos de CI a ciegas. Comprobado hoy:

```
C:\Program Files (x86)\Intel\oneAPI\setvars.bat          <- existe
C:\Program Files (x86)\Intel\oneAPI\2026.1\bin\icx.exe   <- y 2026.0, y 2025.3
```

Hay **tres versiones** instaladas (2025.3, 2026.0, 2026.1) más una copia bajo
`compiler\2025.3\`. Primera decisión: cuál se fija, y dónde se anota — igual que
se hizo con clang-format en [ADR-013](docs/decisions/ADR-013-clang-format-local-22-ci-21.md).

Orden de trabajo:

1. **En local primero.** `setvars.bat` y luego `python make.py test intel release`.
   Que funcione aquí antes de tocar el CI.
2. **`scripts/toolchains.py` y `toolchains.json`**: que Intel se resuelva por la
   ruta real en vez de confiar en el `PATH`, que es lo que ya se hizo con los
   demás compiladores.
3. **Después el CI.** Lo de allí es un problema distinto y hay que separarlo: en
   el runner, `ONEAPI_ROOT` viene **vacío** y `rscohn2/setup-oneapi` deja un
   `CMAKE_PREFIX_PATH` apuntando a `/opt/intel/oneapi/...`, **una ruta de Linux
   en un runner de Windows**. Con el camino local resuelto se sabrá qué hay que
   exigirle al runner.

Hasta entonces el job de Intel en `release.yml` sale **en rojo a propósito**, y
así se queda: es preferible a que vuelva a dar un falso verde.

### 2. `make.py` no detecta que el enlazado ha fallado

**Es lo más serio de la lista**, porque invalida cualquier uso de `make.py build`
como puerta, en un guion o en un workflow.

Reproducción, un solo comando:

```bash
python make.py build uint128 vs_builtin benchs gcc release
# ld.exe: undefined reference to `__gmpz_clear'
# collect2.exe: error: ld returned 1 exit status
# OK Build complete for uint128 vs_builtin benchs!
# exit=0                                            <- y no genera binario
```

El diagnóstico ya está hecho, y son **dos fallos distintos** en
`scripts/build_generic.py`:

- **El código de salida no se propaga.** La función de compilación imprime el
  error del enlazador y **no devuelve nada**; `main()` imprime «Build complete»
  incondicionalmente y termina. No hay ni un `sys.exit(1)` en todo el camino.
- **Línea 268:** `if result.returncode == 0 or output_check.exists():`. Ese `or`
  hace que **un binario viejo de una compilación anterior disfrace un fallo
  nuevo**. Es un fallo latente aparte del anterior, y más traicionero, porque
  solo se manifiesta cuando ya has compilado bien alguna vez.

Al arreglarlo hay que comprobar que no se rompe nada que dependiera del
comportamiento actual: `make.py test`, `bench` y el bucle de `release.yml` usan
esos códigos de salida.

### 3. La penalización de `operator*`: ¿solo N=3, o todos los impares?

El control del benchmark de Karatsuba destapó que el bucle escolar de
`operator*` es un **14 % más lento que una copia idéntica suya escrita como
función libre**, con las mismas primitivas y el mismo compilador. Estable en dos
ejecuciones (0,86× y 0,87×). En N=16, en cambio, la razón sale 1,02×.

**Lo que hay que averiguar:** si el efecto es de los N impares, de los N
pequeños, o solo del 3.

Barrido a hacer, con el benchmark que ya existe:

| N | | qué distinguiría |
|---|---|---|
| 3, 5, 7, 9 | impares | si es cosa de la paridad |
| 6, 10, 12 | pares que **no** usan Karatsuba | separa «impar» de «no es 2, 4, 8» |
| 16 | ya medido, 1,02× | el control que ya pasa |

Si sale que son todos los N∉{2,4,8} pequeños, apunta a la generación de código
del `if constexpr` encadenado; si son solo los impares, a algo del bucle de
acarreo. Mirar el ensamblador de los dos caminos en el N que peor salga.

Las cifras van a [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) con fecha,
compilador y máquina, como exige su regla.

---

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
