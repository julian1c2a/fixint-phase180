#!/bin/bash
# =============================================================================
# dentro.sh - lo que corre `prueba.sh` DENTRO de un Ubuntu 24.04 limpio, como root
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Simula la maquina de medida recien instalada: un administrador con clave, el
# repositorio como «GitHub» (el arbol de trabajo empaquetado), y la preparacion
# y la sesion tal como se haran alli. Cada comprobacion dice [BIEN] o [MAL] y el
# resumen cuenta las dos. No se lanza a mano: lo lanza `prueba.sh`.
# =============================================================================
set -u
r() { echo; echo "##### $*"; }
ok=0; mal=0
espera() {  # espera <rc_esperado> <rc> <que>
    if [ "$2" = "$1" ]; then echo "  [BIEN] $3 (rc=$2)"; ok=$((ok+1)); else echo "  [MAL]  $3 (rc=$2, se esperaba $1)"; mal=$((mal+1)); fi
}

r "el origen: el arbol de trabajo, como repositorio"
apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq git ca-certificates >/dev/null
mkdir /src && tar -xf /arbol.tar -C /src && cd /src
git init -q -b phase-1.90 && git -c user.email=p@p -c user.name=p add -A && git -c user.email=p@p -c user.name=p commit -qm prueba
git config --system --add safe.directory '*'
cd /

r "un administrador con clave, como en la maquina"
useradd -m -s /bin/bash admin
mkdir -p /home/admin/.ssh && echo "ssh-ed25519 AAAAadmin admin@portatil" > /home/admin/.ssh/authorized_keys
export REPO_URL=file:///src RAMA=phase-1.90 SUDO_USER=admin
P=/src/scripts/condiciones_benchmark/prepara_ubuntu.sh
CLAVE="ssh-ed25519 AAAAC3NzaPRUEBA claude@portatil"

r "--comprobar NO toca nada"
bash $P --comprobar; espera 0 $? "--comprobar"
id bench >/dev/null 2>&1; espera 1 $? "tras --comprobar, bench NO existe"

r "sin clave ni --comprobar: se niega"
bash $P >/dev/null 2>&1; espera 2 $? "sin clave"
r "una clave que no es una clave: se niega"
bash $P "hola" >/dev/null 2>&1; espera 2 $? "clave falsa"

r "PRIMERA PASADA con un administrador SIN clave: NO puede quitar la contrasena de SSH"
useradd -m -s /bin/bash sinclave
SUDO_USER=sinclave bash $P "$CLAVE" > /tmp/primera.txt; espera 0 $? "preparacion con administrador sin clave"
grep -A3 '== 5' /tmp/primera.txt
[ ! -e /etc/ssh/sshd_config.d/10-banco.conf ]; espera 0 $? "NO se ha escrito la configuracion de SSH"

r "sshd sin poder validar (contenedor: sin claves de servidor ni /run/sshd): lo dice y no toca"
bash $P "$CLAVE" > /tmp/sinbase.txt; espera 0 $? "preparacion con sshd que no valida"
grep -A2 '== 5' /tmp/sinbase.txt
[ ! -e /etc/ssh/sshd_config.d/10-banco.conf ]; espera 0 $? "...y no ha escrito la configuracion"

r "como en la maquina de verdad: claves de servidor y /run/sshd"
mkdir -p /run/sshd && ssh-keygen -A >/dev/null
r "LA PREPARACION"
bash $P "$CLAVE"; espera 0 $? "preparacion"
r "OTRA VEZ: tiene que poder repetirse"
bash $P "$CLAVE" > /tmp/segunda.txt; espera 0 $? "segunda preparacion"
[ "$(grep -c PRUEBA /home/bench/.ssh/authorized_keys)" = 1 ]; espera 0 $? "la clave, UNA vez tras dos pasadas"

r "lo que tiene que haber quedado"
[ "$(stat -c '%U %a' /usr/local/sbin/cpu_de_medida)" = "root 755" ]; espera 0 $? "cpu_de_medida es root 755"
[ "$(stat -c '%U %a' /etc/sudoers.d/bench-cpu-de-medida)" = "root 440" ]; espera 0 $? "sudoers root 440"
visudo -c >/dev/null; espera 0 $? "visudo -c"
[ "$(getent shadow bench | cut -d: -f2)" = '*' ]; espera 0 $? "bench sin contrasena valida ('*', no '!')"
sshd -T 2>/dev/null | grep -qx 'passwordauthentication no'; espera 0 $? "sshd: passwordauthentication no"
sshd -T 2>/dev/null | grep -qx 'permitrootlogin no'; espera 0 $? "sshd: permitrootlogin no"

