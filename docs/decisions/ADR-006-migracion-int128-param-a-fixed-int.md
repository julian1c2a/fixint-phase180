# ADR-006: Replicar `int128_param_t` en `fixed_int_t` hasta retirarlo

**Estado:** Aceptado
**Fecha:** 2026-08-24
**Autor:** Julián Calderón Almendros

---

## Contexto

El proyecto tiene hoy **dos tipos enteros de precisión extendida** que se solapan:

| | `int128_param_t<Sign, Form>` | `fixed_int_t<N, Sign, Form>` |
|---|---|---|
| Anchura | fija, 128 bits | cualquier N × 64 bits |
| Representaciones | 4: TC, MS, EK, binnat | 2: binnat, complemento a dos |
| Operadores cross-sign | **no** (gap conocido desde v1.81) | sí, con las conversiones usuales de C++ |
| `div`/`mod` `constexpr` | sí | sí (desde v1.90.1) |
| iostreams, `std::format`, `std::hash` | sí | sí (desde v1.90.1) |
| Superficie de headers | 15 ficheros, ~5.800 líneas | 5 ficheros, ~4.400 líneas |

`fixed_int_t` nació como generalización de `int128_param_t` a anchura arbitraria.
Con N = 2 cubre exactamente el mismo caso de uso, y desde v1.90.1 lo hace además
con **más** capacidades: interoperabilidad signed/unsigned al estilo built-in,
que en `int128_param_t` se decidió no implementar (ver la sección MS-INTEROP de
`NEXT_STEPS.md`).

La situación de dos tipos con capacidades cruzadas y distintas tiene un coste
que ya se está pagando:

- **Duplicación real.** Cada capacidad nueva se implementa dos veces, o se
  implementa en uno y el otro se queda atrás. En la auditoría del 23 ago 2026
  hubo que escribir tres headers (`fixed_int_iostreams`, `fixed_int_format`,
  `fixed_int_hash`) que ya existían para el tipo viejo.
- **Coste para quien usa la biblioteca.** No hay una respuesta corta a «¿cuál de
  los dos uso?». La respuesta real —«el viejo si necesitas MS o EK, el nuevo
  para todo lo demás»— obliga a conocer la historia del proyecto.
- **Deuda que crece.** `GM_TABLE` era código muerto en el lado del tipo viejo y
  sobrevivió meses obligando a compilar toda la biblioteca con un flag no
  estándar. Cuanto más código haya en esa mitad, más sitios donde eso pasa.

## Decisión

**Toda la funcionalidad de `int128_param_t` se replica en `fixed_int_t` y, una
vez completada la paridad, `int128_param_t` se retira.**

`fixed_int_t<2, Sign, Form>` pasa a ser el único tipo de 128 bits de la
biblioteca. Las representaciones Magnitude-Sign y Excess-K, que hoy solo existen
en el tipo viejo, **se portan** a `fixed_int_t` como valores adicionales de
`representation_form`; no se abandonan.

La retirada es **gradual y en tres tramos**, sin fecha impuesta:

1. **Paridad de API** (ver el inventario de más abajo). Mientras dure, los dos
   tipos coexisten y `int128_param_t` sigue siendo API pública plena.
2. **Deprecación anunciada.** Cuando la paridad esté completa: `[[deprecated]]`
   en `int128_param_t`, guía de migración en `docs/`, y un alias
   `int128_t = fixed_int_t<2, ...>` para que el código existente siga
   compilando.
3. **Retirada.** En una versión mayor, y no antes de que la deprecación haya
   estado publicada al menos una versión completa.

Ningún tramo empieza antes de que el anterior esté cerrado y verificado con la
suite en verde en los cuatro compiladores.

## Inventario de paridad

Estado a 24 ago 2026. Es el trabajo que implica esta decisión.

### Ya replicado

