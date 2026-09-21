# =============================================================================
# arregla_path_windows.ps1 - Pone el PATH de MAQUINA en el orden correcto
# =============================================================================
#
# Part of int128 Library
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julian Calderon Almendros
#
# HACE FALTA EJECUTARLO COMO ADMINISTRADOR.
#
# QUE ARREGLA, Y POR QUE
# ----------------------
# El PATH efectivo de Windows es PATH_de_MAQUINA + PATH_de_USUARIO, en ese
# orden. En esta maquina el de MAQUINA trae, por este orden:
#
#     ...  C:\msys64\usr\bin        <- entorno POSIX de MSYS2
#          C:\msys64\ucrt64\bin
#          C:\msys64\clang64\bin
#     ...
#          C:\Program Files\CMake\bin                   <- PENULTIMO
#          C:\Program Files\Docker\Docker\resources\bin <- ULTIMO
#
# Consecuencias medidas el 21 sep 2026:
#
#   `cmake`  -> C:\msys64\usr\bin\cmake.exe, que es el POSIX (enlazado contra
#               msys-2.0.dll) y **FALLA al configurar este proyecto**:
#               «Could NOT find Threads (missing: Threads_FOUND)», exit 1.
#               El nativo de Program Files configura bien, exit 0.
#
#   `docker` -> C:\msys64\ucrt64\bin\docker, que **no es un ejecutable**: es un
#               script de shell de podman-docker, sin extension. Windows resuelve
#               los ejecutables por PATHEXT (.COM;.EXE;.BAT;.CMD...), asi que un
#               fichero sin extension no cuenta. De ahi el `spawnsync Docker
#               ENOENT` de VSCode y el `FileNotFoundError` de make.py, que llama
#               a `['docker', 'info']` sin ruta.
#
# Y no es la primera vez: el `find.exe` de C:\msys64\usr\bin ya rompio el
# entorno de MSVC e Intel en este repo, porque se anteponia al `find` de Windows
# dentro de vcvarsall (arreglado en el commit 94bdd05).
#
# QUE HACE, EXACTAMENTE
# ---------------------
#   1. Guarda copia del PATH de MAQUINA con fecha, y escribe un script de
#      deshacer al lado.
#   2. QUITA `C:\msys64\usr\bin` del PATH de MAQUINA. Es lo que recomienda la
#      propia documentacion de MSYS2: ese directorio es el entorno POSIX y no
#      debe estar en el PATH de Windows; sus binarios (bash, find, cmake, sort,
#      pkg-config...) se anteponen a los de Windows y a los nativos.
#      **No borra nada del disco**: MSYS2 sigue entero y su terminal sigue
#      funcionando igual, porque se lo pone el propio arranque de MSYS2.
#   3. SUBE `Docker\resources\bin` y `CMake\bin` al principio, por delante de
#      `ucrt64\bin`, para que ganen al shim de podman.
#   4. Comprueba el resultado y lo imprime.
#
# QUE **NO** TOCA
# ---------------
# `C:\msys64\ucrt64\bin` y `C:\msys64\clang64\bin` se quedan: ahi viven los
# compiladores, clang-format y doxygen que usa la suite, y son binarios NATIVOS
# de Windows (mingw-w64), no POSIX. El problema nunca fueron esos.
#
# POR QUE QUITAR `usr\bin` DEL PATH DE MAQUINA ES SEGURO
# ------------------------------------------------------
# **Porque ese directorio tambien esta en el PATH de USUARIO**, que este script
# no toca. Al quitarlo del de MAQUINA no desaparece: solo deja de ir por
# delante. Lo que se pierde es la PRIORIDAD, no la herramienta.
#
# Comparado herramienta por herramienta el 21 sep 2026, antes y despues:
#
#   gcc  g++  clang  clang-format  doxygen  pkg-config  grep  sed  perl  git
#       pasan de `usr\bin` (POSIX) a `ucrt64\bin` (NATIVO)  <- es una mejora:
#       son los mismos que usa la suite del proyecto
#
#   cmake   pasa al nativo de Program Files, que SI configura el proyecto
#   docker  pasa a Docker Desktop, que SI es un .exe
#
#   make, awk, python, ninja, tar, curl, ssh, bash, find
#       **no cambian**: o no estaban en `usr\bin`, o siguen resolviendo alli
#       por el PATH de USUARIO
#
# No hay ni una herramienta que se quede sin resolver.
#
# COMO SE DESHACE
# ---------------
# El script imprime la ruta del fichero de deshacer. Ejecutarlo como
# administrador devuelve el PATH exactamente como estaba.
#
# Uso:
#   powershell -ExecutionPolicy Bypass -File scripts\arregla_path_windows.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\arregla_path_windows.ps1 -Simular
# =============================================================================

[CmdletBinding()]
param(
    # Enseña lo que haria sin cambiar nada. Conviene pasarlo la primera vez.
    [switch]$Simular
)

$ErrorActionPreference = 'Stop'

