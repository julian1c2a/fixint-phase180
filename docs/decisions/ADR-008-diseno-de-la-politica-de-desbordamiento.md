# ADR-008: Diseño de la política de desbordamiento

**Estado:** Aceptado
**Fecha:** 2026-08-24
**Autor:** Julián Calderón Almendros

**Completa a:** [ADR-007](ADR-007-politica-de-desbordamiento-como-parametro.md),
que decidió que la política sea parámetro de plantilla y dejó seis preguntas de
diseño abiertas. Este ADR las cierra.

---

## Contexto

ADR-007 fijó el *qué*: `fixed_int_t<N, Sign, Form, Policy>`. El *cómo* quedó en
seis preguntas, porque de sus respuestas depende que el diseño sea implementable
y no solo deseable. Se responden aquí.

## Decisiones

### 1. Qué políticas

`wrap` y `checked` en la primera entrega. `saturate` y `trap` quedan
contempladas en el enumerado pero se implementan después.

### 2. Cuál es la de por defecto

**`wrap`**, que es el comportamiento actual y el de los enteros built-in de C++.
Todo el código existente sigue compilando y significando lo mismo.

### 3. Cómo informa `checked`

**No con excepciones.** Toda la aritmética sigue siendo `constexpr` **y**
`noexcept`.

Esta respuesta tiene una consecuencia que conviene ver antes de escribir código,
porque determina el diseño entero:

> Si `a + b` debe seguir devolviendo un `fixed_int_t` —que es toda la premisa de
> ADR-007: escribir la aritmética de siempre— y además ser `noexcept`, entonces
> **el fallo no puede viajar en el tipo de retorno**. Devolver
> `optional<fixed_int_t>` rompe `a + b + c`, que es justo lo que se quería
> conservar.

De modo que `checked` implica **un estado inválido pegajoso dentro del propio
valor**: el equivalente entero de un NaN. La operación que desborda produce un
valor marcado como inválido, la marca **se propaga** por las operaciones
siguientes, y se consulta al final:

```cpp
using u256c = uint_fixed_t<4, overflow_policy::checked>;
u256c r = a * b + c;        // aritmetica normal, noexcept, constexpr
if (!r.valid()) { /* algo desbordo por el camino */ }
```

Reglas de propagación: cualquier operación con al menos un operando inválido
produce un resultado inválido. Las comparaciones con un operando inválido
devuelven `false` salvo `!=`, igual que con NaN. `to_string()` de un valor
inválido devuelve una cadena que lo diga, no basura.

### 4. Dónde va en la lista de parámetros

Cuarto: `fixed_int_t<N, Sign, Form, Policy>`. Los alias `uint_fixed_t<N>` e
`int_fixed_t<N>` mantienen su significado y ganan un parámetro opcional de
política, de modo que el código que usa los alias no cambia.

### 5. Mezclar políticas

**No se mezclan.** `a + b` con políticas distintas es **error de compilación**,
no una conversión silenciosa. Si alguien necesita cruzar dos mundos, que lo
escriba explícitamente con una conversión.

### 6. Interacción con Magnitude-Sign y Excess-K

**Sí la hay, y se reduce al mínimo heredado.** Las representaciones que ADR-006
va a portar no comparten la noción de desbordamiento del complemento a dos:

- En **Magnitude-Signo** hay dos ceros y el rango es simétrico; desbordar por
  arriba y por abajo es la misma condición sobre la magnitud.
- En **Exceso-K** el valor almacenado lleva un sesgo, así que la comprobación no
  es sobre el patrón de bits sino sobre el valor real, y los límites no son
  potencias de dos.

Criterio: **lo mínimo que se pueda heredar**. La política define *qué se hace* al
desbordar; cada representación define *cuándo* ha desbordado, en una única
función por representación. Nada de reimplementar la política cuatro veces.

## Cuestión abierta que queda

Una sola, y es de implementación, no de semántica:

**Dónde vive la marca de inválido.** Hoy `fixed_int_t<N>` ocupa exactamente
`8N` bytes y eso es parte de su contrato: se convierte a y desde
`std::array<std::byte, N*8>`, es trivialmente copiable y `std::bit_cast`
funciona. Un miembro adicional lo llevaría a `8N + 8` con relleno.

Opciones a valorar cuando se implemente:

- **Miembro extra solo cuando `Policy == checked`**, vía especialización o clase
  base vacía. Con `wrap` el tamaño no cambia, que es lo que importa para no
  romper nada de lo que hay hoy.
- **Un bit robado al valor.** Descartable de entrada: en un entero de anchura
  fija todos los patrones de bits son valores válidos; no hay ninguno libre.
- **Un `N+1`-ésimo limbo de estado.** Simple, pero cambia el tamaño siempre.

La primera es la única que no cobra nada a quien use `wrap`, que va a ser la
mayoría.

## Nota sobre `std::expected`

Se planteó usar `std::expected` en vez de `std::optional` para las funciones
`checked_*` que ya existen. Dos hechos:

1. **`std::expected` es C++23.** El suelo del proyecto es C++20 (`CMakeLists.txt`,
   `CMAKE_CXX_STANDARD 20`), y la matriz de CI incluye compiladores donde no está
   disponible. Usarlo obligaría a subir el estándar, que es una decisión aparte.
2. **No hay hoy ningún enumerado que sirva de tipo de error.** Los tres que
   existen son `parse_error` —específico del parseo—, `representation_form` y
   `signedness`.

Por tanto: las `checked_*` **siguen devolviendo `std::optional`** mientras el
suelo sea C++20. Si algún día se sube a C++23, el cambio natural es
`std::expected<fixed_int_t, arith_error>` con un enumerado `arith_error` nuevo
(`overflow`, `underflow`, `division_by_zero`, `invalid_operand`), paralelo a
`parse_error`.

Esto es independiente de la política: la política no usa `optional` para nada,
usa la marca pegajosa de la decisión 3.

## Consecuencias

### Positivas

- La aritmética se escribe igual con cualquier política, y sigue siendo
  `constexpr` y `noexcept`.
- Con `wrap` —la de por defecto— no se paga nada: ni tamaño, ni ramas, ni
  tiempo de compilación apreciable.
- Prohibir la mezcla de políticas convierte en error de compilación lo que en
  otras bibliotecas es una conversión silenciosa.

### Negativas

- **La marca pegajosa es un concepto nuevo que hay que documentar bien.** Un
  entero que puede estar «inválido» sorprende, y quien no compruebe `valid()` al
  final se lleva un valor sin sentido sin enterarse. Es el mismo riesgo que el
  NaN de coma flotante, con la misma mitigación: documentarlo y que la
  conversión a cadena lo diga.
- Cada operación de `checked` tiene que propagar la marca: más código y más
  tests.
- Las comparaciones dejan de ser un orden total cuando hay valores inválidos.

### Neutras

- El enumerado `overflow_policy` nace con cuatro valores aunque solo dos estén
  implementados. Es preferible a ampliarlo después y romper el ABI de la
  plantilla.

## Referencias

- [ADR-007](ADR-007-politica-de-desbordamiento-como-parametro.md): la decisión
  que este ADR completa.
- [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md): la migración que va
  después, y cuyas representaciones MS y EK afecta la decisión 6.
- `include/int128_param_safe.hpp`: las `checked_*` y `saturating_*` existentes.
