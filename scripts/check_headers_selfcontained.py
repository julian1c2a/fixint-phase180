#!/usr/bin/env python3
# =============================================================================
# check_headers_selfcontained.py - Todo header debe compilar por si solo
# =============================================================================
#
# Part of int128 Library
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julian Calderon Almendros
#
# T2.5 (auditoria 23 ago 2026).
#
# Un header instalable tiene que compilar aislado: si depende de que el usuario
# haya incluido otra cosa antes, el orden de inclusion se vuelve parte del
# contrato y se rompe en cuanto alguien cambia el orden.
#
# La auditoria encontro uno asi: int128_param_traits_specializations.hpp usaba
# nstd::int128_param_t sin incluir su definicion. Este script evita que vuelva a
# pasar.
#
# Genera un .cpp con un unico #include por cada header de include/ y lo compila
# con -fsyntax-only. Comprueba tambien que la doble inclusion es idempotente
# (guardas de inclusion correctas).
#
# Uso:
#   python scripts/check_headers_selfcontained.py [--compiler gcc|clang]
#
# Salida: 0 si todos pasan, 1 si alguno falla.
# =============================================================================

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
INCLUDE_DIR = PROJECT_ROOT / "include"

sys.path.insert(0, str(Path(__file__).resolve().parent))
import toolchains  # noqa: E402


def headers():
    """Todos los .hpp de include/, en orden estable."""
    return sorted(INCLUDE_DIR.rglob("*.hpp"))


def check_one(compiler_cmd, header, extra_flags, tmpdir):
    """Compila un TU que solo incluye `header`, dos veces (idempotencia)."""
    rel = header.relative_to(INCLUDE_DIR).as_posix()
    src = Path(tmpdir) / (header.stem + "_selfcontained.cpp")
    src.write_text(
        '#include "%s"\n#include "%s"\n' % (rel, rel),
        encoding="utf-8",
    )

    cmd = [compiler_cmd, "-std=c++20", "-fsyntax-only", f"-I{INCLUDE_DIR}"] + extra_flags + [str(src)]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    return proc.returncode == 0, proc.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="gcc", choices=["gcc", "clang"])
    args = parser.parse_args()

    compiler_cmd = toolchains.resolve(args.compiler)
    print("=" * 78)
    print("  Headers autocontenidos (T2.5)")
    print("=" * 78)
    print("  " + toolchains.describe(args.compiler, compiler_cmd))
    print()

    extra_flags = []
    if args.compiler == "clang":
        # GM_TABLE supera el limite por defecto de pasos constexpr de Clang.
        # Ver T7.3: la solucion de fondo es que la tabla deje de ser constexpr.
        extra_flags.append("-fconstexpr-steps=100000000")

    failed = []
    with tempfile.TemporaryDirectory() as tmpdir:
        for header in headers():
            rel = header.relative_to(PROJECT_ROOT).as_posix()
            ok, stderr = check_one(compiler_cmd, header, extra_flags, tmpdir)
            if ok:
                print(f"  [OK]   {rel}")
            else:
                first = ""
                for line in stderr.splitlines():
                    if "error:" in line:
                        first = line.strip()[:140]
                        break
                print(f"  [FAIL] {rel}")
                if first:
                    print(f"         {first}")
                failed.append(rel)

    print()
    total = len(headers())
    print("=" * 78)
    print(f"  {total - len(failed)}/{total} headers compilan aislados")
    if failed:
        print("  Fallan: " + ", ".join(failed))
    print("=" * 78)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
