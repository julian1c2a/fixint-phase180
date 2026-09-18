# Rendimiento

**Última actualización:** 5 September 2026

Fuente de verdad de las cifras de rendimiento del proyecto. Antes vivían
repartidas entre `README.md`, `PROJECT_STATUS.md` y `EXECUTIVE_SUMMARY.md`, con
medidas de fechas distintas que se contradecían entre sí.

> **Regla:** toda cifra de este documento lleva **fecha, compilador y máquina**.
> Una medida sin esos tres datos no se puede comparar con otra, y por tanto no
> vale. Si una cifra no los tiene, es que viene de antes de esta regla y hay que
> volver a medirla.

---

## Cómo se mide

```bash
python make.py bench gcc release-O2          # suite de benchmarks
bash scripts/benchmark_comparison.bash gcc release-O2 100000   # vs GMP/TomMath/Boost
```

Criterios que se aplican y por qué:

- **Mínimo de N rondas intercaladas, no media.** El mínimo es el estadístico
  robusto en benchmarking: la media la contamina cualquier proceso que despierte
  en la máquina. Durante la auditoría, una comparación aparentó un +40 % de
  regresión que al repetir con rondas intercaladas resultó ser ruido.
- **Intercalar las variantes** (A, B, A, B…), no medir todas las A y luego todas
  las B: el estado térmico de la máquina deriva.
- **Comprobar que los resultados coinciden** entre las variantes comparadas. Un
  benchmark que va más rápido porque calcula otra cosa no es un benchmark.

---

## Cuánto se mueven estas cifras

**Medido el 5 sep 2026.** El mismo binario, en la misma máquina, dos veces
seguidas, sin tocar nada entre medias:

| | rango entre ejecuciones |
|---|---:|
| mediana de las 33 medidas | **5,1 %** |
| percentil 90 | **15,6 %** |
| peor caso | **25,2 %** |

**Eso es el suelo de ruido, y cambia lo que se puede afirmar.** Una diferencia
del 10 % entre dos medidas **no significa nada**: entra de sobra en lo que se
mueve el mismo código consigo mismo. Publicar cifras con dos decimales, como se
venía haciendo, da una precisión que no existe.

Consecuencias prácticas:

- El umbral de aviso de
  [`scripts/bench_history.py`](../scripts/bench_history.py) está en **25 %**, no
  en el 5 % que parecía razonable a ojo. Por debajo de eso serían todo falsos
  positivos.
- Una cifra suelta no vale: hacen falta **varias ejecuciones y el rango**.
- Lo que sí sobrevive al ruido es **el signo y el orden de magnitud**. «Karatsuba
  gana» es sólido; «Karatsuba gana un 1,65×» no lo es tanto como parecía.

### Y la máquina tiene que estar ociosa

Esto se aprendió del peor modo posible: **midiendo mal mientras se escribía la
sección de arriba**. Dos tomas del mismo código, una de ellas con el equipo
compilando en paralelo, dieron diferencias de **hasta un 52 %** — el doble del
peor caso del ruido «en reposo».

O sea que el 25 % de la tabla anterior es el ruido **de una máquina tranquila**.
Con carga de fondo no hay umbral que valga: la medida sencillamente no sirve.
`scripts/bench_history.py` lo avisa antes de empezar.

### Y el orden dentro de la ejecución importa

La posición de un caso dentro del benchmark **afecta a su medida**. Se vio con
N=3: cuando se medía el último, después de N=16, daba 0,86×; al pasarlo a la
cuarta posición dio 1,12×–1,18×. Treinta puntos porcentuales por cambiar de
sitio, sin tocar una línea de código.

El benchmark **no controla esto** hoy. Mientras no lo haga, comparar dos casos
medidos en posiciones distintas no es legítimo.

---

## División — Knuth D frente a división binaria

**Medido el 9 September 2026**, GCC 16.2.0 (MSYS2 UCRT64) −O2, Windows 11 sobre
x86-64, MSI. Mínimo de 4 rondas. Nueve casos, `benchmark_divmod_algorithms`.

| | cyc/op |
|---|---:|
| `big_bin_divrem` (binaria) | 24,02 |
| `D_knuth_divrem` | 18,40 |
| **Knuth D frente a binaria** | **1,31×** |

Por caso, Knuth D gana siempre, pero entre **1,07× y 2,50×**: lo mejor es
«dividir por uno» (2,50×) y lo peor «128/128 grande» (1,07×).

> **Esta tabla decía 6,24×, y no era cierto.** La cifra venía heredada de la fase
> 1.75, sin fecha, compilador ni máquina — exactamente lo que la regla de arriba
> prohíbe. Al medirla salió 1,31×. Y las de por caso eran aún más llamativas:
> «potencia de 2, 12×» mide hoy **1,25×**; «valores de 64 bits, 7×» mide
> **1,14×**. No se sabe de dónde salían: pueden ser de un código muy anterior o
> de una comparación contra otra cosa. Lo único que se puede afirmar es lo que se
> mide hoy.
>
> El benchmark que produce esta tabla **no registraba en el histórico**. Ese fue
> el motivo de que la cifra sobreviviera dos fases sin que nadie la contrastara.

## División — frente a tipos built-in y a otras bibliotecas

**Medido el 9 September 2026**, mismas condiciones. `benchmark_vs_builtin`,
sección «Division (/)», mínimo de 4 rondas.

| Tipo | cyc/op | vs `uint64_t` |
|---|---:|---:|
| `uint64_t` | 4,01 | 1,00× |
| **`nstd::uint128_t`** | **3,26** | **0,81×** |
| `unsigned __int128` | 40,11 | 10,01× |
| Boost `cpp_int` | 45,06 | 11,25× |
| Boost `gmp_int` | 61,76 | 15,41× |
| Boost `tom_int` | 727,12 | 181,49× |

`nstd::uint128_t` sigue siendo **más rápido que el `uint64_t` nativo** en
división, que es el resultado llamativo: el camino rápido para divisores de un
limbo evita la llamada a `__udivti3` que el compilador emite para
`unsigned __int128`.

> **Dos de las tres cifras heredadas no se sostenían.** Decían `0,47×` para
> `nstd::uint128_t` (mide **0,81×**) y `~50×` para Boost `cpp_int` (mide
> **11,25×**). La tercera sí: `unsigned __int128` decía `9,56×` y mide **10,01×**.
>
> Este benchmark **no enlazaba** desde hacía meses, y por eso nadie lo había
> comprobado. No era un problema del código: le faltaban `-lgmp`, `-lgmpxx` y
> `-ltommath` en el enlace. Las tres bibliotecas estaban instaladas.

## División por constante — Granlund-Montgomery

**Medido el 9 September 2026**, mismas condiciones. `benchmark_div_by_const`,
mínimo de 4 rondas. Compara `div<D>()` con la división normal por el mismo valor.

| divisor | `div<D>()` | división normal | razón |
|---|---:|---:|---:|
| 3 | 10,88 | 6,84 | **0,63×** |
| 5 | 13,62 | 7,09 | **0,52×** |
| 7 | 13,65 | 11,11 | **0,81×** |
| 10 | 10,90 | 10,00 | **0,92×** |
| 100 | 12,43 | 14,87 | 1,20× |
| 10^19 | 13,48 | 76,11 | **5,65×** |

**El truco solo paga con divisores grandes.** Con divisores pequeños es más
lento que dividir normalmente, y por un margen que no es de ruido: 0,52× con el 5.

La razón es que la división normal por un divisor de **un limbo** ya toma un
camino rápido, así que no hay mucho que ganar; el 10^19 no cabe en 64 bits y ahí
sí entra Knuth D, que es a lo que Granlund-Montgomery gana de verdad.

> **Aquí decía «4–7× más rápido que Knuth D», sin matices.** Es cierto solo para
> el divisor que no cabe en un limbo (5,65×). Para los demás, el enunciado
> invitaba a usar `div<D>()` justo donde perjudica.

`rt_mulhi_128`: 4 × MUL nativo en GCC/Clang/Intel frente a 16 × MUL de 32 bits —
**1,8–2,2× más rápido**. *(Sigue sin re-medir: no tiene benchmark propio.)*

`div<D>()`, `mod<D>()` y `divmod_const<D>()` siguen siendo de `int128_param_t`;
portarlos a `fixed_int_t` es parte de
[ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md).

---

## `fixed_int_t<N>` — divmod

**Medido el 23 August 2026**, GCC 16.2.0 (MSYS2 UCRT64) −O2, Windows 11 sobre
x86-64. Siete rondas intercaladas, mínimo por caso. La comparación es entre el
código anterior y el posterior a hacer `constexpr` la división (v1.90.1), y sirve
como referencia de coste por operación.

| Caso | ns/op |
|---|---:|
| N=2, divisor de 1 limbo | 49,1 |
| N=2, divisor de 2 limbos | 21,4 |
| N=4, divisor de 1 limbo | 141,0 |
| N=4, divisor de 2 limbos (Knuth D) | 151,9 |
| N=4, divisor de 3 limbos (Knuth D) | 117,9 |
| N=8, divisor de 4 limbos (Knuth D) | 279,0 |

El caso de dos limbos sale más barato que el de uno porque con divisores
aleatorios de la anchura completa el early-out `a < b` se dispara la mitad de las
veces.

---

## Multiplicación — Karatsuba frente al método escolar

**Medido el 25 August 2026**, GCC 16.2.0 (MSYS2 UCRT64) -O2 `-march=native`,
Windows 11 sobre x86-64. 256 operandos pseudoaleatorios, 400 000 iteraciones x
7 rondas intercaladas, mínimo por caso. Dos ejecuciones independientes.

`operator*` toma el camino de Karatsuba para **N=4 y N=8**, y el escolar O(N^2)
para el resto. La referencia es una **copia fiel** del bucle escolar de la
propia biblioteca, con sus mismas primitivas (`intrinsics::umul128` y
`intrinsics::addcarry_u64`), de modo que lo único que cambia entre las dos
ramas es el algoritmo.

| N | bits | razón (escolar / biblioteca) |
|---|---:|---|
| **4** | 256 | **1,65× – 2,04×** |
| **8** | 512 | **1,48× – 1,82×** |
| 2 | 128 | 3,57× – 3,76× *(camino especializado, no Karatsuba)* |

