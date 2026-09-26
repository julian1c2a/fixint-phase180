# Arquitectura interna: el mapa de las capas

**Última actualización:** 26 September 2026

> **Esto no es API pública.** Nada de lo que hay aquí está pensado para que lo
> incluya quien usa la biblioteca, y puede cambiar sin aviso
> ([ADR-014](decisions/ADR-014-cobertura-de-doxygen.md)).
>
> **Pero está documentado, y a propósito.** «Fuera del ámbito público» no es
> «sin documentar»: estas capas son las que tienen el código más difícil del
> proyecto —los núcleos de división y multiplicación, la detección de
> compilador— y su conocimiento es justamente el caro de perder.

## Para qué sirve este documento

La referencia **por símbolo** ya existe: los headers internos llevan sus
comentarios Doxygen completos desde P3.7, y Doxygen los procesa. El **porqué**
de los algoritmos también: está en
[PERFORMANCE.md](PERFORMANCE.md) —1600 líneas de medidas— y en
[ESTUDIO_ALGORITMOS_RAPIDOS.md](ESTUDIO_ALGORITMOS_RAPIDOS.md).

Lo que faltaba es **el mapa**: qué capas hay, quién llama a quién, y dónde vive
la decisión de cada una. Sin él hay que reconstruirlo leyendo `#include`, que es
lo que este documento evita.

---

## Las tres capas, de abajo arriba

```
                    fixed_width_int_t.hpp          <- API publica
                              |
        +---------------------+---------------------+
        |                                           |
  algorithms/                                 representation.hpp
  mul_kernels.hpp   div_kernels.hpp                 |
        |                 |                         |
        +--------+--------+                         |
                 |                                  |
          intrinsics/                               |
   arithmetic_operations.hpp    bit_operations.hpp  |
                 |                    |             |
                 +---------+----------+             |
                           |                        |
              compiler_detection.hpp                |
              fallback_portable.hpp                 |
```

**El cierre transitivo de `fixed_width_int_t.hpp` son ocho cabeceras**, contadas
recorriendo los `#include`:

    arithmetic_operations.hpp   bit_operations.hpp    compiler_detection.hpp
    div_kernels.hpp             fallback_portable.hpp mul_kernels.hpp
    representation.hpp          (y el propio fixed_width_int_t.hpp)

Y **ninguna de `int128_param_*`**: el tipo nuevo no depende del viejo. Por eso
la retirada de la 1.90 es un borrado y no una cirugía
([ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md)).

> **Cómo se comprobó, y cómo NO.** Este recuento sale de recorrer los
> `#include` a mano. Se intentó antes con `g++ -E`, y **la orden no producía
> nada**: el «cero cabeceras» que devolvía era un resultado vacío, no una
> medida. Un mensaje de commit y una enmienda de ADR-006 llegaron a citar esa
> medición inexistente. Si alguien vuelve a comprobarlo, que mire primero si el
> preprocesador ha escrito algo.

---

## Capa 1 — `intrinsics/`: hablar con el hardware, o fingir que se puede

Cinco cabeceras. Su trabajo es dar una cara única a operaciones que cada
compilador expone de forma distinta, y **funcionar igual donde no existen**.

### `compiler_detection.hpp` — quién compila, dónde, y con qué

No tiene código: son **29 macros** que responden tres preguntas —qué compilador,
qué sistema, qué arquitectura— y dos más: qué builtins hay y qué cabecera de
intrínsecos incluir.

El convenio, que no estaba escrito en ninguna parte hasta P3.7: **los macros de
cada grupo existen siempre**, el detectado vale `1` y los demás `0`, para poder
escribir `#if INTRINSICS_COMPILER_GCC` sin `defined()`.

Dos detalles que no son obvios y que se pagaron descubriendo:

- El orden de la cadena importa: **Intel primero**, porque también define
  `__clang__` o `__GNUC__`; y Clang antes que GCC por lo mismo.