r "el sudo de bench: SOLO la CPU"
su - bench -c "sudo -n /bin/true" >/dev/null 2>&1; espera 1 $? "bench NO puede sudo /bin/true"
su - bench -c "sudo -n /usr/local/sbin/cpu_de_medida estado" >/dev/null 2>&1; espera 1 $? "bench NO puede sudo ... estado"
su - bench -c "sudo -n /usr/local/sbin/cpu_de_medida fija" > /tmp/fija.txt 2>&1; espera 1 $? "sudo fija se permite, y en un contenedor NO coge"
grep -q "NO ESTA LISTA" /tmp/fija.txt; espera 0 $? "...y lo dice"
su - bench -c "sudo -n /usr/local/sbin/cpu_de_medida restaura" >/dev/null 2>&1; espera 0 $? "sudo restaura se permite"

r "SESION sin CPU falsa: no se puede fijar, NO MIDE, y devuelve"
su - bench -c "fixint-phase180/scripts/condiciones_benchmark/sesion_ubuntu.sh phase-1.90 --only nucleos" > /tmp/s1.txt 2>&1
espera 1 $? "sesion con la CPU de verdad del contenedor"
grep -q "NO SE MIDE" /tmp/s1.txt; espera 0 $? "...dice NO SE MIDE"
grep -q "devolviendo la CPU" /tmp/s1.txt; espera 0 $? "...y devuelve la CPU"

r "SESION con CPU falsa y una suite: de punta a punta"
cp /cpu_falsa.sh /tmp/cpu_falsa.sh; chmod 755 /tmp/cpu_falsa.sh
su - bench -c "CPU_DE_MEDIDA='bash /tmp/cpu_falsa.sh' fixint-phase180/scripts/condiciones_benchmark/sesion_ubuntu.sh phase-1.90 --only nucleos --espera-ocioso 3 --espera-max 15" > /tmp/s2.txt 2>&1
espera 0 $? "sesion de nucleos"
tail -25 /tmp/s2.txt
ls /home/bench/resultados/benchs/history/*/*.json >/dev/null 2>&1; espera 0 $? "la toma esta en ~/resultados"
[ -z "$(su - bench -c 'git -C fixint-phase180 ls-files --others --exclude-standard -- benchs/history')" ]; espera 0 $? "y no queda en el arbol"
J=$(ls /home/bench/resultados/benchs/history/*/*.json | head -1)
python3 -c "import json,sys; d=json.load(open('$J')); print('  protocolo', d.get('protocolo'), '| orden', d.get('orden'), '| compilador', d.get('compilador'))"
python3 -c "import json,sys; d=json.load(open('$J')); sys.exit(0 if 'g++-14' in d.get('compilador','') or '14.' in d.get('compilador','') else 1)"; espera 0 $? "medida con g++-14"

r "E1, sin tocar la CPU"
su - bench -c "cd fixint-phase180/scripts/condiciones_benchmark/experimentos && python3 e1_muestreador.py --sin-cpu" > /tmp/e1.txt 2>&1
espera 0 $? "E1"; tail -9 /tmp/e1.txt

r "E2 --rapido, con CPU falsa: la fontaneria"
su - bench -c "cd fixint-phase180/scripts/condiciones_benchmark/experimentos && CPU_DE_MEDIDA='bash /tmp/cpu_falsa.sh' python3 e2_turbo.py --rapido --pares 1" > /tmp/e2.txt 2>&1
espera 0 $? "E2 --rapido"; tail -14 /tmp/e2.txt

r "E3 con nucleos, un par, sin tocar la CPU"
su - bench -c "cd fixint-phase180/scripts/condiciones_benchmark/experimentos && GCC_CXX=g++-14 python3 e3_orden.py --suite nucleos --pares 1 --sin-cpu" > /tmp/e3.txt 2>&1
espera 0 $? "E3"; tail -10 /tmp/e3.txt

echo; echo "##### RESUMEN: $ok bien, $mal mal"
[ "$mal" = 0 ]
