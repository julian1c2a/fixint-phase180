# ADR-007: La política de desbordamiento pasa a ser parámetro de plantilla

**Estado:** Aceptado
**Fecha:** 2026-08-24
**Autor:** Julián Calderón Almendros

---

## Contexto

Hoy `fixed_int_t<N, Sign, Form>` es **estrictamente modular**: toda operación
envuelve en 2^(64N), igual que un `unsigned` de C++ y que un entero con signo en
complemento a dos de C++20. `min() / -1` da `min()`. No hay comportamiento
indefinido en ninguna operación, pero tampoco hay forma de enterarse de que un
resultado se ha salido de rango.

Lo único que existe para eso son tres funciones libres —`checked_add`,
`checked_sub`, `checked_mul`— que devuelven `std::optional`. Es una solución
parcial y con dos problemas: no cubre la división, el desplazamiento ni la
negación, y **obliga a cambiar la forma de escribir el código**. Quien quiere
seguridad deja de escribir `a + b` y pasa a encadenar `optional`, lo que en la
práctica significa que casi nadie la usa.

La auditoría del 23 ago 2026 dejó esta idea anotada como pendiente, con cuatro
opciones sobre la mesa. [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md)
señaló además que la decisión no es independiente: replicar los diez headers de
`int128_param_t` y **después** cambiar la semántica de desbordamiento
significaría portar la API dos veces.

## Decisión

**La política de desbordamiento pasa a ser un parámetro de plantilla de
`fixed_int_t`.**

Y, como consecuencia directa de la advertencia de ADR-006:

> **Se introduce ANTES de replicar la API de `int128_param_t`.** El orden de
> trabajo queda fijado: primero la política, después la paridad de ADR-006.

Con la política en la firma del tipo, el usuario escribe `a + b` de siempre y es
el tipo el que decide qué pasa al desbordar. No hay que reescribir el código
llamante para ganar seguridad.

## Consecuencias

### Positivas

- **La seguridad deja de ser opt-in por llamada.** Se elige una vez, al declarar
  el tipo, y vale para toda la aritmética.
- **Cubre todas las operaciones**, no solo suma, resta y multiplicación.
- **Se implementa una sola vez.** Al ir antes que ADR-006, los diez headers que
  hay que portar nacen ya conscientes de la política.
- Alinea la biblioteca con lo que hacen Rust y Boost.SafeNumerics, que es lo que
  espera quien viene de ahí.

### Negativas

- **Cambia la firma del tipo.** Toca `std::common_type`, `std::numeric_limits`,
  los traits `nstd::is_*`, los conceptos, las especializaciones de `std::hash` y
  `std::formatter`, y los operadores cross-N y cross-signo, que ahora tienen que
  decidir también qué política sale del cruce.
- **Es un cambio mayor**: marca la 2.0.
- **Riesgo de explosión combinatoria en los tests.** Hoy la suite ya cruza N ×
  signo; añadir la política multiplica. Habrá que elegir qué combinaciones se
  barren exhaustivamente y cuáles por muestreo, y decirlo por escrito.
- **Coste en tiempo de compilación**: más instanciaciones distintas del mismo
  código.

### Neutras

- El rendimiento del camino modular no debería cambiar: la política se resuelve
  en compilación con `if constexpr`, y la rama modular queda igual que hoy.
  **Hay que medirlo, no suponerlo.**

## Lo que queda por decidir

> **Actualización 2026-08-24:** las seis preguntas de esta sección quedaron
> respondidas el mismo día en
> [ADR-008](ADR-008-diseno-de-la-politica-de-desbordamiento.md). Se conservan
> aquí tal como se plantearon, porque un ADR no se reescribe.


La decisión de fondo está tomada; el diseño concreto no. Estas son las preguntas
abiertas, que conviene cerrar antes de escribir código:

1. **Qué políticas.** Como mínimo `wrap` (la de hoy) y `checked`. Candidatas:
   `saturate` (satura en `max()`/`min()`), `trap` (aborta o lanza). ¿Las cuatro,
   o dos ahora y las demás después?
2. **Cuál es la de por defecto.** `wrap` mantiene la compatibilidad de todo el
   código existente; `checked` sería más segura pero rompe a todo el mundo en
   silencio, que es el peor tipo de ruptura.
3. **Cómo informa `checked`.** Excepción, `std::optional`, `std::expected`, o un
   bit de estado pegajoso en el valor. Cada opción tiene un coste distinto en
   rendimiento y en ergonomía, y condiciona si las operaciones pueden seguir
   siendo `constexpr` y `noexcept`.
4. **Dónde va en la lista de parámetros.** `fixed_int_t<N, Sign, Form, Policy>`
   es lo natural, pero cualquier posición rompe el código que hoy escribe la
   plantilla completa. Los alias `uint_fixed_t<N>` e `int_fixed_t<N>` amortiguan
   casi todo.
5. **Qué pasa al mezclar políticas.** `a + b` con `a` en `wrap` y `b` en
   `checked`: ¿gana la más estricta, es un error de compilación, o se aplican
   las conversiones usuales? Mi recomendación de partida es **error de
   compilación**: mezclar políticas casi siempre es un descuido.
6. **Interacción con las representaciones MS y EK** que ADR-006 va a portar. En
   Exceso-K el desbordamiento no significa lo mismo que en complemento a dos.

## Alternativas consideradas

### A. Completar las funciones *checked* y añadir las saturantes

Descartada como solución de fondo, aunque sigue siendo útil como paso previo.
Es barata —`checked_div`, `checked_neg`, `checked_shl`, la familia
`saturating_*`: cosa de una tarde— pero no resuelve el problema real, que es que
la seguridad obligue a reescribir el código llamante. Puede implementarse antes
como base sobre la que apoyar la política.

### B. Un tipo envoltorio `safe_int<T>`

Descartada. Replicar toda la superficie de operadores en un envoltorio cuesta
casi lo mismo que la política, y deja dos tipos donde hay que explicar cuál usar
—exactamente el problema que ADR-006 va a resolver con `int128_param_t`—.

### C. Dejarlo como está, modular y con las *checked* sueltas

Descartada. Es lo que hay hoy y su coste es conocido: la seguridad existe pero
casi nadie la usa porque exige cambiar la forma de escribir.

### D. Política como parámetro de plantilla — **elegida**

Es la más invasiva y la única que hace que `a + b` signifique lo que el usuario
quiere sin reescribir nada. Su coste —cambiar la firma del tipo— es aceptable
**ahora**, y no lo sería después de portar los diez headers de ADR-006: de ahí
el orden que fija esta decisión.

## Referencias

- [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md), que anotó la
  interacción entre esta decisión y el orden de la migración.
- `NEXT_STEPS.md`, plan de la auditoría del 23 ago 2026, donde la idea quedó
  anotada como pendiente.
- `include/int128_param_safe.hpp`: las `checked_*` y `saturating_*` que ya
  existen para el tipo viejo y que sirven de punto de partida.
