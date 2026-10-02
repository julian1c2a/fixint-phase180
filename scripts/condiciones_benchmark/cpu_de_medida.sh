#!/usr/bin/env bash
# =============================================================================
# cpu_de_medida.sh - fija la CPU para medir, y la devuelve como estaba
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Uso (en la plataforma de medida dedicada, Linux):
#
#   sudo cpu_de_medida.sh fija       turbo apagado, gobernador performance,
#                                    SMT apagado (si lo hay)
#   sudo cpu_de_medida.sh restaura   vuelve a como estaba antes de `fija`
#        cpu_de_medida.sh estado     lo que hay ahora (no necesita root)
#
# Por que existe
# --------------
# Medido el 1 oct 2026 (P2.22): un solo proceso compitiendo infla las cifras un
# 17 % porque le quita turbo al nucleo, y el TSC va a frecuencia fija. Con el turbo
# APAGADO y el gobernador en `performance` el nucleo va a frecuencia fija como el
# TSC, y «ciclos por operacion» son ciclos de verdad. Decidido con el autor el
# 2 oct: la serie de referencia se mide asi.
#
# Es la UNICA pieza del arnes que corre como root, y por eso:
#
#   - VERIFICA cada ajuste leyendolo de vuelta. Si alguno no cogio, sale con
#     error y lo dice: un turbo que no se pudo apagar no puede pasar por apagado.
#   - Si `fija` se llama dos veces, CONSERVA EL ESTADO ORIGINAL, no el ya fijado:
#     si no, `restaura` devolveria la maquina a la configuracion de medida.
#   - RESTAURA EN ORDEN: primero el SMT, porque al encenderlo reaparecen las CPU
#     gemelas, y luego hay que devolverles el gobernador.
#   - COMO ROOT, IGNORA CUALQUIER RAIZ ALTERNATIVA. `BENCH_SYSFS` y
#     `BENCH_ESTADO` existen solo para las pruebas (un sysfs falso); como root no
#     se aceptan, para que este guion no sea una via de escritura como root en
#     rutas arbitrarias.
# =============================================================================

set -u

if [ "$(id -u)" -eq 0 ]; then
    SYSFS=/sys
    ESTADO=/run/cpu_de_medida.estado
else
    SYSFS="${BENCH_SYSFS:-/sys}"
    ESTADO="${BENCH_ESTADO:-/run/cpu_de_medida.estado}"
fi
CPU="$SYSFS/devices/system/cpu"

lee() {
    # El contenido sin espacios, o nada si no se puede leer.
    [ -r "$1" ] && tr -d '[:space:]' < "$1" 2>/dev/null
}

escribe() {
    printf '%s' "$2" > "$1" 2>/dev/null
}

turbo_ahora() {
    # Logica AL REVES segun el sitio: `no_turbo` 1 es apagado; `boost` 0 es apagado.
    local v
    v=$(lee "$CPU/intel_pstate/no_turbo")
    if [ "$v" = 1 ]; then echo apagado; return; fi
    if [ "$v" = 0 ]; then echo encendido; return; fi
    v=$(lee "$CPU/cpufreq/boost")
    if [ "$v" = 0 ]; then echo apagado; return; fi
    if [ "$v" = 1 ]; then echo encendido; return; fi
    echo "¿?"
}

gobernadores() {
    local g
    for g in "$CPU"/cpu[0-9]*/cpufreq/scaling_governor; do
        [ -e "$g" ] && lee "$g" && echo
    done | sort -u | tr '\n' ' '
}

estado() {
    echo "  driver       $(lee "$CPU/cpu0/cpufreq/scaling_driver")"
    echo "  turbo        $(turbo_ahora)"
    echo "  gobernador   $(gobernadores)"
    echo "  SMT          $(lee "$CPU/smt/control")"
    echo "  en linea     $(lee "$CPU/online")"
}

