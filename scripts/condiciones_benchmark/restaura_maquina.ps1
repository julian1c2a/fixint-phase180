# =============================================================================
# Volver a arrancar lo que paro `silencia_maquina.ps1`, y quitar las exclusiones.
# =============================================================================
#
# Abrir PowerShell COMO ADMINISTRADOR y ejecutar:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File "$env:USERPROFILE\OneDrive\escritorio\restaura_maquina.ps1"
#
# Si no esta el fichero de registro --porque se reinicio la maquina, que ya lo
# restaura todo-- este guion no tiene nada que hacer y lo dice en vez de callarse.

$ErrorActionPreference = 'Continue'
$registro = Join-Path $env:USERPROFILE 'servicios_parados_para_medir.txt'

# Arrancar un servicio pide elevacion igual que pararlo. Sin ella, esto fallaria
# en todas y terminaria como si hubiera restaurado algo -- el mismo fallo que
# tuvo `silencia_maquina.ps1` el 27 sep.
$identidad = [Security.Principal.WindowsIdentity]::GetCurrent()
$soyAdmin = (New-Object Security.Principal.WindowsPrincipal($identidad)).IsInRole(
  [Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $soyAdmin) {
  Write-Host ''
  Write-Host '  ESTO NECESITA ADMINISTRADOR, Y NO LO ERES.' -ForegroundColor Red
  Write-Host ''
  Write-Host '  La forma corta, desde esta misma ventana:' -ForegroundColor Cyan
  Write-Host ("    Start-Process powershell -Verb RunAs -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-NoExit','-File','{0}'" -f $PSCommandPath) -ForegroundColor Yellow
  Write-Host ''
  Write-Host '  Y si no quieres elevar: REINICIA. Ningun servicio quedo desactivado'
  Write-Host '  --solo parado--, asi que un reinicio los devuelve todos.'
  Write-Host ''
  exit 1
}

if (-not (Test-Path $registro)) {
  Write-Host 'No hay registro de servicios parados: nada que restaurar.' -ForegroundColor Yellow
  Write-Host '(Si reiniciaste, ya estan todos como estaban: el guion no cambia'
  Write-Host ' el tipo de inicio de nada.)'
} else {
  $n_ok = 0; $n_no = 0
  foreach ($nombre in (Get-Content $registro | Where-Object { $_.Trim() })) {
    try {
      Start-Service -Name $nombre -ErrorAction Stop
      Write-Host ("  [arrancado]  {0}" -f $nombre) -ForegroundColor Green
      $n_ok++
    } catch {
      # Un servicio de inicio MANUAL que nadie ha pedido no tiene por que
      # arrancar, y eso NO es un error: se distingue para no asustar.
      Write-Host ("  [no arranca ahora]  {0} -- {1}" -f $nombre, $_.Exception.Message) -ForegroundColor DarkGray
      $n_no++
    }
  }
  Write-Host ''
  Write-Host ("{0} arrancados, {1} que no arrancan ahora (normal en los de inicio manual)" -f $n_ok, $n_no)
  Remove-Item $registro
}

Write-Host ''
Write-Host '=== Exclusiones de Defender ===' -ForegroundColor Cyan
foreach ($r in @('E:\Dropbox\GitHub\cpp\fixint-phase180\build', 'C:\msys64')) {
  try {
    Remove-MpPreference -ExclusionPath $r -ErrorAction Stop
    Write-Host ("  [quitada]  {0}" -f $r) -ForegroundColor Green
  } catch {
    Write-Host ("  [no estaba]  {0}" -f $r) -ForegroundColor DarkGray
  }
}

Write-Host ''
Write-Host 'Y vuelve a activar Dropbox y OneDrive desde sus iconos.'
