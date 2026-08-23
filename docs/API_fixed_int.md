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

## Class Template `fixed_int_t<N, Sign, Form>`

### Template Parameters

| Parameter | Type | Values | Notes |
|-----------|------|--------|-------|
| `N` | `std::size_t` | `≥ 1` | Number of 64-bit limbs. `N=1` → 64-bit, `N=2` → 128-bit, `N=4` → 256-bit, `N=8` → 512-bit. |
| `Sign` | `signedness` | `unsigned_type`, `signed_type` | Default = `unsigned_type`. |
| `Form` | `representation_form` | `binnat`, `twos_complement` | Default depends on `Sign`. Currently only these two are implemented. |

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

- **v1.90.1 — Auditoria** — `div`/`mod` `constexpr`; `data` privado con
  `limb()`/`set_limb()`/`limbs()`/`limbs_ref()`; `try_from_string` con deteccion
  de desbordamiento; `to_string(base)`/`from_string(base)` en bases 2..36;
  saturacion definida de `inf`/`NaN` en el constructor desde punto flotante;
  contador de desplazamiento saturado; `operator<<`/`>>`, `std::formatter` y
  `std::hash` en headers propios.
- **v1.90** — `fixed_int_t<N, Sign, Form>` unified template, cross-sign operators (`+, -, *, /, %, &, |, ^`), cross-sign comparators (`==, !=, <, <=, >, >=`), cross-sign compounds (`+=, -=, *=, /=, %=, &=, |=, ^=`), `detail::mixed_iu_t`, Knuth Algorithm D divmod, Karatsuba multiplication for N=4/8.
- **v1.81 — Fase MS-INTEROP**: unary `operator+()`, shifts with cross-sign count, `operator<=>` (member + cross-type free), `mixed_iu_t` promoted to public `nstd::`, detection traits, `nstd::is_integral`/`is_arithmetic`/`is_signed`/`is_unsigned`, `nstd::make_signed`/`make_unsigned`, `nstd::integral`/`signed_integral`/`unsigned_integral` concepts, `std::common_type`, `std::numeric_limits`.
