# API Reference — fixed_width_int_t.hpp

> Generic N-limb fixed-width integer template (N × 64 bit). Generalization of
> the 128-bit `int128_param_t` to arbitrary widths.

## Synopsis

```cpp
#include "fixed_width_int_t.hpp"

namespace nstd {

template <std::size_t N,
          signedness Sign           = signedness::unsigned_type,
          representation_form Form  = (Sign == signedness::unsigned_type
                                       ? representation_form::binnat
                                       : representation_form::twos_complement)>
class fixed_int_t;

// Canonical aliases
template <std::size_t N> using uint_fixed_t = fixed_int_t<N, signed=unsigned, form=binnat>;
template <std::size_t N> using int_fixed_t  = fixed_int_t<N, signed=signed,   form=twos_complement>;

// C++ Usual Arithmetic Conversions trait (promoted public — v1.81)
template <std::size_t N, std::size_t M>
using mixed_iu_t = std::conditional_t<(N > M), int_fixed_t<N>, uint_fixed_t<M>>;

// Detection traits (v1.81)
template <typename T> struct is_fixed_int;
template <typename T> struct is_signed_fixed_int;
template <typename T> struct is_unsigned_fixed_int;
template <typename T> inline constexpr bool is_fixed_int_v          = ...;
template <typename T> inline constexpr bool is_signed_fixed_int_v   = ...;
template <typename T> inline constexpr bool is_unsigned_fixed_int_v = ...;

}  // namespace nstd
```

---

## Class Template `fixed_int_t<N, Sign, Form, Policy>`

### Template Parameters

| Parameter | Type | Values | Notes |
|-----------|------|--------|-------|
| `N` | `std::size_t` | `≥ 1` | Number of 64-bit limbs. `N=1` → 64-bit, `N=2` → 128-bit, `N=4` → 256-bit, `N=8` → 512-bit. |
| `Sign` | `signedness` | `unsigned_type`, `signed_type` | Default = `unsigned_type`. |
| `Form` | `representation_form` | `binnat`, `twos_complement` | Default depends on `Sign`. Currently only these two are implemented. |
| `Policy` | `overflow_policy` | `wrap`, `checked` | Default = `wrap`. El enum declara ademas `saturate` y `trap`, que **todavia no estan escritos** y hacen fallar el `static_assert` de la clase; existen desde el principio para no cambiar la ABI de la plantilla al anadirlos. Ver [ADR-009](decisions/ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md). |

### Storage Layout

Los limbos se guardan en orden little-endian: el indice 0 es el menos
significativo y el N-1 el mas significativo. **El array es privado desde v1.90.1**
(antes era publico); el acceso pasa por estos accesores:

```cpp
[[nodiscard]] constexpr std::uint64_t limb(std::size_t i) const noexcept;
              constexpr void          set_limb(std::size_t i, std::uint64_t v) noexcept;
[[nodiscard]] constexpr const std::array<std::uint64_t, N>& limbs()     const noexcept;
[[nodiscard]] constexpr       std::array<std::uint64_t, N>& limbs_ref()       noexcept;
```

Ninguno comprueba el rango: `i` debe estar en `[0, N)`.

| Antes de v1.90.1 | Desde v1.90.1 |
|---|---|
| `x.data[i]` (lectura) | `x.limb(i)` |
| `x.data[i] = v` | `x.set_limb(i, v)` |
| `x.data` (array completo) | `x.limbs()` / `x.limbs_ref()` |

**Consecuencia asumida:** `fixed_int_t` deja de ser *structural type*, asi que ya
no puede usarse como parametro no-tipo de plantilla. Sigue siendo trivialmente
copiable, de modo que `std::bit_cast` y `memcpy` funcionan igual. La decision
recupera el comportamiento de `int128_param_t` en phase-1.75, que tenia sus
limbos privados con `high()` / `low()`.

### Static Constants

```cpp
static constexpr signedness         sign;     // = Sign
static constexpr representation_form form;    // = Form
static constexpr bool               is_signed = (Sign == signedness::signed_type);
```

---

## Range and Limits

| Type | Range | Limbs | Bits |
|------|-------|-------|------|
| `uint_fixed_t<1>` | `[0, 2^64 - 1]` | 1 | 64 |
| `uint_fixed_t<2>` | `[0, 2^128 - 1]` | 2 | 128 |
| `uint_fixed_t<4>` | `[0, 2^256 - 1]` | 4 | 256 |
| `uint_fixed_t<8>` | `[0, 2^512 - 1]` | 8 | 512 |
| `int_fixed_t<1>` | `[-2^63, 2^63 - 1]` | 1 | 64 |
| `int_fixed_t<2>` | `[-2^127, 2^127 - 1]` | 2 | 128 |
| `int_fixed_t<4>` | `[-2^255, 2^255 - 1]` | 4 | 256 |
| `int_fixed_t<8>` | `[-2^511, 2^511 - 1]` | 8 | 512 |