- `INTRINSICS_HAS_BUILTIN_ADDC` exige además **x86-64 y no-ABI-de-MSVC**: Intel
  en Windows anuncia el builtin y no lo tiene.

### `fallback_portable.hpp` — el mismo resultado sin intrínsecos

`addc64_portable`, `clz64_portable`, `ctz64_portable`, `popcount64_portable`,
`popcount64_table`, `bswap64_portable`, `bswap64_shifts`, `rotl64_portable`.

Es la red: donde el compilador no da el intrínseco, esto da **el mismo valor**
más despacio. También es lo que permite que todo sea `constexpr`, porque los
intrínsecos no lo son y hay que tener un camino que sí.

### `arithmetic_operations.hpp` — acarreos y productos de 64 bits

`addcarry_u64`, `add128`, `div128_64`, `div128_64_composed`, y las variantes con
`_addcarry_u64` / `_subborrow_u64` / `_udiv128` de MSVC y con `__asm__` en GCC.

Es la pieza de la que cuelga todo lo demás: la suma con acarreo y el producto
`64x64 -> 128` son las dos operaciones que un procesador tiene y C++ no expone.

### `bit_operations.hpp` — contar bits

`clz64`, `ctz64`, `ffs64`, `parity64`, envolviendo `__builtin_clzll` y compañía,
con el camino portable detrás.

### `byte_operations.hpp` — orden de bytes

`bswap`, `rotl`, `rotr` sobre 16/32/64 bits.

> **Esta cabecera queda huérfana en la 1.90.** Hoy la incluye **solo**
> `int128_parameterized.hpp`, que se borra. El tipo nuevo no la usa: hace su
> `byteswap` sobre el valor en complemento a dos
> ([ADR-018](decisions/ADR-018-la-representacion-no-es-observable.md)). Al
> retirar la familia vieja hay que decidir si se borra también o si se conserva
> por si vuelve a hacer falta.

---

## Capa 2 — `algorithms/`: los núcleos medibles

Dos cabeceras, y las dos existen por la misma razón: **separar el algoritmo del
tipo**, para poder medirlo y cambiarlo sin tocar la API.

### `mul_kernels.hpp` — cuatro algoritmos y sus umbrales

`kmul_full`, `kmul_full_gen`, `mul_escolar_desenrollado`, `mul_escolar_bucle`,
`mul_karatsuba_equilibrado`, `toom3_full`, y los tres functores del término del
medio: `medio_escolar`, `medio_karatsuba`, `medio_reparto`.

**Cada umbral está medido, no supuesto.** Dónde está la medida:

| | Dónde |
|---|---|
| Escolar desenrollado ≤ 21, Karatsuba ≥ 22 | [PERFORMANCE.md § El umbral de Karatsuba](PERFORMANCE.md) |
| El acantilado y el reparto equilibrado | [§ El acantilado, resuelto](PERFORMANCE.md) |
| `x*x` no es `a*b` | [§ El cuadrado](PERFORMANCE.md) |
| Toom-3 ≥ 1024 | [§ Toom-3: dónde cruza de verdad](PERFORMANCE.md) |
| El rango 64..4096 | [§ El rango 64..4096](PERFORMANCE.md) |

Y la teoría de por qué la escalera es esa, en
[ESTUDIO_ALGORITMOS_RAPIDOS.md § 1](ESTUDIO_ALGORITMOS_RAPIDOS.md).

> **`medio_reparto` y la perilla escondida.** El término del medio de Karatsuba
> vuelve al reparto completo, no al escolar, y el parámetro `Base` se
> **arrastra** hacia abajo. Antes no lo hacía: el `8` iba escrito a mano en la
> llamada recursiva, así que cualquier barrido de `Base` medía una mezcla y no
> un corte. Está en el comentario de la clase.

### `div_kernels.hpp` — Knuth D y los estimadores

`div_knuth_d`, `div_2por1_preinv`, `div_3por2_preinv`,
`div_128_64_hi_menor_que_d`, `div_por_constante`, y los dos estimadores del
dígito de cociente: `estimador_knuth`, `estimador_moller_granlund`,
`estimador_auto`.

