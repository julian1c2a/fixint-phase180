# 🔮 NEXT STEPS

**Last Updated:** 27 September 2026
**Versión:** **v1.90.4** publicada · **rama** `phase-1.90` *(abierta el 27 sep)* · árbol limpio · todo en `origin`

> ### Esta rama empieza por la fase 0: **afinar el banco**
>
> No por el borrado de `int128_param_*` ni por el renombrado, aunque sean lo
> gordo. El banco es el instrumento con el que se va a juzgar todo lo demás, y
> hay tres señales medidas de que no está afinado. El orden y el porqué, en la
> sección **«1.90, fase 0»** más abajo.
>
> La rama `phase-1.80` queda como estaba, con la toma de referencia de la 1.80
> dentro: es la última que incluye los nueve bancos del tipo viejo.

> Este documento es **el puntero y lo pendiente a corto**. No acumula historia:
> lo ya hecho vive en [`CHANGELOG.md`](CHANGELOG.md), el plan largo en
> [`ROADMAP.md`](ROADMAP.md) y el estado en [`PROJECT_STATUS.md`](PROJECT_STATUS.md).
>
> **Y no acumula tareas hechas.** Llegó a tener 405 renglones con dos secciones
> «para mañana» y trabajo ya terminado dentro. Si al abrirlo hay algo hecho, se
> borra en el momento: es la única forma de que la lista sirva para decidir.

---

# 📍 POR AQUÍ VAMOS

## Estado al 1 oct 2026

Las cifras de esta tabla las comprueba la **comprobación 10** del armonizador, que
existe porque este bloque ha envejecido **dos veces** de la misma forma: citó un
hash de CI **58 commits atrás** —y en ese hueco el CI estuvo cuatro días en rojo
sin que nadie mirara— y al arreglarlo se escribió otro que llegó a estar **46
commits atrás**, con el recuento de ADR también mal. El párrafo que explicaba el
problema no evitó el problema.

| | |
|---|---|
| **Release** | ✅ **v1.90.4 publicada**, la primera del proyecto: tres zips (gcc, clang, msvc) |
| **Suite** | ✅ **72/72 en CINCO configuraciones**, y **no es una medida congelada**: el CI corre `python make.py test` en gcc, clang, clang+libc++, MSVC e Intel **en cada commit**. La referencia local son 294 s con GCC+libstdc++ (`release-O2`). Aquí se afirmaba antes «medido el 27 sep sobre un árbol cuyos `include/` y `tests/` no han cambiado», y eso dejó de ser cierto el 30 sep con el nivel 3 de la deprecación |
| **CI** | ✅ **último confirmado: 25/25 jobs sobre `79bb7bf`** (1 oct), **contados** con `gh run view`, no supuestos. El armonizador imprime en cada ejecución a cuántos commits queda este hash, y falla pasados 25 |
| **Diseño de la 2.0** | ✅ cerrado, y **escrito entero**: de P1.1 a P1.6, con P1.5 tramo 3 (Magnitud-Signo y Exceso-K) y las tres entregas del punto fijo |
| **`operator*`** | ✅ **el frente de la multiplicación, cerrado** (17 sep 2026). Cuatro algoritmos reunidos en `algorithms/mul_kernels.hpp`, cada umbral medido: escolar desenrollado ≤ 21, **Karatsuba equilibrado** ≥ 22 para *cualquier* N, **cuadrado** propio para `x*x`, y **Toom-3** ≥ 1024 |
| **La división** | ✅ **cerrada por ahora**. Knuth D en su capa medible (`div_kernels.hpp`), `divq` en línea **1,28×–1,39×**, Möller–Granlund **2/1** (~84 → ~17 ciclos/limbo) y **3/2** (hasta **3,3×**). **P2.11 aparcado con medida** ([ADR-016](docs/decisions/ADR-016-burnikel-ziegler-aparcado-por-medida.md)): hasta N=1024 la división ya está dentro del techo de 2–4× que publica GMP |
| **Lo siguiente** | 🔸 **la 1.90 está en su fase 0**: afinar el banco antes de borrar y renombrar. **Cerradas cinco** (P2.17, P2.18, P2.19, P2.21 y **P2.22**, que entró por decisión del autor el 1 oct); quedan **la segunda mitad de P2.23** —la cola térmica; la primera, el calor de la compilación, se descartó midiendo el 2 oct— y **la lectura de las cifras `(*)`**. **Y un dato que decide cómo se toma la referencia**: en el estado normal de esta máquina, una de cada tres medidas de ~40 s sale perturbada aunque se espere antes a que haya calma. Una toma de hora y media no saldrá certificable sin **silenciar la máquina de verdad** (`sesion_medicion.ps1`, como administrador). La referencia nueva será **la primera toma certificable** del histórico: ninguna de las anteriores midió lo que pasaba durante. Después: la referencia nueva, el borrado de `int128_param_*` y el renombrado de [ADR-023](docs/decisions/ADR-023-sin-sufijo-t-en-las-plantillas-de-clase.md)/[ADR-024](docs/decisions/ADR-024-el-nombre-del-tipo-de-punto-fijo.md) |
| **Sin decidir** | 🔸 tres cosas de la tabla de [ADR-019](docs/decisions/ADR-019-punto-fijo-es-un-entero-con-escala.md): operar entre tipos con **distinto `F`**, convertir **desde y hacia coma flotante**, y `sqrt` con escala. El nombre del tipo **ya no está aquí**: lo cerró ADR-024 el 1 oct |
| **Paridad de parámetros** | ✅ **348/348 celdas**, **recomprobadas el 1 oct** con `scripts/check_matriz_paridad.py`: 47 capacidades × **6** columnas, más las sondas del punto fijo, verificadas **compilando** |
| **Doxygen** | 🔸 **257 avisos en `include/`**, igual que el techo, y **todos** de `int128_param_*` ([ADR-014](docs/decisions/ADR-014-cobertura-de-doxygen.md)). Caducan solos con el borrado; P3.2 (`WARN_AS_ERROR = YES`) espera eso |
| **ADR** | **24** registros, ninguna decisión sin documentar |

**Lo primero al retomar: `python scripts/check_docs_consistency.py --doxygen`.**
Con `--doxygen`, que es la orden que corre el CI; sin el flag son 12
comprobaciones en vez de 14 y no sirve de nada.

---

## Prioridades

Cómo se decide qué va antes. **Tres preguntas, en este orden**; la primera que
dé «sí» fija el nivel. Cada criterio sale de un tropiezo concreto, no de una
máxima general.

### Criterio 1 — ¿Miente?

**Todo lo que hace creer al proyecto algo que no es cierto va primero.** No por
pulcritud: las demás prioridades **se deciden leyendo esas señales**. Si mienten,
se prioriza mal y ni siquiera se sabe.

Lo que ha aparecido en dos días:

| Señal | Decía | Pasaba |
|---|---|---|
| `make.py build` | «Build complete», salida 0 | el enlazado había fallado |
| «0 avisos de Doxygen» | cobertura perfecta | la comprobación estaba apagada |
| Job de Intel en la release | `success` en 2,3 min | cero artefactos |
| Benchmark de Karatsuba | 6,23× de mejora | medía contra un espantapájaros |
| CI en verde durante meses | todo bien | el workflow de release llevaba roto |
| «armonizador 7/7» | todo conforme | se corría **sin** `--doxygen`; el CI lo corre **con** |
| `cross-arm32`, `aarch64` | jobs en verde | 1 y 2 tests fallando, tapados |

Siete, y **ninguno se detectó mirando**: todos salieron de comprobar un resultado
que ya se daba por bueno.

### Criterio 2 — ¿Bloquea?

El orden del camino crítico **no es negociable**, lo fija
[ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md):
política de desbordamiento → unificación de tipos → punto fijo. Al revés
significa **portar la API dos veces**. No hay nada que decidir aquí: está
decidido, hay que escribirlo.

### Criterio 3 — ¿Caduca, y hacia qué lado?

De dos tareas igual de secundarias, la que caduca va antes o después según en qué
dirección lo haga:

- **Medir caduca hacia adelante.** Una cifra tomada *después* de reescribir el
  código **no se puede comparar** con las de hoy. Los benchmarks pendientes van
  **antes** de la 2.0, aunque parezcan menos importantes.
- **Documentar `int128_param_*` caduca hacia atrás.** Ese tipo lo retira
  [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md): la
  cuenta **baja sola** conforme se alcanza la paridad, porque cada pieza portada
  permite **borrar** la vieja. El trabajo no es documentar, es borrar.

  **Ojo: eso vale para 257 de los 466 avisos, no para todos.** Contado el 21 sep,
  los otros **209 son del tipo NUEVO** y no los borra nadie. Es la diferencia
  entre una deuda que caduca y una que sólo lo parece — ver **P3.7**.

### Desempate

Entre iguales, **primero lo que se reproduce en local**. Las cuatro releases que
costó publicar v1.90.4 se fueron en probar cosas que solo podían probarse en el
CI; reproducir el fallo de v1.90.2 en local costaba dos segundos.

---

## El orden de las etapas largas (decidido el 30 sep 2026)

**5 (punto fijo, en marcha) → 6 (coma flotante) → 9 (decimal/BCD) → 7 (bigint) → 8 (racionales).**

No sale del orden de numeración de
[Explicación_del_Proyecto.md](AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md);
sale de tres razones, y la segunda es la que más pesa.

**1. La coma flotante antes del decimal, porque MS y EK son una apuesta sin
validar.** La ETAPA 3 se hizo explícitamente *para* la 6 —«estos nuevos tipos
tienen interés para la futura implementación de tipos punto flotante»—. Hoy MS y
EK cuestan **30 `if constexpr`** en la plantilla, dos ADR y un puente, y su
consumidor previsto no existe. El punto fijo las acepta, pero como una forma
**intercambiable** del eje `Form`; la coma flotante las usaría
**estructuralmente** —MS para el par signo+magnitud de la mantisa, EK para el
exponente sesgado—, que es el uso para el que se diseñaron y el único que dice si
el diseño era correcto.

Y comparten el vocabulario de redondeo: `fixed_point_t::rounding_mode` ya tiene
cinco modos y el primero se documenta como «el de IEEE-754». Si la coma flotante
llega **después** de la pasada de nomenclatura de la 1.90, hereda un vocabulario
asentado; si llega antes, fuerza un segundo renombrado sobre lo mismo.

**2. El decimal no es «otro tipo»: rompe una premisa escrita.** La ETAPA 9 es
**BCD** —Natural sin signo, Aiken con signo—, o sea un valor nuevo en el eje
`representation_form`, no un tipo construido encima. Y
[ADR-017](docs/decisions/ADR-017-magnitud-signo-y-exceso-k-como-codificaciones.md),
decisión 1, dice literalmente **«No se escribe aritmética nueva»**: se decodifica
a complemento a dos, se opera, se recodifica. BCD no puede cumplirlo:

- **Semánticamente**, decodificar a binario, operar y recodificar es justo lo que
  el decimal existe para evitar — redondearías en binario.
