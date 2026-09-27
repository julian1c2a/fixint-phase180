# =============================================================================
# Silenciar la maquina para una sesion de medicion. REVERSIBLE.
# =============================================================================
#
# Abrir PowerShell COMO ADMINISTRADOR y ejecutar:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File "$env:USERPROFILE\OneDrive\escritorio\silencia_maquina.ps1"
#
# Lo que hace y lo que NO hace:
#
#   - SOLO PARA servicios (`Stop-Service`). NO toca el tipo de inicio, asi que
#     **un reinicio lo deja todo como estaba**. Y apunta en un fichero lo que ha
#     parado, para poder volver con `restaura_maquina.ps1`.
#   - NO toca Defender, BitLocker, el firewall, RPC/DCOM, el registro de eventos
#     ni Hyper-V/WSL. Defender se trata con EXCLUSIONES, que es lo correcto: lo
#     que molesta no es que exista, es que escanee cada fichero que el compilador
#     escribe y borra mil veces.
#   - NO toca la frecuencia ni la prioridad. Eso cambia el REGIMEN de medida y
#     obliga a empezar una serie nueva de cifras; se decide aparte y midiendo.
#
# Elegidos de la lista de `services.msc` de esta maquina, por una razon cada uno:
# los que SONDEAN (actualizadores, analitica), los que pueden ARRANCAR UN TRABAJO
# GORDO a mitad de la medida (CCleaner, Windows Update, BITS) y los que tocan los
# MISMOS FICHEROS que se estan compilando (Dropbox, a mano).
#
# -----------------------------------------------------------------------------
# UN FALLO QUE TUVO ESTE GUION, Y QUE IMPORTA MAS QUE LA LISTA
# -----------------------------------------------------------------------------
# La primera version buscaba los servicios por su nombre visible escrito SIN
# TILDES --para que el fichero sobreviviera a cualquier codificacion-- y los
# comparaba tal cual contra el nombre real. Resultado: «Optimizacion de
# distribucion», «Cola de impresion» y «Administracion de autenticacion de Xbox
# Live» NO COINCIDIAN NUNCA, porque los de verdad llevan tilde. El guion habria
# dicho «[no esta]» y habria seguido tan tranquilo: tres servicios sin parar y un
# informe en verde.
#
# Arreglado quitando las tildes de LOS DOS LADOS antes de comparar. Y al final
# hay un recuento de los patrones que no resolvieron a ningun servicio: si sale
# alguno, es que la lista esta desfasada, no que el servicio no exista.
# =============================================================================

$ErrorActionPreference = 'Continue'
$registro = Join-Path $env:USERPROFILE 'servicios_parados_para_medir.txt'

function Sin-Tildes([string]$s) {
  if (-not $s) { return '' }
  $d = $s.Normalize([Text.NormalizationForm]::FormD)
  -join ($d.ToCharArray() | Where-Object {
    [Globalization.CharUnicodeInfo]::GetUnicodeCategory($_) -ne 'NonSpacingMark'
  })
}

# --- Lo que puede arrancar un trabajo gordo a mitad de la medida -------------
$gordos = @(
  'CCleaner',
  'Optimizacion de distribucion',
  'Servicio de transferencia inteligente en segundo plano',
  'Servicio orquestador de actualizaciones',
  'SysMain'
)

# --- Sondeadores: poca CPU, pero despiertan cada pocos segundos --------------
$sondas = @(
  'Intel(R) Driver & Support Assistant',
  'Intel(R) Driver & Support Assistant Updater',
  'Killer Analytics Service',
  'Killer Network Service',
  'Killer Provider Data Helper Service',
  'KillerSmartphoneSleepService',
  'Killer Dynamic Bandwidth Management',
  'Killer Smart AP Selection Service',
  'xTendSoftAPService',
  'Nahimic service',
  'Micro Star SCM',
  'NI PSP Service Locator',
  'NI Time Synchronization',
  'HP Print Scan Doctor Service',
  'Steam Client Service',
  'Gaming Services',
  'Administracion de autenticacion de Xbox Live',
  'Experiencias del usuario y telemetria asociadas',
  'Estado y experiencias optimizadas de Windows',
  'Servicio de directivas de diagnostico',
  'Host de sistema de diagnostico',
  'Cliente de seguimiento de vinculos distribuidos',
  'Servicio de geolocalizacion',
  'Cola de impresion'
)