**Los rangos son anchos a propósito**: reúnen las cuatro ejecuciones de los días
25 ago y 5 sep, y esa anchura *es* el resultado. Con un suelo de ruido del 15 %
(ver arriba) no se puede afirmar «1,65×»; lo que se puede afirmar es que
**Karatsuba gana con holgura EN GCC, entre 1,5× y 2×**, que es una conclusión
más pobre y verdadera. Y hay que decir «en GCC»: en MSVC pierde, ver la sección
siguiente.

Los valores absolutos en ciclos por operación están en el histórico
(`benchs/history/`), no aquí: se mueven demasiado entre ejecuciones para
publicarlos como si fueran una propiedad del código.

### El reparto nuevo, y lo que gana (6 sep 2026)

**Esta seccion sustituye en la practica a las dos de abajo**, que quedan como
testimonio de como se llego aqui.

`operator*` reparte asi desde el 6 sep 2026:

| anchura | camino | por que |
|---|---|---|
| N = 2 | especializado de 128 bits | `__int128` o `_umul128`, tres productos |
| N = 3 … 16 | **escolar DESENROLLADO por construccion** | de 2x a 4,9x mas rapido que el bucle |
| N potencia de dos, >= 32 | **Karatsuba** | ahi si gana: N^1.585 vence a la constante |
| resto | escolar en bucle | desenrollar deja de pagar |

Los umbrales son macros --`NSTD_DESENROLLA_MAX`, `NSTD_KARATSUBA_MIN`-- para
poder barrerlos sin tocar el fichero, que es como se han fijado.

**Lo que gana, medido**: minimos de 4 rondas en orden aleatorio, misma maquina,
antes = el arbol en `6270a05`, despues = el de ahora.

| N | GCC 16.2 | Clang 22.1 | MSVC 19.51 | Intel 2026.1 |
|---|---|---|---|---|
| 2 | 0,96x | 1,05x | 1,00x | 1,05x |
| 3 | 2,34x | 3,83x | 1,55x | 1,33x |
| 4 | 2,01x | 1,89x | 2,51x | *(nula)* |
| 5 | 2,97x | 1,90x | 1,98x | 3,30x |
| 6 | 3,04x | 1,85x | 1,86x | 3,06x |
| 7 | 3,29x | 2,12x | 2,13x | 3,12x |
| 8 | 2,92x | 1,09x | 3,04x | 1,63x |
| 9 | 3,99x | 2,23x | 2,22x | 3,57x |
| 10 | 4,52x | 2,46x | 2,23x | 3,56x |
| 12 | **4,92x** | 2,26x | 2,21x | 3,60x |
| 16 | 1,99x | 1,55x | 1,64x | 2,26x |
| 32 | 1,39x | 1,68x | 1,38x | 1,73x |

N=2 no cambia --tiene camino propio-- y esa fila sirve de control: sale 1,00x
como debe. La casilla de Intel en N=4 esta **descartada por la comprobacion de
verosimilitud**: daba 2,03 cyc/op para diez productos, o sea 0,20 ciclos por
producto, y el benchmark sale con error en vez de publicarla.

**Lo que cuesta.** El tiempo de CONSTRUCCION sube alrededor de un 25 %: la suite
completa pasa de 163 a 207 s con GCC, de 434 a 534 con MSVC. Desenrollar N=16
son 136 productos en linea recta, y ese tipo se instancia en muchos sitios. Es
la razon de que el tope este en 16 y no mas arriba, ademas de que a partir de
N=32 el desenrollado ya no paga (1,02x, medido).

**Correccion, antes de creerse nada de esto**: 860 256 pares por compilador
contra una referencia escolar de 32 bits --aritmetica del lenguaje, sin
`umul128` ni `addcarry`, para que no comparta primitivas-- con cero
discrepancias en los cuatro. Y la suite entera, 58/58 en los cuatro.

---

### Los cuatro compiladores (6 sep 2026)

Lo de arriba es **sólo GCC**, y hasta hoy no se había medido en los demás. Con
los otros tres la conclusión no es la misma. Commit `ad7abba`, misma máquina,
cuatro tomas por compilador, razón escolar/biblioteca:

| N | GCC 16.2 | Clang 22.1 | Intel 2026.1 | **MSVC 19.51** |
|---|---|---|---|---|
| 2 | 3,2 – 3,4× | 0,99× | 1,29× | 1,53 – 1,58× |
| **4** | 1,55 – 1,70× | 1,95× | 2,61× | **0,64 – 0,75×** |
| **8** | 1,24 – 1,35× | 1,85× | 2,10× | **0,62 – 0,84×** |
| 3, 5, 6, 7, 9, 10, 12, 16 | ~1,0 | ~1,0 | ~1,0 | ~1,0 |

**En MSVC, Karatsuba es entre un 30 % y un 45 % MÁS LENTO que el escolar**, en
los dos únicos tamaños donde se usa.

Que esto es señal y no ruido lo dicen los controles, que aquí salen gratis: en
todos los N donde Karatsuba **no** está activo las dos ramas son el mismo
código, así que su razón tiene que ser 1,00. Medida, se queda entre **0,99 y
1,11** en los cuatro compiladores y las cuatro tomas. Los 0,6-0,7 de MSVC están
muy fuera de esa banda, y se repiten en las cuatro.

Lo que **no** se sabe todavía: por qué. Karatsuba cambia siete multiplicaciones
de limbo por tres más sumas y restas; que eso salga a perder sugiere que en MSVC
el escolar se optimiza mucho mejor de lo que se penaliza el reparto, o que
`_umul128` y `_addcarry_u64` se comportan distinto de sus equivalentes GNU. Sin
mirar el ensamblador, es una conjetura.

Tampoco se sabe **dónde estaría el umbral en MSVC**: hoy Karatsuba solo se activa
en N=4 y N=8, así que N=16 y N=32 no dicen nada del algoritmo, solo del escolar
contra sí mismo. Para hallar el umbral hay que activarlo en más anchuras primero.


### Los controles, y la anomalía que no era

El benchmark mide varios N que **no** usan Karatsuba, donde la razón debería
salir ~1,00×. Sirven para comprobar que la implementación de referencia es fiel.

**El 25 ago 2026 el control de N=3 dio 0,86× dos veces seguidas**, y se
concluyó que el bucle escolar de la biblioteca era un 14 % más lento que una
copia idéntica suya. Se anotó como deuda: «hay que mirar la generación de
código para N pequeño e impar».

**No era cierto.** Al barrer N=3, 5, 7, 9 frente a N=6, 10, 12, 16 el 5 sep
2026, no aparece ningún patrón:

| N | razón | | N | razón |
|---|---:|---|---|---:|
| 3 | 1,12× – 1,18× | | 6 | 1,03× – 1,06× |
| 5 | 1,13× – 1,15× | | 10 | 0,96× – 1,08× |
| 7 | 1,03× – 1,13× | | 12 | 1,11× – 1,13× |
| 9 | 0,99× – 1,13× | | 16 | 1,00× – 1,02× |

Ni paridad ni tamaño: dispersión sin estructura, y **N=3 sale del lado
contrario** al de agosto.

**El error fue de método, no de medida.** Llamar «estable» a dos ejecuciones
seguidas en la misma sesión no prueba reproducibilidad: prueba que la máquina
estuvo igual durante cinco minutos. Entre sesiones —y cambiando N=3 de la
última posición a la cuarta— el 0,86× no reaparece.

No hay ninguna anomalía de `operator*` que perseguir.

La primera versión de este benchmark usaba una propagación de acarreo
portable en vez de los intrínsecos. El control de N=16 salió **2,00x** y
el de N=2 **6,23x**: no estaba midiendo Karatsuba contra el método escolar,
sino la biblioteca contra un espantapájaros. Los controles existen por eso.

---

## `operator*` conectado a los núcleos: la ganancia real de la biblioteca

**10 September 2026.** `mul_sin_marca` ya no implementa nada: **reparte** entre
los núcleos de `algorithms/mul_kernels.hpp`. El único cambio de comportamiento
es que **Karatsuba deja de exigir potencia de dos**, que era la causa del
acantilado.

```
N == 2                → especializado con __int128 / _umul128   (sin cambio)
evaluación constante  → escolar (los núcleos escolares sí son constexpr)
N ≤ NSTD_DESENROLLA_MAX → escolar desenrollado
hasta NSTD_KARATSUBA_MIN → escolar en bucle
N ≥ NSTD_KARATSUBA_MIN  → Karatsuba EQUILIBRADO, cualquier N
```

### La ganancia, medida antes/después

Mínimo de **tres rondas A/B alternadas**, mismo compilador (clang) y misma
máquina, compilando el «antes» contra el árbol extraído de HEAD:

| N | antes | después | mejora |
|---:|---:|---:|---:|
| 32 | 4 704 | 3 494 | **1,35×** |
| **48** | 21 222 | 13 819 | **1,54×** |
| 64 | 19 198 | 14 958 | **1,28×** |

N=48 es el punto del acantilado: antes caía al bucle por no ser potencia de dos.

### Dos regresiones que este cambio introdujo, y que sólo el antes/después vio

**1. N=64 salió 0,89×, PEOR que antes.** El Karatsuba viejo calculaba su término
del medio con el `operator*` de media anchura, que a HH=32 **volvía a entrar en
Karatsuba**. Al conectar se puso `medio_escolar<NSTD_DESENROLLA_MAX>`, que con el
tope en 20 manda un producto de 32 limbos al **bucle**.

Es el hallazgo del barrido de Karatsuba —que el término del medio vale hasta
1,9×— cometido en la dirección contraria. Arreglado con `medio_reparto`, que
devuelve el medio al reparto completo: N=64 pasó de **0,89× a 1,42×**.

**2. N=12..20 salían 0,77×–0,92×.** `fixed_int_t r{}` ya value-inicializa a cero
y el núcleo **volvía a poner a cero**. Los escolares reciben ahora
`Limpiar=false` cuando el destino ya es cero.

### Y la comprobación que no depende del banco

Tras los dos arreglos, N pequeño seguía oscilando entre 0,82× y 1,59× **sin
patrón**, en una zona donde el camino es idéntico por construcción. En vez de
discutir con el ruido se comparó el **código emitido**:

| | antes | después |
|---|---:|---:|
| `mulq` | 148 | 148 |
| `adcq` | 148 | 148 |
| `addq` | 308 | 308 |
| `movq` | 895 | 895 |
| líneas de ensamblador | 5 473 | 5 473 |

**Cero diferencias que no sean nombres de símbolo** —
`fixed_int_t<…>::fila_desenrollada` pasó a
`algorithms::detail::fila_desenrollada`—. El código es el mismo, luego no puede
haber regresión: era ruido del arnés viejo, que es el que `benchmark_curva_n`
todavía usa.

