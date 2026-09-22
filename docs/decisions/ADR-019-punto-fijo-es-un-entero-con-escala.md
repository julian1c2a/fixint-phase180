# ADR-019: Un punto fijo es un entero con una escala

**Estado:** ✅ Aceptado
**Fecha:** 22 September 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

P1.6 es la última etapa del camino crítico de la 2.0, y hasta hoy era **una línea
en `NEXT_STEPS`**: «Etapa 5: punto fijo». No tenía diseño.

Se escribe ahora, antes de una sola línea de código, por lo que enseñó el tramo 3
de P1.5: allí tres decisiones bien planteadas
([ADR-018](ADR-018-la-representacion-no-es-observable.md)) convirtieron lo que el
inventario describía como «41 puntos de decisión» en trabajo mecánico, y lo que
parecía una aritmética nueva resultó ser **dos funciones**.

## Decisión 1 — Es un entero con una escala, y no lleva aritmética propia

`fixed_point_t` **guarda un `fixed_int_t<N, Sign, Form, Policy>`** y sabe que ese
entero vale `x / 2^(64·F)`. De ahí sale todo:

| Operación | Qué es |
|---|---|
| `+`, `−` | las del entero, **sin tocar nada**: misma escala |
| comparación, orden | las del entero, **sin tocar nada**: la escala es común |
| `*` | la del entero **más descartar `F` limbos** — ahí entra el redondeo |
| `/` | preescalar el dividendo por `2^(64·F)` y dividir — ahí entra otra vez |

**No hay ni un algoritmo aritmético que escribir.** Knuth D, Karatsuba,
Möller–Granlund, Toom-3, las cuatro representaciones y la política de
desbordamiento vienen ya escritos y probados.

Es el mismo hallazgo que ADR-017 con Magnitud-Signo y Exceso-K, y por el mismo
motivo: **antes de escribir una capa, preguntarse si es una capa o una
codificación**. Una escala es aún menos que una codificación.

## Decisión 2 — Los parámetros son `N` y `F`, en limbos

`fixed_point_t<N, F, Sign, Form, Policy, Redondeo>`, donde `N` son los limbos
totales y `F` los fraccionarios. La parte entera es `E = N − F` y **se deduce**.

Se discutió parametrizar por `E` y `F` en **bits**. Eso trae un problema que con
limbos no existe: si `E+F` no es múltiplo de 64, sobran bits en el limbo alto, y
entonces **cada operación** tiene que enmascararlos y leer el signo del bit
`E+F−1` en vez del 63. Es coste por operación y superficie de fallo nueva, y el
tramo 3 acaba de mostrar lo que cuesta cada sitio que asume que «los limbos son
el valor».

Con `N` y `F` en limbos, el almacenamiento **es exactamente** un
`fixed_int_t<N>`: sin relleno, sin nada que enmascarar nunca.

## Decisión 3 — Las cuatro representaciones, y el signo NO se añade

Van las cuatro que admite el entero: `binnat` sin signo, y complemento a dos,
Magnitud-Signo y Exceso-K con signo.

**El bit de signo es el más significativo de la parte entera, no un bit extra.**
Se consideró «E+F **más** un bit de signo» para Magnitud-Signo, y se descarta:
haría que MS ocupara un bit más que complemento a dos con los mismos `N` y `F`,
y con ello dos tipos del mismo rango declarado necesitarían almacenamiento
distinto. En el tipo entero el signo sale de la magnitud y MS y C2 ocupan lo
mismo; aquí se mantiene esa analogía.

> **Esto contradice un documento del proyecto, y el documento es el que cambia.**
> `AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md` §71 dice: *«Se
> implementarán solo 2 modalidades: signed y unsigned, siendo la signed en
> complemento a 2 (únicamente)»*. Se escribió cuando MS y EK sólo existían en
> `int128_param_t` y su destino previsto era la coma flotante. Desde el 22 sep
> están en `fixed_int_t`, comprobadas con 16 624 comprobaciones cruzadas, y
> dejarlas fuera del punto fijo sería una asimetría sin motivo.

## Decisión 4 — El redondeo es una perilla, y NO va dentro de `overflow_policy`

`Redondeo` es un **parámetro de plantilla propio**, al estilo del `Estimador` de
la división.

**Por qué no meterlo en `overflow_policy`.** Por mecánica: `overflow_policy`
tiene cuatro valores y el redondeo tendrá cuatro o cinco modos; un enum único
serían dieciséis o veinte combinaciones, y la
[matriz de paridad](../MATRIZ_DE_PARIDAD.md) tendría que cubrirlas. Hoy son 6
columnas: pasarían a 24. Y conceptualmente son cosas distintas — el
desbordamiento es *no cabe por arriba*, el redondeo es *no cabe por abajo*.

**Por qué una perilla y no un enum más de la matriz.** Es el patrón de
`Estimador`, que **no está en la matriz** porque es una pieza interna
intercambiable y no una combinación que haya que verificar entera. La matriz
cubre el modo por omisión; los demás se comparan con un banco, como se comparó
Knuth contra Möller–Granlund.