Use `std::numeric_limits<T>::min()` / `::max()` after including `fixed_int_limits.hpp`.

---

## Construction

```cpp
constexpr fixed_int_t() noexcept = default;

template <typename T> explicit constexpr fixed_int_t(T v) noexcept;     // T integral, zero/sign-extends
template <std::size_t M, signedness S2, representation_form F2>
    explicit constexpr fixed_int_t(const fixed_int_t<M, S2, F2>& o) noexcept;  // cross-N, cross-sign
template <typename F>  explicit fixed_int_t(F v) noexcept;              // floating-point (not constexpr)
```

Cross-type construction:
- **Widening**: copies the lower `min(M, N)` limbs and fills the rest with sign-extension if source is signed-negative, else zeros.
- **Narrowing**: copies the lower `N` limbs and discards the rest (modular truncation).
- All cross-type constructors are `explicit` — matches built-in C++ behavior where narrowing/cross-sign conversions don't happen implicitly.

Construccion desde punto flotante (no es `constexpr`):

| Entrada | Resultado |
|---|---|
| finita en rango | truncada hacia cero |
| finita fuera de rango | truncada modulo 2^(64N), como entre enteros built-in |
| `NaN` | `0` |
| `+inf` | `max()` |
| `-inf` | `min()` con signo, `0` sin signo |

Los no finitos saturaban de forma indefinida hasta v1.90.1: `std::fmod(inf, 2^64)`
da `NaN` y el `static_cast<uint64_t>` siguiente era comportamiento indefinido.

---

## Operators

### Arithmetic (same type)

```cpp
+ - * / %                  // free + member
+= -= *= /= %=             // compound
++ --                      // pre / post
- +                        // unary (v1.81: unary + added)
~                          // bitwise NOT
```

