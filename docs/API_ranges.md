# API Reference — int128_param_ranges.hpp

> Range generators, statistics, search, transformation and reduction utilities.

## Synopsis

```cpp
#include "int128_param_ranges.hpp"

namespace nstd::int128_ranges {

// Generators
template <Sign, Form, typename OutputIt>
constexpr void generate_arithmetic_sequence(OutputIt, OutputIt, const T&, const T&);
template <Sign, Form, typename OutputIt>
constexpr void iota(OutputIt, OutputIt, const T&);
template <Sign, Form, typename OutputIt>
constexpr void generate_geometric_sequence(OutputIt, OutputIt, const T&, const T&);
template <Sign, Form, typename OutputIt>
constexpr void generate_powers_of_2(OutputIt, OutputIt, int = 0);

// Statistics
template <Sign, Form> struct range_stats;
template <Sign, Form, typename InputIt>
constexpr range_stats<Sign, Form> calculate_stats(InputIt, InputIt);

// Search
template <Sign, Form, typename InputIt, typename Pred>
constexpr std::optional<T> find_first_if(InputIt, InputIt, Pred);
template <Sign, Form, typename InputIt, typename Pred>
constexpr std::size_t count_if(InputIt, InputIt, Pred);

// Transform
template <Sign, Form, typename InputIt, typename OutputIt, typename UnaryOp>
constexpr OutputIt transform(InputIt, InputIt, OutputIt, UnaryOp);
template <Sign, Form, typename InputIt, typename OutputIt, typename Pred>
constexpr OutputIt copy_if(InputIt, InputIt, OutputIt, Pred);

// Reduce
template <Sign, Form, typename InputIt, typename BinaryOp>
constexpr T reduce(InputIt, InputIt, T init, BinaryOp);
template <Sign, Form, typename InputIt>
constexpr T sum(InputIt, InputIt);
template <Sign, Form, typename InputIt>
constexpr T product(InputIt, InputIt);

}
```

---

## Generators

### `generate_arithmetic_sequence`

```cpp
template <signedness Sign, representation_form Form, typename OutputIt>
constexpr void generate_arithmetic_sequence(
    OutputIt first, OutputIt last,
    const int128_param_t<Sign, Form>& start,
    const int128_param_t<Sign, Form>& step);
```

Fills `[first, last)` with `start, start+step, start+2*step, ...`

### `iota`

```cpp
template <signedness Sign, representation_form Form, typename OutputIt>
constexpr void iota(
    OutputIt first, OutputIt last,
    const int128_param_t<Sign, Form>& start);
```

Fills with `start, start+1, start+2, ...`

### `generate_geometric_sequence`

```cpp
template <signedness Sign, representation_form Form, typename OutputIt>
constexpr void generate_geometric_sequence(
    OutputIt first, OutputIt last,
    const int128_param_t<Sign, Form>& start,
    const int128_param_t<Sign, Form>& ratio);
```

Fills with `start, start*ratio, start*ratio^2, ...`

### `generate_powers_of_2`

```cpp
template <signedness Sign, representation_form Form, typename OutputIt>
constexpr void generate_powers_of_2(OutputIt first, OutputIt last, int start_exponent = 0);
```

Fills with `2^start_exponent, 2^(start_exponent+1), ...`

---

## Statistics

### `range_stats<Sign, Form>`

```cpp
template <signedness Sign, representation_form Form>
struct range_stats {
    int128_param_t<Sign, Form> sum;
    int128_param_t<Sign, Form> min_val;
    int128_param_t<Sign, Form> max_val;
    std::size_t count;
    bool valid;

    int128_param_t<Sign, Form> average() const;
    int128_param_t<Sign, Form> range() const;
};
```

### `calculate_stats`

```cpp
template <signedness Sign, representation_form Form, typename InputIt>
constexpr range_stats<Sign, Form> calculate_stats(InputIt first, InputIt last);
```

Computes sum, min, max, count in a single pass.

---

## Search

### `find_first_if`

```cpp
template <signedness Sign, representation_form Form, typename InputIt, typename Pred>
constexpr std::optional<int128_param_t<Sign, Form>> find_first_if(
    InputIt first, InputIt last, Pred pred);
```

Returns the first element satisfying `pred`, or `std::nullopt`.

### `count_if`

```cpp
template <signedness Sign, representation_form Form, typename InputIt, typename Pred>
constexpr std::size_t count_if(InputIt first, InputIt last, Pred pred);
```

---

## Transform / Copy

### `transform`

```cpp
template <signedness Sign, representation_form Form, typename InputIt, typename OutputIt, typename UnaryOp>
constexpr OutputIt transform(InputIt first, InputIt last, OutputIt d_first, UnaryOp op);
```

### `copy_if`

```cpp
template <signedness Sign, representation_form Form, typename InputIt, typename OutputIt, typename Pred>
constexpr OutputIt copy_if(InputIt first, InputIt last, OutputIt d_first, Pred pred);
```

---

## Reductions

### `reduce`

```cpp
template <signedness Sign, representation_form Form, typename InputIt, typename BinaryOp>
constexpr int128_param_t<Sign, Form> reduce(
    InputIt first, InputIt last,
    int128_param_t<Sign, Form> init, BinaryOp op);
```

### `sum`

```cpp
template <signedness Sign, representation_form Form, typename InputIt>
constexpr int128_param_t<Sign, Form> sum(InputIt first, InputIt last);
```

Returns sum of all elements (init = 0).

### `product`

```cpp
template <signedness Sign, representation_form Form, typename InputIt>
constexpr int128_param_t<Sign, Form> product(InputIt first, InputIt last);
```

Returns product of all elements (init = 1).

---

## Example

```cpp
#include "int128_param_ranges.hpp"
using namespace nstd;
using namespace nstd::int128_ranges;

std::array<uint128_t, 10> seq;
iota<signedness::unsigned_type, representation_form::binnat>(
    seq.begin(), seq.end(), uint128_t{1});
// seq = {1, 2, 3, ..., 10}

const auto stats = calculate_stats<signedness::unsigned_type, representation_form::binnat>(
    seq.begin(), seq.end());
// stats.sum == 55, stats.min_val == 1, stats.max_val == 10

const auto total = sum<signedness::unsigned_type, representation_form::binnat>(
    seq.begin(), seq.end());
// total == 55
```