| Capacidad | Header viejo | Header nuevo |
|---|---|---|
| Traits `nstd::is_*`, `make_signed/unsigned`, `std::common_type` | `int128_param_traits_specializations.hpp` | `fixed_int_traits_specializations.hpp` |
| Conceptos `nstd::integral` y compañía | `int128_param_concepts.hpp` | `fixed_int_concepts.hpp` |
| `std::numeric_limits` | `int128_param_limits.hpp` | `fixed_int_limits.hpp` |
| `std::format` | `int128_param_format.hpp` | `fixed_int_format.hpp` |
| iostreams | `int128_param_iostreams.hpp` | `fixed_int_iostreams.hpp` |
| `std::hash` | (dentro de traits) | `fixed_int_hash.hpp` |
| `to_string`/`from_string` en bases 2..36 | en la clase | en la clase |
| División y módulo `constexpr` | en la clase | en la clase |
| `checked_div` y la familia `saturating_add/sub/mul` | `int128_param_safe.hpp` | en la clase (P1.3) |
| `rotl`, `rotr`, `is_power_of_2` y los nombres de `<bit>` (`countl_zero`, `countr_zero`, `popcount`, `bit_width`) | `int128_param_bits.hpp` | `fixed_width_int_t.hpp` (P1.5) |
| `min`, `max`, `clamp`, `midpoint`, `abs_diff`, `abs` libres | `int128_param_cmath.hpp` | `fixed_width_int_t.hpp` (P1.5) |
| `ilog2`, `factorial`, `is_even`, `is_odd`, `sign`, `divmod` libre; `isqrt` ya existía como `sqrt` | `int128_param_numeric.hpp` | `fixed_width_int_t.hpp` (P1.5) |
| `mulhi` (sin signo y con signo) y `mullo`; `widening_mul` ya existía como `mul_wide` | `int128_param_arithmetic.hpp` | `fixed_width_int_t.hpp` (P1.5 tramo 2) |

### Pendiente

Agrupado por header de origen, con lo que falta de verdad —las funciones que ya
existen en `fixed_int_t` bajo otro nombre no cuentan como hueco—:

| Origen | Qué falta en `fixed_int_t` |
|---|---|
| `int128_param_algorithm.hpp` | **Decisión pendiente, ver abajo.** Sus 10 funciones son clones literales de `<algorithm>` restringidos al tipo |
| `int128_param_ranges.hpp` | **Parcialmente en la misma decisión.** `range_stats`/`calculate_stats` y los tres generadores de secuencias no están en `std::`; `iota`, `transform`, `copy_if`, `count_if`, `reduce`, `sum` y `product` sí |
| ~~`int128_param_thread_safety.hpp`~~ | ✅ **portado (18 sep 2026)**: `include/fixed_int_atomic.hpp`. No es copia: el viejo usaba mutex siempre, éste va sin bloqueo donde `is_always_lock_free` lo garantiza — que es la condición exacta para no arrastrar `-latomic` |
| `int128_param_divmod.hpp` | `div<D>` / `mod<D>` / `divmod_const<D>` por divisor constante (Granlund-Montgomery) |
| `representation.hpp` | **las representaciones MS y EK** como `representation_form` de `fixed_int_t` |

**La tabla de abajo se escribió de memoria y le faltaban cosas.** Al portar el
tramo 1 se contrastó función por función contra los tres headers viejos, y
aparecieron cuatro públicas que ninguna fila mencionaba: `is_power_of_2`,
`sign`, `abs` libre y `divmod` libre. Se han portado también. Lo que queda
pendiente hay que leerlo sabiendo eso: es una guía, no un contrato --el contrato
es el header viejo, y hay que abrirlo antes de dar una fila por cerrada.

La única que se decidió **no** portar es `power`, alias de `pow` que existía "por
consistencia con phase166". Esa consistencia es justo la capa que este ADR
retira.

