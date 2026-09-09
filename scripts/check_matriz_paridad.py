#!/usr/bin/env python3
# =============================================================================
# check_matriz_paridad.py - Cada capacidad publica, en cada celda del tipo
# =============================================================================
#
# Part of int128 Library
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julian Calderon Almendros
#
# P1.5 tramo 2 (9 sep 2026).
#
# POR QUE EXISTE
# --------------
# `fixed_int_t` tiene cuatro parametros de plantilla. Cada capacidad publica
# --construir, imprimir, formatear, dividir, `gcd`...-- tiene que funcionar en
# cada combinacion de esos parametros, y nada comprobaba eso.
#
# El dia que se escribio, P1.1 llevaba semanas habiendo anadido el cuarto
# parametro y SIETE sitios publicos se habian quedado con tres: el trait
# `is_unsigned_fixed_int` (mientras su hermano con signo si se generalizo), un
# temporal dentro de `to_string(base)`, los dos operadores de iostreams, la
# especializacion de `std::formatter`, dos alias internos y el constructor de
# conversion. Con un tipo `checked` no compilaba ni `std::cout << x`. La suite
# entera estaba en verde: probaba solo la politica por defecto.
#
# Ninguna tabla escrita a mano habria cazado eso, porque el problema es
# precisamente que la tabla y el codigo se separan sin que nadie lo note. Por
# eso este guion COMPILA una sonda por celda en vez de creerse una lista.
#
# COMO FUNCIONA
# -------------
# Para cada capacidad y cada celda (signo x politica) genera una unidad de
# traduccion minima y la compila con `-fsyntax-only`. Compara el resultado con
# lo que la capacidad DECLARA esperar:
#
#   TODAS      compila en las cuatro celdas
#   SIN_SIGNO  solo en las dos sin signo (y debe FALLAR en las de con signo)
#   CON_SIGNO  solo en las dos con signo
#
# Que una capacidad falle donde dice que debe fallar es tan importante como que
# funcione donde dice: si `midpoint` empezara a aceptar tipos con signo sin que
# nadie lo decidiera, esto lo dice.
#
# Ademas comprueba que `saturate` y `trap` **siguen sin compilar**. Estan en el
# enum desde P1.1 para no cambiar la ABI mas adelante, pero no estan escritas, y
# el `static_assert` de la clase tiene que seguir rechazandolas de forma ruidosa.
# El dia que se escriban, esta comprobacion falla y obliga a abrir dos columnas
# nuevas -- que es exactamente lo que se quiere.
#
# Uso:
#   python scripts/check_matriz_paridad.py [--compiler gcc|clang|clang-libstdcxx]
#                                          [--jobs N] [--escribe-doc]
#
# `--escribe-doc` regenera la tabla de docs/MATRIZ_DE_PARIDAD.md.
#
# Salida: 0 si toda celda se comporta como declara, 1 si alguna no.
# =============================================================================

import argparse
import concurrent.futures
import os
import subprocess
import sys
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
INCLUDE_DIR = PROJECT_ROOT / "include"
DOC = PROJECT_ROOT / "docs" / "MATRIZ_DE_PARIDAD.md"

sys.path.insert(0, str(Path(__file__).resolve().parent))
import toolchains  # noqa: E402
from env_setup.compiler_env import CompilerEnvironment  # noqa: E402

# =============================================================================
# Las celdas: signo x politica.
# =============================================================================
#
# `Form` no es un eje independiente: ADR-011 lo ata al signo (sin signo <=>
# binnat, con signo <=> complemento a dos), y el `static_assert` de la clase lo
# impone. Cuando se porten Magnitud-Signo y Exceso-K (P1.5 tramo 3) pasara a
# serlo, y entonces esta lista crece.
#
# `N` tampoco: los caminos que dependen de N --N=2 especializado, desenrollado
# hasta 20, Karatsuba en potencias de dos-- son de `operator*` y los cubre
# `benchmark_curva_n` mas `test_fixed_karatsuba`. Aqui se fija N=2 porque lo que
# se busca son huecos por PARAMETRO, no por anchura.

