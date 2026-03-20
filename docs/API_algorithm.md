# API Reference — int128_param_algorithm.hpp

> STL-style algorithm overloads for containers of 128-bit integers.

## Synopsis

```cpp
#include "int128_param_algorithm.hpp"

namespace nstd {

// Filling
template <S, F, typename ForwardIt>
constexpr void fill(ForwardIt first, ForwardIt last, const int128_param_t<S, F>& value) noexcept;

template <S, F, typename OutputIt, typename Size>
constexpr OutputIt fill_n(OutputIt first, Size count, const int128_param_t<S, F>& value) noexcept;

// Reordering
template <S, F, typename BidirIt>
constexpr void reverse(BidirIt first, BidirIt last) noexcept;

// Searching
template <S, F, typename InputIt>
constexpr InputIt find(InputIt first, InputIt last, const int128_param_t<S, F>& value) noexcept;

template <S, F, typename InputIt>
constexpr auto count(InputIt first, InputIt last, const int128_param_t<S, F>& value) noexcept;

// Predicates
template <S, F, typename InputIt, typename UnaryPredicate>
constexpr bool all_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept;

template <S, F, typename InputIt, typename UnaryPredicate>
constexpr bool any_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept;

template <S, F, typename InputIt, typename UnaryPredicate>
constexpr bool none_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept;

// Min/Max
template <S, F, typename ForwardIt>
constexpr ForwardIt min_element(ForwardIt first, ForwardIt last) noexcept;

template <S, F, typename ForwardIt>
constexpr ForwardIt max_element(ForwardIt first, ForwardIt last) noexcept;

// Reduction
template <S, F, typename InputIt>
constexpr int128_param_t<S, F> accumulate(InputIt first, InputIt last, int128_param_t<S, F> init) noexcept;

}
```

---

## Functions

### `fill`

```cpp
template <signedness S, representation_form F, typename ForwardIt>
constexpr void fill(ForwardIt first, ForwardIt last, const int128_param_t<S, F>& value) noexcept;
```

Assigns `value` to every element in `[first, last)`.

### `fill_n`

```cpp
template <signedness S, representation_form F, typename OutputIt, typename Size>
constexpr OutputIt fill_n(OutputIt first, Size count, const int128_param_t<S, F>& value) noexcept;
```

Assigns `value` to the first `count` elements. Returns iterator past the last assigned.

### `reverse`

```cpp
template <signedness S, representation_form F, typename BidirIt>
constexpr void reverse(BidirIt first, BidirIt last) noexcept;
```

Reverses elements in `[first, last)`.

### `find`

```cpp
template <signedness S, representation_form F, typename InputIt>
constexpr InputIt find(InputIt first, InputIt last, const int128_param_t<S, F>& value) noexcept;
```

Returns iterator to first element equal to `value`, or `last` if not found.

### `count`

```cpp
template <signedness S, representation_form F, typename InputIt>
constexpr auto count(InputIt first, InputIt last, const int128_param_t<S, F>& value) noexcept;
```

Returns the number of elements equal to `value`.

### `all_of` / `any_of` / `none_of`

```cpp
template <signedness S, representation_form F, typename InputIt, typename UnaryPredicate>
constexpr bool all_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept;
constexpr bool any_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept;
constexpr bool none_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept;
```

Predicate-based range checks.

### `min_element` / `max_element`

```cpp
template <signedness S, representation_form F, typename ForwardIt>
constexpr ForwardIt min_element(ForwardIt first, ForwardIt last) noexcept;
constexpr ForwardIt max_element(ForwardIt first, ForwardIt last) noexcept;
```

Returns iterator to the smallest/largest element.

### `accumulate`

```cpp
template <signedness S, representation_form F, typename InputIt>
constexpr int128_param_t<S, F> accumulate(InputIt first, InputIt last, int128_param_t<S, F> init) noexcept;
```

Sums all elements in `[first, last)`, starting from `init`.

---

## Example

```cpp
#include "int128_param_algorithm.hpp"
using namespace nstd;

std::array<uint128_t, 4> arr;
nstd::fill(arr.begin(), arr.end(), uint128_t{42});

const auto total = nstd::accumulate(arr.begin(), arr.end(), uint128_t{0});
// total == 168

const auto it = nstd::find(arr.begin(), arr.end(), uint128_t{42});
// it == arr.begin()

const bool all_42 = nstd::all_of<signedness::unsigned_type, representation_form::binnat>(
    arr.begin(), arr.end(), [](const uint128_t& v) { return v == uint128_t{42}; });
// all_42 == true
```
