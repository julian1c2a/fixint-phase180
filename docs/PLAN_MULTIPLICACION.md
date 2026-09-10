# Plan de `operator*` — bajar el exponente, no sólo el factor

**Fecha:** 10 September 2026 · **Tarea:** P2.8

> Diseño escrito del reparto de `operator*` para toda anchura N, y de la
> ampliación de Karatsuba. Sale del acantilado medido en
> [PERFORMANCE](PERFORMANCE.md): toda N > 20 que no sea potencia de dos cae al
> bucle escolar.

---

## Dónde estamos, medido

| N | camino de hoy | cyc/op | por limbo |
|---:|---|---:|---:|
| 20 | escolar desenrollado | 679 | 34,0 |
| **24** | **escolar en bucle** | **3035** | **126,4** |
| 32 | Karatsuba | 3273 | 102,3 |
| **48** | **escolar en bucle** | **13909** | **289,8** |
| 64 | Karatsuba | 16763 | 261,9 |

GCC 16.2 −O2, 10 sep 2026. El acantilado es real: N=24 cuesta casi lo mismo que
N=32, que es un tercio más grande.

**El desenrollado hasta 31 está medido y decidido.** Compilando el mismo
benchmark con `NSTD_DESENROLLA_MAX=32`:

| N=24 | cyc/op | por limbo |
|---|---:|---:|
| bucle | 3035 | 126,4 |
| desenrollado | **1245** | **51,9** |

**2,44×**, y cuesta 3 s más de compilación y 68 KB más de binario en esa unidad.

> ⚠️ **Este 2,44× NO está confirmado, y contradice a otra medida.** El barrido
> del 6 sep 2026 —anotado en el bloque `@def` de `NSTD_DESENROLLA_MAX`, dentro
> de `fixed_width_int_t.hpp`— da **1,11×** para el mismo N=24. Difieren 2,2×,
> muy por encima del ruido del banco.
>
> El 10 sep se afirmó aquí que el 1,11× «estaba mal atribuido». **Era falso**:
> es una medida real de N=24 y está fechada en el header. El error fue no abrir
> ese bloque antes de negar que la cifra existiera.
>
> **Consecuencia para este plan: el paso 1 deja de estar decidido.** Sube a la
> sesión de medición, que empieza por reproducir las dos.

> **Aviso de método.** Entre las dos compilaciones, filas que no deberían cambiar
> varían hasta un 35 % (`add` N=16: 48,1 frente a 64,5). El 2,44× está muy fuera
> de ese ruido; una diferencia del 10 % no lo estaría. Ver
> [PLAN_BENCHMARK_AND_TESTING_METHODOLOGY](PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md).

---

## El objetivo: bajar el exponente

Karatsuba es Θ(N^1,585); el escolar es Θ(N²). Lo que se busca es que **toda** N
tenga el exponente bajo, no sólo las potencias de dos.

Esto importa para elegir bien, porque hay dos formas de repartir una N que no es
potencia de dos, y **sólo una baja el exponente**.

### Reparto A — potencia de dos más el resto

`N = 2ⁿ + r`, con 2ⁿ el mayor por debajo de N. Entonces

```
a·b = a₁b₁·B^(2·2ⁿ) + (a₁b₀ + a₀b₁)·B^(2ⁿ) + a₀b₀
```

con `a₀`, `b₀` de 2ⁿ limbos y `a₁`, `b₁` de r. Karatsuba para `a₀b₀`, escolar
para `a₁b₁`, y los dos cruzados por escolar.

### Reparto B — mitades equilibradas

`N = ⌈N/2⌉ + ⌊N/2⌋`, y Karatsuba normal sobre esas dos mitades, recursivamente.
Vale para **cualquier** N, no sólo potencias de dos.

### La cuenta

Productos de limbo 1×1 —la unidad honesta para comparar algoritmos, que **no es
tiempo**: no cuenta sumas ni recursión—:

| N | escolar | reparto A | reparto B | A sobre escolar | B sobre escolar |
|---:|---:|---:|---:|---:|---:|
| 20 | 400 | 225 | 243 | 1,78× | 1,65× |
| 24 | 576 | 401 | **243** | 1,44× | **2,37×** |
| 31 | 961 | 786 | **324** | 1,22× | **2,97×** |
| 48 | 2304 | 1523 | **729** | 1,51× | **3,16×** |
| 63 | 3969 | 3188 | **972** | 1,24× | **4,08×** |
| 96 | 9216 | 5849 | **2187** | 1,58× | **4,21×** |
| 127 | 16129 | 12762 | **2916** | 1,26× | **5,53×** |

### Por qué A no baja el exponente

El término que manda es el de los dos productos cruzados, `2·r·2ⁿ`. Cuando `r`
se acerca a `2ⁿ` —o sea en N=31, 63, 127, que es **justo donde el acantilado es
peor**— eso vale `2·(N/2)·(N/2) = N²/2`. Sigue siendo cuadrático.

Comprobado numéricamente: el coste del reparto A dividido por N² tiende a una
constante.

| N | A / N² | B / N^1,585 |
|---:|---:|---:|
| 24 | 0,696 | 1,5775 |
| 96 | 0,635 | 1,5774 |
| 384 | 0,600 | 1,5773 |
| 768 | 0,589 | 1,5773 |