- **Por coste**, la conversión BCD↔binario es multiplicar/dividir por diez
  repetidamente. El párrafo de ADR-017 que justifica el puente («con Exceso-K una
  conversión es invertir un bit; con MS, una negación en el peor caso — frente al
  coste de una multiplicación de N limbos, es ruido») **deja de valer**.

Así que BCD sería la primera **aritmética** en un eje diseñado para
codificaciones, y obliga a revisar ADR-017 —probablemente partiendo el eje en dos—.
Eso se hace una vez, y después de que la coma flotante haya dicho si el eje
aguanta.

**3. La longitud variable al final, porque es la única que cambia el modelo de
almacenamiento.** De `std::array<uint64_t, N>` con `N` de compilación a un búfer
con `N` de ejecución. Eso rompe tres propiedades que hoy se garantizan en todas
partes: `constexpr` (la evaluación constante es un camino explícito en
`operator*`), `noexcept` ([ADR-004](docs/decisions/ADR-004-sin-excepciones-en-el-nucleo.md):
la aritmética no lanza, y con asignación dinámica sí puede) y **cero
asignaciones**. Y los umbrales (`NSTD_KARATSUBA_MIN`, `NSTD_DESENROLLA_MAX`) pasan
de `if constexpr` a ramas de ejecución, con lo que cada algoritmo necesita un
segundo camino y la matriz de pruebas se dobla.

### Dos cabos que quedan de esta decisión

- **Los racionales quedan al final por herencia, no por decisión.** La ETAPA 8
  depende de la 7, pero los racionales **no necesitan** longitud variable: `ratio`
  sobre `fixed_int_t<N>` es utilizable, y la versión bigint puede llegar después.
  Si interesan antes, hay que decidirlo a propósito.
- **Los marcadores de estado de `Explicación_del_Proyecto.md` están
  desfasados**: la ETAPA 5 dice «por comenzar» con 1.278 líneas y tres ADR
  escritos, y la 3 dice «Exceso-K pendiente» cuando las cuatro representaciones
  están en `fixed_int_t` con 16.624 comprobaciones cruzadas. Conviene ponerlos al
  día antes de volver a usar ese documento para planificar.

---

## Los siguientes pasos, ordenados

### P0 — Señales que mienten

| | Qué | Estado |
|---|---|---|
| ~~P0.1~~ | ~~`make.py` y el enlazado~~ | ✅ **hecho** (`05ba169`) |
| ~~P0.2~~ | ~~Confirmar que el CI se pone verde~~ | ✅ **confirmado** en `314543e` |
| ~~P0.3~~ | ~~47 `assert()` que `-DNDEBUG` borra~~ | ✅ **hecho**, y comprobado que ahora saltan |
| ~~P0.4~~ | ~~`test_template_type.cpp` no comprueba nada~~ | ✅ **hecho**: las identidades de tipo son `static_assert` |
| ~~P0.5~~ | ~~`cross-arm32` y `aarch64` tapan fallos reales~~ | ✅ **hecho**: un bug de compilacion y tres timeouts |
| ~~P0.6~~ | ~~Intel oneAPI en Windows~~ | ✅ **en local**: 55/55 con Intel 2026.1. Queda **solo el runner del CI** |
| ~~P0.7~~ | ~~El clang del proyecto: CLANG64 (libc++) o UCRT64~~ | ✅ **cerrado a favor de las DOS**: `nstd::is_integral...` se define para libstdc++ y para libc++. Eran tres causas: la guarda que se causaba a sí misma, una regresión de P1.1 en el 4.º parámetro, y `-latomic` a ciegas |
| ~~P0.8~~ | ~~Qué saca la matriz del CI ahora que compila de verdad~~ | ✅ **nada tapado**: 24/24 jobs verdes sobre `f959f53`, matriz completa |
| ~~P0.9~~ | ~~Tests de guardas de include y de macros de configuración~~ | ✅ **hecho**: `tests/test_config_macros.cpp`. Cazó un fallo real en su primera compilación — las guardas entre los dos ficheros de traits eran asimétricas y solo permitían un orden de inclusión |

### P1 — Camino crítico (el orden lo fija ADR-007)

| | Qué | Depende de |
|---|---|---|
| ~~P1.1~~ | ~~Almacenamiento de la política~~ | ✅ **hecho** en `6ea1ae4`: 32 bytes con `wrap` en los cuatro compiladores |
| ~~P1.2~~ | ~~Propagación de la marca, `valid()`, comparación~~ | ✅ **hecho** en `5ad2bad`: `+ - * << - ++ --` y sus `op=`, orden total, `to_string` |
| ~~P1.3~~ | ~~`checked_div` y las tres `saturating_*`~~ | ✅ **hecho** en `d684bb6`, y las `checked_*` dejan `std::optional`. Destapó el producto con signo, que se leía sin signo |
| ~~P1.4~~ | ~~`representation_traits<binnat>` y el `static_assert` al bicondicional de [ADR-011](docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md)~~ | ✅ **hecho** en `c73e55a` |
| **P1.5** | **Retirar `int128_param_t`**. Los tramos de paridad, cerrados. **Tramo 2 (deprecación) hecho el 23 sep**: los seis alias públicos llevan `[[deprecated]]`, hay [guía de migración](docs/MIGRACION_int128_param.md), y la compilación propia sigue con **cero avisos** gracias a cuatro alias internos sin marcar. **Tramo 3 es el borrado**, y ADR-006 lo exige así: no antes de que la deprecación se haya publicado. **Va después de la fase 0** (afinar el banco), por el motivo que se explica allí | ✅ tramos 1 y 2 |
| ~~P1.5 tramo 1~~ | ~~`bits`, `cmath` y `numeric`~~ | ✅ **hecho**: `rotl`/`rotr`, los nombres de `<bit>`, `min`/`max`/`clamp`/`midpoint`/`abs_diff`, `ilog2`/`factorial`/`is_even`/`is_odd`, y las cuatro que el inventario del ADR no listaba (`is_power_of_2`, `sign`, `abs` y `divmod` libres). `tests/test_fixed_bits_numeric.cpp`, 60 `static_assert` |
| ~~P1.5 tramo 2a~~ | ~~La política fijada en nueve firmas~~ | ✅ **hecho**: `mul_wide`, `pow`, `sqrt`, `gcd` y `lcm` no compilaban con un tipo `checked`. Ahora llevan `Policy` deducible |
| ~~P1.5 tramo 2b~~ | ~~`mulhi` y `mullo`~~ | ✅ **hecho**: `widening_mul` no se porta, es `mul_wide`. Cruzado con 400.000 pares al azar |
| ~~P1.5 tramo 2c~~ | ~~Los clones de `<algorithm>`~~ | ✅ **decidido (10 sep)**: no se portan. La regla que sale: no es «`std::` frente a `nstd::`», es si `std::` **acepta o rechaza** el tipo. Los de iterador lo aceptan; los restringidos a `integral` lo rechazan, y ahí `nstd::` es la única opción |
| ~~P1.5 tramo 2 (matriz)~~ | ~~Documento maestro de cobertura~~ | ✅ **hecho**: [MATRIZ_DE_PARIDAD](docs/MATRIZ_DE_PARIDAD.md) + `scripts/check_matriz_paridad.py`. 42 capacidades × 4 celdas, **comprobadas compilando**, no a mano. Destapó **siete sitios** que P1.1 dejó con tres parámetros |
| ~~P1.5 tramo 2f~~ | ~~Las siete `checked_*` y `saturating_*` sólo con `wrap`~~ | ✅ **hecho (10 sep)**: aceptan cualquier política, y **la marca es pegajosa**. Saturar no limpia una marca previa, porque un valor marcado guarda dentro el resultado *envuelto* y saturar a partir de él no da el valor correcto |
| ~~P1.5 tramo 2d~~ | ~~`atomic_*` y el envoltorio atómico~~ | ✅ **hecho**: `include/fixed_int_atomic.hpp`. **No es una copia del viejo**: aquél usaba mutex siempre y mentía con `is_always_lock_free() == false` fijo; éste va **sin bloqueo** donde se puede. La condición **no es el tamaño sino `is_always_lock_free`**, porque es la única que garantiza no arrastrar `-latomic` — en gcc, `std::atomic<T>` de 16 bytes **no enlaza sin él**. Y corre en **MSVC e Intel**, donde el test viejo tiene excepción |
| ~~P1.5 tramo 2e~~ | ~~`div<D>`/`mod<D>`/`divmod_const<D>` por divisor constante~~ | ✅ **hecho**, y **no hubo que portar Granlund–Montgomery**: basta con que el preámbulo (normalizar y calcular el recíproco, un `divq`) se resuelva en compilación, porque ya era `constexpr`. **8,13× en N=2**, y nunca pierde. De paso desmintió dos afirmaciones de `PERFORMANCE.md`: el eje que manda es `N`, no el tamaño del divisor, y **10¹⁹ sí cabe en 64 bits** |
| ~~P1.5 tramo 3~~ | ~~**Magnitud-Signo y Exceso-K**~~ | ✅ **hecho (22 sep)**. Todo el tramo cabe en **dos funciones** --`a_c2` y `desde_c2`-- y quince sitios que las cruzan: **ni un algoritmo aritmético nuevo**. El test cruzado que fijó [ADR-018](docs/decisions/ADR-018-la-representacion-no-es-observable.md) sacó seis huecos, todos la misma equivocación —dar por hecho que los limbos son el valor—, incluido que en Exceso-K `T x{}` valía **-2¹²⁷ en vez de cero**. **16 624 comprobaciones** cruzadas contra complemento a dos |
| **P1.6** (1ª entrega) | **Punto fijo: el tipo y las conversiones.** ✅ **hecho (22 sep)**. `include/fixed_point_t.hpp`: el tipo, `desde_crudo`, las constantes, y **lo exacto** —suma, resta, negación, producto por un entero, las seis comparaciones, `<=>`, `suelo()`, `parte_fraccionaria()`, `to_string()`—, todo `constexpr`. **13 020 comprobaciones** contra un oráculo de `__int128` escalado. El único fallo fue el tipo **puramente fraccionario** (`F == N`): sin parte entera donde recoger la cifra, `to_string` daba `0.0` para un medio; se arregla multiplicando **al doble de ancho**. Buscar las otras dos esquinas saco un segundo fallo: `to_string(min())` llevaba **dos** signos, porque negar el minimo envuelve. Y el cruce de **las cuatro representaciones** paso entero a la primera hasta que se rompio el codigo a proposito: era vacuo --construido desde enteros, el limbo bajo es siempre cero-- y con fracciones de verdad el mismo fallo da **166**. Y contar los ejes del tipo --`N`, `F`, `Sign`, `Form`, `Policy`-- saco el quinto sin cruzar: `checked` estaba declarado pero **`valid()` no se reexponia** | ✅ P1.5 |
| **P1.6** (2ª entrega) | **El redondeo, `*`, `/`, `%`, `++`.** ✅ **hecho (22 sep)**, según [ADR-020](docs/decisions/ADR-020-los-operadores-multiplicativos-del-punto-fijo.md). Ni una trae algoritmo nuevo: `*` es `mul_wide` más un desplazamiento, `/` un preescalado más `divmod`, y **`%` resultó ser exacto** —el resto de dos múltiplos de `epsilon` es múltiplo de `epsilon`—. `++` suma **uno**, como en `float`. **13 020 comprobaciones** contra **dos** oráculos en dos anchuras, y el test falsificado modo por modo con seis averías. Sacó un fallo real: con divisor **impar**, cuatro de los cinco modos se comportaban como `toward_pos_inf` | ✅ P1.6 1ª |
| **P1.6** (3ª entrega) | **`to_string` redondeado, y los desplazamientos.** ✅ **hecho (22 sep)**. Se redondea el **valor**, no la magnitud —lo contrario rompe los modos dirigidos— y de ahí salen dos espejos que costaron un fallo cada uno. `<<` exacto, `>>` redondea; **sin bitwise**, vigilado por la matriz. **13 020 comprobaciones**, y el arnés de falsificación destapó que el cruce de `>>` era vacuo: los enteros tienen 64 bits bajos a cero y no hay nada que redondear | ✅ P1.6 2ª |

