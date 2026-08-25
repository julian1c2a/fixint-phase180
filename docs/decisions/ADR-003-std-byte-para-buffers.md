# ADR-003: Los buffers de bytes son `std::byte`, no `char` ni `uint8_t`

**Estado:** ✅ Aceptado · documentado a posteriori
**Decidido:** antes de este repositorio (presente ya en el commit inicial, 11 ene 2026)
**Documentado:** 25 August 2026
**Autor:** Julián Calderón Almendros

> Reconstruido a partir del código y del `CHANGELOG.md`; ver la nota
> [Sobre los ADR 001–005](README.md#sobre-los-adr-001005).

---

## Contexto

La biblioteca expone el valor como bytes en dos sitios: el acceso byte a byte
(`get_byte` / `set_byte`) y la conversión a y desde
`std::array<std::byte, N*8>`. Había que elegir con qué tipo se representa «un
byte crudo».

Los candidatos históricos de C++ tienen todos el mismo defecto: **son tipos
aritméticos**. Un buffer de bytes no es un vector de números pequeños, y tratarlo
como tal es de donde salen los errores.

## Decisión

**`std::byte`** (C++17), en toda la superficie de bytes.

Estaba así desde el commit inicial: `constexpr std::byte get_byte(size_t) const`
y `set_byte(size_t, std::byte)`. Hoy lo mismo, más la conversión a
`std::array<std::byte, N*8>`.

### Por qué

- **No es un tipo aritmético.** No hay promoción integral, no se puede sumar por
  accidente, no participa en las conversiones implícitas que ADR-001 pretende
  cerrar. Los únicos operadores que tiene son los de bits, que es exactamente lo
  que se hace con un byte crudo.
- **No tiene la ambigüedad de signo de `char`**, cuyo carácter con o sin signo es
  definido por la implementación: el mismo desplazamiento a la derecha da
  resultados distintos en x86 y en ARM.
- **Dice lo que es.** Un `std::array<std::byte, 32>` en una firma comunica «esto
  son bits, no un número» sin comentario que lo acompañe.
- Conserva el permiso de alias de los tipos carácter, así que sigue siendo legal
  inspeccionar la representación de un objeto a través de él.

## Alternativas descartadas

| Candidato | Por qué no |
|---|---|
| `unsigned char` | Aritmético: promociona a `int` y se suma sin querer |
| `char` | Además, con o sin signo según la implementación |
| `std::uint8_t` | Suele ser un alias de `unsigned char`: hereda los mismos problemas y encima aparenta ser un tipo distinto |

## Consecuencias

### Positivas

- Es imposible hacer aritmética con un byte de la biblioteca sin escribir la
  conversión.
- El contrato de serialización queda expresado en el tipo.

### Negativas

- **Hace falta `std::to_integer<T>(b)` para cualquier operación numérica**, y
  `static_cast<std::byte>(...)` para volver. Es verboso. Esa verbosidad *es* la
  decisión: cada conversión entre «bits» y «número» queda escrita.
- Obliga a C++17 como suelo en esa parte de la API. Sin coste real: el proyecto
  pide C++20.

## Referencias

- [ADR-002](ADR-002-almacenamiento-little-endian-de-limbos.md): el orden en que
  salen esos bytes.
- [ADR-001](ADR-001-constructores-y-conversiones-explicitos.md): la misma idea
  —ninguna conversión sin escribirla— aplicada a los constructores.
- [STYLE_CONVENTIONS.md](../../STYLE_CONVENTIONS.md).