Van seis de once. Las tres que cerró P1.5 --bits, cmath y numeric-- eran las
más independientes: no tocan el núcleo del tipo ni la representación, así que se
podían portar y comprobar por separado. Lo que queda va de menos a más acoplado,
y el último punto es el de más peso: `fixed_int_t` tiene hoy un `static_assert`
que solo admite `binnat` sin signo y complemento a dos con signo. Portar MS y EK
significa generalizarlo y revisar cada operación que hoy asume complemento a
dos.

## Decisión tomada: los clones de `<algorithm>` no se portan

Auditados función por función el 9 sep 2026, al portar el tramo 2.

`int128_param_algorithm.hpp` tiene diez funciones --`fill`, `fill_n`, `reverse`,
`find`, `all_of`, `any_of`, `none_of`, `min_element`, `max_element`,
`accumulate`-- que son **copias literales** de las de `<algorithm>` y
`<numeric>`, restringidas al tipo por el parámetro del valor. `nstd::fill` es
`std::fill` con el mismo cuerpo. No aportan una especialización más rápida ni un
comportamiento distinto.

En `int128_param_ranges.hpp` pasa con una parte: `iota`, `transform`, `copy_if`,
`count_if`, `find_first_if`, `reduce`, `sum` y `product` están en `std::` o son
un `std::reduce` con el `init` puesto. Lo que **no** está en `std::` son
`range_stats` / `calculate_stats` (mínimo, máximo, media y recorrido de una
pasada) y los tres generadores de secuencias --aritmética, geométrica y potencias
de dos--.

**Lo que se comprobó y resultó falso:** la sospecha de que tener
`nstd::fill` junto a `std::fill` crearía ambigüedad al resolver por ADL. No la
crea: la sobrecarga más especializada gana, y se verificó compilando el caso
(`using std::fill;` y llamada sin cualificar sobre un `vector<fixed_int_t>`).
Así que **no hay un argumento de corrección** en contra de portarlas; el
argumento es de coste y de duplicación.

Las tres salidas:

| | Qué implica |
|---|---|
| **(a) No portar los clones** | Se retiran con el header. `std::` ya los cubre y `fixed_int_t` cumple los requisitos de iterador y de comparación que piden. Se portan solo `range_stats`, `calculate_stats` y los tres generadores. Menos código que documentar y mantener |
| **(b) Portarlos todos** | Paridad literal: nada de lo que hoy compila contra `int128_param_t` deja de compilar al cambiar de tipo. Son ~24 funciones más que escribir, documentar y probar, que hacen lo mismo que `std::` |
| **(c) Portarlos como alias finos** | `nstd::fill(...)` reenvía a `std::fill(...)`. Conserva el nombre sin duplicar la lógica, pero deja igualmente 24 nombres en la API |

**Decidido el 10 sep 2026: (a).** Donde había `nstd::fill` sobre
`int128_param_t`, va `std::fill` sobre `fixed_int_t`. El motivo de que
existieran es que `int128_param_t` es anterior a que el tipo cumpliera los
requisitos de `std::`, y eso ya no es cierto.

### La línea que separa lo que hay que portar de lo que no

Esta decisión da una regla general, y es lo más útil que sale de ella. **No
es «`std::` frente a `nstd::`»; es si `std::` acepta el tipo o lo rechaza.**

| Qué pide `std::` | Sobre `fixed_int_t` | Consecuencia |
|---|---|---|
| Nada (algoritmos de iterador: `fill`, `find`, `all_of`, `min_element`, `accumulate`) | **funciona** | `nstd::` no debe duplicarlo |
| `std::is_integral` (`gcd`, `lcm`, `midpoint`, `popcount`, `rotl`, `abs`…) | **lo rechaza** | `nstd::` es la **única** opción |

Verificado compilando: `std::find`, `std::min_element`, `std::accumulate`,
`std::all_of` y `std::fill` funcionan sobre un `vector<fixed_int_t>`; y
`std::gcd` falla con *«std::gcd arguments must be integers»*.

