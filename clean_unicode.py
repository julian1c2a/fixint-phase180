#!/usr/bin/env python3
"""Clean up non-ASCII characters in test files"""

import os

# Files to clean
files = [
    "tests/test_divmod_final.cpp",
    "tests/test_divmod_debug.cpp",
    "tests/test_divmod_suite.cpp"
]

for filepath in files:
    if os.path.exists(filepath):
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Replace Unicode characters with ASCII
        original_len = len(content)
        content = content.replace('✓', '[OK]')
        content = content.replace('✗', '[FAIL]')
        content = content.replace('✅', '[PASS]')
        content = content.replace('❌', '[ERROR]')
        content = content.replace('⚠️', '[WARNING]')
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        
        print(f"[OK] Cleaned: {filepath}")
    else:
        print(f"[FAIL] Not found: {filepath}")

print("\nDone! All test files cleaned.")
