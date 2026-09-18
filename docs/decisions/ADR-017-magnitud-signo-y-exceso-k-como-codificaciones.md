# ADR-017: Magnitud-Signo y Exceso-K son codificaciones, no aritméticas

**Estado:** ✅ Aceptado
**Fecha:** 18 September 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

[ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md) deja como último tramo
de P1.5 portar **Magnitud-Signo** y **Exceso-K** a `fixed_int_t`, y lo describe
como *«el de más peso: hay que generalizar el `static_assert` de la clase y
revisar cada operación que hoy asume complemento a dos»*.

Contadas el 18 sep 2026, esas operaciones son **41 puntos** en
`fixed_width_int_t.hpp`. Escribir aritmética nativa para cada representación
significaría triplicar ese número y triplicar la superficie de fallo.

## Decisión 1 — Son codificaciones: se decodifica, se opera, se recodifica

**No se escribe aritmética nueva.** Magnitud-Signo y Exceso-K son formas de
*escribir* un entero con signo, no formas distintas de sumarlos. Toda operación
sobre un `fixed_int_t<N, signed, MS|EK, Policy>`:

1. decodifica los operandos a complemento a dos,
2. opera con el código que ya existe y está probado,
3. recodifica el resultado.

Es lo que ya hace `int128_param_t` —de sus 115 menciones a MS y EK, casi todas
son conversiones— y es lo que permite que el tramo 3 **no añada ni un algoritmo
aritmético**.

El coste es dos conversiones por operación. Con Exceso-K una conversión es
*invertir un bit*; con Magnitud-Signo, una negación en el peor caso. Frente al
coste de una multiplicación o una división de N limbos, es ruido — y si algún día
deja de serlo, se mide antes de optimizar.

## Decisión 2 — El sesgo de Exceso-K es `2^(64N-1)`

`representation_traits<excess_k>::default_bias_high` vale `1ULL << 62`, o sea
**2¹²⁶** para 128 bits. Comprobado:

| sesgo | rango representable | ¿simétrico? | ¿cubre `int128`? |
|---|---|---|---|
| 2¹²⁶ | `[-2¹²⁶, 3·2¹²⁶−1]` | no | **no llega a −2¹²⁷** |
| 2¹²⁷ | `[-2¹²⁷, 2¹²⁷−1]` | sí | **exactamente** |

`fixed_int_t` usa **`2^(64N-1)`**, el único que hace de Exceso-K una biyección
con el rango con signo de la misma anchura.

El 2¹²⁶ de `int128_param_t` **no se toca**: ese tipo lo retira ADR-006, y
cambiárselo ahora rompería su propia paridad sin ganar nada.

Con el sesgo canónico, Exceso-K es *complemento a dos con el bit alto invertido*
—la identidad conocida con «offset binary»—, así que la ida y la vuelta son la
misma función.

## Decisión 3 — El mínimo no existe en Magnitud-Signo, y se satura

`-2^(64N-1)` no tiene representación en MS: su magnitud es `2^(64N-1)` y pisaría
el bit de signo. Es la asimetría clásica.

`c2_a_ms` **satura** a `-(2^(64N-1) − 1)`. La alternativa —envolver, o dejarlo
indefinido— daría un valor que no es ni el pedido ni uno señalado.

**Esto pierde información**, y quien convierta de ida y vuelta por complemento a
dos tiene que saberlo. Está en el `@warning` de la función y es un caso propio
del test, incluida la comprobación de que al volver **no** da el mínimo.

## Decisión 4 — Los dos ceros de MS se comportan como uno

MS tiene `+0` y `−0`; complemento a dos tiene un cero. Al decodificar, los dos
van al mismo valor.

Consecuencia directa: **`a == b` compara valores, no patrones de bits**, así que
`+0 == −0` es `true`. Cualquier otra cosa haría que el tipo no fuera un entero.

Lo que **no** se hace es normalizar `−0` a `+0` al construir: el patrón que el
usuario escribe se conserva, porque el tipo expone `limb()` y `limbs()` y ahí lo
honesto es devolver lo que hay.

## Decisión 5 — Qué queda **abierto**, y por qué no se decide aquí

Estas necesitan medirse o discutirse antes, y adivinarlas sería peor que
dejarlas escritas como pendientes:

| Cuestión | Por qué no se cierra hoy |
|---|---|
| `<<` y `>>` en MS/EK | En complemento a dos son multiplicar y dividir por potencias de dos. En MS el desplazamiento de la *magnitud* hace eso; el de la *representación* no. Hay que elegir, y la elección cambia qué significa el tipo |
| `&`, `|`, `^`, `~` en MS/EK | Operan sobre bits. Sobre la representación son una cosa y sobre el valor decodificado otra, y ninguna es obviamente «la buena» |
| El orden de `<=>` | Decodificando sale el orden natural. Sobre la representación, EK ordena igual que sin signo (ésa es su gracia) y MS no |

## Consecuencias

- La pieza fundacional está escrita y verificada: `nstd::repr` en
  `representation.hpp`, con las conversiones para cualquier `N` y **2 894
  comprobaciones**.
- Lo que queda del tramo es **conectar**, no inventar: relajar el
  `static_assert` y meter la decodificación en los 41 puntos.
- `scripts/check_matriz_paridad.py` vigila ahora `int/magnitude_sign` e
  `int/excess_k` en la lista de reservadas, igual que a `saturate` y `trap`.
  **Eso era un hueco**: el día que se relajara el `static_assert` sin abrir sus
  columnas, la matriz habría seguido diciendo «todas como declaran» mientras dos
  representaciones enteras quedaban sin comprobar en ninguna capacidad. Ahora esa
  comprobación **falla en cuanto se implementen**, y obliga a abrirlas.

## La regla que sale

**Antes de escribir una capa nueva, preguntarse si es una capa o una
codificación.** Una codificación se resuelve con dos funciones y no toca el
resto; una capa se paga en cada operación para siempre. Aquí la diferencia entre
las dos lecturas eran 41 puntos de decisión contra 2.
