# ADR-001: Todos los constructores y conversiones son `explicit`

**Estado:** ✅ Aceptado · documentado a posteriori
**Decidido:** antes de este repositorio (presente ya en el commit inicial, 11 ene 2026)
**Documentado:** 25 August 2026
**Autor:** Julián Calderón Almendros

> Reconstruido a partir del código y del `CHANGELOG.md`; ver la nota
> [Sobre los ADR 001–005](README.md#sobre-los-adr-001005).

---

## Contexto

La biblioteca imita la **aritmética** de los enteros built-in de C++: mismos
operadores, mismo envolvimiento, misma sintaxis. La pregunta era si debía imitar
también su **sistema de conversiones**.

Las conversiones implícitas entre enteros del lenguaje son una fuente clásica de
fallos: promociones integrales, cambios de signo silenciosos, estrechamientos que
pierden bits sin avisar. `f(-1)` sobre un parámetro `unsigned` compila y hace
algo que casi nunca es lo que se quería.

## Decisión

**Todo constructor que convierta y toda conversión de salida son `explicit`.**

El estado hoy en `fixed_int_t`:

- **8 constructores `explicit`**; el único no explícito es el por defecto, que no
  puede ser convertidor.
- Las tres conversiones de salida son explícitas:
  `operator bool()`, `operator T()` para integrales,
  `operator F()` para coma flotante ([fixed_width_int_t.hpp:490-534](../../include/fixed_width_int_t.hpp#L490-L534)).

Estaba así desde el primer commit del repositorio, donde ya se lee
`explicit constexpr int128_param_t(T value) noexcept`.

### La consecuencia que hay que asumir

Sin conversión implícita, `a + 42` no funcionaría por conversión: **hay que
proveer sobrecargas heterogéneas explícitas**. Es la razón de que
`fixed_width_int_t.hpp` tenga tantas: `operator==(const uint_fixed_t<N>&, T)`,
`operator/=(T)`, las variantes con `__int128`, las de N distinto entre sí. No es
repetición gratuita: es el precio de esta decisión, pagado a cambio de que ninguna
conversión ocurra sin que alguien la haya escrito.

## Alternativas descartadas

- **Constructor implícito desde los enteros built-in**, por comodidad. Reintroduce
  exactamente lo que se quería evitar, y además crea ambigüedades de sobrecarga
  en cuanto conviven varias anchuras: con `uint128_fixed_t` y `uint256_fixed_t`
  ambas convertibles desde `int`, una llamada con `42` se vuelve ambigua.
- **`explicit` solo en los constructores, no en las conversiones de salida.** Es
  la mitad peligrosa: dejaría que un `fixed_int_t<4>` se degradase a `int` en
  silencio, perdiendo 192 bits.

## Consecuencias

### Positivas

- Ninguna conversión ocurre sin escribirse. Los estrechamientos son errores de
  compilación, no pérdidas silenciosas.
- `explicit operator bool()` **sigue funcionando** en `if (x)`, `while (x)` y
  `!x`: la conversión contextual a `bool` no exige que sea implícita. Se conserva
  la comodidad sin abrir la puerta a la aritmética accidental.

### Negativas

- `f(42)` sobre un parámetro `fixed_int_t` no compila; hay que escribir
  `f(uint256_fixed_t{42})`. Es deliberado, y es lo primero que sorprende a quien
  llega.
- El número de sobrecargas heterogéneas es grande y hay que mantenerlo en paralelo
  para cada operador nuevo.

## Referencias

- [STYLE_CONVENTIONS.md](../../STYLE_CONVENTIONS.md), «Conversiones explícitas».
- [ADR-004](ADR-004-sin-excepciones-en-el-nucleo.md): la otra mitad de imitar la
  aritmética built-in sin heredar sus accidentes.
