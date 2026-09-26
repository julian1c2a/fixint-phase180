# Nomenclatura de punto fijo: qué han hecho los demás

**Escrito el 26 September 2026.** Material previo al ADR del punto fijo; **no es
una decisión todavía**.

Se han mirado seis fuentes, y lo que se busca no es «qué nombre suena mejor»
sino **cuál es canónico**: el que alguien que llega de otra biblioteca o de otro
lenguaje ya conoce.

> **Sobre la fiabilidad de este documento.** Los nombres marcados ✔ se han leído
> de la fuente primaria en esta sesión. Los marcados ~ se citan de segunda mano y
> **hay que confirmarlos antes de usarlos**. No se ha dado ninguno por bueno de
> memoria.

---

## 1. ISO/IEC TR 18037 — `<stdfix.h>` ✔

El único **estándar** de los seis: WG14, viene de N1169, publicado como TR en
2004 y revisado en 2008. Define el punto fijo como **tipos del lenguaje**, no
como biblioteca, y de ahí sale casi toda la nomenclatura del mundo embebido.

### Los tipos

Dos familias, y la distinción es la idea más valiosa de todo el documento:

| | Qué es | Rango |
|---|---|---|
| **`_Fract`** | **sólo fracción**, sin parte entera | `[-1, 1)` |
| **`_Accum`** | fracción **más** parte entera («acumulador») | según los bits enteros |

Cruzadas con `short` / (nada) / `long` / `long long`, con `signed`/`unsigned`, y
con el calificador **`_Sat`** para la variante saturante:

```c
short _Fract   _Fract   long _Fract   long long _Fract
short _Accum   _Accum   long _Accum   long long _Accum
unsigned _Fract        _Sat _Fract        _Sat unsigned _Accum   ...
```

### Los sufijos de literal

Sistemáticos, y valen como clave de lectura de todos los nombres de función:

| tipo | sufijo |
|---|---|
| `short _Fract` | `hr` / `HR` |
| `_Fract` | `r` / `R` |
| `long _Fract` | `lr` / `LR` |
| `long long _Fract` | `llr` / `LLR` |
| `short _Accum` | `hk` / `HK` |
| `_Accum` | `k` / `K` |
| `long _Accum` | `lk` / `LK` |
| `long long _Accum` | `llk` / `LLK` |

Sin signo, se prefija `u`: `uhr`, `ur`, `ulr`, `uhk`, `uk`, `ulk`…
(`0.5r`, `1.5k`, `0.25ur`.)

### Las funciones

Cada una viene en **dos formas**: la genérica por tipo, sufijo `fx`, y una por
cada tipo concreto. Es un patrón que merece la pena copiar.

| Genérica | Por tipo | Qué hace |
|---|---|---|
| `absfx` | `abshr`, `absr`, `abslr`, `absllr`, `abshk`, `absk`, `abslk`, `absllk` | valor absoluto |
| `roundfx` | `roundhr`, `roundr`, `roundlr`, … | redondea a `n` bits fraccionarios |
| `countlsfx` | `countlshr`, `countlsr`, … | **cuenta bits de signo por delante** |
| — | `bitshr`, `bitsr`, `bitslr`, … | el tipo → su patrón de bits como entero |
| — | `hrbits`, `rbits`, `lrbits`, `kbits`, … | el entero → el tipo, **reinterpretando** |
| — | `muli`, `divi`, `idiv` | mezcla con entero (`rdivi`, `lrdivi`, `urdivi`…) |
| — | `strtofx` (`strtofxhr`…) | desde cadena |
| — | `hrtoa`, `rtoa`, `hktoa`, `ktoa` | a cadena |

Y macros de propiedades: **`FBIT`** (bits fraccionarios) e **`IBIT`** (bits
enteros), con prefijo por tipo: `SFRACT_FBIT`, `ACCUM_IBIT`, `LACCUM_FBIT`…
Más `*_MIN`, `*_MAX`, `*_EPSILON`, y `FX_FULL_PRECISION`.

### Lo que hay que traerse de aquí

1. **`countls`** — contar bits de signo por delante. La biblioteca **no lo
   tiene**, y es la operación que hace falta para normalizar antes de
   multiplicar sin perder bits. Es útil de verdad, no folclore.
2. **`FBIT` / `IBIT`** como nombres de las dos cuentas de bits. Aquí son
   `escala_bits` y `limbos_enteros`; el concepto coincide.
3. **`bits*` / `*bits`** — el par ida/vuelta al patrón crudo. Aquí es
   `desde_crudo` y su inverso.
4. La distinción **`_Fract` vs `_Accum`**: aquí es `F == N` frente a `F < N`, y
   resulta que **el caso puramente fraccionario es el que tuvo el fallo de
   `to_string`**. TR 18037 le da tipo propio por algo.
