# Plan de la sesión de medición

**Fecha:** 10 September 2026 · **Alcance:** medir lo que ya existe, sin escribir
algoritmos nuevos.

> El objetivo es un **conjunto de medidas sólido** de los caminos actuales de
> `operator*` y `operator/`, y de sus umbrales, con clang como compilador
> principal y GCC como contraste. Todo lo demás —Karatsuba equilibrado, Toom-3,
> Möller–Granlund, Burnikel–Ziegler— se escribe *después*, y esta sesión deja
> montado el arnés que esas medidas van a necesitar.

---

## 0. Punto de partida: dos medidas de lo mismo que no coinciden

**Esto va primero, antes que cualquier rejilla nueva.** Del desenrollado frente
al bucle en N=24 hay dos cifras tomadas con cuatro días de diferencia en la
misma máquina:

| Cuándo | Cómo | Razón |
|---|---|---:|
| 6 sep 2026 | barrido de `NSTD_DESENROLLA_MAX`, anotado en el bloque `@def` de la macro | **1,11×** |
| 10 sep 2026 | `benchmark_curva_n` compilado con `=20` y con `=32` | **2,44×** |

Difieren **2,2×**, muy por encima del ruido del banco (~35 %). Una de las dos
está mal.

> El 10 sep se escribió en `PERFORMANCE.md` que el 1,11× «estaba mal atribuido».
> **Era falso**: es una medida real de N=24, fechada en el header. El fallo fue
> no abrir el bloque `@def` de la macro antes de negar que existiera — el mismo
> patrón que esta semana ya había aparecido tres veces, y esta vez cometido al
> corregirlo.

**Mientras no se resuelva, el tope de desenrollado no se sube.** Hipótesis a
descartar, en este orden:

1. **Bancos distintos.** ¿Con qué se midió el 6 sep? Reproducir con ese mismo
   banco antes de comparar.
2. **Posición dentro de la ejecución.** En este proyecto ya se comprobó que la
   posición cambia la medida; de ahí la regla de un caso por proceso y orden
   aleatorio.
3. **Operandos distintos.** Densidad de bits y localidad en caché.

---

## 1. Lo que hay que construir antes de medir nada

### Iteraciones adaptativas

El protocolo de hoy fija 50 000 iteraciones × 5 rondas. Extrapolando del `mul`
medido en N=64 con exponente 1,585, a 3 GHz:

| N | una casilla con el protocolo de hoy |
|---:|---|
| 256 | 13 s |
| 512 | 38 s |
| 1024 | 2 min |
| 4096 | **17 min** |

Y una casilla es **una** operación, **una** anchura, **un** algoritmo, **un**
compilador. La rejilla completa se iría a días.

**Cambio:** fijar **tiempo por casilla** (~200 ms) y deducir las iteraciones con
una pasada de calibración. Con eso, una casilla cuesta lo mismo en N=8 que en
N=1024, y la rejilla entera cabe en minutos.

Es la pieza que todas las sesiones de medición posteriores van a reutilizar, así
que se escribe una vez y bien.

### Guarda de verosimilitud, ya existente

`benchmark_karatsuba` y `benchmark_curva_n` ya tienen el suelo de 0,35
ciclos/producto y el parámetro `toca_todos_los_limbos`. El arnés nuevo lo hereda:
una medida por debajo del suelo es un error de medición, no un récord.

---

## 2. La rejilla

**Compilador principal: clang.** GCC como contraste **sólo en los puntos donde
se decida un umbral**.

> **La salvedad que hay que tener presente.** Los umbrales de este proyecto se
> eligieron para valer en los cuatro compiladores: `NSTD_KARATSUBA_MIN` está en
> 32 porque «a partir de 32, Karatsuba gana en los cuatro». Un umbral afinado
> sólo con clang puede ser malo para GCC o MSVC. El contraste con GCC en los
> puntos de decisión es lo que evita cambiar un default global con datos de un
> solo compilador.

| Eje | Valores |
|---|---|
| **N (bajas)** | **1..64 completo**, sin saltar ni una |
| **N (altas)** | 72, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 640, 768, 896, 1024, 1280, 1536, 1792, **2048** |
| **Operación** | `mul`, `div`, y `add`/`shl` como testigo de que el banco no deriva |
| **Camino de `mul`** | escolar en bucle · escolar desenrollado · Karatsuba (hoy sólo potencias de dos) |
| **`NSTD_DESENROLLA_MAX`** | 8, 12, 16, 20, 24, 28, 32 |
| **`NSTD_KARATSUBA_MIN`** | 8, 16, 32, 64, 128 |
| **Repeticiones** | **10 por casilla, mínimo** |

Son **85 anchuras**.

**Ni una potencia de dos suelta: interpolado en todo el rango.** El acantilado
de `operator*` se descubrió justo porque nadie había mirado entre 16 y 32, y la
rejilla de potencias de dos es lo que lo escondió durante meses. Por eso las
bajas van completas de 1 a 64, y las altas llevan puntos intermedios (160, 224,
320, 448, 640, 896, 1280, 1792) y no sólo 128-256-512-1024.

