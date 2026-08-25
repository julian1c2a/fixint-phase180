# Convenciones de nombres

**Última actualización:** 25 August 2026

Fuente de verdad de cómo se nombran las cosas en este proyecto. Antes vivía
dentro de `AI_PROMPT/ai-instructions.md` §9, donde nadie la encontraba.

Para el formato del código (llaves, inicialización, `const`, `constexpr`) ver
[STYLE_CONVENTIONS.md](STYLE_CONVENTIONS.md). Para el resto de la guía de
desarrollo, [AI-GUIDE.md](AI-GUIDE.md).

---

## Tabla

| Elemento | Estilo | Ejemplos |
|---|---|---|
| Tipos y clases | `snake_case_t` | `uint128_t`, `fixed_int_t`, `int128_param_t` |
| Parámetros de plantilla | `PascalCase` o mayúscula suelta | `N`, `Sign`, `Form`, `S2`, `F2` |
| Enumerados (`enum class`) | `snake_case` | `signedness`, `parse_error`, `representation_form` |
| Valores de enumerado | `snake_case` | `unsigned_type`, `signed_type`, `success` |
| Funciones y métodos | `snake_case` | `to_string()`, `count_leading_zeros()`, `is_negative()` |
| Variables | `snake_case` | `const std::uint64_t carry` |
| Constantes estáticas públicas | `UPPER_SNAKE` | `BITS`, `BYTES`, `LIMBS` |
| Detalles privados | sufijo `_` | `DIGIT_PAIRS_`, `digit_char_()`, `digit_value_()` |
| Espacios de nombres | `snake_case` | `nstd`, `intrinsics`, `detail` |
| Ficheros | `snake_case.hpp` | `fixed_width_int_t.hpp`, `representation.hpp` |
| Guardas de inclusión | `UPPER_SNAKE_HPP` | `FIXED_WIDTH_INT_T_HPP` |

## Patrones

- **Alias de tipo terminan en `_t`**: `uint128_t`, `uint_fixed_t<N>`.
- **Métodos booleanos empiezan por `is_`**: `is_zero()`, `is_negative()`,
  `is_power_of_two()`.
- **Conversión: `to_`**; **construcción desde: `from_`**. `to_string()`,
  `from_string()`, `from_bytes()`.
- **Variantes que no lanzan: prefijo `try_`**, devolviendo `parse_result` u
  `optional`: `try_from_string()`.
- **Los detalles de implementación llevan sufijo `_`** o viven en
  `namespace detail`. Nunca los dos a la vez.

## Por qué `snake_case_t` y no `PascalCase`

Para que los tipos se lean como los de la biblioteca estándar. `uint_fixed_t<4>`
al lado de `std::uint64_t` y de `std::string` no desentona; `UIntFixed<4>` sí.
La biblioteca aspira a que sus tipos se usen **como si fueran built-in**, y eso
empieza por que se escriban igual.

## Ficheros de tests, benchmarks y demos

```
tests/test_[tipo]_[caracteristica].cpp
benchs/benchmark_[caracteristica].cpp
demos/tutorials/NN_nombre.cpp        secuencia de aprendizaje
demos/examples/nombre.cpp            casos de uso reales
demos/showcase/nombre.cpp            demostraciones avanzadas
```

Los binarios salen a
`build/build_[tests|benchs|demos]/[compilador]/[modo]/[nombre]_[compilador].exe`.

---

## Divergencias entre esta convención y el código de hoy

Escrito el 25 ago 2026 al extraer este documento. La versión anterior de la
convención describía cosas que el código **no cumple**, así que aquí está la
tabla corregida y, abajo, lo que la versión vieja pedía y nunca se aplicó. Hay
que decidir qué gana.

| La convención vieja pedía | El código hace | Ficheros afectados |
|---|---|---|
| `enum class` con sufijo `_ec_t`: `parse_error_ec_t` | `parse_error`, `signedness`, `representation_form` — **sin sufijo** | los 3 enumerados del proyecto |
| Tipos plantilla con sufijo `_tt`: `int128_base_tt.hpp` | ningún fichero usa `_tt` | ninguno |
| Constantes `MSULL` / `LSULL` para índices de limbo | no existen; se usa `limb(0)` y `limb(N-1)` | — |
| Límites numéricos con patrón `UI64_MAX` | se usa `std::numeric_limits` | — |

**Recomendación:** que gane el código. Los tres enumerados llevan años con su
nombre actual, son API pública, y `_ec_t` no aporta nada que `enum class` no
diga ya. Renombrarlos sería una ruptura sin beneficio. Lo mismo con `_tt`: la
plantilla se distingue por su uso, no por su nombre de fichero.

Si se prefiere lo contrario, es una ruptura de API y necesita su propio ADR.
