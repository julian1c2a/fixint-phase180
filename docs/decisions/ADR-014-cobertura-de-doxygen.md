# ADR-014: La cobertura de Doxygen se mide de verdad, y se cierra por etapas

**Estado:** ✅ Aceptado
**Fecha:** 25 August 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

El proyecto venía diciendo, en `PROJECT_STATUS.md` y en el armonizador, que hay
**«0 avisos de Doxygen desde `include/`»**. Es cierto, y **no significa nada**.

El `Doxyfile` tiene:

```
EXTRACT_ALL            = YES
WARN_IF_UNDOCUMENTED   = NO
WARN_NO_PARAMDOC       = NO
```

`EXTRACT_ALL = YES` hace que Doxygen extraiga también lo que no está
documentado, y `WARN_IF_UNDOCUMENTED = NO` le dice que no avise de ello. Con esa
configuración **es imposible tener un aviso de cobertura**, esté la biblioteca
documentada o no. Los ceros no medían la documentación: medían que la
comprobación estaba apagada.

`AI-GUIDE.md` §26, además, especifica lo contrario (`EXTRACT_ALL = NO`,
`WARN_IF_UNDOCUMENTED = YES`). La guía y el `Doxyfile` llevaban tiempo
contradiciéndose.

## La medida real

Activando la comprobación sobre `include/`, el 25 ago 2026:

**891 avisos de cobertura.** Repartidos así:

| Cabecera | Avisos |
|---|---:|
| `fixed_width_int_t.hpp` | **257** |
| `int128_param_limits.hpp` | 136 |
| `int128_parameterized.hpp` | 118 |
| `int128_param_traits_specializations.hpp` | 37 |
| `fixed_int_limits.hpp` | 34 |
| `int128_param_thread_safety.hpp` | 30 |
| `intrinsics/compiler_detection.hpp` | 29 |
| `int128_param_divmod.hpp` | 28 |
| resto | 222 |

Y dentro del tipo insignia, la forma importa más que el número:

| Qué | Cuántos |
|---|---:|
| **Sobrecargas heterogéneas de operador** | **182** |
| Miembros y funciones libres | 61 |
| Alias, enumerados y `@param`/`@return` sueltos | 14 |

## Decisiones

### 1. La comprobación se enciende

`EXTRACT_ALL = NO` y `WARN_IF_UNDOCUMENTED = YES`. Sin eso no hay cobertura que
medir, solo una cifra que da cero por construcción.

**`WARN_AS_ERROR` se queda en `NO` mientras haya deuda.** Encender la puerta con
891 avisos pendientes dejaría el CI en rojo indefinidamente, y un CI que siempre
está rojo no lo mira nadie. La puerta se cierra cuando el ámbito de la decisión 3
llegue a cero.

### 2. Las 182 sobrecargas de operador se documentan **en grupo**, no una a una

Son las que existen porque [ADR-001](ADR-001-constructores-y-conversiones-explicitos.md)
prohíbe las conversiones implícitas: `operator+(uint_fixed_t<N>, T)`,
`operator+(T, uint_fixed_t<N>)`, las variantes con `__int128`, las de N distinto.
Ponerle a cada una un `@brief` propio produciría 182 frases idénticas, que es
ruido con aspecto de documentación: hace el HTML más largo y no informa de nada.

Se usan grupos de Doxygen (`@name` / `@{` … `@}`), con **una descripción por
familia** que diga lo que de verdad hay que saber: qué tipo resulta de la mezcla,
si la aritmética es modular, y qué operadores no son `noexcept`. Un lector quiere
entender la familia, no leer 182 veces la misma frase.

> **Medido, y no salió como se esperaba.** Al aplicarlo, los avisos de
> `fixed_width_int_t.hpp` bajaron de **257 a 224**: los grupos documentan la
> familia para el lector, pero **`WARN_IF_UNDOCUMENTED` sigue contando cada
> miembro por separado**. Doxygen no sabe expresar «documentado como familia».
>
> No se cambia la decisión por eso: la descripción por familia es lo correcto
> para quien lee, y generar 182 `@brief` mecánicos sería justo lo que la
> decisión 4 prohíbe. Lo que cambia es **la métrica**: los miembros que están
> dentro de un `@name` con descripción **cuentan como documentados**, y el aviso
> de Doxygen sobre ellos se ignora a sabiendas. La cifra de la que se responde
> es la de los miembros **fuera** de un grupo documentado.