$noResuelven = @()

function Para-Servicios($nombres, $etiqueta) {
  Write-Host ''
  Write-Host "=== $etiqueta ===" -ForegroundColor Cyan
  foreach ($n in $nombres) {
    $patron = Sin-Tildes $n
    $svc = Get-Service | Where-Object { (Sin-Tildes $_.DisplayName) -like "*$patron*" }
    if (-not $svc) {
      Write-Host ("  [NO RESUELVE]  {0}" -f $n) -ForegroundColor Yellow
      $script:noResuelven += $n
      continue
    }
    foreach ($s in @($svc)) {
      if ($s.Status -ne 'Running') {
        Write-Host ("  [ya estaba parado]  {0}" -f $s.DisplayName) -ForegroundColor DarkGray
        continue
      }
      try {
        Stop-Service -Name $s.Name -Force -ErrorAction Stop
        Add-Content -Path $registro -Value $s.Name
        Write-Host ("  [PARADO]  {0}  ({1})" -f $s.DisplayName, $s.Name) -ForegroundColor Green
      } catch {
        Write-Host ("  [NO SE PUDO]  {0} -- {1}" -f $s.DisplayName, $_.Exception.Message) -ForegroundColor Red
      }
    }
  }
}

if (Test-Path $registro) { Remove-Item $registro }
Para-Servicios $gordos 'Los que pueden arrancar un trabajo gordo'
Para-Servicios $sondas 'Los que sondean'

# --- Defender: EXCLUSIONES, no apagarlo -------------------------------------
Write-Host ''
Write-Host '=== Defender: exclusiones del arbol de compilacion ===' -ForegroundColor Cyan
Write-Host '  No se apaga el antivirus. Se le dice que no escanee lo que el'
Write-Host '  compilador escribe y borra mil veces, que es lo que cuesta caro.'
foreach ($r in @('E:\Dropbox\GitHub\cpp\fixint-phase180\build', 'C:\msys64')) {
  try {
    Add-MpPreference -ExclusionPath $r -ErrorAction Stop
    Write-Host ("  [excluida]  {0}" -f $r) -ForegroundColor Green
  } catch {
    Write-Host ("  [NO SE PUDO excluir]  {0} -- {1}" -f $r, $_.Exception.Message) -ForegroundColor Yellow
  }
}

# --- Lo que queda por hacer A MANO ------------------------------------------
Write-Host ''
Write-Host '=== A MANO, que no se automatiza sin riesgo ===' -ForegroundColor Cyan
Write-Host '  1. PAUSA DROPBOX desde su icono. No pares DbxSvc a lo bruto: al'
Write-Host '     volver resincroniza con prisa y ESO si es carga. El repo vive'
Write-Host '     dentro de la carpeta sincronizada.'
Write-Host '  2. Pausa OneDrive igual.'
Write-Host '  3. Cierra el navegador. Una pestana con un video o un chat mueve'
Write-Host '     mas que todos los servicios de esta lista juntos.'
Write-Host '  4. Cierra Docker Desktop si no lo necesitas: son siete procesos'
Write-Host '     mas una maquina virtual (vmmemWSL) que puede despertarse.'
Write-Host ''
Write-Host '  Y mira si hay tareas programadas a punto de disparar:' -ForegroundColor Cyan
Write-Host '    Get-ScheduledTask | ? State -eq ''Ready'' | Get-ScheduledTaskInfo |'
Write-Host '      ? { $_.NextRunTime -and $_.NextRunTime -lt (Get-Date).AddHours(3) } |'
Write-Host '      Sort NextRunTime | Select TaskName, NextRunTime -First 20'

if ($noResuelven.Count -gt 0) {
  Write-Host ''
  Write-Host ("[OJO] {0} patron(es) no resolvieron a ningun servicio:" -f $noResuelven.Count) -ForegroundColor Yellow
  foreach ($n in $noResuelven) { Write-Host ("       - {0}" -f $n) -ForegroundColor Yellow }
  Write-Host '      Eso NO significa que el servicio no exista: significa que la'
  Write-Host '      lista de este guion esta desfasada. Comprobarlo a mano.'
}

Write-Host ''
Write-Host ("Lo parado queda apuntado en: {0}" -f $registro)
Write-Host 'Para volver atras: restaura_maquina.ps1  (o reiniciar, que hace lo mismo)'
