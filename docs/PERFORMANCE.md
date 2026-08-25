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
- Multiplicación: no hay cifras de Karatsuba frente a schoolbook para N=4 y N=8,
  pese a ser una de las optimizaciones destacadas de la v1.90.
- Coste de las conversiones a y desde cadena, ahora que hay bases 2..36.
- Coste de la política de desbordamiento cuando se implemente
  ([ADR-008](decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md) dice
  que el camino `wrap` no debería cambiar, pero **hay que medirlo, no
  suponerlo**).