CELDAS = [
    ("uint/wrap", "nstd::uint_fixed_t<2>"),
    ("uint/checked", "nstd::uint_fixed_t<2, nstd::overflow_policy::checked>"),
    ("int/wrap", "nstd::int_fixed_t<2>"),
    ("int/checked", "nstd::int_fixed_t<2, nstd::overflow_policy::checked>"),
]

SIN_SIGNO = {"uint/wrap", "uint/checked"}
CON_SIGNO = {"int/wrap", "int/checked"}
TODAS = SIN_SIGNO | CON_SIGNO

# Politicas declaradas en el enum pero NO escritas. Tienen que seguir sin
# compilar; ver ADR-009.
RESERVADAS = [
    ("uint/saturate", "nstd::uint_fixed_t<2, nstd::overflow_policy::saturate>"),
    ("uint/trap", "nstd::uint_fixed_t<2, nstd::overflow_policy::trap>"),
]

BASE = ['#include "fixed_width_int_t.hpp"']

# =============================================================================
# Las capacidades. Una fila de la matriz cada una.
# =============================================================================
#
# `cuerpo` se compila dentro de `main()`, con `T` como el tipo de la celda.
# Anadir una capacidad es anadir una entrada aqui; no hay nada mas que tocar.

CAPACIDADES = [
    # --- nucleo del tipo ---------------------------------------------------
    dict(grupo="Nucleo", nombre="construir desde uint64", espera=TODAS,
         cuerpo="const T x{std::uint64_t{42}}; (void)x;"),
    # Sin signo desde un `int` NEGATIVO tambien compila, y esta bien: es lo
    # mismo que `unsigned x = -42;` en C++, que esta definido y da el valor
    # modulo 2^n. Lo declaraba CON_SIGNO y era mi declaracion la equivocada, no
    # el codigo. Queda escrito aqui para que nadie lo "arregle".
    dict(grupo="Nucleo", nombre="construir desde int", espera=TODAS,
         cuerpo="const T x{-42}; (void)x;"),
    dict(grupo="Nucleo", nombre="conversion cross-N", espera=TODAS,
         cuerpo="const T x{std::uint64_t{42}};\n"
                "    const auto y = nstd::fixed_int_t<4, T::sign, T::form, T::policy>{x};\n"
                "    (void)y;"),
    dict(grupo="Nucleo", nombre="max() / min() / one()", espera=TODAS,
         cuerpo="(void)T::max(); (void)T::min(); (void)T::one();"),
    dict(grupo="Nucleo", nombre="limb() / set_limb()", espera=TODAS,
         cuerpo="T x{}; x.set_limb(0, std::uint64_t{7}); (void)x.limb(0);"),

    # --- operadores --------------------------------------------------------
    dict(grupo="Operadores", nombre="aritmetica + - *", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)(a + b); (void)(a - b); (void)(a * b);"),
    dict(grupo="Operadores", nombre="division / y %", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)(a / b); (void)(a % b);"),
    dict(grupo="Operadores", nombre="divmod estatico", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    const auto qr = T::divmod(a, b); (void)qr.first;"),
    dict(grupo="Operadores", nombre="bitwise y desplazamientos", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)(a & b); (void)(a | b); (void)(a ^ b); (void)(~a);\n"
                "    (void)(a << 3U); (void)(a >> 3U);"),
    dict(grupo="Operadores", nombre="comparacion y <=>", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)(a == b); (void)(a < b); (void)(a <=> b);"),
    dict(grupo="Operadores", nombre="incremento y compuestos", espera=TODAS,
         cuerpo="T a{std::uint64_t{7}}; ++a; a--; a += T::one(); a *= T::one();"),

    # --- cadenas -----------------------------------------------------------
    dict(grupo="Cadenas", nombre="to_string()", espera=TODAS,
         cuerpo="const T x{std::uint64_t{42}}; (void)x.to_string();"),
    dict(grupo="Cadenas", nombre="to_string(base)", espera=TODAS,
         cuerpo="const T x{std::uint64_t{42}}; (void)x.to_string(16); (void)x.to_string(36);"),
    dict(grupo="Cadenas", nombre="try_from_string(base)", espera=TODAS,
         cuerpo="const auto r = T::try_from_string(\"2a\", 16); (void)r.value;"),
    dict(grupo="Cadenas", nombre="from_string(base)", espera=TODAS,
         cuerpo="(void)T::from_string(\"42\", 10);"),

    # --- integracion con la STL -------------------------------------------
    dict(grupo="STL", nombre="operator<< / >>", espera=TODAS,
         incluye=['#include "fixed_int_iostreams.hpp"', "#include <sstream>"],
         cuerpo="const T x{std::uint64_t{42}};\n"
                "    std::ostringstream os; os << x;\n"
                "    std::istringstream is(\"42\"); T y{}; is >> y;"),
    dict(grupo="STL", nombre="std::format", espera=TODAS,
         incluye=['#include "fixed_int_format.hpp"', "#include <format>"],
         cuerpo="const T x{std::uint64_t{42}};\n"
                "    (void)std::format(\"{}\", x); (void)std::format(\"{:#x}\", x);"),
    dict(grupo="STL", nombre="std::hash", espera=TODAS,
         incluye=['#include "fixed_int_hash.hpp"', "#include <unordered_set>"],
         cuerpo="std::unordered_set<T> s; s.insert(T{std::uint64_t{42}});"),
    dict(grupo="STL", nombre="std::numeric_limits", espera=TODAS,
         incluye=['#include "fixed_int_limits.hpp"'],
         cuerpo="static_assert(std::numeric_limits<T>::is_specialized);\n"
                "    static_assert(std::numeric_limits<T>::digits > 0);"),
    dict(grupo="STL", nombre="std::common_type", espera=TODAS,
         incluye=['#include "fixed_int_traits_specializations.hpp"'],
         cuerpo="static_assert(std::is_same_v<std::common_type_t<T, T>, T>);"),
    dict(grupo="STL", nombre="algoritmos de iterador de std::", espera=TODAS,
         incluye=["#include <algorithm>", "#include <numeric>", "#include <vector>"],
         cuerpo="std::vector<T> v(3, T{std::uint64_t{2}});\n"
                "    (void)std::find(v.begin(), v.end(), T{std::uint64_t{2}});\n"
                "    (void)std::min_element(v.begin(), v.end());\n"
                "    (void)std::accumulate(v.begin(), v.end(), T{});"),

    # --- traits y conceptos ------------------------------------------------
    dict(grupo="Traits", nombre="nstd::is_integral y familia", espera=TODAS,
         incluye=['#include "fixed_int_traits_specializations.hpp"'],
         cuerpo="static_assert(nstd::is_integral<T>::value);\n"
                "    static_assert(nstd::is_arithmetic<T>::value);"),
    dict(grupo="Traits", nombre="is_fixed_int_v / signed / unsigned", espera=TODAS,
         cuerpo="static_assert(nstd::is_fixed_int_v<T>);\n"
                "    static_assert(nstd::is_signed_fixed_int_v<T> != nstd::is_unsigned_fixed_int_v<T>);"),
    dict(grupo="Traits", nombre="nstd::integral (concepto)", espera=TODAS,
         incluye=['#include "fixed_int_concepts.hpp"'],
         cuerpo="static_assert(nstd::integral<T>);"),
    dict(grupo="Traits", nombre="make_signed / make_unsigned", espera=TODAS,
         incluye=['#include "fixed_int_traits_specializations.hpp"'],
         cuerpo="using S = typename nstd::make_signed<T>::type;\n"
                "    using U = typename nstd::make_unsigned<T>::type;\n"
                "    static_assert(nstd::is_signed_fixed_int_v<S>);\n"
                "    static_assert(nstd::is_unsigned_fixed_int_v<U>);"),

    # --- aritmetica de orden superior --------------------------------------
    dict(grupo="Aritmetica", nombre="mul_wide", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}}; (void)nstd::mul_wide(a, b);"),
    dict(grupo="Aritmetica", nombre="mulhi / mullo", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)nstd::mulhi(a, b); (void)nstd::mullo(a, b);"),
    dict(grupo="Aritmetica", nombre="pow", espera=TODAS,
         cuerpo="const T a{std::uint64_t{3}};\n"
                "    (void)nstd::pow(a, typename nstd::make_unsigned<T>::type{std::uint64_t{5}});",
         incluye=['#include "fixed_int_traits_specializations.hpp"']),
    dict(grupo="Aritmetica", nombre="sqrt", espera=SIN_SIGNO,
         cuerpo="(void)nstd::sqrt(T{std::uint64_t{144}});"),
    dict(grupo="Aritmetica", nombre="gcd / lcm", espera=TODAS,
         cuerpo="const T a{std::uint64_t{12}}, b{std::uint64_t{18}};\n"
                "    (void)nstd::gcd(a, b); (void)nstd::lcm(a, b);"),
    # ESTA MATRIZ LAS ENCONTRO. En su primera pasada, las siete `checked_*` y
    # `saturating_*` solo aceptaban operandos `wrap`, asi que `checked_add(a, b)`
    # sobre un tipo `checked` no compilaba. Generalizadas en P1.5 tramo 2f, con
    # la marca PEGAJOSA: saturar no limpia una marca previa, porque un valor
    # marcado guarda dentro el resultado envuelto y saturar a partir de el no da
    # el valor saturado correcto.
    dict(grupo="Aritmetica", nombre="checked_add / sub / mul", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)nstd::checked_add(a, b); (void)nstd::checked_sub(a, b);\n"
                "    (void)nstd::checked_mul(a, b);"),
    dict(grupo="Aritmetica", nombre="saturating_add / sub / mul", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)nstd::saturating_add(a, b); (void)nstd::saturating_sub(a, b);\n"
                "    (void)nstd::saturating_mul(a, b);"),

    # --- bits (P1.5 tramo 1) -----------------------------------------------
    dict(grupo="Bits", nombre="rotl / rotr", espera=TODAS,
         cuerpo="const T x{std::uint64_t{42}}; (void)nstd::rotl(x, 7); (void)nstd::rotr(x, -7);"),
    dict(grupo="Bits", nombre="nombres de <bit>", espera=TODAS,
         cuerpo="const T x{std::uint64_t{42}};\n"
                "    (void)nstd::countl_zero(x); (void)nstd::countr_zero(x);\n"
                "    (void)nstd::popcount(x); (void)nstd::bit_width(x);"),
    dict(grupo="Bits", nombre="is_power_of_2", espera=TODAS,
         cuerpo="(void)nstd::is_power_of_2(T{std::uint64_t{8}});"),

    # --- numericas (P1.5 tramo 1) ------------------------------------------
    dict(grupo="Numericas", nombre="min / max / clamp", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)nstd::min(a, b); (void)nstd::max(a, b); (void)nstd::clamp(a, b, a);"),
    dict(grupo="Numericas", nombre="midpoint / abs_diff", espera=SIN_SIGNO,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}};\n"
                "    (void)nstd::midpoint(a, b); (void)nstd::abs_diff(a, b);"),
    dict(grupo="Numericas", nombre="abs (libre)", espera=TODAS,
         cuerpo="(void)nstd::abs(T{std::uint64_t{7}});"),
    dict(grupo="Numericas", nombre="is_even / is_odd / sign", espera=TODAS,
         cuerpo="const T x{std::uint64_t{42}};\n"
                "    (void)nstd::is_even(x); (void)nstd::is_odd(x); (void)nstd::sign(x);"),
    dict(grupo="Numericas", nombre="ilog2", espera=SIN_SIGNO,
         cuerpo="(void)nstd::ilog2(T{std::uint64_t{42}});"),
    dict(grupo="Numericas", nombre="divmod (libre)", espera=TODAS,
         cuerpo="const T a{std::uint64_t{7}}, b{std::uint64_t{3}}; (void)nstd::divmod(a, b);"),

    # --- la marca ----------------------------------------------------------
    dict(grupo="Politica", nombre="valid()", espera=TODAS,
         cuerpo="(void)T{std::uint64_t{1}}.valid();"),
]


