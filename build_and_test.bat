@echo off
cmake -S . -B build
cmake --build build
ctest --test-dir build -R test_param_iostreams
