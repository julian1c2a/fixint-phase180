# Plan del acompañamiento de `std` — llegar a 24 de 24

**Fecha:** 22 September 2026 · **Tarea:** P4 (nueva)

> Plan para que **las dos formas nuevas** —`fixed_int_t` en sus cuatro
> representaciones y `fixed_point_t`— tengan el acompañamiento completo de la
> biblioteca estándar, y para unificar los nombres que hoy divergen.
>
> Todo lo que hay aquí está **medido compilando**, no leído. Las cifras salen de
> `scripts/check_acompanamiento_std.py`, que se añade en la etapa 0 para poder
> ir tachando contra una comprobación y no contra una lista escrita de memoria.

---

## Lo que la medición cambió respecto a lo que parecía

Antes de ordenar nada, tres hallazgos que **reducen el trabajo** y uno que lo
aumenta. Los cuatro salieron de sondas, no de leer los ficheros.

### 1. `<algorithm>` ya funciona. No hay nada que portar

Es el hallazgo que más cambia el plan. Hoy, sin tocar una línea, compilan para
`uint_fixed_t<2>` **y** para `sfixed_point_t<2,1>`:

```
std::ranges::sort   std::ranges::find    std::ranges::max_element
std::ranges::count  std::ranges::reverse std::accumulate   std::iota
```

Y los conceptos que esos algoritmos piden se cumplen todos:

| | `regular` | `totally_ordered` | `sortable<T*>` | `permutable<T*>` | `three_way_comparable` |
|---|---|---|---|---|---|
| `fixed_int_t` | sí | sí | sí | sí | sí |
| `fixed_point_t` | sí | sí | sí | — | sí |

**Lo único que falla es `std::integral<T>`, y eso no tiene arreglo**: es un
concepto del núcleo, atado a los tipos del lenguaje. Ningún tipo de usuario
puede satisfacerlo.

Eso confirma la regla del 10 sep —*no es `std::` frente a `nstd::`, es si
`std::` **acepta o rechaza** el tipo*— y la deja con un alcance exacto y
pequeño: lo que hay que duplicar en `nstd::` es **sólo** lo que el estándar
restringe a `integral`:

- `std::gcd` / `std::lcm` → ya existen como `nstd::gcd` / `nstd::lcm` ✅
- `std::midpoint` → existe en 1.75, **falta en limbos** ❌
- `<bit>` entero → `popcount`, `countl_zero`, `countr_zero`, `rotl`, `rotr`,
  `bit_width` ya existen; **faltan seis** ❌
- `std::to_chars` / `from_chars` → **faltan en las dos familias** ❌

`int128_param_algorithm.hpp` —con `accumulate`, `find`, `max_element`,
`reverse`, `fill`…— es por tanto **código muerto por duplicado**: reimplementa
cosas que `std::` ya acepta. Su retirada entra en el plan como etapa aparte, no
como porte.

### 2. `views::iota` cuesta **una línea**, no un rediseño

La duda era si `difference_type` podía ser el propio tipo. No puede: el estándar
exige `is-signed-integer-like<iter_difference_t<I>>`, y eso sólo lo cumplen los
enteros con signo del lenguaje y los *integer-class types*, que son **definidos
por la implementación** — un tipo de usuario no puede serlo.

Pero no hace falta. Comprobado con una sonda que especializa
`std::incrementable_traits` desde fuera, sin tocar las cabeceras:

```
weakly_incrementable<uint_fixed_t<2>>    1
weakly_incrementable<sfixed_point_t<2,1>> 1
views::iota(U{0}, U{5}) recorre 5 elementos
views::iota(Q{0}, Q{5}) recorre 5 elementos
```

Con `difference_type = std::ptrdiff_t` y el `operator++` que **ya devuelve
`T&`**, funciona en las dos formas. En el punto fijo el paso es **uno**, que es
coherente con la decisión 2 de ADR-020.

`input_or_output_iterator` sigue siendo falso, y **está bien**: un número no es
un iterador, le falta `operator*`. No es una carencia.

