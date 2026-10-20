#!/usr/bin/env bash
# =============================================================================
# prepara_ubuntu.sh - deja la maquina de medida lista. UNA vez, como root.
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Para la plataforma de medida dedicada (NEXT_STEPS, «La plataforma de medida
# dedicada»): un mini PC con Ubuntu Server 24.04 recien instalado. Lo corre el
# usuario ADMINISTRADOR de la maquina, no yo: es lo unico que necesita su sudo.
#
#   curl -fsSLO https://raw.githubusercontent.com/julian1c2a/fixint-phase180/phase-1.90/scripts/condiciones_benchmark/prepara_ubuntu.sh
#   less prepara_ubuntu.sh                              # LEERLO antes
#   sudo bash prepara_ubuntu.sh --comprobar             # que haria, sin tocar nada
#   sudo bash prepara_ubuntu.sh "ssh-ed25519 AAAA... claude@portatil"
#
# La clave publica que se pasa es la que usara el portatil para entrar como
# `bench`. Se puede repetir el guion sin dano: cada paso mira antes si ya esta.
#
# LO QUE HACE, EN ORDEN
#
#   1. Paquetes: g++-14, git, python3 y psutil, y las cabeceras de Boost, GMP y
#      TomMath, sin las que `benchmark_vs_builtin` no compila.
#   2. Usuario `bench`, SIN contrasena (solo entra con la clave dada).
#   3. El repositorio, clonado en ~bench desde GitHub (solo lectura).
#   4. `cpu_de_medida` en /usr/local/sbin, DE ROOT, y `sudo` para `bench` SOLO
#      para fijar y devolver la CPU. Se instala desde un clon propio de root, no
#      desde el de `bench`: si se copiara de alli, `bench` podria editarlo antes
#      de la siguiente preparacion y eso seria una puerta a root.
#   5. SSH sin contrasena y sin root, SOLO SI el administrador ya entra con
#      clave. Si no, se salta y lo dice: quitar la contrasena sin clave dejaria
#      la maquina sin forma de entrar.
#   6. Fuera lo que se despierta solo y mete ruido en las medidas: las
#      actualizaciones automaticas, snapd y otros temporizadores. Las
#      actualizaciones pasan a hacerse a mano, fuera de las sesiones.
#   7. Comprobacion final, y lo que hay que mirar del turbo en esta CPU.
#
# Variables para probarlo fuera de la maquina (contenedor): REPO_URL, RAMA.
# =============================================================================

set -u

REPO_URL="${REPO_URL:-https://github.com/julian1c2a/fixint-phase180.git}"
RAMA="${RAMA:-phase-1.90}"
USUARIO=bench
CASA="/home/$USUARIO"
CLON="$CASA/fixint-phase180"
CPU_SBIN=/usr/local/sbin/cpu_de_medida
SUDOERS=/etc/sudoers.d/bench-cpu-de-medida
SSHD_CONF=/etc/ssh/sshd_config.d/10-banco.conf   # antes que 50-cloud-init: gana
PAQUETES=(g++-14 git python3 python3-psutil libboost-dev libgmp-dev libtommath-dev
          ca-certificates sudo openssh-server)
# Lo que se despierta solo. Ninguno hace falta en una maquina que solo mide.
TEMPORIZADORES=(apt-daily.timer apt-daily-upgrade.timer unattended-upgrades.service
                motd-news.timer man-db.timer fwupd-refresh.timer
                snapd.service snapd.socket snapd.seeded.service)

COMPROBAR=0
CLAVE=""
for a in "$@"; do
    case "$a" in
        --comprobar) COMPROBAR=1 ;;
        -h | --help) sed -n '2,45p' "$0"; exit 0 ;;
        *) CLAVE="$a" ;;
    esac
done

FALLOS=0
paso() { printf '\n== %s\n' "$*"; }
bien() { printf '   ok    %s\n' "$*"; }
haria() { printf '   haria %s\n' "$*"; }
nota() { printf '   nota  %s\n' "$*"; }
falla() { printf '   FALLA %s\n' "$*"; FALLOS=$((FALLOS + 1)); }
hay_systemd() { [ -d /run/systemd/system ]; }

if [ "$(id -u)" -ne 0 ]; then
    echo "Hay que correrlo como root: sudo bash $0 ..." >&2
    exit 2
fi
if [ "$COMPROBAR" = 0 ] && [ -z "$CLAVE" ]; then
    echo "Falta la clave publica para \`$USUARIO\` (o --comprobar). Ver la cabecera." >&2
    exit 2
fi
if [ -n "$CLAVE" ]; then
    case "$CLAVE" in
        ssh-ed25519\ * | ssh-rsa\ * | ecdsa-sha2-*\ *) ;;
        *) echo "Eso no parece una clave publica de SSH: «${CLAVE:0:30}...»" >&2; exit 2 ;;
    esac
fi

# -----------------------------------------------------------------------------
paso "0. La maquina"
. /etc/os-release 2>/dev/null
echo "   ${PRETTY_NAME:-?}, $(uname -m), nucleo $(uname -r)"
[ "${VERSION_ID:-}" = "24.04" ] || nota "pensado para Ubuntu 24.04; esto es ${VERSION_ID:-?}"
[ "$(uname -m)" = x86_64 ] || nota "pensado para x86-64 (el TSC y el arnes); esto es $(uname -m)"

