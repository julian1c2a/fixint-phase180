#!/usr/bin/env bash
# =============================================================================
# sesion_ubuntu.sh - una toma en la plataforma de medida dedicada, de principio
#                    a fin, y la CPU devuelta como estaba pase lo que pase
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Lo corre `bench` en la maquina de medida; normalmente lanzado desde el
# portatil, y DESPEGADO de la conexion para que un corte no la mate:
#
#   ssh bench@banco 'setsid nohup fixint-phase180/scripts/condiciones_benchmark/sesion_ubuntu.sh \
#                    <commit> > sesiones/ultima.log 2>&1 < /dev/null &'
#
#   sesion_ubuntu.sh <commit> [argumentos de bench_history.py ...]
#
# Ejemplos:   sesion_ubuntu.sh 31a5715                 la toma completa (~1,5 h)
#             sesion_ubuntu.sh 31a5715 --only nucleos  una suite
#
# LO QUE HACE
#
#   1. Trae el commit pedido y se coloca en el, SIN ramas: la toma es de un
#      commit, y un arbol con cambios no se puede atribuir a ninguno.
#   2. Comprueba lo que la toma necesita: g++-14 y psutil.
#   3. Fija la CPU (turbo apagado, `performance`, SMT apagado) con el
#      `cpu_de_medida` de root. Si no coge, NO MIDE.
#   4. Corre `bench_history.py` con g++-14, esperando antes a que haya calma.
#   5. Devuelve la CPU -- tambien si algo falla o se corta (trap).
#   6. Saca la toma del arbol a ~/resultados: si se quedara, el siguiente
#      checkout chocaria con ella cuando el portatil la suba al repositorio.
#
# PARA PROBARLO SIN LA MAQUINA: `CPU_DE_MEDIDA=<orden>` sustituye a
# `sudo -n /usr/local/sbin/cpu_de_medida` y se ejecuta sin sudo.
# =============================================================================

set -u

REPO="${REPO:-$HOME/fixint-phase180}"
RESULTADOS="${RESULTADOS:-$HOME/resultados}"
COMPILADOR="${GCC_CXX:-g++-14}"

if [ $# -lt 1 ] || [ "${1#-}" != "$1" ]; then
    sed -n '2,36p' "$0"
    exit 2
fi
COMMIT="$1"
shift

if [ -n "${CPU_DE_MEDIDA:-}" ]; then
    # shellcheck disable=SC2206
    CPU=($CPU_DE_MEDIDA)
    echo "[OJO] CPU_DE_MEDIDA=$CPU_DE_MEDIDA: la CPU NO se fija de verdad; esto es una prueba."
else
    CPU=(sudo -n /usr/local/sbin/cpu_de_medida)
fi

inicio=$(date +%s)
echo "== sesion de medida, $(date '+%Y-%m-%d %H:%M'), en $(hostname)"

# 1. EL COMMIT -----------------------------------------------------------------
cd "$REPO" || { echo "FALLA: no existe $REPO"; exit 1; }
if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    echo "FALLA: el arbol de $REPO tiene cambios; una toma asi no es de ningun commit."
    git status --short --untracked-files=no | head
    exit 1
fi
git fetch -q origin || { echo "FALLA: git fetch"; exit 1; }
git checkout -q --detach "$COMMIT" 2>/dev/null \
    || git checkout -q --detach "origin/$COMMIT" 2>/dev/null \
    || { echo "FALLA: no encuentro el commit $COMMIT"; exit 1; }
echo "   commit $(git rev-parse --short HEAD): $(git log -1 --format=%s | cut -c1-70)"

# 2. LO QUE HACE FALTA ---------------------------------------------------------
command -v "$COMPILADOR" >/dev/null || { echo "FALLA: no hay $COMPILADOR"; exit 1; }
python3 -c "import psutil" 2>/dev/null \
    || { echo "FALLA: sin psutil la toma no puede certificarse (P2.22)"; exit 1; }

# 3 y 5. LA CPU, Y DEVOLVERLA SIEMPRE ------------------------------------------
devuelve() {
    echo "== devolviendo la CPU"
    "${CPU[@]}" restaura || echo "[OJO] restaura fallo: mirar la maquina"
}
trap devuelve EXIT
trap 'echo "[OJO] sesion cortada"; exit 130' INT TERM HUP

echo "== fijando la CPU"
if ! "${CPU[@]}" fija; then
    echo "FALLA: la CPU no se pudo fijar. NO SE MIDE."
    exit 1
fi

# 4. LA TOMA -------------------------------------------------------------------
echo "== midiendo con $COMPILADOR"
export GCC_CXX="$COMPILADOR"
python3 scripts/bench_history.py --compiler gcc --mode release-O2 \
    --espera-ocioso 60 --espera-max 1800 "$@"
rc=$?

# 6. LA TOMA, FUERA DEL ARBOL --------------------------------------------------
mkdir -p "$RESULTADOS"
movidas=0
while IFS= read -r f; do
    [ -n "$f" ] || continue
    mkdir -p "$RESULTADOS/$(dirname "$f")"
    mv "$f" "$RESULTADOS/$f" && movidas=$((movidas + 1)) && echo "   toma: $RESULTADOS/$f"
done < <(git ls-files --others --exclude-standard -- benchs/history)
[ "$movidas" -gt 0 ] || echo "[OJO] no salio ninguna toma nueva"

echo "== fin: rc=$rc, $(( ($(date +%s) - inicio) / 60 )) min"
exit "$rc"
