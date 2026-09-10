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