# -----------------------------------------------------------------------------
paso "1. Paquetes"
faltan=()
for p in "${PAQUETES[@]}"; do
    dpkg -s "$p" >/dev/null 2>&1 || faltan+=("$p")
done
if [ ${#faltan[@]} -eq 0 ]; then
    bien "estan todos"
elif [ "$COMPROBAR" = 1 ]; then
    haria "apt-get install ${faltan[*]}"
else
    export DEBIAN_FRONTEND=noninteractive
    if apt-get update -qq && apt-get install -y -qq "${faltan[@]}" >/dev/null; then
        bien "instalados: ${faltan[*]}"
    else
        falla "apt-get no pudo instalar: ${faltan[*]}"
    fi
fi

# -----------------------------------------------------------------------------
paso "2. Usuario $USUARIO"
if id "$USUARIO" >/dev/null 2>&1; then
    bien "existe"
elif [ "$COMPROBAR" = 1 ]; then
    haria "useradd $USUARIO, sin contrasena"
else
    useradd --create-home --shell /bin/bash "$USUARIO" && bien "creado" || falla "useradd"
fi
if [ "$COMPROBAR" = 0 ] && id "$USUARIO" >/dev/null 2>&1; then
    # `*` y no `!`: con `!` (cuenta bloqueada) OpenSSH puede negar tambien la
    # entrada con clave. Con `*` no hay contrasena valida y la clave entra.
    usermod -p '*' "$USUARIO"
    install -d -m 700 -o "$USUARIO" -g "$USUARIO" "$CASA/.ssh"
    touch "$CASA/.ssh/authorized_keys"
    if grep -qxF "$CLAVE" "$CASA/.ssh/authorized_keys"; then
        bien "la clave ya estaba"
    else
        printf '%s\n' "$CLAVE" >> "$CASA/.ssh/authorized_keys" && bien "clave anadida"
    fi
    chown "$USUARIO:$USUARIO" "$CASA/.ssh/authorized_keys"
    chmod 600 "$CASA/.ssh/authorized_keys"
fi

# -----------------------------------------------------------------------------
paso "3. El repositorio, en $CLON"
if [ -d "$CLON/.git" ]; then
    bien "ya esta clonado (las sesiones lo actualizan)"
elif [ "$COMPROBAR" = 1 ]; then
    haria "git clone $REPO_URL (rama $RAMA)"
else
    if sudo -u "$USUARIO" git clone -q --branch "$RAMA" "$REPO_URL" "$CLON"; then
        bien "clonado"
    else
        falla "git clone $REPO_URL"
    fi
fi

# -----------------------------------------------------------------------------
paso "4. cpu_de_medida, de root, y sudo solo para eso"
if [ "$COMPROBAR" = 1 ]; then
    [ -x "$CPU_SBIN" ] && bien "$CPU_SBIN esta" || haria "instalar $CPU_SBIN desde un clon de root"
    [ -f "$SUDOERS" ] && bien "$SUDOERS esta" || haria "escribir $SUDOERS"
else
    tmp=$(mktemp -d)
    if git clone -q --depth 1 --branch "$RAMA" "$REPO_URL" "$tmp/repo" 2>/dev/null; then
        install -o root -g root -m 0755 "$tmp/repo/scripts/condiciones_benchmark/cpu_de_medida.sh" "$CPU_SBIN"
        bien "$CPU_SBIN, del commit $(git -C "$tmp/repo" rev-parse --short HEAD)," \
            "sha256 $(sha256sum "$CPU_SBIN" | cut -c1-16)"
    else
        falla "no se pudo clonar para instalar cpu_de_medida"
    fi
    rm -rf "$tmp"
    cat > "$tmp.sudoers" <<EOF
# prepara_ubuntu.sh: \`$USUARIO\` solo puede fijar la CPU para medir y devolverla.
$USUARIO ALL=(root) NOPASSWD: $CPU_SBIN fija, $CPU_SBIN fija-con-turbo, $CPU_SBIN restaura
EOF
    if visudo -cf "$tmp.sudoers" >/dev/null; then
        install -o root -g root -m 0440 "$tmp.sudoers" "$SUDOERS" && bien "$SUDOERS"
    else
        falla "el sudoers generado no es valido; no se instala"
    fi
    rm -f "$tmp.sudoers"
fi

# -----------------------------------------------------------------------------
paso "5. SSH: sin contrasena y sin root"
ADMIN="${SUDO_USER:-}"
ADMIN_CASA=$(getent passwd "$ADMIN" 2>/dev/null | cut -d: -f6)
if [ -z "$ADMIN" ] || [ "$ADMIN" = root ]; then
    nota "no se sabe quien es el administrador (corre con sudo desde tu usuario): se salta"
elif [ ! -s "$ADMIN_CASA/.ssh/authorized_keys" ]; then
    nota "$ADMIN no tiene clave en ~/.ssh/authorized_keys: SE SALTA, para no dejarte fuera."
    nota "Desde el portatil (PowerShell), y luego vuelve a correr esto:"
    nota "  type \$env:USERPROFILE\\.ssh\\id_ed25519.pub | ssh $ADMIN@<ip> \"cat >> ~/.ssh/authorized_keys\""
elif [ "$COMPROBAR" = 1 ]; then
    haria "escribir $SSHD_CONF (PasswordAuthentication no, PermitRootLogin no)"
elif ! base=$(sshd -t 2>&1); then
    # «NO PUEDO COMPROBARLO» NO ES «ESTA MAL». Si sshd no valida ni sin el
    # fichero nuevo --sin claves de servidor, sin /run/sshd--, el fallo no es
    # de la configuracion, y escribirla a ciegas podria dejar a ssh sin arrancar.
    nota "sshd no valida ni SIN el fichero nuevo, asi que no se toca: ${base:0:100}"
else
    cat > "$SSHD_CONF" <<'EOF'
# prepara_ubuntu.sh: solo con clave, y nunca como root.
PasswordAuthentication no
KbdInteractiveAuthentication no
PermitRootLogin no
EOF
    if err=$(sshd -t 2>&1); then
        if hay_systemd; then systemctl reload ssh 2>/dev/null || systemctl reload sshd 2>/dev/null; fi
        bien "$SSHD_CONF ($(sshd -T 2>/dev/null | grep -i '^passwordauthentication'))"
    else
        rm -f "$SSHD_CONF"
        falla "sshd -t rechaza ESTE fichero, y se quita: ${err:0:100}"
    fi
fi

# -----------------------------------------------------------------------------
paso "6. Fuera lo que se despierta solo"
if ! hay_systemd; then
    nota "sin systemd (¿un contenedor?): no se tocan servicios"
else
    for u in "${TEMPORIZADORES[@]}"; do
        systemctl list-unit-files "$u" >/dev/null 2>&1 || continue
        [ -n "$(systemctl list-unit-files "$u" --no-legend 2>/dev/null)" ] || continue
        if [ "$(systemctl is-enabled "$u" 2>/dev/null)" = disabled ] || \
           [ "$(systemctl is-enabled "$u" 2>/dev/null)" = masked ]; then
            bien "$u ya esta apagado"
        elif [ "$COMPROBAR" = 1 ]; then
            haria "systemctl disable --now $u"
        else
            systemctl disable --now "$u" >/dev/null 2>&1 && bien "$u apagado" || falla "$u"
        fi
    done
    nota "las actualizaciones, a mano y fuera de las sesiones: sudo apt update && sudo apt upgrade"
    nota "para deshacerlo: sudo systemctl enable --now <unidad>"
fi

# -----------------------------------------------------------------------------
paso "7. Comprobacion final"
# Con --comprobar, lo que falta es lo que el paso 1 INSTALARIA: es una nota, no
# un fallo. Contarlo como fallo hacia salir con error a `--comprobar` en toda
# maquina recien instalada, que es justo donde se usa (cazado en un contenedor).
falta() { if [ "$COMPROBAR" = 1 ]; then nota "falta $* (lo instalaria el paso 1)"; else falla "$*"; fi; }
command -v g++-14 >/dev/null && bien "$(g++-14 --version | head -1)" || falta "g++-14"
python3 -c "import psutil" 2>/dev/null && bien "python3 con psutil" || falta "psutil para python3"
if [ "$COMPROBAR" = 0 ]; then
    [ "$(stat -c '%U %a' "$CPU_SBIN" 2>/dev/null)" = "root 755" ] && bien "$CPU_SBIN es de root" \
        || falla "$CPU_SBIN no es root 755"
    sudo -l -U "$USUARIO" 2>/dev/null | grep -q "$CPU_SBIN fija" && bien "$USUARIO puede fijar la CPU con sudo" \
        || falla "el sudo de $USUARIO no esta"
fi
CPU=/sys/devices/system/cpu
echo "   La CPU, para el primer dia:"
echo "     modelo     $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2- | sed 's/^ //')"
echo "     driver     $(cat $CPU/cpu0/cpufreq/scaling_driver 2>/dev/null || echo '¿?')"
[ -r $CPU/amd_pstate/status ] && echo "     amd_pstate $(cat $CPU/amd_pstate/status)"
if [ -e $CPU/intel_pstate/no_turbo ] || [ -e $CPU/cpufreq/boost ]; then
    bien "hay por donde apagar el turbo"
else
    nota "NO hay $CPU/cpufreq/boost ni intel_pstate/no_turbo: cpu_de_medida no podra"
    nota "apagar el turbo y se negara a medir. En AMD: probar el nucleo HWE"
    nota "(linux-generic-hwe-24.04) o amd_pstate=passive en la linea del nucleo."
fi

echo
if [ "$FALLOS" -ne 0 ]; then
    echo "$FALLOS paso(s) fallaron. La maquina NO esta lista."
    exit 1
fi
[ "$COMPROBAR" = 1 ] && echo "Comprobado. Nada se ha tocado." || echo "Lista para medir."
