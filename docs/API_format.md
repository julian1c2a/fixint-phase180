# API Reference — int128_param_format.hpp

> `std::format` / `std::formatter` specialization for 128-bit integers (C++20).

## Synopsis

```cpp
#include "int128_param_format.hpp"

namespace std {

template <nstd::signedness S, nstd::representation_form F>
struct formatter<nstd::int128_param_t<S, F>> {
    constexpr auto parse(std::format_parse_context& ctx);
    auto format(const nstd::int128_param_t<S, F>& val, std::format_context& ctx) const;
};

}
```

---

## Format Specification

Full C++20 standard format spec supported:

```
{:[fill][align][sign][#][0][width][type]}
```

### Type Specifiers

| Specifier | Output | Example |
|:---------:|--------|---------|
| `d` (default) | Decimal | `"12345"` |
| `x` | Lowercase hex | `"3039"` |
| `X` | Uppercase hex | `"3039"` |
| `b` | Binary | `"11000000111001"` |
| `B` | Binary (uppercase prefix) | `"11000000111001"` |
| `o` | Octal | `"30071"` |

### Alignment

| Char | Meaning | Example |
|:----:|---------|---------|
| `<` | Left-align | `"42    "` |
| `>` | Right-align (default) | `"    42"` |
| `^` | Center | `"  42  "` |

### Sign

| Char | Meaning | Example |
|:----:|---------|---------|
| `+` | Always show sign | `"+42"` |
| `-` | Negative only (default) | `"42"` |
| ` ` | Space for positive | `" 42"` |

### Other Options

| Option | Meaning | Example |
|:------:|---------|---------|
| `#` | Alternate form (prefix) | `"0x3039"`, `"0b1010"`, `"030071"` |
| `0` | Zero-pad | `"00042"` |
| *width* | Minimum field width | `"{:20d}"` → 20-char field |
| *fill* | Custom fill character | `"{:*>10}"` → `"******42"` |

---

## Examples

```cpp
#include "int128_param_format.hpp"
using namespace nstd;

const uint128_t x{12345};

// Basic types
const auto s1 = std::format("{}", x);     // "12345"
const auto s2 = std::format("{:x}", x);   // "3039"
const auto s3 = std::format("{:X}", x);   // "3039"
const auto s4 = std::format("{:b}", x);   // "11000000111001"
const auto s5 = std::format("{:o}", x);   // "30071"

// Width and alignment
const auto s6 = std::format("{:>20}", x);    // "               12345"
const auto s7 = std::format("{:<20}", x);    // "12345               "
const auto s8 = std::format("{:^20}", x);    // "       12345        "

// Fill and zero-pad
const auto s9 = std::format("{:0>20}", x);   // "00000000000000012345"
const auto s10 = std::format("{:*>20}", x);  // "***************12345"
const auto s11 = std::format("{:020}", x);   // "00000000000000012345"

// Sign
const auto s12 = std::format("{:+}", x);        // "+12345"
const auto s13 = std::format("{: }", x);         // " 12345"

// Alternate form (prefix)
const auto s14 = std::format("{:#x}", x);    // "0x3039"
const auto s15 = std::format("{:#b}", x);    // "0b11000000111001"
const auto s16 = std::format("{:#o}", x);    // "030071"

// Combined
const auto s17 = std::format("{:+#020x}", x);  // "+0x0000000000003039"
```

---

## Compiler Support

Requires `<format>` header (C++20). Supported by:

- GCC 13+
- Clang 17+ (with libc++)
- MSVC 19.29+