> **Lección de método**: cuando dos medidas de algo que debería ser idéntico no
> coinciden, comparar el código emitido zanja la discusión en un minuto.
> `scripts/bench_asm.py` existe para eso.

### Lo que queda abierto

- **El hueco entre los dos umbrales.** Con 20 y 32, las anchuras 21..31 van al
  bucle, y el equilibrado le gana ahí ~2,2×. Cerrarlo es bajar
  `NSTD_KARATSUBA_MIN`, pero antes hay que medir el equilibrado **contra el
  desenrollado** cara a cara — sólo se ha comparado contra el bucle.
- **Código muerto en la clase.** `kmul_full`, `filas_desenrolladas` y compañía ya
  no los usa `operator*`, pero `benchmark_karatsuba.cpp` todavía llama a
  `kmul_full`. Retirarlos va aparte.

---

## El acantilado, resuelto: Karatsuba con reparto equilibrado

**Escrito y medido el 10 September 2026.** `mul_karatsuba_equilibrado` quita la
limitación que causaba el acantilado: el Karatsuba de la biblioteca sólo admite
**potencias de dos**, así que toda anchura mayor que el tope de desenrollado que
no lo sea cae al bucle escolar y cuesta 3–4× por limbo — N=24 tardaba más que
N=32.

El equilibrado parte en dos mitades iguales cuando N es par y **rellena con un
solo limbo** cuando es impar. No hasta la potencia de dos siguiente: eso ya se
descartó midiendo, porque para N=48 daría el coste de N=64, que es peor que el
del escolar.

### Correcto en las 63 anchuras, no sólo en las que le convienen

`tests/test_mul_kernels.cpp` lo cruza contra el escolar en **N = 2..64, todas**,
con 60 pares al azar cada una más `max·max` —el que más acarreos encadena— y una
potencia de dos en el limbo más alto. **63 de 63.**

Lo que había que comprobar no eran las potencias de dos, que ya funcionaban, sino
las **impares**, que llevan el camino del relleno.

### El acantilado desaparece

Contra el bucle escolar, clang, barrido denso de N=8 a 64:

| N | clase | razón |
|---:|---|---:|
| 16 | pot2 | 2,93× |
| 20 | par | 2,76× |
| **24** | par | **2,96×** ← el punto del acantilado |
| 32 | pot2 | 2,41× |
| 36 | par | 1,95× |
| **48** | par | **1,77×** ← el otro punto |
| 52 | par | 1,59× |

**La razón más baja de todo el barrido es 1,25× en N=9**: el equilibrado **no
pierde en ninguna anchura**, que es justo lo que el reparto de hoy no puede
decir.

Y no hay dientes: **las potencias de dos ya no destacan sobre sus vecinas**. Antes
eran las únicas que tenían Karatsuba; ahora todas lo tienen.

> **Sobre esta tanda**: se midió con la máquina cargada y varias casillas salen
> con recorridos altos, alguna por encima del 200 %. La conclusión no depende de
> ninguna casilla suelta — se sostiene sobre 57 anchuras consecutivas, todas del
> mismo lado— pero las cifras individuales de esta tabla no valen a dos dígitos.

---

## El umbral de Karatsuba, y una segunda perilla que estaba escondida

**Medido el 10 September 2026** con `benchmark_barrido_karatsuba`, en los cuatro
compiladores, cada uno en solitario. Dos ejes: desde qué anchura gana Karatsuba,
y **cómo se calculan sus dos términos del medio** — que es otra decisión de
reparto, y estaba enterrada dentro del algoritmo hasta que
`algorithms/mul_kernels.hpp` la sacó.

Sólo potencias de dos: es la limitación del Karatsuba de hoy.

### Karatsuba contra el mejor escolar

| N | clang | gcc | MSVC | Intel |
|---:|---:|---:|---:|---:|
| 4 | 1,30× | 0,62× | 0,45× | 0,71× |
| 8 | 1,16× | 0,48× | 0,48× | 0,63× |
| 16 | 1,21× | 0,53× | 0,62× | 0,95× |
| **32** | 2,49× | **1,02×** | 1,35× | 1,44× |
| 64 | 2,55× | 1,40× | 1,42× | 2,07× |
| 128 | 2,80× | 1,57× | 1,77× | 2,48× |

Las ganancias de clang en N=4, 8 y 16 **no superan el ruido de sus casillas** y
no cuentan. Los otros tres pierden ahí de forma clara. **Karatsuba no debe bajar
de 32.**

### Los dos umbrales interactúan, y esto es lo importante

En N=32 la cifra de arriba depende de con qué se compare. Contra el **bucle**,
que es el rival real si el tope de desenrollado queda por debajo de 32:

| N=32 | contra el mejor escolar | contra el **bucle** |
|---|---:|---:|
| clang | 2,49× | **3,25×** |
| gcc | **1,02×** | **1,42×** |
| MSVC | 1,35× | **1,51×** |
| Intel | 1,44× | **2,12×** |

La diferencia es enteramente el desenrollado. Con GCC, Karatsuba y el
desenrollado **empatan** en N=32; contra el bucle, Karatsuba gana 1,42×.

**El par (`NSTD_DESENROLLA_MAX` = 26, `NSTD_KARATSUBA_MIN` = 32) es coherente**:
con el tope en 26 el desenrollado no existe en N=32, y ahí Karatsuba gana a su
rival real en los cuatro compiladores, de 1,42× a 3,25×.

Dicho al revés: **si el tope de desenrollado subiera a 32, el umbral de
Karatsuba habría que revisarlo**, porque en GCC dejarían de estar claramente
ordenados. Los dos números no se pueden elegir por separado.

### La segunda perilla: el término del medio

Karatsuba necesita dos productos de N/2 limbos. Hoy los calcula con el
`operator*` de media anchura, que vuelve a repartir por las macros; a N=64 eso
significa el **bucle escolar** para un producto de 32 limbos, justo donde
Karatsuba ya gana. La alternativa es que recurra en sí mismo.

Recursivo frente a escolar, que es lo de hoy:

| | N=32 | N=64 | N=128 |
|---|---:|---:|---:|
| **clang** | **1,49×** | **1,93×** | **1,94×** |
| **Intel** | 0,87× ⁿˢ | **1,57×** | **1,69×** |
| gcc | 0,91× ⁿˢ | 1,22× ⁿˢ | 1,19× ⁿˢ |
| MSVC | 0,87× ⁿˢ | 1,09× ⁿˢ | 1,23× ⁿˢ |

ⁿˢ = la diferencia no supera la suma de los recorridos de sus dos casillas.

> **Corrección.** Al ver sólo la columna de clang se escribió aquí que «la
> biblioteca está dejando cerca de 2× sobre la mesa». **Eso vale para clang, y a
> medias para Intel; no para los cuatro.** GCC y MSVC no dan una ganancia
> demostrable, y en N=32 los tres no-clang salen levemente peor. Es el mismo
> error que este documento lleva media semana cazando —concluir de un compilador
> lo que hace falta medir en cuatro— cometido esta vez sobre el hallazgo propio.

Lo que sí sostienen los cuatro: **de N=64 en adelante el medio recursivo no
pierde en ninguno** (1,09× a 1,93×) y gana de forma demostrable en dos. En N=32
no hay caso. Sería, si se adopta, un **tercer umbral**: medio recursivo desde 64.

### Sobre el ruido de esta tanda, y una hipótesis mía que resultó falsa

La primera pasada de gcc, MSVC e Intel salió con 30–63 % de recorrido, frente al
8–37 % de la de clang. Se atribuyó a que los tres corrieron encadenados
—compilar y medir seguido, con la máquina caliente— y se repitió **con 90 s de
enfriamiento entre compilar y medir y entre compiladores**.

**No cambió nada**: la segunda pasada da 45–69 %. La hipótesis era falsa; la
dispersión de esos tres binarios es intrínseca, no térmica.

Lo que sí quedó demostrado es más útil: **las razones reproducen entre las dos
tandas independientes dentro del 5–15 %**, pese al recorrido alto de cada
casilla. Vuelve a confirmar la regla del banco — el mínimo y las razones son
fiables, las cifras absolutas no valen a más de dos dígitos.

---

## El cuadrado: `x * x` no es `a * b` (16 sep 2026)

En Karatsuba los dos términos del medio de `a · a` —`a_lo·a_hi` y `a_hi·a_lo`—
**son el mismo**: se calcula uno y se suma dos veces. `benchmark_cuadrado`, clang,
barrido de N=4 a 64:

> **Gana en las dieciséis anchuras, de 1,17× a 2,47×, mediana 1,5×.**

`operator*` lo detecta **comparando punteros, no valores**. El caso que importa
es `pow`, que hace `base *= base`; comparar valores costaría N comparaciones para
ganar en un caso raro, mientras que detectar la variable repetida cuesta una.

### Y un cuadrado que se escribió y se tiró el mismo día

Se escribió también `sqr_escolar_bucle`, para las anchuras por debajo de
Karatsuba. **Perdía en las dieciséis, de 0,35× a 0,80×**, y se retiró.

El fallo era de **diseño, no de aritmética**: se implementó calculando el
cuadrado *completo* de 2N limbos y truncando a N, porque doblar los cruzados sin
perder acarreos es más cómodo así. Pero eso cuesta exactamente lo que la simetría
ahorra —el doble de trabajo para ahorrar la mitad—, y encima deja la sobrecarga
del array intermedio. Queda anotado en el header, en el hueco donde estaba.

---

## El rango 64..4096, que nunca se había medido (16 sep 2026)

Todo lo anterior llegaba a N=64 y `NSTD_LIMBOS_MAX` es 4096: la **mitad alta del
rango admitido estaba sin mirar**. `benchmark_rango_alto`, clang, trece anchuras
con un punto intermedio entre cada par de potencias de dos —mirar sólo potencias
de dos es lo que escondió el acantilado durante meses—.

| N | bucle | equilibrado | razón |
|---:|---:|---:|---:|
| 64 | 20 990 | 6 346 | 3,31× |
| 128 | 83 710 | 20 864 | 4,01× |
| 256 | 388 501 | 89 157 | 4,36× |
| 512 | 1 342 145 | 239 049 | 5,61× |
| 1024 | 5 408 039 | 763 134 | 7,09× |
| 2048 | 21 437 766 | 2 340 489 | 9,16× |
| 4096 | 87 813 680 | 7 063 876 | **12,43×** |