fija() {
    local fallos=0 g smt

    # 1. EL ESTADO ORIGINAL, y solo si no hay ya uno guardado.
    if [ ! -f "$ESTADO" ]; then
        {
            echo "no_turbo=$(lee "$CPU/intel_pstate/no_turbo")"
            echo "boost=$(lee "$CPU/cpufreq/boost")"
            echo "smt=$(lee "$CPU/smt/control")"
            for g in "$CPU"/cpu[0-9]*/cpufreq/scaling_governor; do
                [ -e "$g" ] && echo "gob:$g=$(lee "$g")"
            done
            for g in "$CPU"/cpu[0-9]*/cpufreq/energy_performance_preference; do
                [ -e "$g" ] && echo "epp:$g=$(lee "$g")"
            done
            # EL ESTADO DEL GRUPO TIENE QUE SER EL DE LA ESCRITURA. Sin este `true`
            # era el del ultimo `[ -e ]`, y en una CPU sin EPP --AMD, o Intel sin
            # HWP-- el `||` de abajo saltaba diciendo que no se pudo guardar el
            # estado, cuando se habia guardado bien. Lo cazaron las pruebas.
            true
        } > "$ESTADO" || { echo "FALLA: no se pudo guardar el estado en $ESTADO"; exit 1; }
    fi

    # 2. SMT, si lo hay. Antes que el gobernador: al apagarlo desaparecen las
    #    CPU gemelas, y no tiene sentido fijarles nada.
    smt=$(lee "$CPU/smt/control")
    if [ "$smt" = on ]; then
        escribe "$CPU/smt/control" off
        [ "$(lee "$CPU/smt/control")" = off ] \
            || { echo "FALLA: no se pudo apagar el SMT"; fallos=$((fallos + 1)); }
    fi

    # 3. EL TURBO, por el sitio que tenga esta CPU.
    if [ -e "$CPU/intel_pstate/no_turbo" ]; then
        escribe "$CPU/intel_pstate/no_turbo" 1
    elif [ -e "$CPU/cpufreq/boost" ]; then
        escribe "$CPU/cpufreq/boost" 0
    fi
    [ "$(turbo_ahora)" = apagado ] \
        || { echo "FALLA: el turbo no esta apagado (ahora: $(turbo_ahora))"; fallos=$((fallos + 1)); }

    # 4. EL GOBERNADOR en todas las CPU que queden en linea. La preferencia de
    #    energia (EPP) no se toca: con `performance` intel_pstate la fija solo, y
    #    escribirla a mano puede fallar. Se guarda para restaurarla.
    local alguno=0
    for g in "$CPU"/cpu[0-9]*/cpufreq/scaling_governor; do
        [ -e "$g" ] || continue
        alguno=1
        escribe "$g" performance
        [ "$(lee "$g")" = performance ] \
            || { echo "FALLA: el gobernador de ${g#$CPU/} no es performance"; fallos=$((fallos + 1)); }
    done
    [ "$alguno" = 1 ] || { echo "FALLA: no hay gobernadores que fijar"; fallos=$((fallos + 1)); }

    estado
    if [ "$fallos" -ne 0 ]; then
        echo "$fallos ajuste(s) no se pudieron aplicar: ESTA MAQUINA NO ESTA LISTA PARA MEDIR"
        exit 1
    fi
    echo "  lista para medir"
}

restaura() {
    local linea dato ruta valor
    if [ ! -f "$ESTADO" ]; then
        echo "  no hay estado guardado: nada que restaurar"
        return 0
    fi
    # EN ORDEN. Primero el SMT: al encenderlo reaparecen las CPU gemelas, y sus
    # gobernadores solo se pueden devolver despues.
    [ "$(sed -n 's/^smt=//p' "$ESTADO")" = on ] && escribe "$CPU/smt/control" on
    valor=$(sed -n 's/^no_turbo=//p' "$ESTADO")
    [ -n "$valor" ] && escribe "$CPU/intel_pstate/no_turbo" "$valor"
    valor=$(sed -n 's/^boost=//p' "$ESTADO")
    [ -n "$valor" ] && escribe "$CPU/cpufreq/boost" "$valor"
    # Los gobernadores antes que la EPP: con `performance` la EPP no se deja tocar.
    for clase in gob epp; do
        while IFS= read -r linea; do
            case "$linea" in
                "$clase":*)
                    dato=${linea#"$clase":}
                    ruta=${dato%%=*}
                    valor=${dato#*=}
                    [ -e "$ruta" ] && [ -n "$valor" ] && escribe "$ruta" "$valor"
                    ;;
            esac
        done < "$ESTADO"
    done
    rm -f "$ESTADO"
    estado
    echo "  restaurada"
}

case "${1:-}" in
    fija) fija ;;
    restaura) restaura ;;
    estado) estado ;;
    *)
        echo "uso: $0 fija|restaura|estado" >&2
        exit 2
        ;;
esac
