# Migrar de `int128_param_t` a `fixed_int_t`

**Última actualización:** 23 September 2026
**Estado:** `int128_param_t` está **deprecado** desde la 1.80. Se retira en la 2.0.

> Esto es el tramo 2 de
> [ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md): la
> deprecación se anuncia, el código sigue compilando, y la retirada llega en una
> versión mayor. Nada deja de funcionar hoy.

---

## Lo primero: no hay prisa, y no se rompe nada

`[[deprecated]]` **avisa, no rompe**. Ningún job de este proyecto compila con
`-Werror`, y tu código tampoco tiene por qué. Si quieres silencio mientras
migras, define la macro antes de incluir:

```cpp
#define NSTD_SILENCIA_INT128_PARAM_DEPRECADO
#include "int128_parameterized.hpp"
```

---

## La tabla de sustituciones

| Deprecado | Sustituto | Nota |
|---|---|---|
| `nstd::uint128_t` | `nstd::uint128_fixed_t` | = `uint_fixed_t<2>` |
| `nstd::uint128_bn_t` | `nstd::uint128_fixed_t` | era un alias de `uint128_t` |
| `nstd::int128_t` | `nstd::int128_fixed_t` | = `int_fixed_t<2>` |
| `nstd::int128_tc_t` | `nstd::int128_fixed_t` | complemento a dos, el mismo |
| `nstd::int128_ms_t` | `nstd::fixed_int_t<2, signedness::signed_type, representation_form::magnitude_sign>` | |
| `nstd::int128_ek_t` | `nstd::fixed_int_t<2, signedness::signed_type, representation_form::excess_k>` | |
| `nstd::int128_param_t<S, F>` | `nstd::fixed_int_t<2, S, F>` | el `2` es la anchura en limbos |

Y la cabecera:

```cpp
- #include "int128_parameterized.hpp"
+ #include "fixed_width_int_t.hpp"
```

### `int128_t` no se ha reapuntado, y es deliberado

El tramo 2 de ADR-006 preveía crear un alias `int128_t = fixed_int_t<2, ...>`
para que el código existente siguiera compilando. **Ese nombre ya estaba
ocupado**: `int128_t` era el tipo viejo desde antes de que se escribiera el ADR.

Reapuntarlo habría cambiado el tipo bajo los pies del código existente **sin que
el compilador dijera nada** — misma sintaxis, otro tipo, otra superficie de
miembros. Es lo contrario de lo que una deprecación debe hacer. Así que
`int128_t` sigue siendo el tipo viejo, deprecado como los demás, y el sustituto
tiene nombre propio: `int128_fixed_t`.

---

## Lo que cambia en el comportamiento: nada

`fixed_int_t<2, S, F>` se comporta **igual que `int128_param_t<S, F>` en todo**.
No es una promesa de diseño: está comprobado. `tests/test_fixed_vs_param.cpp`
cruza los dos tipos operación por operación exigiendo resultados idénticos bit a
bit, y `tests/test_fixed_differential.cpp` cruza el tipo nuevo contra un oráculo
independiente con **46.800 comprobaciones**.

Las cuatro representaciones se comportan igual entre sí y **se distinguen solo
por lo que devuelven `limb()` y `limbs()`**
([ADR-018](decisions/ADR-018-la-representacion-no-es-observable.md)).

### La única asimetría real

En **Magnitud-Signo** el mínimo es `-(2^127 - 1)`, uno más alto que el de
complemento a dos, porque el cero negativo ocupa el hueco. La conversión satura
([ADR-017](decisions/ADR-017-magnitud-signo-y-exceso-k-como-codificaciones.md)).
Ya era así en el tipo viejo.

---

## Lo que ganas al migrar

Migrar no es solo quitar un aviso. El tipo nuevo tiene cosas que el viejo nunca
tuvo:

| | `int128_param_t` | `fixed_int_t` |
|---|---|---|
| Anchura | fija, 128 bits | **cualquier N × 64 bits** |
| Operadores entre signos distintos | **no** (hueco conocido desde v1.81) | sí, con las conversiones usuales de C++ |
| Política de desbordamiento | fija | **parámetro** (`wrap`, `checked`, `saturate`) — [ADR-007](decisions/ADR-007-politica-de-desbordamiento-como-parametro.md) |
| `std::ranges`, `views::iota` | no | sí |
| Superficie de cabeceras | 15 ficheros | 5 |

---

## Miembros que cambian de nombre

Casi todo se llama igual. Las excepciones:

| `int128_param_t` | `fixed_int_t` | Por qué |
|---|---|---|
| `high()`, `low()` | `limb(1)`, `limb(0)` | el tipo nuevo tiene N limbos, no dos |

Si tu código usa `high()`/`low()` para reconstruir un valor, mira antes si lo que
quieres es `limbs()`, que devuelve el array entero.

> **Ojo con leer limbos para decidir cosas.** En Magnitud-Signo y Exceso-K los
> limbos **no son el valor**: `limb()` y `limbs()` devuelven lo *almacenado*. Si
> lo que quieres es el número, usa las operaciones, no los bits. Seis fallos
> reales de este proyecto salieron de esa misma confusión.

---

## Qué desaparece y no tiene sustituto

`int128_param_algorithm.hpp` tenía diez clones de `<algorithm>` restringidos al
tipo. **No se portan**, y no hace falta:
[`std::` ya acepta el tipo nuevo](decisions/ADR-006-migracion-int128-param-a-fixed-int.md).

```cpp
- nstd::sort_int128(v.begin(), v.end());
+ std::sort(v.begin(), v.end());
```

La regla que salió de medirlo: no es «`std::` frente a `nstd::`», es si `std::`
**acepta o rechaza** el tipo. Los algoritmos de iterador lo aceptan; los
restringidos a `std::integral` lo rechazan, y ahí `nstd::` es la única opción.

---

## Calendario

| Versión | Qué pasa |
|---|---|
| **1.80** | Deprecación anunciada. Todo sigue compilando. |
| **1.90** | Retirada: las 16 cabeceras se van. |

Si algo de tu código no tiene sustituto claro en esta guía, **dilo antes de la
2.0**: el inventario de paridad se cerró midiendo, pero un uso que nadie previó
es exactamente lo que una deprecación publicada sirve para descubrir.
