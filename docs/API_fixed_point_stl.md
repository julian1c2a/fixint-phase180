# API Reference — integración de `fixed_point_t` con la biblioteca estándar

> `fixed_point_limits.hpp`, `fixed_point_traits_specializations.hpp`,
> `fixed_point_hash.hpp`, `fixed_point_format.hpp`, `fixed_point_iostreams.hpp`
>
> Los cinco headers son independientes: se incluye sólo el que se necesite.
> `fixed_int_t` ya tenía los equivalentes; éstos son la paridad para el punto
> fijo, y con ellos las dos formas nuevas llegan a **24/24** en el verificador
> `scripts/check_acompanamiento_std.py`.

**Lo que hay que leer antes de usarlos:** el punto fijo **no es ni un entero ni
una coma flotante**, y tres miembros de `numeric_limits` no significan lo que un
lector de cualquiera de las dos familias asumiría. Está en
[ADR-022](decisions/ADR-022-numeric-limits-del-punto-fijo.md) y resumido abajo.

## Synopsis

```cpp
#include "fixed_point_limits.hpp"                   // std::numeric_limits
#include "fixed_point_traits_specializations.hpp"   // nstd::is_*, make_*, common_type
#include "fixed_point_hash.hpp"                     // std::hash
#include "fixed_point_format.hpp"                   // std::formatter
#include "fixed_point_iostreams.hpp"                // operator<<, operator>>

namespace nstd {

    // is_arithmetic: sí.  is_integral: NO.
    template <typename T> struct is_fixed_point;
    template <typename T> inline constexpr bool is_fixed_point_v;

    // make_signed / make_unsigned conservan F
    template <...> struct make_signed<fixed_point_t<...>>;
    template <...> struct make_unsigned<fixed_point_t<...>>;

    template <...>
    std::ostream &operator<<(std::ostream &, const fixed_point_t<...> &);
    template <...>
    std::istream &operator>>(std::istream &, fixed_point_t<...> &);
}

// std::numeric_limits<nstd::fixed_point_t<N, F, Sign, Form, Policy, Redondeo>>
// std::hash<nstd::fixed_point_t<...>>
// std::formatter<nstd::fixed_point_t<...>, CharT>
// std::common_type<nstd::fixed_point_t<...>, T>
```

---

## `fixed_point_limits.hpp`

### Los tres miembros que se leen mal

| Miembro | Valor | Por qué sorprende |
|---|---|---|
| `min()` | el **más negativo** | No es el positivo más pequeño, que es lo que `min()` significa en coma flotante. Código genérico que lo use como «el más pequeño» hará cosas distintas según la respuesta **y compilará en los dos casos**. El positivo más pequeño es `epsilon()`. |
| `is_integer` / `is_exact` | `false` / `true` | No son la misma pregunta. `is_exact` habla de la **representación**: cada valor es exactamente `k/2^(64·F)`. Que `*` y `/` redondeen no lo cambia — `operator/` del entero también redondea y allí `is_exact` es cierto. |
| `epsilon()` | el ulp, **absoluto** | En coma flotante `epsilon` es relativo y crece con la magnitud. Aquí el paso es el mismo en todo el rango. |

De ese último sale una triple igualdad que no se da en coma flotante:

```
epsilon()  ==  el paso entre dos valores consecutivos, en cualquier punto
           ==  el valor positivo más pequeño
           ==  denorm_min()
```

Y de ahí que el error absoluto de una suma redondeada esté acotado por
`epsilon/2` **en todo el rango**.

### Tabla completa

