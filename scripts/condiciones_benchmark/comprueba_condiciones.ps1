# =============================================================================
# En que estado esta la maquina para medir. SOLO LEE: no para nada, no necesita
# administrador, no cambia nada.
# =============================================================================
#
# POR QUE EXISTE, SEPARADO DE `silencia_maquina.ps1`: parar servicios exige
# elevacion y es una decision de la persona; COMPROBAR no exige nada y puede
# hacerse en cada toma. Sin esta comprobacion, una medida solo puede decir «la
# carga era del 2 %», que es cierto y no basta: un `CCleaner` parado a las
# 11:40 y arrancado a las 12:00 no aparece en esa cifra.
#
# Lo que imprime son lineas `clave=valor`, para que `bench_history.py` las meta
# tal cual en el JSON de la toma. Una sola linea `verdicto=` al final.
#
# OJO CON EL ORDEN: esto hay que correrlo ANTES de la espera de maquina ociosa,
# nunca despues. Arrancar PowerShell cuesta uno o dos segundos de CPU, o sea que
# la comprobacion **rompe la condicion que acaba de verificar** si se hace al
# final. Con la espera despues, ese pico se absorbe.

$ErrorActionPreference = 'SilentlyContinue'

function Sin-Tildes([string]$s) {
  if (-not $s) { return '' }
  $d = $s.Normalize([Text.NormalizationForm]::FormD)
  -join ($d.ToCharArray() | Where-Object {
    [Globalization.CharUnicodeInfo]::GetUnicodeCategory($_) -ne 'NonSpacingMark'
  })
}

# Los mismos patrones que para `silencia_maquina.ps1`. Si se cambian ahi, aqui
# tambien: son dos listas y es una deuda conocida, pero duplicarlas es mejor que
# que un guion que solo LEE tenga que cargar otro que PARA servicios.
$sondeadores = @(
  'CCleaner', 'Optimizacion de distribucion',
  'Servicio de transferencia inteligente en segundo plano',
  'Servicio orquestador de actualizaciones', 'SysMain',
  'Intel(R) Driver & Support Assistant', 'Killer Analytics Service',
  'Killer Network Service', 'Killer Provider Data Helper Service',
  'KillerSmartphoneSleepService', 'Killer Dynamic Bandwidth Management',
  'Killer Smart AP Selection Service', 'xTendSoftAPService', 'Nahimic service',
  'Micro Star SCM', 'NI PSP Service Locator', 'NI Time Synchronization',
  'HP Print Scan Doctor Service', 'Steam Client Service', 'Gaming Services',
  'Administracion de autenticacion de Xbox Live',
  'Experiencias del usuario y telemetria asociadas',
  'Estado y experiencias optimizadas de Windows',
  'Servicio de directivas de diagnostico', 'Host de sistema de diagnostico',
  'Cliente de seguimiento de vinculos distribuidos',
  'Servicio de geolocalizacion', 'Cola de impresion'
)

# Si esto se corre sin elevar, `silencia_maquina.ps1` no habria podido hacer nada,
# y eso explica por si solo un «26 sondeadores vivos». El dato va al JSON de la
# toma para que la explicacion no se pierda.
$identidad = [Security.Principal.WindowsIdentity]::GetCurrent()
$soyAdmin = (New-Object Security.Principal.WindowsPrincipal($identidad)).IsInRole(
  [Security.Principal.WindowsBuiltInRole]::Administrator)
Write-Output ("elevado=" + $soyAdmin.ToString().ToLower())

$todos = Get-Service
$vivos = @()
$sin_resolver = 0
foreach ($p in $sondeadores) {
  $pn = Sin-Tildes $p
  $m = $todos | Where-Object { (Sin-Tildes $_.DisplayName) -like "*$pn*" }
  if (-not $m) { $sin_resolver++; continue }
  foreach ($s in @($m)) {
    if ($s.Status -eq 'Running') { $vivos += $s.Name }
  }
}

Write-Output ("sondeadores_vivos=" + $vivos.Count)
Write-Output ("sondeadores_cuales=" + ($vivos -join ','))
# Un patron que no resuelve NO es «no esta»: es que esta lista esta desfasada.
# Se publica para que no pase inadvertido, que es como se cuelan los ceros
# tranquilizadores.
Write-Output ("sondeadores_sin_resolver=" + $sin_resolver)

# --- Procesos que tocan los mismos ficheros o comen CPU ---------------------
$ruidosos = @('Dropbox', 'OneDrive', 'msedge', 'chrome', 'firefox', 'Docker Desktop',
              'com.docker.backend', 'vmmemWSL', 'Teams', 'Zoom', 'steam')
$proc_vivos = @()
foreach ($n in $ruidosos) {
  if (Get-Process -Name $n -ErrorAction SilentlyContinue) { $proc_vivos += $n }
}
Write-Output ("procesos_ruidosos=" + ($proc_vivos -join ','))

# --- Defender: exclusiones puestas? ----------------------------------------
$excl = @()
try { $excl = (Get-MpPreference).ExclusionPath } catch { }
$tiene_build = $false
foreach ($e in @($excl)) { if ($e -like '*fixint-phase180*') { $tiene_build = $true } }
Write-Output ("defender_excluye_build=" + $tiene_build.ToString().ToLower())

# --- Tareas programadas a punto de disparar --------------------------------
# LA CAUSA CLASICA DE LOS PICOS RAROS, y la que no se ve en ninguna lista de
# servicios: una tarea que salta a mitad de la tanda.
$pronto = 0
$cuales = @()
try {
  $limite = (Get-Date).AddHours(2)
  $ts = Get-ScheduledTask | Where-Object { $_.State -eq 'Ready' } |
        Get-ScheduledTaskInfo |
        Where-Object { $_.NextRunTime -and $_.NextRunTime -lt $limite } |
        Sort-Object NextRunTime
  $pronto = @($ts).Count
  # LOS NOMBRES, no solo el recuento. Un «8» no se puede accionar: hay que saber
  # si son ocho comprobaciones de nada o un backup de media hora.
  foreach ($t in @($ts)) {
    $cuales += ('{0}@{1:HH:mm}' -f ($t.TaskName -replace '[,;=]', '_'), $t.NextRunTime)
  }
} catch { $pronto = -1 }
Write-Output ("tareas_en_2h=" + $pronto)
Write-Output ("tareas_cuales=" + (($cuales | Select-Object -First 12) -join ','))

# --- El veredicto, en una linea -------------------------------------------
$puntos = @()
if ($vivos.Count -gt 8) { $puntos += ("{0} sondeadores vivos" -f $vivos.Count) }
if ($proc_vivos -contains 'Dropbox') { $puntos += 'Dropbox activo sobre el repo' }
if (-not $tiene_build) { $puntos += 'Defender sin excluir build/' }
if ($pronto -gt 3) { $puntos += ("{0} tareas programadas en 2 h" -f $pronto) }
if ($puntos.Count -eq 0) {
  Write-Output 'verdicto=preparada'
} else {
  Write-Output ("verdicto=" + ($puntos -join '; '))
}
