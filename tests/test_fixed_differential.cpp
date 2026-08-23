// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: fixed_int_t<N> — fuzz diferencial contra un oraculo independiente
// Part of int128 Library - Phase 1.90
// SPDX-License-Identifier: BSL-1.0
// =============================================================================
//
// T5.2 del plan de auditoria (23 ago 2026).
//
// La auditoria comprobo la aritmetica generando pares aleatorios en C++,
// imprimiendo los resultados y recalculandolos con los enteros grandes de
// Python: 17.600 operaciones, 0 discrepancias. Eso encontro (o descarto) mas
// que ningun test escrito a mano, pero vivia fuera del repositorio.
//
// Aqui queda dentro, con el oraculo escrito en C++ para no depender de Python:
// una implementacion deliberadamente ESTUPIDA de enteros grandes, en base 2^32,
// con algoritmos de libro (schoolbook, division larga bit a bit) que no
// comparten NADA con fixed_int_t: ni intrinsecos, ni Karatsuba, ni Knuth D, ni
// los caminos rapidos por plataforma. Si ambas coinciden en millones de bits,
// el camino optimizado esta bien.
//
// Cubre, para N = 2, 4 y 8, con y sin signo:
//   + - * / % & | ^ << >>   y el round-trip to_string/from_string en base 10.
//
// La semilla es fija: el mismo fallo se reproduce siempre.

#include "fixed_width_int_t.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using nstd::int_fixed_t;
using nstd::uint_fixed_t;

static int g_passed{0};
static int g_failed{0};

#define TEST(name, cond)                              \
    do                                                \
    {                                                 \
        if (cond)                                     \
        {                                             \
            std::cout << "[OK]   " << (name) << "\n"; \
            ++g_passed;                               \
        }                                             \
        else                                          \
        {                                             \
            std::cout << "[FAIL] " << (name) << "\n"; \
            ++g_failed;                               \
        }                                             \
    } while (false)

// =============================================================================
// Oraculo: entero grande sin signo en base 2^32, a proposito ingenuo.
// Representa exactamente 64*N bits (nada de precision arbitraria: queremos
// comparar la aritmetica MODULAR, no la matematica ideal).
// =============================================================================

class big
{
public:
    explicit big(std::size_t words32) : w_(words32, 0) {}

    static big from_fixed_limbs(const std::vector<std::uint64_t> &limbs)
    {
        big r{limbs.size() * 2};
        for (std::size_t i = 0; i < limbs.size(); ++i)
        {
            r.w_[2 * i] = static_cast<std::uint32_t>(limbs[i] & 0xFFFFFFFFU);
            r.w_[2 * i + 1] = static_cast<std::uint32_t>(limbs[i] >> 32);
        }
        return r;
    }

    [[nodiscard]] std::vector<std::uint64_t> to_fixed_limbs() const
    {
        std::vector<std::uint64_t> out(w_.size() / 2, 0);
        for (std::size_t i = 0; i < out.size(); ++i)
            out[i] =
                static_cast<std::uint64_t>(w_[2 * i]) | (static_cast<std::uint64_t>(w_[2 * i + 1]) << 32);
        return out;
    }

    [[nodiscard]] std::size_t words() const { return w_.size(); }
    [[nodiscard]] std::uint32_t word(std::size_t i) const { return w_[i]; }
    void set_word(std::size_t i, std::uint32_t v) { w_[i] = v; }

    [[nodiscard]] bool is_zero() const
    {
        for (const std::uint32_t x : w_)
            if (x != 0)
                return false;
        return true;
    }

    [[nodiscard]] bool bit(std::size_t k) const { return ((w_[k / 32] >> (k % 32)) & 1U) != 0; }
    void set_bit(std::size_t k) { w_[k / 32] |= (std::uint32_t{1} << (k % 32)); }

    [[nodiscard]] big add(const big &o) const
    {
        big r{w_.size()};
        std::uint64_t carry = 0;
        for (std::size_t i = 0; i < w_.size(); ++i)
        {
            const std::uint64_t s = static_cast<std::uint64_t>(w_[i]) + o.w_[i] + carry;
            r.w_[i] = static_cast<std::uint32_t>(s & 0xFFFFFFFFU);
            carry = s >> 32;
        }
        return r; // modulo 2^(32*words)
    }

    [[nodiscard]] big sub(const big &o) const
    {
        big r{w_.size()};
        std::int64_t borrow = 0;
        for (std::size_t i = 0; i < w_.size(); ++i)
        {
            std::int64_t d = static_cast<std::int64_t>(w_[i]) - static_cast<std::int64_t>(o.w_[i]) - borrow;
            if (d < 0)
            {
                d += (std::int64_t{1} << 32);
                borrow = 1;
            }
            else
            {
                borrow = 0;
            }
            r.w_[i] = static_cast<std::uint32_t>(d);
        }
        return r;
    }

