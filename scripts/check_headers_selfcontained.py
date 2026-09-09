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
#   python scripts/check_headers_selfcontained.py [--compiler gcc|clang|clang-libstdcxx]
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
from env_setup.compiler_env import CompilerEnvironment  # noqa: E402


def headers():
    """Todos los .hpp de include/, en orden estable."""
    return sorted(INCLUDE_DIR.rglob("*.hpp"))


def check_one(compiler_cmd, header, extra_flags, tmpdir, env):
    """Compila un TU que solo incluye `header`, dos veces (idempotencia).

    `env` es el entorno AISLADO del toolchain, no el heredado. Ver la nota
    de `main()`: sin el, el compilador puede ni arrancar.
    """
    rel = header.relative_to(INCLUDE_DIR).as_posix()
    src = Path(tmpdir) / (header.stem + "_selfcontained.cpp")
    src.write_text(
        '#include "%s"\n#include "%s"\n' % (rel, rel),
        encoding="utf-8",
    )

    cmd = [compiler_cmd, "-std=c++20", "-fsyntax-only", f"-I{INCLUDE_DIR}"] + extra_flags + [str(src)]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False, env=env)
    return proc.returncode == 0, proc.returncode, proc.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--compiler",
        default="gcc",
        # Los tres de MinGW. MSVC e Intel quedan fuera porque este script
        # usa `-fsyntax-only`, que es sintaxis de GCC/clang.
        choices=["gcc", "clang", "clang-libstdcxx"],
    )
    args = parser.parse_args()

    compiler_cmd = toolchains.resolve(args.compiler)

    # EL ENTORNO IMPORTA, y no es el heredado. Los binarios de MSYS2 cargan
    # su runtime (libstdc++-6.dll y compania) por PATH; si delante va el bin
    # de otro toolchain --el mingw64 que trae Git, tipicamente-- el proceso
    # muere al cargar con 0xC0000139 y SIN ESCRIBIR NADA en stderr. Devuelve
    # 1, igual que un error de compilacion.
    #
    # Este script invocaba al compilador con el entorno heredado. Desde un
    # shell cuyo PATH no llevaba delante el bin del compilador informaba
    # 0/31 headers y ni una linea de error: exactamente lo que parece un
    # fallo del codigo. Se usa el entorno aislado de los guiones de
    # construccion, que ya resolvian esto.
    env = CompilerEnvironment(args.compiler).get_env()

    print("=" * 78)
    print("  Headers autocontenidos (T2.5)")
    print("=" * 78)
    print("  " + toolchains.describe(args.compiler, compiler_cmd))
    print()

    # T7.3: ya no hace falta -fconstexpr-steps con Clang; GM_TABLE era codigo
    # muerto y se ha eliminado. Se compila con los flags por defecto a proposito,
    # para que este script detecte cualquier regresion.
    extra_flags: list = []

    # Sonda de arranque: si el compilador no puede ni con un `int main(){}`,
    # el problema no esta en ningun header, y decirlo asi ahorra buscarlo
    # donde no esta.
    with tempfile.TemporaryDirectory() as tmpdir:
        sonda = Path(tmpdir) / "sonda_arranque.cpp"
        sonda.write_text("int main() {}", encoding="utf-8")
        p = subprocess.run(
            [compiler_cmd, "-std=c++20", "-fsyntax-only", str(sonda)],
            capture_output=True, text=True, check=False, env=env,
        )
        if p.returncode != 0:
            print("  [FALLO] el compilador no compila ni un `int main(){}`.")
            print("          salida %d, %d bytes de stderr."
                  % (p.returncode, len(p.stderr)))
            if p.stderr.strip():
                print("          " + p.stderr.strip().splitlines()[0][:140])
            else:
                print("          Sin stderr: casi seguro que muere al cargar sus")
                print("          DLL (0xC0000139). Revisa el PATH del entorno.")
            print("  No se comprueba ningun header: no habria forma de separar")
            print("  un header roto de un compilador que no arranca.")
            return 1

    failed = []
    with tempfile.TemporaryDirectory() as tmpdir:
        for header in headers():
            rel = header.relative_to(PROJECT_ROOT).as_posix()
            ok, rc, stderr = check_one(compiler_cmd, header, extra_flags, tmpdir, env)
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
                else:
                    print(f"         salida {rc}, sin linea 'error:' "
                          f"({len(stderr)} bytes de stderr)")
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