def fuente(cap, tipo):
    """Genera la unidad de traduccion de una celda."""
    inc = BASE + list(cap.get("incluye", []))
    inc += ["#include <cstdint>", "#include <type_traits>"]
    vistos, unicos = set(), []
    for i in inc:
        if i not in vistos:
            vistos.add(i)
            unicos.append(i)
    return "%s\n\nusing T = %s;\n\nint main()\n{\n    %s\n    return 0;\n}\n" % (
        "\n".join(unicos), tipo, cap["cuerpo"])


def compila(compiler_cmd, env, tmpdir, nombre, tipo, cuerpo_cap):
    """Compila una celda. Devuelve (ok, stderr)."""
    src = Path(tmpdir) / (nombre + ".cpp")
    src.write_text(cuerpo_cap, encoding="utf-8")
    cmd = [compiler_cmd, "-std=c++20", "-fsyntax-only", f"-I{INCLUDE_DIR}", str(src)]
    p = subprocess.run(cmd, capture_output=True, text=True, check=False, env=env)
    return p.returncode == 0, p.stderr


def limpia(s):
    return "".join(c if c.isalnum() else "_" for c in s)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--compiler", default="gcc",
                    choices=["gcc", "clang", "clang-libstdcxx"])
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 1))
    ap.add_argument("--escribe-doc", action="store_true",
                    help="regenera la tabla de docs/MATRIZ_DE_PARIDAD.md")
    args = ap.parse_args()

    compiler_cmd = toolchains.resolve(args.compiler)
    env = CompilerEnvironment(args.compiler).get_env()

    print("=" * 78)
    print("  Matriz de paridad: cada capacidad publica, en cada celda del tipo")
    print("=" * 78)
    print("  " + toolchains.describe(args.compiler, compiler_cmd))
    print("  %d capacidades x %d celdas = %d sondas, con %d en paralelo"
          % (len(CAPACIDADES), len(CELDAS), len(CAPACIDADES) * len(CELDAS), args.jobs))
    print()

    with tempfile.TemporaryDirectory() as tmpdir:
        # Sonda de arranque: la leccion de check_headers_selfcontained.
        sonda = Path(tmpdir) / "arranque.cpp"
        sonda.write_text("int main() {}", encoding="utf-8")
        p = subprocess.run([compiler_cmd, "-std=c++20", "-fsyntax-only", str(sonda)],
                           capture_output=True, text=True, check=False, env=env)
        if p.returncode != 0:
            print("  [FALLO] el compilador no compila ni un `int main(){}`"
                  " (salida %d, %d bytes de stderr)." % (p.returncode, len(p.stderr)))
            print("  No se comprueba nada: no habria forma de separar una celda rota")
            print("  de un compilador que no arranca.")
            return 1

        tareas = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
            for ci, cap in enumerate(CAPACIDADES):
                for celda, tipo in CELDAS:
                    nombre = "c%02d_%s" % (ci, limpia(celda))
                    tareas.append((ci, celda,
                                   ex.submit(compila, compiler_cmd, env, tmpdir,
                                             nombre, tipo, fuente(cap, tipo))))
            # y las politicas reservadas, sobre una capacidad cualquiera
            reserva = []
            for celda, tipo in RESERVADAS:
                cap = CAPACIDADES[0]
                reserva.append((celda,
                                ex.submit(compila, compiler_cmd, env, tmpdir,
                                          "res_" + limpia(celda), tipo, fuente(cap, tipo))))

            resultados = {}
            for ci, celda, fut in tareas:
                resultados[(ci, celda)] = fut.result()
            reservadas = {celda: fut.result() for celda, fut in reserva}

    # --- informe ------------------------------------------------------------
    filas = []
    incoherencias = []
    grupo_actual = None
    ancho = max(len(c["nombre"]) for c in CAPACIDADES)

    for ci, cap in enumerate(CAPACIDADES):
        if cap["grupo"] != grupo_actual:
            grupo_actual = cap["grupo"]
            print("  --- %s ---" % grupo_actual)
        marcas, fila = [], []
        for celda, _ in CELDAS:
            ok, err = resultados[(ci, celda)]
            debe = celda in cap["espera"]
            if ok == debe:
                marcas.append("  ok  " if ok else "  --  ")
                fila.append("sí" if ok else "n/a")
            else:
                marcas.append(" ROTO " if debe else " DEMAS")
                fila.append("**ROTO**" if debe else "**de más**")
                incoherencias.append(
                    (cap["nombre"], celda, debe,
                     (err.strip().splitlines() or [""])[0][:120]))
        print("  %-*s %s" % (ancho, cap["nombre"], "".join(marcas)))
        filas.append((cap["grupo"], cap["nombre"], fila))

    print()
    print("  --- politicas declaradas pero NO escritas (ADR-009) ---")
    for celda, _ in RESERVADAS:
        ok, _ = reservadas[celda]
        if ok:
            print("  %-16s COMPILA -- y no deberia: no esta escrita" % celda)
            incoherencias.append((celda, "reservada", False,
                                  "compila una politica que no esta implementada"))
        else:
            print("  %-16s rechazada, como debe" % celda)

    print()
    print("=" * 78)
    if incoherencias:
        print("  %d celdas NO se comportan como declaran:" % len(incoherencias))
        for nombre, celda, debe, err in incoherencias:
            print("    %s / %s: %s" % (nombre, celda,
                                       "deberia compilar y no compila" if debe
                                       else "compila y no deberia"))
            if err:
                print("      %s" % err)
    else:
        print("  %d celdas, todas se comportan como declaran"
              % (len(CAPACIDADES) * len(CELDAS) + len(RESERVADAS)))
    print("=" * 78)

    if args.escribe_doc:
        escribe_doc(filas, reservadas, args.compiler,
                    toolchains.version_line(compiler_cmd))
        print("  docs/MATRIZ_DE_PARIDAD.md actualizado")

    return 1 if incoherencias else 0


