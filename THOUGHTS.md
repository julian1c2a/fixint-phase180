<!-- =========================================================================
     CUADERNO PERSONAL DEL AUTOR — FUERA DE TODO FLUJO DE DOCUMENTACION

     Notas informales, tentativas y material para conversar. NO es fuente de
     verdad de nada, NO se consolida, NO se archiva y NO lo revisa ningun
     verificador.

     Si estas haciendo una pasada de documentacion: este fichero no se toca.
     ========================================================================= -->

**Predicados/bit utilities:**

count_leading_zeros(),
count_trailing_zeros() (ambos tipos)
bit_width(),
popcount() en int_fixed_t (ahora solo en uint)
is_power_of_two() (unsigned)
is_positive(),
signum() (signed)

**Aritmética superior:**

mul_wide(a, b) → T<2N> — producto sin truncar
pow(base, exp) — exponenciación modular
sqrt() — raíz entera
gcd(),
lcm()
checked_add/sub/mul → optional<T> (la aritmética segura pendiente)

**Strings/formato:**

Bases 2, 8, 16 en to_string/from_string
operator<< / >> para std::ostream/istream
std::formatter (C++20)

**Integración stdlib:**

std::numeric_limits<> specialization
std::hash<> specialization
operator<=> (C++20 spaceship)
Trait template<typename T> nstd::is_fixed_int_v<T>

**Integración y operaciones binarias entre tipos enteros built-in:**

La integración con los tipos built-in que se daba en int128_parameterized_t (p. ej., operator+ entre int128_parameterized_t y __int128) se hace prioritaria: no solo es necesaria para que el tipo sea lo que pretende, una auténtica extensión de los tipos built-in, sino que cuando se generalice, si N == 2*M, si existe en el compilador un tipo built-in __int128_t, podríamos hacer un tipo int_fixed_t<M> que use como tipo base __int128_t. Además si int_fixed_t<N> es un tipo built-in y int_fixed_t<M> es otro, debería ocurrir que las operaciones binarias entre ellos se deben dar de forma natural.