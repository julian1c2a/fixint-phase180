#!/bin/bash
# =============================================================================
# entrypoint-riscv32.sh
# Registra qemu-riscv32-static como intérprete binfmt para ELF rv32
# y luego lanza el comando recibido.
#
# Requiere --privileged (acceso a /proc/sys/fs/binfmt_misc).
# =============================================================================
set -e

BINFMT_REG=/proc/sys/fs/binfmt_misc/register

if [ -w "$BINFMT_REG" ]; then
    # Magic bytes para ELF riscv32 little-endian:
    #   EI_CLASS=1 (32-bit), EI_DATA=1 (LE), e_machine=0x00F3 (RISC-V)
    echo ':qemu-riscv32:M::\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xf3\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-riscv32-static:F' \
        > "$BINFMT_REG" 2>/dev/null || true
else
    echo "[WARN] binfmt_misc no accesible — ejecuta con --privileged" >&2
    echo "[WARN] Los binarios rv32 no se podrán ejecutar directamente." >&2
fi

exec "$@"