**El reparto A es Θ(N²) con un factor constante que nunca pasa de ~2×.** El
reparto B es Θ(N^1,585) de verdad, y su ventaja **crece** con N: 2,4× en N=24,
5,5× en N=127, y sigue subiendo.

### Y además A cuesta más código

El reparto A necesita una pieza que **hoy no existe**: un producto
**rectangular** `r × 2ⁿ`. Todo lo que hay hoy —el desenrollado y `kmul_full`—
es cuadrado, N×N. Los dos cruzados son el 64 % del trabajo en N=24, así que no
es un detalle: es el grueso.

El reparto B no necesita nada nuevo salvo tratar mitades de tamaño distinto en
uno y el acarreo de `(a₀+a₁)`, que puede desbordar un limbo. Es **un camino en
vez de tres**.

---

## El reparto que se propone

```
N ≤ 2            → especializado a mano (ya existe)
N ≤ 31           → escolar DESENROLLADO por recursión de plantilla   [medido 2,44× en N=24]
32 ≤ N           → Karatsuba con reparto EQUILIBRADO, para toda N
                   (caso base de la recursión: el desenrollado de arriba)
```

Con esto desaparece el caso «potencia de dos o nada»: `NSTD_KARATSUBA_MIN` sigue
siendo el umbral, pero deja de exigir que N sea potencia de dos.

Las macros que quedan:

| Macro | Hoy | Propuesto |
|---|---:|---|
| `NSTD_DESENROLLA_MAX` | 20 | **31** |
| `NSTD_KARATSUBA_MIN` | 32 | 32, pero **sin exigir potencia de dos** |
| `NSTD_KARATSUBA_MAX` | 4096 | igual |

---

## Toom-Cook, la ampliación que baja más el exponente

Toom-3 parte cada operando en **tres** y hace 5 productos en vez de 9:
Θ(N^log₃5) = **Θ(N^1,465)**, frente al 1,585 de Karatsuba.

Lo que hay que saber antes de meterlo:

- **Necesita divisiones exactas por 2 y por 3** en la interpolación. Sobre
  enteros son exactas, pero la división por 3 no es un desplazamiento: hay que
  escribirla (multiplicación por el inverso modular de 3, que para 64 bits es
  `0xAAAAAAAAAAAAAAAB`).
- **El umbral está alto, pero no tanto como se dijo aquí.** Esta línea decía
  «en las bibliotecas de referencia suele caer en el rango de los cientos de
  limbos», y **es falso**: la mediana que GMP mide y publica para
  `MUL_TOOM33_THRESHOLD` es **67 limbos**, con un rango de 38 a 122. Decenas,
  no cientos. Ver [ESTUDIO_ALGORITMOS_RAPIDOS](ESTUDIO_ALGORITMOS_RAPIDOS.md).
- **Pero la cuenta de productos dice que aporta menos de lo que parece.**
  Proyectado sobre el rango de esta biblioteca: **nada por debajo de N=128**,
  y en N=128 **pierde** contra Karatsuba (7 260 productos frente a 6 912),
  porque `⌈N/3⌉` redondea hacia arriba. Gana 1,23× en N=256 y 1,77× en
  N=2048. Además su exponente efectivo **oscila** entre 1,2 y 1,66 según
  dónde caiga el redondeo, mientras que el de Karatsuba es 1,585 constante.
- **Cinco puntos de evaluación** significan cinco caminos donde equivocarse.
  Necesita el mismo trato que llevó Karatsuba: test diferencial contra el
  escolar sobre operandos al azar, y en todas las N del rango, no en una.

Por eso va **después** de que el reparto equilibrado esté medido y en verde: si
el equilibrado no da lo que promete la cuenta, Toom-3 tampoco lo dará.

---

## Orden de trabajo

| # | Paso | Depende de | Cómo se sabe que está bien |
|---|---|---|---|
| 1 | `NSTD_DESENROLLA_MAX` de 20 a 31 | — | Ya medido: 2,44× en N=24. Vigilar tiempo de compilación de la suite entera, no sólo de un fichero |
| 2 | Karatsuba con reparto equilibrado, para toda N | 1 | Test diferencial contra el escolar en **toda** N de 32 a 64, no sólo potencias de dos. El acantilado tiene que desaparecer de la curva |
| 3 | Re-medir la curva completa y actualizar `PERFORMANCE.md` | 2 | `benchmark_curva_n` sin picos: la columna «por limbo» de `mul` debe crecer suavemente |
| 4 | Medir el umbral real de Karatsuba con el reparto nuevo | 3 | Puede que baje de 32: el equilibrado no tiene el salto de las potencias de dos |
| 5 | Toom-3 | 4 | Umbral **medido**. Si no gana por debajo de N=4096 (`NSTD_KARATSUBA_MAX`), se documenta y no se activa |

Los pasos 1 y 2 cierran P2.8. Del 3 al 5 son trabajo nuevo que sale de él.

---

## Lo que este plan **no** hace

- **No toca la multiplicación de N pequeño.** N ≤ 2 tiene su camino
  especializado y no entra aquí.
- **No promete el 2,4× de la tabla en tiempo.** La tabla cuenta productos de
  limbo. La realidad medida es que Karatsuba no gana hasta N=32 pese a que la
  cuenta de productos le favorece desde mucho antes: las sumas y la recursión
  mandan en anchuras pequeñas. La cuenta dice **qué exponente** se consigue, no
  cuántos ciclos.
- **No decide si Toom-3 entra.** Eso lo decide el umbral medido en el paso 5.
