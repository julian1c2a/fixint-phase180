# Matriz de paridad — qué tiene que funcionar, y dónde

**Última actualización:** 10 September 2026

> Documento maestro de cobertura. Dice **qué capacidad pública tiene que
> funcionar en qué combinación de parámetros de plantilla**, y la tabla del
> final **no está escrita a mano**: la genera
> [`scripts/check_matriz_paridad.py`](../scripts/check_matriz_paridad.py)
> compilando una sonda por celda.

---

## Por qué existe

`fixed_int_t<N, Sign, Form, Policy>` tiene cuatro parámetros. Cada capacidad
—construir, imprimir, formatear, dividir, `gcd`— tiene que funcionar en cada
combinación de esos parámetros, y hasta el 9 sep 2026 **nada comprobaba eso**.

El resultado, medido: P1.1 añadió el cuarto parámetro y **siete sitios públicos
se quedaron con tres**.

| Sitio | Qué se rompía |
|---|---|
| `is_unsigned_fixed_int` | `nstd::unsigned_integral<T>` falso — y `is_signed_fixed_int` **sí** se había generalizado |
| un temporal en `to_string(base)` | `to_string(16)` no compilaba; `to_string()` sí |
| `operator<<` y `operator>>` | no compilaban |
| `std::formatter` | no compilaba |
| dos alias `using U = uint_fixed_t<N>` | rompían la división con signo |
| el constructor de conversión | sólo aceptaba orígenes `wrap`; era la causa de fondo |

Con un tipo `checked` no compilaba ni `std::cout << x`. **La suite entera estaba
en verde**, con 60 ficheros y 46.800 comprobaciones diferenciales: probaba sólo
la política por defecto.

Ese mismo día se encontraron otros dos casos del mismo patrón —una lista escrita
de memoria que se separa del código sin que nadie lo note—:

- El inventario de [ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md)
  no listaba cuatro funciones públicas que sí existían en los headers viejos.
- La lista de «funciones que fijan la política» tenía tres entradas; eran nueve.

**Ninguna tabla escrita a mano habría cazado nada de esto.** Por eso la de abajo
se genera compilando.

---

## Los ejes

### 1. Capacidades (las filas)

Una fila por capacidad **pública**, agrupadas por familia: núcleo del tipo,
operadores, cadenas, integración con la STL, traits y conceptos, aritmética de
orden superior, bits, numéricas y política.

Viven en `CAPACIDADES`, dentro del guion. Añadir una es añadir una entrada con
su cuerpo de prueba; no hay nada más que tocar.

### 2. Dimensiones (las columnas)

**Hoy son cuatro celdas: signo × política.**

| | `wrap` | `checked` |
|---|---|---|
| **sin signo** | `uint_fixed_t<2>` | `uint_fixed_t<2, checked>` |
| **con signo** | `int_fixed_t<2>` | `int_fixed_t<2, checked>` |

Dos parámetros **no** son ejes independientes hoy, y conviene saber por qué:

- **`Form` está atado al signo** por [ADR-011](decisions/ADR-011-sin-signo-equivale-a-binnat.md):
  sin signo ⟺ `binnat`, con signo ⟺ complemento a dos, y el `static_assert` de
  la clase lo impone. Cuando se porten Magnitud-Signo y Exceso-K (P1.5 tramo 3)
  **pasará a serlo**, y esta matriz duplicará sus columnas.
- **`N` no es un eje de paridad.** Los caminos que dependen de N —N=2
  especializado, escolar desenrollado hasta 20, Karatsuba en potencias de dos
  desde 32— son de `operator*`, y los cubren `benchmark_curva_n` y
  `test_fixed_karatsuba`. Aquí se fija N=2 a propósito: lo que se busca son
  huecos **por parámetro**, no por anchura.

### 3. Estado (las celdas)

Cada capacidad **declara** en qué celdas debe funcionar: `TODAS`, `SIN_SIGNO`,
`CON_SIGNO` o `SOLO_WRAP`. El guion compila y compara.

- **`sí`** — compila, y debía.
- **`n/a`** — no compila, y no debía. `midpoint` sobre un tipo con signo, por
  ejemplo.
