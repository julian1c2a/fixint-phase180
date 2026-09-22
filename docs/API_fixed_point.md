# API Reference — fixed_point_t.hpp

> Binary fixed-point numbers built on `fixed_int_t`: `N` limbs total, `F` of
> them fractional. A fixed-point value **is an integer with a scale**, so it
> carries no arithmetic of its own.

## Synopsis

```cpp
#include "fixed_point_t.hpp"

namespace nstd {

enum class rounding_mode : std::uint8_t {
    to_nearest_even,   // default
    to_nearest_away,
    toward_zero,
    toward_neg_inf,
    toward_pos_inf,
};

template <std::size_t N, std::size_t F,
          signedness          Sign     = signedness::unsigned_type,
          representation_form Form     = representation_form::binnat,
          overflow_policy     Policy   = overflow_policy::wrap,
          rounding_mode       Redondeo = rounding_mode::to_nearest_even>
class fixed_point_t;

template <std::size_t N, std::size_t F, overflow_policy P = overflow_policy::wrap,
          rounding_mode R = rounding_mode::to_nearest_even>
using ufixed_point_t = /* unsigned, binnat */;

template <std::size_t N, std::size_t F, overflow_policy P = overflow_policy::wrap,
          rounding_mode R = rounding_mode::to_nearest_even>
using sfixed_point_t = /* signed, two's complement */;

using ufixed_64_64_t = ufixed_point_t<2, 1>;   // Q64.64, unsigned
using sfixed_64_64_t = sfixed_point_t<2, 1>;   // Q64.64, signed

}
```

---

## The value model

A `fixed_point_t<N, F, ...>` stores one `fixed_int_t<N, Sign, Form, Policy>`,
reachable through `crudo()`. The represented value is

```
value  =  crudo()  /  2^(64·F)
```

Everything follows from that identity. There is **no new arithmetic** in this
header: Knuth D, Karatsuba, Möller–Granlund, Toom-3, the four representations
and the overflow policy all come from the integer type, already tested.

See [ADR-019](decisions/ADR-019-punto-fijo-es-un-entero-con-escala.md) and
[ADR-020](decisions/ADR-020-los-operadores-multiplicativos-del-punto-fijo.md).

### Why `N` and `F` count **limbs**, not bits

The integer part is `E = N − F` and is deduced. Parameterising by bits would
leave spare bits in the top limb whenever `E+F` is not a multiple of 64, and
then *every* operation would have to mask them and read the sign from bit
`E+F−1` instead of bit 63. With limbs, the storage **is exactly** a
`fixed_int_t<N>`: no padding, nothing to mask.

### Two edge shapes are legal and behave differently

| | Meaning |
|---|---|
| `F == 0` | An integer under another name. `es_entero()` is always `true` and there is nothing to round — but `to_string(3)` still writes `7.000`, because `decimales` means what it says. |
| `F == N` | Purely fractional, i.e. `[0, 1)` unsigned. `one()`, `operator++` and `operator--` **do not exist** there and say so with a `static_assert`. |

`F > N` is a compile error.

---

## Template parameters

| Parameter | Meaning |
|---|---|
| `N` | Total limbs of storage (64 bits each). |
| `F` | Fractional limbs. Must satisfy `F <= N`. |
| `Sign` | `unsigned_type` or `signed_type`, as in the integer. |
| `Form` | `binnat`, `twos_complement`, `magnitude_sign` or `excess_k`. The sign bit is the most significant bit of the integer part, **not an extra bit**, so all four occupy the same storage. |
| `Policy` | Overflow policy, inherited from the integer. `wrap` and `checked` are implemented. |
| `Redondeo` | What to do with what does not fit *below*. Last in the list, so adding it changed no existing use. |

> `Form` is **not observable from behaviour**
> ([ADR-018](decisions/ADR-018-la-representacion-no-es-observable.md)). Two
> values that differ only in `Form` compare, print, shift and arithmetise
> identically; only `crudo().limb()` differs.

### Published constants

```cpp
static constexpr std::size_t num_limbs;              // N
static constexpr std::size_t limbos_fraccionarios;   // F
static constexpr std::size_t limbos_enteros;         // N − F
static constexpr std::size_t escala_bits;            // 64·F
static constexpr signedness          sign;
static constexpr representation_form form;
static constexpr overflow_policy     policy;
static constexpr rounding_mode       redondeo;
using entero = fixed_int_t<N, Sign, Form, Policy>;
```