*(la tabla completa lleva además 96, 192, 384, 768, 1536 y 3072; ninguna casilla
salió marcada como ruidosa)*

**La caché no manda, y la pregunta se responde al revés de como se planteó.**
El ajuste log-log sobre los trece puntos da:

| | exponente | peor residuo |
|---|---:|---:|
| bucle escolar | **1,993** | 14,6 % |
| Karatsuba equilibrado | **1,687** | 20,3 % |

El bucle es un N² de libro. Y el equilibrado, por tramos de potencias de dos, da
1,675 → 1,617 → **1,594** en los tres últimos: **converge hacia el 1,585 teórico
en vez de alejarse hacia 2**. A N=4096 —donde un operando son 32 KB y los dos no
caben en L2— la caché todavía no ha entrado.

### La perilla `Base`, que estaba sin medir

`kmul_full_gen` corta la recursión en `Base` limbos y baja al escolar completo.
Estaba en 8 **porque se escribió así**. Barrido de `Base` ∈ {4, 8, 16, 32, 64},
las cinco entrelazadas:

| N | Base=4 | Base=8 | Base=16 | Base=32 | Base=64 | mejor |
|---:|---:|---:|---:|---:|---:|---:|
| 128 | 23 092 | **20 968** | 21 187 | 22 963 | 24 381 | 8 |
| 512 | 270 360 | 248 596 | **239 399** | 291 029 | 296 643 | 16 |
| 2048 | 2 665 125 | **2 297 369** | 2 341 794 | 2 704 597 | 2 983 698 | 8 |

**Resultado negativo, y por eso mismo útil**: ninguna diferencia sobre el 8 supera
el ruido, así que **no había ganancia gratis**. La curva sí es real —Base=4 cuesta
un 10–16 % más y Base=64 un 16–30 %— con el fondo plano entre 8 y 16. El 8 se
queda, ahora **medido en vez de supuesto**.

Antes de poder medirlo hubo que arreglar el banco: `medio_reparto` llamaba a
`mul_karatsuba_equilibrado<H, 8, …>` con el **8 escrito a mano**, así que cambiar
`Base` arriba no cambiaba nada por debajo del primer nivel. Un barrido hecho sin
eso mide una mezcla y no un corte — y habría dicho «8 es el mejor» por
construcción.

---

## Toom-3: dónde cruza de verdad (17 sep 2026)

Toom-3 hace cinco productos de M/3 donde Karatsuba hace tres de M/2: exponente
log5/log3 = **1,465** frente a log3/log2 = 1,585.

> ### La proyección teórica se equivocó por un factor de 4 a 8
>
> El [estudio](ESTUDIO_ALGORITMOS_RAPIDOS.md) situaba el cruce en N=128–256 y
> decía «gana 1,23× en N=256». **Lo medido: pierde en 256 (0,87×), pierde en 512
> (0,96×) y no cruza hasta ~1024.**
>
> No falla la teoría: el exponente 1,465 es correcto. Falla **la constante**, que
> se estimó en 2,5×–3× del término líder y es **4,6×**. Un cruce en 1024 implica
> exactamente `1024^(1,687−1,465) = 4,6`.

### Y hacen falta dos umbrales, no uno

El mismo M se comporta distinto según el papel que juegue:

| M | Toom≥96 | Toom≥256 | Toom≥512 | Toom≥1024 |
|---:|---:|---:|---:|---:|
| 512 | 0,92× | 0,94× | 0,96× | *(no entra)* |
| 1024 | 1,05× | 1,02× | 1,00× | 0,98× |
| 2048 | **1,13×** | 1,07× | 1,05× | 1,02× |
| 4096 | 1,12× | **1,17×** | 1,04× | 1,08× |

- **Entrar** en Toom-3 con 512 limbos **pierde un 5–8 %**.
- Pero un subproducto de ~512 limbos **dentro** de uno de 4096 sale **mejor** con
  Toom-3 que con Karatsuba. Lo más probable es la caché: a esa altura ya no queda
  nada útil en L2, y los subproblemas de Toom-3 son de M/3 frente a los M/2 de
  Karatsuba.

Con un solo umbral no se pueden tener las dos cosas, porque el mismo M aparece en
los dos papeles. De ahí `NSTD_TOOM3_MIN` = **1024** (entrada, alto: **no hay
regresión en ninguna anchura**) y `NSTD_TOOM3_REC` = **96** (recursión, bajo: es
de donde sale la ganancia).

### La confirmación, y por qué hizo falta

La primera tanda dejó **las seis casillas marcadas como ruidosas**, y la serie
hermana —Toom-3 de un solo nivel— salía errática (0,76 · 0,91 · 0,85 · 1,02 ·
0,79 · 0,99), que es justo el aspecto que tiene el ruido. Se repitió sólo K contra
TR, con **30 repeticiones** y **dos vueltas**:

| M | K | recorrido | TR | recorrido | K/TR |
|---:|---:|---:|---:|---:|---:|
| 2048 | 2 368 751 | 27,6 % | 2 098 900 | 26,9 % | 1,129× |
| 3072 | 4 418 347 | 26,5 % | 3 910 735 | 26,1 % | 1,130× |
| 4096 | 7 160 572 | 28,6 % | 6 277 795 | 27,4 % | 1,141× |
| 2048 *(2ª)* | 2 379 857 | 27,3 % | 2 096 911 | 32,8 % | 1,135× |
| 3072 *(2ª)* | 4 448 038 | 26,4 % | 3 891 633 | 27,2 % | 1,143× |
| 4096 *(2ª)* | 7 232 276 | 29,5 % | 6 270 654 | 27,6 % | 1,153× |

**Los mínimos reproducen dentro del 0,09–1,00 % y las razones dentro del
0,5–1,2 %.** Seis medidas independientes entre 1,129× y 1,153×.

> ### El criterio de «casilla ruidosa» del banco está mal planteado
>
> Compara la razón contra la **suma de recorridos** de las dos casillas. Con un
> 27 % por casilla el listón queda en el 55 %, y así **no marcaría nunca nada**.
>
> Pero el estadístico que el banco usa **es el mínimo**, y el mínimo reproduce al
> 1 %. El recorrido mide la dispersión de las repeticiones —cada interrupción del
> planificador entra ahí—, no la incertidumbre del mínimo. **Hay que contrastar
> contra la reproducibilidad del mínimo entre tandas, no contra el recorrido
> dentro de una.** Queda pendiente arreglarlo en `bench_adaptativo.hpp`.
>
> Corolario práctico: una marca de ruido mal calibrada **cuesta una tanda entera**,
> porque obliga a repetir para poder concluir algo que ya estaba medido.

### La trampa aritmética que casi pasa desapercibida

Toom-3 evalúa en x = −1, o sea `a0 − a1 + a2`, que puede ser **negativo** sobre
arrays de limbos sin signo. Se trabaja en complemento a dos sobre Z/2^(64W), con
lo que sumar y restar son las operaciones de siempre. Dos cosas no se salvan
solas:

1. **El producto de doble anchura.** Multiplicar sin signo dos representaciones en
   complemento a dos acierta los limbos **bajos** y nada más. Sin la corrección
   —restar Y de la mitad alta si X es negativo, y X si lo es Y— el resultado
   *parece* correcto en las anchuras pequeñas y falla en cuanto el punto −1 sale
   negativo de verdad.
2. **La división por 3 no es multiplicar por `0xAAAAAAAAAAAAAAAB`.** Ese es el
   inverso de 3 módulo 2⁶⁴, **no** módulo 2^(64W): `3 · 0xAAAA…AAAB` vale 2⁶⁵ + 1,
   que truncado a un limbo es 1 pero sobre el anillo entero deja 2⁶⁵ de residuo.
   Hay que usar la cadena de división exacta de Jebelean. El contraejemplo —M=3,
   todo unos: 6q² daba `[2, 0, …fa, 3]` en vez de `[2, …fc, 1, 0]`— está escrito
   en el código, porque el error parece razonable y se volvería a cometer.

---

## La división: la línea base, y un comentario que mentía (17 sep 2026)

Primer paso del frente de la división, y va **antes** de tocar ningún algoritmo:
una cifra tomada después de reescribir el código no se puede comparar con las de
hoy.

### Lo primero: sacar Knuth D de dentro de `divmod`

Hasta hoy el algoritmo D vivía **dentro** de `fixed_int_t::divmod`, unas 300
líneas con los casos especiales, las ramas `#if` por compilador y el bucle
principal mezclados. Con el algoritmo dentro del método **no se pueden medir dos
variantes entrelazadas**: habría que recompilar entre una y otra, y el protocolo
de [PLAN_SESION_MEDICION](PLAN_SESION_MEDICION.md) exige rondas alternas en el
mismo proceso.

Ahora está en `include/algorithms/div_kernels.hpp`, y **la estimación del dígito
del cociente —el paso D3— es un parámetro de plantilla**, no código escondido
dentro. Es el bucle interno de toda la división y es exactamente lo que sustituye
Möller–Granlund: con la perilla fuera, comparar las dos estimaciones será cambiar
un tipo. Es la lección de `medio_reparto` en Karatsuba, aplicada antes de
necesitarla.

**La extracción salió gratis**: A/B contra el árbol de `HEAD`, tres rondas
alternas, razones entre 0,94× y 1,07× repartidas a los dos lados del 1,00.

> **Y aquí la comparación de ensamblador NO zanjó la cuestión**, al revés que con
> `operator*`. El código emitido cambió de forma: **2–3× más pequeño** en N=4, 8 y
> 16 (459→172, 597→290, 654→286 instrucciones) y **2,5× más grande** en N=32
> (840→2087), porque clang decidió desenrollar. Un cambio de forma no es una
> regresión ni una mejora: hay que medirlo. La regla «asm idéntico ⟹ sin
> regresión» sirve cuando sale idéntico; cuando no, no dice nada.

### Lo que hay que medir no es una curva, es una superficie

La multiplicación depende de una variable. La división depende de **dos**, y
quien no lo tenga en cuenta medirá la casilla equivocada:

- **N**, la anchura de los operandos;
- **n**, los limbos **significativos del divisor**.

El coste de Knuth D es `(N − n + 1)` pasadas de `O(n)` cada una, o sea
`O((N−n)·n)`: máximo en `n = N/2` y **se desploma en los dos extremos**. Con
`n = 1` entra el camino rápido de un limbo; con `n = N` hay una sola pasada.