MARCA_INI = "<!-- MATRIZ:INICIO -- generado por scripts/check_matriz_paridad.py -->"
MARCA_FIN = "<!-- MATRIZ:FIN -->"


def escribe_doc(filas, reservadas, compilador, version):
    """Reescribe la tabla entre las dos marcas. El resto del doc no se toca."""
    out = [MARCA_INI, ""]
    out.append("Generado con `%s` (%s)." % (compilador, version.strip()))
    out.append("")
    out.append("| | Capacidad | " + " | ".join(c for c, _ in CELDAS) + " |")
    out.append("|---|---|" + "---|" * len(CELDAS))
    grupo = None
    for g, nombre, celdas in filas:
        etiqueta = ""
        if g != grupo:
            grupo = g
            etiqueta = "**%s**" % g
        out.append("| %s | `%s` | %s |" % (etiqueta, nombre, " | ".join(celdas)))
    out.append("")
    out.append("Políticas declaradas en el enum pero **no escritas** (ADR-009), que")
    out.append("tienen que seguir sin compilar:")
    out.append("")
    out.append("| Celda | Estado |")
    out.append("|---|---|")
    for celda, _ in RESERVADAS:
        ok, _ = reservadas[celda]
        out.append("| `%s` | %s |" % (celda,
                                      "⚠️ **compila, y no debería**" if ok
                                      else "rechazada, como debe"))
    out.append("")
    out.append(MARCA_FIN)

    texto = "\n".join(out)
    doc = DOC.read_text(encoding="utf-8")
    i, j = doc.index(MARCA_INI), doc.index(MARCA_FIN) + len(MARCA_FIN)
    DOC.write_text(doc[:i] + texto + doc[j:], encoding="utf-8", newline="\n")


if __name__ == "__main__":
    sys.exit(main())