La causa de la segunda fila es la decisión de v1.80: **`std::is_integral` no se
puede especializar** —lo prohíbe el estándar— así que `fixed_int_t` nunca será
`std::integral`, y todo lo de `std::` restringido a enteros lo rechaza. Por eso
existe `nstd::`, y por eso el tramo 1 de P1.5 no es duplicación sino la única
forma de tener esas funciones.

Lo que **sí** se porta de `int128_param_ranges.hpp`, porque no está en `std::`:
`range_stats` / `calculate_stats` y los tres generadores de secuencias.

## Consecuencias

### Positivas

- **Un solo tipo que aprender y documentar.** `fixed_int_t<N>` con N = 2 es el
  de 128 bits; no hay que explicar por qué existen dos.
- **Cada capacidad se implementa una vez.** Se acaba la duplicación que ya
  costó tres headers en la última sesión.
- **El caso de 128 bits gana la interoperabilidad signed/unsigned** que se
  decidió no llevar a `int128_param_t`.
- **Menos superficie donde esconder deuda.** El episodio de `GM_TABLE` es el
  precedente.

### Negativas

- **Es mucho trabajo.** Diez headers por portar, y uno de ellos —las
  representaciones MS y EK— toca el núcleo del tipo.
- **Rompe a los usuarios de `int128_param_t`**, aunque sea con aviso y con
  alias. Hoy son código propio y sus tests, pero la biblioteca es pública desde
  que tiene `LICENSE.txt`.
- **Riesgo de regresión en el camino.** `int128_param_t` lleva años de
  afinado y de benchmarks; `fixed_int_t<2>` tiene que igualarlos, no solo
  compilar.
- **Los tests se duplican durante la transición** y la suite se alarga.

### Neutras

- La numeración de versiones: la retirada es un cambio mayor y marca la 2.0.

## Alternativas consideradas

### A. Coexistencia permanente, documentando cuándo usar cada uno

Descartada. Es lo que hay hoy de facto y su coste es real: cada capacidad nueva
se implementa dos veces o se queda coja en un lado, y el usuario tiene que
conocer la historia del proyecto para elegir. Además la justificación de la
coexistencia —«el viejo tiene MS y EK»— desaparece en cuanto se porten, que es
trabajo acotado.

### B. Deprecar `int128_param_t` sin portar MS ni EK

Descartada. Sería perder funcionalidad: las cuatro representaciones son uno de
los objetivos del proyecto (ver `AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`),
no un accidente. Retirar el tipo sin llevárselas convertiría una unificación en
un recorte.

### C. Convertir `int128_param_t` en un alias fino de `fixed_int_t<2>`

Descartada como atajo, pero **es el punto final del tramo 2**. Hacerlo antes de
tener MS y EK en `fixed_int_t` obligaría a que el alias mintiese sobre las
representaciones que acepta.

### D. Quedarse con `int128_param_t` y retirar `fixed_int_t`

Descartada. `fixed_int_t` es el que generaliza a cualquier anchura, el que tiene
la interoperabilidad al estilo built-in y el que ya ocupa el sitio de los tipos
de 256 bits. Retirarlo sería ir contra el objetivo declarado de la rama.

## Relación con la política de desbordamiento

Esta migración y la decisión sobre **aritmética no modular** (operaciones
*checked*, saturantes, o una política como parámetro de plantilla) deben
diseñarse **juntas**. Portar diez headers y después cambiar la semántica de
desbordamiento significaría portar la API dos veces. Si se opta por una política
como parámetro de plantilla —`fixed_int_t<N, Sign, Form, Policy>`—, ese es el
momento de introducirla, antes de replicar la superficie del tipo viejo.

## Referencias

- `NEXT_STEPS.md`, sección MS-INTEROP: la decisión de v1.81 de no llevar los
  operadores cross-sign a `int128_param_t`.
- `docs/API_fixed_int.md`, `docs/API_fixed_int_stl.md`: la API que ya existe.
- `docs/API_parameterized.md`: la que hay que replicar.
- `AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`: los 12 objetivos, entre
  ellos las cuatro representaciones.
