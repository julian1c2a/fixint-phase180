# ADR-005: La representación es un parámetro de plantilla (binnat, TC, MS, EK)

**Estado:** ✅ Aceptado · documentado a posteriori
**Decidido:** antes de este repositorio (presente ya en el commit inicial, 11 ene 2026)
**Documentado:** 25 August 2026
**Autor:** Julián Calderón Almendros

> Reconstruido a partir del código y del `CHANGELOG.md`; ver la nota
> [Sobre los ADR 001–005](README.md#sobre-los-adr-001005).

---

## Contexto

Es la decisión que da forma al proyecto entero, y la que explica por qué esta
biblioteca no es simplemente «un entero de 128 bits». El comentario del tipo, en
el commit inicial, dice para qué se hizo:

> This template extends the unified template from Phase 1.66 by adding a
> `representation_form` parameter, **allowing investigation into different
> encodings suitable for floating-point research**.

Y sobre Exceso-K, en el mismo fichero:

> Used primarily for exponents in IEEE 754 floating point.

Ahí está el propósito: el objetivo no era competir con `__int128`, sino tener un
**sustrato para investigar codificaciones** con vistas a implementar coma
flotante. Y una coma flotante IEEE-754 guarda precisamente **la mantisa en
magnitud-signo y el exponente en exceso-K**, no en complemento a dos. Las dos
representaciones «raras» de esta biblioteca no son un capricho académico: son las
dos piezas de un `float`.

## Decisión

La representación es un **parámetro de plantilla**, no un campo en tiempo de
ejecución ni cuatro tipos separados:

```cpp
template <signedness Sign, representation_form Form>
class int128_param_t;
```

con cuatro formas ([representation.hpp:53](../../include/representation.hpp#L53)):

| Forma | Qué es | Para qué |
|---|---|---|
| `binnat` | binario natural, sin signo | el caso sin signo puro |
| `twos_complement` | complemento a dos | lo que hace el hardware; intrínsecos |
| `magnitude_sign` | bit de signo + magnitud | **la mantisa** de IEEE-754; tiene `+0` y `-0` |
| `excess_k` | valor sesgado, real = almacenado − k | **el exponente** de IEEE-754 |

### Por qué parámetro de plantilla

- **Coste cero en ejecución.** No hay un `switch` por operación ni un campo que
  ocupe espacio; el compilador genera el código de una sola forma.
- **Cada combinación es un tipo distinto**, así que mezclar un MS con un TC no
  compila en vez de dar un número equivocado. Es la misma idea que
  [ADR-007](ADR-007-politica-de-desbordamiento-como-parametro.md) aplicaría
  después a la política de desbordamiento.
- **Permite `representation_traits<Form>`**, con `has_implicit_sign_bit`,
  `uses_inversion`, `hardware_optimized`, `has_two_zeros`: el código genérico
  consulta la traits en vez de ramificar por forma
  ([representation.hpp:133+](../../include/representation.hpp#L133)).

## Alternativas descartadas

- **Un campo de representación en tiempo de ejecución.** Cuesta espacio en cada
  objeto, mete una rama en cada operación, y permite mezclar representaciones sin
  que nada avise.
- **Cuatro clases sin relación.** Duplicaría cuatro veces la aritmética común y
  haría imposible escribir un algoritmo —Knuth D, Karatsuba— una sola vez.

## Consecuencias

### Positivas

- Una sola implementación de cada algoritmo, parametrizada.
- El invariante «la representación es inmutable tras la construcción», que ya
  figuraba en el commit inicial, sale gratis: está en el tipo.

### Negativas — la deuda que esto genera

- **La superficie de pruebas se multiplica.** Cuatro formas por dos signos son
  ocho combinaciones, y no todas están implementadas ni probadas por igual.
- **MS y EK no comparten la noción de desbordamiento del complemento a dos**, así
  que cada operación necesita su tratamiento explícito de signo o de sesgo. Es lo
  más pesado de portar en
  [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md), y lo que obligó a
  responder la pregunta 6 de
  [ADR-008](ADR-008-diseno-de-la-politica-de-desbordamiento.md).

## Dos cabos sueltos, para la migración de ADR-006

Detectados al escribir este documento. No se resuelven aquí —la decisión que este
ADR documenta es anterior— pero conviene que la migración los cierre en vez de
arrastrarlos:

1. **Los dos tipos no admiten las mismas combinaciones.** `int128_param_t`
   documenta un alias `uint128_tc_t` (sin signo + complemento a dos), mientras
   que `fixed_int_t` lo prohíbe con un `static_assert`: solo acepta
   `binnat`+sin signo y `twos_complement`+con signo
   ([fixed_width_int_t.hpp:207-209](../../include/fixed_width_int_t.hpp#L207-L209)).
   Hay que decidir cuál de las dos reglas sobrevive.
2. **`binnat` no tiene especialización de `representation_traits`.** Las otras
   tres sí. El código genérico que consulte la traits de `binnat` no compila.

## Referencias

- `include/representation.hpp`: el enumerado y las traits.
- [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md): la migración que debe
  llevar las cuatro formas a `fixed_int_t`.
- `CHANGELOG.md`, entradas de enero y febrero de 2026 sobre MS y EK.