### ⬅️ Por aquí se sigue (22 sep 2026)

**El camino crítico de la 2.0 está cerrado.**

P1.5 y P1.6 están hechos: el punto fijo tiene el tipo, las conversiones, la
aritmética exacta, los operadores multiplicativos y la perilla de redondeo con
sus cinco modos. **P1.6 está completo**, `to_string` redondeado incluido.

Lo abierto de verdad sigue en la tabla de ADR-019: operaciones entre tipos con
**distinto `F`**, conversión **desde y hacia coma flotante**, y `sqrt` con
escala. Ninguna está decidida, y la primera es la que más toca la ergonomía.

**Y P3.7 ya puede empezar.** Estaba esperando al tramo 3 a propósito: documentar
antes habría sido documentar operaciones que acababan de reescribirse.


#### P1.6 (punto fijo) no puede empezar antes

Depende de P1.5 entero por [ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md),
y hacerlo al revés significa portar la API dos veces. Lo que sí está listo para
cuando llegue: el tipo ya lleva sus **cuatro** parámetros publicados
(`num_limbs`, `sign`, `form`, `policy`), que es lo que hace falta para
reconstruirlo desde código genérico — y eso lo destapó el envoltorio atómico, no
el punto fijo.


> **Resuelto en 2a**, y eran nueve firmas, no tres: `mul_wide` y `sqrt` tenían el mismo defecto. La lección se repite: una
> lista escrita de memoria se queda corta, y hay que contarlas abriendo el fichero.

### P2 — Medir, antes de que el código cambie

| | Qué | Por qué antes |
|---|---|---|
| ~~P2.8~~ | ~~**El acantilado de `operator*`**~~ | ✅ **hecho** en `54ce3b6`, `a8ef36e` y `97523f3`. Karatsuba con reparto **equilibrado** para toda N; el acantilado desaparece (N=48: 1,54×). Los umbrales quedaron en **21 y 22**, o sea **una sola frontera**: el escolar en bucle no gana en ninguna de las 244 casillas medidas, así que dejó de elegirse. El reparto `2^n + r` se descartó: **no baja el exponente** |
| ~~P2.1~~ | ~~**Barrido de `operator*`**: ¿paridad o tamaño?~~ | ✅ **respondido, y la pregunta estaba mal planteada**: no era la paridad de N sino **si N era potencia de dos**, porque el Karatsuba viejo sólo admitía esas. Con el reparto equilibrado la distinción desaparece: verificado en las **63 anchuras** de 2 a 64, impares incluidas |
| ~~P2.2~~ | ~~Karatsuba en **Clang, MSVC e Intel**~~ | ✅ **hecho** en `6cc0d26`: el barrido de `NSTD_KARATSUBA_MIN` en los cuatro. **No coinciden** — el cruce está en 14 (clang), 18 (Intel), 22 (MSVC) y 28 (gcc), y 22 es el óptimo por media y por peor caso |
| ~~P2.3~~ | ~~Re-medir las tablas heredadas~~ | ✅ **hecho**: y **dos de las tres no se sostenían**. «Knuth D 6,24×» mide **1,31×**; «Granlund-Montgomery 4–7×» solo vale para divisores que no caben en un limbo |
| ~~P2.4~~ | ~~Coste de las conversiones a y desde cadena, bases 2..36~~ | ✅ **hecho**: `benchmark_bases`. La base 10 gana a todas las potencias de dos, y la 3 cuesta 9,6× lo que la 10 |
| ~~P2.5~~ | ~~**Montar el histórico de benchmarks**~~ | ✅ **hecho (27 sep)**. El histórico pasa de **90 medidas de 2 suites** a **975 de 23**, y `--compare` decide con **el ruido de cada casilla** en vez de un umbral plano. El suelo está **calibrado** con dos tomas del mismo código: 0 falsos positivos frente a los 26 del umbral plano. Destapó que `benchmark_cuadrado.cpp` **no compilaba desde el 17 sep** y que el barrido de `Base` no registraba nada. Abre P2.16 y P2.17 |
| ~~P2.6~~ | ~~La tercera combinación: `clang + libstdc++`~~ | ✅ **hecho**: `make.py test clang-libstdcxx`, 59/59. Destapó que la lista de compiladores estaba repetida en **siete sitios** — ahora vive solo en `toolchains.py` |
| ~~P2.7~~ | ~~Desguace de benchmarks de algoritmo~~ | ✅ **las cuatro piezas hechas**: algoritmo/desenrollado, verosimilitud, código emitido (`scripts/bench_asm.py`) y coste teórico declarado |

| ~~P2.10 (2/1)~~ | ~~**Möller–Granlund 2/1** para `div_un_limbo`~~ | ✅ **hecho** en `e216c1b`: el coste por limbo cae de **~84 a ~17 ciclos**, hasta **4,97×**. Y **sí tiene umbral**, contra lo que se creía: calcular el inverso cuesta un `divq`, así que en N=1 es **0,23×** y en N=2 **0,91×**. `NSTD_MG_2POR1_MIN = 3` |
| ~~P2.10 (3/2)~~ | ~~**Möller–Granlund 3/2** para la estimación de q̂ dentro de Knuth D~~ | ✅ **hecho**: **3,3×** con divisor corto, **1,3×–1,8×** con `n = N/2`. El umbral **no se mide en anchura sino en dígitos de cociente** (`N - n + 1`), y está en 3: con uno solo, la 3/2 pura **pierde el 47%**. `NSTD_MG_3POR2_MIN = 3` |

> **«Mejora la constante, así que no tiene umbral» era falso, y lo fue dos
> veces.** Se escribió aquí antes de medir. La **2/1** lo desmintió: en N=1
> pierde 4×, porque el inverso cuesta una división que con un solo limbo no se
> amortiza; un barrido que empezaba en N=4 lo tapaba entero. La **3/2** lo
> desmintió otra vez, y por el mismo motivo con otra cara: el inverso se paga
> **por llamada** y ahorra **por dígito de cociente**, así que con un solo
> dígito —que es lo que dan dos operandos aleatorios de la misma anchura— es
> coste puro. El barrido que lo tapaba cruzaba `n=2` y `n=N/2`, pero **no
> `n=N`**.
>
> **Cuando un barrido sale perfecto, mirar dónde empieza y qué eje no se cruzó.**
> Van tres: el acantilado de Karatsuba, la 2/1 y la 3/2.
>
> Y el hermano de esa regla, en corrección: **9 300 casos aleatorios pasaron
> limpios y el primer barrido de esquinas tumbó la 3/2 en 9 anchuras de 11**. El
> fallo era real —Knuth D no garantiza la precondición de la 3/2— y ningún
> número razonable de aleatorios lo habría encontrado, porque esa rama tiene
> probabilidad ~2⁻⁶⁴ con entrada uniforme.
| ~~P2.11~~ | ~~**Burnikel–Ziegler**~~ | 🅿️ **aparcado por medida** ([ADR-016](docs/decisions/ADR-016-burnikel-ziegler-aparcado-por-medida.md)). El umbral esperado era falso **por veinte veces**: se copió el ~45–50 limbos de `DC_DIV_QR_THRESHOLD` de GMP sin copiar el denominador. Medido (`benchmark_techo_division`), la división `(N, n=N/2)` cuesta **1,6×–2,8×** una multiplicación de `N/2` hasta N=1024 — **ya dentro del techo de 2–4× de GMP**, donde B-Z no tiene qué recoger. Sólo lo pasa en **N=2048 (4,9×)**. Se reabre si la multiplicación baja de su 1,68 medido, o si el uso real sube de N=1024 |
| ~~P2.9~~ | ~~**Toom-3**: Θ(N^1,465)~~ | ✅ **hecho** en `97523f3`, **y la proyección del estudio era falsa en un orden de magnitud**. Decía «gana 1,23× en N=256»: lo medido es que **pierde** en 256 (0,87×) y en 512 (0,96×), y **no cruza hasta ~1024**. Entra con `NSTD_TOOM3_MIN` = 1024 y da **1,13× en 2048 y 1,14×–1,15× en 4096** |
| ~~P2.12~~ | ~~**`mul_wide` calcula el producto completo con una multiplicación modular de 2N×2N**~~ | ✅ **hecho** en `5f834b5`: **2,3×–2,8×**. Y lo heredó **sólo `mulhi`** — `checked_mul` y `saturating_mul` salieron planos porque **no usaban `mul_wide`**, lo que destapó P2.15 |
| ~~P2.15~~ | ~~**`checked_mul` multiplicaba DOS VECES**~~ | ✅ **hecho** en `1232175`: `producto_desborda` era un escolar **cuadrático** completo escrito sólo para mirar la mitad alta, y después se volvía a multiplicar por el camino rápido. Ahora una pasada: **1,4×–1,9×, y creciendo con N** porque la detección deja de ser cuadrática. `saturating_mul` lo hereda entero |
| ~~P2.13~~ | ~~**Sacar Knuth D de dentro de `divmod`** a una capa medible~~ | ✅ **hecho el 17 sep 2026**: `include/algorithms/div_kernels.hpp`. La extracción sale **gratis** (A/B contra HEAD: 0,94×–1,07×, ruido a los dos lados) y deja **la estimación de q̂ como parámetro de plantilla**, que es donde enchufa P2.10. `fixed_width_int_t.hpp` baja de 5 737 a 5 518 líneas |
| ~~P2.14~~ | ~~**`__udivti3` no emite un `divq`: emite una llamada**~~ | ✅ **hecho el 17 sep 2026**, y era un **comentario que mentía**. Decía «con `rem < d` se emite un solo `divq`»; en el binario había **16 llamadas a `__udivti3` y 4 `divq`**. Con `divq` en línea para GCC/Clang en x86-64: **1,28×–1,39× de punta a punta** en `n=1` y `n=2`, las siete anchuras. Trae consigo `check_precondiciones_div.py` |

