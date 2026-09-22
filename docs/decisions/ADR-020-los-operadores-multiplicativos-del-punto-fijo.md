# ADR-020: Los operadores multiplicativos del punto fijo

**Estado:** ✅ Aceptado
**Fecha:** 22 September 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

[ADR-019](ADR-019-punto-fijo-es-un-entero-con-escala.md) dejó escrito que `*` y
`/` necesitan redondeo, que el redondeo sería una **perilla**, y que el orden de
trabajo era *el tipo y las conversiones primero, el redondeo después*. La
primera entrega está hecha y verificada; toca la segunda.

Al ir a escribirla aparecen tres preguntas que ADR-019 no contestó, porque ni
siquiera las planteó: **qué es `%`**, **qué es `++`**, y **cuál de las tres
operaciones necesita redondeo de verdad**. Las tres tienen respuesta corta, y
las tres son menos trabajo de lo que parecían.

## Decisión 1 — `%` **no necesita redondeo**, y es el `%` de los crudos

La sorpresa de este ADR. Se esperaba que `%` fuera la tercera operación con
redondeo y **no lo es: es exacta**.

`%` sigue a `std::fmod`, no a nada nuevo: el resto de truncar el cociente hacia
cero, con el signo del dividendo. Con `a = A/2^k` y `b = B/2^k`:

```
a / b  =  A / B                    la escala se cancela
q      =  trunc(A / B)             entero
r      =  a − q·b                  crudo:  A − q·B  =  A % B
```

**El resto es siempre exactamente representable.** `a` y `b` son múltiplos de
`epsilon`, `q` es un entero, luego `q·b` es múltiplo de `epsilon` y `r` también.
No hay nada que descartar, así que no hay nada que redondear.

Y el resultado es literalmente `desde_crudo(A % B)`: **el módulo entero de los
crudos**, sin escalar, sin ensanchar y sin perilla. Es la misma lectura barata
que ADR-017 con las codificaciones y ADR-019 con la escala, una vez más.

> Corolario que conviene no perder: `a == (a/b)·b + a%b` **no se cumple** cuando
> `/` redondea, porque `(a/b)` ya no es `trunc(a/b)`. La identidad vale entre
> `%` y el cociente **truncado**, no entre `%` y `operator/`. Se documenta en vez
> de disimularse, igual que el suelo de `suelo()`.

## Decisión 2 — `++` suma **uno**, no un `epsilon`

`++x` es `x += 1`, y `--x` es `x -= 1`. Es lo que hacen `float` y `double` en
C++, y la instrucción del proyecto es seguir el estándar lo más de cerca
posible.

La alternativa —avanzar un `epsilon`, al siguiente valor representable— es
tentadora porque en punto fijo *existe* ese siguiente valor, cosa que en un
entero no pasa. Pero esa operación ya tiene nombre en el estándar y no es `++`:
es `std::nextafter`. Darle a `++` un significado distinto del que tiene en
`float` sería una sorpresa silenciosa en código genérico, que es justo lo que
este tipo quiere evitar.

Son **exactas**: sumar uno no cambia la escala. Y con `F == N` **no existen**,
por la misma razón que `one()`: ese tipo sólo representa `[0, 1)`. El
`static_assert` lo dice.

## Decisión 3 — El redondeo se decide con `(q, r, d)`, y nada más

Toda operación con redondeo acaba en la misma forma: el valor exacto es
`q + r/d`, con `q` entero, `0 ≤ r < d` y `d > 0`. `q` es el **suelo**, y `r`
**nunca es negativo**, que es lo que hace que la fórmula valga igual para
negativos sin un caso aparte (ADR-019).

De ahí salen los cinco modos con tres datos: si `r` es cero, cómo queda `2r`
contra `d`, y la paridad y el signo de `q`.

| Modo | `2r < d` | `2r > d` | empate `2r == d` |
|---|---|---|---|
| `to_nearest_even` **(por omisión)** | `q` | `q+1` | `q` si par, `q+1` si impar |
| `to_nearest_away` | `q` | `q+1` | `q+1` si `q ≥ 0`, `q` si no |
| `toward_zero` | — | — | `q+1` si `q < 0` y `r ≠ 0`; si no `q` |
| `toward_neg_inf` | — | — | siempre `q` |
| `toward_pos_inf` | — | — | `q+1` si `r ≠ 0` |

`toward_neg_inf` es **gratis**: es el desplazamiento aritmético tal cual.
`toward_zero` es el que ya tiene `operator/` del entero.

### De dónde salen `q`, `r` y `d` en cada operación

| | `q` | `r`, `d` |
|---|---|---|
| `*` | `mul_wide(A,B) >> k` | los `k` bits bajos del producto; `d = 2^k` |
| `/` | suelo de `(A << k) / B` | resto de esa división; `d = \|B\| ensanchado` |
| `%` | — | **no hay**: es exacto |

En `*`, `mul_wide` devuelve el producto **exacto** en `2N` limbos, así que
—como ya decía ADR-019— **todos los bits que se descartan están calculados** y
no hace falta guardar ni uno de más.

En `/` el dividendo se preescala a `2N` limbos antes de dividir, y el cociente
truncado de `divmod` se baja a suelo cuando el resto no es cero y los signos
difieren. Es el único sitio con un ajuste de signo, y es de tres líneas.

## Decisión 4 — La perilla es el **sexto** parámetro, y va al final

`fixed_point_t<N, F, Sign, Form, Policy, Redondeo>`.

