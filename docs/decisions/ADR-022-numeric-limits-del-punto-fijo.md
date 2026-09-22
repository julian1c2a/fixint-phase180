# ADR-022: `numeric_limits` del punto fijo — qué significa cada miembro

**Estado:** ✅ Aceptado
**Fecha:** 23 September 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

`std::numeric_limits` tiene una forma para los enteros y otra para la coma
flotante, y **el punto fijo no es ninguna de las dos**. La mitad de sus miembros
significan algo distinto según con cuál se le compare, y elegir mal no rompe la
compilación: rompe el código genérico **en silencio**.

Ésta es la etapa E4 del [plan del acompañamiento](../PLAN_ACOMPANAMIENTO_STD.md),
y va antes que `traits`, `hash`, `format` e `iostreams` porque todas ellas
preguntan aquí.

## La regla que decide casi todo

**Donde el tipo ya publica un miembro, `numeric_limits` tiene que coincidir con
él.** `fixed_point_t` ya tiene `min()`, `max()` y `epsilon()`. Si
`numeric_limits<T>::min()` devolviera otra cosa que `T::min()`, habría dos
respuestas para la misma pregunta y la discrepancia aparecería sólo en el código
genérico, que es donde menos se mira.

Esa regla resuelve las dos decisiones que tenían trampa.

## Decisión 1 — `min()` es **el más negativo**, como en los enteros

Es la trampa principal, y no compila distinto: compila igual y hace otra cosa.

| | Qué devuelve `min()` |
|---|---|
| entero | el **más negativo** |
| coma flotante | el **positivo más pequeño** normal |

El punto fijo tiene que elegir. Se elige **la convención de los enteros**, por
tres razones que apuntan al mismo sitio:

1. `T::min()` ya devuelve el más negativo. La regla de arriba cierra el caso.
2. [ADR-019](ADR-019-punto-fijo-es-un-entero-con-escala.md) dice que un punto
   fijo **es un entero con una escala**. No hay exponente ni subnormales, que es
   lo que motiva la convención de la coma flotante.
3. Desde C++11 existe `lowest()` justamente para preguntar «el más negativo» sin
   depender de la familia. Quien quiera el positivo más pequeño tiene
   `epsilon()`, que aquí **es** ese valor.

> Un algoritmo genérico que use `numeric_limits<T>::min()` como «el más pequeño»
> dará resultados distintos según la respuesta, **y compilará en los dos casos**.
> Por eso está escrito aquí y no sólo en el código.

## Decisión 2 — `is_integer` es `false`, `is_exact` es `true`

No son lo mismo y aquí se separan, que es lo interesante.

- **`is_integer = false`.** Es lo único que distingue este tipo del entero desde
  `numeric_limits`, y hay valores entre dos enteros consecutivos.
- **`is_exact = true`.** El conjunto representable **es exacto**: cada valor es
  `k/2^(64·F)`, sin aproximación. Que `*` y `/` redondeen no lo cambia —
  `operator/` del entero también redondea y `is_exact` es `true` ahí.

`is_exact` habla de la **representación**, no de las operaciones. La coma
flotante lo tiene a `false` porque sus valores son aproximaciones de otra cosa;
aquí no lo son.

## Decisión 3 — `epsilon()` es el **ulp**, y es **absoluto**

`epsilon()` devuelve `T::epsilon()`, o sea `2^-(64·F)`.

**Y significa algo distinto que en coma flotante.** Allí `epsilon` es *relativo*:
la distancia de 1 al siguiente representable, que crece con la magnitud. Aquí el
paso es el mismo en todo el rango, así que `epsilon` es **absoluto** y cumple
tres cosas a la vez:

```
epsilon()  ==  el paso entre dos valores consecutivos, en cualquier punto
           ==  el valor positivo más pequeño
           ==  denorm_min()
```

Esa triple igualdad no se da en coma flotante y conviene decirla, porque es lo
que hace que en punto fijo el error absoluto de una suma esté acotado por
`epsilon/2` **en todo el rango**.

> **El borde `F == 0`.** Ahí el tipo es un entero con otro nombre y `epsilon()`
> vale **1**, mientras que `numeric_limits<int>::epsilon()` vale **0**. Se
> mantiene el 1, por la regla de arriba: `T::epsilon()` ya vale 1 y tener dos
> respuestas sería peor que apartarse de la convención entera en un borde.

## Decisión 4 — `digits` cuenta **todos** los bits de valor

`digits = 64·N − (con signo ? 1 : 0)`, igual que en el entero.

Se consideró que contara los **fraccionarios**, por analogía con la mantisa de
la coma flotante. Se descarta: la mantisa mide la *precisión relativa*, y aquí la
precisión no es relativa (decisión 3). Lo que `digits` mide en un tipo exacto es
**cuántos dígitos en base `radix` caben**, y eso son todos.

Quien quiera los fraccionarios tiene `T::escala_bits`, y quien quiera el reparto
tiene `limbos_enteros` y `limbos_fraccionarios`. `numeric_limits` da la vista
genérica; el tipo da la suya.

