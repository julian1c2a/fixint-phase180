#!/usr/bin/env python3
"""
Actualiza los nombres de funciones de API en los benchmarks
para coincidir con el template unificado int128_base_t.
"""

import os
import re
import glob

# Mapeo de nombres antiguos a nuevos
API_REPLACEMENTS = [
    # Funciones de string
    (r'\.to_string_base\((\d+)\)', r'.to_string(\1)'),  # .to_string_base(16) -> .to_string(16)
    (r'uint128_t::from_string\(', r'uint128_t('),        # uint128_t::from_string("...") -> uint128_t("...")
    (r'int128_t::from_string\(', r'int128_t('),          # int128_t::from_string("...") -> int128_t("...")
    (r'uint128_t::from_string_base\(([^,]+),\s*(\d+)\)', r'uint128_t(\1, \2)'),  # uint128_t::from_string_base("...", 16) -> uint128_t("...", 16)
    (r'int128_t::from_string_base\(([^,]+),\s*(\d+)\)', r'int128_t(\1, \2)'),    # int128_t::from_string_base("...", 16) -> int128_t("...", 16)
    
    # Funciones de bits renombradas
    (r'\bget_bit\(', r'test_bit('),                       # get_bit -> test_bit
    
    # Namespaces viejos
    (r'using namespace uint128_bits;', r'using namespace nstd;'),
    (r'using namespace int128_bits;', r'using namespace nstd;'),
    (r'using namespace uint128_algorithm;', r'using namespace nstd;'),
    (r'using namespace int128_algorithm;', r'using namespace nstd;'),
    (r'using namespace uint128_cmath;', r'using namespace nstd;'),
    (r'using namespace int128_cmath;', r'using namespace nstd;'),
    (r'using namespace uint128_numeric;', r'using namespace nstd;'),
    (r'using namespace int128_numeric;', r'using namespace nstd;'),
    
    # Prefijos de namespace en llamadas (ej: uint128_bits::rotl -> nstd::rotl)
    (r'\buint128_bits::', r'nstd::'),
    (r'\bint128_bits::', r'nstd::'),
    (r'\buint128_cmath::', r'nstd::'),
    (r'\bint128_cmath::', r'nstd::'),
    (r'\buint128_numeric::', r'nstd::'),
    (r'\bint128_numeric::', r'nstd::'),
]

def fix_file(filepath):
    """Aplica los reemplazos de API a un archivo."""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    
    original = content
    changes = []
    
    for pattern, replacement in API_REPLACEMENTS:
        new_content = re.sub(pattern, replacement, content)
        if new_content != content:
            matches = re.findall(pattern, content)
            if matches:
                changes.append(f"  {pattern} -> {replacement} ({len(matches)} matches)")
            content = new_content
    
    if content != original:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        return changes
    return None

def main():
    print("=" * 60)
    print("  Actualizando API en benchmarks al template unificado")
    print("=" * 60)
    print()
    
    benchs_dir = os.path.join(os.path.dirname(__file__), '..', 'benchs')
    files = glob.glob(os.path.join(benchs_dir, '*.cpp'))
    
    modified = 0
    for filepath in sorted(files):
        filename = os.path.basename(filepath)
        changes = fix_file(filepath)
        if changes:
            print(f"[OK] {filename}")
            for change in changes:
                print(change)
            print()
            modified += 1
        else:
            print(f"[ ] {filename} (sin cambios)")
    
    print()
    print("=" * 60)
    print(f"  Resumen: {modified}/{len(files)} archivos modificados")
    print("=" * 60)

if __name__ == '__main__':
    main()
