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

```cpp
std::array<std::uint64_t, N> data;   // data[0] = LSB, data[N-1] = MSB (little-endian limbs)
```

`data` is public for advanced use (custom serialization, intrinsics dispatch).

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

## Related Headers

| Header | Provides |
|--------|----------|
| `fixed_int_traits_specializations.hpp` | `nstd::is_integral/is_signed/is_unsigned`, `nstd::make_signed/unsigned`, `std::common_type` |
| `fixed_int_concepts.hpp` | `nstd::integral/signed_integral/unsigned_integral` (aglutinating built-ins + `fixed_int_t`); detection concepts |
| `fixed_int_limits.hpp` | `std::numeric_limits<fixed_int_t<N, Sign, Form>>` for all N, Sign, Form |

See [API_fixed_int_traits.md](API_fixed_int_traits.md) for the complete reference.

---

## Version Notes

- **v1.90** — `fixed_int_t<N, Sign, Form>` unified template, cross-sign operators (`+, -, *, /, %, &, |, ^`), cross-sign comparators (`==, !=, <, <=, >, >=`), cross-sign compounds (`+=, -=, *=, /=, %=, &=, |=, ^=`), `detail::mixed_iu_t`, Knuth Algorithm D divmod, Karatsuba multiplication for N=4/8.
- **v1.81 — Fase MS-INTEROP**: unary `operator+()`, shifts with cross-sign count, `operator<=>` (member + cross-type free), `mixed_iu_t` promoted to public `nstd::`, detection traits, `nstd::is_integral`/`is_arithmetic`/`is_signed`/`is_unsigned`, `nstd::make_signed`/`make_unsigned`, `nstd::integral`/`signed_integral`/`unsigned_integral` concepts, `std::common_type`, `std::numeric_limits`.
