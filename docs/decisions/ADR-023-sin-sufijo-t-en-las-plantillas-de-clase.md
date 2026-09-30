# ADR-023: El sufijo `_t` marca un alias, no una plantilla de clase

**Estado:** ✅ Aceptado
**Fecha:** 30 September 2026
**Autor:** Julián Calderón Almendros

> Cierra el cabo suelto que quedaba del renombrado de la 1.90: se había decidido
> el nombre de la plantilla del entero (23 sep) y no la regla que lo justifica,
> y por eso la familia del punto fijo se quedaba fuera.

---

## Contexto

Las dos plantillas principales llevan `_t`:

```cpp
nstd::fixed_int_t<N, Sign, Form, Policy>        // el entero
nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo>   // el punto fijo
```

El 23 sep se decidió que `fixed_int_t` pasa a **`fixed_width_int`** al abrir la
1.90. La razón inmediata no era estética:

```cpp
nstd::fixed_int_t<2, Sign, Form, Policy>   // la plantilla
nstd::int_fixed_t<2>                       // el alias con signo
```

Están **a una transposición de distancia** y significan cosas distintas. Eso es
una trampa de lectura, no una cuestión de gusto.

Pero el argumento que se usó para quitarle el `_t` —que la `std` moderna no lo
pone a las plantillas de clase— **vale igual para `fixed_point_t`**, y aquella
decisión no lo dijo. El resultado habría sido una familia sin `_t` y otra con él.

### Lo que hace la `std`, que es el precedente

No es que la `std` no use `_t`: lo usa mucho, y de forma **sistemática**.

| sin `_t` — plantillas de clase | con `_t` — alias |
|---|---|
| `std::array`, `std::vector`, `std::optional` | `std::size_t`, `std::ptrdiff_t` |
| `std::ratio`, `std::complex`, `std::bitset` | `std::int64_t`, `std::uint8_t` |
| `std::remove_cv`, `std::is_same` | `std::remove_cv_t`, `std::make_signed_t` |

La última fila es la que lo deja claro: `remove_cv` y `remove_cv_t` **coexisten**,
y el `_t` es exactamente lo que distingue el alias de la plantilla. No sobra: es
información.

## Decisión

### 1. `_t` marca un alias; una plantilla de clase no lo lleva

| hoy | pasa a |
|---|---|
| `fixed_int_t` | **`fixed_width_int`** |
| `fixed_point_t` | **`fixed_point`** |

Los dos nombres nuevos están **libres**: comprobado el 30 sep, cero apariciones
de `fixed_width_int` y de `fixed_point` como identificadores en `include/`.

### 2. Los alias conservan su `_t`

No se tocan, y no por inercia: es que ahora **significa algo**.

```cpp
int_fixed_t, uint_fixed_t                      // enteros
ufixed_point_t, sfixed_point_t                 // punto fijo
ufixed_64_64_t, sfixed_64_64_t                 // instancias concretas
```

Y desaparece la trampa de la transposición: `fixed_width_int` frente a
`int_fixed_t` ya no se confunden.

### 3. El nombre del fichero es el nombre de la clase

La convención ya existía y **una de las dos familias la cumplía**:

| familia | cabecera principal | clase | satélites | ¿cumplía? |
|---|---|---|---|---|
| punto fijo | `fixed_point_t.hpp` | `fixed_point_t` | `fixed_point_*.hpp` | **sí** |
| entero | `fixed_width_int_t.hpp` | `fixed_int_t` | `fixed_int_*.hpp` (7) | **no** |

La del entero se rompió cuando la cabecera principal pasó a
`fixed_width_int_t.hpp` sin que la siguieran ni la clase ni los siete satélites.
Con la regla de arriba queda:

    fixed_width_int.hpp        fixed_width_int_atomic.hpp
                               fixed_width_int_concepts.hpp
                               fixed_width_int_format.hpp
                               fixed_width_int_hash.hpp
                               fixed_width_int_iostreams.hpp
                               fixed_width_int_limits.hpp
                               fixed_width_int_traits_specializations.hpp

    fixed_point.hpp            fixed_point_format.hpp
                               fixed_point_hash.hpp
                               fixed_point_iostreams.hpp
                               fixed_point_limits.hpp
                               fixed_point_traits_specializations.hpp