5. **El patrón «genérica + por tipo»**: en C++ la genérica es gratis con
   plantillas, pero el nombre `absfx`/`roundfx`/`countlsfx` es el canónico.

---

## 2. P0105 «Rounding and Overflow in C++» ✔

De Lawrence Crowl. **La taxonomía más completa que existe** de modos de
redondeo y desbordamiento, y es la fuente de la que beben P0106 y
Boost.Fixed_point.

### `enum class rounding`

Dieciocho, en dos grupos de nueve. El prefijo dice **a qué se aplica**:

| `all_*` — a todo valor no representable | `tie_*` — sólo al empate |
|---|---|
| `all_to_neg_inf` | `tie_to_neg_inf` |
| `all_to_pos_inf` | `tie_to_pos_inf` |
| `all_to_zero` | `tie_to_zero` |
| `all_away_zero` | `tie_away_zero` |
| `all_to_even` | `tie_to_even` |
| `all_to_odd` | `tie_to_odd` |
| `all_fastest` | `tie_fastest` |
| `all_smallest` | `tie_smallest` |
| `all_unspecified` | `tie_unspecified` |

**Esa separación `all_` / `tie_` es la idea que hay que robar.** Los cinco modos
de esta biblioteca son todos `all_*` salvo el de «al más cercano», que es un
`tie_*`. Hoy están en el mismo plano y **no se distingue en el nombre** a qué se
aplica cada uno.

### `enum class overflow`

`impossible`, `undefined`, `abort`, `quick_exit`, `exception`, `special`,
`saturate`, `modulo_shifted`.

Aquí hay `wrap`, `checked` y `saturate`. El mapeo:

| aquí | P0105 | nota |
|---|---|---|
| `wrap` | `modulo_shifted` | el nombre canónico dice **por qué** envuelve |
| `saturate` | `saturate` | coincide |
| `checked` | `special` | «un valor especial que se propaga» — es exactamente lo que hace |

`checked` → `special` es la que más gano tendría: `checked` no dice qué pasa al
desbordar, `special` sí.

### Funciones

Redondeo: `convert`, `divide`, `scale_down`.
Desbordamiento: `convert`, `limit`, `limit_nonnegative`, `limit_signed`,
`limit_twoscomp`, y las `*_bits`; `scale_up`.
Las dos cosas: `convert`, `scale`.

**`scale_up` / `scale_down`** son los nombres canónicos de lo que aquí hacen
`<<` y `>>` sobre el punto fijo (ADR-020: `<<` exacto, `>>` redondea). Y
**`scale_down` es el que lleva el modo de redondeo**, que es justo la asimetría
que ADR-020 decidió.

---

## 3. P0106 «C++ Binary Fixed-Point Arithmetic» ✔

También de Crowl (antes N3352). **Ocho** plantillas, en un cuadrante:

| | valor (sin política) | con política |
|---|---|---|
| entero sin signo | `card_val<Crng>` | `cardinal<Crng, Covf>` |
| entero con signo | `intg_val<Crng>` | `integral<Crng, Covf>` |
| fraccionario sin signo | `nonn_val<Crng, Crsl>` | `nonnegative<Crng, Crsl, Crnd, Covf>` |
| fraccionario con signo | `negt_val<Crng, Crsl>` | `negatable<Crng, Crsl, Crnd, Covf>` |

Parámetros: **`Crng`** (rango, en bits), **`Crsl`** (resolución, en bits),
**`Crnd`** (redondeo), **`Covf`** (desbordamiento). Por omisión,
`rounding::tie_to_odd` y `overflow::exception`.

Funciones: `increment<overflow>()`, `scale_up<n>()`, `scale<n, r>()`,
`divide<round>(u)`.

### Lo interesante

1. **Rango y resolución en BITS, y por separado.** Aquí son `N` (total) y `F`
   (fraccionarios), con `E = N - F` deducido. P0106 nombra las dos mitades de
   forma **independiente**, que es exactamente la decisión de promoción que se
   tomó el 26 sep: `max(E)` y `max(F)` por separado.
2. **`divide<round>(u)`**: el redondeo va en la **llamada**, no sólo en el tipo.
   Aquí está en el 6.º parámetro de plantilla. Las dos formas pueden convivir.
3. Los pares `*_val` sin política / con política: separa el **valor** de la
   **política**. Aquí la política es un parámetro del tipo y no hay versión sin
   ella; con `wrap` no cuesta nada (ADR-009), así que no hace falta.

---

## 4. P0037 «Fixed-Point Real Numbers» ✔

De John McFarlane, seis revisiones. Es el que llegó más lejos en el comité.

```cpp
fixed_point<Rep, Exponent, Radix>
```

**`Exponent`, no «bits fraccionarios».** El valor es `rep · Radix^Exponent`, con
`Exponent` normalmente negativo. Eso permite escalas **mayores que uno**
(`Exponent > 0`) y radix distinto de 2 — decimal incluido.