> ### La lección de los tres días: comprobar gana a añadir
>
> Cuatro mejoras entre el 16 y el 18 de septiembre, y **ninguna vino de añadir un
> algoritmo**:
>
> | Hallazgo | Qué afirmaba el código | Qué hacía | Ganancia |
> |---|---|---|---|
> | `__udivti3` | «emite un solo `divq`» | emitía una **llamada** | 1,28×–1,39× |
> | `mul_wide` | nadie lo había mirado | ensanchaba para calcular ceros | 2,3×–2,8× |
> | `checked_mul` | «se construye sobre `mul_wide`» | multiplicaba **dos veces** | 1,4×–1,9× |
> | Toom-3 | «gana 1,23× en N=256» | pierde hasta ~1024 | 1,13×–1,15× |
>
> **Toom-3 —el único algoritmo nuevo de verdad— fue el que menos dio.** Y el
> `checked_mul` salió de comprobar una frase escrita *en este mismo repositorio*
> sin haberla medido. Antes de abrir P2.10 o P2.11, conviene preguntar qué otras
> afirmaciones llevan años sin comprobarse.

> **La lección de P2.9, que vale para todo lo que queda.** El
> [estudio](docs/ESTUDIO_ALGORITMOS_RAPIDOS.md) proyectó el cruce de Toom-3 en
> N=128–256 y está en ~1024: se equivocó por un factor de 4 a 8. No porque la
> teoría falle —el exponente 1,465 es correcto— sino porque **la constante de la
> interpolación se estimó en 2,5×–3× y es 4,6×**. Las proyecciones teóricas
> sirven para ordenar el trabajo, no para decidirlo: P2.10 y P2.11 traen cifras
> del mismo estudio y **hay que medirlas igual antes de creerlas**.

### P3 — Lo que se abarata o desaparece esperando

| | Qué | Nota |
|---|---|---|
| ~~P3.1~~ | ~~Ámbito de Doxygen para los headers internos~~ | ✅ **decidido (10 sep)**: fuera del ámbito. Criterio: entra lo que un usuario puede incluir, o sea la raíz de `include/`; `intrinsics/` y `algorithms/` no. Desbloquea P3.2 |
| **P3.2** | Cerrar la puerta: `WARN_AS_ERROR = YES` cuando el ámbito llegue a cero | ✅ **desbloqueado (23 sep)** por P3.7: ya no espera trabajo propio, solo a que P1.5 retire `int128_param_*`. Los 257 que quedan son **todos** de esa familia |
| ~~P3.3~~ | ~~Los **8** headers sin `API_*.md` propio que señala el armonizador~~ | ✅ **cerrada de otra forma (26 sep)**: los ocho lo estaban **a propósito**. Siete son internos —ADR-014 los dejó fuera del ámbito público— y el octavo es de la familia que se borra en 1.90. Escribirles `API_*.md` habría dicho que son API. En su lugar: [ARQUITECTURA_INTERNA.md](docs/ARQUITECTURA_INTERNA.md), el mapa de capas, y el armonizador distingue ahora **tres grupos** en vez de uno |
| ~~P3.4~~ | ~~`benchmark_vs_builtin` no enlaza sin GMP~~ | ✅ **hecho**: le faltaban `-lgmp`, `-lgmpxx` y `-ltommath`. Las tres bibliotecas estaban instaladas |
| **P3.5** | Documentar `int128_param_*` (**257** avisos) | **Caduca hacia atrás**: baja sola con P1.5. Desde P3.7 (23 sep) son **el techo entero**: los otros 209 ya están escritos. No se documenta lo que se va a borrar |
| **P3.6** | Decidir si Intel sale de la matriz de release | Se cae solo si P0.6 sale bien |
| ~~P2.16~~ | ~~**El CI no compila los benchmarks**~~ | ✅ **hecho (27 sep)**: job `benchs-build`, y **no compila: pasa el front-end**. Lo que hay que cazar es una búsqueda de nombres, y eso lo ve `-fsyntax-only` — **medido**, 26 s frente a 254 s en el más pesado, 80 s los 24 ficheros en vez de ~13 min. **Falsificado contra el fichero roto de verdad** (`git show 8581ff6:`): lo rechaza en 3 s nombrando `sqr_escolar_bucle`. No enlaza, así que sólo necesita las cabeceras de GMP/TomMath/Boost |
| ~~P2.21~~ | ~~**El job «los benchmarks compilan» no caza todo lo que parece**~~ | ✅ **hecho (30 sep)** en `0e2c21b`. `gcc` pasa a `-c -O2` y **clang se queda en el front-end**, y las dos mitades están **medidas contra el fallo real**, no supuestas: reconstruido el `cmp_mide` sin semilla volátil, `-fsyntax-only`, `-O0` y `-O1` **no lo ven con ninguno de los dos**, y sólo `gcc -c -O2` lo caza. Hace falta el `-O2` porque el error nace de que GCC **pliega** una constante, y eso sólo pasa al optimizar; y clang no sube porque **a `-O2` tampoco lo ve** —su rama de `doNotOptimize` es otra—, así que pagar quince minutos por una detección que no ocurre sería tirar el tiempo | Salió de P2.19, tropezando con ello |
| **P2.20** 🔸 | **Volver a medir el umbral del cuadrado.** `operator*` desvía `x·x` al núcleo de cuadrado desde **N=4**, apoyado en un «gana desde N=4, **2,47×**» medido con **clang** el 16 sep. La toma del 27 sep con **gcc 16** dice lo contrario: en N=4 el desvío **cuesta 1,45×** y no empata hasta **N=12**. O el umbral depende del compilador —y entonces no puede ser una constante— o una de las dos medidas está mal | Salió de dibujar la comparativa. Necesita el banco afinado (fase 0) |
| ~~P2.19~~ | ~~**El banco de comparaciones no mide comparaciones**~~ | ✅ **hecho el 29 sep en `0c2a9ab`, y la fila se quedó abierta dos días por descuido** — se vio el 1 oct, al ir a «atacarla»: describía un defecto que ya no existía en el código. Eran **dos** fallos y el segundo no estaba en el enunciado: ⓐ dos formas de bucle (`a += r` frente a `if (r) a += 1`) con `r` `volatile`, que metía el reenvío de almacén a carga **dentro** de la cadena de dependencia — los 5,17 ciclos de `uint64_t` eran casi exactos ese reenvío; y ⓑ **los operandos no cambiaban**, así que tras la primera vuelta el predictor acertaba siempre. Ahora un solo bucle para los siete tipos, el acumulador fuera de `volatile`, y ocho valores construidos para que cuatro queden por debajo del comparando **por construcción**. **Verificado contra el síntoma**: `uint64_t` 5,17 → **1,72** (el más rápido, como debe ser), y los cuatro de 128 bits de ancho fijo juntos en **2,14–2,18**. **Boost no nos gana comparando: empatamos** — el 12 % era del arnés. Desde entonces el fichero sólo ha cambiado en una línea, el `#define` de la puerta del legado | Salió de dibujar la comparativa: la tabla se autodesmentía |
| **P2.23** 🔸 | **Lo que el detector de P2.22 no ve.** **Entra en la fase 0** (decisión del autor, 2 oct). **Primera mitad, cerrada (2 oct): el +6,6 % de la primera ventana NO es el calor de la compilación.** Cinco condiciones, cada una apagando una causa —sin compilar; compilación ligera; **compilación pesada** (11–17 s, para ver la dosis); ejecutable nuevo para el antivirus sin compilar; y lo que hace `make.py`—, la misma casilla en cinco ventanas para que cada ejecución traiga su propia referencia, orden rotado y **puerta de calma** antes de cada repetición con el criterio de P2.22. **Ninguna infla la primera ventana**: todo dentro de ±2 %. Y un dato de mecanismo: **Defender sí inspecciona los ejecutables nuevos** (hasta 1,16 CPU), pero **en el arranque y la calibración**, antes de la primera vuelta cronometrada — la calibración lo absorbe. El +6,6 % fue, con toda probabilidad, una ráfaga: era la ventana más cargada de su toma. **El arreglo propuesto —compilar todas las suites antes de medir— no se hace**: nada lo justifica. Límites: la carga de trabajo no reserva memoria y `bases` sí; y 17 s sólo aproximan las compilaciones de minutos. Cuatro fallos del diseño cazados en dos pasadas en seco antes de gastar la máquina. **Queda la segunda mitad: la cola térmica** (+12 % en una ventana tras 90 s de carga fuerte, un solo dato) | Salió de cerrar P2.22 |
| ~~P2.22~~ | ~~**El arnés no ve las ráfagas que ocurren DURANTE una toma**~~ | ✅ **cerrada (1 oct)**, con la norma del autor por delante: **1º la corrección, 2º lo que mide, 3º toda infraestructura es primaria**. ⓐ **Corrección**: `maquina_tranquila: True` afirmaba más de lo que medía —sólo la espera previa— y pasa a `tranquila_al_empezar`. ⓑ **El arnés sabe CUÁNDO mide**: cada ventana lleva `t_inicio`/`t_fin` del reloj de pared, validado que es el mismo que el de Python. ⓒ **Se mide durante**: `otros`, la CPU de los demás procesos **restando la del propio benchmark**, validado contra una suma independiente proceso a proceso (±0,4 CPU; de paso se corrigió una cuenta doble de psutil en Windows) y con un coste por debajo del 0,05 % de una CPU. ⓓ **Lo que de verdad importa, medido** (E2): **un solo proceso compitiendo infla todas las cifras un 17 %**, y 15 las duplican — **sin que la dispersión lo vea**: detecta cambios dentro de la ventana, no niveles. Contar ventanas sucias no puede certificar una toma. ⓔ **El detector** (E3): el **suelo** de `otros` (p10) separa lo que daña (≥ 1,10) de lo inocuo, incluida la carga a ráfagas (≤ 0,30); la media no. Umbral **0,6 CPU**. Validado fuera de muestra con una **contaminación real** que se coló en E2-bis —todo un 34 % más lento—: **78 de 78** ventanas acertadas. ⓕ `--compare` usa la referencia **certificada** más reciente que tenga las suites, y aparta las casillas perturbadas. ⓖ **36 pruebas** de los guiones —no había ninguna—, falsificadas con **12 averías** (una destapó una prueba vacua), en Windows y en Linux, y en el CI. Lo que no ve: P2.23 | Salió de cerrar P2.18 |
| ~~P2.18~~ | ~~**Las doce casillas sucias: ¿pasajeras o sistemáticas?**~~ | ✅ **cerrada (1 oct): TRANSITORIAS, en ráfagas de tiempo.** No hizo falta la toma de quince minutos: el histórico ya tenía una **réplica** —`0cdc8d4` y `a014469`, mismo código medido, mismo día, 25 vueltas—. Tres cosas, y la tercera es la que decide. ⓐ **La unidad es la ventana, no la variante**: las 12 «casillas» eran 7 ventanas, porque las variantes de un N se miden entrelazadas y una interrupción las toca a todas. ⓑ **Las reglas se fijaron antes de mirar**, porque ya me había engañado una vez: las 7 ventanas sucias estaban en N ≥ 23 y parecía un patrón, pero ahí cae el 67 % de las ventanas (p = 0,06) y el corte lo elegí después de verlo. Resultado: coinciden 4 donde el azar da 1,87 (p = 0,083), y la concentración por tamaño queda en p = 0,052 — **en el borde, y no se movió la línea**. ⓒ **El orden temporal lo zanja**: los barridos recorren N en orden, así que N consecutivos son tiempo consecutivo, y la suciedad viene **en rachas**. En la toma sucia, `hueco` salió **entera limpia** y `barrido_desenrollado` tiene sucias **15 de sus 16 ventanas con N ≤ 16** —las más pequeñas de todo el barrido—. Eso entierra la hipótesis del tamaño y explica por qué rozaba el 0,05: dentro de un barrido N y tiempo van juntos. **No hay nada que arreglar en el arnés**: la barra de cola baja ya pide más a la casilla que sale ruidosa. Lo que sí sale es P2.22 | Salió de la calibración del 27 sep; cerrada sin gastar máquina |
| ~~P2.17~~ | ~~**Migrar al arnés adaptativo las once suites que quedan**~~ | ✅ **hecho (30 sep)**, y **eran tres, no once**: ocho de las once miden `int128_param_t`, que ADR-006 retira en la 1.90, así que migrarlas sería trabajo sobre código muerto. Migradas `curva_n` (`1621523`), `bases` y `karatsuba`. Coste: `bases` pasa de 2,5 s a **13,9 min** y `karatsuba` de 19 s a **3,6 min**. **Lo que destapó al verificarla importa más que la migración**: ver «El arnés viejo no medía lo que decía» | Salió de P2.5, con la medida hecha |
| ~~P3.7~~ | ~~**Documentar el tipo NUEVO: 209 miembros públicos sin `@brief`**~~ | ✅ **hecho (23 sep)**. Techo **466 → 257**, y los 257 restantes son **todos** de `int128_param_*`. De paso, dos defectos de marcado que hacían **perder documentación ya escrita** —un `@def` sin argumento y un `@example` en línea— y 45 avisos más en `intrinsics/`+`algorithms/`, que no cuentan contra el techo pero son el código que ejecutan los núcleos |
| ~~P3.8~~ | ~~**`numeric_limits<fixed_int_t>::is_modulo` miente**~~ | ✅ **hecho (26 sep)** en `f1e9de5`, CI 24/24 sobre `a77ae4c`. Pasa a `Policy == overflow_policy::wrap`, como ya hacía el punto fijo ([ADR-022](docs/decisions/ADR-022-numeric-limits-del-punto-fijo.md), decisión 6). **Eran dos mitades, no una**: también mentía SIN signo, porque `uint` + `checked` decía `true` y ahí no se envuelve, se marca. Y la documentación de la clase **ya decía lo correcto**: el código llevaba contradiciéndola desde el primer día |
| ~~P3.9~~ | ~~**Medir el techo de doxygen en 1.9.8**, la versión del CI~~ | ✅ **hecho (23 sep)**: **286**, leída del log del CI sobre `7f37017`. Llevaba en 518 desde agosto. La distancia con la local (257) son **29 avisos sobre el mismo árbol**, que es justo por lo que hay una cifra por versión |