These are enough to rebuild the type from generic code.

---

## Construction

```cpp
constexpr fixed_point_t() noexcept;                  // zero
template <typename T> explicit constexpr fixed_point_t(T v) noexcept;
static constexpr fixed_point_t desde_crudo(const entero &x) noexcept;
constexpr const entero &crudo() const noexcept;
```

**The two constructors mean different things and differ by a factor of
`2^(64·F)`.** The integral constructor takes an **integer part** — it scales
up. `desde_crudo` takes the **already scaled** raw integer.

```cpp
sfixed_64_64_t a{3};                                  // three
auto b = sfixed_64_64_t::desde_crudo(entero{3});      // three epsilons
assert(a != b);
```

That is why `desde_crudo` is a named function and not a second constructor:
they would have the same signature and confusing them is a silent error.

The integral constructor is `explicit`, like everything in this library
(ADR-001) — without it, `x + 1` would compile with two possible meanings.

### Constants

```cpp
static constexpr fixed_point_t zero()    noexcept;
static constexpr fixed_point_t one()     noexcept;   // static_assert(F < N)
static constexpr fixed_point_t epsilon() noexcept;   // 2^-(64·F), the ulp
static constexpr fixed_point_t max()     noexcept;
static constexpr fixed_point_t min()     noexcept;   // zero when unsigned
```

`epsilon()` is the step between two consecutive representable values —
`desde_crudo(entero::one())`.

---

## Exact operations — no rounding happens here

Two values of the same type share a scale, so adding them is adding the
underlying integers. Nothing to adjust, nothing to round, and the overflow
policy works by itself because the integer carries it.

```cpp
constexpr fixed_point_t operator+(const fixed_point_t &) const noexcept;
constexpr fixed_point_t operator-(const fixed_point_t &) const noexcept;
constexpr fixed_point_t operator-() const noexcept;
constexpr fixed_point_t operator+() const noexcept;
constexpr fixed_point_t &operator+=(const fixed_point_t &) noexcept;
constexpr fixed_point_t &operator-=(const fixed_point_t &) noexcept;

// multiplying by an INTEGER does not change the scale, so it is exact
constexpr fixed_point_t  operator* (const entero &k) const noexcept;
constexpr fixed_point_t &operator*=(const entero &k)       noexcept;

// comparison and ordering: the scale is common, so the integer decides
constexpr bool operator==/!=/</>/<=/>=(const fixed_point_t &) const noexcept;
constexpr std::strong_ordering operator<=>(const fixed_point_t &) const noexcept;
```

### The complete operator set

| Group | Operators | Rounds? |
|---|---|---|
| additive | `+` `-` `+=` `-=`, unary `+` `-` | no — same scale |
| increment | `++` `--`, pre and post | no — adds **one** |
| by an integer | `* (entero)` `*= (entero)` | no — the scale does not change |
| multiplicative | `*` `/` `*=` `/=` | **yes** |
| remainder | `%` `%=` | no — the remainder is always representable |
| shifts | `<<` `<<=` (exact), `>>` `>>=` (**rounds**) | see above |
| comparison | `==` `!=` `<` `>` `<=` `>=` `<=>` | no |

Deliberately absent: `&` `|` `^` `~` and their compound forms — see below.

The ordering is **strong**: there is no NaN and no second zero here — those
belong to floating point, not to fixed point.

### `++` and `--` add **one**, not one epsilon

```cpp
constexpr fixed_point_t &operator++();      // static_assert(F < N)
constexpr fixed_point_t &operator--();      // static_assert(F < N)
constexpr fixed_point_t  operator++(int);
constexpr fixed_point_t  operator--(int);
```

`++x` is `x += 1`, which is what `float` and `double` do in C++.

Advancing to the next representable value is tempting, because unlike an
integer this type *has* one — but that operation is already named in the
standard and it is not `++`, it is `std::nextafter`. Giving `++` a different
meaning from the one it has in `float` would be a silent surprise in generic
code (ADR-020).

They are exact: adding one does not change the scale.

---

## Operations that round