- **`ROTO`** — debía compilar y no compila.
- **`de más`** — compila y **no debía**. Tan importante como lo anterior: si
  `midpoint` empezara a aceptar tipos con signo sin que nadie lo decidiera, eso
  es un cambio de contrato silencioso.

Declarar es una afirmación, y puede estar equivocada. La primera pasada dio
`de más` en «construir desde `int`» para los tipos sin signo, y **la equivocada
era la declaración**: `uint_fixed_t<2>{-42}` compila igual que
`unsigned x = -42;` en C++, que está definido y da el valor módulo 2ⁿ. Queda
anotado en el guion para que nadie lo «arregle».

---

## Las dos reglas que lo mantienen vivo

**1. Añadir un valor a un `enum` de plantilla abre una columna entera, y nace en
rojo.**

`overflow_policy` declara cuatro valores; sólo `wrap` y `checked` están
escritos. `saturate` y `trap` existen desde P1.1 **para no cambiar la ABI de la
plantilla al añadirlos** ([ADR-009](decisions/ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md)),
y el `static_assert` de la clase los rechaza.

El guion comprueba que **siguen sin compilar**. El día que se escriba
`saturate`, esa comprobación falla y obliga a abrir dos columnas nuevas — que es
exactamente lo que se quiere que pase.

Lo mismo valdrá para `representation_form` cuando entren MS y EK.

**2. Ninguna fila se marca hecha sin un test que la cubra en esa celda.**

La matriz dice que *compila*, que es la mitad. La otra mitad es que *hace lo
correcto*, y eso son tests. La sección 6 de
[`test_fixed_stl_integration.cpp`](../tests/test_fixed_stl_integration.cpp) —115
comprobaciones repitiendo toda la superficie de la STL con `checked`— es el
patrón: cuando una capacidad entra en la matriz, entra también en un test que la
ejercite en las celdas que declara.

---

## Cómo se usa

```bash
python scripts/check_matriz_paridad.py                      # gcc, por defecto
python scripts/check_matriz_paridad.py --compiler clang     # y las otras dos
python scripts/check_matriz_paridad.py --escribe-doc        # regenera la tabla
```

Devuelve 0 si **toda** celda se comporta como declara, y 1 si alguna no. Antes
de mirar ninguna celda compila un `int main(){}`: si eso falla, para en seco y lo
dice, en vez de acusar a las capacidades — la lección que dejó
`check_headers_selfcontained.py` cuando informaba 0/31 falsos.

**Dónde encaja en el flujo:** es una comprobación de `ACTUALIZA_DOC`, no de cada
compilación. Tarda lo que tarden unas 160 compilaciones de sintaxis, en
paralelo. Correrla al cerrar una tanda de trabajo que haya tocado la clase, y
siempre antes de dar por cerrado un tramo de P1.5.

---

## Lo que encontró en su primera pasada

Además del error de declaración de arriba, una cosa del código:

**Las siete `checked_*` y `saturating_*` sólo aceptan operandos `wrap`.**
`checked_add(a, b)` sobre un `uint_fixed_t<2, checked>` no compila.

No está claro si es un hueco o el diseño, así que la matriz lo declara
`SOLO_WRAP` —lo que hay— en vez de marcarlo `ROTO`. **La matriz dice lo que hay,
no lo que a uno le gustaría.**

- **A favor de dejarlo**: sobre un tipo `checked`, `a + b` **ya** comprueba, así
  que `checked_add` no añade nada.
- **En contra**: código genérico que llame a `saturating_add` deja de compilar
  en cuanto alguien cambia la política del tipo — que es justo lo que un
  parámetro de plantilla no debería provocar.

Y hay una pregunta de semántica sin responder que bloquea generalizarlo:
**`saturating_add` sobre un valor ya marcado, ¿satura y limpia la marca, satura
y la conserva, o no tiene sentido?** Sin esa respuesta no se puede generalizar
sin inventarse el contrato. Anotado como **P1.5 tramo 2f**.

---

## Estado actual

<!-- MATRIZ:INICIO -- generado por scripts/check_matriz_paridad.py -->

