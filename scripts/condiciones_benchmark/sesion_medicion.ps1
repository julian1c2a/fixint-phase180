# =============================================================================
# Una sola orden para preparar (o devolver) la maquina, con TODO a un log.
# =============================================================================
#
#   .\sesion_medicion.ps1                  # comprobar -> silenciar -> comprobar
#   .\sesion_medicion.ps1 -Accion comprobar
#   .\sesion_medicion.ps1 -Accion restaurar
#
# POR QUE EXISTE. Los tres guiones sueltos obligaban a: abrir una consola
# elevada a mano, componer una orden de `Start-Process` de cuatro lineas,
# lanzarlos en orden, y COPIAR A MANO paredes de salida para poder mirarlas
# despues. Aqui:
#
#   - **se eleva solo** si la accion lo necesita (sale el aviso de UAC);
#   - **todo va a un log de ruta fija**, que se puede leer despues sin copiar
#     nada: %USERPROFILE%\condiciones_benchmark.log
#   - **comprueba antes y despues**, que es lo unico que demuestra que sirvio de
#     algo. Un guion que dice «he parado 24 servicios» no prueba que la maquina
#     este preparada; dos comprobaciones con el estado antes y despues, si.
#
# El log se ANADE, no se sobrescribe: la historia de las sesiones es justo lo que
# permite mirar despues por que una toma salio movida.

param(
  [ValidateSet('comprobar', 'silenciar', 'restaurar')]
  [string]$Accion = 'silenciar'
)

$ErrorActionPreference = 'Continue'
$aqui = Split-Path -Parent $PSCommandPath
$log = Join-Path $env:USERPROFILE 'condiciones_benchmark.log'
$necesitaAdmin = ($Accion -ne 'comprobar')

$identidad = [Security.Principal.WindowsIdentity]::GetCurrent()
$soyAdmin = (New-Object Security.Principal.WindowsPrincipal($identidad)).IsInRole(
  [Security.Principal.WindowsBuiltInRole]::Administrator)

# --- Elevarse solo, si hace falta -------------------------------------------
if ($necesitaAdmin -and -not $soyAdmin) {
  Write-Host ''
  Write-Host ("  '{0}' necesita administrador. Relanzando elevado (saldra el aviso de UAC)..." -f $Accion) -ForegroundColor Cyan
  Write-Host ("  La salida ira al log: {0}" -f $log) -ForegroundColor Cyan
  Write-Host ''
  # OJO: `$args` es una variable automatica de PowerShell. Llamarla asi
  # funcionaria, pero pisar una automatica en un guion que ademas recibe
  # parametros es la clase de cosa que luego no se entiende.
  $argumentos = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $PSCommandPath,
                  '-Accion', $Accion)
  try {
    $p = Start-Process powershell -Verb RunAs -ArgumentList $argumentos -PassThru -Wait
    Write-Host ("  La ventana elevada termino con codigo {0}." -f $p.ExitCode)
    Write-Host ("  Mira el log: {0}" -f $log) -ForegroundColor Green
    exit $p.ExitCode
  } catch {
    Write-Host ('  No se pudo elevar: ' + $_.Exception.Message) -ForegroundColor Red
    Write-Host '  (Si cancelaste el aviso de UAC, es esto.)'
    exit 1
  }
}

# --- Registro ---------------------------------------------------------------
function Escribe($texto) {
  # A la consola y al log en la misma llamada. `Tee-Object` haria esto, pero
  # anadiendo una tuberia por linea; asi es mas simple y no depende del host.
  Write-Host $texto
  Add-Content -Path $log -Value $texto -Encoding UTF8
}

function Corre($guion, $titulo) {
  Escribe ''
  Escribe ('########## ' + $titulo + ' ##########')
  $ruta = Join-Path $aqui $guion
  if (-not (Test-Path $ruta)) {
    Escribe ('  [FALTA] ' + $ruta)
    return 127
  }
  # `*>&1` junta TODOS los flujos --salida, errores, avisos, informacion-- en
  # uno. Sin eso, un error del guion hijo saldria por la consola y NO al log,
  # que es justo lo que uno quiere leer despues.
  $salida = & $ruta *>&1
  $codigo = $LASTEXITCODE
  foreach ($l in $salida) { Escribe ('  ' + $l) }
  # Un guion que no llama a `exit` deja `$LASTEXITCODE` sin tocar, y un hueco
  # donde deberia haber un numero parece un fallo del guion.
  $texto_codigo = if ($null -eq $codigo -or $codigo -eq '') { 'sin codigo (no llamo a exit)' } else { $codigo }
  Escribe ('---------- ' + $titulo + ': ' + $texto_codigo + ' ----------')
  return $codigo
}

Escribe ''
Escribe '============================================================================='
Escribe ("  SESION DE MEDICION -- accion '{0}' -- {1}" -f $Accion, (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'))
Escribe ("  maquina {0}, usuario {1}, elevado {2}" -f $env:COMPUTERNAME, $env:USERNAME, $soyAdmin)
Escribe '============================================================================='

switch ($Accion) {
  'comprobar' {
    $null = Corre 'comprueba_condiciones.ps1' 'ESTADO DE LA MAQUINA'
  }
  'silenciar' {
    $null = Corre 'comprueba_condiciones.ps1' 'ANTES'
    $null = Corre 'silencia_maquina.ps1'      'SILENCIANDO'
    $null = Corre 'comprueba_condiciones.ps1' 'DESPUES'
  }
  'restaurar' {
    $null = Corre 'restaura_maquina.ps1'      'RESTAURANDO'
    $null = Corre 'comprueba_condiciones.ps1' 'DESPUES'
  }
}

Escribe ''
Escribe 'LO QUE NO SE AUTOMATIZA, Y HAY QUE MIRAR A OJO:'
Escribe '  - Dropbox y OneDrive pausados desde su icono (el repo esta dentro de Dropbox).'
Escribe '  - Navegador cerrado.'
Escribe '  - Y en el bloque DESPUES: sondeadores_vivos bajo, mantenimiento_corriendo vacio.'
Escribe ''
Escribe ("Log completo: {0}" -f $log)
Escribe '============================================================================='

Write-Host ''
Write-Host ("  Todo esto esta en {0}" -f $log) -ForegroundColor Green
if ($soyAdmin -and $necesitaAdmin) {
  # La ventana elevada se cierra al terminar y con ella la salida. Por eso el log
  # existe; pero una pausa corta permite leer el resumen sin abrirlo.
  Write-Host '  (esta ventana se cierra en 15 s; el log queda)' -ForegroundColor DarkGray
  Start-Sleep -Seconds 15
}
