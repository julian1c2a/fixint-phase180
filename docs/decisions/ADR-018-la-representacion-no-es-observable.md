# ADR-018: La representación no es observable desde el comportamiento

**Estado:** ✅ Aceptado
**Fecha:** 21 September 2026
**Autor:** Julián Calderón Almendros

> Cierra las tres cuestiones que
> [ADR-017](ADR-017-magnitud-signo-y-exceso-k-como-codificaciones.md) dejó
> abiertas a propósito. No revoca nada de aquél: lo completa.

---

## Contexto

ADR-017 decidió que Magnitud-Signo y Exceso-K son **codificaciones, no
aritméticas**, y dejó tres cuestiones sin cerrar porque adivinarlas habría sido
peor que dejarlas escritas:

1. Qué hacen `<<` y `>>` sobre MS y EK.
2. Qué hacen `&`, `|`, `^` y `~`.
3. Qué orden sigue `<=>`.

Las tres tienen la misma forma: **¿la operación mira el valor o mira los bits?**

## Lo que hacía `int128_param_t`, y por qué no sirve de precedente

Antes de decidir se fue a ver cómo lo resolvía el tipo viejo, que lleva MS y EK
implementados desde antes de este repositorio. **Medido el 21 sep 2026,
ejecutándolo:**

| | `3 << 1` | `-3 >> 1` | `~3` |
|---|---|---|---|
| Complemento a dos | 6 ✓ | −2 | −4 ✓ |
| **Magnitud-Signo** | 6 ✓ | −1 | **−1,70e38** ✗ |
| **Exceso-K** | **8,5e37** ✗ | **−4,25e37** ✗ | **1,70e38** ✗ |

En **Magnitud-Signo** sí hubo una elección deliberada, y está comentada en el
código: *«MS: Extrae signo, desplaza magnitud como unsigned, restaura signo»*.
Eso hace que `>>` trunque hacia cero en vez de redondear hacia −∞. Pero **`~`
está roto**: `~mag_high` pone a uno el bit 63, que es el de signo, y falta
enmascararlo.

En **Exceso-K no hay ninguna rama**. Los desplazamientos operan sobre la
representación sesgada, que no significa nada; los bitwise aciertan por
casualidad con positivos pequeños.

### Por qué no se detectó

Así prueba `~` el test de Magnitud-Signo:

```cpp
TEST("ms not changes value",      ~x != x);               // sólo que cambia
TEST("ms demorgan ~(a&b)==~a|~b", ~(a & b) == (~a | ~b)); // identidad estructural
TEST("ms shl pos sign preserved", !(x << 1).is_negative()); // sólo el signo
```

**Ninguno comprueba el valor.** De Morgan se cumple igual aunque el resultado sea
basura, porque es una identidad estructural: se cumple sobre las magnitudes y los
signos coinciden a los dos lados. Y `test_param_ek.cpp` no prueba desplazamientos
ni bitwise en absoluto.

Es el mismo patrón que P0.4 —`test_template_type.cpp` no comprobaba nada— y el
mismo que [ADR-014](ADR-014-cobertura-de-doxygen.md): **una comprobación que no
puede fallar no es una comprobación.**

**Consecuencia para esta decisión:** no hay compatibilidad que preservar. El
precedente está roto en una representación y ausente en la otra, y ningún test
fija ese comportamiento.

## Decisión

**El comportamiento observable no depende de la representación.** En las tres
cuestiones, la operación se define **sobre el valor**: se decodifica a
complemento a dos, se opera con el código que ya existe y está probado, y se
recodifica.

| | Qué se hace | `-3 >> 1` | `~3` |
|---|---|---|---|
| `<<`, `>>` | sobre el valor | **−2** en las tres representaciones | |
| `&`, `\|`, `^`, `~` | sobre el valor | | **−4** en las tres |
| `<=>` | orden por valor | | |

Es decir: `fixed_int_t<N, signed, MS, P>` y `fixed_int_t<N, signed, TC, P>`
**dan lo mismo en todo**, y se distinguen sólo por lo que devuelven `limb()` y
`limbs()`.

### El atajo de Exceso-K en `<=>` es una optimización, no una excepción

Con el sesgo `2^(64N-1)` de ADR-017, **la representación de Exceso-K ordena igual
que el valor si se compara sin signo**. Ésa es literalmente su razón de ser: por
eso se usa en los exponentes de coma flotante.

Así que en EK, `<=>` puede compararse directo. Pero eso se escribe **como
optimización de una semántica que es el orden por valor**, no como una regla
distinta, y el test cruza las dos formas para comprobar que coinciden.

Magnitud-Signo **no** tiene esa propiedad: `-1` es `0x8…01` y `-2` es `0x8…02`,
así que por representación saldría `-1 < -2`, que es falso. Ahí se decodifica.

## Alternativas descartadas

**Operar sobre la representación** (lo que hace el tipo viejo en MS). Es más
rápido —no hay conversión— y para `<<` da lo mismo. Se descarta porque hace la
representación **observable**: `-3 >> 1` valdría −2 o −1 según un parámetro de
plantilla que el usuario eligió por cómo quiere *almacenar* el número, no por
cómo quiere que *se comporte*.

Y porque rompe la premisa de ADR-017. Si MS y EK son codificaciones, cambiar de
codificación no puede cambiar el resultado; en cuanto lo cambia, son tipos
distintos con el mismo nombre.

**No ofrecer los bitwise en MS/EK** (`static_assert`). Es defendible: son las
únicas operaciones cuyo significado depende de los bits, y un `float` tampoco los
tiene. Se descarta porque `x & 1` para saber si un número es impar es idioma
corriente, y dejaría de compilar al cambiar de representación — que es
exactamente la observabilidad que se quiere evitar, sólo que en tiempo de
compilación.

## Consecuencias

- El tramo 3 de P1.5 no añade **ninguna** decisión de comportamiento: es
  decodificar, llamar a lo que ya existe, recodificar.
- **El test tiene que comprobar valores, no propiedades estructurales.** La forma
  natural: para cada operación y cada valor, cruzar el resultado en MS y en EK
  contra el de complemento a dos. Si son la misma cosa, tienen que coincidir
  siempre — y eso es una comprobación que **sí puede fallar**.
- `PROJECT_STATUS` marcaba «Magnitud-Signo y Exceso-K en `int128_param_t`: ✅».
  Queda corregido con lo medido: EK tiene los desplazamientos rotos y MS tiene
  `~` roto.
- **No se arregla el tipo viejo.** Lo retira ADR-006, y arreglarlo sería escribir
  dos veces lo mismo. Pero queda dicho, porque ese ✅ funcionaba como argumento
  tácito de que el problema estaba resuelto.

## La regla que sale

**Un parámetro de plantilla que dice cómo se guarda algo no debe cambiar lo que
ese algo hace.** Si lo cambia, no es un parámetro de representación: es un tipo
distinto, y merece otro nombre.