Generado con `gcc` (g++.exe (Rev3, Built by MSYS2 project) 16.2.0).

| | Capacidad | uint/wrap | uint/checked | int/wrap | int/checked |
|---|---|---|---|---|---|
| **Nucleo** | `construir desde uint64` | sí | sí | sí | sí |
|  | `construir desde int` | sí | sí | sí | sí |
|  | `conversion cross-N` | sí | sí | sí | sí |
|  | `max() / min() / one()` | sí | sí | sí | sí |
|  | `limb() / set_limb()` | sí | sí | sí | sí |
| **Operadores** | `aritmetica + - *` | sí | sí | sí | sí |
|  | `division / y %` | sí | sí | sí | sí |
|  | `divmod estatico` | sí | sí | sí | sí |
|  | `bitwise y desplazamientos` | sí | sí | sí | sí |
|  | `comparacion y <=>` | sí | sí | sí | sí |
|  | `incremento y compuestos` | sí | sí | sí | sí |
| **Cadenas** | `to_string()` | sí | sí | sí | sí |
|  | `to_string(base)` | sí | sí | sí | sí |
|  | `try_from_string(base)` | sí | sí | sí | sí |
|  | `from_string(base)` | sí | sí | sí | sí |
| **STL** | `operator<< / >>` | sí | sí | sí | sí |
|  | `std::format` | sí | sí | sí | sí |
|  | `std::hash` | sí | sí | sí | sí |
|  | `std::numeric_limits` | sí | sí | sí | sí |
|  | `std::common_type` | sí | sí | sí | sí |
|  | `algoritmos de iterador de std::` | sí | sí | sí | sí |
| **Traits** | `nstd::is_integral y familia` | sí | sí | sí | sí |
|  | `is_fixed_int_v / signed / unsigned` | sí | sí | sí | sí |
|  | `nstd::integral (concepto)` | sí | sí | sí | sí |
|  | `make_signed / make_unsigned` | sí | sí | sí | sí |
| **Aritmetica** | `mul_wide` | sí | sí | sí | sí |
|  | `mulhi / mullo` | sí | sí | sí | sí |
|  | `pow` | sí | sí | sí | sí |
|  | `sqrt` | sí | sí | n/a | n/a |
|  | `gcd / lcm` | sí | sí | sí | sí |
|  | `checked_add / sub / mul` | sí | sí | sí | sí |
|  | `saturating_add / sub / mul` | sí | sí | sí | sí |
| **Bits** | `rotl / rotr` | sí | sí | sí | sí |
|  | `nombres de <bit>` | sí | sí | sí | sí |
|  | `is_power_of_2` | sí | sí | sí | sí |
| **Numericas** | `min / max / clamp` | sí | sí | sí | sí |
|  | `midpoint / abs_diff` | sí | sí | n/a | n/a |
|  | `abs (libre)` | sí | sí | sí | sí |
|  | `is_even / is_odd / sign` | sí | sí | sí | sí |
|  | `ilog2` | sí | sí | n/a | n/a |
|  | `divmod (libre)` | sí | sí | sí | sí |
| **Politica** | `valid()` | sí | sí | sí | sí |

Políticas declaradas en el enum pero **no escritas** (ADR-009), que
tienen que seguir sin compilar:

| Celda | Estado |
|---|---|
| `uint/saturate` | rechazada, como debe |
| `uint/trap` | rechazada, como debe |

<!-- MATRIZ:FIN -->

---

## Lo que esta matriz **no** cubre

Escrito para que no se confunda su alcance:

- **No prueba comportamiento**, sólo que compila. Ver la regla 2.
- **No cubre N.** Ver el eje 2.
- **No cubre `int128_param_t`**, el tipo que [ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md)
  retira. Sería trabajo sobre código que se va.
- **No cubre los cinco compiladores a la vez.** Se ejecuta con uno; los tres de
  MinGW valen. Un hueco por parámetro es del código, no del compilador, así que
  con uno basta para encontrarlo — pero si aparece una diferencia entre
  compiladores, correrla con los tres es la forma de verla.
