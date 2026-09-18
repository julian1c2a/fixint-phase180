# ADR-016: Burnikel–Ziegler se aparca, y el motivo es una medida

**Estado:** ✅ Aceptado
**Fecha:** 18 September 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

`docs/ESTUDIO_ALGORITMOS_RAPIDOS.md` situaba **Burnikel–Ziegler en el puesto 3**
de las ocho mejoras candidatas, por delante de Toom-3, con esta nota:

> «Θ(N²) → Θ(M(N)·log N). En N=1024, del orden de **5×**. Umbral esperado ~45–50
> limbos.»

Ese umbral venía de `DC_DIV_QR_THRESHOLD` de GMP, que la documentación de GMP
sitúa «algo por encima del doble de `MUL_TOOM22_THRESHOLD`».

Nunca se comprobó si ese número se traslada a esta biblioteca.

## La medida

Los algoritmos rápidos de división —Burnikel–Ziegler, Newton/Barrett— **convierten
la división en multiplicaciones**. Su techo es el que publica GMP:

> *«a 2N×N division is about 2 to 4 times slower than an N×N multiplication»*

Así que hay una sola pregunta que decide si merece la pena escribirlos: **¿cuánto
cuesta hoy una división `(N, n=N/2)` comparada con una multiplicación de
`N/2 × N/2`?** Si ya está dentro de 2–4×, no hay diferencia que recoger.

Se mide `n = N/2` porque es donde la superficie de la división tiene su máximo:
el coste de Knuth D es `O((N-n)·n)`, que se maximiza ahí.

**Medido el 18 sep 2026 con clang**, las dos entrelazadas en un proceso, 20
repeticiones, mínimo (`benchs/benchmark_techo_division.cpp`):

| N | N/2 | división | multiplicación | razón |
|---:|---:|---:|---:|---:|
| 8 | 4 | 357 | 99 | 3,6× |
| 16 | 8 | 770 | 274 | 2,8× |
| 32 | 16 | 2 081 | 1 112 | 1,9× |
| 64 | 32 | 6 139 | 3 745 | **1,6×** |
| 128 | 64 | 22 783 | 13 287 | 1,7× |
| 256 | 128 | 86 671 | 44 662 | 1,9× |
| 512 | 256 | 281 862 | 128 114 | 2,2× |
| 1 024 | 512 | 1 234 470 | 438 939 | 2,8× |
| 2 048 | 1 024 | 4 359 190 | 892 486 | **4,9×** |

La razón tiene un **mínimo en N=64** y crece a los dos lados. **No pasa del techo
hasta N=2048.**

Lo que pasa en 2048 es concreto: ahí la multiplicación entra en **Toom-3**
(`NSTD_TOOM3_MIN` = 1024, y el factor mide `N/2`), su exponente baja, y la
división se queda sin nada con que seguirla.

## Decisión

**Burnikel–Ziegler no se escribe por ahora.** P2.11 queda aparcado, documentado
con esta medida, y el trabajo pasa al camino crítico de la 2.0 (P1.5 tramos
2d/2e/3 y P1.6), que es lo que [ADR-007](ADR-007-politica-de-desbordamiento-como-parametro.md)
marca como bloqueante.

Los dos criterios del proyecto lo dicen igual:

- **¿Bloquea?** B-Z no bloquea nada. P1.5 y P1.6 sí: son el camino crítico, y
  hacerlos al revés significa portar la API dos veces.
- **¿Rinde?** En el rango donde la biblioteca se usa (N ≤ 512 limbos = 32 768
  bits), la división **ya está dentro del techo publicado**. Escribir B-Z ahí
  sería pagar su sobrecoste sin recoger ninguna diferencia de exponente.

Se reabre si cambia alguna de las dos cosas que lo sostienen:

1. **Que la multiplicación mejore.** Su exponente medido es **1,68**; el teórico
   de Karatsuba es 1,585. Cada punto que baje ahí sube la razón div/mul y acerca
   el umbral de B-Z.
2. **Que el uso real suba de N=1024.** Hoy no hay constancia de que lo haga.

## Consecuencias

- `ESTUDIO_ALGORITMOS_RAPIDOS.md` deja de prometer «umbral ~45–50 limbos» y
  lleva la tabla medida.
- El banco queda en el repo (`benchmark_techo_division`), así que la decisión se
  puede **volver a comprobar** cuando la multiplicación cambie, en vez de
  reabrirse por intuición.
- Newton/Barrett hereda el mismo argumento: está **por encima** de B-Z en la
  escalera, así que si B-Z no tiene sitio, Newton tampoco. Lo que sí puede
  tenerlo por otro motivo es **Barrett para módulo repetido**, que no compite con
  la división general sino con hacer N divisiones seguidas con el mismo divisor.

## La regla que sale

**Un umbral publicado es un umbral relativo a la biblioteca que lo publicó.**

GMP no se equivoca al poner `DC_DIV_QR_THRESHOLD` en ~45 limbos: lo mide contra
**su** `mpn_mul`, con ensamblador afinado y Toom-4. Copiar el número sin copiar
el denominador es comparar contra una multiplicación que aquí no existe.

Es el segundo umbral heredado que se cae al medirlo. El primero fue Toom-3: el
estudio prometía 1,23× en N=256 y lo medido fue **0,87×**, con el cruce real en
~1024.