### 3. Las divergencias de nombre son **tres**, y una es de firma

No sólo `sqrt`. Medido llamando a 43 nombres contra las dos familias:

| Qué | 1.75 | limbos | Tipo de divergencia |
|---|---|---|---|
| raíz entera | `isqrt` | `sqrt` | **nombre** |
| producto ancho | `widening_mul` | `mul_wide` | **nombre** |
| potencia | `pow(T, unsigned)` | `pow(T, T)` | **firma** — el mismo nombre acepta cosas distintas |

Y una cuarta, contra el propio estándar: existe `is_power_of_2` en las dos
familias, pero **el nombre de `<bit>` es `has_single_bit`** y no está en
ninguna.

### 4. Lo que sí crece: el punto fijo no tiene nada

19 de 24 capacidades ausentes. Y **no es portar**: varias no significan lo mismo
en punto fijo que en un entero, y decidir qué significan es la parte cara.

---

## Alcance: qué formas entran

**Entran `fixed_int_t` y `fixed_point_t`.** El objetivo de 24/24 es para las dos
formas nuevas.

**`int128_param_t` (1.75) no entra**, salvo en la unificación de nombres, donde
es una de las dos partes. El motivo está en
[ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md): esa familia
está en migración hacia `fixed_int_t`, y completarla sería invertir en lo que se
va a retirar. Sí se le pone `<=>`, porque es la única capacidad donde las formas
nuevas van por delante y su ausencia se nota al comparar.

> ⚠️ **Si quieres 1.75 también a 24/24, dilo y cambia el plan**: son ~14
> capacidades más y una familia de 16 cabeceras. El plan asume que no.

---

## El orden, y por qué es ese

```
   E0  el verificador ........................ sin él no se puede tachar
   ↓
   E1  nombres (ADR) ......................... antes de escribir más código
   ↓                                            que use los nombres viejos
   E2  ranges: difference_type ............... barato, desbloquea E5
   ↓
   E3  <bit> y to_chars completos ............ cierra los limbos: 24/24
   ↓
   E4  numeric_limits del punto fijo (ADR) ... el que tiene diseño de verdad
   ↓
   E5  traits, hash, format, iostreams ....... dependen de E4
   ↓
   E6  funciones libres del punto fijo ....... dependen de E1 y E4
   ↓
   E7  retirar int128_param_algorithm ........ opcional, limpieza
```

La única dependencia dura es **E4 antes de E5**: `format` y `traits` necesitan
saber qué dice `numeric_limits` (dígitos, si es exacto, si es entero). Lo demás
se puede reordenar.

---

## E0 — El verificador, para poder tachar ✅ **hecho (23 sep)**

- [x] `scripts/check_acompanamiento_std.py`: las 24 capacidades × las formas,
      **comprobadas compilando**, con sonda de arranque
- [x] Cada capacidad declara `aplica` / `no aplica` / `aplica con otra
      semántica` por forma — el «no aplica» es parte de la respuesta, no un
      hueco
- [ ] Conectarlo a `make.py` y al CI **cuando llegue a 24/24**. Hoy devuelve 1
      por diseño --faltan capacidades-- así que meterlo ahora dejaría el CI en
      rojo de forma permanente y el rojo dejaría de significar nada. Mientras
      tanto se corre a mano, como la matriz de paridad
- [x] Sembrarlo con el estado de partida: **uint 22/24, int/C2-MS-EK 19/24, punto fijo 8/24**

> **Va primero por lo que costó descubrirlo.** Mi primera auditoría dio
> «`std::hash` no existe para los limbos» porque sólo incluía la cabecera
> principal, y hay un `fixed_int_hash.hpp`. Y tres de sus sondas —`epsilon()`,
> `common_type_t<T,T>`, `std::swap`— **no podían dar «no»**: compilan para
> cualquier tipo por la plantilla primaria. Un verificador que no sabe fallar
> convierte cada tachón en una promesa sin respaldo.

## E1 — Unificar los nombres · **ADR-021** ✅ **hecho (23 sep)**