```cpp
constexpr fixed_point_t  operator* (const fixed_point_t &) const noexcept;
constexpr fixed_point_t  operator/ (const fixed_point_t &) const;   // may throw
constexpr fixed_point_t &operator*=(const fixed_point_t &)       noexcept;
constexpr fixed_point_t &operator/=(const fixed_point_t &);         // may throw
```

`a * b` is `mul_wide` plus a shift: the product of the raws is `A·B / 2^(2k)`,
so the result's raw is `A·B / 2^k` and exactly the low `k` bits are dropped.

`a / b` pre-scales the dividend to `2N` limbs and divides; the remainder of
that division is what decides the rounding.

> **No guard bits are stored, and none are needed.** `mul_wide` returns the
> **exact** `2N`-limb product, so every bit that is about to be discarded has
> already been computed. Guard bits come from floating point, where the product
> is truncated *as it is computed*.

`operator/` and `operator/=` **can throw** `std::domain_error` on a zero
divisor, like the integer's (ADR-004), and are therefore not `noexcept`. In a
constant-evaluated context the `throw` turns the expression into a compile
error, just like `1 / 0` on an `int`.

### The rounding modes

The exact result is always `q + r/d`, with `q` integral, `0 ≤ r < d` and
`d > 0`. `q` is the **floor** and `r` is never negative, which is what makes
one formula cover negatives without a special case.

| Mode | `2r < d` | `2r > d` | tie, `2r == d` |
|---|---|---|---|
| `to_nearest_even` **(default)** | `q` | `q+1` | `q` if even, `q+1` if odd |
| `to_nearest_away` | `q` | `q+1` | `q+1` if `q ≥ 0`, else `q` |
| `toward_zero` | — | — | `q+1` iff `q < 0` and `r ≠ 0` |
| `toward_neg_inf` | — | — | always `q` |
| `toward_pos_inf` | — | — | `q+1` iff `r ≠ 0` |

`to_nearest_even` is the default, chosen for **bias** rather than cost: what
people do with fixed point is accumulate, and a biased rounding drifts further
the longer the chain. `toward_neg_inf` is free — it is the arithmetic shift as
it stands. `toward_zero` is what the integer's `operator/` already does.

```cpp
using Preciso = sfixed_point_t<2, 1>;                       // to_nearest_even
using Barato  = sfixed_point_t<2, 1, overflow_policy::wrap,
                               rounding_mode::toward_zero>;
```

### Where the two policies meet

**Rounding up from `max()` overflows.** That case is decided by
`overflow_policy`, not by `Redondeo` — which is exactly why they are separate
parameters.

---

## `operator%` is exact — it does **not** round

```cpp
constexpr fixed_point_t  operator% (const fixed_point_t &) const;   // may throw
constexpr fixed_point_t &operator%=(const fixed_point_t &);         // may throw
```

`%` follows `std::fmod`: the remainder of truncating the quotient toward zero,
carrying the sign of the dividend.

**The remainder is always exactly representable**, so there is nothing to
round: `a` and `b` are multiples of `epsilon`, the truncated quotient is
integral, hence `q·b` is a multiple of `epsilon` and so is `r = a − q·b`. It
turns out to be literally the integer `%` of the raws.

> **`a == (a/b)*b + a%b` does not hold when `/` rounds**, because then `a/b` is
> no longer the truncated quotient — in fixed point `−12345/7` is `−1763.571…`,
> not `−1763`. The identity that does hold goes through the quotient truncated
> **to an integer**:
>
> ```cpp
> const auto r = a % b;
> const auto k = (a - r) / b;     // exact, and k.es_entero()
> assert(k * b + r == a);         // k is integral, so k*b is exact too
> ```

---

## Shifts scale the value

```cpp
constexpr fixed_point_t  operator<< (unsigned n) const noexcept;   // exact
constexpr fixed_point_t  operator>> (unsigned n) const noexcept;   // rounds
constexpr fixed_point_t &operator<<=(unsigned n)       noexcept;
constexpr fixed_point_t &operator>>=(unsigned n)       noexcept;
```

`x << n` is `x · 2^n` and is **exact** (overflow aside). `x >> n` is `x / 2^n`
and **rounds**: shifting down by `n` drops `n` bits, and dropping them is
exactly what the knob decides. With `toward_neg_inf` it reduces to the plain
arithmetic shift, which is free. This is what the TR 18037 fixed-point types do.

