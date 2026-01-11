#!/usr/bin/env python3
"""
Script para localizar vcvarsall.bat en instalaciones de Visual Studio.
Imprime la ruta completa del archivo vcvarsall.bat si lo encuentra.
"""

import os
import sys
from pathlib import Path

def find_vcvarsall():
    """Busca vcvarsall.bat en las rutas comunes de Visual Studio."""
    
    # Rutas comunes donde Visual Studio instala vcvarsall.bat
    search_paths = [
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat",
        r"C:\Program Files\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
        r"C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat",
        r"C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat",
    ]
    
    # Buscar en las rutas definidas
    for path in search_paths:
        if os.path.exists(path):
            return path
    
    # Si no se encuentra en rutas comunes, buscar usando variable de entorno
    vs_install_dir = os.environ.get("VSINSTALLDIR")
    if vs_install_dir:
        vcvars_path = os.path.join(vs_install_dir, "VC", "Auxiliary", "Build", "vcvarsall.bat")
        if os.path.exists(vcvars_path):
            return vcvars_path
    
    # Buscar recursivamente en Program Files
    program_files = [
        r"C:\Program Files\Microsoft Visual Studio",
        r"C:\Program Files (x86)\Microsoft Visual Studio"
    ]
    
    for base_dir in program_files:
        if not os.path.exists(base_dir):
            continue
        for root, dirs, files in os.walk(base_dir):
            if "vcvarsall.bat" in files:
                return os.path.join(root, "vcvarsall.bat")
    
    return None

if __name__ == "__main__":
    vcvars_path = find_vcvarsall()
    if vcvars_path:
        print(vcvars_path)
        sys.exit(0)
    else:
        # No imprimir nada si no se encuentra, el workflow maneja el caso
        sys.exit(1)
