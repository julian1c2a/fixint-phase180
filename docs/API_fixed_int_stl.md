# API Reference — integración de `fixed_int_t` con la biblioteca estándar

> `fixed_int_iostreams.hpp`, `fixed_int_format.hpp`, `fixed_int_hash.hpp` — v1.90.1
>
> Los tres headers son independientes: se incluye solo el que se necesite.
> `int128_param_t` ya tenía los equivalentes (`int128_param_iostreams.hpp`,
> `int128_param_format.hpp`); estos son la paridad para el tipo nuevo.

---

## Synopsis

```cpp
#include "fixed_int_iostreams.hpp"   // operator<<, operator>>
#include "fixed_int_format.hpp"      // std::formatter
#include "fixed_int_hash.hpp"        // std::hash

namespace nstd {
    template <std::size_t N, signedness Sign, representation_form Form>
    std::ostream& operator<<(std::ostream&, const fixed_int_t<N, Sign, Form>&);

    template <std::size_t N, signedness Sign, representation_form Form>
    std::istream& operator>>(std::istream&, fixed_int_t<N, Sign, Form>&);
}

// std::formatter<nstd::fixed_int_t<N, Sign, Form>, CharT>
// std::hash<nstd::fixed_int_t<N, Sign, Form>>
```

---

## `fixed_int_iostreams.hpp`

### Salida — `operator<<`

Respeta los manipuladores del flujo igual que un entero built-in:

| Manipulador | Efecto |
|---|---|
| `std::dec` / `std::hex` / `std::oct` | base 10 / 16 / 8 |
| `std::showbase` | prefijo `0x` en hexadecimal, `0` en octal |
| `std::uppercase` | dígitos hexadecimales en mayúsculas (por defecto, minúsculas) |
| `std::showpos` | `+` delante de los positivos (nunca delante del cero) |
| `std::setw` / `std::setfill` | ancho mínimo y carácter de relleno |
| `std::left` / `std::right` / `std::internal` | alineación; `internal` mete el relleno detrás del signo y del prefijo |

El ancho se consume tras un solo uso, como manda la biblioteca estándar.

```cpp
using nstd::uint256_fixed_t;
std::cout << uint256_fixed_t{255};                              // 255
std::cout << std::hex << uint256_fixed_t{255};                  // ff
std::cout << std::hex << std::showbase << uint256_fixed_t{255}; // 0xff
std::cout << std::setw(6) << std::setfill('*') << uint256_fixed_t{42};  // ****42
std::cout << std::hex << std::showbase << std::internal
          << std::setfill('0') << std::setw(8) << uint256_fixed_t{255}; // 0x0000ff
```

**Con signo y base distinta de 10 se imprime el patrón de bits**, no el signo y
la magnitud — igual que `std::cout << std::hex << -1` con un `int`:

```cpp
std::cout << std::hex << nstd::int256_fixed_t{-1};   // ffff...ff (64 dígitos)
```

Es la diferencia deliberada con `std::format`, que en ese caso imprime `-1`.

### Entrada — `operator>>`

Lee signo, prefijo y dígitos, saltando los espacios iniciales si el flujo tiene
`skipws`. La base sale del `basefield` del flujo; **si no hay `basefield`, se
deduce del prefijo**, como con los enteros built-in.

Activa `failbit` sin tocar el destino cuando la entrada es inválida, está vacía
o **desborda** el tipo. Esto último es posible porque internamente usa
`try_from_string`, que devuelve el error en vez de lanzarlo.

```cpp
std::istringstream is{"0x10"};
nstd::uint256_fixed_t v{};
is.unsetf(std::ios_base::basefield);
is >> v;                                    // v == 16, base deducida del prefijo
```

Los tipos sin signo aceptan `-` y aplican el valor en módulo 2^(64N), igual que
hacen los flujos con `unsigned int`.

---

## `fixed_int_format.hpp`

Especificación completa, la misma que los enteros built-in:

```
[[fill]align][sign][#][0][width][type]
```

| Campo | Valores | Por defecto |
|---|---|---|
| `fill` | cualquier carácter | espacio |
| `align` | `<` izquierda, `>` derecha, `^` centrado | `>` (números) |
| `sign` | `+` siempre, `-` solo negativos, ` ` espacio si positivo | `-` |
| `#` | prefijo de base: `0x`, `0X`, `0b`, `0B`, `0` | sin prefijo |
| `0` | relleno con ceros entre el signo/prefijo y los dígitos | sin relleno |
| `width` | ancho mínimo del campo | 0 |
| `type` | `d` decimal, `x`/`X` hexadecimal, `b`/`B` binario, `o` octal | `d` |

```cpp
using nstd::uint256_fixed_t;
std::format("{}",       uint256_fixed_t{255});   // "255"
std::format("{:x}",     uint256_fixed_t{255});   // "ff"
std::format("{:#X}",    uint256_fixed_t{255});   // "0XFF"
std::format("{:#b}",    uint256_fixed_t{5});     // "0b101"
std::format("{:>8}",    uint256_fixed_t{42});    // "      42"
std::format("{:*^8}",   uint256_fixed_t{42});    // "***42***"
std::format("{:08}",    uint256_fixed_t{42});    // "00000042"
std::format("{:#010x}", uint256_fixed_t{255});   // "0x000000ff"
std::format("{:+}",     uint256_fixed_t{42});    // "+42"
```

**Con signo y base distinta de 10 se imprime el signo y la magnitud**, no el
patrón de bits — al contrario que iostreams, y otra vez igual que los built-in:

```cpp
std::format("{:x}", nstd::int256_fixed_t{-255});   // "-ff"
```

Un especificador inválido lanza `std::format_error`.

El header se protege con `#if __has_include(<format>)`, así que incluirlo con un
compilador sin `<format>` no rompe la compilación: simplemente no define nada.

---

## `fixed_int_hash.hpp`

```cpp
template <std::size_t N, nstd::signedness Sign, nstd::representation_form Form>
struct std::hash<nstd::fixed_int_t<N, Sign, Form>>;
```

Mezcla **todos** los limbos (finalizador de splitmix64 más el primo de FNV-1a),
no solo el bajo: dos valores que difieran únicamente en el limbo alto producen
hashes distintos.

```cpp
std::unordered_set<nstd::uint256_fixed_t> s;
s.insert(nstd::uint256_fixed_t::max());

std::unordered_map<nstd::uint256_fixed_t, std::string> m;
m[nstd::uint256_fixed_t{42}] = "respuesta";
```

> Especializar `std::hash` para un tipo propio **sí** lo permite el estándar: es
> una de las excepciones explícitas de `[namespace.std]/2`. No ocurre lo mismo
> con `std::is_integral`, que el proyecto resuelve con `nstd::integral` (ver
> [API_fixed_int_traits.md](API_fixed_int_traits.md)).

---

## Tests

`tests/test_fixed_stl_integration.cpp` — 95 asserts sobre los tres headers, más
los round-trips de `to_string(base)` / `from_string(base)` en las 35 bases con
valores frontera. Verificado en GCC 16.2, Clang 22.1 y MSVC 19.5x.

---

## Version Notes

- **v1.90.1** — los tres headers, nuevos. Antes `fixed_int_t` no se podía
  imprimir con `std::cout`, ni usar con `std::format`, ni como clave de un
  contenedor no ordenado.
