#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Que acompanamiento de `std` tiene cada forma, comprobado COMPILANDO.

El objetivo es 24 de 24 en las dos formas nuevas: `fixed_int_t` en sus cuatro
representaciones y `fixed_point_t`. `int128_param_t` (1.75) queda fuera por
ADR-006: esta en migracion hacia `fixed_int_t` y completarla seria invertir en
lo que se va a retirar.

POR QUE SE COMPILA Y NO SE LEE
------------------------------
Un `API_*.md` o un `.hpp` con el nombre adecuado no dice si la capacidad existe
para un tipo concreto. La matriz de paridad destapo el 21 sep siete sitios
escritos sobre alias que fijaban parametros, y todos «existian» segun el nombre
del fichero.

Y esta misma auditoria se equivoco dos veces antes de quedar asi, las dos por
medir una cosa e ir a informar de otra:

1. **Incluia solo la cabecera principal.** Con eso `std::hash` salia «no existe»
   para los limbos, habiendo un `fixed_int_hash.hpp`. Eso responde a «¿viene de
   balde con el tipo?», que no es la pregunta.
2. **Tres sondas no podian dar «no»:** `numeric_limits<T>::epsilon()`,
   `common_type_t<T,T>` y `std::swap` compilan para cualquier tipo por la
   plantilla primaria. Decian «si» sin comprobar nada.

De ahi las dos columnas de cada celda:

    EXISTE  con toda la familia incluida: ¿esta escrito para este tipo?
    SUELTO  solo con la cabecera del tipo: ¿viene sin incluir nada mas?

Y de ahi la **autoprueba**: antes de informar, el script comprueba que sabe
decir «no». Si la sonda de un tipo inventado pasara, ningun «si» significaria
nada.