All `mod 2^(64N)` (wraparound for unsigned, 2's-complement wrap for signed).

#### Cómo multiplica `operator*`, y cómo ajustarlo

`operator*` no tiene un solo algoritmo: elige según la anchura. El reparto está
medido, no supuesto — ver [PERFORMANCE.md](PERFORMANCE.md).

| anchura | camino | por qué |
|---|---|---|
| `N = 2` | especializado de 128 bits | `__int128` o `_umul128`, tres productos |
| `x * x`, `N ≥ 4` | **cuadrado** (`sqr_karatsuba_equilibrado`) | los dos términos del medio son el mismo: de 1,17× a 2,47×, mediana 1,5× |
| `N ≥ 22` | **Karatsuba con reparto equilibrado**, *cualquier* N | ya no exige potencia de dos, que era la causa del acantilado |
| `N = 3 … 21` | escolar **desenrollado por construcción** | de 1,4× a 4,9× más rápido que el bucle |
| evaluación constante | escolar en bucle | es el único camino `constexpr` |

**`x * x` se detecta comparando punteros, no valores.** `x * x` con la misma
variable entra en el camino del cuadrado; `a * b` con dos objetos que resulten
iguales, no. Comparar valores costaría N comparaciones para ganar en un caso
raro, mientras que detectar la variable repetida —que es lo que hace `pow` con
`base *= base`— cuesta una.

Por debajo, en los productos **completos** que Karatsuba usa internamente, hay un
cuarto algoritmo: **Toom-3**, que hace cinco productos de M/3 donde Karatsuba
hace tres de M/2 (exponente 1,465 frente a 1,585). Entra a partir de 1024 limbos
y da un 13–15 % en la parte alta del rango.

Los umbrales son macros y se pueden redefinir **antes** de incluir el header, o
desde la línea de órdenes con `-D`:

```cpp
#define NSTD_DESENROLLA_MAX 15   // desenrollar hasta N=15 (por defecto 21)
#define NSTD_KARATSUBA_MIN  16   // Karatsuba desde N=16   (por defecto 22)
#include "fixed_width_int_t.hpp"
```

| macro | por defecto | qué hace |
|---|---|---|
| `NSTD_DESENROLLA_MAX` | 21 | anchura máxima con el escolar desenrollado |
| `NSTD_KARATSUBA_MIN` | 22 | anchura mínima con Karatsuba, **sea o no potencia de dos** |
| `NSTD_KARATSUBA_MAX` | 4096 | anchura máxima con Karatsuba |
| `NSTD_LIMBOS_MAX` | 4096 | tope de N que la plantilla acepta; es una **guarda de pila** |
| `NSTD_TOOM3_MIN` | 1024 | anchura desde la que el producto completo **entra** en Toom-3 |
| `NSTD_TOOM3_REC` | 96 | hasta dónde se sigue repartiendo en tres **una vez dentro** |

**Los dos primeros son una sola frontera, no dos.** Con
`NSTD_DESENROLLA_MAX + 1 == NSTD_KARATSUBA_MIN` no queda ninguna anchura para el
escolar en bucle, y eso es deliberado: el bucle **no gana en ninguna de las 244
casillas medidas**. Si dejaran de ser consecutivos reaparecería el hueco que
costaba un 34 % de media y un 44 % con Intel. La suite lo comprueba con un
`static_assert` en `tests/test_config_macros.cpp`.

**Los dos de Toom-3 son dos a propósito.** El mismo M se comporta distinto según
el papel: entrar en Toom-3 con 512 limbos **pierde** un 5–8 %, pero un
subproducto de ~512 limbos *dentro* de uno de 4096 sale mejor con Toom-3 que con
Karatsuba. Con un solo umbral no se pueden tener las dos cosas.

**Subir `NSTD_DESENROLLA_MAX` cuesta tiempo de compilación** —son `N(N+1)/2`
productos en línea recta— pero solo lo paga quien **instancia** esa anchura.
Bajarlo no acelera nada: solo renuncia a la ganancia. Con MSVC, además, pasar de
31 rompe el límite de secciones de COFF (`C1128`) y obliga a `/bigobj`.

Todos los valores por defecto salen de barridos publicados en
[PERFORMANCE.md](PERFORMANCE.md), y los bancos que los producen están en el
árbol: `benchmark_barrido_desenrollado`, `benchmark_barrido_karatsuba`,
`benchmark_rango_alto` y `benchmark_toom3`. Si se cambian, conviene rehacerlos —y
tener presente que **son de una máquina**: GMP publica para su umbral equivalente
de Karatsuba un rango de 16 a 46 limbos entre modelos.

### Bitwise (same type)

```cpp
& | ^                      // free + member
&= |= ^=                   // compound
<< >>                      // shift by unsigned, or by fixed_int_t<M,S2,F2> (v1.81)
<<= >>=                    // compound (v1.81: cross-sign count accepted)
```

Right shift on signed types is arithmetic (sign-extends).

### Comparison (same type)

```cpp
== != < <= > >=
<=>                        // v1.81: returns std::strong_ordering
```

The 6 manual comparators and `<=>` coexist; explicit overloads win during overload resolution.

### Cross-N and Cross-Sign (v1.90 + v1.81)

Per C++ Usual Arithmetic Conversions, both operands are promoted to
`nstd::mixed_iu_t<N, M>` (cross-sign) or `fixed_int_t<max(N,M), Sign>`
(same-sign cross-N), then the operation runs in that wider type.

**Cross-sign free operators (all 28 overloads, both orientations):**

```cpp
mixed_iu_t<N,M> operator+(int_fixed_t<N>,  uint_fixed_t<M>);
mixed_iu_t<N,M> operator+(uint_fixed_t<M>, int_fixed_t<N>);
// ...same for -, *, /, %, &, |, ^
bool operator==(int_fixed_t<N>, uint_fixed_t<M>);
// ...same for !=, <, <=, >, >=
std::strong_ordering operator<=>(fixed_int_t<N,S1,F1>, fixed_int_t<M,S2,F2>);  // v1.81
```

**Cross-sign compound assignment (members, `enable_if<S2 != Sign>`):**

```cpp
fixed_int_t& operator+=(const fixed_int_t<M, S2, F2>&);
fixed_int_t& operator-=(const fixed_int_t<M, S2, F2>&);
fixed_int_t& operator*=(const fixed_int_t<M, S2, F2>&);
fixed_int_t& operator/=(const fixed_int_t<M, S2, F2>&);
fixed_int_t& operator%=(const fixed_int_t<M, S2, F2>&);
fixed_int_t& operator&=(const fixed_int_t<M, S2, F2>&);
fixed_int_t& operator|=(const fixed_int_t<M, S2, F2>&);
fixed_int_t& operator^=(const fixed_int_t<M, S2, F2>&);
// Computed in mixed_iu_t<N,M>, then assigned to LHS (truncates if LHS is narrower).
```

### UAC Rule (`mixed_iu_t`)

```cpp
template <std::size_t N, std::size_t M>
using mixed_iu_t = std::conditional_t<(N > M), int_fixed_t<N>, uint_fixed_t<M>>;
```

- **N > M** → `int_fixed_t<N>` (signed wider rank wins, unsigned zero-extends)
- **N ≤ M** → `uint_fixed_t<M>` (unsigned rank ≥ signed → signed converts to unsigned)

This mirrors C++ built-in `signed op unsigned` semantics exactly. Convention: `N` is the rank of the signed side, `M` of the unsigned side.

Example:

```cpp
nstd::int_fixed_t<2>  a = -1;       // a is i128 signed
nstd::uint_fixed_t<2> b = 5;        // b is u128 unsigned
auto r = a + b;                     // r is uint_fixed_t<2> (unsigned wins, N==M)
// r == (2^128 - 1) + 5 mod 2^128 = 4

auto s = nstd::int_fixed_t<4>{-1} + nstd::uint_fixed_t<2>{5};
// s is int_fixed_t<4> (signed wins, N=4 > M=2). s == 4.
```

### Shifts cross-sign (v1.81)

```cpp
template <std::size_t M, signedness S2, representation_form F2>
constexpr fixed_int_t operator<<(const fixed_int_t<M, S2, F2>& count) const noexcept;
// + operator>>, operator<<=, operator>>=
```

LHS type is preserved (mirrors built-in `int << anything == int`). Count is reduced to `unsigned`; negative signed counts produce zero (wraps to huge unsigned via two's complement, then `count >= 64*N` returns zero — same UB-handling as built-in).

---

## Static Factory Methods

```cpp
static constexpr fixed_int_t zero() noexcept;
static constexpr fixed_int_t one()  noexcept;
static constexpr fixed_int_t min()  noexcept;   // 0 if unsigned, -2^(64N-1) if signed
static constexpr fixed_int_t max()  noexcept;   // 2^(64N) - 1 if unsigned, 2^(64N-1) - 1 if signed
```

These are also exposed via `std::numeric_limits<T>::min()` / `::max()` (include `fixed_int_limits.hpp`).

---

## Detection Traits (v1.81)

```cpp
template <typename T> struct is_fixed_int          : /* true_type iff T is some fixed_int_t */;
template <typename T> struct is_signed_fixed_int   : /* true_type iff T is fixed_int_t<N, signed_type, ...> */;
template <typename T> struct is_unsigned_fixed_int : /* true_type iff T is fixed_int_t<N, unsigned_type, ...> */;

template <typename T> inline constexpr bool is_fixed_int_v;
template <typename T> inline constexpr bool is_signed_fixed_int_v;
template <typename T> inline constexpr bool is_unsigned_fixed_int_v;
```

All three trait classes strip cv-qualifiers from `T` before matching.

---

## Cross-Sign Interop Cheatsheet

| Built-in C++ | `fixed_int_t` equivalent | Result type |
|---|---|---|
| `unsigned + signed` (same rank) | `uint_fixed_t<N> + int_fixed_t<N>` | `uint_fixed_t<N>` |
| `signed + unsigned` (signed wider) | `int_fixed_t<N> + uint_fixed_t<M>` with `N > M` | `int_fixed_t<N>` |
| `signed + unsigned` (unsigned wider) | `int_fixed_t<N> + uint_fixed_t<M>` with `N ≤ M` | `uint_fixed_t<M>` |
| `(int)INT_MIN > (unsigned)0` is **true** | `int_fixed_t<2>::min() > uint_fixed_t<2>{0}` is **true** | bool — gotcha! |
| `(unsigned)-1 == UINT_MAX` | `static_cast<uint_fixed_t<N>>(int_fixed_t<N>{-1}) == max()` | (sign extension fills all limbs) |
| `int << signed_int` is UB if negative | `int_fixed_t<N> << int_fixed_t<M>{-1}` returns zero | matches UB-handling |
| `<=>` (C++20) | `<=>` (v1.81) | `std::strong_ordering` |

---

## `constexpr` (v1.90.1)

**Todas** las operaciones son evaluables en tiempo de compilacion, division y
modulo incluidos. Hasta v1.90.1 `divmod`, `/`, `%`, `/=` y `%=` eran solo de
ejecucion, porque los caminos por plataforma usaban intrinsecos (`_udiv128`,
`_umul128`, `asm divq`) que no son constexpr.

```cpp
static_assert((uint256_fixed_t{1000000} / uint256_fixed_t{7}) == uint256_fixed_t{142857});
static_assert((int128_fixed_t{-7} % int128_fixed_t{3}) == int128_fixed_t{-1});
static_assert(nstd::sqrt(uint256_fixed_t{144}) == uint256_fixed_t{12});
```

La division por cero lanza `std::domain_error`. En contexto constante eso hace
que la expresion no sea constante, es decir: **error de compilacion**, igual que
`1/0` con un `int`. Es el comportamiento deseado, no una limitacion.

Tambien son `constexpr` las funciones que dependen de la division: `sqrt`, `lcm`,
`gcd`, `pow`, `mul_wide` y `checked_add` / `checked_sub` / `checked_mul`.

---

## Conversion a y desde cadena

```cpp
[[nodiscard]] std::string to_string() const;                       // base 10
[[nodiscard]] std::string to_string(int base) const;               // base 2..36

[[nodiscard]] static parse_result<fixed_int_t>
              try_from_string(const char* s, int base = 10) noexcept;   // no lanza
static fixed_int_t from_string(const char* s, int base = 10);           // lanza
```

### Bases

`base` en `[2, 36]`, o `0` para deducirla del prefijo. Los digitos por encima de
9 se escriben en **mayusculas**. Una base fuera de rango hace que `to_string`
lance `std::invalid_argument` y que `try_from_string` devuelva
`parse_error::invalid_base`.

Prefijos aceptados al parsear: `0x`/`0X` (16), `0b`/`0B` (2), `0o`/`0O` (8),
tanto con `base = 0` como cuando coinciden con la base pedida.

> Un `0` suelto **no** se interpreta como octal: `from_string("077", 0)` da 77,
> no 63. Es una desviacion deliberada de `strtoul`.

### Errores

`try_from_string` no lanza; devuelve `parse_result<fixed_int_t>` con el codigo y
el indice del caracter culpable.

| `parse_error` | Cuando |
|---|---|
| `success` | todo bien; `error_index == std::string::npos` |
| `null_pointer` | puntero nulo |
| `empty_string` | cadena vacia |
| `no_digits` | solo el signo, o solo el prefijo |
| `invalid_character` | el caracter no es alfanumerico (`$`, espacio...) |
| `digit_out_of_range` | si lo es, pero su valor es `>= base` (`'9'` en base 8, `'x'` en base 10) |
| `invalid_base` | `base` fuera de `[2, 36]` y distinta de 0 |
| `overflow` | el valor no cabe en el tipo |

`from_string` lanza `std::invalid_argument` para todos salvo `overflow`, que
lanza **`std::out_of_range`** (como `std::stoull`).

> Hasta v1.90.1 el desbordamiento **no se detectaba**:
> `uint_fixed_t<4>::from_string("2^256")` devolvia `0` en silencio.

Los tipos sin signo **no** aceptan signo, ni `+` ni `-`. Los que tienen signo
aceptan ambos.

---

## Desplazamientos con contador `fixed_int_t`

El contador se satura a `64*N` cuando no cabe en `[0, 64N)` o es negativo, que es
el camino de «desplazamiento completo» de la sobrecarga `unsigned`: `0` para `<<`
y relleno de signo para `>>` con signo. **No hay comportamiento indefinido**.

```cpp
uint256_fixed_t c{}; c.set_limb(1, 1);           // 2^64
assert((uint256_fixed_t{1} << c).is_zero());     // antes de v1.90.1 devolvia 1
```

Hasta v1.90.1 el contador se truncaba a `static_cast<unsigned>(shift.limb(0))`,
lo que perdia por dos sitios: los limbos altos y los bits por encima de 32.

---

## Funciones libres (P1.5 tramo 1)

Portadas desde `int128_param_bits.hpp`, `int128_param_cmath.hpp` y
`int128_param_numeric.hpp` como parte de la retirada de `int128_param_t`
([ADR-006](decisions/ADR-006-migracion-int128-param-a-fixed-int.md)). Todas
estan en `nstd::`, son plantillas sobre **los cuatro** parametros y funcionan en
contexto `constexpr`.

En las firmas, `T` es `fixed_int_t<N, Sign, Form, Policy>` con cualquier signo, y
`U` es la variante **sin signo**.

### Rotaciones y nombres de `<bit>`

| Funcion | Firma | Semantica | Coste |
|---|---|---|---|
| `rotl` | `(const T& x, int s) -> T` | Rotacion a la izquierda sobre los `64*N` bits. `s` se toma modulo la anchura y **admite negativos**, que rotan al otro lado, igual que `std::rotl`. | O(N) |
| `rotr` | `(const T& x, int s) -> T` | `rotl(x, -s)`. | O(N) |
| `countl_zero` | `(const T& x) -> unsigned` | Ceros por delante; `64*N` si `x` es cero. Alias del metodo `count_leading_zeros()`. | O(N) |
| `countr_zero` | `(const T& x) -> unsigned` | Ceros por detras; `64*N` si `x` es cero. Alias de `count_trailing_zeros()`. | O(N) |
| `popcount` | `(const T& x) -> unsigned` | Bits a uno. Alias del metodo homonimo. | O(N) |
| `bit_width` | `(const T& x) -> unsigned` | Bits necesarios para representarlo; 0 si es cero. | O(N) |
| `is_power_of_2` | `(const T& x) -> bool` | Un solo bit a uno **y no negativo**. | O(N) |

`rotl` y `rotr` rotan el patron de bits completo, sin tratar el signo aparte. El
`rotl` de `int128_param_t` si lo trataba en Magnitud-Signo, pero eso es propio de
esa representacion y no aplica mientras `fixed_int_t` solo admita `binnat` y
complemento a dos ([ADR-011](decisions/ADR-011-sin-signo-equivale-a-binnat.md)).

> **`is_power_of_2` y el minimo con signo.** En complemento a dos,
> `int_fixed_t<N>::min()` tiene puesto **un solo bit** --el de signo-- asi que un
> `popcount(x) == 1` a secas diria que es potencia de dos. No lo es. De ahi la
> condicion de no ser negativo.

### Comparacion y mezcla

| Funcion | Firma | Semantica | Coste |
|---|---|---|---|
| `min` | `(const T& a, const T& b) -> const T&` | El menor; `a` si son iguales. | O(N) |
| `max` | `(const T& a, const T& b) -> const T&` | El mayor; `a` si son iguales. | O(N) |
| `clamp` | `(const T& x, const T& lo, const T& hi) -> const T&` | `lo` si `x < lo`, `hi` si `x > hi`, si no `x`. **Pre:** `lo <= hi`. | O(N) |
| `midpoint` | `(const U& a, const U& b) -> U` | `(a + b) / 2` redondeado hacia `a`, **sin desbordar**. Solo sin signo. | O(N) |
| `abs_diff` | `(const U& a, const U& b) -> U` | Diferencia en valor absoluto. Solo sin signo. | O(N) |
| `abs` | `(const T& x) -> T` | Valor absoluto. **Acepta tambien sin signo**, donde es la identidad. | O(N) |

`min`, `max` y `clamp` devuelven **referencia**, como los de `std::`, con la
misma trampa: no guardar el resultado de una llamada sobre temporales mas alla
de la expresion completa.

`midpoint` no calcula `(a + b) / 2` sino `a + (b - a) / 2`: la primera desborda
precisamente cuando hace falta. Por eso `midpoint(U::max(), U::max())` da
`U::max()` y no basura.

`abs` libre acepta sin signo aunque el **metodo** `abs()` solo exista con signo.
Pedirle el absoluto a un `uint` suele ser un error de quien escribe, pero la
funcion libre la llama codigo generico que vale para los dos, y negar el caso
obliga a un `if constexpr` en cada sitio que la use.

### Predicados y funciones enteras

| Funcion | Firma | Semantica | Coste |
|---|---|---|---|
| `is_even` / `is_odd` | `(const T& x) -> bool` | Bit mas bajo. | O(1) |
| `sign` | `(const T& x) -> int` | `-1`, `0` o `+1`. Sin signo solo devuelve `0` o `+1`. | O(N) |
| `ilog2` | `(const U& x) -> unsigned` | `bit_width(x) - 1`. **Lanza `std::domain_error` si `x` es cero.** | O(N) |
| `factorial<N, Form, Policy>` | `(unsigned n) -> U` | `n!` truncado a `64*N` bits. | O(n) productos |
| `divmod` | `(const T& a, const T& b) -> std::pair<T, T>` | Cociente y resto de una vez. **Lanza `std::domain_error` si `b` es cero.** | Un solo Knuth D |

Dos comportamientos se apartan a proposito del header viejo:

- **`ilog2(0)` lanza**; el viejo devolvia `-1` y lo documentaba como
  comportamiento indefinido. El logaritmo de cero no existe, y devolver un
  numero es dar por bueno un calculo que no lo es.
- **`divmod(a, 0)` lanza**; el viejo devolvia `{0, 0}`. Ademas asi coincide con
  lo que ya hacian `/` y `%` en `fixed_int_t`.

`divmod` cuesta lo mismo que **una** division: el algoritmo de Knuth produce los
dos resultados a la vez, y pedirlos con `/` y `%` por separado lo ejecuta dos
veces.

#### Cómo divide `divmod`, y qué determina su coste

| caso | camino |
|---|---|
| divisor de **un limbo** | N divisiones de hardware encadenadas |
| divisor de **≥ 2 limbos** | **Knuth D** (TAOCP vol. 2 §4.3.1) |
| `N = 2` | especializado de 128 bits |
| evaluación constante | los mismos, en su versión portable |

**El coste no depende sólo de N: depende también de cuántos limbos
*significativos* tiene el divisor.** Knuth D hace `(N − n + 1)` pasadas de `O(n)`
trabajo cada una, o sea `O((N−n)·n)`, donde `n` son los limbos no nulos de `b`.
Eso tiene su **máximo en `n = N/2`** y se desploma en los dos extremos: con `n=1`
entra el camino rápido y con `n=N` hay una sola pasada.

En la práctica: **dividir por un número «grande» de anchura parecida al dividendo
es barato; dividir por uno de la mitad de ancho es lo caro.** Las cifras, en
[PERFORMANCE.md](PERFORMANCE.md).

| macro | por defecto | qué hace |
|---|---|---|
| `NSTD_DIV_COMPRUEBA_PRECONDICIONES` | apagada | comprueba en cada llamada las precondiciones internas de la división y aborta nombrando la que se rompa |

La macro **no es para producción**: cuesta una comparación por división. Existe
porque en x86-64 la división de 128/64 usa una instrucción `divq`, que exige que
el cociente quepa en 64 bits; si un cambio futuro rompiera esa garantía, el
procesador aborta el proceso (`STATUS_INTEGER_OVERFLOW`) en vez de dar un
resultado equivocado. Con la macro encendida, el aborto **dice cuál precondición
se rompió**, y en evaluación constante se convierte en error de compilación.
`scripts/check_precondiciones_div.py` compila y ejecuta la suite entera así.

`factorial` desborda muy pronto: **34! cabe** en 128 bits (2,95e38 frente a
3,40e38) y **35! ya no**. Con `wrap` envuelve en silencio, que es lo que hacen
los enteros del lenguaje; con `checked` el resultado queda marcado y `valid()`
devuelve `false`. No se pone un tope artificial porque el que cabe depende de N.

### Ejemplo

```cpp
#include "fixed_width_int_t.hpp"
#include <cassert>
#include <cstdint>

using U = nstd::uint_fixed_t<2>; // 128 bits

int main()
{
    // Rotar es reversible y conserva el numero de bits a uno.
    constexpr U x{std::uint64_t{0xDEADBEEF}};
    static_assert(nstd::rotr(nstd::rotl(x, 37), 37) == x);
    static_assert(nstd::popcount(nstd::rotl(x, 41)) == nstd::popcount(x));

    // El caso que justifica que `midpoint` exista: (a + b) desbordaria.
    static_assert(nstd::midpoint(U::max(), U::max()) == U::max());

    // Cociente y resto de una vez, al precio de una sola division.
    constexpr auto qr = nstd::divmod(U{std::uint64_t{1000}}, U{std::uint64_t{7}});
    static_assert(qr.first == U{std::uint64_t{142}});
    static_assert(qr.second == U{std::uint64_t{6}});

    // Con `checked`, el desbordamiento del factorial queda marcado.
    const auto f = nstd::factorial<2, nstd::representation_form::binnat,
                                   nstd::overflow_policy::checked>(35);
    assert(!f.valid());
    return 0;
}
```

## Funciones libres (P1.5 tramo 2)

### `mulhi` y `mullo` — las dos mitades del producto

| Funcion | Firma | Semantica | Coste |
|---|---|---|---|
| `mulhi` | `(const U& a, const U& b) -> U` | Los `64*N` bits **altos** de `a * b`. | Un `mul_wide` |
| `mulhi` | `(const I& a, const I& b) -> I` | Idem con signo; el resultado sale **con signo**. | Un `mul_wide` |
| `mullo` | `(const T& a, const T& b) -> T` | Los `64*N` bits **bajos**. Es `a * b`. | Un `operator*` |

`mulhi` es lo que `operator*` **tira**: no se puede sacar del producto modular,
hace falta el de doble anchura. Junto a `mullo` reconstruye el producto exacto
sin declarar un tipo de `2N` limbos:

```cpp
using U = nstd::uint_fixed_t<2>;
using W = nstd::uint_fixed_t<4>;
constexpr U a = U::max(), b = U::max();
static_assert((W{nstd::mulhi(a, b)} << 128U) + W{nstd::mullo(a, b)} == nstd::mul_wide(a, b));
```

Con signo, la mitad alta lleva la **extension de signo** del producto completo, y
por eso se devuelve con signo: `mulhi(min(), 2)` es `-1`, porque
`min() * 2 == -2^(64N)` y su mitad alta es `-1`, no `2^(64N) - 1`. Leerla sin
signo es exactamente el error que tenia `producto_desborda` antes de P1.3.

`mulhi` **no marca** con la politica `checked`, y `mullo` **si**. No es una
inconsistencia: en `mulhi` el resultado exacto se calcula en `2N` limbos y solo
se elige que mitad devolver, asi que no hay nada que desbordar; `mullo` es
`operator*`, que es modular y por tanto puede perder bits.

> **`widening_mul` no existe aqui**: es exactamente `mul_wide`, que ya estaba.
> Igual que con `power`/`pow`, no se anade un segundo nombre para la misma
> operacion. Quien venga de `int128_param_t` busca `mul_wide`.

### La politica ya no bloquea: `pow`, `gcd`, `lcm`, `sqrt`, `mul_wide`

Estas nueve firmas estaban escritas sobre `uint_fixed_t<N>` / `int_fixed_t<N>`,
es decir, con la politica **fijada** a la de por defecto. Sobre un tipo
`checked` daban «no matching function». Ahora llevan `Policy`, que es
**deducible** del argumento, asi que ninguna llamada existente cambia:

```cpp
using C = nstd::uint_fixed_t<2, nstd::overflow_policy::checked>;
static_assert(nstd::gcd(C{12u}, C{18u}) == C{6u});          // antes no compilaba
static_assert(std::is_same_v<decltype(nstd::gcd(C{}, C{})), C>);
```

La politica **se conserva** en el resultado, incluso cuando cambia el signo
--`gcd` y `lcm` con signo devuelven sin signo--, que es lo mismo que hacen
`make_signed` y `make_unsigned` por
[ADR-008](decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md).

---

## Qué funciona con qué combinación de parámetros

No a mano: [`MATRIZ_DE_PARIDAD.md`](MATRIZ_DE_PARIDAD.md) la genera compilando
una sonda por celda. 42 capacidades × 4 combinaciones de signo y política.

Lo que hoy **no** vale para las cuatro:

| Capacidad | Dónde vale | Por qué |
|---|---|---|
| `sqrt`, `midpoint`, `abs_diff`, `ilog2` | solo sin signo | Su definición es sobre naturales |
| las siete `checked_*` y `saturating_*` | solo con `wrap` | **Sin decidir** si es hueco o diseño; ver la matriz |

---

## Related Headers

| Header | Provides |
|--------|----------|
| `fixed_int_traits_specializations.hpp` | `nstd::is_integral/is_signed/is_unsigned`, `nstd::make_signed/unsigned`, `std::common_type` |
| `fixed_int_concepts.hpp` | `nstd::integral/signed_integral/unsigned_integral` (aglutinating built-ins + `fixed_int_t`); detection concepts |
| `fixed_int_limits.hpp` | `std::numeric_limits<fixed_int_t<N, Sign, Form>>` for all N, Sign, Form |
| `fixed_int_iostreams.hpp` | `operator<<` y `operator>>` respetando los manipuladores del flujo |
| `fixed_int_format.hpp` | `std::formatter`, especificacion completa |
| `fixed_int_hash.hpp` | `std::hash`, para `unordered_map` / `unordered_set` |

Los tres ultimos son de v1.90.1; ver
[API_fixed_int_stl.md](API_fixed_int_stl.md).

See [API_fixed_int_traits.md](API_fixed_int_traits.md) for the complete reference.

---

## Version Notes

- **phase-1.80 (P1.5 tramo 2)** — `mulhi` (con y sin signo) y `mullo`; y las
  nueve firmas de `pow`/`gcd`/`lcm`/`sqrt`/`mul_wide` generalizadas al cuarto
  parametro, que antes fijaban la politica por defecto.
- **phase-1.80 (P1.5 tramo 1)** — 19 funciones libres portadas de
  `int128_param_{bits,cmath,numeric}.hpp`: `rotl`/`rotr`, los nombres de
  `<bit>`, `is_power_of_2`, `min`/`max`/`clamp`/`midpoint`/`abs_diff`/`abs`,
  `is_even`/`is_odd`/`sign`/`ilog2`/`factorial`/`divmod`. Todas con los
  cuatro parametros de plantilla.
- **phase-1.80 (P1.1)** — cuarto parametro de plantilla `overflow_policy`.
- **v1.90.1 — Auditoria** — `div`/`mod` `constexpr`; `data` privado con
  `limb()`/`set_limb()`/`limbs()`/`limbs_ref()`; `try_from_string` con deteccion
  de desbordamiento; `to_string(base)`/`from_string(base)` en bases 2..36;
  saturacion definida de `inf`/`NaN` en el constructor desde punto flotante;
  contador de desplazamiento saturado; `operator<<`/`>>`, `std::formatter` y
  `std::hash` en headers propios.
- **v1.90** — `fixed_int_t<N, Sign, Form>` unified template, cross-sign operators (`+, -, *, /, %, &, |, ^`), cross-sign comparators (`==, !=, <, <=, >, >=`), cross-sign compounds (`+=, -=, *=, /=, %=, &=, |=, ^=`), `detail::mixed_iu_t`, Knuth Algorithm D divmod, Karatsuba multiplication (N=4/8 entonces; desde el 6 sep 2026, N>=32 y escolar desenrollado por debajo).
- **v1.81 — Fase MS-INTEROP**: unary `operator+()`, shifts with cross-sign count, `operator<=>` (member + cross-type free), `mixed_iu_t` promoted to public `nstd::`, detection traits, `nstd::is_integral`/`is_arithmetic`/`is_signed`/`is_unsigned`, `nstd::make_signed`/`make_unsigned`, `nstd::integral`/`signed_integral`/`unsigned_integral` concepts, `std::common_type`, `std::numeric_limits`.
