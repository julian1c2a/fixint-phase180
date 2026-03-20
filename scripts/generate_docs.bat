@echo off
REM Script para generar documentación en Windows (PowerShell/CMD)

echo ================================================================================
echo Generando Documentacion con Doxygen
echo ================================================================================
echo.

REM Verificar que Doxygen esta instalado
where doxygen >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Doxygen no esta instalado
    echo.
    echo Instalacion:
    echo   Windows: choco install doxygen.install graphviz
    echo   MSYS2:   pacman -S mingw-w64-ucrt-x86_64-doxygen
    exit /b 1
)

REM Cambiar al directorio raiz del proyecto
cd /d "%~dp0.."

REM Crear directorio de salida
if not exist "documentation\generated" mkdir "documentation\generated"

echo Ejecutando Doxygen...
echo --------------------------------------------------------------------------------

REM Ejecutar Doxygen
doxygen Doxyfile

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ================================================================================
    echo Documentacion generada exitosamente
    echo ================================================================================
    echo.
    echo Ubicacion: %CD%\documentation\generated\html
    echo.
    echo Para ver: start documentation\generated\html\index.html
    echo.
    
    REM Preguntar si abrir
    set /p OPEN="Abrir documentacion en el navegador? (s/n): "
    if /i "%OPEN%"=="s" (
        start documentation\generated\html\index.html
    )
) else (
    echo.
    echo ================================================================================
    echo ERROR al generar documentacion
    echo ================================================================================
    exit /b 1
)