- [x] **ADR-021**, con **cuatro** decisiones:
  - [x] raíz: un solo nombre. Recomendación: **`sqrt`**, porque es el de
        `std::`, con `isqrt` como alias en desuso. Ojo: en punto fijo `sqrt`
        **no es** `floor(√x)` sino la raíz redondeada según la perilla — el
        nombre se comparte, la semántica se documenta por forma
  - [x] producto ancho: **`mul_wide`**, con `widening_mul` como alias
  - [x] `pow`: las **dos** sobrecargas. Recomendación: **`pow(T, T)` y `pow(T, unsigned)`**
        como sobrecargas, no una u otra
- [x] Añadir `has_single_bit` como nombre principal; `is_power_of_2` pasa a alias
- [x] Los alias en desuso **se documentan y se vigilan** (sin `[[deprecated]]`: el CI usa `-Werror`), no se borran en silencio
      ([ADR-012](decisions/ADR-012-no-se-mueve-un-tag-publicado.md))
- [x] Sondas en la matriz: los dos nombres compilan y **dan lo mismo**

> Va antes que todo lo demás **porque cada etapa posterior escribe llamadas**.
> Unificar después significa reescribirlas.

## E2 — `std::ranges` de verdad ✅ **hecho (23 sep)**

- [x] `difference_type = std::ptrdiff_t` en `fixed_int_t` y en `fixed_point_t`
- [x] Decidido: **miembro**. miembro de la clase (cero includes) o especialización
      de `std::incrementable_traits` en una cabecera aparte (núcleo más limpio).
      Recomendación: **miembro**, porque es el punto de personalización que el
      estándar lee primero y no obliga a incluir nada
- [x] Comprobado que `operator++` devuelve `T&` en las cuatro representaciones
      —MS y EK incluidas— y no sólo en complemento a dos
- [x] Test: `views::iota`, `views::take`, `views::filter`
- [x] **Falsificado**: quitar `difference_type` y comprobar que el test se pone
      rojo
- [x] Documentado que `input_or_output_iterator` es falso **a propósito**

## E3 — Cerrar los limbos: `<bit>` y `to_chars`

Lo que falta para que `fixed_int_t` llegue a 24/24.

- [ ] `countl_one`, `countr_one`
- [ ] `bit_ceil`, `bit_floor`, `has_single_bit`
- [ ] `byteswap`
- [x] `midpoint` **con signo** — y de paso se arregló: no redondeaba hacia `a` (existe sin signo; el verificador lo sacó)
- [x] `floor`, `ceil`, `round`, `trunc` **también para los enteros** — son la
      identidad, pero código genérico los llama y hoy no existen en ninguna forma
- [x] ~~`sqrt` con signo~~ — hecho en E1 (ADR-021, decisión 3)
- [ ] `to_chars` / `from_chars` con base, para las dos familias
- [ ] Todos en las **cuatro** representaciones, cruzados contra complemento a
      dos como fijó [ADR-018](decisions/ADR-018-la-representacion-no-es-observable.md)

## E4 — `numeric_limits` del punto fijo · **ADR-022** ✅ **hecho (23 sep)**

**La etapa con diseño de verdad.** No es copiar la del entero: la mitad de los
miembros significan otra cosa.

- [x] **ADR-022**, escrito y **corregido por el test** en `round_error()`:
  - [ ] `is_integer` → **`false`**. Es lo que separa este tipo del entero
  - [ ] `is_exact` → **`true`**: el conjunto representable es exacto, aunque las
        operaciones redondeen
  - [ ] `digits` → ¿bits totales, o sólo los de la parte entera? En coma
        flotante son los de la mantisa; aquí no hay mantisa
  - [ ] `epsilon()` → el `ulp`, que **ya existe en el tipo**. En coma flotante
        `epsilon` es relativo; aquí es **absoluto**, y esa diferencia hay que
        escribirla
  - [ ] `min()` → en coma flotante es el **positivo más pequeño**; en un entero
        es el **más negativo**. El punto fijo tiene que elegir, y elegir mal
        rompe el código genérico en silencio
  - [ ] `round_style` → tiene que **leer la perilla `Redondeo`**, no ser fijo
  - [ ] `radix`, `max_digits10`, `digits10`
