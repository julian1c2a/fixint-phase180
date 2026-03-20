# API Reference — int128_param_iostreams.hpp

> Stream I/O operators and string formatting utilities for 128-bit integers.

## Synopsis

```cpp
#include "int128_param_iostreams.hpp"

namespace nstd {

// Stream operators
template <signedness S, representation_form F>
std::ostream& operator<<(std::ostream& os, const int128_param_t<S, F>& val);

template <signedness S, representation_form F>
std::istream& operator>>(std::istream& is, int128_param_t<S, F>& val);

}

namespace nstd::iostreams {

// Formatting utilities
template <S, F> std::string format(const T&, int base=10, int width=0, char fill=' ',
    bool uppercase=false, bool show_base=false, bool show_pos=false, bool left_align=false);
template <S, F> std::string hex(const T&, bool prefix=true, bool uppercase=true);
template <S, F> std::string oct(const T&, bool prefix=true);
template <S, F> std::string dec(const T&, bool show_pos=false);
template <S, F> std::string bin(const T&, bool prefix=true);

}
```

---

## Stream Operators

### `operator<<`

```cpp
template <signedness S, representation_form F>
std::ostream& operator<<(std::ostream& os, const int128_param_t<S, F>& val);
```

Outputs `val` to the stream. Respects `std::hex`, `std::oct`, `std::dec` manipulators and width/fill settings.

### `operator>>`

```cpp
template <signedness S, representation_form F>
std::istream& operator>>(std::istream& is, int128_param_t<S, F>& val);
```

Reads an integer from the stream. Supports decimal, hex (`0x`), octal (`0`), binary (`0b`) prefixes.

---

## Formatting Utilities (`nstd::iostreams`)

### `format`

```cpp
template <signedness S, representation_form F>
std::string format(
    const int128_param_t<S, F>& val,
    int base = 10,
    int width = 0,
    char fill = ' ',
    bool uppercase = false,
    bool show_base = false,
    bool show_pos = false,
    bool left_align = false);
```

General-purpose formatting with full control.

### `hex` / `oct` / `dec` / `bin`

```cpp
template <signedness S, representation_form F>
std::string hex(const int128_param_t<S, F>& val, bool prefix = true, bool uppercase = true);
// "0xDEADBEEF" (with prefix) or "DEADBEEF"

template <signedness S, representation_form F>
std::string oct(const int128_param_t<S, F>& val, bool prefix = true);
// "0777" or "777"

template <signedness S, representation_form F>
std::string dec(const int128_param_t<S, F>& val, bool show_pos = false);
// "42" or "+42"

template <signedness S, representation_form F>
std::string bin(const int128_param_t<S, F>& val, bool prefix = true);
// "0b101010" or "101010"
```

---

## Example

```cpp
#include "int128_param_iostreams.hpp"
using namespace nstd;

const uint128_t x{0xDEAD, 0xBEEF};

// Stream output
std::cout << x << std::endl;                // decimal
std::cout << std::hex << x << std::endl;    // hex

// Formatting utilities
const auto h = nstd::iostreams::hex(x);     // "0xDEAD00000000BEEF"
const auto b = nstd::iostreams::bin(x);     // "0b..."
const auto d = nstd::iostreams::dec(x);     // "..."

// Stream input
uint128_t y;
std::istringstream iss{"12345678901234567890"};
iss >> y;
```
