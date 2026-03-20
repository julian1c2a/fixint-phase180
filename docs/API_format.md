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

## Supported Format Specifiers

| Specifier | Output | Example |
|:---------:|--------|---------|
| `d` (default) | Decimal | `"12345"` |
| `x` | Lowercase hex | `"0x3039"` |
| `X` | Uppercase hex | `"0X3039"` |
| `b` | Binary | `"0b11000000111001"` |
| `o` | Octal | `"030071"` |

---

## Example

```cpp
#include "int128_param_format.hpp"
using namespace nstd;

const uint128_t x{12345};

const auto s1 = std::format("{}", x);     // "12345"
const auto s2 = std::format("{:x}", x);   // "3039"
const auto s3 = std::format("{:X}", x);   // "3039"
const auto s4 = std::format("{:b}", x);   // "11000000111001"
const auto s5 = std::format("{:o}", x);   // "30071"

// Width and fill
const auto s6 = std::format("{:>20}", x);   // "               12345"
const auto s7 = std::format("{:0>20}", x);  // "00000000000000012345"
```

---

## Compiler Support

Requires `<format>` header (C++20). Supported by:

- GCC 13+
- Clang 17+ (with libc++)
- MSVC 19.29+
