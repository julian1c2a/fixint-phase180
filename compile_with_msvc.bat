@echo off
REM Compilar con MSVC con setup correcto
cd /d "%~dp0"

setlocal enabledelayedexpansion

REM Setup MSVC environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

REM Compile
echo Compilando con MSVC...
cl.exe /std:c++20 /O2 /Iinclude /Fe:build\test_msvc.exe tests\test_divmod_final.cpp

if %ERRORLEVEL% equ 0 (
    echo.
    echo ======== Ejecutando tests MSVC ========
    echo.
    build\test_msvc.exe
) else (
    echo Compilacion MSVC fallida
    exit /b 1
)