**Hasta 2048**, no 1024: con iteraciones adaptativas cuesta lo mismo por
casilla, y es la mitad alta del rango que la biblioteca admite. Por encima está
`NSTD_LIMBOS_MAX = 4096`, y medir ahí sube el coste sin que caiga ningún umbral
de los que interesan.

---

## 3. Protocolo

Heredado de [PLAN_BENCHMARK_AND_TESTING_METHODOLOGY](PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md),
y no se negocia:

1. **Un caso por proceso.** La posición dentro de una ejecución cambia la medida.
2. **Orden aleatorio** entre casos.
3. **Mínimo**, no media: el mínimo es la medida menos contaminada por el sistema.
   Pero el mínimo **de qué**: de las 10 repeticiones, no de una.
4. **Rondas entrelazadas** entre las variantes que se comparan.
5. **Comprobar el código de salida.** Ya se ignoró dos veces en este proyecto y
   las dos dio cifras imposibles que casi se publican.
6. **Diez repeticiones por casilla, como mínimo, y se publica la dispersión.**
   No es un diagnóstico aparte: es el protocolo. Una casilla sin dispersión no
   es una medida, es una anécdota — y este proyecto ya ha publicado dos cifras
   contradictorias (§0) por medir una sola vez.
7. **Repetir la tanda entera en otra sesión** antes de dar un umbral por bueno.
   Dos ejecuciones seguidas no prueban reproducibilidad: prueban que la máquina
   no cambió en treinta segundos.

### Lo que cuesta, medido

**Compilar** la unidad con las 85 anchuras, clang −O2, el 10 sep 2026:

| `NSTD_DESENROLLA_MAX` | compila en | binario |
|---:|---:|---:|
| 20 | 21,5 s | 662 KB |
| 32 | **38,2 s** | **1 482 KB** |

Subir el tope de desenrollado de 20 a 32 cuesta **1,8× de tiempo de compilación
y 2,2× de binario** en esa unidad. Es un dato que pesa en la decisión del tope,
y que no se tenía.

**Ejecutar**, con 200 ms por casilla y 10 repeticiones:

| Tanda | Cuenta | Tiempo |
|---|---|---:|
| Curva completa | 85 N × 4 op × 10 rep | ~11 min |
| Barrido de `DESENROLLA_MAX` | 7 topes × 40 N × 10 rep, sólo `mul` | ~9 min + 4 min de compilar |
| Barrido de `KARATSUBA_MIN` | 5 topes × 45 N × 10 rep, sólo `mul` | ~8 min + 3 min de compilar |
| Contraste con GCC en los puntos de decisión | sólo `mul` | ~3 min |
| **Total** | | **~40 min de máquina** |

Con la regla 7 —repetir la tanda entera en otra sesión— son unos 80 minutos en
total. Cabe de sobra en una sesión.

### Qué se guarda

Cada tanda va a `scripts/bench_history.py` con fecha, commit y compilador, para
que las tandas se puedan comparar entre sí. Es lo que permitirá saber, la
próxima vez que dos cifras no coincidan, cuál de las dos era.

---

## 4. Lo que esta sesión tiene que dejar decidido

| # | Pregunta | Cómo se responde |
|---|---|---|
| 1 | ¿1,11× o 2,44× en N=24? | Reproducir las dos con el mismo banco |
| 2 | ¿Dónde va `NSTD_DESENROLLA_MAX`? | El barrido, con el contraste de GCC |
| 3 | ¿Dónde va `NSTD_KARATSUBA_MIN`? | Ídem. Puede bajar de 32 |
| 4 | ¿Cuánto ruido tiene este banco? | Sale solo: con 10 repeticiones por casilla, la dispersión es un producto de cada tanda, no un experimento aparte. **Sin ella no se puede saber qué diferencias son reales** |
| 5 | ¿El acantilado sale en la curva de 1..64? | Debe verse el escalón exacto |

La 4 dejó de ser un experimento aparte al pasar las 10 repeticiones al
protocolo, y es lo mejor que sale de este plan: hoy se dice «~35 % de ruido» a
partir de dos ejecuciones, que no es una medida de dispersión de nada.

---

## 5. Fuera de alcance, a propósito

- **No se escribe ningún algoritmo nuevo.** Karatsuba equilibrado, Toom-3,
  Möller–Granlund y Burnikel–Ziegler van después, cada uno con su medición.
- **No se cambia ningún default** hasta que la 4 diga qué diferencias son reales.
- **No se mide por encima de N=2048.** El límite duro es
  `NSTD_LIMBOS_MAX = 4096`, pero ahí una casilla se va a minutos y no cae ningún
  umbral de los que interesan.
- **No se tocan MSVC ni Intel.** Entran cuando haya que fijar un default global.

---

## 6. El límite de N, ya decidido

Medido el 10 sep: **114 bytes de pila por limbo** para `c = a * b`, sobre **1 MB**
de pila en Windows (leído del PE, no supuesto). N=4096 usa el 44 %, N=8192 el
88 %. `NSTD_LIMBOS_MAX` queda en **4096**, con `static_assert` que lo dice y
explica por qué. Ver el bloque `@def` de esa macro en `fixed_width_int_t.hpp`.
