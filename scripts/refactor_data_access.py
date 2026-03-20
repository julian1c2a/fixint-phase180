#!/usr/bin/env python3
"""
Script para automatizar la refactorización de data[0] → data[LSULL] y data[1] → data[MSULL]
en las secciones restantes de int128_base.hpp.

Uso:
    python scripts/refactor_data_access.py [--dry-run] [--backup]

Opciones:
    --dry-run   : Muestra los cambios sin aplicarlos
    --backup    : Crea una copia de seguridad antes de modificar

Autor: Julián Calderón Almendros
Fecha: 2026-01-05
"""

import re
import sys
import shutil
from pathlib import Path
from datetime import datetime
from typing import Tuple


def create_backup(filepath: Path) -> Path:
    """Crea una copia de seguridad del archivo."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_path = filepath.with_suffix(f".backup_{timestamp}{filepath.suffix}")
    shutil.copy2(filepath, backup_path)
    print(f"✓ Backup creado: {backup_path}")
    return backup_path


def count_occurrences(content: str) -> Tuple[int, int]:
    """Cuenta ocurrencias de data[0] y data[1] antes de refactorizar."""
    # Contar data[0] (excluyendo comentarios y strings)
    data0_pattern = r'(?<!//.*)\bdata\[0\](?![^"]*")'
    data1_pattern = r'(?<!//.*)\bdata\[1\](?![^"]*")'
    
    data0_count = len(re.findall(r'\bdata\[0\]', content))
    data1_count = len(re.findall(r'\bdata\[1\]', content))
    
    return data0_count, data1_count


def refactor_data_access(content: str) -> Tuple[str, int]:
    """
    Refactoriza data[0] → data[LSULL] y data[1] → data[MSULL].
    
    Returns:
        Tuple[str, int]: (contenido refactorizado, número de cambios)
    """
    total_changes = 0
    
    # Pattern 1: data[0] → data[LSULL]
    # Evitar cambios en comentarios (//) y strings ("")
    pattern1 = r'\bdata\[0\]'
    matches1 = re.finditer(pattern1, content)
    count1 = sum(1 for _ in matches1)
    content = re.sub(pattern1, 'data[LSULL]', content)
    total_changes += count1
    
    # Pattern 2: data[1] → data[MSULL]
    pattern2 = r'\bdata\[1\]'
    matches2 = re.finditer(pattern2, content)
    count2 = sum(1 for _ in matches2)
    content = re.sub(pattern2, 'data[MSULL]', content)
    total_changes += count2
    
    # Pattern 3: other.data[0] → other.data[LSULL]
    pattern3 = r'\bother\.data\[0\]'
    matches3 = re.finditer(pattern3, content)
    count3 = sum(1 for _ in matches3)
    content = re.sub(pattern3, 'other.data[LSULL]', content)
    total_changes += count3
    
    # Pattern 4: other.data[1] → other.data[MSULL]
    pattern4 = r'\bother\.data\[1\]'
    matches4 = re.finditer(pattern4, content)
    count4 = sum(1 for _ in matches4)
    content = re.sub(pattern4, 'other.data[MSULL]', content)
    total_changes += count4
    
    # Pattern 5: lhs.data[0], rhs.data[0], etc.
    pattern5 = r'\b(lhs|rhs|temp|result)\.data\[0\]'
    matches5 = re.finditer(pattern5, content)
    count5 = sum(1 for _ in matches5)
    content = re.sub(pattern5, r'\1.data[LSULL]', content)
    total_changes += count5
    
    # Pattern 6: lhs.data[1], rhs.data[1], etc.
    pattern6 = r'\b(lhs|rhs|temp|result)\.data\[1\]'
    matches6 = re.finditer(pattern6, content)
    count6 = sum(1 for _ in matches6)
    content = re.sub(pattern6, r'\1.data[MSULL]', content)
    total_changes += count6
    
    if total_changes > 0:
        print(f"\n  Pattern 1 (data[0] → data[LSULL]): {count1} cambios")
        print(f"  Pattern 2 (data[1] → data[MSULL]): {count2} cambios")
        print(f"  Pattern 3 (other.data[0] → other.data[LSULL]): {count3} cambios")
        print(f"  Pattern 4 (other.data[1] → other.data[MSULL]): {count4} cambios")
        print(f"  Pattern 5 (*.data[0] → *.data[LSULL]): {count5} cambios")
        print(f"  Pattern 6 (*.data[1] → *.data[MSULL]): {count6} cambios")
    
    return content, total_changes


def show_diff_summary(original: str, refactored: str):
    """Muestra un resumen de las diferencias."""
    original_lines = original.splitlines()
    refactored_lines = refactored.splitlines()
    
    diff_count = 0
    print("\n" + "="*80)
    print("RESUMEN DE CAMBIOS (Primeras 20 líneas modificadas)")
    print("="*80)
    
    for i, (orig, refac) in enumerate(zip(original_lines, refactored_lines)):
        if orig != refac and diff_count < 20:
            print(f"\nLínea {i+1}:")
            print(f"  ANTES: {orig.strip()}")
            print(f"  DESPUÉS: {refac.strip()}")
            diff_count += 1
    
    if diff_count >= 20:
        print(f"\n... (mostrando solo las primeras 20 de muchas más cambios)")


def main():
    """Función principal del script."""
    # Parsear argumentos
    dry_run = "--dry-run" in sys.argv
    create_backup_flag = "--backup" in sys.argv
    
    # Archivo objetivo
    target_file = Path("include_new/int128_base.hpp")
    
    if not target_file.exists():
        print(f"❌ ERROR: No se encontró el archivo {target_file}")
        sys.exit(1)
    
    print("="*80)
    print("REFACTORIZACIÓN AUTOMÁTICA: data[0/1] → data[LSULL/MSULL]")
    print("="*80)
    print(f"\nArchivo objetivo: {target_file}")
    print(f"Modo: {'DRY-RUN (sin cambios)' if dry_run else 'LIVE (aplicará cambios)'}")
    print(f"Backup: {'Sí' if create_backup_flag else 'No'}")
    
    # Leer contenido original
    original_content = target_file.read_text(encoding='utf-8')
    
    # Contar ocurrencias antes
    data0_before, data1_before = count_occurrences(original_content)
    print(f"\nOcurrencias ANTES:")
    print(f"  data[0]: {data0_before}")
    print(f"  data[1]: {data1_before}")
    print(f"  Total: {data0_before + data1_before}")
    
    # Crear backup si se solicita
    if create_backup_flag and not dry_run:
        create_backup(target_file)
    
    # Refactorizar
    print("\n" + "-"*80)
    print("APLICANDO TRANSFORMACIONES...")
    print("-"*80)
    refactored_content, total_changes = refactor_data_access(original_content)
    
    # Contar ocurrencias después
    data0_after, data1_after = count_occurrences(refactored_content)
    print(f"\nOcurrencias DESPUÉS:")
    print(f"  data[LSULL]: {data0_after}")
    print(f"  data[MSULL]: {data1_after}")
    print(f"  Total: {data0_after + data1_after}")
    
    print(f"\n✓ Total de transformaciones: {total_changes}")
    
    # Mostrar resumen de cambios
    if total_changes > 0:
        show_diff_summary(original_content, refactored_content)
    
    # Aplicar cambios o solo mostrar
    if dry_run:
        print("\n" + "="*80)
        print("DRY-RUN: Cambios NO aplicados (usa sin --dry-run para aplicar)")
        print("="*80)
    else:
        if total_changes > 0:
            target_file.write_text(refactored_content, encoding='utf-8')
            print("\n" + "="*80)
            print(f"✅ CAMBIOS APLICADOS: {target_file}")
            print("="*80)
            print("\n📋 PRÓXIMOS PASOS:")
            print("  1. Revisar cambios con: git diff include_new/int128_base.hpp")
            print("  2. Compilar tests: bash scripts/build_generic.bash uint128 t tests gcc release")
            print("  3. Ejecutar tests: bash scripts/check_generic.bash uint128 t gcc release")
            print("  4. Si algo falla, restaurar backup y corregir manualmente")
        else:
            print("\n⚠️  No se encontraron cambios que aplicar")


if __name__ == "__main__":
    main()