### 4. Se hace **dentro** de la 1.90, y después de la fase 0

No antes, y no es una preferencia de orden. El principio de
[ADR-012](ADR-012-no-se-mueve-un-tag-publicado.md) —lo publicado no se retoca,
los arreglos van en una versión nueva— dice que esto no puede entrar en la 1.80
ya publicada. Y la 1.90 **ya es rompedora** porque borra la familia
`int128_param_*`, así que es la ventana más barata que va a haber: un solo
cambio de versión donde el consumidor paga una vez.

Y va **después** de afinar el banco (fase 0 de la 1.90), por la razón que ya
está escrita allí: con el instrumento sin calibrar, una regresión y un artefacto
del arnés son indistinguibles, y este renombrado toca 1.277 sitios.

### 5. Queda un alias deprecado, con la frontera de ADR-006

```cpp
using fixed_int_t   = fixed_width_int;   // deprecado
using fixed_point_t = fixed_point;       // deprecado
```

Con el mismo mecanismo de dos caras que se usó para `int128_param_t`: sin marcar
para la propia biblioteca, marcado para quien la consume. Si el aviso no llega a
nadie, la deprecación no ha servido —es la lección del 29 sep, cuando se midió
que 45 ficheros propios silenciaban el `[[deprecated]]` de `int128_param_t` y la
compilación daba cero avisos.

## Alternativas descartadas

| alternativa | por qué no |
|---|---|
| **`fw_int_tt`** (propuesta inicial) | `_tt` no es convención de nada y se lee como errata. Y no resuelve el problema real, que era la transposición |
| **`wide_int`** (P0539, y la biblioteca de Kormanyos) | Buen nombre y con precedente, pero **no dice que la anchura sea fija**, que es la propiedad que define este tipo frente a un bigint. Y la 1.90 deja la puerta abierta a enteros de longitud arbitraria (etapa 7) |
| **Dejar el `_t` en las dos** | Conserva la trampa `fixed_int_t` / `int_fixed_t`, y deja el `_t` sin significado: si lo llevan la plantilla y el alias, no distingue nada |
| **Quitarlo también a los alias** | Perdería la distinción que la `std` sí hace, y obligaría a renombrar `int_fixed_t`, `uint_fixed_t` y las instancias concretas, que hoy no estorban |

## Coste, medido el 30 sep 2026

| qué | apariciones |
|---|---|
| `fixed_int_t` (identificador) | 925 |
| `fixed_point_t` (identificador) | 230 |
| `fixed_int_*.hpp` (nombres de fichero) | 122 |
| **total** | **1.277** en 25 ficheros para los nombres de fichero |

Mecánico, pero **no es un `sed`**: toca el verificador de cabeceras
autocontenidas, los `API_*.md`, el `Doxyfile` y los `#include` de tests, demos y
bancos.

## Consecuencias

- Los `docs/API_fixed_int*.md` pasan a `API_fixed_width_int*.md`, y hay que
  ajustar la comprobación 3 del armonizador, que empareja `API_*.md` con
  cabeceras por nombre.
- El techo de Doxygen se recalcula: los `@brief` que citan `fixed_int_t` en el
  texto también cambian.
- Las cifras del banco **no** se ven afectadas: el renombrado no cambia código
  generado. Si el banco dice lo contrario después de renombrar, el que está mal
  es el banco.

## La regla que sale

**Si lleva `_t`, es un alias.** Si es una plantilla de clase, no lo lleva. Y el
fichero se llama como la clase que define.

---

**Relacionados:** [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md) (la
frontera de deprecación en dos caras),
[ADR-012](ADR-012-no-se-mueve-un-tag-publicado.md) (lo publicado no se retoca:
de ahí que esto entre en la 1.90 y no en la 1.80),
[ADR-021](ADR-021-un-nombre-por-operacion.md) (el precedente de decidir nombres
por escrito y con alternativas).
