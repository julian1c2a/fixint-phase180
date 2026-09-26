# -*- coding: utf-8 -*-
# =============================================================================
# int128 Library - 128-bit Integer Types for C++20
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Copyright (c) 2024-2026 Julian Calderon Almendros
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at
#  https://www.boost.org/LICENSE_1_0.txt)
#
# =============================================================================
# @file       rutas.py
# @brief      UN solo sitio que decide donde se compila y donde se busca
# @date       2026-09-26
# =============================================================================
#
# POR QUE EXISTE ESTE FICHERO
#
# El 26 sep 2026 la suite entera salio roja --gcc 1/72, MSVC e Intel con 216
# errores, WSL 0/3-- y el codigo estaba bien: `clang-libstdcxx` daba 72/72 con el
# mismo arbol. Tres defectos encadenados, y ninguno era del codigo probado:
#
#   1. WINDOWS Y WSL COMPARTIAN EL DIRECTORIO DE SALIDA.
#      Los dos escribian en `build/build_tests/<compilador>/<modo>/`, asi que los
#      ELF de Linux quedaban junto a los PE de Windows. `clang-libstdcxx` se
#      salvo por una razon tonta: es la unica familia que WSL no construye.
#
#   2. EL `.exe` SE DECIDIA MIRANDO `sys.platform`.
#      Eso no describe lo que se esta construyendo, describe **que interprete se
#      lanzo**. Y el `python` del PATH paso a ser el de MSYS, que dice
#      `sys.platform == 'cygwin'`, no `'win32'`. Con eso, `check_generic` dejo de
#      anadir `.exe` de un dia para otro sin que nadie tocara nada.
#
#   3. CYGWIN RESUELVE `foo` COMO `foo.exe` DE FORMA TRANSPARENTE.
#      Eso TAPABA el defecto 2 --todo seguia funcionando-- **mientras no hubiera
#      un fichero real sin extension**. Cuando WSL dejo los suyos, ganaron ellos,
#      y `Path.exists()` decia si sobre un ELF que Windows no puede ejecutar.
#
# La leccion: el nombre del binario no se RECONSTRUYE, se BUSCA. Y la plataforma
# no se deduce del interprete, se pregunta a `platform.system()`, que distingue
# `Linux` de `CYGWIN_NT-*`, `MSYS_NT-*` y `Windows` sin ambiguedad.
# =============================================================================
from __future__ import annotations

import platform
from pathlib import Path


def plataforma() -> str:
    """`'linux'` dentro de WSL o Linux; `'windows'` en Windows.

    **No usa `sys.platform`**, que depende del interprete y no del destino: la
    Python de MSYS dice `'cygwin'`, la de MinGW dice `'win32'` y las dos corren
    en Windows y producen binarios PE. `platform.system()` si los distingue:
    devuelve `CYGWIN_NT-10.0`, `MSYS_NT-10.0` o `Windows` en Windows, y `Linux`
    dentro de WSL.
    """
    return "linux" if platform.system() == "Linux" else "windows"


def raiz_proyecto() -> Path:
    """La raiz del repositorio, desde la ubicacion de este fichero."""
    return Path(__file__).resolve().parent.parent.parent


def dir_salida(clase: str, compilador: str, modo: str,
               raiz: Path | None = None) -> Path:
    """Donde van los binarios de `clase` (`tests`, `benchs`, `demos`).

    **La plataforma va en la ruta**, que es lo que impide que Windows y WSL se
    pisen. Antes era `build/build_tests/<compilador>/<modo>/` para los dos.

        build/build_tests/windows/gcc/release-O2/test_x_gcc.exe
        build/build_tests/linux/gcc/release-O2/test_x_gcc

    Un `make.py clean` sigue borrando los dos, porque borra `build_tests/`
    entero; lo que ya no pasa es que uno lea los binarios del otro.
    """
    r = raiz or raiz_proyecto()
    return r / "build" / f"build_{clase}" / plataforma() / compilador / modo


def dir_salida_clase(clase: str, raiz: Path | None = None) -> Path:
    """El directorio de `clase` para esta plataforma, sin compilador ni modo."""
    r = raiz or raiz_proyecto()
    return r / "build" / f"build_{clase}" / plataforma()


def resuelve_binario(directorio: Path, tronco: str) -> Path | None:
    """Encuentra el binario de verdad. **Lo busca, no lo reconstruye.**

    Acepta `tronco` y `tronco.exe` porque MinGW anade `.exe` al nombre que se le
    pasa en `-o` si no trae extension, mientras MSVC e Intel lo ponen en el
    propio `-o`: el mismo comando deja nombres distintos segun el compilador, y
    reconstruirlo a mano es lo que fallaba.

    Mira el **listado** del directorio, no `stat`: en Cygwin `stat('foo')`
    resuelve a `foo.exe` y entonces los dos nombres parecen existir siempre.

    @return La ruta al binario, o `None` si no hay ninguno.
    @raise  RuntimeError si existen **los dos** nombres. Eso ya no deberia poder
            pasar con la plataforma en la ruta, pero si vuelve a pasar es
            contaminacion entre plataformas y hay que verlo, no elegir uno en
            silencio -- que es exactamente como se perdio un dia entero.
    """
    # SE LISTA EL DIRECTORIO. No se pregunta por el fichero.
    #
    # En Cygwin, `stat()` sobre `foo` resuelve a `foo.exe` de forma transparente,
    # asi que `exists()` Y `is_file()` dan cierto para los dos nombres cuando
    # solo existe el `.exe`. La primera version de esta funcion usaba `is_file()`
    # creyendo que eso lo evitaba --lo decia el comentario-- y **era falso**: el
    # mapeo esta en `stat`, no en `exists`. El resultado fue una guarda que
    # saltaba siempre, con un falso positivo de «contaminacion».
    #
    # `readdir` no sintetiza entradas: la lista trae los nombres REALES.
    # Comprobado en la Python de MSYS: `is_file('foo')` da True y
    # `'foo' in listdir()` da False.
    import os
    try:
        nombres = set(os.listdir(directorio))
    except OSError:
        return None

    hay_pelado = tronco in nombres
    hay_exe = (tronco + ".exe") in nombres
    pelado = directorio / tronco
    con_exe = directorio / (tronco + ".exe")

    if hay_pelado and hay_exe:
        raise RuntimeError(
            "CONTAMINACION ENTRE PLATAFORMAS en %s: existen '%s' y '%s.exe'.\n"
            "  Un binario de Linux y uno de Windows con el mismo nombre. Borra\n"
            "  `build/` y vuelve a compilar. Si se repite, algo esta escribiendo\n"
            "  fuera del directorio de su plataforma (ver scripts/env_setup/rutas.py)."
            % (directorio, tronco, tronco))

    if hay_exe:
        return con_exe
    if hay_pelado:
        return pelado
    return None


def nombre_esperado(tronco: str) -> str:
    """Como se llamaria el binario en esta plataforma, para los mensajes de
    error. **Solo para mensajes**: para encontrarlo esta `resuelve_binario`."""
    return tronco + (".exe" if plataforma() == "windows" else "")