| Miembro | Valor | Nota |
|---|---|---|
| `is_specialized` | `true` | |
| `is_signed` | según `Sign` | |
| `is_integer` | **`false`** | lo único que lo separa del entero aquí |
| `is_exact` | **`true`** | la representación es exacta |
| `is_bounded` | `true` | |
| `is_modulo` | `Policy == wrap` | **sale de la política, no del signo**: aquí `wrap` está definido también con signo |
| `is_iec559` | `false` | no lo pretende |
| `has_infinity`, `has_quiet_NaN`, `has_signaling_NaN` | `false` | |
| `has_denorm` | `denorm_absent` | el paso es el mismo en todo el rango |
| `radix` | `2` | |
| `digits` | `64·N − (con signo ? 1 : 0)` | **todos** los bits de valor, no los fraccionarios. Para ésos, `T::escala_bits` |
| `digits10` | `floor(digits · log10 2)` | |
| `max_digits10` | `digits10 + 2` | cifras para **distinguir**; para no perder nada, `to_string(64·F)` |
| `min_exponent`, `max_exponent`, y sus `10` | `0` | no hay exponente: ése es el punto del punto fijo |
| `traps`, `tinyness_before` | `false` | |
| `round_style` | **lee la perilla** | ver abajo |
| `min()` | el más negativo; `0` sin signo | |
| `max()` | el mayor representable | |
| `lowest()` | `min()` | |
| `epsilon()` | el ulp, `2^-(64·F)` | con `F == 0` vale **1**, no 0 como en los enteros |
| `round_error()` | `0.5` / `1` **en ULPs** | ver abajo |
| `infinity()`, `quiet_NaN()`, `signaling_NaN()` | `zero()` | no existen |
| `denorm_min()` | `epsilon()` | no hay subnormales, pero el positivo más pequeño sí existe |

### `round_style` lee la perilla

| `rounding_mode` | `float_round_style` |
|---|---|
| `to_nearest_even` | `round_to_nearest` |
| `to_nearest_away` | `round_to_nearest` |
| `toward_zero` | `round_toward_zero` |
| `toward_neg_inf` | `round_toward_neg_infinity` |
| `toward_pos_inf` | `round_toward_infinity` |

> **El enum del estándar no distingue los dos modos «al más cercano».** Los dos
> caen en `round_to_nearest`, así que **el desempate no se puede deducir de
> `round_style`**. Es una pérdida de información del estándar, no de aquí.

### `round_error()` se mide en ULPs

`0.5` con los modos al más cercano y `1` con los dirigidos, igual que
`numeric_limits<float>::round_error()` vale `0.5` — medio **ULP**, no medio
`float`.

Dos bordes: con `F == 0` el `0.5` no cabe y devuelve cero, como los enteros; con
`F == N` el uno no cabe y los dirigidos devuelven `max()`, que sin signo es
exactamente `1 − epsilon`.

```cpp
#include "fixed_point_limits.hpp"
#include <limits>

using Q = nstd::sfixed_point_t<2, 1>;
using L = std::numeric_limits<Q>;

static_assert(!L::is_integer);          // hay valores entre dos enteros
static_assert(L::is_exact);             // pero cada uno es exacto
static_assert(L::digits == 127);        // 128 bits menos el de signo
static_assert(L::round_style == std::round_to_nearest);

// `epsilon` es el paso, y el mismo en todo el rango:
const Q lejos{1000000};
assert(lejos + L::epsilon() != lejos);
```

---

## `fixed_point_traits_specializations.hpp`

| Trait | Resultado | Por qué |
|---|---|---|
| `nstd::is_arithmetic_v<T>` | `true` | suma, resta, multiplica y divide |
| `nstd::is_integral_v<T>` | **`false`** | hay valores entre dos enteros consecutivos |
| `nstd::is_fixed_point_v<T>` | `true` | la pregunta que el código genérico quiere hacer de verdad: «¿tiene escala?» |
| `nstd::is_signed_v` / `is_unsigned_v<T>` | según `Sign` | |
| `nstd::make_signed_t<T>` | el hermano con signo, **misma `F`** | el hermano de un Q64.64 es otro Q64.64 |
| `nstd::make_unsigned_t<T>` | el hermano sin signo, **misma `F`** | `Form` pasa a `binnat`: ADR-011 ata sin-signo con binnat |
| `std::common_type_t<T, entero>` | **`T`** | gana el punto fijo, como `common_type<double,int>` da `double` |
| `std::common_type_t<T, fixed_int_t<...>>` | **`T`** | igual |

> `is_integral` **no** se especializa a `true_type` a propósito: la primaria ya
> da `false` y dejarlo así es la respuesta. Está documentado en el header para
> que no se lea como un olvido.

```cpp
using Q  = nstd::sfixed_point_t<2, 1>;
using QU = nstd::make_unsigned<Q>::type;      // ufixed_point_t<2, 1>
static_assert(QU::limbos_fraccionarios == Q::limbos_fraccionarios);
static_assert(std::is_same_v<std::common_type_t<Q, std::uint64_t>, Q>);
```

---

## `fixed_point_hash.hpp`

