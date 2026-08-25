# ADR-002: Los limbos se almacenan en orden little-endian

**Estado:** ✅ Aceptado · documentado a posteriori
**Decidido:** antes de este repositorio (presente ya en el commit inicial, 11 ene 2026)
**Documentado:** 25 August 2026
**Autor:** Julián Calderón Almendros

> Reconstruido a partir del código y del `CHANGELOG.md`; ver la nota
> [Sobre los ADR 001–005](README.md#sobre-los-adr-001005).

---

## Contexto

Un entero de `N × 64` bits se guarda como un vector de N limbos de 64 bits. Hay
que decidir en qué orden, y la decisión es irreversible en la práctica: la toca
absolutamente todo, desde el bucle de acarreo hasta la serialización.

El commit inicial ya lo tenía escrito en el comentario del tipo:

> **Memory Layout:**
> - `data[0]`: Low 64 bits (LSB)
> - `data[1]`: High 64 bits (MSB, contains sign or bias information)
> - Always 16 bytes, little-endian indexing

## Decisión

**El limbo de índice 0 es el menos significativo.** Hoy, en `fixed_int_t`, el
almacenamiento es `std::array<std::uint64_t, N> data` y los accesores lo dicen
en su documentación: `limb(i)`, `limbs()`, «los N limbos en orden little-endian»
([fixed_width_int_t.hpp:244-254](../../include/fixed_width_int_t.hpp#L244-L254)).

### Por qué

1. **El acarreo va hacia arriba.** Sumar, restar y multiplicar propagan de menos
   a más significativo. Con little-endian el bucle es `for (i = 0; i < N; ++i)`,
   que es además el orden en que el prefetcher lee la memoria.
2. **Es lo que hace posible `fixed_int_t<N>` genérico.** Al crecer N los limbos
   se **añaden al final** y ninguno cambia de índice. Con el orden inverso, pasar
   de 128 a 256 bits renumeraría todos los limbos existentes, y el mismo valor
   tendría índices distintos según el tipo. Esta es la razón de peso: sin ella
   no habría un `fixed_int_t<N>` sino una familia de tipos sin relación.
3. **Coincide con el orden de bytes de las plataformas objetivo** (x86-64,
   ARM64, RISC-V little-endian), de modo que la conversión a bytes y `bit_cast`
   no reordenan nada en la práctica.

### Lo que es ortogonal

El orden de los **limbos** es cosa de la biblioteca; el orden de los **bytes
dentro** de un `uint64_t` es de la plataforma. No se confunden: la conversión a
`std::array<std::byte, N*8>` se hace con desplazamientos, no con `memcpy`
([fixed_width_int_t.hpp:499-506](../../include/fixed_width_int_t.hpp#L499-L506)),
así que **la serialización es little-endian en toda plataforma**, también en una
big-endian. El formato en disco no depende de dónde se compile.

## Alternativas descartadas

- **Limbos big-endian (índice 0 = más significativo).** Ganaría en un solo sitio:
  la comparación sería un recorrido lexicográfico directo. Pierde en el acarreo,
  en la genericidad sobre N —motivo 2, que es descalificante— y en la
  correspondencia con la memoria.
- **Guardar el signo o la marca en el limbo 0.** Rompería la propiedad de que los
  limbos numéricos son contiguos desde el 0, que es de lo que se aprovechan los
  algoritmos de Knuth y Karatsuba.

## Consecuencias

### Positivas

- Un bucle ascendente para toda la aritmética.
- `fixed_int_t<N>` es genuinamente genérico en N.
- Los algoritmos clásicos (Knuth D, Karatsuba) están escritos para este orden en
  la literatura; no hay que invertir los índices al transcribirlos.

### Negativas

- **Todo lo textual va al revés.** `to_string()` y la impresión recorren de N-1 a
  0, y el limbo del signo es el último, lo que se lee raro la primera vez.
- Al depurar, un volcado de memoria muestra el número «invertido» respecto a
  cómo se escribe.

## Referencias

- [ADR-003](ADR-003-std-byte-para-buffers.md): el tipo de los bytes que produce
  esta serialización.
- `CHANGELOG.md`, «Priority 11: Array & Bitset Conversions» (18 ene 2026), donde
  se fija el contrato de bytes.