NO APLICA NO ES UN HUECO
------------------------
Algunas capacidades no tienen sentido en punto fijo: `popcount`, `rotl` y demas
son de bits, y en un punto fijo los bits no son el valor. Eso se declara, y
cuenta como respondida. Es lo que ADR-020 razono para los bitwise.
"""

import argparse
import concurrent.futures
import os
import pathlib
import subprocess
import sys
import tempfile

RAIZ = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RAIZ / "scripts"))

import toolchains  # noqa: E402
from env_setup.compiler_env import CompilerEnvironment  # noqa: E402

INC = RAIZ / "include"

# Toda la familia de cada forma. Sin esto, la pregunta que se contesta es otra.
FAMILIA_LIMBOS = ['#include "fixed_width_int_t.hpp"'] + [
    '#include "%s"' % p.name for p in sorted(INC.glob("fixed_int_*.hpp"))
]
# OJO: la familia del punto fijo es `fixed_point_*.hpp`, no solo el tipo. Si
# aqui se listara solo `fixed_point_t.hpp`, este guion volveria a contestar «¿te
# viene de balde?» en vez de «¿existe?», que es el error que ya cometio dos
# veces la auditoria a mano.
FAMILIA_FIJO = sorted('#include "%s"' % p.name for p in INC.glob('fixed_point_*.hpp')) + FAMILIA_LIMBOS

# Las celdas: las cuatro representaciones del entero mas las dos del punto fijo.
# «Los enteros con sus distintos formatos» es parte del objetivo, no un detalle:
# MS y EK se implementaron el 22 sep y una capacidad puede existir en
# complemento a dos y no en ellas.
CELDAS = [
    ("uint", "nstd::uint_fixed_t<2>", FAMILIA_LIMBOS),
    ("int/C2", "nstd::int_fixed_t<2>", FAMILIA_LIMBOS),
    ("int/MS", "nstd::fixed_int_t<2, nstd::signedness::signed_type, "
               "nstd::representation_form::magnitude_sign>", FAMILIA_LIMBOS),
    ("int/EK", "nstd::fixed_int_t<2, nstd::signedness::signed_type, "
               "nstd::representation_form::excess_k>", FAMILIA_LIMBOS),
    ("fijo/u", "nstd::ufixed_point_t<2, 1>", FAMILIA_FIJO),
    ("fijo/s", "nstd::sfixed_point_t<2, 1>", FAMILIA_FIJO),
]

ENTEROS = {"uint", "int/C2", "int/MS", "int/EK"}
FIJOS = {"fijo/u", "fijo/s"}
CON_SIGNO = {"int/C2", "int/MS", "int/EK", "fijo/s"}
TODAS = ENTEROS | FIJOS

# =============================================================================
# Las 24 capacidades
# =============================================================================
#
# `cuerpo` se compila dentro de `main()`, con `T` como el tipo de la celda y `a`
# y `b` ya construidos. `aplica` dice en que celdas TIENE que existir; donde no
# esta, se informa como «no aplica» y cuenta como respondida.

CAPS = [
    # --- nucleo: el suelo de la tabla -------------------------------------
    dict(grupo="Nucleo", nombre="construir y comparar", aplica=TODAS,
         cuerpo="(void)(a < b); (void)(a == b);"),
    dict(grupo="Nucleo", nombre="to_string miembro", aplica=TODAS,
         cuerpo="(void)a.to_string();"),

    # --- limites ----------------------------------------------------------
    dict(grupo="Limites", nombre="numeric_limits ESPECIALIZADO", aplica=TODAS,
         incluye=["#include <limits>"],
         cuerpo="static_assert(std::numeric_limits<T>::is_specialized);"),
    dict(grupo="Limites", nombre="limits: max/min/digits/radix", aplica=TODAS,
         incluye=["#include <limits>"],
         cuerpo="static_assert(std::numeric_limits<T>::digits > 0);\n"
                "    static_assert(std::numeric_limits<T>::radix == 2);\n"
                "    (void)std::numeric_limits<T>::max();\n"
                "    (void)std::numeric_limits<T>::min();"),

    # --- traits -----------------------------------------------------------
    dict(grupo="Traits", nombre="nstd::is_integral_v", aplica=ENTEROS,
         cuerpo="static_assert(nstd::is_integral_v<T>);"),
    dict(grupo="Traits", nombre="nstd::make_unsigned", aplica=TODAS,
         cuerpo="using U = typename nstd::make_unsigned<T>::type; (void)sizeof(U);"),
    dict(grupo="Traits", nombre="common_type con uint64_t", aplica=TODAS,
         incluye=["#include <type_traits>"],
         cuerpo="using C = std::common_type_t<T, std::uint64_t>; (void)sizeof(C);"),

    # --- STL --------------------------------------------------------------
    dict(grupo="STL", nombre="std::hash", aplica=TODAS,
         incluye=["#include <functional>"],
         cuerpo="(void)std::hash<T>{}(a);"),
    dict(grupo="STL", nombre="std::format", aplica=TODAS,
         incluye=["#include <format>"],
         cuerpo='(void)std::format("{}", a);'),
    dict(grupo="STL", nombre="iostream <<", aplica=TODAS,
         incluye=["#include <sstream>"],
         cuerpo="std::ostringstream os; os << a; (void)os.str();"),
    dict(grupo="STL", nombre="iostream >>", aplica=TODAS,
         incluye=["#include <sstream>"],
         cuerpo='T x{}; std::istringstream is("7"); is >> x; (void)x;'),
    dict(grupo="STL", nombre="ordenar en un vector", aplica=TODAS,
         incluye=["#include <vector>", "#include <algorithm>"],
         cuerpo="std::vector<T> v{b, a}; std::sort(v.begin(), v.end()); (void)v;"),

    # --- conceptos --------------------------------------------------------
    dict(grupo="Conceptos", nombre="std::three_way_comparable", aplica=TODAS,
         incluye=["#include <compare>", "#include <concepts>"],
         cuerpo="static_assert(std::three_way_comparable<T>);"),

    # --- cmath ------------------------------------------------------------
    dict(grupo="cmath", nombre="abs", aplica=TODAS,
         cuerpo="(void)nstd::abs(a);"),
    dict(grupo="cmath", nombre="sqrt", aplica=TODAS,
         cuerpo="(void)nstd::sqrt(a);"),
    dict(grupo="cmath", nombre="pow", aplica=TODAS,
         cuerpo="(void)nstd::pow(a, b);"),
    dict(grupo="cmath", nombre="floor/ceil/round/trunc", aplica=TODAS,
         cuerpo="(void)nstd::floor(a); (void)nstd::ceil(a);\n"
                "    (void)nstd::round(a); (void)nstd::trunc(a);"),

    # --- numeric ----------------------------------------------------------
    dict(grupo="numeric", nombre="gcd / lcm", aplica=TODAS,
         cuerpo="(void)nstd::gcd(a, b); (void)nstd::lcm(a, b);"),
    dict(grupo="numeric", nombre="midpoint", aplica=TODAS,
         cuerpo="(void)nstd::midpoint(a, b);"),

    # --- bits: NO aplican al punto fijo -----------------------------------
    #
    # En un punto fijo los bits no son el valor, asi que `popcount` contaria
    # bits de una codificacion y `rotl` moveria la coma. Es el mismo argumento
    # que dejo fuera los bitwise en ADR-020.
    dict(grupo="bits", nombre="popcount / countl_zero", aplica=ENTEROS,
         cuerpo="(void)nstd::popcount(a); (void)nstd::countl_zero(a);"),
    dict(grupo="bits", nombre="rotl / rotr", aplica=ENTEROS,
         cuerpo="(void)nstd::rotl(a, 3); (void)nstd::rotr(a, 3);"),

    # --- politicas --------------------------------------------------------
    dict(grupo="Politicas", nombre="checked_add / saturating_add", aplica=TODAS,
         cuerpo="(void)nstd::checked_add(a, b); (void)nstd::saturating_add(a, b);"),
    dict(grupo="Politicas", nombre="envoltorio atomico", aplica=ENTEROS,
         incluye=['#include "fixed_int_atomic.hpp"'],
         cuerpo="nstd::atomic_fixed_int_t<T::num_limbs, T::sign, T::form, T::policy> at{a};\n"
                "    (void)at.load();"),

    # --- ranges -----------------------------------------------------------
    dict(grupo="ranges", nombre="views::iota", aplica=TODAS,
         incluye=["#include <ranges>"],
         cuerpo="auto r = std::views::iota(a, b); (void)r.begin();"),
]

# =============================================================================
# Los nombres de `<bit>` que faltan (etapa E3). NO cuentan en las 24.
# =============================================================================
#
# Se vigilan aparte para que el 24/24 no se mueva con ellos, pero sin callarlos:
# son cobertura que no se ha obtenido.
EXTRA_BIT = [
    ("countl_one / countr_one", "(void)nstd::countl_one(a); (void)nstd::countr_one(a);"),
    ("bit_ceil / bit_floor", "(void)nstd::bit_ceil(a); (void)nstd::bit_floor(a);"),
    ("has_single_bit", "(void)nstd::has_single_bit(a);"),
    ("byteswap", "(void)nstd::byteswap(a);"),
    ("to_chars / from_chars", "char buf[64]; (void)nstd::to_chars(buf, buf + 64, a);"),
]


def fuente(cap, cabeceras, tipo, con_signo):
    """La unidad de traduccion de una sonda."""
    incs = list(cabeceras) + list(cap.get("incluye", [])) + ["#include <cstdint>"]
    vistos, unicos = set(), []
    for i in incs:
        if i not in vistos:
            vistos.add(i)
            unicos.append(i)
    # `a` y `b` se construyen siempre igual; el signo solo cambia si el tipo lo
    # admite, para no pedirle a un tipo sin signo que guarde un negativo.
    val_a = "7" if not con_signo else "7"
    val_b = "3" if not con_signo else "3"
    return ("%s\n\nusing T = %s;\n\nint main()\n{\n"
            "    const T a{%s};\n    const T b{%s};\n    (void)a;\n    (void)b;\n"
            "    %s\n    return 0;\n}\n"
            % ("\n".join(unicos), tipo, val_a, val_b, cap["cuerpo"]))


def limpia(s):
    return "".join(c if c.isalnum() else "_" for c in s)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--compiler", default="gcc",
                    choices=["gcc", "clang", "clang-libstdcxx"])
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 1))
    ap.add_argument("--detalle", action="store_true",
                    help="imprime la primera linea de error de cada hueco")
    args = ap.parse_args()

    cc = toolchains.resolve(args.compiler)
    env = CompilerEnvironment(args.compiler).get_env()

    print("=" * 96)
    print("  Acompanamiento de std: 24 capacidades x %d celdas" % len(CELDAS))
    print("=" * 96)
    print("  " + toolchains.describe(args.compiler, cc))
    print("  EXISTE (toda la familia incluida) / SUELTO (solo la cabecera del tipo)")
    print()

    with tempfile.TemporaryDirectory() as tmp:
        tmpd = pathlib.Path(tmp)

        # --- sonda de arranque --------------------------------------------
        sonda = tmpd / "arranque.cpp"
        sonda.write_text("int main() {}", encoding="utf-8")
        p = subprocess.run([cc, "-std=c++20", "-fsyntax-only", str(sonda)],
                           capture_output=True, text=True, env=env)
        if p.returncode != 0:
            print("  [FALLO] el compilador no compila ni un `int main(){}`"
                  " (salida %d, %d bytes de stderr)." % (p.returncode, len(p.stderr)))
            print("  No se audita nada: no habria forma de separar una carencia del")
            print("  tipo de un compilador que no arranca.")
            return 1

        def compila(nombre, src):
            f = tmpd / (nombre + ".cpp")
            f.write_text(src, encoding="utf-8")
            r = subprocess.run([cc, "-std=c++20", "-fsyntax-only", "-I" + str(INC), str(f)],
                               capture_output=True, text=True, env=env)
            primera = next((l for l in r.stderr.splitlines() if "error" in l.lower()), "")
            return r.returncode == 0, primera

        # --- autoprueba: la sonda tiene que saber decir «no» --------------
        falso = ('#include "fixed_width_int_t.hpp"\n\nint main()\n{\n'
                 "    const nstd::uint_fixed_t<2> a{7};\n"
                 "    (void)nstd::funcion_que_no_existe(a);\n    return 0;\n}\n")
        ok_falso, _ = compila("autoprueba", falso)
        if ok_falso:
            print("  [FALLO] la autoprueba COMPILA una funcion que no existe.")
            print("  Si la sonda no sabe decir «no», ningun «si» de esta tabla")
            print("  significa nada. No se informa.")
            return 1
        print("  sonda de arranque: OK | autoprueba: sabe decir «no»")
        print()

        # --- las sondas ----------------------------------------------------
        tareas = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
            for ci, cap in enumerate(CAPS):
                for celda, tipo, fam in CELDAS:
                    if celda not in cap["aplica"]:
                        continue
                    con_signo = celda in CON_SIGNO
                    sola = [fam[0]]
                    tareas.append((ci, celda, "todas",
                                   ex.submit(compila, "a%02d_%s" % (ci, limpia(celda)),
                                             fuente(cap, fam, tipo, con_signo))))
                    tareas.append((ci, celda, "sola",
                                   ex.submit(compila, "s%02d_%s" % (ci, limpia(celda)),
                                             fuente(cap, sola, tipo, con_signo))))
            # los extras de <bit>, solo sobre los enteros
            extras = []
            for ei, (nombre, cuerpo) in enumerate(EXTRA_BIT):
                cap = dict(nombre=nombre, cuerpo=cuerpo, aplica=ENTEROS)
                for celda, tipo, fam in CELDAS:
                    if celda not in ENTEROS:
                        continue
                    extras.append((ei, celda,
                                   ex.submit(compila, "e%02d_%s" % (ei, limpia(celda)),
                                             fuente(cap, fam, tipo, celda in CON_SIGNO))))
            res = {(ci, celda, modo): f.result() for ci, celda, modo, f in tareas}
            res_extra = {(ei, celda): f.result() for ei, celda, f in extras}

    # --- informe -------------------------------------------------------------
    ancho = max(len(c["nombre"]) for c in CAPS)
    print("  %-*s  %s" % (ancho, "", "".join("%-9s" % c[0] for c in CELDAS)))
    grupo = None
    faltan = {c[0]: [] for c in CELDAS}

    for ci, cap in enumerate(CAPS):
        if cap["grupo"] != grupo:
            grupo = cap["grupo"]
            print("  --- %s ---" % grupo)
        marcas = []
        for celda, _, _ in CELDAS:
            if celda not in cap["aplica"]:
                marcas.append("%-9s" % "  n/a")
                continue
            e, err = res[(ci, celda, "todas")]
            s, _ = res[(ci, celda, "sola")]
            marcas.append("%-9s" % ("%s/%s" % ("si" if e else "NO", "si" if s else "-")))
            if not e:
                faltan[celda].append((cap["nombre"], err))
        print("  %-*s  %s" % (ancho, cap["nombre"], "".join(marcas)))

    print()
    print("  --- E3: nombres de `<bit>` que faltan (NO cuentan en las 24) ---")
    ancho_e = max(len(n) for n, _ in EXTRA_BIT)
    pendientes_bit = 0
    for ei, (nombre, _) in enumerate(EXTRA_BIT):
        marcas = []
        for celda, _, _ in CELDAS:
            if celda not in ENTEROS:
                marcas.append("%-9s" % "  n/a")
                continue
            ok, _ = res_extra[(ei, celda)]
            marcas.append("%-9s" % ("  si" if ok else "  NO"))
            if not ok:
                pendientes_bit += 1
        print("  %-*s  %s" % (ancho_e, nombre, "".join(marcas)))

    print()
    print("=" * 96)
    total_mal = 0
    for celda, _, _ in CELDAS:
        aplican = sum(1 for c in CAPS if celda in c["aplica"])
        n_a = len(CAPS) - aplican
        mal = len(faltan[celda])
        bien = aplican - mal
        total_mal += mal
        print("  %-8s %2d/%d   (%d respondidas: %d si, %d no aplica, %d FALTAN)"
              % (celda, bien + n_a, len(CAPS), len(CAPS) - mal, bien, n_a, mal))
    print("  %-8s %d sondas de <bit> pendientes (etapa E3)" % ("extra", pendientes_bit))
    print("=" * 96)
    print()

    for celda, _, _ in CELDAS:
        if not faltan[celda]:
            continue
        print("  %s -- faltan %d:" % (celda, len(faltan[celda])))
        for nombre, err in faltan[celda]:
            print("    - %s" % nombre)
            if args.detalle and err:
                print("        %s" % err.split("error:")[-1].strip()[:110])
        print()

    return 1 if total_mal else 0


if __name__ == "__main__":
    raise SystemExit(main())
