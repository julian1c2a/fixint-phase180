#!/usr/bin/env python3
"""
update_benchmarks_includes.py - Actualiza includes de benchmarks al template unificado

Reemplaza:
  - "uint128/uint128_*.hpp" -> "int128_base_*.hpp"
  - "int128/int128_*.hpp" -> "int128_base_*.hpp"
  - "../include/uint128/uint128_*.hpp" -> "int128_base_*.hpp"
  - "../include/int128/int128_*.hpp" -> "int128_base_*.hpp"

También actualiza:
  - using namespace uint128_* -> using namespace nstd
  - using namespace int128_* -> using namespace nstd
"""

import os
import re
from pathlib import Path

# Mapeo de headers viejos a nuevos
HEADER_MAP = {
    # Tipo principal
    'uint128_t.hpp': 'int128_base_tt.hpp',
    'int128_t.hpp': 'int128_base_tt.hpp',
    # Features
    'uint128_traits.hpp': 'int128_base_traits.hpp',
    'int128_traits.hpp': 'int128_base_traits.hpp',
    'uint128_limits.hpp': 'int128_base_limits.hpp',
    'int128_limits.hpp': 'int128_base_limits.hpp',
    'uint128_concepts.hpp': 'int128_base_concepts.hpp',
    'int128_concepts.hpp': 'int128_base_concepts.hpp',
    'uint128_algorithm.hpp': 'int128_base_algorithm.hpp',
    'int128_algorithm.hpp': 'int128_base_algorithm.hpp',
    'uint128_numeric.hpp': 'int128_base_numeric.hpp',
    'int128_numeric.hpp': 'int128_base_numeric.hpp',
    'uint128_bits.hpp': 'int128_base_bits.hpp',
    'int128_bits.hpp': 'int128_base_bits.hpp',
    'uint128_cmath.hpp': 'int128_base_cmath.hpp',
    'int128_cmath.hpp': 'int128_base_cmath.hpp',
    'uint128_iostreams.hpp': 'int128_base_iostreams.hpp',
    'int128_iostreams.hpp': 'int128_base_iostreams.hpp',
    'uint128_format.hpp': 'int128_base_format.hpp',
    'int128_format.hpp': 'int128_base_format.hpp',
    'uint128_ranges.hpp': 'int128_base_ranges.hpp',
    'int128_ranges.hpp': 'int128_base_ranges.hpp',
    'uint128_safe.hpp': 'int128_base_safe.hpp',
    'int128_safe.hpp': 'int128_base_safe.hpp',
    'uint128_thread_safety.hpp': 'int128_base_thread_safety.hpp',
    'int128_thread_safety.hpp': 'int128_base_thread_safety.hpp',
}

# Patrones de include a reemplazar
INCLUDE_PATTERNS = [
    # "uint128/uint128_*.hpp" -> "int128_base_*.hpp"
    (r'#include\s*["<](?:\.\./include/)?uint128/(uint128_\w+\.hpp)[">]', 
     lambda m: f'#include "{HEADER_MAP.get(m.group(1), m.group(1))}"'),
    # "int128/int128_*.hpp" -> "int128_base_*.hpp"  
    (r'#include\s*["<](?:\.\./include/)?int128/(int128_\w+\.hpp)[">]',
     lambda m: f'#include "{HEADER_MAP.get(m.group(1), m.group(1))}"'),
]

# Patrones de namespace a reemplazar
NAMESPACE_PATTERNS = [
    (r'using namespace uint128_\w+;', 'using namespace nstd;'),
    (r'using namespace int128_\w+;', 'using namespace nstd;'),
]

def update_file(filepath: Path) -> tuple[bool, list[str]]:
    """Actualiza un archivo. Retorna (modificado, lista_de_cambios)"""
    changes = []
    
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    original = content
    
    # Aplicar patrones de include
    for pattern, replacement in INCLUDE_PATTERNS:
        if callable(replacement):
            content = re.sub(pattern, replacement, content)
        else:
            content = re.sub(pattern, replacement, content)
    
    # Aplicar patrones de namespace
    for pattern, replacement in NAMESPACE_PATTERNS:
        content = re.sub(pattern, replacement, content)
    
    if content != original:
        with open(filepath, 'w', encoding='utf-8', newline='\n') as f:
            f.write(content)
        
        # Encontrar qué cambió
        for line_num, (old, new) in enumerate(zip(original.split('\n'), content.split('\n')), 1):
            if old != new:
                changes.append(f"  L{line_num}: {old.strip()} -> {new.strip()}")
        
        return True, changes
    
    return False, []

def main():
    benchs_dir = Path(__file__).parent.parent / 'benchs'
    
    if not benchs_dir.exists():
        print(f"Error: Directorio {benchs_dir} no existe")
        return 1
    
    print("=" * 60)
    print("  Actualizando includes de benchmarks al template unificado")
    print("=" * 60)
    print()
    
    modified_count = 0
    total_count = 0
    
    for cpp_file in sorted(benchs_dir.glob('*.cpp')):
        total_count += 1
        modified, changes = update_file(cpp_file)
        
        if modified:
            modified_count += 1
            print(f"[OK] {cpp_file.name}")
            for change in changes[:5]:  # Mostrar máximo 5 cambios
                print(change)
            if len(changes) > 5:
                print(f"  ... y {len(changes) - 5} cambios más")
            print()
        else:
            print(f"[--] {cpp_file.name} (sin cambios)")
    
    print()
    print("=" * 60)
    print(f"  Resumen: {modified_count}/{total_count} archivos modificados")
    print("=" * 60)
    
    return 0

if __name__ == '__main__':
    exit(main())
