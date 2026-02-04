@echo off
REM Compilar con Intel oneAPI con setup correcto
cd /d "%~dp0"

setlocal enabledelayedexpansion

REM Setup MSVC environment first (Intel needs MSVC libraries)
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

REM Setup Intel environment
call "C:\Program Files (x86)\Intel\oneAPI\setvars.bat" intel64 >nul 2>&1

REM Compile
echo Compilando con Intel ICX...
icx.exe /std:c++20 /O2 /Iinclude /Fe:build\test_intel.exe tests\test_divmod_final.cpp

if %ERRORLEVEL% equ 0 (
    echo.
    echo ======== Ejecutando tests Intel ========
    echo.
    build\test_intel.exe
) else (
    echo Compilacion Intel fallida
    exit /b 1
)