# --------------------------------------------------------------- privilegios ---
$esAdmin = ([Security.Principal.WindowsPrincipal] `
            [Security.Principal.WindowsIdentity]::GetCurrent()
           ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $esAdmin -and -not $Simular) {
    Write-Host ""
    Write-Host "  HACE FALTA ADMINISTRADOR." -ForegroundColor Red
    Write-Host "  El PATH de MAQUINA no se puede cambiar desde una cuenta normal."
    Write-Host ""
    Write-Host "  Abre PowerShell como administrador y vuelve a lanzarlo, o prueba"
    Write-Host "  primero en seco:"
    Write-Host "      powershell -ExecutionPolicy Bypass -File $PSCommandPath -Simular"
    Write-Host ""
    exit 1
}

# ------------------------------------------------------------------- copias ---
$carpeta = Join-Path $env:USERPROFILE 'path-backups'
New-Item -ItemType Directory -Force $carpeta | Out-Null
$sello   = Get-Date -Format 'yyyyMMdd-HHmmss'
$copia   = Join-Path $carpeta "PATH_maquina_$sello.bak"
$deshacer= Join-Path $carpeta "deshacer_PATH_$sello.ps1"

$original = [Environment]::GetEnvironmentVariable('Path', 'Machine')
$original | Set-Content -Path $copia -Encoding UTF8

# El script de deshacer lleva el PATH viejo dentro, entre comillas simples, para
# que ni `$` ni los espacios de "Program Files" lo estropeen.
@"
# Deshace el cambio de PATH hecho el $sello. Ejecutar COMO ADMINISTRADOR.
`$viejo = '$($original -replace "'", "''")'
[Environment]::SetEnvironmentVariable('Path', `$viejo, 'Machine')
Write-Host 'PATH de MAQUINA restaurado. Cierra y vuelve a abrir las terminales y VSCode.'
"@ | Set-Content -Path $deshacer -Encoding UTF8

# ----------------------------------------------------------------- el cambio ---
$FUERA    = 'C:\msys64\usr\bin'           # el entorno POSIX: se quita
$DELANTE  = @(                            # estas van al principio, en este orden
    'C:\Program Files\Docker\Docker\resources\bin',
    'C:\Program Files\CMake\bin'
)

$partes = $original -split ';' | Where-Object { $_ -ne '' }

# Quitar el POSIX y las que se van a reponer delante. La comparacion normaliza
# la barra final: 'C:\X' y 'C:\X\' son el mismo sitio.
function Normaliza([string]$p) { $p.TrimEnd('\').ToLowerInvariant() }
$quitar = @($FUERA) + $DELANTE | ForEach-Object { Normaliza $_ }

$resto = $partes | Where-Object { $quitar -notcontains (Normaliza $_) }
$nuevo = (@($DELANTE) + $resto) -join ';'

# ------------------------------------------------------------------- informe ---
Write-Host ""
Write-Host "=== ANTES ===" -ForegroundColor Cyan
foreach ($t in 'cmake','docker','pkg-config','gcc') {
    $env:Path = $original + ';' + [Environment]::GetEnvironmentVariable('Path','User')
    $c = Get-Command $t -ErrorAction SilentlyContinue
    "{0,-12} -> {1}" -f $t, $(if ($c) { $c.Source } else { 'NO ESTA' })
}

Write-Host ""
Write-Host "=== CAMBIOS ===" -ForegroundColor Cyan
$habia = $partes | Where-Object { (Normaliza $_) -eq (Normaliza $FUERA) }
if ($habia) { Write-Host "  QUITA   $FUERA" -ForegroundColor Yellow }
else        { Write-Host "  (no estaba ${FUERA}: nada que quitar)" }
foreach ($d in $DELANTE) { Write-Host "  DELANTE $d" -ForegroundColor Green }

if ($Simular) {
    Write-Host ""
    Write-Host "=== SIMULACION: no se ha cambiado nada ===" -ForegroundColor Yellow
    Write-Host "PATH que quedaria (primeras 6 entradas):"
    ($nuevo -split ';' | Select-Object -First 6) | ForEach-Object { "    $_" }
    Write-Host ""
    Write-Host "copia de seguridad ya guardada en: $copia"
    Write-Host "deshacer:                          $deshacer"
    exit 0
}

[Environment]::SetEnvironmentVariable('Path', $nuevo, 'Machine')

# ---------------------------------------------------------------- resultado ---
$env:Path = $nuevo + ';' + [Environment]::GetEnvironmentVariable('Path','User')
Write-Host ""
Write-Host "=== DESPUES ===" -ForegroundColor Cyan
$ok = $true
foreach ($t in 'cmake','docker','pkg-config','gcc') {
    $c = Get-Command $t -ErrorAction SilentlyContinue
    $ruta = if ($c) { $c.Source } else { 'NO ESTA' }
    "{0,-12} -> {1}" -f $t, $ruta
    if ($t -eq 'cmake'  -and $ruta -notlike '*Program Files\CMake*') { $ok = $false }
    if ($t -eq 'docker' -and $ruta -notlike '*Docker\resources*')    { $ok = $false }
}

Write-Host ""
if ($ok) {
    Write-Host "  LISTO. `cmake` y `docker` resuelven a los nativos." -ForegroundColor Green
} else {
    Write-Host "  ATENCION: algo no resuelve donde deberia. Revisa arriba." -ForegroundColor Red
}
Write-Host ""
Write-Host "  copia de seguridad: $copia"
Write-Host "  para deshacer:      powershell -ExecutionPolicy Bypass -File `"$deshacer`""
Write-Host ""
Write-Host "  CIERRA Y VUELVE A ABRIR VSCode y las terminales: el PATH se hereda" -ForegroundColor Yellow
Write-Host "  al arrancar el proceso, asi que los que ya estan abiertos siguen" -ForegroundColor Yellow
Write-Host "  con el viejo." -ForegroundColor Yellow
Write-Host ""
