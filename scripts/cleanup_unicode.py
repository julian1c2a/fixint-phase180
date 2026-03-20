#!/usr/bin/env python3
"""
Script para limpiar caracteres Unicode de archivos .cpp
Reemplaza símbolos Unicode por equivalentes ASCII
"""

import os
import sys
from pathlib import Path

# Mapeo de caracteres Unicode a ASCII
REPLACEMENTS = {
    '✓': '[OK]',
    '✗': '[ERROR]',
    '➜': '->',
    '═': '=',
    '─': '-',
    '│': '|',
    '🔨': '[BUILD]',
    '🧪': '[TEST]',
    '⚡': '[RUN]',
    '📊': '[INFO]',
    '✅': '[OK]',
    '❌': '[FAIL]',
    '⚠️': '[WARN]',
    '"': '"',
    '"': '"',
    ''': "'",
    ''': "'",
    '…': '...',
    '–': '-',
    '—': '-',
    '•': '*',
}

def clean_file(filepath):
    """Limpia caracteres Unicode de un archivo"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original = content
        for unicode_char, ascii_char in REPLACEMENTS.items():
            content = content.replace(unicode_char, ascii_char)
        
        if content != original:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            return True
        return False
    except Exception as e:
        print(f"Error procesando {filepath}: {e}", file=sys.stderr)
        return False

def main():
    """Procesa todos los archivos .cpp en tests/ y benchs/"""
    project_root = Path(__file__).parent.parent
    
    directories = ['tests', 'benchs']
    total_files = 0
    modified_files = 0
    
    print("Limpiando caracteres Unicode de archivos .cpp...\n")
    
    for directory in directories:
        dir_path = project_root / directory
        if not dir_path.exists():
            print(f"Directorio no encontrado: {dir_path}")
            continue
        
        for cpp_file in dir_path.rglob('*.cpp'):
            total_files += 1
            if clean_file(cpp_file):
                modified_files += 1
                print(f"[OK] {cpp_file.name}")
    
    print(f"\nArchivos procesados: {total_files}")
    print(f"Archivos modificados: {modified_files}")
    return 0 if modified_files >= 0 else 1

if __name__ == "__main__":
    sys.exit(main())