Miembros: `rep`, `radix`, `exponent`.
Libres: **`to_rep`**, **`from_rep`**, **`from_value`**.
Deducción: `fixed_point(Integer) -> fixed_point<Integer, 0>`.

### Lo interesante

1. **`to_rep` / `from_rep` es el par canónico de C++** para el crudo. Aquí es
   `desde_crudo` y el acceso al crudo. **Es el nombre que hay que adoptar**: lo
   usan P0037 y CNL, y quien venga de ahí lo buscará así.
2. **`from_value`**: construye deduciendo la escala del valor dado. No existe
   aquí y es cómodo.
3. **`Radix` como parámetro** abre el decimal sin tocar nada. Aquí la escala es
   `2^(64F)` fija. No es urgente, pero **un `Radix` que hoy siempre vale 2 cuesta
   un parámetro y deja la puerta abierta**; añadirlo después es romper la API.

---

## 5. CNL — Compositional Numeric Library ✔

La evolución de P0037, del mismo autor. El nombre del tipo cambió a
**`cnl::scaled_integer`**, y eso es un dato: *McFarlane dejó de llamarlo
«fixed_point»*.

Plantillas: `cnl::scaled_integer`, `cnl::elastic_integer`,
`cnl::elastic_scaled_integer`, `cnl::overflow_integer`,
`cnl::rounding_integer`, `cnl::wide_integer`, `cnl::static_integer`,
`cnl::static_number`, `cnl::fraction`.

Libres: `cnl::to_rep()`, `cnl::quotient()`, `cnl::make_elastic_scaled_integer()`.
~ `from_rep`, `from_value`, `sqrt`, `digits`, `set_digits`, `scale`, `power`,
`saturated_overflow_tag`, `nearest_rounding_tag` — citados pero **no leídos de la
fuente primaria**.

### Lo interesante, y es mucho

1. **La composición.** En vez de un tipo con seis parámetros, CNL tiene tipos
   **pequeños que se apilan**: `overflow_integer<rounding_integer<wide_integer<…>>>`.
   Esta biblioteca tomó el camino contrario —seis parámetros— y **es
   defendible**, pero la alternativa existe y merece constar en el ADR.
2. **`cnl::quotient`** para la división que no pierde: devuelve un tipo lo bastante
   ancho. Aquí `operator/` redondea según la perilla. Un `quotient` exacto que
   ensanche es una función que falta.
3. **`cnl::fraction`** — par dividendo/divisor sin evaluar. Nada que ver con el
   punto fijo, pero es la pieza que hace exacta la división.
4. **`digits` / `set_digits`** como traits de anchura: `digits<T>` y
   `set_digits<T, N>::type`. Aquí hay `numeric_limits::digits` (ADR-022) pero
   **no** el `set_digits`, que es lo que permite escribir código genérico que
   ensancha.
5. **El nombre `scaled_integer`** describe la implementación con exactitud —y es
   literalmente lo que dice ADR-019: «un punto fijo es un entero con una
   escala».

---

## 6. libfixmath ~

C, `Q16.16`, clásico en embebido. **Nombres citados de memoria, sin verificar en
esta sesión**: `fix16_t`; `fix16_add`, `fix16_sub`, `fix16_mul`, `fix16_div`;
las saturantes con `s` — `fix16_sadd`, `fix16_smul`, `fix16_sdiv`;
`fix16_sqrt`, `fix16_sin`, `fix16_exp`, `fix16_log`;
`fix16_from_int` / `fix16_to_int`, `fix16_from_dbl` / `fix16_to_dbl`,
`fix16_from_str` / `fix16_to_str`; `fix16_one`, `fix16_pi`, `fix16_maximum`,
`fix16_overflow`.

Lo único que aporta al debate: **la convención `Qm.n`**, que es como el mundo
embebido *nombra* los formatos. Aquí `fixed_point_t<2,1>` es `Q64.64`. Poner la
notación `Q` en la documentación cuesta nada y **orienta de inmediato** a quien
venga de ahí.

---

## 7. Ada ✔

El planteamiento más distinto de los seis, y el que más tiene que enseñar.

### Dos clases de tipo

```ada
type Volt  is delta 0.125 range 0.0 .. 255.0;   -- fijo ORDINARIO
type Money is delta 0.01 digits 15;             -- fijo DECIMAL
```

En el **ordinario** el `delta` es una petición: el compilador elige el
**`'Small`** real, normalmente una potencia de dos. En el **decimal** el `delta`
ha de ser potencia de diez y **`'Small` es igual al `'Delta`**.

### Los atributos