**El detalle que importa: si divides dos números al azar de N limbos, casi
siempre te sale `n = N`, que es el caso BARATO.** Un banco escrito sin pensarlo
mediría justo la casilla que no duele.

Ciclos por `divmod`, las cuatro formas entrelazadas (`benchmark_division`):

| N | n = 1 | n = 2 | n = N/2 | n = N |
|---:|---:|---:|---:|---:|
| 4 | 320 | 333 | 345 | 81 |
| 8 | 694 | 814 | 652 | 77 |
| 16 | 1 436 | 1 735 | 1 321 | 219 |
| 32 | 3 064 | 3 473 | 3 025 | 287 |
| 64 | 6 152 | 7 108 | 8 897 | 537 |
| 128 | 12 236 | 14 357 | 27 524 | 370 |
| 256 | 24 985 | 29 243 | **88 789** | 2 944 |

**El máximo se mueve con N.** Hasta N=32 el peor caso es `n=2`; desde N=64 lo es
`n=N/2`. Es el cruce entre el término lineal `2(N−2)` y el cuadrático `N²/4`,
sólo que cae en **N≈48** y no en N=8 como diría la cuenta a secas — señal de que
**el coste por dígito de cociente domina sobre el coste por limbo**.

### El hallazgo: `__udivti3` no emite un `divq`, emite una llamada

El código afirmaba, sobre el camino rápido de divisor de un limbo:

> *«With `rem < d`, libgcc's `__udivti3` / `_udiv128` emit a SINGLE divq. Total
> cost: N divq instructions instead of 64N² bit-loop iterations.»*

**Es falso.** En la unidad de traducción de prueba hay **16 llamadas a
`__udivti3` y sólo 4 instrucciones `divq`**. Clang no puede saber que `hi < d`
—es una precondición escrita en la documentación, no en el tipo— así que llama a
la rutina general de 128/128. La línea base lo corrobora: la columna `n = 1`
costaba **~118 ciclos por limbo**, constante de N=4 a N=256.

El arreglo ya estaba escrito **en el propio fichero**: la rama de ICX-Windows usa
`__asm__("divq %4" ...)` porque allí `__udivti3` ni siquiera existe en el
runtime. Extendida a GCC y Clang en x86-64, bajo `!is_constant_evaluated()`:

| N | `__udivti3` | por limbo | `divq` | por limbo | razón |
|---:|---:|---:|---:|---:|---:|
| 4 | 384 | 95,9 | 290 | 72,5 | 1,32× |
| 16 | 1 926 | 120,4 | 1 335 | 83,4 | **1,44×** |
| 64 | 7 300 | 114,1 | 5 506 | 86,0 | 1,33× |
| 256 | 30 506 | 119,2 | 22 541 | 88,0 | 1,35× |

**No son 3×, y la razón está en los números**: de esos ~115 ciclos por limbo,
unos **30 son la llamada** y los **~85 restantes son la latencia del propio
`divq`**. La cadena es serial —cada división espera al resto de la anterior— así
que no hay nada que solapar.

De punta a punta sobre `divmod` real, A/B contra `HEAD`:

| N | n=1 | n=2 | n=N/2 | n=N |
|---:|---:|---:|---:|---:|
| 4 | 1,311× | 1,315× | 1,254× | 1,133× |
| 16 | 1,284× | 1,389× | 1,171× | 1,034× |
| 64 | 1,340× | 1,334× | 1,048× | 1,035× |
| 256 | 1,364× | 1,326× | 1,040× | 1,031× |

**1,28×–1,39× en `n=1` y `n=2`, en las siete anchuras, sin una excepción.** Y el
patrón se sostiene solo: en `n=N/2` la ganancia decae con N (1,25× → 1,04×)
porque ahí el bucle de multiplicar-y-restar se come la fracción del `divq`.

### El precio: cambia el modo de fallo, y qué se hizo al respecto

`divq` lanza `#DE` si el cociente no cabe en 64 bits, o sea si `hi ≥ d`. Antes,
violar la precondición daba un **valor equivocado en silencio**; ahora mata el
proceso con `STATUS_INTEGER_OVERFLOW` (0xC0000095 — que, comprobado, describe
exactamente lo que pasó y no miente diciendo «división por cero»).

Para un `detail::` no alcanzable desde fuera, violar la precondición es **un bug
nuestro**, y para un bug propio ruidoso es mejor que callado. Así que no se
restauró el silencio; se subió el listón por dos vías:

1. **La precondición está en el nombre**: `div_128_64_hi_menor_que_d`. Un
   llamante futuro puede no leer el `@pre` del Doxygen; no puede no leer lo que
   teclea.
2. **Y se comprueba a máquina**: con `NSTD_DIV_COMPRUEBA_PRECONDICIONES` la
   función verifica `hi < d` en cada llamada y aborta nombrando la condición.
   `scripts/check_precondiciones_div.py` compila y ejecuta **los 61 ficheros de
   la suite** con la macro encendida: ninguno la rompe. En evaluación constante
   la violación se vuelve **error de compilación**, porque el abortador no es
   `constexpr` a propósito.

> **Y el verificador se valida a sí mismo en cada ejecución.** Antes de mirar los
> 61 ficheros compila una sonda que **rompe la precondición a propósito** y exige
> que aborte diciéndolo; si no salta, se declara no fiable y no ejecuta la suite.
> Un verde que no puede ponerse en rojo es peor que no tener comprobación, y este
> proyecto ya pagó una: el «0 avisos de Doxygen» cuando la comprobación estaba
> apagada.

---

## Trabajo que se hacía dos veces (18 sep 2026)

Dos mejoras del mismo día y de la misma naturaleza: **ninguna añade un
algoritmo**. Las dos quitan trabajo duplicado que nadie había medido.

### `mul_wide` ensanchaba antes de multiplicar

Era, literalmente:

```cpp
return uint_fixed_t<2N, Policy>{a} * uint_fixed_t<2N, Policy>{b};
```

Ensanchar los dos operandos —**con los N limbos altos a cero**— y multiplicar de
forma **modular** 2N × 2N. Y `operator*` no mira los valores: reparte en dos
mitades de N y calcula **los dos términos del medio igualmente**, aunque valgan
cero. Tres productos de N donde hacía falta uno.

`kmul_full_gen<N>` da directamente el producto completo N×N → 2N. Las dos
variantes **entrelazadas en el mismo proceso** —mejor que el A/B entre binarios
que hubo que usar para la división, porque aquí sí pueden coexistir—, 20
repeticiones:

| N | viejo | nuevo | razón | | N | viejo | nuevo | razón |
|---:|---:|---:|---:|---|---:|---:|---:|---:|
| 2 | 74 | 16 | **4,61×** | | 32 | 10 428 | 4 391 | 2,37× |
| 4 | 314 | 176 | 1,79× | | 48 | 23 548 | 8 857 | 2,66× |
| 8 | 1 142 | 501 | 2,28× | | 64 | 38 027 | 15 560 | 2,44× |
| 16 | 3 034 | 1 226 | 2,47× | | 128 | 136 608 | 49 615 | **2,75×** |

**2,3×–2,8× en todo el rango.** La cifra encaja con el mecanismo: el ~3× de
calcular tres productos de N en vez de uno.

> **Una cautela mía que resultó infundada, y por qué.** Antes de medir supuse que
> Karatsuba recuperaría parte del desperdicio, porque la mitad alta es cero.
> **No**: el reparto es **estático** y no mira los valores, así que calcula los
> términos del medio aunque sean nulos. La intuición sobre lo que «el compilador
> seguramente optimiza» falló; la medida no.

**El caso con signo no se salva solo.** El núcleo es sin signo, y multiplicar sin
signo dos representaciones en complemento a dos acierta los limbos **bajos** y
nada más. Hay que restar `B` de la mitad alta cuando `A` es negativo, y `A`
cuando lo es `B` — la misma trampa que Toom-3 al evaluar en x = −1. Sin ella el
resultado **parece correcto mientras los dos operandos son positivos**, que es lo
que da un generador al azar la mayoría de las veces. La sonda fuerza los cuatro
cuadrantes: 24/24 anchuras, 4 800 casos × 2.

**Y la marca pegajosa casi se pierde.** El camino viejo la propagaba *sin
querer* —el constructor de ensanchado la arrastra— y el nuevo construye desde
limbos crudos. Habría sido una regresión silenciosa **justo en la política que
existe para que no las haya**, y la sonda inicial no podía verla porque usa
`wrap`. Se añadió `detail::marca_de_los_factores`, verificado con 27
comprobaciones sobre `checked`.

### `checked_mul` multiplicaba dos veces

El `operator*` con política `checked` hacía esto:

```cpp
const bool desbordo = producto_desborda(*this, o);   // producto #1
fixed_int_t r = this->mul_sin_marca(o);              // producto #2
```

Y `producto_desborda` era un **escolar cuadrático completo** N×N → 2N, sin
Karatsuba ni Toom-3, escrito **sólo para mirar si la mitad alta era cero**.

Los números lo delataban sin leer el código: en N=64, `checked_mul` costaba
**28 304** ciclos y un producto ancho entero cuesta **14 531**. Pagaba dos
productos, y el primero con el peor algoritmo de la casa.

Ahora `producto_desborda` devuelve las dos cosas de **una pasada**, con el
reparto de siempre: la mitad baja es el resultado y la alta dice si desbordó.

| N | `checked_mul` | `saturating_mul` |
|---:|---:|---:|
| 4 | 1,53× | 1,32× |
| 8 | 1,43× | 1,40× |
| 16 | 1,68× | 1,63× |
| 32 | 1,85× | 1,86× |
| 64 | **1,92×** | 1,85× |

**La mejora crece con N** (1,43 → 1,68 → 1,85 → 1,92), y no sólo porque se
multiplique una vez en lugar de dos: **la detección deja de ser cuadrática**.

`saturating_mul` lo hereda entero sin tocarlo — es literalmente
`checked_mul(wa, wb)` y luego saturar según la marca.