> ### ✅ P3.7 — «P3.5 caduca hacia atrás» era sólo medio cierto (21 sep 2026, cerrado el 23)
>
> P3.5 dice que documentar `int128_param_*` **baja sola** al retirar ese tipo con
> P1.5, y de ahí se seguía que P3.2 —`WARN_AS_ERROR = YES` cuando el ámbito
> llegue a cero— se desbloquearía solo. **Contado, no es así.**
>
> Los **466** avisos de cobertura del ámbito público se reparten:
>
> | origen | avisos | ¿lo retira P1.5? |
> |---|---:|---|
> | `int128_param_*` | **257** | sí |
> | `fixed_width_int_t.hpp` | **177** | **no** |
> | `fixed_int_limits.hpp` | **32** | **no** |
>
> Es decir: retirar el tipo viejo baja de 466 a **209**, no a 0. Los 209 que
> quedan son del tipo que se queda, y **son trabajo propio que no estaba en
> ninguna lista**. De ellos **196 son funciones** y 28 variables.
>
> (Hay otros 41 avisos en `intrinsics/` y `algorithms/` que **no cuentan**:
> [ADR-014](docs/decisions/ADR-014-cobertura-de-doxygen.md) los dejó fuera del
> ámbito público. Por eso el total de doxygen es 511 y el techo 466.)
>
> **Qué hacer con esto.** No es «documentar 209 cosas» de una sentada; es la
> misma escalera de ADR-014: fijar el techo, bajarlo por tandas y no dejar que
> suba. Dos avisos de esta semana muestran que el mecanismo ya funciona —el techo
> saltó al insertar métodos en medio del bloque Doxygen de `divmod`, y otra vez
> al citar un mensaje de `ld` con comillas asimétricas—, así que lo que falta es
> **bajar**, no vigilar.
>
> **Y el orden importa**: hacerlo antes de P1.5 tramo 3 sería documentar
> operaciones que ese tramo va a tocar. Va **después**.
>
> ---
>
> **Cerrado el 23 sep 2026.** Los 209 escritos, techo en **257**, y la tabla de
> arriba queda con una sola fila viva: `int128_param_*`. Lo que no se vio al
> contar: **dos de los avisos no eran cobertura sino marcado roto**, y los dos
> hacían que Doxygen *perdiera* documentación ya escrita —un bloque de 30 líneas
> salía como «sin documentar»—, que es peor que no tenerla, porque el fichero
> parece bien y la referencia sale vacía.

---

## El detalle de lo inmediato

### P0.6 (lo que queda) — Intel en el runner del CI

**En local ya funciona**: 55/55 con Intel 2026.1 (581,9 s). Lo que se arregló:

- `compiler_env.py` tenía la versión **cableada a `2025.3`** teniendo instaladas
  2025.3, 2026.0 y 2026.1. No era una decisión, era un descuido: cogía la más
  vieja de las tres. Ahora `INTEL_VERSION = "auto"` descubre y coge la más alta.
- Las dos versiones de 2026 **fallaban hasta para responder a `--version`** con
  `icpx: error #10026: error generating temporary file`, por culpa del `TMP`.
  Con `C:\msys64\tmp` —lo que hay en esta máquina, por MSYS2— revientan; con
  `%LOCALAPPDATA%\Temp` van bien. La 2025.3 no se ve afectada, que es por lo
  que el problema pasó desapercibido mientras la versión estuvo cableada a la
  vieja. El entorno aislado de Intel ahora lleva un `TMP` que Intel acepta.

**Queda el runner**, que es un problema distinto: allí `ONEAPI_ROOT` viene
vacío y `rscohn2/setup-oneapi` deja un `CMAKE_PREFIX_PATH` apuntando a
`/opt/intel/oneapi/...` —una ruta de Linux en un runner de Windows—. Con el
camino local resuelto, ahora se sabe qué hay que exigirle: una instalación real
bajo `C:\Program Files (x86)\Intel\oneAPI\compiler\<ver>\bin\icpx.exe`.

Y sigue en pie la decisión de **si Intel se queda en la matriz de release**
(P3.6): las cabeceras son idénticas en los cuatro compiladores, así que el
contenido útil del zip no cambia.

### P2.1 — La penalización de `operator*`

El control del benchmark de Karatsuba destapó que el bucle escolar de
`operator*` es un **14 % más lento que una copia idéntica suya escrita como
función libre**, con las mismas primitivas y el mismo compilador. Estable en dos
ejecuciones (0,86× y 0,87×). En N=16 la razón sale 1,02×.

Barrido: **N=3, 5, 7, 9** (impares) frente a **N=6, 10, 12** (pares que tampoco
usan Karatsuba). Eso separa «es cosa de la paridad» de «es cosa de no ser 2, 4
u 8». Si sale lo segundo, apunta a la generación de código del `if constexpr`
encadenado; si lo primero, al bucle de acarreo. Después, el ensamblador del N que
peor salga.

### ⚠ El arnés viejo no medía lo que decía (30 sep 2026, saliendo de P2.17)

Migrar `karatsuba` movió las cifras. Comparar contra el histórico del 27 sep no
decidía nada, porque aquella toma se hizo con la máquina silenciada y ésta no,
así que la prueba es otra: **sacar de git la versión previa y correrla en la
misma máquina y el mismo minuto**. Entonces la única diferencia es el arnés.

#### 1. La ventana corta, en `karatsuba`

| N | arnés viejo | arnés nuevo | cambio | ventana del viejo |
|---|---|---|---|---|
| 3 | 9,86 | 19,13 | **+94,0 %** | 1,3 ms |
| 4 | 18,07 | 25,86 | **+43,1 %** | 2,4 ms |
| 7 | 46,03 | 57,52 | **+25,0 %** | 6,1 ms |
| 8 | 70,54 | 77,61 | **+10,0 %** | 9,4 ms |
| 9 | 80,65 | 79,37 | −1,6 % | 10,8 ms |
| 16 | 613,31 | 608,46 | −0,8 % | 81,8 ms |
| 32 | 2950,35 | 2913,75 | −1,2 % | 393,4 ms |

Ventanas **cortas** (<10 ms): **+32,2 %** de media. **Largas** (≥10 ms):
**−0,2 %**.

**La explicación alternativa, y por qué no vale.** Un coste fijo por iteración
del arnés nuevo encajaría igual con la forma de la tabla: sumar ~8 ciclos infla
un 90 % una casilla de 9 cyc/op y un 0,3 % una de 2950. Lo que la descarta es
**`N=8` contra `N=9`**: cuestan casi lo mismo (70,5 y 80,7) y se mueven **+10,0 %
y −1,6 %**. Un sumando constante les daría el mismo porcentaje. Lo que los separa
es la ventana, 9,4 ms y 10,8 ms, a los dos lados del escalón.

#### 2. Pero NO se generaliza, y eso hay que decirlo

El mismo A/B sobre `bases` da **el signo contrario y diez veces más pequeño**:
mediana **2,2 %**, y las casillas más baratas **bajan** un 15–18 % en vez de
subir. La diferencia estructural entre los dos arneses viejos es el
**calentamiento**: el de `karatsuba` tenía `WARMUP` y el de `bases` **no tenía
ninguno**. Sin calentar se mide frío y se lee de más; calentando pero con una
ventana de 1,3 ms se pilla la ráfaga de turbo y se lee de menos.

Lo único que vale para las dos: **con ventana corta el arnés viejo era poco de
fiar, y el signo del error depende de detalles**. La primera lectura —«medía de
menos»— era la de `karatsuba` tomada por ley general.

#### 3. Lo que sí cambia una conclusión escrita

La **razón justa** (escolar desenrollado / biblioteca) del arnés viejo **corrido
hoy** reproduce el histórico del 27 sep casi clavada —0,878 → 0,880 en N=4,
0,808 → 0,830 en N=8, 1,776 → 1,800 en N=32—, o sea que **aquí no hay deriva: el
cambio es del arnés entero**. Y **seis casillas cruzan el 1,00×**:

| N | 27 sep | viejo hoy | nuevo hoy |
|---|---|---|---|
| 4 | 0,878 | 0,880 | **1,000** |
| 5 | 0,949 | 0,940 | **1,120** |
| 6 | 0,922 | 0,930 | **1,220** |
| 8 | 0,808 | 0,830 | **1,110** |
| 10 | 0,881 | 0,900 | **1,350** |
| 12 | 0,901 | 0,930 | **1,310** |

Con el arnés viejo la biblioteca **perdía** contra el escolar desenrollado en
las seis; con el nuevo **gana**. El párrafo de `benchmark_karatsuba.cpp` que dice
«con `-funroll-loops` la razón en N=4 cae a 0,88×, o sea Karatsuba PIERDE» se
apoya en esas cifras.

**El mecanismo, y es el que el propio fichero creía tener resuelto.** El arnés
viejo medía las tres variantes **en orden fijo** dentro de cada ronda: la
biblioteca **siempre la primera**. Ir primero se paga —cachés y predictor
fríos—, así que `mejor_k` salía inflado y la razón `d/k` deflactada. La cabecera
del fichero decía que intercalar reparte la deriva por igual; **intercalar en
orden fijo no reparte nada**, sólo rotarlo. Es la cuarta vez que esta
comparación mide algo que no es.

#### 4. Y un aviso sobre el histórico

`bases` se mueve hasta un **±27 % entre el 27 sep y hoy con el MISMO arnés**
(mediana 5,1 %). Es una suite que reserva memoria y toca el asignador, así que
depende mucho de en qué estado esté la máquina. Para esa suite, comparar entre
tomas hechas en condiciones distintas no decide gran cosa, y conviene tenerlo en
cuenta antes de leer un `--compare`.

#### 5. Lo que se hizo con todo esto, el mismo día

- **La frontera está marcada**, y no como una fecha a mano: cada toma guarda su
  `protocolo`, y `--compare` avisa cuando dos tomas no se midieron igual —el
  mismo patrón que ya usaba para el compilador y para el número de
  repeticiones—. Falsificado en los dos sentidos: avisa contra una toma del
  histórico real (que no trae el campo) y **calla** contra una toma del mismo
  protocolo.
- **El apartado de `benchmark_karatsuba.cpp` está reescrito**, no parcheado: lo
  que aquel hallazgo del 6 sep descubrió sigue primero y en pie, y la cifra con
  la que lo cerró va después, corregida.
- **`bases` recorta la rejilla**: de las 35 bases pasa a seis —2, 3, 10, 16, 23
  y 36—, y de **13,9 min a 2,5 min**. Se recorta la rejilla, **no el
  protocolo**, así que sus casillas siguen siendo comparables con el resto. Las
  cifras coinciden con las de la rejilla completa (base 10 en N=2: 331,2 →
  333,7).

#### 6. La banda del barrido, retirada y sustituida (30 sep)

La banda pedía que el barrido saliera entre **0,95× y 1,05×** sobre una cantidad
cuyo recorrido real es del **134 %** —de 0,759× a 1,776×—. No estaba mal
calibrada: pedía que fuera constante una razón entre **un bucle y una versión
desenrollada por construcción**, y desenrollar 528 productos en línea recta
(N=32) no cuesta por producto lo mismo que desenrollar 6 (N=3).

**Lo que la sustituye** es el coste por **producto de limbo**, que es lo que sí
debería ser plano y que el fichero ya sabía calcular (`productos_escolar`) sin
usarlo nunca para controlar nada:

| N | biblioteca | escolar | desenrollado | salto |
|---|---|---|---|---|
| 9 | 1,722 | 6,469 | 2,354 | 0,81× |
| 10 | 1,723 | 7,265 | 2,353 | 1,00× |
| 12 | 1,706 | 8,280 | 2,216 | 0,99× |
| 16 | **4,376** | 9,009 | **3,910** | **2,56×** |

El bucle escolar escala liso de punta a punta. La biblioteca y la referencia
desenrollada van a la par hasta N=12 y **caen por un escalón en N=16**. El
umbral (×1,5) está **calibrado**, no elegido: en el tramo liso el mayor salto
consecutivo es ×1,06 y el escalón vale ×2,56, así que deja ~40 % de margen por
los dos lados.

**No aborta, y es deliberado**: el escalón es una propiedad reproducible, no un
fallo de medida. Un control que lo convirtiera en error estaría rojo siempre y a
la semana nadie lo miraría. Abortar se reserva para lo imposible, que es
`verosimil`.

**Y el escalón no está donde está la frontera del despacho.** `operator*` no
cambia de camino hasta N=22; el código generado se rompe en N=16. Que no
coincidan es lo interesante, y queda apuntado.

#### 7. Lo que salió de filtrar el barrido: el fichero etiquetaba mal

Al separar los N que usan Karatsuba de los que no, **N=4 y N=8 quedaron fuera y
N=32 dentro** — al revés de lo que dice el fichero. Los umbrales reales:

    NSTD_KARATSUBA_MIN   22      NSTD_DESENROLLA_MAX  21
    NSTD_KARATSUBA_MAX   4096

o sea: N=2 camino especializado, **N=3..21 escolar desenrollado**, N≥22
Karatsuba —y para **cualquier N**, no sólo potencias de dos, desde que entró el
reparto equilibrado el 16 sep—. **De todo lo que mide el fichero, el único que
usa Karatsuba es N=32.** La tabla llevaba dos semanas imprimiendo «`<-
Karatsuba`» junto a N=4 y N=8 en cada ejecución.

El header ya se había corregido —hay allí un comentario del 18 sep que llama a
su propia versión anterior «tres afirmaciones falsas en cinco líneas»— y el
benchmark se quedó atrás. **El arreglo de raíz no es cambiar las etiquetas**,
que es lo que envejeció: `regimen(N)` las deduce de las macros. Falsificado
recompilando con `-DNSTD_KARATSUBA_MIN=4`: nueve etiquetas se mueven solas.

**Queda pendiente, y no es de una línea:** `productos_karatsuba()` modela el
reparto por potencias de dos (`3^log2(N/2)`), que ya no es la implementación.
Alimenta las columnas `razon esperada` y `sin explicar`, así que **esas dos no
se pueden leer hoy para N ≥ 22**. Rehacerlo pide derivar la cuenta del reparto
equilibrado. Está avisado en el propio `@warning` de la función.

### ✅ P2.5 — Histórico de benchmarks (cerrada el 27 sep 2026)

**Más frecuente no es mejor si las medidas no son comparables.** Una cifra de un
runner compartido y otra de esta máquina no van en la misma serie, y una serie
con medidas incomparables es peor que no tener serie: invita a leer tendencias
que no existen.

1. ✅ Línea legible por máquina: existía, y **tiraba lo importante**. Guardaba un
   `double` pelado teniendo al lado la dispersión que el arnés adaptativo ya
   calculaba. Ahora `bench_record` lleva cuatro columnas más —opcionales, para
   no romper las líneas de tres— y `bench::registra(caso, Medida)` las rellena.
2. ✅ `benchs/history/<máquina>/`: existía. Lo que no existía era **contenido**:
   7 de 23 suites habían registrado alguna vez y la última toma era del 9 sep.
   Hoy hay una toma de **975 medidas de las 23**.
3. ✅ Comparación **con el ruido de cada casilla**, no con un umbral único: la
   barra es la suma de los dos recorridos, y el plano del 25 % queda para las
   medidas que no lo traen. **Calibrado** con dos tomas del mismo código: 0
   falsos positivos, frente a los 26 del umbral plano.
4. ⏸ **Frecuencia: lo único que queda, y se ha convertido en P2.16.** No es que
   el CI no mida: es que **no compila** los benchmarks, y por eso uno estuvo
   diez días roto.

**Las cuatro causas del atasco, que no eran una.** Al diagnosticarlo salieron
separadas, y mezclarlas ya había costado caro en septiembre:

| | |
|---|---|
| **5** no llamaban a `bench_record` | imprimen su tabla a mano |
| **1** no compilaba | `benchmark_cuadrado.cpp`, desde el 17 sep |
| **3** tardan más de 150 s | nadie los había dejado terminar |
| **14** iban bien | nadie los había vuelto a correr |

**Y dos hallazgos que no se buscaban.** `benchmark_cuadrado.cpp` medía
`sqr_escolar_bucle`, un núcleo **retirado por medida** el 16 sep; el fichero
llevaba diez días sin compilar y nadie lo vio. Y el barrido de la perilla `Base`
de `rango_alto` —el banco que justifica el 8 que lleva escrito el código— **no
registraba nada en absoluto**.

---



## 1.90, fase 0: **afinar el banco antes de tocar la biblioteca**

Decidido el 27 sep. **Va antes del borrado de `int128_param_*` y antes del
renombrado**, y no por gusto de orden: el banco es **el instrumento con el que se
va a juzgar todo lo que haga la 1.90**. Con el instrumento sin calibrar, una
regresión y un artefacto del arnés son indistinguibles — y la 1.90 borra
dieciséis cabeceras y renombra la plantilla principal, que es justo cuando más
falta hace poder distinguirlos.

Hay **tres señales independientes** de que no está afinado, y las tres salieron
de mirar las cifras, no de sospechar:

1. **El banco de comparaciones se autodesmiente** (P2.19). `uint64_t` y
   `unsigned __int128` salen más lentos que los tipos de 128 bits. De los siete
   bucles, cinco escriben `if (r) a += 1;` y dos escriben `a += r;` sobre una
   `volatile bool`: la segunda forma mete el reenvío de almacén a carga dentro de
   la cadena de dependencia del bucle.
2. **Hay cifras marcadas `(*)` que nadie lee.** El propio banco avisa al pie de
   que son un «patrón sin acumulación, sin dependencia de bucle», y con eso
   TomMath aparenta multiplicar en **0,72 ciclos/op** — más rápido que `uint64_t`.
   La marca existe; lo que no existe es que se respete al leer la tabla.
3. **Tres suites miden con una ventana demasiado corta** (P2.18): `hueco`,
   `equilibrado` y `barrido_desenrollado` salen las peores por dos caminos
   independientes —vueltas limpias y movimiento entre tomas—.

Y hay una cuarta, de otra clase: **el umbral del cuadrado no se sostiene con
estas cifras**. `operator*` desvía `x·x` al núcleo de cuadrado desde N=4 apoyado
en un «gana desde N=4, 2,47×» medido con clang; con gcc 16 el desvío *cuesta*
1,45× en N=4 y no empata hasta N=12. Eso no es un fallo del banco sino una
decisión que hay que volver a medir, y hace falta el banco afinado para hacerlo.

### El orden, y qué significa para las series de cifras

1. **Afinar** (P2.19, P2.18, P2.17 y la lectura del `(*)`).
2. **Tomar una referencia nueva** con el banco ya afinado. Empieza serie: las
   cifras del arnés afinado **no son comparables** con las de antes, igual que no
   lo eran las de 10 y 25 vueltas.
