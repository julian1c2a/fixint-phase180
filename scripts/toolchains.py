#!/usr/bin/env python3
# =============================================================================
# toolchains.py - Resolucion centralizada de compiladores
# =============================================================================
#
# Part of int128 Library
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julian Calderon Almendros
#
# Motivo (T1.1, auditoria 23 ago 2026):
#
#   En Windows con MSYS2 instalado, `g++` y `clang++` A SECAS resuelven a
#   C:\\msys64\\usr\\bin\\ -- el toolchain MSYS (target x86_64-pc-windows-cygnus),
#   que NO es el que usa el proyecto. Los binarios resultantes dependen de
#   msys-2.0.dll y no representan la plataforma objetivo (UCRT / MinGW-w64).
#
#   Este modulo es la UNICA fuente de verdad de que compilador se usa. Lo
#   consumen build_generic.py y check_generic.py.
#
# Prioridad de resolucion:
#   1. Variable de entorno (GCC_CXX / CLANG_CXX / INTEL_CXX / MSVC_CXX)
#      -> es lo que usa el CI, que inyecta g++-14, clang++-19, etc.
#   2. toolchains.json en la raiz del proyecto (si existe)
#   3. Default por plataforma (ver DEFAULTS_*)
#   4. Nombre pelado ("g++"), si nada de lo anterior existe en disco
#
# Uso como CLI (diagnostico):
#   python scripts/toolchains.py            # tabla de compiladores resueltos
#   python scripts/toolchains.py gcc        # solo uno
# =============================================================================

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent

ENV_VARS = {
    "gcc": "GCC_CXX",
    "clang": "CLANG_CXX",
    "intel": "INTEL_CXX",
    "msvc": "MSVC_CXX",
}

# Windows: rutas explicitas de MSYS2. Coinciden con AI_PROMPT/ai-instructions.md
# seccion "0. Compiler Paths (CRITICAL - DO NOT CHANGE)".
DEFAULTS_WINDOWS = {
    "gcc": r"C:/msys64/ucrt64/bin/g++.exe",
    "clang": r"C:/msys64/clang64/bin/clang++.exe",
    "intel": "icx",
    "msvc": "cl.exe",
}

DEFAULTS_POSIX = {
    "gcc": "g++",
    "clang": "clang++",
    "intel": "icpx",
    "msvc": "cl.exe",
}

# Toolchains que NO queremos usar por accidente en Windows.
_UNWANTED_WINDOWS_PREFIXES = (
    "c:/msys64/usr/bin",
    "c:\\msys64\\usr\\bin",
    "/usr/bin",  # dentro de la shell MSYS
)

_FALLBACK_NAMES = {"gcc": "g++", "clang": "clang++", "intel": "icx", "msvc": "cl.exe"}


def _platform_key():
    """Clave de plataforma para toolchains.json."""
    return "windows" if sys.platform == "win32" else "posix"


def _load_overrides():
    """Rutas de toolchains.json para ESTA plataforma, si las hay.

    El fichero admite dos formas:

        {"compilers": {"windows": {...}, "posix": {...}}}   <- recomendada
        {"compilers": {"gcc": "...", ...}}                  <- heredada

    La segunda se interpreta como si fuera de Windows, que es de donde venia.
    Esta distincion NO es cosmetica: la primera version de este fichero tenia
    las rutas de Windows en la raiz y el CI de Linux acabo intentando ejecutar
    'C:/msys64/ucrt64/bin/g++.exe'.
    """
    path = PROJECT_ROOT / "toolchains.json"
    if not path.exists():
        return {}
    try:
        with open(path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
        compilers = data.get("compilers", {}) or {}
        if any(k in compilers for k in ("windows", "posix", "linux")):
            key = _platform_key()
            out = compilers.get(key, {}) or {}
            if not out and key == "posix":
                out = compilers.get("linux", {}) or {}
            return out
        # Forma heredada: rutas sueltas, que eran de Windows.
        return compilers if sys.platform == "win32" else {}
    except (OSError, ValueError) as exc:
        print(f"[WARN] toolchains.json ilegible ({exc}); se ignora", file=sys.stderr)
        return {}


def resolve(name):
    """Devuelve el comando de compilador a usar para `name` (gcc|clang|intel|msvc)."""
    if name not in ENV_VARS:
        raise ValueError(f"compilador desconocido: {name}")

    env_value = os.environ.get(ENV_VARS[name])
    if env_value:
        return env_value

    overrides = _load_overrides()
    candidate = overrides.get(name) or None

    if candidate is None:
        defaults = DEFAULTS_WINDOWS if sys.platform == "win32" else DEFAULTS_POSIX
        candidate = defaults[name]

    # Toda ruta absoluta que no exista cae al nombre pelado, venga del JSON o del
    # default de plataforma. Sin esta red, un toolchains.json escrito en una
    # maquina rompe el build en cualquier otra.
    if ("/" in candidate or "\\" in candidate) and not Path(candidate).exists():
        return _FALLBACK_NAMES[name]

    return candidate


def target_triple(command):
    """Devuelve el target del compilador (-dumpmachine), o '' si no se puede."""
    try:
        out = subprocess.run(
            [command, "-dumpmachine"],
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
        )
        return out.stdout.strip().splitlines()[0] if out.stdout.strip() else ""
    except (OSError, subprocess.SubprocessError, IndexError):
        return ""


def version_line(command):
    """Primera linea de `command --version`, o '' si no se puede."""
    try:
        out = subprocess.run(
            [command, "--version"],
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
        )
        text = out.stdout or out.stderr
        return text.strip().splitlines()[0] if text.strip() else ""
    except (OSError, subprocess.SubprocessError, IndexError):
        return ""


def warn_if_unwanted(name, command):
    """Avisa si en Windows se ha resuelto el toolchain MSYS en vez del correcto."""
    if sys.platform != "win32" or name not in ("gcc", "clang"):
        return

    full = shutil.which(command) or command
    normalized = full.replace("\\", "/").lower()
    if any(normalized.startswith(p.replace("\\", "/").lower()) for p in _UNWANTED_WINDOWS_PREFIXES):
        print(
            f"[WARN] '{name}' resuelve a {full}\n"
            f"[WARN] Ese es el toolchain MSYS (msys-2.0.dll), no el del proyecto.\n"
            f"[WARN] Esperado: {DEFAULTS_WINDOWS[name]}\n"
            f"[WARN] Fija {ENV_VARS[name]} o crea toolchains.json para corregirlo.",
            file=sys.stderr,
        )


def describe(name, command=None):
    """Linea de diagnostico: 'gcc: <ruta> | <version> | <target>'."""
    cmd = command or resolve(name)
    full = shutil.which(cmd) or cmd
    parts = [f"{name}: {full}"]
    ver = version_line(cmd)
    if ver:
        parts.append(ver)
    triple = target_triple(cmd)
    if triple:
        parts.append(triple)
    return " | ".join(parts)


def main(argv):
    names = argv[1:] if len(argv) > 1 else list(ENV_VARS.keys())
    print("=" * 78)
    print("  Compiladores resueltos (scripts/toolchains.py)")
    print("=" * 78)
    for name in names:
        if name not in ENV_VARS:
            print(f"[ERROR] compilador desconocido: {name}")
            continue
        cmd = resolve(name)
        print(f"  {describe(name, cmd)}")
        warn_if_unwanted(name, cmd)
    print("=" * 78)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
