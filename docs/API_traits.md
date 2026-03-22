# API Reference — int128_param_traits.hpp + int128_param_traits_specializations.hpp

> Type traits, `std::common_type` specializations, `make_signed`/`make_unsigned`, and `std::hash` for 128-bit integers.

## Synopsis

```cpp
#include "int128_param_traits_specializations.hpp"  // MUST include BEFORE <iostream> etc.
#include "int128_param_traits.hpp"

// In namespace std
template <> struct common_type<nstd::uint128_t, nstd::uint128_t>;
template <> struct common_type<nstd::int128_tc_t, nstd::int128_tc_t>;
// ... all combinations including cross-repr and int128<->integral

// In namespace nstd (or std with LIBCPP)
template <typename T> struct is_integral;
template <typename T> struct is_arithmetic;
template <typename T> struct is_signed;
template <typename T> struct is_unsigned;
template <typename T> struct make_signed;
template <typename T> struct make_unsigned;
template <typename T> struct hash;  // in std
```

---

## `std::common_type` Specializations

### Same-Type

```cpp
template <> struct common_type<uint128_t, uint128_t>   { using type = uint128_t; };
template <> struct common_type<int128_tc_t, int128_tc_t> { using type = int128_tc_t; };
template <> struct common_type<int128_ms_t, int128_ms_t> { using type = int128_ms_t; };
template <> struct common_type<int128_ek_t, int128_ek_t> { using type = int128_ek_t; };
```

### Cross-Representation

Mixed sign/form combinations resolve to a common type (generally `uint128_t` when mixing unsigned with signed).

### With Built-in Integrals

```cpp
template <signedness S, representation_form F, typename T>
struct common_type<int128_param_t<S, F>, T>;  // T is std::integral

template <typename T, signedness S, representation_form F>
struct common_type<T, int128_param_t<S, F>>;  // symmetric
```

Both resolve to the `int128_param_t` type.

### CV-Qualified

All specializations also handle `const`, `volatile`, and `const volatile` overloads.

---

## Type Traits (`nstd::` namespace)

These trait structs specialize over the standard traits to report correct values for 128-bit types. All four int128 instantiations are specialized.

| Trait | True for uint128 | True for int128_tc/ms/ek |
|-------|:-----------------:|:------------------------:|
| `is_integral<T>` | yes | yes |
| `is_arithmetic<T>` | yes | yes |
| `is_unsigned<T>` | yes | no |
| `is_signed<T>` | no | yes |
| `is_trivially_copyable<T>` | yes | yes |
| `is_trivially_constructible<T>` | yes | yes |
| `is_trivially_default_constructible<T>` | yes | yes |
| `is_trivially_copy_constructible<T>` | yes | yes |
| `is_trivially_move_constructible<T>` | yes | yes |
| `is_trivially_copy_assignable<T>` | yes | yes |
| `is_trivially_move_assignable<T>` | yes | yes |
| `is_trivially_destructible<T>` | yes | yes |
| `is_standard_layout<T>` | yes | yes |

### Variable Templates

```cpp
template <typename T> inline constexpr bool is_integral_v;
template <typename T> inline constexpr bool is_arithmetic_v;
template <typename T> inline constexpr bool is_unsigned_v;
template <typename T> inline constexpr bool is_signed_v;
template <typename T> inline constexpr bool is_trivially_copyable_v;
// ... etc. for all traits above
```

---

## Sign Conversion

### `make_signed<T>` / `make_unsigned<T>`

```cpp
template <typename T> struct make_signed   { using type = ...; };
template <typename T> struct make_unsigned  { using type = ...; };

template <typename T> using make_signed_t   = typename make_signed<T>::type;
template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;
```

| Input | `make_signed_t` | `make_unsigned_t` |
|-------|:---------------:|:-----------------:|
| `uint128_t` | `int128_tc_t` | `uint128_t` |
| `int128_tc_t` | `int128_tc_t` | `uint128_t` |
| `int128_ms_t` | `int128_ms_t` | `uint128_t` |
| `int128_ek_t` | `int128_ek_t` | `uint128_t` |

---

## `std::hash`

```cpp
// Both namespaces available:
namespace nstd {
template <> struct hash<uint128_t>    { size_t operator()(const uint128_t&) const noexcept; };
template <> struct hash<int128_tc_t>  { size_t operator()(const int128_tc_t&) const noexcept; };
template <> struct hash<int128_ms_t>  { size_t operator()(const int128_ms_t&) const noexcept; };
template <> struct hash<int128_ek_t>  { size_t operator()(const int128_ek_t&) const noexcept; };
}

namespace std {
template <> struct hash<nstd::uint128_t>    { size_t operator()(const nstd::uint128_t&) const noexcept; };
template <> struct hash<nstd::int128_tc_t>  { size_t operator()(const nstd::int128_tc_t&) const noexcept; };
template <> struct hash<nstd::int128_ms_t>  { size_t operator()(const nstd::int128_ms_t&) const noexcept; };
template <> struct hash<nstd::int128_ek_t>  { size_t operator()(const nstd::int128_ek_t&) const noexcept; };
}
```

**Hash formula:** `std::hash<uint64_t>()(val.high()) ^ (std::hash<uint64_t>()(val.low()) << 1)`

Both `nstd::hash<T>` and `std::hash<T>` produce the same values. The `std::hash` specializations enable direct use in standard containers:

```cpp
// Works on all compilers (GCC, Clang, MSVC, Intel)
std::unordered_map<nstd::uint128_t, std::string> map;
map[nstd::uint128_t{42}] = "hello";

std::unordered_set<nstd::int128_tc_t> set;
set.insert(nstd::int128_tc_t{-1});
```

**Note:** `nstd::hash` is always available regardless of standard library (libstdc++ or libc++).

---

## `is_int128_param` Helper

```cpp
template <typename T> struct is_int128_param : std::false_type {};
template <signedness S, representation_form F>
struct is_int128_param<int128_param_t<S, F>> : std::true_type {};

template <typename T> inline constexpr bool is_int128_param_v = is_int128_param<T>::value;
```

---

## Important: Include Order

```cpp
// CORRECT - traits_specializations BEFORE stdlib headers
#include "int128_param_traits_specializations.hpp"
#include <iostream>
#include <unordered_map>

// WRONG - stdlib first may not see specializations
#include <iostream>
#include "int128_param_traits_specializations.hpp"  // Too late!
```