3. **Entonces** borrar, renombrar y medir el antes y el después contra esa
   referencia.

**La toma del 27 sep no se pierde ni se repite**: es el registro de la 1.80 tal
como se publicó, y la última que incluye los nueve bancos del tipo viejo. Para
eso se hizo antes de abrir la rama.

---


## Deprecar de verdad el tipo viejo (29 sep 2026)

**Hoy la deprecación es una etiqueta para el de fuera.** Medido:

| | |
|---|---:|
| ficheros que definen `NSTD_SILENCIA_INT128_PARAM_DEPRECADO` | **45** (9 bancos + 35 tests + la cabecera) |
| usos de los cuatro alias internos sin marcar | **161** en 6 cabeceras |
| avisos de deprecación que ve el proyecto al compilarse | **0** |

O sea que **nadie dentro del proyecto ve nunca un aviso**, por dos válvulas: los
alias internos (`uint128_interno_t` y los tres hermanos), que no llevan el
atributo, y un macro que silencia por fichero.

### Lo que la medida dice, y que cambia el trabajo

**Ninguna cabecera que sobreviva a la 1.90 depende del tipo viejo.** Las 17
apariciones de `int128_param` en `fixed_width_int_t.hpp`, `representation.hpp` y
`div_kernels.hpp` son **comentarios en prosa** —comparaciones con el tipo que se
retira—, ni una línea de código. Y las dos cabeceras que sí lo usan
—`algorithms/karatsuba.hpp` y `algorithms/div_by_const.hpp`— sólo las incluye la
propia familia `int128_param*`, así que **mueren con ella**.

Conclusión: la superficie es **conocida y cerrada**, y toda está programada para
borrarse. No hay código de biblioteca que migrar.

**Entonces lo que falta no es migrar: es que la lista no crezca.** El macro es
por fichero, invisible y sin contar — un test nuevo que use el tipo viejo añade
su `#define` y nadie se entera.

### ✅ Hecho el 29 sep: niveles 3 y 1

**Nivel 3** (`10ec519`): la familia no se sirve por defecto. Hay que declarar
`NSTD_QUIERO_INT128_PARAM`, y sin él la compilación para con un `#error` que
apunta al tipo nuevo y a la guía. **Dos puertas**, porque
`int128_param_divmod.hpp` no incluye a nadie y compila sola.

**Nivel 1**: el armonizador cuenta la superficie y no la deja crecer.

| | hoy | invariante |
|---|---:|---|
| ficheros que declaran `NSTD_QUIERO_INT128_PARAM` | **52** (9 bancos, 6 demos, 37 tests) | techo: sólo puede bajar |
| ficheros **de fuera de la familia** que usan `*_interno_t` | **0** | cero duro |

El segundo no es un techo porque no hay nada que bajar: los alias internos son
asunto interno de la familia, y que los usara alguien de fuera sería abrir una
tercera válvula justo después de cerrar dos.

Falsificado con tres averías —un fichero nuevo que declara, un alias interno
desde fuera, y las dos a la vez—: las tres en rojo, **con el nombre del
culpable**, y vuelve a verde al deshacerlas.

**Queda el nivel 2** (cerrar los alias internos y que el proyecto se avise a sí
mismo), y sigue sin merecer la pena: los 52 ficheros mueren con la familia en el
tramo 3, así que forzar su migración es trabajo tirado. Se reconsidera si el
tramo 3 se retrasa.

### Los tres niveles, tal como se plantearon

1. **Trinquete sobre las dos válvulas.** Contar los 45 ficheros que silencian y
   los 6 que usan alias internos, y que **sólo puedan bajar**. Cuesta unas líneas
   en el armonizador, no compila nada y hace visible la superficie. *Esto se
   puede hacer ya.*
2. **Cerrar los alias internos**: quitarlos y dejar que el proyecto se avise a sí
   mismo. Sabiendo que los 45 ficheros mueren en el tramo 3, forzar su migración
   sería trabajo tirado — es la misma regla que «no se documenta lo que se va a
   borrar».
3. **Dejar de servirlo por defecto**: que `int128_parameterized.hpp` exija un
   `#define NSTD_QUIERO_INT128_PARAM` para compilar. Es el paso estándar antes de
   retirar algo, y el único que hace que la deprecación **muerda de verdad** a
   quien la usa desde fuera. Rompe a los que no reaccionaron al `[[deprecated]]`,
   que es justamente el objetivo de una deprecación.

**El 3 es el que responde a «de forma real»**, y conviene decidirlo antes del
borrado: si el tramo 3 llega sin que nadie haya tenido que reaccionar, la
deprecación no habrá servido de nada — se habrá pasado de «está marcado» a «ya no
está» sin escalón intermedio.

---

## Nomenclatura del punto fijo — seis decisiones tomadas (27 sep 2026)

**Van en la 1.90, en la misma pasada que el renombrado de `fixed_int_t` a
`fixed_width_int`.** Renombrar después de publicar la API cuesta mucho más, y las
dos listas se tocan. Material de apoyo en
[NOMENCLATURA_PUNTO_FIJO.md](docs/NOMENCLATURA_PUNTO_FIJO.md), con las seis
fuentes leídas y marcadas por confianza.

| hoy | pasa a | de dónde, y qué gana |
|---|---|---|
| `desde_crudo` | **`from_rep`** / **`to_rep`** | P0037 y CNL. Es el par canónico en C++ para «construir desde la representación» y su inverso, y hoy sólo existe la mitad |
| `escala_*` | **`scale_up`** / **`scale_down`** | P0105. Y `scale_down` es el que lleva el redondeo: la misma asimetría que ya decidió [ADR-020](docs/decisions/) por otro camino |
| los cinco modos de redondeo | **`all_*` / `tie_*`** | P0105 separa los que se aplican a **todo valor no representable** de los que sólo actúan **en el empate**. Los nuestros son cuatro `all_*` y un `tie_*`, y el nombre actual no lo distingue |
| `overflow_policy::checked` | **`overflow::special`** | P0105. `checked` dice que se comprueba; `special` dice **qué pasa** al desbordar, que es lo que hay que saber en el punto de uso |
| — | **`countls`** | TR 18037. Operación que **falta**, y hace falta para normalizar antes de multiplicar |
| `F == N` | **especialización propia** | TR 18037 le da tipo propio al caso puramente fraccionario (`_Fract` frente a `_Accum`). Aquí sería `fixed_point<F, F>`, y **no es sólo un nombre**: ese caso se comporta distinto, y es justo el que tuvo el fallo de `to_string`. Con especialización, la diferencia está en la firma en vez de en un `if` |

### Falta una decisión en el renombrado: **los nombres de fichero**

Anotado el 30 sep, mirando el árbol. El plan dice que la 1.90 «renombra la
plantilla principal» y no dice nada de las cabeceras, y ahí la familia del entero
se quedó a medias:

| familia | cabecera principal | clase | satélites | ¿cumple? |
|---|---|---|---|---|
| punto fijo | `fixed_point_t.hpp` | `fixed_point_t` | `fixed_point_*.hpp` | **sí** |
| entero | `fixed_width_int_t.hpp` | `fixed_int_t` | `fixed_int_*.hpp` (7) | **no** |

La convención de la casa es **nombre de cabecera = nombre de la clase**, y la del
punto fijo la cumple entera. La del entero la rompió cuando la cabecera principal
pasó a `fixed_width_int_t.hpp` sin que la siguieran ni la clase ni los siete
satélites —que hoy concuerdan con la clase, `fixed_int_t`, y no con su propia
cabecera principal—.

**Decidido el 30 sep, y escrito como
[ADR-023](docs/decisions/ADR-023-sin-sufijo-t-en-las-plantillas-de-clase.md):**
`_t` marca un **alias**, no una plantilla de clase — que es lo que hace la `std`,
donde `remove_cv` y `remove_cv_t` coexisten y el sufijo es justo lo que los
distingue. Así que:

| hoy | pasa a | |
|---|---|---|
| `fixed_int_t` | **`fixed_width_int`** | 925 sitios |
| `fixed_point_t` | **se queda**, como alias de `scaled_integer` ([ADR-024](docs/decisions/ADR-024-el-nombre-del-tipo-de-punto-fijo.md)) | **0 sitios** |
| `fixed_point_t` (la clase) | **`scaled_integer`** | interno |
| `fixed_width_int_t.hpp` | **`fixed_width_int.hpp`** + `fixed_width_int_*.hpp` (7) | |
| `fixed_point_t.hpp` | **`scaled_integer.hpp`** + `scaled_integer_*.hpp` (5) | |

Los alias **conservan** su `_t` —`int_fixed_t`, `uint_fixed_t`,
`ufixed_point_t`, `sfixed_point_t`— y no por inercia: es que ahora significa
algo. Y desaparece la trampa de la transposición, que era la razón original:
`fixed_width_int` frente a `int_fixed_t` ya no se confunden.

Los dos nombres nuevos están **libres**: cero apariciones de `fixed_width_int` y
de `fixed_point` como identificadores en `include/`.

**Coste**: medido el 30 sep en **1.277 apariciones**, y ADR-024 lo baja a **~1.047**
al dejar `fixed_point_t` como alias — los 925 de `fixed_int_t`, los 122 de los
nombres de fichero `fixed_int_*.hpp` (en 25 ficheros: 12 en `include/`, 9 en
`tests/`, 8 en `docs/`, 2 en `scripts/` y los cuatro de raíz) y los de
`fixed_point_*.hpp`, pero **ninguno** de los 230 que escriben el nombre del tipo.
Mecánico, pero **no es un `sed`**: toca el verificador de cabeceras
autocontenidas, los `API_*.md`, el `Doxyfile` y los `#include` de tests, demos y
bancos.

**Va en la misma pasada que el renombrado de la plantilla**, no antes: hacerlo
ahora obliga a renombrar dos veces, porque el nombre de la clase todavía cambia.
Y sigue valiendo el orden de la fase 0 —afinar el banco, tomar referencia, y
*entonces* borrar y renombrar—, que existe para que una regresión y un artefacto
del arnés no se confundan justo cuando más se parecen.

**El nombre del tipo: decidido el 1 oct, y es
[ADR-024](docs/decisions/ADR-024-el-nombre-del-tipo-de-punto-fijo.md).** Las dos
candidaturas tenían razón en cosas distintas —`scaled_integer` describe la
implementación y es a donde llegó McFarlane tras seis revisiones de P0037;
`fixed_point` es el término que usa el dominio y por el que la gente busca—, así
que van **las dos, con jerarquía**: la clase es `scaled_integer` y
`fixed_point_t` sobrevive como **alias**.

Y eso no dobla la regla de ADR-023, la cumple: `_t` marca un alias, y aquí
`fixed_point_t` **es** un alias. Lo que significa que **sus ~230 sitios no se
tocan** y el renombrado de la 1.90 baja de 1.277 a ~1.047 — un 18 % menos, y lo
que desaparece es justo la parte que toca código de usuario.

