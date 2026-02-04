@echo off
REM Batch script para compilar y testear con MSVC e Intel

setlocal enabledelayedexpansion

cd /d "%~dp0"

echo.
echo ======================================================================
echo  MSVC 2026 Compilation
echo ======================================================================
echo.

set "MSVC_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
set "INTEL_PATH=C:\Program Files (x86)\Intel\oneAPI\compiler\latest\bin\icx.exe"
set "VSCMD_DEBUG=0"

REM Test MSVC
if exist "%MSVC_PATH%" (
    echo [Compiling with MSVC...]
    "%MSVC_PATH%" /std:c++latest /O2 /Iinclude /Fe:build\test_msvc.exe tests\test_divmod_final.cpp
    if !ERRORLEVEL! equ 0 (
        echo [Running MSVC tests...]
        build\test_msvc.exe
    ) else (
        echo [MSVC compilation failed]
    )
) else (
    echo [MSVC not found at: %MSVC_PATH%]
)

echo.
echo ======================================================================
echo  Intel oneAPI Compilation
echo ======================================================================
echo.

REM Test Intel
if exist "%INTEL_PATH%" (
    echo [Compiling with Intel ICX...]
    "%INTEL_PATH%" /std:c++20 /O2 /Iinclude /Fe:build\test_intel.exe tests\test_divmod_final.cpp
    if !ERRORLEVEL! equ 0 (
        echo [Running Intel tests...]
        build\test_intel.exe
    ) else (
        echo [Intel compilation failed]
    )
) else (
    echo [Intel not found at: %INTEL_PATH%]
)

echo.
echo ======================================================================
echo  Summary
echo ======================================================================
echo.

dir /b build\test_*.exe 2>nul

pause
