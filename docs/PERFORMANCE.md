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

*Medido con GCC −O2. Pendiente de re-medir con fecha y máquina; cifras heredadas
de la fase 1.75.*

| Caso | Binaria (ns) | Knuth D (ns) | Mejora |
|---|---:|---:|---:|
| Potencia de 2 | ~7,0 | ~0,6 | 12× |
| Valores de 64 bits | ~7,0 | ~1,0 | 7× |
| Híbrido 128/64 | ~7,0 | ~1,1 | 6,4× |
| 128/128 grande | ~7,0 | ~1,2 | 5,8× |
| **Media** | **7,17** | **1,15** | **6,24×** |

## División — frente a tipos built-in

*GCC −O2, tiempo de división relativo a `uint64_t`. Mismo origen que la tabla
anterior.*

| Tipo | Tiempo relativo |
|---|---:|
| `nstd::uint128_t` | **0,47×** (más rápido que el nativo) |
| `unsigned __int128` | 9,56× |
| Boost `cpp_int` | ~50× |

El 0,47× no es magia: el camino rápido de `nstd::uint128_t` para divisores de un
limbo evita la llamada a `__udivti3` que emite el compilador para
`unsigned __int128`.

## División por constante — Granlund-Montgomery

`div<D>()`, `mod<D>()`, `divmod_const<D>()`: **4–7× más rápido que Knuth D**
cuando el divisor se conoce en compilación. Sigue siendo de `int128_param_t`;
portarlo a `fixed_int_t` es parte de [ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md).

`rt_mulhi_128`: 4 × MUL nativo en GCC/Clang/Intel frente a 16 × MUL de 32 bits —
**1,8–2,2× más rápido**.

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
**Karatsuba gana con holgura, entre 1,5× y 2×**, que es una conclusión más
pobre y verdadera.

Los valores absolutos en ciclos por operación están en el histórico
(`benchs/history/`), no aquí: se mueven demasiado entre ejecuciones para
publicarlos como si fueran una propiedad del código.

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

- Re-medir las tablas de Knuth D y de comparación con built-in **con fecha,
  compilador y máquina**, que es lo que exige la regla de arriba.
- **Controlar el orden dentro de la ejecución**, que hoy afecta a la medida: el
  mismo caso da 0,86× o 1,18× según en qué posición se mida.
- **Calibrar el umbral de aviso con más de dos ejecuciones.** El 25 % de hoy
  sale de dos, que es lo mínimo para tener un rango y muy poco para fiarse.
- Karatsuba en Clang, MSVC e Intel: las cifras de arriba son solo de GCC.
- Coste de las conversiones a y desde cadena, ahora que hay bases 2..36.
- Coste de la política de desbordamiento cuando se implemente
  ([ADR-008](decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md) dice
  que el camino `wrap` no debería cambiar, pero **hay que medirlo, no
  suponerlo**).
