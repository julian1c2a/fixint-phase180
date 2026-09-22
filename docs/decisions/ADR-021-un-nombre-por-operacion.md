# ADR-021: Un nombre por operación, y que exista en las dos familias

**Estado:** ✅ Aceptado
**Fecha:** 23 September 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

La auditoría del acompañamiento de `std`
([PLAN_ACOMPANAMIENTO_STD](../PLAN_ACOMPANAMIENTO_STD.md)) midió, llamando a 43
nombres contra las dos familias, que **tres operaciones se llaman distinto según
dónde estés**, y una cuarta diverge del propio estándar.

No salió de leer los ficheros. Una expresión regular sobre las firmas habría
dado falsos positivos —sobrecargas, nombres en comentarios, plantillas que no
aplican a ese tipo— y esa es justo la clase de lista que en este proyecto se ha
quedado corta cuatro veces.

| Operación | `int128_param_t` (1.75) | `fixed_int_t` (limbos) | Divergencia |
|---|---|---|---|
| raíz entera | `isqrt` | `sqrt` | **nombre** |
| producto ancho | `widening_mul` | `mul_wide` | **nombre** |
| potencia | `pow(T, unsigned)` | `pow(T, T)` | **firma** |
| potencia de dos | `is_power_of_2` | `is_power_of_2` | ninguna entre familias, pero **`<bit>` lo llama `has_single_bit`** |

## La decisión anterior que hay que respetar

`fixed_width_int_t.hpp` ya lleva escrito por qué `widening_mul` **no** se portó:

> *«`widening_mul(a, b)` es exactamente `mul_wide(a, b)`, que ya existe. No se
> porta el nombre, por lo mismo que no se portó `power`: un segundo nombre para
> la misma operación es deuda recién estrenada.»*

Ese razonamiento sigue en pie y **descarta la solución ingenua**: poner alias en
las dos direcciones duplicaría todos los nombres y empeoraría exactamente lo que
se quiere arreglar.

## Decisión 1 — Un nombre canónico, y tiene que existir en las dos familias

Para cada operación se elige **un** nombre. Ese nombre **existe en las dos
familias**. El otro se queda **sólo donde ya estaba**, documentado como nombre
antiguo.

| Operación | Canónico | Por qué | Queda como nombre antiguo |
|---|---|---|---|
| raíz entera | **`sqrt`** | es el de `std::` | `isqrt`, sólo en 1.75 |
| producto ancho | **`mul_wide`** | ya era la decisión escrita | `widening_mul`, sólo en 1.75 |
| potencia | **`pow`** con **las dos sobrecargas** | ninguna firma es mejor | — |
| potencia de dos | **`has_single_bit`** | es el de `<bit>` | `is_power_of_2`, en las dos |

El resultado es que **código nuevo escrito con los nombres canónicos compila en
las dos familias**, que es lo único que la unificación tenía que conseguir. Y no
se crea ni un nombre nuevo donde no lo había: los antiguos no se propagan.

## Decisión 2 — Los nombres antiguos **no** llevan `[[deprecated]]`

Se documentan, no se marcan.

El motivo es mecánico: el CI compila con `-Werror` en varios jobs, y un
`[[deprecated]]` sobre un nombre que la propia biblioteca todavía usa —
`is_power_of_2` aparece en la matriz de paridad — pondría el árbol en rojo sin
que nadie haya hecho nada mal. Y el de fondo es
[ADR-012](ADR-012-no-se-mueve-un-tag-publicado.md): lo publicado no se mueve.

Lo que sí se hace es **vigilar en la matriz que los dos nombres compilen y den
el mismo resultado**. Un alias que se desincroniza es peor que dos nombres.

## Decisión 3 — `sqrt` acepta con signo, y devuelve 0 para negativos

Hoy `sqrt` en los limbos **sólo acepta `uint_fixed_t`**. El verificador lo sacó:
la fila sale verde en `uint` y roja en `int/C2`, `int/MS` e `int/EK`.

`sqrt` pasa a aceptar las cuatro representaciones. Para un radicando negativo
**devuelve cero**, que es lo que ya hace `isqrt` en 1.75.

> **Esto es una verruga, y se hereda a propósito.** Devolver cero en silencio
> esconde un error de quien llama; lo honesto sería una precondición con
> aserción. Pero cambiar el comportamiento de 1.75 está fuera del alcance —esa
> familia se retira ([ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md))—
> y **dos familias que dan el mismo nombre a comportamientos distintos serían
> peor que una verruga compartida**: el objetivo de este ADR es justamente que
> mover código entre familias no cambie lo que hace.
>
> Queda anotado: si algún día se revisa, se revisa en las dos a la vez.

`std::sqrt(-1.0)` devuelve NaN, y aquí no hay NaN. Esa es la raíz del problema y
no la resuelve ningún nombre.

## Decisión 4 — `pow` lleva las dos sobrecargas en las dos familias

`pow(T, unsigned)` y `pow(T, T)`.

No hay una mejor: el exponente natural de una potencia es un entero pequeño
—`unsigned` sobra— pero en código genérico sobre `T` lo que se tiene a mano es
un `T`. Las dos son razonables y las dos ya existen, cada una en un sitio. Se
ponen las dos en los dos sitios y se acabó la pregunta.

No hay ambigüedad al resolver: `unsigned` y `T` no se convierten entre sí,
porque el constructor desde entero es `explicit` ([ADR-001](ADR-001-constructores-y-conversiones-explicitos.md)).

## Consecuencias

- Código nuevo se escribe con `sqrt`, `mul_wide`, `pow` y `has_single_bit`, y
  compila en las dos familias.
- El verificador `check_acompanamiento_std.py` pasa a verde en las filas `sqrt`
  y `pow` para las cuatro representaciones del entero.
- La matriz de paridad gana sondas que comprueban que **los dos nombres dan el
  mismo valor**, no sólo que los dos compilan.
- `int128_param_t` gana cuatro funciones de una línea. No es completarla —sigue
  fuera del objetivo de 24/24— es dejar de obligar a saberse dos vocabularios.

## Un defecto que apareció de paso, y no se arregla aquí

`isqrt` en 1.75 lleva escrito en su propia documentación:

> *«For EK: Operates on stored value (not real value). Convert to TC for
> semantic correctness.»*

Es **exactamente** la equivocación que
[ADR-018](ADR-018-la-representacion-no-es-observable.md) prohibió y que el tramo
3 se encontró seis veces: operar sobre los limbos como si fueran el valor. En
Exceso-K no lo son.

No se arregla en este ADR porque 1.75 está fuera de alcance, pero **queda
anotado en `NEXT_STEPS`**: es un fallo de valor, no de estilo, y el `sqrt`
canónico que se añade a 1.75 lo hereda.

## La regla que sale

**Antes de añadir un alias, preguntarse si el nombre nuevo tiene que existir en
los dos sitios o sólo en uno.** Un alias bidireccional duplica el vocabulario;
uno canónico que existe en todas partes lo unifica. La diferencia se ve al
escribir la tabla de quién tiene qué.
