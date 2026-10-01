# ADR-024: El tipo de punto fijo se llama `scaled_integer`, y `fixed_point_t` queda como alias

**Estado:** ✅ Aceptado
**Fecha:** 1 October 2026
**Autor:** Julián Calderón Almendros

> Cierra la única pregunta que
> [ADR-023](ADR-023-sin-sufijo-t-en-las-plantillas-de-clase.md) **dio por supuesta
> sin decirlo**. No revoca su regla —que sigue intacta— pero **sustituye una fila
> de su tabla**: donde decía `fixed_point_t` → `fixed_point`, va
> `fixed_point_t` → `scaled_integer`, con `fixed_point_t` sobreviviendo como alias.

---

## Contexto

ADR-023 decidió que **`_t` marca un alias, no una plantilla de clase**, y aplicó
la regla a las dos familias:

| ADR-023 decía | |
|---|---|
| `fixed_int_t` | → `fixed_width_int` |
| `fixed_point_t` | → `fixed_point` |

La segunda fila **decidía el sufijo y daba el tronco por supuesto**. Y el tronco
no estaba decidido: `NEXT_STEPS.md` lo tenía escrito como pregunta abierta, y
`NOMENCLATURA_PUNTO_FIJO.md` lo cierra diciendo literalmente *«el segundo describe
la implementación; el primero, el concepto. **No lo decide este documento**»*.

Dejarlo así tenía una consecuencia concreta: si el tronco acabara siendo
`scaled_integer`, se renombrarían 230 sitios **dos veces** — exactamente lo que
ADR-023 argumenta que no hay que hacer.

### Las dos candidaturas, y lo que sostiene cada una

**`scaled_integer`** es a donde llegó McFarlane tras **seis revisiones de P0037**:
en CNL el tipo se llama `cnl::scaled_integer`, o sea *dejó de llamarlo
«fixed_point»*. Y es **literalmente el título de
[ADR-019](ADR-019-punto-fijo-es-un-entero-con-escala.md)**: «un punto fijo es un
entero con una escala». Describe la implementación con exactitud.

**`fixed_point`** es el término del dominio entero —TR 18037 (`_Fract`, `_Accum`),
Ada, Boost, libfixmath— y por tanto lo que alguien busca. Describe el concepto, y
comparte prefijo con `fixed_width_int`.

El patrón de este proyecto apunta al primero: las seis decisiones de nomenclatura
del punto fijo **adoptan el nombre de la propuesta incluso cuando el actual
servía** (`desde_crudo` → `from_rep`, `escala_*` → `scale_up`/`scale_down`,
`checked` → `special`). Pero renunciar a la descubribilidad del segundo es un
coste real, no una cuestión de gusto.

## Decisión

**Las dos, con jerarquía: `scaled_integer` es la clase, `fixed_point_t` es el
alias.**

```cpp
/// La implementacion: un entero con una escala (ADR-019).
template <std::size_t N, std::size_t F, signedness Sign, representation_form Form,
          overflow_policy Policy, rounding_mode Redondeo>
class scaled_integer { /* ... */ };

/// El concepto, para quien lo busca por su nombre de dominio.
template <std::size_t N, std::size_t F, signedness Sign, representation_form Form,
          overflow_policy Policy, rounding_mode Redondeo>
using fixed_point_t = scaled_integer<N, F, Sign, Form, Policy, Redondeo>;
```

### 1. No contradice ADR-023: la confirma

ADR-023 dice que `_t` marca un alias. Aquí `fixed_point_t` **es** un alias, así
que **su `_t` ya es correcto** y no hay nada que renombrar en sus 230 sitios. La
regla no se dobla para que encaje: se cumple por primera vez en este nombre.

Lo que cambia es la fila de la tabla, no el criterio.

### 2. El nombre canónico, para documentación y mensajes, es `scaled_integer`

Los `API_*.md`, los `@brief` y los mensajes de error hablan de `scaled_integer`.
`fixed_point_t` se documenta **como lo que es**: el nombre de dominio del mismo
tipo. Un lector que llegue buscando «fixed point» lo encuentra; uno que lea el
código ve de qué está hecho.

### 3. Qué pasa con los demás nombres de la familia

| hoy | pasa a | por qué |
|---|---|---|
| `ufixed_point_t`, `sfixed_point_t` | **se quedan** | son alias, y su `_t` es correcto |
| `ufixed_64_64_t`, `sfixed_64_64_t` | **se quedan** | ídem, instancias concretas |
| `fixed_point_t.hpp` | **`scaled_integer.hpp`** | la cabecera se llama como la clase (ADR-023, decisión 3) |
| `fixed_point_*.hpp` (5) | **`scaled_integer_*.hpp`** | ídem, los satélites siguen a su cabecera |
| `docs/API_fixed_point*.md` | **`API_scaled_integer*.md`** | la comprobación 3 del armonizador los empareja por nombre |

### 4. El ahorro, medido

| | ADR-023 solo | con ADR-024 |
|---|---|---|
| `fixed_int_t` → `fixed_width_int` | 925 | 925 |
| `fixed_point_t` → … | **230** | **0** (sobrevive como alias) |
| nombres de fichero | 122 | 122 + los de `fixed_point_*` |
| **total** | **1.277** | **~1.047** |

Baja un **18 %**, y lo que desaparece es justo la parte que toca código de
usuario: los 230 sitios que escriben el nombre del tipo.

## Alternativas descartadas

| alternativa | por qué no |
|---|---|
| **`fixed_point` a secas** (lo que decía ADR-023) | Descarta el dato de que el autor de P0037 abandonó ese nombre tras seis revisiones, y deja el código describiendo el concepto en vez de la implementación — al revés de lo que hace el resto de la biblioteca |
| **`scaled_integer` a secas, sin alias** | Lo más consistente con el patrón de adoptar el nombre de la propuesta, pero borra del código el término por el que la gente busca, y obliga a renombrar los 230 sitios para nada |
| **`fixed_point` como clase y `scaled_integer_t` como alias** | La jerarquía al revés: el alias debería ser el nombre de conveniencia, no el técnico. Y `scaled_integer_t` no existe en ninguna fuente |

## Consecuencias

- **El renombrado de la 1.90 no toca el nombre del tipo de punto fijo en código
  de usuario.** Sólo cambian los nombres de fichero y la documentación.
- Hay **dos nombres para un tipo**, y eso hay que documentarlo o confunde. Es lo
  que hace la propia `std` (`std::string` es un alias de
  `basic_string<char>`), pero allí el alias **colapsa parámetros** y aquí no:
  `fixed_point_t` es un sinónimo puro. Se asume a cambio de la descubribilidad.
- Los mensajes de error de plantilla mostrarán `scaled_integer`, no
  `fixed_point_t`. Conviene que la guía de migración lo diga, porque es lo
  primero que desconcierta.

## La regla que sale

**El nombre de la clase describe la implementación; el alias, el concepto.** Y
sigue valiendo la de ADR-023: si lleva `_t`, es un alias.

---

**Relacionados:** [ADR-023](ADR-023-sin-sufijo-t-en-las-plantillas-de-clase.md)
(la regla del `_t`, que esto confirma y cuya tabla corrige en una fila),
[ADR-019](ADR-019-punto-fijo-es-un-entero-con-escala.md) (de donde sale el nombre:
su título es la definición), [ADR-021](ADR-021-un-nombre-por-operacion.md) (el
precedente de adoptar el nombre publicado).