- [x] Implementar `fixed_point_limits.hpp`
- [x] Test cruzado contra la aritmética: `epsilon()` es el paso real entre dos
      valores consecutivos, `max()+epsilon()` desborda, etc.

> `min()` es la trampa. Un algoritmo genérico que use `numeric_limits<T>::min()`
> como «el más pequeño» hará cosas distintas según la respuesta, **y compilará
> en los dos casos**.

## E5 — Traits, hash, format, iostreams del punto fijo ✅ **hecho (23 sep)**

- [x] `fixed_point_traits_specializations.hpp`
  - [ ] `nstd::is_integral_v` → decidir: probablemente **`false`**, y entonces
        hace falta `nstd::is_fixed_point_v`
  - [ ] `nstd::make_unsigned`, `nstd::make_signed`
  - [ ] `std::common_type` con enteros del lenguaje y con `fixed_int_t`
- [x] `fixed_point_hash.hpp` — y decidir si dos tipos con el mismo valor y
      distinta `Form` deben dar **el mismo** hash (por ADR-018, sí)
- [x] `fixed_point_format.hpp`
  - [ ] `std::formatter` con **precisión**: `{:.3}` se apoya en `to_string(d)`
  - [ ] `{:f}`, `{:e}`? Decidir qué presentaciones se admiten
- [x] `fixed_point_iostreams.hpp` — `<<` y `>>`, con `std::setprecision`

## E6 — Funciones libres del punto fijo ✅ **hecho (23 sep)**

- [x] `abs`
- [x] `floor`, `ceil`, `round`, `trunc` — **los cuatro**, y aquí `suelo()` es
      `floor`. `round` tiene que usar la perilla
- [x] `sqrt` con escala: **no es la del entero**. `√(x/2^k) = √(x·2^k)/2^k`, así
      que hay que preescalar antes de la raíz
- [x] `pow` con exponente entero, en las dos firmas
- [x] `checked_add` / `saturating_add` y familia
- [x] `gcd` / `lcm` → **aplican**, en unidades de `epsilon`. Para múltiplos de `epsilon` tienen
      sentido; puede que la respuesta sea «no aplica», y eso se escribe
- [x] `midpoint`
- [ ] **No** `popcount`, `rotl`, `bit_width`: son de bits, y en punto fijo los
      bits no son el valor (mismo argumento que los bitwise en ADR-020)

## E7 — Retirar `int128_param_algorithm.hpp` (opcional)

- [ ] Comprobar que cada función de ahí tiene equivalente en `std::` que
      **acepta** el tipo
- [ ] Marcar en desuso, no borrar de golpe
- [ ] Recuperar sus tests apuntando a `std::`

---

## Cómo se tacha

Una casilla se tacha cuando **`check_acompanamiento_std.py` lo dice**, no
cuando el código está escrito. Y cada etapa lleva su falsificación: romper a
propósito lo que se acaba de escribir y comprobar que el test se pone rojo.

Esa disciplina ya ha pagado cuatro veces en este proyecto, la última el mismo
22 sep: el cruce de `>>` contra `/2^n` pasaba entero **y también pasaba con `>>`
roto**, porque los valores de prueba eran enteros y su crudo tiene 64 bits bajos
a cero. Con crudos de verdad, la misma avería da 1008 fallos.

## Cuenta final

| | Al empezar | Tras E0–E2 | Al acabar |
|---|---|---|---|
| `uint` | 22/24 | 23/24 | **24/24** ✅ |
| `int/C2`, `int/MS`, `int/EK` | 19/24 | 22/24 | **24/24** ✅ |
| `fixed_point_t` | 8/24 | 9/24 | **24/24** ✅ · con cuatro «no aplica» documentados |
| Cabeceras nuevas | — | 5 de punto fijo |
| ADR nuevos | — | 021 (nombres), 022 (`numeric_limits`) |
| Nombres divergentes | 3 + 1 contra `<bit>` | 0 |
