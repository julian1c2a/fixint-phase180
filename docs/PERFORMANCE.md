# Rendimiento

**Última actualización:** 25 August 2026

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

| N | bits | biblioteca (cyc/op) | escolar (cyc/op) | razón |
|---|---:|---:|---:|---:|
| **4** | 256 | 28,6 - 30,5 | 50,4 | **1,65x - 1,75x** |
| **8** | 512 | 158,4 - 164,4 | 243,0 | **1,48x - 1,53x** |
| 2 | 128 | 2,9 | 10,4 | 3,57x *(camino especializado, no Karatsuba)* |

Los dos números por celda son las dos ejecuciones; la dispersión entre
ellas da la idea del error de medida.

### Los controles, y lo que destapó uno de ellos

El benchmark mide dos casos que **no** usan Karatsuba, donde la razón tiene
que salir 1,00x. Sirven para comprobar que la implementación de referencia es
fiel; si fallan, ninguna cifra de arriba vale.

| Control | Razón | |
|---|---:|---|
| N=16 (1024 bits) | **1,02x** | OK, la referencia es fiel |
| N=3 (192 bits) | **0,86x - 0,87x** | falla, estable en dos ejecuciones |

**El control de N=3 falla, y no es ruido:** sale igual en las dos ejecuciones.
Significa que el bucle escolar de la biblioteca es un **14 % más lento que
una copia idéntica suya escrita como función libre**, con las mismas
primitivas y el mismo compilador. Es un hallazgo sobre la biblioteca, no sobre
el benchmark, y queda anotado como deuda: hay que mirar la generación de
código de `operator*` para N pequeño e impar.

Para las cifras de Karatsuba ese sesgo juega **a favor de la prudencia**: se
está comparando el camino de Karatsuba contra un escolar *más rápido*
que el de la propia biblioteca, así que 1,65x y 1,48x son cotas
conservadoras.

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
- **Por qué el bucle escolar de `operator*` es un 14 % más lento que una copia
  idéntica suya escrita como función libre**, en N=3. Lo destapó el control
  del benchmark de Karatsuba.
- Karatsuba en Clang, MSVC e Intel: las cifras de arriba son solo de GCC.
- Coste de las conversiones a y desde cadena, ahora que hay bases 2..36.
- Coste de la política de desbordamiento cuando se implemente
  ([ADR-008](decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md) dice
  que el camino `wrap` no debería cambiar, pero **hay que medirlo, no
  suponerlo**).