    // Schoolbook truncado a words() palabras: aritmetica modular.
    [[nodiscard]] big mul(const big &o) const
    {
        big r{w_.size()};
        for (std::size_t i = 0; i < w_.size(); ++i)
        {
            std::uint64_t carry = 0;
            for (std::size_t j = 0; i + j < w_.size(); ++j)
            {
                const std::uint64_t cur = static_cast<std::uint64_t>(r.w_[i + j]) +
                                          static_cast<std::uint64_t>(w_[i]) * o.w_[j] + carry;
                r.w_[i + j] = static_cast<std::uint32_t>(cur & 0xFFFFFFFFU);
                carry = cur >> 32;
            }
        }
        return r;
    }

    [[nodiscard]] int cmp(const big &o) const
    {
        for (std::size_t i = w_.size(); i-- > 0;)
        {
            if (w_[i] != o.w_[i])
                return w_[i] < o.w_[i] ? -1 : 1;
        }
        return 0;
    }

    // Division larga bit a bit: lo mas tonto que existe, y por eso util.
    void divmod(const big &d, big &q, big &r) const
    {
        q = big{w_.size()};
        r = big{w_.size()};
        for (std::size_t k = w_.size() * 32; k-- > 0;)
        {
            r = r.shl(1);
            if (bit(k))
                r.set_bit(0);
            if (r.cmp(d) >= 0)
            {
                r = r.sub(d);
                q.set_bit(k);
            }
        }
    }

    [[nodiscard]] big shl(std::size_t n) const
    {
        big r{w_.size()};
        const std::size_t bits = w_.size() * 32;
        if (n >= bits)
            return r;
        for (std::size_t k = bits; k-- > n;)
            if (bit(k - n))
                r.set_bit(k);
        return r;
    }

    [[nodiscard]] big shr(std::size_t n) const
    {
        big r{w_.size()};
        const std::size_t bits = w_.size() * 32;
        if (n >= bits)
            return r;
        for (std::size_t k = 0; k + n < bits; ++k)
            if (bit(k + n))
                r.set_bit(k);
        return r;
    }

    [[nodiscard]] big bit_and(const big &o) const
    {
        big r{w_.size()};
        for (std::size_t i = 0; i < w_.size(); ++i)
            r.w_[i] = w_[i] & o.w_[i];
        return r;
    }
    [[nodiscard]] big bit_or(const big &o) const
    {
        big r{w_.size()};
        for (std::size_t i = 0; i < w_.size(); ++i)
            r.w_[i] = w_[i] | o.w_[i];
        return r;
    }
    [[nodiscard]] big bit_xor(const big &o) const
    {
        big r{w_.size()};
        for (std::size_t i = 0; i < w_.size(); ++i)
            r.w_[i] = w_[i] ^ o.w_[i];
        return r;
    }
    [[nodiscard]] big neg() const
    {
        big zero{w_.size()};
        return zero.sub(*this);
    }
    [[nodiscard]] bool negative_as_signed() const { return (w_.back() >> 31) != 0; }

    // Decimal por division repetida entre 10: otra vez, lo mas simple posible.
    [[nodiscard]] std::string to_decimal() const
    {
        if (is_zero())
            return "0";
        big ten{w_.size()};
        ten.w_[0] = 10;
        big cur = *this;
        std::string rev;
        while (!cur.is_zero())
        {
            big q{w_.size()}, r{w_.size()};
            cur.divmod(ten, q, r);
            rev.push_back(static_cast<char>('0' + r.w_[0]));
            cur = q;
        }
        return std::string(rev.rbegin(), rev.rend());
    }

private:
    std::vector<std::uint32_t> w_;
};

// =============================================================================
// Puentes fixed_int_t <-> big
// =============================================================================

template <typename T, std::size_t N>
static big to_big(const T &v)
{
    std::vector<std::uint64_t> limbs(N);
    for (std::size_t i = 0; i < N; ++i)
        limbs[i] = v.limb(i);
    return big::from_fixed_limbs(limbs);
}

template <typename T, std::size_t N>
static bool equals(const T &v, const big &b)
{
    const auto limbs = b.to_fixed_limbs();
    for (std::size_t i = 0; i < N; ++i)
        if (v.limb(i) != limbs[i])
            return false;
    return true;
}

// =============================================================================
// Comparacion masiva para un N y un signo
// =============================================================================

