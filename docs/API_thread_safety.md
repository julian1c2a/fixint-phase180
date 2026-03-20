# API Reference — int128_param_thread_safety.hpp

> Lock-based atomic wrapper for 128-bit integers with full `std::atomic`-like interface.

## Synopsis

```cpp
#include "int128_param_thread_safety.hpp"

namespace nstd {

template <signedness Sign, representation_form Form>
class atomic_int128_param_t;

// Type aliases
using atomic_uint128_t   = atomic_int128_param_t<unsigned_type, binnat>;
using atomic_int128_tc_t = atomic_int128_param_t<signed_type, twos_complement>;
using atomic_int128_ms_t = atomic_int128_param_t<signed_type, magnitude_sign>;
using atomic_int128_ek_t = atomic_int128_param_t<signed_type, excess_k>;
using atomic_int128_t    = atomic_int128_tc_t;

// Free functions
template <S, F> T atomic_load(const atomic_int128_param_t<S, F>*) noexcept;
template <S, F> void atomic_store(atomic_int128_param_t<S, F>*, const T&) noexcept;
template <S, F> T atomic_exchange(atomic_int128_param_t<S, F>*, const T&) noexcept;

}
```

---

## Class `atomic_int128_param_t<Sign, Form>`

### Member Types

| Type | Definition |
|------|-----------|
| `value_type` | `int128_param_t<Sign, Form>` |

### Constructors

```cpp
constexpr atomic_int128_param_t() noexcept;                       // zero-init
constexpr atomic_int128_param_t(const value_type& val) noexcept;
constexpr atomic_int128_param_t(std::uint64_t hi, std::uint64_t lo) noexcept;
```

Non-copyable, non-movable.

---

### Load / Store / Exchange

```cpp
value_type load(std::memory_order order = std::memory_order_seq_cst) const noexcept;
void store(const value_type& desired, std::memory_order order = std::memory_order_seq_cst) noexcept;
value_type exchange(const value_type& desired, std::memory_order order = std::memory_order_seq_cst) noexcept;
```

### Compare-and-Swap

```cpp
bool compare_exchange_strong(
    value_type& expected, const value_type& desired,
    std::memory_order order = std::memory_order_seq_cst) noexcept;

bool compare_exchange_weak(
    value_type& expected, const value_type& desired,
    std::memory_order order = std::memory_order_seq_cst) noexcept;
```

### Fetch Operations

```cpp
value_type fetch_add(const value_type& arg, std::memory_order order = std::memory_order_seq_cst) noexcept;
value_type fetch_sub(const value_type& arg, std::memory_order order = std::memory_order_seq_cst) noexcept;
value_type fetch_and(const value_type& arg, std::memory_order order = std::memory_order_seq_cst) noexcept;
value_type fetch_or(const value_type& arg, std::memory_order order = std::memory_order_seq_cst) noexcept;
value_type fetch_xor(const value_type& arg, std::memory_order order = std::memory_order_seq_cst) noexcept;
```

### Arithmetic Operators

```cpp
value_type operator++() noexcept;      // pre-increment
value_type operator++(int) noexcept;   // post-increment
value_type operator--() noexcept;      // pre-decrement
value_type operator--(int) noexcept;   // post-decrement

value_type operator+=(const value_type&) noexcept;
value_type operator-=(const value_type&) noexcept;
value_type operator&=(const value_type&) noexcept;
value_type operator|=(const value_type&) noexcept;
value_type operator^=(const value_type&) noexcept;
```

### Lock-Free Query

```cpp
static constexpr bool is_lock_free() noexcept;        // false (uses mutex)
static constexpr bool is_always_lock_free() noexcept;  // false
```

---

## Free Functions

```cpp
template <signedness Sign, representation_form Form>
inline int128_param_t<Sign, Form> atomic_load(
    const atomic_int128_param_t<Sign, Form>* obj) noexcept;

template <signedness Sign, representation_form Form>
inline void atomic_store(
    atomic_int128_param_t<Sign, Form>* obj,
    const int128_param_t<Sign, Form>& desired) noexcept;

template <signedness Sign, representation_form Form>
inline int128_param_t<Sign, Form> atomic_exchange(
    atomic_int128_param_t<Sign, Form>* obj,
    const int128_param_t<Sign, Form>& desired) noexcept;
```

---

## Build Requirements

Thread safety features require additional linker flags:

```bash
-pthread -latomic
```

The build system (make.py) auto-detects and adds these flags.

---

## Example

```cpp
#include "int128_param_thread_safety.hpp"
using namespace nstd;

atomic_uint128_t counter{uint128_t{0}};

// Thread 1
counter.fetch_add(uint128_t{1});

// Thread 2
const auto val = counter.load();

// CAS loop
auto expected = counter.load();
while (!counter.compare_exchange_weak(expected, expected + uint128_t{10})) {
    // retry
}
```