> ### Cómo apareció, que es lo que más vale de todo esto
>
> Al acelerar `mul_wide` dejé escrito en el código que la ganancia la heredarían
> `mulhi`, `checked_mul` y `saturating_mul`. **Lo medí después y era falso**:
>
> | N | `mul_wide` | `mulhi` | `checked_mul` | `sat_mul` |
> |---:|---:|---:|---:|---:|
> | 16 | 2,45× | 2,57× | 0,97× | 1,06× |
> | 64 | 2,52× | 2,38× | 0,96× | 0,97× |
>
> Sólo `mulhi`. Y al ir a corregir la frase se vio que `checked_mul` no es que
> heredara poco: **pagaba dos productos**. El 1,9× salió de comprobar una
> afirmación que yo mismo había escrito sin medir.
>
> La cabecera además **ya decía desde antes** que `mul_wide` era «la operación
> sobre la que se construyen `checked_mul()` y la división por constante».
> También falso. Las dos correcciones quedan escritas en su sitio.

### El patrón de los tres días

Cuatro mejoras, y **ninguna vino de añadir un algoritmo**:

| Hallazgo | Qué afirmaba el código | Qué hacía |
|---|---|---|
| `__udivti3` | «emite un solo `divq`» | emitía una **llamada** — 1,28×–1,39× |
| `mul_wide` | (nada; nadie lo había mirado) | ensanchaba para calcular ceros — 2,3×–2,8× |
| `checked_mul` | «se construye sobre `mul_wide`» | multiplicaba **dos veces** — 1,4×–1,9× |
| Toom-3 | «gana 1,23× en N=256» | pierde hasta ~1024 |

**Toom-3 —el único algoritmo nuevo de verdad— fue el que menos dio.** Las otras
tres salieron de comprobar lo que el código decía de sí mismo.

---

## Möller–Granlund 2/1: dividir sin dividir (18 sep 2026)

Primera mitad de P2.10. La división por divisor de un limbo deja de usar `divq` y
usa **dos multiplicaciones con un inverso precalculado** (Möller & Granlund,
*Improved division by invariant integers*, 2011, algoritmos 1 y 4).

**Lo que gana no es «menos operaciones», es menos latencia.** La línea base decía
que `div_un_limbo` costaba ~84 ciclos **por limbo**, y que casi todo era la
latencia del `divq`: la cadena es **serial** —cada división espera al resto de la
anterior— así que no hay nada que solapar. Una multiplicación tiene ~3–5 ciclos
de latencia; una división, ~85.

Las dos variantes entrelazadas, 20 repeticiones, con **la normalización y el
cálculo del inverso contados dentro**:

| N | `divq` | por limbo | MG | por limbo | razón |
|---:|---:|---:|---:|---:|---:|
| 1 | 24 | 23,7 | 103 | 103,0 | **0,23×** |
| 2 | 118 | 59,1 | 130 | 65,1 | **0,91×** |
| 3 | 189 | 62,9 | 132 | 44,0 | 1,43× |
| 4 | 284 | 71,0 | 137 | 34,3 | 2,07× |
| 8 | 626 | 78,2 | 218 | 27,2 | 2,88× |
| 16 | 1 258 | 78,6 | 353 | 22,0 | 3,57× |
| 32 | 2 695 | 84,2 | 604 | 18,9 | 4,46× |
| 64 | 5 296 | 82,8 | 1 126 | 17,6 | 4,71× |
| 128 | 11 034 | 86,2 | 2 222 | 17,4 | **4,97×** |
| 256 | 20 860 | 81,5 | 4 198 | 16,4 | 4,97× |

**El coste por limbo cae de ~84 a ~17 ciclos.** La razón crece con N porque el
coste fijo se amortiza.

### El umbral es el hallazgo, no el 5×

El primer barrido **empezaba en N=4** y daba de 1,90× a 5,05×: un resultado
redondo, sin una sola casilla mala. Integrarlo ahí habría metido una **regresión
de más de 4× en `uint64_fixed_t`**, el tipo más pequeño de la biblioteca.

La causa es concreta: **calcular el inverso cuesta un `divq`**. Con uno o dos
limbos esa división extra no se amortiza, y en N=1 se pagan literalmente **dos
divisiones donde bastaba una**.

De ahí `NSTD_MG_2POR1_MIN = 3`, con la tabla entera —**incluidas las dos casillas
perdedoras**— en su `@def`.

> **Es el error del acantilado otra vez.** Allí fue mirar sólo potencias de dos;
> aquí, empezar el barrido en N=4. En los dos casos el rango cómodo daba un
> resultado limpio y escondía justo la región que dolía. La regla que sale:
> **cuando un barrido sale perfecto, sospechar de dónde empieza.**

### La normalización se cuenta dentro

MG exige el divisor con el bit alto a uno. Si no lo está hay que desplazarlo, y
entonces **el dividendo también**: se recorre arrastrando los bits que cruzan de
un limbo al siguiente, y el resto se desplaza de vuelta al final. Ese trasiego es
parte del coste y está en las cifras; medir la operación suelta habría dado un
número mejor y falso.

### Y un llamante nuevo de la precondición, comprobado

El inverso es `(~d : ~0) / d`, o sea una llamada más a
`div_128_64_hi_menor_que_d`, cuya precondición `hi < d` se cumple **por
construcción**: con `d` normalizado, `~d < d`.

Eso es un razonamiento, y desde el 17 sep los razonamientos sobre esta
precondición se comprueban: `check_precondiciones_div.py` compila y ejecuta los
61 ficheros con la macro encendida, y ninguno la rompe.

**Todo es `constexpr`.** A diferencia de `mul_wide` y `checked_mul`, aquí no hace
falta despachar con `is_constant_evaluated`: MG sólo usa multiplicaciones y
desplazamientos, y las primitivas ya lo eran.

---

## Möller–Granlund 3/2: la estimación de q̂ (18 sep 2026)

El paso D3 de Knuth —estimar el dígito del cociente— es el bucle interno de toda
la división. `div_knuth_d` lo tiene como **parámetro de plantilla**, así que las
tres variantes existen a la vez en un binario y se pueden entrelazar.

**Medido con clang, las tres entrelazadas, 20 repeticiones, mínimo.** `n` son los
limbos significativos del divisor; los dígitos de cociente son `N - n + 1`.
`auto` es lo que usa la biblioteca por omisión.

| N | divisor | dígitos | knuth | MG 3/2 | razón | `auto` | razón |
|---:|:---|---:|---:|---:|---:|---:|---:|
| 2 | n=N | 1 | 59 | 111 | **0,53×** | 61 | 0,97× |
| 3 | n=2 | 2 | 137 | 127 | 1,08× | 136 | 1,00× |
| 3 | n=N | 1 | 67 | 112 | **0,60×** | 64 | 1,06× |
| 4 | n=2 | 3 | 198 | 143 | 1,38× | 149 | 1,33× |
| 4 | n=N | 1 | 65 | 117 | **0,56×** | 70 | 0,93× |
| 5 | n=N | 1 | 92 | 129 | 0,71× | 100 | 0,91× |
| 6 | n=N/2 | 4 | 301 | 178 | 1,69× | 185 | 1,63× |
| 6 | n=N | 1 | 113 | 134 | 0,84× | 117 | 0,96× |
| 7 | n=N | 1 | 106 | 138 | 0,77× | 113 | 0,94× |
| 8 | n=2 | 7 | 511 | 222 | 2,30× | 231 | 2,21× |
| 8 | n=N/2 | 5 | 381 | 221 | 1,72× | 237 | 1,61× |
| 8 | n=N | 1 | 115 | 143 | 0,81× | 120 | 0,96× |
| 16 | n=2 | 15 | 1 114 | 404 | 2,76× | 411 | 2,71× |
| 16 | n=N/2 | 9 | 800 | 451 | 1,77× | 462 | 1,73× |
| 16 | n=N | 1 | 139 | 180 | 0,77× | 141 | 0,98× |
| 32 | n=2 | 31 | 2 299 | 751 | 3,06× | 799 | 2,88× |
| 32 | n=N/2 | 17 | 1 902 | 1 141 | 1,67× | 1 156 | 1,65× |
| 32 | n=N | 1 | 221 | 254 | 0,87× | 225 | 0,98× |
| 64 | n=2 | 63 | 4 770 | 1 585 | 3,01× | 1 493 | **3,19×** |
| 64 | n=N/2 | 33 | 5 116 | 3 517 | 1,45× | 3 455 | 1,48× |
| 64 | n=N | 1 | 435 | 466 | 0,93× | 438 | 0,99× |
| 128 | n=2 | 127 | 9 586 | 2 878 | **3,33×** | 3 048 | 3,14× |
| 128 | n=N/2 | 65 | 16 479 | 12 408 | 1,33× | 12 416 | 1,33× |
| 128 | n=N | 1 | 667 | 683 | 0,98× | 677 | 0,99× |

`benchs/benchmark_estimador.cpp`.

### La columna que no estaba, y que lo cambiaba todo

Si esta tabla sólo cruzara `n = 2` y `n = N/2` —que es como se midió la primera
vez— iría de **1,33× a 3,33× sin una sola casilla mala**, y la 3/2 pura se habría
integrado tal cual.

La columna que lo desmiente es **`n = N`**: el divisor de la anchura entera. Ahí
hay **un solo dígito de cociente**, el inverso no tiene sobre qué amortizarse, y
la 3/2 pura **pierde hasta el 47%**. Y no es un caso de laboratorio: es lo que dan
dos operandos aleatorios de la misma anchura, o sea **el caso más frecuente de
todos**.

Es la tercera vez que un barrido cómodo tapa el borde. Va al lado del acantilado
de Karatsuba y del `N=1` de la 2/1.

### El umbral se mide en dígitos, no en anchura

El inverso se paga **una vez por llamada** y ahorra **por dígito**. Lo que decide
no es `N`:

| dígitos | N=16 knuth | MG | razón | N=64 knuth | MG | razón |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 140 | 180 | **0,78×** | 434 | 476 | **0,91×** |
| 2 | 214 | 208 | 1,03× | 576 | 588 | 0,98× |
| 3 | 339 | 264 | 1,28× | 833 | 753 | 1,11× |
| 4 | 419 | 302 | 1,39× | 1 078 | 955 | 1,13× |
| 5 | 522 | 354 | 1,48× | 1 289 | 1 118 | 1,15× |
| 7–9 | 658 | 409 | 1,61× | 2 184 | 1 709 | 1,28× |

`NSTD_MG_3POR2_MIN` está en **3** y no en 2 porque la casilla de dos dígitos
**empata dentro del ruido**: 0,96×, 0,98×, 1,02× y 1,03× en cuatro tandas. Un
cambio que no se distingue del ruido no se integra.

### La decisión va dentro del estimador, y eso también se midió

`n` no se conoce hasta ejecución, así que la elección no puede ser un parámetro
de plantilla. Hay dos formas de resolverlo, y se midieron las dos:

| | velocidad | código objeto (N = 2..64) |
|---|---|---|
| **híbrido**: una rama por dígito, un solo bucle | referencia | 202 786 B (**1,01×**) |
| **rama fuera**: dos bucles, cero ramas | empata dentro del ruido | 339 473 B (**1,69×**) |

La rama la predice el procesador siempre bien, porque no cambia dentro de una
llamada. Como no gana un ciclo y cuesta **un 69% más de código** en una
biblioteca que se instancia por cada `N` del cliente, va el híbrido.

> La primera medida de «rama fuera» salía peor de lo que debía, y el sesgo era
> mío: el envoltorio llamaba a `limbos_significativos` **dos veces**, o sea un
> recorrido de N limbos de más, justo en el eje donde se la quería comparar. La
> forma del error lo delató (0,99× en N=2 y 0,88× en N=64, creciendo con N). Se
> arregló antes de decidir.

Y donde la anchura es tan pequeña que MG **no podría usarse nunca** —con `n ≥ 2`
siempre, los dígitos no pasan de `N - 1`— el `if constexpr` de
`estimador_auto<N>` lo quita en compilación: para `N ≤ 3` el tipo queda vacío y la
división corta se queda exactamente como estaba, sin rama ni estado.

Queda un residuo de **3–9% en `N = 4..7` con un solo dígito**: es el precio del
despacho en ejecución, y se paga a cambio de 1,3×–3,2× en el resto.

### Corrección: 37 776 casos, y por qué los aleatorios no bastaban

Los tres estimadores comparados contra `estimador_knuth` en clang y gcc:
aleatorios de `N = 2..40` y **esquinas** de `N = 2..12`. Cero fallos.

Las esquinas no son adorno. Con **9 300 casos aleatorios la 3/2 pasó limpia**; el
primer barrido de esquinas la tumbó en **9 anchuras de 11**. El fallo era real:

- La 3/2 exige `(u0,u1) < (v1,v2)`, y **Knuth D no lo garantiza**. Su invariante
  es sobre la ventana entera de `n+1` limbos, y los dos limbos altos pueden
  empatar si los de abajo compensan.
- Cuando eso pasa, el dígito es `B-1` **exactamente**, no estimado: con `v1`
  normalizado sale `U/V > B - 2/B`, y el invariante da `q ≤ B-1`. Es la misma
  rama que lleva GMP en `mpn_sbpi1_div_qr`.

Con entrada uniforme esa rama tiene probabilidad del orden de `2⁻⁶⁴`. Ningún
número razonable de aleatorios la toca. Lo que la encontró fue construir los
limbos del divisor de `{0, 1, 2, 2⁶³±1, 2⁶³, 2⁶⁴-2, 2⁶⁴-1}` y cruzarlos con seis
formas del dividendo (`a = b`, `a = b - 1`, todo unos, pegado al techo…).

---

## El tope de desenrollado: barrido con dispersión

**Medido el 10 September 2026** con `benchmark_barrido_desenrollado`, el primer
banco de este proyecto que cumple el protocolo entero: las dos variantes
**entrelazadas con el orden rotando**, **diez repeticiones** por casilla,
iteraciones calibradas a 200 ms, y **se publica la dispersión**. Una razón que no
supere la suma de los recorridos de sus dos casillas se marca `(ruido)` y no
cuenta.

Lo permite `include/algorithms/mul_kernels.hpp`, que saca los núcleos a
funciones libres: hasta entonces sólo había una variante por binario y las
rondas entrelazadas eran imposibles.

### El desenrollado tiene TRES regiones, no dos

Se salió a buscar un punto de rendimientos decrecientes y hay un **cruce**:

| N | clang | gcc |
|---|---|---|
| 1–2 | 0,94–0,99× — **no aporta nada** | ídem |
| 3–29 | 1,36×–2,41× | 1,74×–3,27× |
| 30–47 | se desvanece en el ruido | sigue ganando hasta 38 |
| 48–96 | **pierde**, hasta 0,53× | **pierde** desde ~60, hasta 0,60× |

En N=96 con clang, **el bucle es casi el doble de rápido que el desenrollado**.
Es coherente con que el desenrollado sea O(N²) en *tamaño de código*: pasado
cierto punto deja de caber en la caché de instrucciones, y el bucle, que es
diminuto, gana.

### Los CUATRO compiladores, y no coinciden

| | MSVC | clang | Intel | gcc |
|---|---|---|---|---|
| Ganancia en N=24 | 1,63× | 1,69× | 2,12× | **2,76×** |
| Última ganancia **significativa** | **N=26** | N=32 | N=36 | **N=38** |
| Cruce (razón = 1) | N≈33 | N≈44–48 | fuera de rango | N≈56–60 |
| Anchuras donde **pierde** | 5 | 7 | **0** | 6 |
| Peor pérdida | 0,86× (N=34) | 0,53× (N=96) | — | 0,60× (N=96) |

El desenrollado de Intel es el mejor de los cuatro: **no pierde en ninguna
anchura del barrido**. El de MSVC es el peor, y cruza casi veinte limbos antes
que el de GCC.

La diferencia en N=24 va de 1,63× a 2,76× — **1,7× entre compiladores**, más que
muchas de las decisiones que se toman mirando una sola cifra.

### Por qué esto justifica medir los cuatro, y no dos

Con clang y GCC —los dos que la sesión iba a usar— el tope habría salido **32**.
Con MSVC en la mesa, no: su razón en N=32 es **1,02×**, plenamente dentro del
ruido. **La decisión con dos de cuatro habría sido la equivocada**, y es
exactamente el escenario contra el que avisaba
[PLAN_SESION_MEDICION](PLAN_SESION_MEDICION.md).

### Un matiz que hay que leer bien: «no significativo» no es «pierde»

MSVC entre N=27 y N=32 da 1,21× · 1,25× · 1,22× · 1,04× · 1,11× · 1,02×.
Ninguna supera su propio ruido, así que ninguna cuenta por separado. Pero **las
seis están por encima de 1**, y seis de seis del mismo lado no es casualidad:
como signo, es evidencia aunque cada celda no lo sea.

Lo mismo pasa con clang entre 33 y 40. El patrón real no es «gana hasta X y
luego pierde», sino **una caída suave que cruza el 1 en un sitio distinto para
cada compilador**: MSVC hacia 33, clang hacia 46, GCC hacia 58, e Intel más allá
del final del barrido.

Por eso el criterio conservador —la última anchura donde **los cuatro** ganan de
forma demostrable— da **26**, y el criterio permisivo —donde ninguno pierde
todavía— daría 32.

### El tercer coste del desenrollado: rompe MSVC

No es lentitud ni tamaño. Es un límite duro del formato COFF:

```
fatal error C1128: el numero de secciones supero el limite de
formato de archivo objeto: compile con /bigobj
```

La unidad del barrido, con 54 anchuras desenrolladas instanciadas, **no compila
con MSVC** sin `/bigobj`. GCC y clang no tienen ese límite.

El proyecto no pasaba esa bandera; **ahora sí** (`scripts/build_generic.py`, para
MSVC e Intel-Windows). No cuesta nada —sólo permite más secciones— y sin ella el
fallo aparecería como un error incomprensible en el código de quien usa la
biblioteca, no aquí.

Pesa en la decisión del tope: subirlo acerca ese muro a todo el que instancie
varias anchuras.

### Lo que dicen los cuatro sobre el tope

`NSTD_DESENROLLA_MAX` está en **20**, y el barrido dice que **se está dejando
ganancia sobre la mesa**: entre 21 y 26 los cuatro compiladores ganan de forma
demostrable.

| Candidato | Argumento a favor | Argumento en contra |
|---|---|---|
| **26** | La última anchura donde **los cuatro** ganan de forma demostrable. En N=26: MSVC 1,30×, clang 1,51×, Intel 1,87×, GCC 2,44× | Deja sin aprovechar 27..32, donde tres de los cuatro siguen ganando claramente |
| **32** | Nadie pierde todavía (MSVC 1,02×, clang 1,22×, Intel 1,73×, GCC 1,52×), y tres ganan de forma clara | La ganancia de MSVC ahí no es demostrable, y está a un paso de su cruce (~33). Más secciones de objeto, con el muro de `/bigobj` más cerca |
| 20 (hoy) | Nada que cambiar | Deja ganancia medida sobre la mesa en 21..26, **en los cuatro** |

> **Una advertencia sobre generalizar**: esto es **una máquina**. Los umbrales de
> este tipo varían con la CPU — GMP publica un rango de 16 a 46 limbos para su
> umbral de Karatsuba entre modelos. Un tope elegido aquí es un default
> razonable, no una verdad.

### Y el otro coste, que no es de velocidad

Subir el tope cuesta **tiempo de compilación y tamaño de binario**, y sólo lo
paga quien instancia esas anchuras. Medido sobre la unidad del barrido, con las
54 anchuras instanciadas:

| | clang | gcc |
|---|---:|---:|
| Compilar | 215 s | 266 s |
| Binario | 10,0 MB | 7,3 MB |

Es un caso extremo a propósito —nadie instancia 54 anchuras— pero acota el
techo: el desenrollado no es gratis ni en tiempo de máquina ni en el de quien
espera a que compile.

### La dispersión, publicada por primera vez

Se venía diciendo «~35 % de ruido» a partir de dos ejecuciones, que no es una
medida de nada. Lo real, con diez repeticiones por casilla:

- **Dentro de una ejecución**, el recorrido `(max−min)/min` cae entre el **4 % y
  el 50 %**, según la casilla.
- **El mínimo entre ejecuciones distintas** es mucho más estable: **5–12 %**.
- **Las razones entre dos variantes**, más todavía: 1,78 / 1,77 / 1,69 en tres
  pasadas del mismo caso.

De ahí la regla: **el mínimo es el estadístico que se publica, las razones son
fiables, y las cifras absolutas no valen a más de dos dígitos.**

---

## La curva: coste de cada operación frente a la anchura N

**Medido el 9 September 2026**, GCC 16.2.0 (MSYS2 UCRT64) −O2, Windows 11 sobre
x86-64, MSI. 64 operandos aleatorios, 50 000 iteraciones × 5 rondas, mínimo por
caso. `benchmark_curva_n`.

Los demás benchmarks miden anchuras sueltas —N=2, N=4, N=8— y con eso se ve un
punto, no una curva: no se ve **dónde** cambia el comportamiento. Aquí se barre
N de 1 a 64 y se publica el **coste por limbo**, que es lo que se lee: una
operación lineal sale plana y una cuadrática sale creciendo.