`digits10` sale de `digits` con la fórmula de siempre.

### `max_digits10`: los decimales que hacen falta para no perder nada

`max_digits10 = digits10 + 2`, la fórmula de los tipos con redondeo.

Aquí hay un matiz que conviene no olvidar: la expansión decimal de un
`k/2^(64·F)` **termina**, y tiene exactamente hasta `64·F` cifras tras la coma.
Así que «imprimir sin perder nada» y «imprimir para distinguir» son cosas
distintas, y `max_digits10` responde la segunda. Para la primera,
`to_string(64*F)`.

## Decisión 5 — `round_style` **lee la perilla**

No es un valor fijo. Sale de `Redondeo`:

| `rounding_mode` | `float_round_style` |
|---|---|
| `to_nearest_even` | `round_to_nearest` |
| `to_nearest_away` | `round_to_nearest` |
| `toward_zero` | `round_toward_zero` |
| `toward_neg_inf` | `round_toward_neg_infinity` |
| `toward_pos_inf` | `round_toward_infinity` |

> **El enum del estándar no distingue los dos modos «al más cercano».** Sólo
> tiene `round_to_nearest`, así que `to_nearest_even` y `to_nearest_away` caen en
> el mismo sitio. Es una pérdida de información del estándar, no de aquí, y se
> documenta para que nadie deduzca el desempate a partir de `round_style`.

### `round_error()` se mide en **ULPs**, y eso lo corrigió el test

> **Esta parte se escribió mal la primera vez.** Decía que `round_error()`
> devolvía «medio ulp, en las unidades del tipo». El test lo desmintió en una
> línea: **medio ulp no es representable**, porque el ulp *es* el valor positivo
> más pequeño. Que la mitad de la unidad mínima no quepa es la prueba de que la
> lectura era la equivocada — y es exactamente el tipo de error que una tabla de
> constantes comparada consigo misma no habría encontrado.

Lo que dice `<limits>` es que `round_error()` da el error máximo **en ULPs**:
`numeric_limits<float>::round_error()` vale `0.5`, que son medio ULP, no medio
`float`. Con esa lectura sale todo:

| | `round_error()` |
|---|---|
| al más cercano (los dos) | **0.5** |
| dirigidos (los tres) | **1** |

Dos bordes, y los dos se documentan en el código:

- Con **`F == 0`** no hay parte fraccionaria y `0.5` no cabe: devuelve **cero**,
  igual que los enteros.
- Con **`F == N`** el uno no cabe —el tipo llega hasta `[0,1)`— y los dirigidos
  devuelven `max()`. Sin signo eso es exactamente `1 − epsilon`, que es el
  supremo real del error de truncar.

## Decisión 6 — `is_modulo` sale de la **política**, no del signo

`is_modulo = (Policy == overflow_policy::wrap)`.

En el entero de esta biblioteca está puesto a `!is_signed`, copiando la
convención del estándar, donde el desbordamiento con signo es comportamiento
indefinido. **Aquí no lo es**: `wrap` está definido y envuelve, con signo y sin
él ([ADR-007](ADR-007-politica-de-desbordamiento-como-parametro.md)). Y con
`checked` no envuelve: marca.

Decir `is_modulo` según el signo sería repetir una convención cuya razón de ser
no se da aquí.

> Queda anotado que **el entero tiene el mismo problema** y lo dice al revés que
> su propio comportamiento. No se toca en este ADR —es la familia que ya está
> publicada— pero está en `NEXT_STEPS`.

## Lo que no existe y devuelve cero

`infinity()`, `quiet_NaN()`, `signaling_NaN()`: no hay ninguno de los tres, sus
`has_*` son `false` y las funciones devuelven cero, igual que hacen los enteros.

`has_denorm = denorm_absent`, y aun así `denorm_min()` devuelve `epsilon()`, que
es el valor positivo más pequeño. Es lo que hacen los enteros con `min()`.

`min_exponent`, `max_exponent` y sus versiones decimales son `0`: no hay
exponente. Ése es precisamente el punto del punto fijo.

## Consecuencias

- `traits`, `hash`, `format` e `iostreams` (E5) ya pueden preguntar aquí.
- El test tiene que cruzar `numeric_limits` **contra la aritmética**, no contra
  una tabla: que `epsilon()` sea de verdad el paso entre dos consecutivos, que
  `max() + epsilon()` desborde, que `min()` sea de verdad el menor. Una tabla de
  constantes comparada consigo misma no puede fallar.
- `round_style` hay que instanciarlo con **los cinco modos**, porque cuatro de
  ellos podrían estar mal y el test del modo por omisión pasaría igual.

## La regla que sale

**Cuando un tipo nuevo cae entre dos familias del estándar, la pregunta no es a
cuál se parece más, sino qué contesta ya el propio tipo.** `min()` y `epsilon()`
tenían respuesta antes de llegar aquí, y esa respuesta manda: dos formas de
preguntar lo mismo tienen que dar lo mismo, y si no, la que se descubre tarde es
la que rompe código.