Dispersa el **crudo**, que es lo correcto porque la escala es una constante del
tipo: dos valores del mismo tipo son iguales si y sólo si sus crudos lo son.

| | |
|---|---|
| Complejidad | `O(N)` sobre los limbos |
| Contrato | `a == b` ⟹ `hash(a) == hash(b)`, dentro de un mismo tipo |

> Dos tipos con la misma `F` y distinta `Form` dan hashes distintos para el
> mismo valor. **No es un problema**: son tipos distintos, y el contrato de
> `std::hash` sólo obliga dentro de un tipo.
>
> En Magnitud-Signo hay dos codificaciones del cero, y si `−0` fuera alcanzable
> habría dos valores `==` con limbos distintos. **No lo es**: toda operación
> canonicaliza a `+0`. Comprobado, no supuesto.

```cpp
#include "fixed_point_hash.hpp"
#include <unordered_set>

std::unordered_set<nstd::sfixed_point_t<2, 1>> s;
s.insert(nstd::sfixed_point_t<2, 1>{1});
```

---

## `fixed_point_format.hpp`

| Especificación | Efecto |
|---|---|
| `{}` | seis cifras, como `printf("%f")` |
| `{:.3}` | tres cifras |
| `{:.0}` | sin coma |
| `{:>12}` `{:<12}` `{:^12}` | alineado a la derecha, izquierda o centrado |
| `{:*>8.1}` | con relleno |

**No** se admiten `{:x}`, `{:b}` ni las demás presentaciones enteras: en un
punto fijo una base distinta de diez pediría decidir qué se hace con la parte
fraccionaria, y eso no está decidido. Pedirlas lanza `std::format_error`, que es
un error de formato y no una salida rara.

El redondeo lo hace `to_string`, que consulta la perilla: **el formateo no
decide nada sobre el redondeo, sólo coloca**.

```cpp
#include "fixed_point_format.hpp"

const auto x = nstd::sfixed_point_t<2, 1>{2}
             + nstd::sfixed_point_t<2, 1>::desde_crudo(
                   nstd::sfixed_point_t<2, 1>::entero::one() << 63U);   // 2,5

std::format("{}", x);        // "2.500000"
std::format("{:.2}", x);     // "2.50"
std::format("{:>8.1}", x);   // "     2.5"
```

---

## `fixed_point_iostreams.hpp`

### Salida — `operator<<`

Respeta `std::setprecision`; sin él usa la de la corriente, que es **6**, igual
que `printf("%f")`. Redondea, porque `to_string` redondea.

### Entrada — `operator>>`

Acepta espacios delante, un signo opcional, cifras, y opcionalmente una coma
decimal con más cifras. Sin ninguna cifra pone `failbit` y **no toca** el
destino, como los operadores de la estándar.

> **La fracción se construye con `operator/`**, que ya redondea según la
> perilla. No es pereza: reimplementarla aquí sería escribir por segunda vez la
> maquinaria de [ADR-020](decisions/ADR-020-los-operadores-multiplicativos-del-punto-fijo.md)
> y arriesgarse a que las dos copias se separen.
>
> El límite de eso es que `10^k` tiene que caber en la parte entera: se usan
> sólo las cifras decimales cuyo `10^k` cabe, y con `F == N` **ninguna**.

```cpp
#include "fixed_point_iostreams.hpp"
#include <sstream>

nstd::sfixed_point_t<2, 1> x{};
std::istringstream is("-12.25");
is >> x;                        // exacto: 0,25 es una potencia de dos

std::ostringstream os;
os.precision(2);
os << x;                        // "-12.25"
```

---

## Tests

`tests/test_fixed_point_std.cpp` (E5 y E6) y `tests/test_fixed_point_limits.cpp`
(E4). El segundo cruza `numeric_limits` **contra la aritmética**, no contra una
tabla: comprobar que `epsilon()` vale `2^-(64F)` comparándolo con `2^-(64F)`
escrito a mano no puede fallar.

Los cinco modos de redondeo se instancian en los dos ficheros, porque cuatro
podrían estar mal y el test del modo por omisión pasaría igual.

## Version Notes

Añadidos el **23 sep 2026** (P4, etapas E4 y E5). Con ellos `fixed_point_t`
alcanza 24/24 en `scripts/check_acompanamiento_std.py`.
