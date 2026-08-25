# ADR-004: La aritmética no lanza; las excepciones son para errores de programación

**Estado:** ✅ Aceptado · documentado a posteriori
**Decidido:** antes de este repositorio (el invariante está en el commit inicial, 11 ene 2026)
**Documentado:** 25 August 2026
**Autor:** Julián Calderón Almendros

> Reconstruido a partir del código y del `CHANGELOG.md`; ver la nota
> [Sobre los ADR 001–005](README.md#sobre-los-adr-001005).

---

## Contexto

El comentario del tipo, en el commit inicial, ya declaraba el invariante:

> `@invariant All operations are noexcept unless otherwise noted`

Un entero de anchura fija tiene un modo de fallo que ocurre constantemente: el
desbordamiento. Si desbordar lanzara, **ninguna operación aritmética podría ser
`noexcept`**, cada `+` de un bucle caliente sería un punto de desenrollado
potencial, y la aritmética dejaría de poder usarse en contexto constante.

## Decisión

**Una excepción señala un error de programación, nunca un resultado esperado.**

- **Desbordar es un resultado esperado.** Se devuelve un valor, no se lanza: para
  eso están las `checked_*` y las `saturating_*` de
  `include/int128_param_safe.hpp`.
- **Un índice fuera de rango, una cadena que no es un número o una base fuera de
  [2, 36] son errores de programación.** Ahí sí se lanza.

Es el reparto que hay en el código. Todo lo que lanza en `include/` es:

| Excepción | Cuándo |
|---|---|
| `std::domain_error` | división por cero |
| `std::invalid_argument` | cadena inválida, base fuera de rango |
| `std::out_of_range` | índice de byte, valor que no cabe al parsear |
| `std::format_error` | especificador de `std::format` inválido |

Ninguna sale de una operación aritmética salvo la primera. `fixed_width_int_t.hpp`
lleva **328 apariciones de `noexcept`**, y **todos** los operadores aritméticos lo
son **excepto `/` y `%`**.

### La división por cero, que es la excepción deliberada

Con un `int`, `1/0` es comportamiento indefinido. Aquí es un `std::domain_error`
diagnosticable, y por eso `/` y `%` renuncian a `noexcept`. Es una mejora sobre
el built-in, no una concesión.

Y en contexto constante encaja exactamente: **un `throw` dentro de una función
`constexpr` convierte la expresión en no-constante**, es decir, en un error de
compilación. `constexpr auto x = a / b;` con `b == 0` no compila, igual que
`constexpr int x = 1/0;`.

## Alternativas descartadas

- **Lanzar al desbordar.** Mata el `noexcept` de toda la aritmética, mete un
  punto de desenrollado en cada operación y hace inservible el camino
  `constexpr`. Es justamente lo que ADR-008 volvió a descartar dos años después,
  por los mismos motivos.
- **Un indicador de error global**, al estilo de `errno`. No es seguro entre
  hilos, no existe en contexto constante, y hace que el fallo viaje lejos del
  sitio donde ocurrió.
- **No lanzar nunca, tampoco al dividir por cero.** Devolvería un valor
  cualquiera para una operación sin resultado, y perdería la detección en
  compilación.

## Consecuencias

### Positivas

- **Es lo que hace posible que toda la aritmética sea `constexpr`**, división
  incluida, comprobado con `static_assert` en los tests.
- El código llamante no necesita `try`/`catch` para hacer cuentas.
- El compilador optimiza mejor con `noexcept`: no hay que emitir el andamiaje de
  desenrollado.

### Negativas

- **Quien no compruebe el desbordamiento no se entera.** Es el comportamiento de
  los enteros built-in y era el objetivo, pero es un filo. Toda la línea
  [ADR-007](ADR-007-politica-de-desbordamiento-como-parametro.md) →
  [ADR-010](ADR-010-orden-total-con-valores-invalidos.md) existe para dar una
  respuesta mejor **sin** retirar esta decisión.
- `/` y `%` son la asimetría del conjunto, y hay que recordarlo al escribir
  código genérico que exija `noexcept`.

## Referencias

- [ADR-008](ADR-008-diseno-de-la-politica-de-desbordamiento.md), decisión 3: esta
  decisión es la premisa de la que sale la marca de inválido.
- [STYLE_CONVENTIONS.md](../../STYLE_CONVENTIONS.md), «Errores sin excepciones en
  el camino normal».
- `include/int128_param_safe.hpp`.