**Y una advertencia para el ADR:** Ada exige **conversión explícita al multiplicar
dos fijos**, porque el tipo del resultado es ambiguo. Aquí decidimos lo contrario
—promocionar parte entera y fraccionaria por separado— y la decisión se sostiene,
porque la anchura es un parámetro de plantilla y el resultado **sí** se puede
nombrar, mientras que en Ada el `'Small` real lo elige el compilador. Pero el ADR
tiene que decir que Ada decidió al revés y por qué aquí no aplica: tomar la
decisión sabiéndolo es distinto de tomarla sin saberlo.

---

## Cosas que conviene no redescubrir

- **El armonizador se ejecuta con `--doxygen`.** Sin ese flag son 7
  comprobaciones; el CI hace 9. Correrlo sin él y cantar «7/7» fue lo que dejó el
  CI en rojo cinco commits sin que nadie lo viera.
- **doxygen no cuenta igual según la versión.** 1.9.8 (CI) da 518 avisos y 1.18.0
  (local) da 499 sobre el mismo árbol. Por eso el techo de
  `check_docs_consistency.py` es **una cifra por versión**.
- **clang-format: en local la 22.1.8, en el CI la 21.** El árbol es punto fijo de
  las dos, medido; la regla es que lo siga siendo. La serie 19 sí lo rompe.
- En Windows, **`g++` y `clang++` a secas NO son los del proyecto**: resuelven al
  toolchain MSYS. Usar `python scripts/toolchains.py`.
- El `Makefile` es un **shim** sobre `make.py`, que es la capa canónica.
- **`make.py build` ya sí detecta el fallo de enlazado** (desde `05ba169`). Antes
  no, y todo lo construido encima heredaba la mentira.
- **Los `run:` de GitHub Actions van con `bash -e`.** Un comando que devuelve
  distinto de cero **fuera de un `if`** mata el paso en el acto. Sacar un
  `timeout` de su `if` para guardar `$?` dejó el job de aarch64 muerto a mitad
  del bucle. La forma correcta es `code=0` y luego `cmd || code=$?`, que deja el
  comando comprobado y a la vez guarda el código.
- **Intel: `icpx` usa flags estilo GCC**, no de MSVC — los `/Qstd:c++20` y
  `/EHsc` son de `icx-cl`. Y necesita el entorno que monta `compiler_env.py`;
  invocarlo a pelo da «`'cstddef' file not found`» y no significa nada.
- **No se toca `compiler_env.py` con una compilación en marcha.** Cada test se
  compila en un proceso nuevo que relee el módulo, así que el cambio se cuela a
  mitad de la suite y el resultado sale mezclado sin que nada lo diga.
- **QEMU está en WSL** (`qemu-arm`, `qemu-aarch64`, y las toolchains cruzadas).
  Reproducir un fallo de ARM en local cuesta minutos; por el CI, media hora.
- **Para probar que una comprobación salta, primero se commitea el arreglo.**
  Al validar los `assert()` se rompió un test a propósito y se restauró con
  `git checkout --`, que deshizo la rotura **y también el arreglo**, porque
  todavía no estaba commiteado. El fichero volvió a quedar inerte y la prueba
  dijo «siguen inertes» cuando lo que fallaba era el procedimiento.
- Antes de cerrar sesión: `/guarda_y_sube`, que ejecuta los cuatro verificadores
  que exige el CI.


### P0.9 — Tests de guardas de include y de macros de configuración

**De dónde sale.** De que en tres sesiones han aparecido once señales que
mentían, y las que más caras han salido no eran fallos del código sino de la
**configuración**: una guarda de libc++ que causaba el problema que decía evitar
y dejaba al proyecto sin `nstd::is_integral` durante meses, un
`[[no_unique_address]]` que elegía la rama equivocada porque Intel define
`_MSC_VER` **y** `__clang__` a la vez, y un `-latomic` que se añadía a ciegas.
Ninguna de las tres la habría cazado un test de aritmética.

**Qué tiene que comprobar**, y todo en `static_assert` donde se pueda:

1. **Qué hay debajo, dicho en voz alta.** Compilador, versión y biblioteca
   estándar (`__GLIBCXX__` / `_LIBCPP_VERSION` / `_MSVC_STL_VERSION`). Que el
   test *imprima* en qué combinación corre es media batalla.
2. **Las macros propias resuelven a lo que deben** en esa combinación:
   `INTRINSICS_USES_MSVC_ABI` frente a `INTRINSICS_USES_GNU_ABI`,
   `NSTD_NO_UNIQUE_ADDRESS`, `INTRINSICS_IS_CONSTANT_EVALUATED`,
   `NSTD_TRAITS_PRIMARY_DEFINED`.
3. **Lo que existe, existe en todas las combinaciones**: `nstd::is_integral` y
   sus hermanas, `nstd::hash`, `nstd::numeric_limits`, para `wrap` **y** para
   `checked`. Es el agujero del 6 sep 2026, y como `static_assert` no se puede
   volver a abrir en silencio.
4. **Los contratos que dependen de la ABI**: `sizeof(fixed_int_t<4,…,wrap>) ==
   32`, `== 40` con `checked`, `is_standard_layout` en las dos. Hoy viven
   sueltos en la cabecera del tipo.
5. **Las guardas son idempotentes**: incluir dos veces cada header no cambia
   nada, y **el orden entre los dos ficheros de traits tampoco** — que es donde
   vive `NSTD_TRAITS_PRIMARY_DEFINED` y donde estaba el fallo.

**La vuelta de tuerca que lo hace útil:** el test debe **fallar si no reconoce la
combinación**, no pasar de largo. Una configuración nueva y no contemplada tiene
que salir en rojo. Si no, es otro verde que no significa nada.

### P2.6 — La tercera combinación: `clang + libstdc++`

Hay **tres** combinaciones compilador/biblioteca disponibles en esta máquina, y
solo se usaban dos. Medido el 6 sep 2026 con una sonda que imprime los macros:

| toolchain | compilador | biblioteca |
|---|---|---|
| `ucrt64/g++` | gcc 16.2.0 | libstdc++ 20260807 |
| `ucrt64/clang++` | clang 22.1.8 | **libstdc++ 20260807** ← la que faltaba |
| `clang64/clang++` | clang 22.1.8 | libc++ 220108 |

La tercera **ya está instalada**: es el clang de UCRT64, con el que se trabajó
media sesión sin caer en que era una configuración distinta. Añadirla es
cuestión de que `toolchains.json` la nombre y `make.py test` la acepte.

**La cuarta, `gcc + libc++`, no es viable en MinGW y se descarta.** No por culpa
del proyecto: un hola-mundo compila (`gcc 16.2.0 / libc++ 220108`, con
`-nostdinc++ -isystem …/include/c++/v1 -lc++`), pero con código real revienta
dentro de la propia libc++ —`__algorithm/equal.h` usa un `static` en función
`constexpr`, que es de C++23, y GCC lo rechaza en modo C++20; forzando C++23 cae
en `cstdlib`, porque UCRT no tiene el `aligned_alloc` de C11—. Con las cabeceras
de UCRT64 y con las de CLANG64, igual. Queda como job experimental del CI en
Linux si algún día interesa, nunca como requisito.


### P2.7 — Desguace de benchmarks de algoritmo

**De dónde sale.** De que la comparación Karatsuba/escolar ha medido tres cosas
distintas de lo que decía medir, en tres ocasiones:

| Cuándo | Decía | Medía |
|---|---|---|
| ago 2026 | Karatsuba gana 6,23× | contra un espantapájaros |
| ago–sep 2026 | Karatsuba gana 1,65× | un desenrollado contra un bucle |
| 6 sep 2026 | escolar desenrollado, 2,1 cyc/op en N=4 con Intel | el compilador se había llevado el trabajo |

Las tres se destaparon **mirando**, no por una alarma. Un número solo, sin nada
con qué contrastarlo, no puede delatarse a sí mismo. El desguace consiste en no
publicar nunca un número solo.

**Las cuatro piezas, y cuáles ya existen.**

**1. Aportación del algoritmo, aislada del desenrollado.** ✅ *hecho el 6 sep
2026*. Se miden **tres** implementaciones y no dos: la de la biblioteca, la
referencia escrita como bucle, y la misma referencia desenrollada por
construcción. Se publican `razon justa` (los dos lados desenrollados) y `aporte
del desenrollado` (bucle frente a desenrollado). Sin esto, el «1,65×» de
Karatsuba resultó ser 0,59×.

**2. Verosimilitud: el suelo físico.** ✅ *hecho el 6 sep 2026*. Se compara la
cifra medida con lo que la máquina no puede bajar: para el escolar de N limbos,
N(N+1)/2 productos por su coste mínimo. Por debajo del suelo, la medida **no se
publica y el benchmark sale con error**. Ojo con el suelo: `CycleTimer` usa
RDTSC, que cuenta a la frecuencia invariante del TSC y no a la del núcleo, así
que con turbo un ciclo real mide menos de un «ciclo» TSC.

**3. Aportación del compilador: contar el código emitido.** ✅ *hecho el 6 sep 2026*, en `scripts/bench_asm.py`. Lo
que destapó el fallo del desenrollado no fue el cronómetro sino el
`-S`: en GCC, N=4, Karatsuba salía con 119 instrucciones y **9** `mul`
—desenrollado— y la referencia con 61 y **1** `mul` —bucle—. Esa cuenta debe
tomarse automáticamente y publicarse junto al tiempo:

- compilar la sonda con `-S` (o `/FA` en MSVC), trocear por función y contar
  instrucciones, `mul`, `adc`/`sbb`, `mov` y `call`;
- **la señal es la discordancia**: si el tiempo dice 1,7× y el número de `mul`
  dice 0,9×, hay una tercera variable, y hay que nombrarla antes de publicar.
- `call` dentro de un núcleo numérico es aviso por sí solo: MSVC dejaba
  `kmul_full` fuera de línea.

**4. Coste teórico declarado, y su distancia con lo medido.** ✅ *hecho el 6 sep 2026*.
Cada algoritmo declara su cuenta de operaciones —escolar N(N+1)/2 productos;
Karatsuba T(N)=3·T(N/2)+O(N)— y el benchmark publica la razón **esperada** al
lado de la **medida**. La distancia entre ambas es el resultado interesante: es
donde viven el desenrollado, la presión de registros, la planificación y los
fallos de medida. Hoy esa distancia se calcula a mano y sólo cuando alguien
sospecha.

**Forma que propongo.** Un `benchs/bench_desguace.hpp` con el arnés —las tres
medidas, la verosimilitud y el coste declarado— y un `scripts/bench_asm.py` que
haga la parte 3, porque necesita invocar al compilador y trocear el ensamblador.
El benchmark de Karatsuba sería el primer cliente; `divmod` y las conversiones a
cadena, los siguientes.

**Regla que sale de esto, y que vale aunque no se escriba una línea de código:**
un benchmark de algoritmo no publica una razón sin decir **contra qué** y
**compilada cómo**. Las tres veces que esto falló, la razón estaba bien
calculada; lo que estaba mal era el otro lado de la división.