Va la última porque es la que menos gente va a tocar y porque así **ningún uso
existente cambia**. Y va separada de `overflow_policy` por lo que ya razonó
ADR-019: son dos políticas distintas —el desbordamiento es *no cabe por
arriba*, el redondeo es *no cabe por abajo*— y meterlas en un enum único
multiplicaría las columnas de la matriz.

### Dónde se cruzan las dos políticas

**Redondear hacia arriba desde el máximo desborda.** Ese caso lo decide
`overflow_policy`, no `Redondeo`, y es justo el punto que hacía falta que fueran
parámetros separados para poder describir.

## Decisión 5 — `to_string` redondea, y se redondea el **valor**, no la magnitud

> **Revisada el mismo día.** Se escribió primero que `to_string` seguiría
> truncando y que el redondeo iría en una tercera entrega, por no mezclar dos
> cambios observables. Al pedirse expresamente, se hizo aquí — y resultó traer
> una decisión de diseño que merecía estar en un ADR, no en un `@warning`.

`to_string(d)` devuelve el decimal **redondeado** según `Redondeo`, y los tres
cambios de salida que eso trae están escritos abajo.

**Se redondea el valor, no la magnitud.** Es la única forma de que los modos
dirigidos salgan bien: `toward_pos_inf` sobre `−2,55` con una cifra da `−2,5`,
no `−2,6`. Redondear la magnitud y pegar el signo después daría lo segundo,
porque «hacia arriba» en la magnitud es «hacia abajo» en el valor.

Las cifras, en cambio, **sí** se escriben desde la magnitud: es lo que evita que
la coma tenga que saber de signos. Así que hay que volver al marco `(q, r, d)`:

```
positivo              q = Dmag,       r = resto
negativo, resto > 0   q = −(Dmag+1),  r = 2^k − resto
```

De donde salen dos espejos que no son evidentes y que costaron un fallo cada
uno:

- **Subir el valor es NO subir la magnitud**, y al revés.
- La paridad que mira el desempate al par es la de `q`, que en los negativos es
  la de `Dmag+1`: **la contraria** de la última cifra escrita.

### Lo que cambia en la salida

| | Antes | Ahora | Por qué |
|---|---|---|---|
| `max().to_string(2)` | `…807.99` | `…808.00` | el valor es `…807,99999999999999999995`; a dos cifras, sube, y el acarreo llega a la parte entera |
| `to_string(3)` con `F == 0` | `7` | `7.000` | `decimales` significa lo que dice, como `printf("%.3f", 7.0)` |
| un negativo que redondea a cero | — | `-0.00` | igual que `printf`. El tipo no tiene cero negativo; la cadena dice «un negativo pequeño», que es información |

El acarreo **puede alargar la cadena** —`9,999…` con dos cifras es `10.00`— y
por eso la coma se inserta al final y contando desde la derecha, nunca
reservando hueco por delante.

## Decisión 6 — `<<` y `>>` escalan el valor; no hay bitwise

`x << n` es `x · 2^n` y es **exacto**. `x >> n` es `x / 2^n` y **redondea**: al
bajar `n` posiciones se caen `n` bits, y que se caigan es exactamente lo que la
perilla decide. Con `toward_neg_inf` sale el desplazamiento aritmético tal cual,
que es gratis. Es lo que hacen los tipos de coma fija del TR 18037.

**No hay `&`, `|`, `^` ni `~`.** El entero los tiene —y por ADR-018 operan sobre
el valor—, pero en un punto fijo no significan nada útil, y ni `float` ni los
tipos del TR 18037 los ofrecen. Añadirlos sería inventar semántica en vez de
seguir el estándar. La matriz vigila que **sigan sin compilar**, para que nadie
los habilite sin querer al tocar una conversión.

## Consecuencias

- Las tres operaciones nuevas **no traen ni un algoritmo aritmético**: `*` es
  `mul_wide` más un desplazamiento, `/` es un preescalado más `divmod`, y `%`
  es `divmod` a secas. Cuarta vez que la lectura barata es la correcta.
- El test tiene que cruzar **contra un oráculo**, no comprobar propiedades: con
  redondeo, la asociatividad y la distributividad **no se cumplen**. El oráculo
  natural sigue siendo `__int128` escalado, y para el redondeo, aritmética
  exacta sobre el producto de `2N` limbos.
- **Cada modo hay que cruzarlo por separado.** Un test que sólo ejercite el modo
  por omisión pasaría igual con los otros cuatro mal escritos, que es la forma
  exacta de test vacuo que apareció el 22 sep con las representaciones.
- **Dos** de las cinco sondas del punto fijo cambian de signo —`*` y `/` pasan a
  tener que compilar— y eso **es lo que tenía que pasar**: esa vigilancia estaba
  puesta para ponerse roja hoy. Las otras tres siguen: la de control, `one()`
  con `F == N` y `F > N`. Y entran cuatro positivas nuevas —`%`, los compuestos,
  `++`/`--` y **los cinco modos instanciados**— más una negativa, `++` con
  `F == N`, que por la decisión 2 tampoco tiene a qué sumar. Con los
  desplazamientos y las dos negativas de bitwise, **289 → 297 celdas**.

## La regla que sale

**Antes de dar por hecho que una operación necesita redondeo, mirar si su
resultado es representable.** `%` parecía la tercera y no lo era: el resto de
dos múltiplos de `epsilon` es múltiplo de `epsilon`. Es la misma pregunta que
ADR-017 hizo con las capas y ADR-019 con las escalas, aplicada al redondeo:
mirar qué se pierde de verdad antes de escribir la maquinaria para no perderlo.
