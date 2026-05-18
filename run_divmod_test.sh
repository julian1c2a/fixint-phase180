#!/bin/bash
OUT=/project/test_divmod_output.txt
BINARY=/project/build/build_tests/gcc/release-O2/test_param_divmod_gcc
if [ -f "$BINARY" ]; then
    echo "Binary found" > "$OUT"
    "$BINARY" >> "$OUT" 2>&1
    echo "exit_code=$?" >> "$OUT"
else
    echo "Binary NOT found" > "$OUT"
    ls /project/build/ >> "$OUT" 2>&1 || echo "(missing)" >> "$OUT"
fi
