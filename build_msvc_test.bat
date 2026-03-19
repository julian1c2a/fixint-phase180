@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
echo INCLUDE=%INCLUDE%
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe" /std:c++20 /O2 /EHsc /W3 /I include tests\test_knuth_d_correctness.cpp /Fe:C:\Temp\test_knuth_msvc.exe
set BUILD_EXIT=%ERRORLEVEL%
echo BUILD_EXIT=%BUILD_EXIT%
if %BUILD_EXIT% EQU 0 (
    C:\Temp\test_knuth_msvc.exe
    echo RUN_EXIT=%ERRORLEVEL%
)
