#!/usr/bin/env bash
# =============================================================================
# prueba.sh - la preparacion y la sesion de la maquina de medida, en un
#             Ubuntu 24.04 LIMPIO, sin tener la maquina
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Uso, en Linux con Docker (WSL vale):   bash prueba.sh
#
# Empaqueta el ARBOL DE TRABAJO tal como esta --sin hacer commit-- y lo usa como
# «GitHub» dentro de un contenedor `ubuntu:24.04` recien bajado. Alli `dentro.sh`
# corre `prepara_ubuntu.sh` (repetida, con un administrador sin clave, con un
# sshd que no puede validar y con uno que si), comprueba permisos, sudo y SSH,
# lanza una sesion que no puede fijar la CPU (y no debe medir) y otra de punta a
# punta con `cpu_falsa.sh`, y los tres experimentos en modo fontaneria.
#
# Lo que NO puede probar: la CPU de verdad (turbo, gobernador, SMT), systemd y
# las cifras de los experimentos. Eso, en la maquina.
#
# Escrito el 20 oct 2026, cuando la maquina aun no tenia Ubuntu. Cazo seis fallos
# en los guiones antes de que llegaran a ella.
# =============================================================================

set -u
AQUI="$(cd "$(dirname "$0")" && pwd)"
RAIZ="$(git -C "$AQUI" rev-parse --show-toplevel)" || exit 9
cd "$RAIZ" || exit 9

# Lo que git conoce --seguido, o nuevo sin ignorar--, menos lo que no es del
# proyecto: un clon dentro del arbol, si lo hay.
TAR=$(mktemp --suffix=.tar)
git ls-files -co --exclude-standard -z -- . ':!fixint-phase180/' | tar --null -T - -cf "$TAR"
echo "arbol de trabajo: $(du -h "$TAR" | cut -f1)"

# Una configuracion de Docker vacia y temporal: la de un WSL con Docker Desktop
# apunta a `docker-credential-desktop.exe`, que desde aqui no se encuentra. Para
# bajar una imagen publica no hace falta, y asi no se toca la del usuario.
export DOCKER_CONFIG=$(mktemp -d)
echo '{}' > "$DOCKER_CONFIG/config.json"

docker run --rm -v "$TAR":/arbol.tar:ro -v "$AQUI/dentro.sh":/dentro.sh:ro \
    -v "$AQUI/cpu_falsa.sh":/cpu_falsa.sh:ro ubuntu:24.04 bash /dentro.sh
rc=$?
rm -rf "$TAR" "$DOCKER_CONFIG"
echo "rc=$rc"
exit $rc