| N | add | por limbo | mul | por limbo | div | por limbo | camino de `mul` |
|---:|---:|---:|---:|---:|---:|---:|---|
| 1 | 1,3 | 1,35 | 2,0 | 2,02 | 14 | 14,0 | — |
| 2 | 2,3 | 1,14 | 3,8 | 1,89 | 95 | 47,4 | 128 bits |
| 3 | 15,2 | 5,05 | 21,8 | 7,26 | 230 | 76,8 | desenrollado |
| 4 | 23,1 | 5,76 | 35,7 | 8,93 | 342 | 85,6 | desenrollado |
| 8 | 31,1 | 3,88 | 137 | 17,1 | 607 | 75,9 | desenrollado |
| 16 | 38,6 | 2,41 | 408 | 25,5 | 1088 | 68,0 | desenrollado |
| 20 | 56,4 | 2,82 | 623 | **31,2** | 1448 | 72,4 | desenrollado |
| **24** | 55,8 | 2,32 | **2925** | **121,9** | 1801 | 75,0 | **bucle** |
| **32** | 85,3 | 2,66 | **2897** | **90,5** | 2485 | 77,7 | **Karatsuba** |
| **48** | 132 | 2,75 | **12259** | **255,4** | 4520 | 94,2 | **bucle** |
| 64 | 200 | 3,12 | 14419 | 225,3 | 6921 | 108,2 | Karatsuba |

### El acantilado, que solo se ve mirando la curva entera

**Multiplicar 1536 bits (N=24) cuesta más que multiplicar 2048 (N=32)**: 2925
frente a 2897 cyc/op. Un número más grande se calcula más rápido.

La causa es el reparto: `operator*` desenrolla hasta N=20 y usa Karatsuba en las
potencias de dos desde 32. **Todo lo que queda entre medias —y toda anchura
mayor que 20 que no sea potencia de dos— cae al bucle escolar**, que cuesta
entre 3× y 4× por limbo lo que cualquiera de los otros dos caminos:

| N | por limbo | camino |
|---:|---:|---|
| 20 | 31,2 | desenrollado |
| 24 | 121,9 | bucle ← **3,9×** |
| 32 | 90,5 | Karatsuba |
| 48 | 255,4 | bucle ← **2,8×** frente a N=64 |
| 64 | 225,3 | Karatsuba |

Ya se había visto la punta de esto con N=20, cuando el tope de desenrollado
estaba en 16. Resultó ser más ancho: no son cuatro anchuras raras, es **la mitad
del rango por encima de 20**.

Ya no está abierto: el diseño está escrito en
[PLAN_MULTIPLICACION](PLAN_MULTIPLICACION.md), y el estado del arte con el que se
contrasta, en [ESTUDIO_ALGORITMOS_RAPIDOS](ESTUDIO_ALGORITMOS_RAPIDOS.md), y va por dos pasos —subir el
tope de desenrollado a 31, y Karatsuba con reparto **equilibrado** para toda N
en vez de sólo potencias de dos—.

> **Dos medidas de lo mismo que no coinciden, y sigue sin resolverse.**
>
> Del desenrollado frente al bucle en **N=24** hay dos cifras, tomadas con
> cuatro días de diferencia y en la misma máquina:
>
> | Cuándo | Cómo | Razón |
> |---|---|---:|
> | 6 sep 2026 | barrido de `NSTD_DESENROLLA_MAX`, anotado en el bloque `@def` de la macro en `fixed_width_int_t.hpp` | **1,11×** |
> | 10 sep 2026 | `benchmark_curva_n` compilado dos veces, con `NSTD_DESENROLLA_MAX=20` y `=32` | **2,44×** (3035 → 1245 cyc/op) |
>
> Difieren **2,2×**, muy por encima del ruido de este banco (~35 % entre
> ejecuciones). Una de las dos está mal y **no se sabe cuál**.
>
> **Corrección de una corrección.** El 10 sep se escribió aquí que el 1,11×
> «estaba mal atribuido», que salía de la fila N=12 de la tabla del barrido de
> paridad. **Eso era falso**: el 1,11× es una medida real de N=24, y está en el
> header con su fecha. El error fue mío al no abrir el bloque `@def` de la
> macro antes de afirmar que la cifra no existía — exactamente el fallo que
> este proyecto lleva toda la semana persiguiendo, y esta vez cometido al
> corregirlo.
>
> Hipótesis para la diferencia, ninguna comprobada: bancos distintos
> (`benchmark_karatsuba` frente a `benchmark_curva_n`), operandos distintos, o
> **la posición dentro de la ejecución**, que en este proyecto ya se ha visto
> que cambia la medida. Resolverlo es el punto de partida de la sesión de
> medición: ver [PLAN_SESION_MEDICION](PLAN_SESION_MEDICION.md).
>
> Mientras no se resuelva, **el 2,44× no se da por bueno** y el tope de
> desenrollado no se sube.

### Lo que confirma

- **`add`, `sub` y `shl` son lineales**: por limbo se quedan entre 2,3 y 3,9
  desde N=8 en adelante. El pico de N=3 y N=4 (≈5,7) es el precio de salir del
  camino especializado de 128 bits, no un problema de escala.
- **`cmp` es O(1) en la práctica**: ~4 cyc/op sea cual sea N, porque sale por el
  limbo alto en cuanto los operandos difieren. Su columna «por limbo» baja, y
  eso es correcto, no un fallo.
- **`div` por limbo va de 68 a 108** entre N=16 y N=64: crece, pero mucho menos
  que `mul`. Knuth D aguanta bien.

## Conversión a y desde cadena — las bases 2..36

**Medido el 9 September 2026**, GCC 16.2.0 (MSYS2 UCRT64) −O2, Windows 11 sobre
x86-64, MSI. 128 operandos aleatorios, 20 000 iteraciones × 5 rondas, mínimo por
caso. `benchmark_bases`.

`to_string(base)` y `from_string(s, base)` aceptan 2..36 desde v1.90.1 y **nunca
se habían medido**: los dos benchmarks que había cubren la 10 y la 16, que son
justo las dos que el código trata de forma especial.

| base | `to_string` N=2 | N=4 | `from_string` N=2 | N=4 | |
|---|---:|---:|---:|---:|---|
| 2 | 1323 | 3427 | 1025 | 7736 | potencia de dos |
| 3 | **3433** | **7108** | 741 | 5517 | la más cara |
| 4 | 944 | 2100 | 655 | 4613 | potencia de dos |
| 8 | 623 | 1623 | 497 | 3171 | potencia de dos |
| **10** | **358** | **1231** | 458 | 2753 | **la más barata** |
| 16 | 855 | 1806 | 760 | 2787 | potencia de dos |
| 32 | 530 | 1338 | 524 | 2040 | potencia de dos |
| 36 | 1193 | 2676 | 483 | 2152 | |

### Lo que no cuadra, dicho antes de explicarlo

Lo esperado era: las potencias de dos claramente más baratas —convertir es partir
en grupos de bits, sin dividir— y el resto parecidas entre sí, bajando despacio
según crece la base porque salen menos dígitos.

**No es lo que sale**, en dos puntos:

1. **La base 10 gana a todas las potencias de dos**, incluida la 8 (358 frente a
   623) y la 32 (358 frente a 530). Es la única base no potencia de dos que tiene
   implementación propia: `to_string(10)` desvía a `to_string()` sin parámetro.
2. **La base 3 cuesta 9,6× lo que la 10** (3433 frente a 358), y el camino
   general ya trocea: divide por `base^k` con `k` el mayor exponente que cabe en
   64 bits, así que hace ~2 divisiones grandes en los dos casos.

**Conjetura, y va marcada como tal:** el camino general llama a
`uint_fixed_t<N>::divmod` con un divisor que es un `fixed_int_t` con un solo
limbo distinto de cero, y es posible que ahí no haya camino rápido de un limbo y
se esté pagando Knuth D completo por cada trozo; la implementación dedicada de la
base 10 no pasa por ahí. **No está comprobado.** Para comprobarlo hay que mirar
si `divmod` detecta el divisor de un limbo, que es media hora de trabajo y no se
ha hecho.

Si la conjetura se confirma, las 33 bases que van por el camino general podrían
acercarse al coste de la base 10 — un factor de entre 2× y 9× en una API pública.
Queda anotado como tarea, no como afirmación.

### Lo que sí se puede afirmar

- `from_string` es mucho más plano que `to_string`: entre 458 y 1025 para N=2,
  sin el pico de la base 3. Leer no divide; escribir sí.
- El coste crece con la anchura más o menos como se espera: de N=2 a N=4,
  `to_string` se multiplica por entre 2,1× y 2,6×.
- La base 2 es la más cara de las potencias de dos, y es lo razonable: produce
  127 dígitos frente a los 32 de la base 16.

---

## Tiempo de compilación

**Medido el 24 August 2026**, GCC 16.2.0 −O2, la misma máquina.

| | antes | después |
|---|---:|---:|
| Un TU que incluya `int128_param_divmod.hpp` | 2,4 s | **0,95 s** |
| Suite completa (55 ficheros) | ~230 s | **~170 s** |

La causa fue eliminar `GM_TABLE`, una tabla `constexpr` de 1024 entradas que
**no usaba nadie** y gastaba unos 2 millones de pasos de evaluación en
compilación — el doble del límite por defecto de Clang, lo que además obligaba a
compilar toda la biblioteca con `-fconstexpr-steps=100000000`.

---

## Qué falta medir

- **Calibrar el umbral de aviso con más de dos ejecuciones.** El 25 % de hoy
  sale de dos, que es lo mínimo para tener un rango y muy poco para fiarse.
- `rt_mulhi_128`: el 1,8–2,2× sigue sin re-medir porque no tiene benchmark
  propio. Es la única cifra de este documento sin fecha.
- Las medidas de división y de cadenas son solo de **GCC**. Con
  `clang-libstdcxx` ya disponible son cinco configuraciones posibles, y ninguna
  se ha cruzado.
- **Comprobar la conjetura de la base 3**: si `divmod` no tiene camino rápido
  para divisores de un limbo, arreglarlo abarataría 33 de las 35 bases.
- Coste de la política de desbordamiento cuando se implemente
  ([ADR-008](decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md) dice
  que el camino `wrap` no debería cambiar, pero **hay que medirlo, no
  suponerlo**).
