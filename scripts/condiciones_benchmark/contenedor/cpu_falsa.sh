#!/bin/bash
# SPDX-License-Identifier: BSL-1.0
#
# cpu_de_medida FALSO para las pruebas en contenedor: recuerda el modo y lo cuenta.
# Se usa con `CPU_DE_MEDIDA="bash cpu_falsa.sh"`, que los guiones ejecutan SIN
# sudo. Un experimento corrido con el no dice nada de la CPU: solo prueba la
# fontaneria.
E=/tmp/cpu_falsa.estado
case "${1:-}" in
    fija) echo apagado > $E; echo "  [falsa] turbo apagado" ;;
    fija-con-turbo) echo encendido > $E; echo "  [falsa] turbo encendido" ;;
    restaura) rm -f $E; echo "  [falsa] restaurada" ;;
    estado) echo "  turbo        $(cat $E 2>/dev/null || echo encendido)" ;;
    *) exit 2 ;;
esac
