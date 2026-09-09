# Estudio: multiplicación, división y resto rápidos de enteros grandes

**Fecha:** 10 September 2026 · **Ámbito:** estado del arte y proyección sobre
`fixed_int_t<N>` con N limbos de 64 bits, N ≤ 4096.

> Recopilación de lo que hacen las bibliotecas de referencia y la literatura, con
> **los umbrales que ellas miden**, y la proyección teórica de lo que cada
> técnica daría en el rango de N de esta biblioteca.

---

## 0. La corrección que obliga a hacer este estudio

En [PLAN_MULTIPLICACION](PLAN_MULTIPLICACION.md) se escribió que el umbral de
Toom-3 «en las bibliotecas de referencia suele caer en el rango de los cientos
de limbos». **Es falso.** Los umbrales que GMP mide y publica por CPU:

| Umbral | Qué activa | Mediana | Rango medido |
|---|---|---:|---|
| `MUL_TOOM22_THRESHOLD` | Karatsuba | **23 limbos** | 16 – 46 |
| `MUL_TOOM33_THRESHOLD` | Toom-3 | **67 limbos** | 38 – 122 |
| `MUL_FFT_THRESHOLD` | Schönhage-Strassen | **4224 limbos** | 2752 – 11136 |

Toom-3 entra en **decenas** de limbos, no cientos. Y hay algo más importante:
**`NSTD_KARATSUBA_MAX` vale 4096, que es prácticamente la mediana del umbral de
FFT**. Es decir, el rango de esta biblioteca no acaba antes de la FFT: acaba
justo donde la FFT empieza a ganar.