| | Qué |
|---|---|
| **`'Delta`** | el paso **pedido** |
| **`'Small`** | el paso **real** con el que se implementa |
| **`'Fore`** | caracteres necesarios **antes** del punto, incluido el signo |
| **`'Aft`** | dígitos decimales necesarios **después** del punto: el menor `N` con `10^N · 'Delta ≥ 1` |
| **`'Scale`** | el `N` tal que `'Delta = 10^-N` (decimal). **No siempre igual a `'Aft`** |
| **`'Digits`** | dígitos significativos (decimal) |
| `'Round`, `'First`, `'Last`, `'Machine_Radix` | |

### Lo interesante

1. **`'Delta` frente a `'Small`** — pedido frente a real. Aquí la escala es
   exactamente `2^-(64F)`: **coinciden siempre**, y eso es una propiedad fuerte
   que merece decirse. Ada la tiene que separar porque no puede garantizarlo.
2. **`'Fore` y `'Aft`** — los dos números que hacen falta para formatear, y
   están **en el tipo**, no en la llamada. Aquí `to_string` los calcula cada vez.
   Exponerlos como constantes es gratis y hace el formateo predecible.
3. **`'Aft` está definido por una desigualdad**, no por `F`: «el menor `N` con
   `10^N·delta ≥ 1`». Es la cuenta correcta y la biblioteca la necesita.
4. **Multiplicar dos fijos exige conversión explícita.** Ada devuelve un
   `universal_fixed` anónimo que **hay que convertir**, precisamente porque el
   tipo del resultado es ambiguo.

   **Esto va en contra de lo que se decidió el 26 sep** —promocionar a
   `max(E)+max(F)`— y el ADR debe decir por qué se aparta: aquí la anchura es un
   parámetro y el resultado **sí** se puede nombrar sin ambigüedad, mientras que
   en Ada el `'Small` lo elige el compilador y no hay tipo que nombrar. La
   decisión sigue siendo buena; lo que no vale es tomarla sin saber que Ada
   decidió lo contrario.

---

## Resumen: qué adoptar

**Nombres, por orden de cuánto ganan:**

| Cambio | De dónde | Por qué |
|---|---|---|
| `to_rep` / `from_rep` | P0037, CNL | **El par canónico de C++.** Hoy es `desde_crudo` |
| `scale_up` / `scale_down` | P0105 | Lo que hacen `<<` y `>>`, y `scale_down` es el que redondea — la asimetría de ADR-020 |
| `all_*` / `tie_*` en los modos | P0105 | Distingue en el **nombre** si el modo se aplica a todo o sólo al empate |
| `special` en vez de `checked` | P0105 | `checked` no dice qué pasa; `special` sí |
| `countls` | TR 18037 | **Operación que falta**, y hace falta para normalizar |
| `fore` / `aft` como constantes | Ada | Formateo predecible, cuesta cero |
| `set_digits` | CNL | Permite genérico que ensancha |
| `quotient` exacto | CNL | División que no pierde |
| notación `Qm.n` en los docs | libfixmath | Orienta al instante a quien viene de embebido |

**Y el nombre del tipo.** Sobre la mesa: `fixed_point` (coherente con
`fixed_width_int`), o **`scaled_integer`**, que es a donde McFarlane llegó
después de seis revisiones y es **literalmente la frase de ADR-019**. El segundo
describe la implementación; el primero, el concepto. No lo decide este documento.

---

## Fuentes

- [stdfix.h — The LLVM C Library](https://libc.llvm.org/headers/stdfix.html)
- [AVR-LibC `<stdfix.h>`: ISO/IEC TR 18037 Fixed-Point Arithmetic](https://avrdudes.github.io/avr-libc/avr-libc-user-manual-2.3.0/group__avr__stdfix.html)
- [ISO/IEC TR 18037:2008 (muestra)](https://cdn.standards.iteh.ai/samples/51126/4257ea2d26ae480ab4e8ba54ebb77a84/ISO-IEC-TR-18037-2008.pdf)
- [WG14 N968 — borrador de TR 18037](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n968.pdf)
- [P0105R1 — Rounding and Overflow in C++](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0105r1.html)
- [P0106R0 — C++ Binary Fixed-Point Arithmetic](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0106r0.html)
- [P0037R6 — Fixed-Point Real Numbers](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0037r6.html)
- [CNL — Compositional Numeric Library](https://johnmcfarlane.github.io/cnl/)
- [Ada RM 3.5.10 — Operations of Fixed Point Types](https://www.adaic.org/resources/add_content/standards/05aarm/html/AA-3-5-10.html)
- [Ada RM 3.5.9 — Fixed Point Types](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5-9.html)
- [Introduction to Ada — Fixed-point types](https://learn.adacore.com/courses/intro-to-ada/chapters/fixed_point_types.html)
- [Advanced Ada — Numeric Attributes](https://learn.adacore.com/courses/advanced-ada/parts/data_types/numeric_attributes.html)
