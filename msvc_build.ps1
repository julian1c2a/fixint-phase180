$cl = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe'
$msvc = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717'
$sdk = 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0'
$sdklib = 'C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0'

$env:INCLUDE = "$msvc\include;$sdk\ucrt;$sdk\um;$sdk\shared"
$env:LIB = "$msvc\lib\x64;$sdklib\ucrt\x64;$sdklib\um\x64"

Write-Output "INCLUDE=$($env:INCLUDE)"

Set-Location 'C:\msys64\ucrt64\home\julian\CppProjects\int128-phase175'

$test = $args[0]
if (-not $test) { $test = 'tests\test_knuth_d_correctness.cpp' }
$exe = 'C:\Temp\test_msvc.exe'

Write-Output "Compiling: $test"
& $cl /std:c++20 /O2 /EHsc /W3 /I include $test /Fe:$exe
$build_exit = $LASTEXITCODE
Write-Output "BUILD_EXIT=$build_exit"

if ($build_exit -eq 0) {
    Write-Output "Running: $exe"
    & $exe
    Write-Output "RUN_EXIT=$LASTEXITCODE"
}