Fuentes: [MUL_TOOM22_THRESHOLD](https://gmplib.org/devel/thres/MUL_TOOM22_THRESHOLD),
[MUL_TOOM33_THRESHOLD](https://gmplib.org/devel/thres/MUL_TOOM33_THRESHOLD),
[MUL_FFT_THRESHOLD](https://gmplib.org/devel/thres/MUL_FFT_THRESHOLD).

---

## 1. La escalera de la multiplicación

| Algoritmo | Coste | Exponente | Entra en |
|---|---|---:|---|
| Escolar | Θ(N²) | 2,000 | siempre |
| Karatsuba (Toom-2) | Θ(N^log₂3) | **1,585** | ~23 limbos |
| Toom-3 | Θ(N^log₃5) | **1,465** | ~67 limbos |
| Toom-4 | Θ(N^log₄7) | **1,404** | ~150–300 limbos |
| Toom-6½, Toom-8½ | → 1,3… | ~1,3 | antes de la FFT |
| Schönhage-Strassen | Θ(N log N log log N) | ~1 | ~4200 limbos |
| Harvey–van der Hoeven | Θ(N log N) | 1 | **inalcanzable** |

**Karatsuba** hace 3 productos de tamaño N/2 en vez de 4. **Toom-3** hace 5 de
tamaño N/3 en vez de 9; el precio es la evaluación en 5 puntos y la
interpolación, que incluye **divisiones exactas por 2 y por 3** — la de 3 no es
un desplazamiento, hay que multiplicar por el inverso modular
(`0xAAAAAAAAAAAAAAAB` en 64 bits).

**Sobre Harvey–van der Hoeven** (Annals of Mathematics, 2021), que confirmó la
conjetura de Schönhage y Strassen de 1971: es un resultado teórico y **nada
más**. Su ventaja aparece por encima de ~10^(8,5·10¹⁰) dígitos. No es que sea
poco práctico: es que ese número no cabe en el universo observable. Conviene
tenerlo claro porque aparece citado como si fuera aplicable.

Fuentes: [GMP: Karatsuba](https://gmplib.org/manual/Karatsuba-Multiplication),
[GMP: Toom 3-Way](https://gmplib.org/manual/Toom-3_002dWay-Multiplication),
[GMP: FFT](https://gmplib.org/manual/FFT-Multiplication),
[Harvey & van der Hoeven, *Integer multiplication in time O(n log n)*](https://projecteuclid.org/journals/annals-of-mathematics/volume-193/issue-2/Integer-multiplication-in-time-Onmathrmlog-n/10.4007/annals.2021.193.2.4.short),
[Lemire, *Better computational complexity does not imply better speed*](https://lemire.me/blog/2019/11/26/better-computational-complexity-does-not-imply-better-speed/).

---

## 2. La escalera de la división y el resto

Es la parte donde esta biblioteca tiene más recorrido, porque hoy sólo tiene el
primer escalón.

| Algoritmo | Coste | Qué es |
|---|---|---|
| Knuth D (escolar) | Θ(N·M) | Lo que hay hoy |
| Möller–Granlund 3-por-2 | mejora la **constante** | Recíproco precalculado; sustituye la división por multiplicaciones |
| Burnikel–Ziegler | Θ(M(N)·log N) | Divide y vencerás; convierte la división en multiplicaciones y las pasa a Karatsuba/Toom |
| Newton / Barrett | **Θ(M(N))** | Recíproco por Newton-Raphson; la división pasa a costar un múltiplo constante de una multiplicación |

**El techo de esta escalera es concreto y está publicado:** en GMP, *«a 2N×N
division is about 2 to 4 times slower than an N×N multiplication»* a tamaños
medianos y grandes. Es decir, **la división puede llegar a costar 2–4× lo que
una multiplicación**, sea cual sea N. Hoy, con Knuth D, esa razón crece con N.

- **Möller–Granlund** (IEEE Trans. Computers 60(2), 2011) es el más barato de
  adoptar y el que más rinde por línea escrita: divide dos palabras entre una
  con **una `umul` y una `umullo`**, usando una aproximación precalculada del
  recíproco. Es la base del `divrem` interno de GMP y del `div_qr_1`. No cambia
  el exponente: mejora la constante del bucle interno de Knuth D.
- **Burnikel–Ziegler** (1998) baja a Θ(M(N)·log N). El umbral publicado es
  **~860 bits** (≈14 limbos) en su artículo; GMP sitúa `DC_DIV_QR_THRESHOLD`
  «algo por encima del doble de `MUL_TOOM22_THRESHOLD`», o sea ~45–50 limbos.
  Dentro del rango de Toom-3, GMP cifra su coste en **≈2,63·M(N)**.
- **Newton/Barrett** llega a Θ(M(N)) y es lo que GMP usa por encima de
  `MU_DIV_QR_THRESHOLD`.

**Para el resto solo**, y sobre todo para el resto **repetido con el mismo
módulo**, hay dos técnicas que no son división:

- **Barrett**: precalcula ⌊B^{2k}/d⌋ una vez y luego cada reducción son dos
  multiplicaciones. Encaja cuando se quiere cociente **y** resto.
- **Montgomery**: cambia de dominio y hace la reducción sin divisiones. Paga la
  conversión de entrada y salida, así que sólo compensa con muchas operaciones
  seguidas en el mismo módulo — el caso de la criptografía.

Fuentes: [GMP: Divide and Conquer Division](https://gmplib.org/manual/Divide-and-Conquer-Division),
[Möller & Granlund, *Improved Division by Invariant Integers*](https://www.lysator.liu.se/~nisse/archive/draft-division-paper.pdf),
[Granlund & Montgomery, *Division by Invariant Integers using Multiplication*](https://gmplib.org/~tege/divcnst-pldi94.pdf),
[Burnikel & Ziegler, *Fast Recursive Division*](https://www.semanticscholar.org/paper/Fast-recursive-division-Burnikel-Ziegler/c9d46de024099fb3fbd44030ddfba1561738939e),
[Barrett reduction](https://en.wikipedia.org/wiki/Barrett_reduction).

---

## 3. Proyección teórica sobre el rango de esta biblioteca

Contado en **productos de limbo 1×1** (`mulx`), con recursión hasta un caso base
y los umbrales medianos de GMP (23 / 67 / 300). **No es tiempo**: no cuenta
sumas, acarreos ni recursión. Dice qué **exponente** se consigue.

| N | escolar | Karatsuba | Toom-3 | Toom-4 | K/esc | T3/esc | T4/esc |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 | 64 | 64 | 64 | 64 | 1,0× | 1,0× | 1,0× |
| 16 | 256 | 256 | 256 | 256 | 1,0× | 1,0× | 1,0× |
| 32 | 1 024 | 768 | 768 | 768 | 1,3× | 1,3× | 1,3× |
| 64 | 4 096 | 2 304 | 2 304 | 2 304 | 1,8× | 1,8× | 1,8× |
| 128 | 16 384 | 6 912 | 7 260 | 7 260 | 2,4× | 2,3× | 2,3× |
| 256 | 65 536 | 20 736 | 16 875 | 16 875 | 3,2× | **3,9×** | 3,9× |
| 512 | 262 144 | 62 208 | 50 625 | 50 820 | 4,2× | **5,2×** | 5,2× |
| 1 024 | 1 048 576 | 186 624 | 135 375 | 118 125 | 5,6× | 7,7× | **8,9×** |
| 2 048 | 4 194 304 | 559 872 | 316 875 | 355 740 | 7,5× | **13,2×** | 11,8× |
| 4 096 | 16 777 216 | 1 679 616 | 950 625 | 826 875 | 10,0× | 17,6× | **20,3×** |

### Lo que dice esta tabla

**1. Toom-3 no aporta nada por debajo de N≈128, y a N=128 incluso pierde**
(7 260 frente a 6 912 de Karatsuba). Es el efecto de la escalera: `⌈N/3⌉`
redondea hacia arriba y a veces la partición en tres sale peor que dos niveles
de partición en dos. La ganancia real empieza en **N=256 (1,23× sobre
Karatsuba)** y llega a **1,77× en N=2048**.

**2. Toom-4 sólo despega a partir de N≈1024**, y ni siquiera de forma
monótona: en N=2048 pierde contra Toom-3. Antes de N=512 no aporta nada.

**3. El exponente efectivo entre N y 2N oscila**, y eso es real, no ruido:

| N → 2N | escolar | Karatsuba | Toom-3 |
|---|---:|---:|---:|
| 64 → 128 | 2,000 | 1,585 | 1,656 |
| 128 → 256 | 2,000 | 1,585 | **1,217** |
| 512 → 1024 | 2,000 | 1,585 | 1,419 |

Karatsuba da 1,585 **constante**; Toom-3 oscila entre 1,2 y 1,66 según dónde
caiga el redondeo, promediando 1,465. **Karatsuba es predecible y Toom-3 no**, y
eso importa para una biblioteca de anchura fija donde N es un parámetro de
plantilla: el usuario elige N y merece un coste que no dependa de la aritmética
del redondeo.

### Y la advertencia que ya nos ha mordido

Estas cifras son **cuenta de productos**, y la realidad medida en este proyecto
es que **Karatsuba no gana al escolar desenrollado hasta N=32**, pese a que la
cuenta le favorece desde N=23. La distancia entre la cuenta y el reloj son las
sumas y la recursión, que en anchuras pequeñas mandan.

Nótese que el umbral medido aquí (32) está **por encima** de la mediana de GMP
(23) y dentro de su rango (16–46). No es sospechoso: el escolar de esta
biblioteca está **desenrollado por recursión de plantilla**, que es más rápido
que el bucle de GMP a esas anchuras, así que el listón que Karatsuba tiene que
superar está más alto.

---

## 4. Lo que hacen los proyectos vivos

| Proyecto | Qué aporta | Dato |
|---|---|---|
| **GMP** | Referencia. Escalera completa: básico → Toom-2/3/4/6½/8½ → SSA. Umbrales **autoajustados por CPU** (`tuneup`) | Los umbrales de §0 |
| **FLINT** | Sustituyó SSA por **NTT con primos pequeños** (`fft_small`), 50 bits, con FMA de doble vectorizado | **3× más rápido que GMP** en un millón de dígitos, ~**7× asintóticamente**; 10× con 8 hilos |
| **NTL** | Referencia académica; se apoya en GMP para enteros | — |
| **MPIR** | Bifurcación de GMP | 485 s frente a los 360 s de FLINT en 17·10⁹ bits |

**El cambio de fondo de la última década es que la FFT entera ya no se hace con
Schönhage-Strassen**, sino con **NTT sobre varios primos de palabra**, porque la
aritmética modular de 50 bits se vectoriza con FMA de doble precisión y SSA no.
Es la razón de que FLINT saque 7× a GMP en el extremo alto.

### Hardware: donde está la ganancia que no cambia el exponente

- **AVX-512 IFMA** (`VPMADD52LUQ`/`HUQ`): ocho multiplicaciones de 52 bits por
  instrucción, con 12 bits de sobra por carril para acumular acarreos —
  representación de **radix reducido**. Publicado: **~3× sobre AVX-512F** y
  **~3× sobre GMP** en multiplicación grande; hasta 4,3× en Montgomery.
- **SIMD general** (AVX2, NEON, SVE2): el problema es la cadena de acarreos, que
  es secuencial por naturaleza; se ataca con prefijos paralelos tipo
  Kogge-Stone.
- **GPU**: hay trabajo reciente específico de **división** multiprecisión en GPU.

Fuentes: [FLINT: applications & benchmarks](https://flintlib.org/applications.html),
[FLINT furnished with faster FFT](https://fredrikj.net/blog/2023/04/flint-furnished-with-faster-fft/),
[NTL vs FLINT (Shoup, 2021)](https://libntl.org/benchmarks.pdf),
[Shoup, *Arithmetic Software Libraries*](https://shoup.net/papers/akl-chapter.pdf),
[Edamatsu & Takahashi, *Accelerating Large Integer Multiplication Using Intel AVX-512IFMA*](https://link.springer.com/chapter/10.1007/978-3-030-38991-8_5),
[Drucker & Gueron, *Fast modular squaring with AVX512IFMA*](https://eprint.iacr.org/2018/335.pdf),
[*Leveraging SIMD for Accelerating Large-number Arithmetic*](https://arxiv.org/pdf/2604.21566),
[*On GPU Implementation for Multi-Precision Integer Division*](https://arxiv.org/pdf/2606.06386).

---

## 5. Qué sale de todo esto para `fixed_int_t`

Ordenado por relación entre lo que da y lo que cuesta.

| # | Qué | Ganancia proyectada | Coste | Nota |
|---|---|---|---|---|
| 1 | Karatsuba **equilibrado** para toda N | Quita el acantilado; 1,585 constante en todo el rango | Medio | Ya decidido, [PLAN_MULTIPLICACION](PLAN_MULTIPLICACION.md) |
| 2 | **Möller–Granlund 3-por-2** en el bucle de Knuth D | Sólo la constante, pero afecta a **toda** división | **Bajo** | Es el mejor cambio por línea escrita. `algorithms/div_by_const.hpp` ya tiene la idea, pero sólo para el `uint128_t` viejo |
| 3 | **Burnikel–Ziegler** | Θ(N²) → Θ(M(N)·log N). En N=1024, del orden de **5×** | Medio-alto | Umbral esperado ~45–50 limbos |
| 4 | **Toom-3** | 1,23× sobre Karatsuba en N=256; 1,77× en N=2048. **Nada por debajo de 128** | Alto | División exacta por 3; exponente **no monótono** |
| 5 | **Newton/Barrett** para división | Θ(M(N)); techo de 2–4× una multiplicación | Alto | Encima de Burnikel–Ziegler |
| 6 | **Toom-4** | Sólo a partir de N≈1024 | Alto | Poco defendible antes que 3, 4 y 5 |
| 7 | **NTT / FFT** | El extremo: N≈4096 es donde empieza a ganar | Muy alto | `NSTD_KARATSUBA_MAX` = 4096 cae justo ahí |
| 8 | **AVX-512 IFMA** | ~3× publicado, **sin tocar el exponente** | Alto | Rompe la portabilidad: es una ruta aparte, no un reemplazo |

### La conclusión que cambia el orden que teníamos

**La división está peor servida que la multiplicación, y es más barata de
arreglar.** `operator*` ya tiene Karatsuba y le falta un reparto; `operator/`
está en el primer escalón de una escalera de cuatro, y el segundo escalón
—Möller–Granlund— es de constante, no de exponente, así que **no tiene umbral**:
gana en todas las anchuras, incluidas las pequeñas, que son las que más se usan.

Contrasta con Toom-3, que estaba anotado como P2.9: **no aporta nada por debajo
de N=128 y pierde contra Karatsuba en N=128**.

Sugerencia de reordenación, para decidir:

```
P2.8  Karatsuba equilibrado         (ya decidido)
  ↓
P2.10 Möller-Granlund 3-por-2       (nuevo: barato, sin umbral, toda N)
  ↓
P2.11 Burnikel-Ziegler              (nuevo: el salto de exponente en division)
  ↓
P2.9  Toom-3                        (baja de prioridad: nada por debajo de N=128)
```

---

## 6. Lo que este estudio **no** dice

- **No mide nada.** Todas las cifras de coste son cuentas de operaciones o
  números publicados por terceros. Lo único medido aquí es lo que ya estaba en
  [PERFORMANCE](PERFORMANCE.md).
- **No dice que los umbrales de GMP valgan aquí.** GMP los autoajusta por CPU y
  su básico es un bucle; el de aquí está desenrollado. El umbral de Karatsuba
  medido en esta biblioteca (32) ya salió distinto de la mediana de GMP (23).
- **No cubre la multiplicación desequilibrada** (N×M con N≠M), que GMP trata
  aparte y que aquí no existe porque el tipo es de anchura fija.