| | Dónde |
|---|---|
| Knuth D frente a división binaria | [PERFORMANCE.md § División — Knuth D](PERFORMANCE.md) |
| Möller–Granlund 2/1 y 3/2, y sus umbrales | [§ La división: la línea base](PERFORMANCE.md) |
| División por constante (Granlund–Montgomery) | [§ División por constante](PERFORMANCE.md) |
| Por qué **no** hay Burnikel–Ziegler | [ADR-016](decisions/ADR-016-burnikel-ziegler-aparcado-por-medida.md) |

> **`estimador_auto` decide en ejecución, y no puede ser de otra forma.** La
> elección depende de `n` —los dígitos de cociente—, que no se conoce hasta que
> se llama. Se midió la alternativa sin rama —sacar la decisión fuera y tener
> dos bucles— y **empata en velocidad dentro del ruido** pero cuesta **1,69× de
> código objeto**. Por eso el núcleo se instancia una sola vez.
>
> Y hay una rama que **ningún operando aleatorio encontró**: 9300 casos pasaron
> limpios, y la sacó el primer barrido de esquinas. Está comentada en el propio
> `operator()`.

---

## Capa 3 — `representation.hpp`: el puente entre codificaciones

No es `intrinsics/` ni `algorithms/`, pero sí es interno en el mismo sentido.
Contiene `a_c2` y `desde_c2`, las **dos funciones** en las que cabe todo
Magnitud-Signo y Exceso-K.

Las decisiones están en
[ADR-005](decisions/ADR-005-representacion-como-parametro-de-plantilla.md),
[ADR-011](decisions/ADR-011-sin-signo-equivale-a-binnat.md),
[ADR-017](decisions/ADR-017-magnitud-signo-y-exceso-k-como-codificaciones.md) y
[ADR-018](decisions/ADR-018-la-representacion-no-es-observable.md).

> **La regla que más veces se ha roto en este proyecto**: en Magnitud-Signo y
> Exceso-K **los limbos no son el valor**. Toda operación que mire bits tiene
> que pasar por `a_c2()`. Seis fallos reales salieron de olvidarlo, y un séptimo
> --`from_string` perdiendo el signo al saturar-- el 23 sep.

---

## Por qué no hay un `API_*.md` por cada una

El armonizador (`scripts/check_docs_consistency.py`) lista estas cabeceras como
«sin `API_*.md` propio». **Es información, no deuda.**

Un `docs/API_div_kernels.md` diría que `div_kernels.hpp` es API, y no lo es: si
alguien lo incluye y luego cambiamos un umbral, se le rompe el código sin que
hayamos roto ninguna promesa. ADR-014 fijó el criterio: **entra lo que un
usuario puede incluir, o sea la raíz de `include/`**.

Lo que sí hacía falta es este mapa, y es lo que estás leyendo.

### `int128_param_divmod.hpp` tampoco lo tendrá

Es de la familia deprecada, **se borra en la 1.90**
([ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md), tramo 3).
Documentar lo que se retira en la siguiente versión es trabajo tirado.

---

## Qué hacer al tocar una de estas capas

1. **Los umbrales se miden, no se copian.** Un número publicado por GMP o por
   otra biblioteca es relativo a **su** denominador; aquí ya falló dos veces
   (Toom-3 y Burnikel–Ziegler, este último por 20×).
2. **Los aleatorios no bastan.** Un valor de 128 bits al azar no es potencia de
   dos nunca, y `is_power_of_2` pasó 400 casos estando rota para todas. Las
   esquinas se construyen.
3. **Romper a propósito.** Un test en verde no dice nada hasta que se sabe que
   puede ponerse rojo. En este proyecto la falsificación ha pagado seis veces.
4. **Cruzar las cuatro representaciones**, y recordar que el cruce vale **para
   todo valor que las cuatro puedan representar**: hay exactamente uno que no
   (ADR-018).
