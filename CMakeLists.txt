# =============================================================================
# int128 - 128-bit Integer Types for C++20
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julián Calderón Almendros
#
# CMake build system para el proyecto int128
#
# ⚠️ WORKFLOW OBLIGATORIO ⚠️
# Este CMakeLists.txt DELEGA a make.py - NO reemplaza el workflow Python
# 
# CORRECTO:
#   python make.py build uint128 bits tests gcc release
#   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
#
# INCORRECTO:
#   g++ -std=c++20 tests/int128_*.cpp  ❌ NUNCA COMPILAR MANUALMENTE
#
# Ver: WORKFLOW_OBLIGATORIO.md para detalles completos
#
# =============================================================================

cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# =============================================================================
# Configuración del proyecto
# =============================================================================

project(int128
    VERSION 1.0.0
    DESCRIPTION "128-bit Integer Types for C++20"
    HOMEPAGE_URL "https://github.com/julian1c2a/int128"
    LANGUAGES CXX
)

# C++20 es obligatorio
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  INT128 PROJECT - CMake Configuration")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Source Dir:   ${CMAKE_SOURCE_DIR}")
message(STATUS "  Binary Dir:   ${CMAKE_BINARY_DIR}")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "")

# =============================================================================
# Opciones de configuración
# =============================================================================

option(INT128_BUILD_TESTS "Build test executables" ON)
option(INT128_BUILD_BENCHMARKS "Build benchmark executables" ON)
option(INT128_BUILD_DEMOS "Build demo executables" ON)
option(INT128_ENABLE_TESTING "Enable CTest integration" ON)
option(INT128_ENABLE_COVERAGE "Enable code coverage reporting" OFF)

# Opciones de análisis estático
option(INT128_ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)
option(INT128_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
option(INT128_ENABLE_INFER "Enable infer static analysis" OFF)
option(INT128_ENABLE_PVS_STUDIO "Enable PVS-Studio static analysis" OFF)

# Opciones de sanitizers
option(INT128_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(INT128_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(INT128_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(INT128_ENABLE_MSAN "Enable MemorySanitizer (Clang only)" OFF)

# =============================================================================
# Incluir módulos de configuración
# =============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 1. Detección de compilador y plataforma
include(CompilerDetection)

# 2. Configuración de sanitizers
include(SanitizerConfig)

# 3. Configuración de análisis estático
include(StaticAnalysis)

# 4. Configuración de testing
include(TestConfig)

# =============================================================================
# Directorios de include
# =============================================================================

include_directories(${CMAKE_SOURCE_DIR}/include)

# =============================================================================
# Subdirectorios con targets
# =============================================================================

if(INT128_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(INT128_BUILD_BENCHMARKS)
    add_subdirectory(benchs)
endif()

if(INT128_BUILD_DEMOS)
    add_subdirectory(demos)
endif()

# =============================================================================
# Targets personalizados que delegan a make.py
# =============================================================================

find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
    message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
    
    # Target: build-all (delega a make.py)
    add_custom_target(build-all
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/make.py all
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building all targets via make.py..."
        VERBATIM
    )
    
    # Target: test-all (delega a make.py)
    add_custom_target(test-all
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/make.py test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running all tests via make.py..."
        VERBATIM
    )
    
    # Target: bench-all (delega a make.py)
    add_custom_target(bench-all
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/make.py bench
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running all benchmarks via make.py..."
        VERBATIM
    )
    
    # Target: clean-python (limpia usando make.py)
    add_custom_target(clean-python
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/make.py clean
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Cleaning via make.py..."
        VERBATIM
    )
    
else()
    message(WARNING "Python3 not found - make.py delegation targets disabled")
    message(STATUS "Install Python3 to use: build-all, test-all, bench-all targets")
endif()

# =============================================================================
# Target: format (formatea código con clang-format)
# =============================================================================

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format clang-format-19)

if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/include/*.hpp
        ${CMAKE_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_SOURCE_DIR}/benchs/*.cpp
        ${CMAKE_SOURCE_DIR}/demos/*.cpp
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting code with clang-format..."
        VERBATIM
    )
    
    message(STATUS "Target 'format' created (clang-format found)")
else()
    message(STATUS "clang-format not found - 'format' target not available")
endif()

# =============================================================================
# Información final de configuración
# =============================================================================

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  CONFIGURATION SUMMARY")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  Compiler:     ${INT128_COMPILER_FULL_ID}")
message(STATUS "  Platform:     ${INT128_PLATFORM}")
message(STATUS "")

if(INT128_BUILD_TESTS)
    message(STATUS "  Tests:        ✅ ENABLED")
else()
    message(STATUS "  Tests:        ❌ DISABLED")
endif()

if(INT128_BUILD_BENCHMARKS)
    message(STATUS "  Benchmarks:   ✅ ENABLED")
else()
    message(STATUS "  Benchmarks:   ❌ DISABLED")
endif()

if(INT128_BUILD_DEMOS)
    message(STATUS "  Demos:        ✅ ENABLED")
else()
    message(STATUS "  Demos:        ❌ DISABLED")
endif()

message(STATUS "")

if(INT128_ENABLE_ASAN OR INT128_ENABLE_UBSAN OR INT128_ENABLE_TSAN OR INT128_ENABLE_MSAN)
    message(STATUS "  Sanitizers:   ✅ ENABLED")
else()
    message(STATUS "  Sanitizers:   ❌ DISABLED")
endif()

if(INT128_ENABLE_CPPCHECK OR INT128_ENABLE_CLANG_TIDY OR INT128_ENABLE_INFER OR INT128_ENABLE_PVS_STUDIO)
    message(STATUS "  Analysis:     ✅ ENABLED")
else()
    message(STATUS "  Analysis:     ❌ DISABLED")
endif()

message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "")

# =============================================================================
# Información de uso
# =============================================================================

message(STATUS "Build commands:")
message(STATUS "  cmake --build .                   # Build all")
message(STATUS "  cmake --build . --target check    # Run tests")
message(STATUS "  cmake --build . --target format   # Format code")
message(STATUS "")
message(STATUS "Python delegation targets:")
message(STATUS "  cmake --build . --target build-all  # Via make.py")
message(STATUS "  cmake --build . --target test-all   # Via make.py")
message(STATUS "  cmake --build . --target bench-all  # Via make.py")
message(STATUS "")
message(STATUS "⚠️  REMEMBER: Prefer Python workflow (see WORKFLOW_OBLIGATORIO.md)")
message(STATUS "")

# =============================================================================
# EOF
# =============================================================================