### El modo por omisión: al más cercano, desempate al par

Los tres candidatos y lo que se pierde con cada uno:

| | A favor | En contra |
|---|---|---|
| truncar hacia cero | coherente con `operator/` del entero; el más barato | **sesga** siempre hacia cero |
| truncar hacia −∞ | coherente con `operator>>` | sesga hacia un lado |
| **al más cercano, desempate al par** | **no sesga**; es el defecto de IEEE-754 | más caro: hace falta el *sticky* |

Se elige el tercero. El argumento que decide no es el rendimiento sino el
**sesgo**: lo que la gente hace con punto fijo es acumular, y un redondeo sesgado
deriva en la dirección del sesgo tanto más cuanto más larga es la cadena. Truncar
queda disponible como perilla para quien quiera el camino barato.

## Decisión 5 — No hacen falta bits de guarda

Se planteó guardar bits de más para poder redondear. **No hacen falta**, y la
razón es concreta: `mul_wide` devuelve el producto **exacto** de `2N` limbos, así
que **todos los bits que se van a descartar están ya calculados**.

Los bits de guarda vienen de la coma flotante, donde el producto se trunca *al
calcularlo*. Aquí no: para redondear correctamente basta mirar los `F` limbos
bajos del producto, de donde salen las dos señales que hacen falta:

- el bit más alto de lo descartado → ***round***
- si el resto es distinto de cero → ***sticky***

Con esas dos se implementa cualquier modo, el desempate al par incluido, **sin
almacenar un bit de más**.

> Lo que sí sería otro problema es la acumulación de error en cadenas largas, y
> ahí unos bits extra ayudarían. Pero eso no es «redondear bien», es **precisión
> extendida**, es una decisión distinta, y probablemente se resuelve mejor dando
> un tipo más ancho que escondiendo bits en éste.

### Cómo queda el redondeo, en concreto

Tras desplazar aritméticamente, el valor exacto es `q + r/2^k` con `0 ≤ r < 2^k`:

```
r > 2^(k-1)                    ->  q + 1
r < 2^(k-1)                    ->  q
r = 2^(k-1)  (empate exacto)   ->  q si q es par, q + 1 si es impar
```

Vale igual para `q` negativo, porque el desplazamiento aritmético da el suelo y
`r` nunca es negativo. Es la misma fórmula para las cuatro representaciones,
porque por [ADR-018](ADR-018-la-representacion-no-es-observable.md) se opera en
complemento a dos.

### Dónde se aplica, y dónde no

**Se aplica** en `*`, en `/`, al convertir entre tipos con distinto `F`, al
convertir desde coma flotante y en `to_string` con un número fijo de decimales.

**No se aplica** en `+`, `−`, la comparación ni el orden: son exactos.

**Y se cruza con el desbordamiento**: redondear hacia arriba desde el máximo
representable desborda. Ese caso lo decide `overflow_policy`, no `Redondeo` — son
dos políticas que se encuentran en un punto, que es justo por qué conviene que
sean parámetros separados.

## Consecuencias

- El orden de trabajo es **el tipo y las conversiones primero, el redondeo
  después**: con la perilla puesta, los modos se pueden comparar midiendo en vez
  de elegirlos por analogía.
- El test tiene que seguir la forma de ADR-018: **cruzar contra un oráculo**. Aquí
  el oráculo natural es la aritmética exacta en racionales, o el mismo cálculo
  con `F` mayor. Comprobar propiedades (asociatividad, distributividad) **no
  vale**: con redondeo no se cumplen, y las que sí se cumplen lo harían también
  con el redondeo mal puesto.
- La matriz de paridad crecerá al abrir las columnas del tipo nuevo, no por el
  redondeo.

## Lo que queda abierto

No se decide aquí, y se anota para no darlo por resuelto:

| Cuestión | Por qué no se cierra hoy |
|---|---|
| Operaciones entre tipos con **distinto `F`** | ¿se promociona al mayor, se prohíbe, o se pide conversión explícita? Afecta a la ergonomía más que a la corrección |
| Conversión **desde y hacia coma flotante** | tiene su propio redondeo y sus casos raros (infinitos, NaN), y el tipo entero ya tuvo un fallo ahí (T2.2) |
| `sqrt` y las demás funciones numéricas | `sqrt` en punto fijo no es el del entero: la escala entra en la raíz |
| El **nombre** del tipo y sus alias | trivial, pero mejor decidirlo antes de escribir cien usos |

## La regla que sale

**Antes de escribir una capa, mirar si es una capa, una codificación o una
escala.** Las tres se parecen desde fuera y cuestan muy distinto: una capa se
paga en cada operación, una codificación son dos funciones, y una escala es un
desplazamiento. Este proyecto se ha encontrado las tres, y en los tres casos la
lectura barata era la correcta.