### 3. El ámbito que hay que cerrar es `fixed_int_t`, no la biblioteca entera

Se documentan `fixed_width_int_t.hpp` y los `fixed_int_*.hpp`. **La familia
`int128_param_*` queda exenta**, y no por pereza: [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md)
decide que ese tipo **se retira**. Documentar a conciencia 349 miembros de un
tipo que va a desaparecer es trabajo que se tira, y además retrasa la migración
que lo elimina.

La exención es explícita y con fecha de caducidad: desaparece cuando desaparezca
`int128_param_t`.

### 4. Lo que cuenta como documentado

No basta con que Doxygen calle. Un `@brief` que repite el nombre de la función
—«@brief Suma dos valores», sobre `operator+`— cumple la herramienta y no
informa. La regla del proyecto, la misma que se aplica a los comentarios del
código: **se documenta lo que no es evidente**. Precondiciones, qué pasa al
desbordar, si es `constexpr`, qué excepción puede salir y cuándo, y las
sorpresas —que `/` y `%` no son `noexcept`, por ejemplo.

## Enmienda (10 sep 2026): qué cuenta como público

La decisión original decía «todo `include/`», y eso metía en el ámbito dos
headers que **nadie incluye directamente**:

| Header | Avisos | Qué es |
|---|---|---|
| `intrinsics/compiler_detection.hpp` | 29 | Detección de compilador y de intrínsecos disponibles |
| `algorithms/karatsuba.hpp` | 3 | Implementación de `kmul_full`, llamada desde `operator*` |
| `intrinsics/arithmetic_operations.hpp` | 1 | Acarreos y productos de 64 bits |

**Son 33, no 32.** La cifra que se manejaba al plantear la decisión eran los dos
primeros headers; al medirla apareció un tercero. Por eso el criterio se escribe
sobre **directorios** y no sobre una lista de ficheros: una lista se queda corta
en cuanto alguien añade un header.

**Quedan fuera del ámbito.** Son implementación: los subdirectorios
`intrinsics/` y `algorithms/` existen precisamente para separarlos de la API que
se instala y se documenta. Documentarlos al nivel de un header público es
trabajo sobre código que nadie va a leer desde fuera.

El criterio, para que no haya que decidirlo caso por caso otra vez: **entra en
el ámbito lo que un usuario puede incluir**, es decir lo que está en la raíz de
`include/`. `intrinsics/` y `algorithms/` no.

Lo que esto desbloquea es lo que importa: con el ámbito público en cero avisos,
`WARN_AS_ERROR = YES` pasa a ser posible (P3.2), y eso **cierra la puerta** a
que la cobertura se degrade otra vez en silencio — que es exactamente lo que
había pasado antes de este ADR, cuando `EXTRACT_ALL = YES` hacía imposible que
apareciera un aviso y el «0» que se publicaba no significaba nada.

**El riesgo asumido**: la frontera entre «público» e «interno» se difumina con
el tiempo. Si algún día un header de `intrinsics/` pasa a ser parte de la API,
hay que moverlo a la raíz de `include/` o volver a meterlo en el ámbito a mano.

## Consecuencias

### Positivas

- La cifra de cobertura pasa a medir algo. Antes era un cero decorativo.
- El trabajo queda acotado: 257 avisos en el ámbito real, de los que 182 se
  cierran con un puñado de grupos.
- La guía y el `Doxyfile` dejan de contradecirse.

### Negativas

- **Aparecen 891 avisos donde antes había 0.** Es una regresión aparente que hay
  que explicar cada vez que alguien mire el CI: no ha empeorado nada, se ha
  encendido la luz.
- La puerta (`WARN_AS_ERROR`) queda abierta durante un tiempo, y una puerta
  abierta no protege de nada. El compromiso es cerrarla al llegar a cero en el
  ámbito de la decisión 3.

## Referencias

- `Doxyfile`.
- `AI-GUIDE.md` §26, que especificaba esta configuración desde el principio.
- [ADR-001](ADR-001-constructores-y-conversiones-explicitos.md): por qué existen
  las 182 sobrecargas.
- [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md): por qué se exime a
  `int128_param_*`.