Shifting by more than the width of the type is defined, not undefined: the whole
value falls off, and whatever is left decides the rounding as anywhere else.

### There is no `&`, `|`, `^` or `~`

The integer has them, and by ADR-018 they operate on the value there. On a fixed
point they would not mean anything useful, and neither `float` nor the TR 18037
types offer them. Adding them would be inventing semantics instead of following
the standard. The parity matrix watches that they **keep failing to compile**.

---

## The two halves

```cpp
constexpr entero suelo() const noexcept;
constexpr fixed_int_t<N, unsigned_type, binnat, Policy>
          parte_fraccionaria() const noexcept;
constexpr bool es_entero() const noexcept;
```

`suelo()` is the integer part **truncated toward −∞**, not toward zero: `-1.5`
gives `-2`. It is an arithmetic shift, and it is the same asymmetry `operator>>`
has against `operator/` in the integer. This is documented rather than hidden.

`parte_fraccionaria()` is always **non-negative**: it is the `r` of
`value = suelo() + r/2^(64·F)` with `0 ≤ r < 2^(64·F)`, which is what makes the
rounding formula work unchanged for negative values.

---

## Queries

```cpp
constexpr bool is_zero()     const noexcept;
constexpr bool is_negative() const noexcept;   // always false when unsigned
constexpr bool valid()       const noexcept;
```

`valid()` reports whether the value carries an overflow mark. With
`overflow_policy::checked` the mark is set and propagated by the underlying
integer and merely re-exposed here; with the other policies it is always
`true`. **The mark is sticky**: returning to range does not clear it, because a
marked value holds the already-wrapped result inside (P1.5 tranche 2f).

---

## Text

```cpp
std::string to_string(unsigned decimales = 6) const;
```

**It rounds**, according to `Redondeo`.

**The value is rounded, not the magnitude.** That is the only way the directed
modes come out right: `toward_pos_inf` on `-2.55` with one digit gives `-2.5`,
not `-2.6`. The digits are still written from the magnitude — that keeps the
decimal point from having to know about signs — so two mirrors apply, and
neither is obvious:

- for a negative value, **raising the value means not raising the magnitude**;
- the parity the even-tie uses is that of `q = -(Dmag+1)`, i.e. **the opposite**
  of the last digit written.

The carry **can lengthen the string**: `9.999…` with two digits is `10.00`. The
decimal point is therefore inserted last, counted from the right.

| Case | Result | Why |
|---|---|---|
| `max().to_string(2)` | `…808.00` | the value is `…807.99999999999999999995`; at two digits it rounds up and the carry reaches the integer part |
| `to_string(3)` with `F == 0` | `7.000` | `decimales` means what it says, like `printf("%.3f", 7.0)` |
| a negative that rounds to zero | `-0.00` | same as `printf`. The type has no negative zero; the string says "a small negative number", which is information |

With `decimales == 0` no decimal point is written.

The minimum prints with **one** sign, not two: negating the minimum wraps, so
`to_string` detects that case instead of prefixing a second `-`. Nothing is
rounded there — the minimum has no fractional part.

---

## Not here yet

Deliberately absent, and tracked:

| | Why |
|---|---|
| Operations between different `F` | Promote to the wider one, forbid, or require an explicit conversion? Affects ergonomics more than correctness. Open in ADR-019. |
| Conversion to and from floating point | Has its own rounding and its own odd cases (infinities, NaN); the integer type already had a bug there (T2.2). |
| `to_chars` / `from_chars` | Missing for both families, not only this type. |
| Reading more decimal digits than the integer part can hold | `operator>>` uses only the digits whose `10^k` fits in the integer part; with `F == N` that is none. Documented on the operator. |

---

## See also

- [API_fixed_int.md](API_fixed_int.md) — the integer underneath
- [API_representation.md](API_representation.md) — the four representations
- [MATRIZ_DE_PARIDAD.md](MATRIZ_DE_PARIDAD.md) — which capability exists in which cell
- [ADR-019](decisions/ADR-019-punto-fijo-es-un-entero-con-escala.md) — a fixed point is an integer with a scale
- [ADR-020](decisions/ADR-020-los-operadores-multiplicativos-del-punto-fijo.md) — the multiplicative operators