template <std::size_t N, bool Signed>
static void sweep(const char *tag, int cases)
{
    using T = std::conditional_t<Signed, int_fixed_t<N>, uint_fixed_t<N>>;

    std::mt19937_64 rng(0xC0FFEEULL + N * 1000U + (Signed ? 1U : 0U));

    long long checks = 0;
    int mismatches = 0;
    std::string first_failure;

    const auto note = [&](const char *op, const T &a, const T &b, unsigned sh)
    {
        if (mismatches++ == 0)
        {
            first_failure = std::string{op} + "  a=" + a.to_string() + "  b=" + b.to_string() +
                            "  sh=" + std::to_string(sh);
        }
    };

    for (int c = 0; c < cases; ++c)
    {
        T a{}, b{};
        // Mezcla de anchos: valores densos, con limbos altos a cero y pequenyos.
        const int la = 1 + static_cast<int>(rng() % N);
        const int lb = 1 + static_cast<int>(rng() % N);
        for (int k = 0; k < static_cast<int>(N); ++k)
        {
            a.set_limb(static_cast<std::size_t>(k), k < la ? rng() : 0);
            b.set_limb(static_cast<std::size_t>(k), k < lb ? rng() : 0);
        }
        if (b.is_zero())
            b.set_limb(0, 1);

        const big ba = to_big<T, N>(a);
        const big bb = to_big<T, N>(b);
        const unsigned sh = static_cast<unsigned>(rng() % (64U * N));

        // --- Aritmetica modular: identica con y sin signo (complemento a dos) ---
        if (!equals<T, N>(a + b, ba.add(bb)))
            note("+", a, b, sh);
        ++checks;
        if (!equals<T, N>(a - b, ba.sub(bb)))
            note("-", a, b, sh);
        ++checks;
        if (!equals<T, N>(a * b, ba.mul(bb)))
            note("*", a, b, sh);
        ++checks;
        if (!equals<T, N>(-a, ba.neg()))
            note("neg", a, b, sh);
        ++checks;

        // --- Bitwise ---
        if (!equals<T, N>(a & b, ba.bit_and(bb)))
            note("&", a, b, sh);
        ++checks;
        if (!equals<T, N>(a | b, ba.bit_or(bb)))
            note("|", a, b, sh);
        ++checks;
        if (!equals<T, N>(a ^ b, ba.bit_xor(bb)))
            note("^", a, b, sh);
        ++checks;

        // --- Desplazamientos ---
        if (!equals<T, N>(a << sh, ba.shl(sh)))
            note("<<", a, b, sh);
        ++checks;

        // --- Division y modulo: sobre magnitudes, con las reglas de signo aparte ---
        if constexpr (!Signed)
        {
            big q{2 * N}, r{2 * N};
            ba.divmod(bb, q, r);
            if (!equals<T, N>(a / b, q))
                note("/", a, b, sh);
            ++checks;
            if (!equals<T, N>(a % b, r))
                note("%", a, b, sh);
            ++checks;

            // >> logico
            if (!equals<T, N>(a >> sh, ba.shr(sh)))
                note(">>", a, b, sh);
            ++checks;

            // Round-trip decimal contra el oraculo.
            if (a.to_string() != ba.to_decimal())
                note("to_string", a, b, sh);
            ++checks;
            if (T::from_string(a.to_string().c_str()) != a)
                note("from_string", a, b, sh);
            ++checks;
        }
        else
        {
            // Division truncada: signo del cociente = xor de signos; el resto
            // toma el signo del dividendo.
            const bool a_neg = ba.negative_as_signed();
            const bool b_neg = bb.negative_as_signed();
            const big ma = a_neg ? ba.neg() : ba;
            const big mb = b_neg ? bb.neg() : bb;
            big q{2 * N}, r{2 * N};
            ma.divmod(mb, q, r);
            const big expect_q = (a_neg != b_neg) ? q.neg() : q;
            const big expect_r = a_neg ? r.neg() : r;
            if (!equals<T, N>(a / b, expect_q))
                note("/", a, b, sh);
            ++checks;
            if (!equals<T, N>(a % b, expect_r))
                note("%", a, b, sh);
            ++checks;

            // >> aritmetico: relleno de signo.
            big expect_shr = ba.shr(sh);
            if (a_neg && sh > 0)
            {
                const std::size_t bits = 64U * N;
                for (std::size_t k = (sh >= bits ? 0 : bits - sh); k < bits; ++k)
                    expect_shr.set_bit(k);
            }
            if (!equals<T, N>(a >> sh, expect_shr))
                note(">>", a, b, sh);
            ++checks;

            // to_string con signo: '-' + magnitud.
            const std::string expect_str = a_neg ? "-" + ba.neg().to_decimal() : ba.to_decimal();
            if (a.to_string() != expect_str)
                note("to_string", a, b, sh);
            ++checks;
            if (T::from_string(a.to_string().c_str()) != a)
                note("from_string", a, b, sh);
            ++checks;
        }
    }

    const std::string name =
        std::string{"diferencial "} + tag + " (" + std::to_string(checks) + " comprobaciones)";
    if (mismatches != 0)
        std::cout << "       primera discrepancia: " << first_failure << "\n";
    TEST(name.c_str(), mismatches == 0);
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "====================================================================\n";
    std::cout << "fixed_int_t<N> — fuzz diferencial contra oraculo independiente\n";
    std::cout << "====================================================================\n";
    std::cout << "Oraculo: enteros grandes en base 2^32, schoolbook + division larga\n";
    std::cout << "bit a bit. No comparte codigo con fixed_int_t.\n\n";

    // El oraculo es O(bits^2) en la division, asi que el numero de casos baja
    // conforme sube N. Aun asi son decenas de miles de comprobaciones.
    sweep<2, false>("uint128", 900);
    sweep<2, true>("int128", 900);
    sweep<4, false>("uint256", 600);
    sweep<4, true>("int256", 600);
    sweep<8, false>("uint512", 300);
    sweep<8, true>("int512", 300);

    std::cout << "\n====================================================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "====================================================================\n";

    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
