# =============================================================================
# TestConfig.cmake - Configuración de testing con CTest
# =============================================================================
#
# Configura CTest para ejecutar tests automáticos del proyecto int128
#
# Características:
#   - Descubrimiento automático de tests
#   - Timeouts configurables
#   - Soporte para cobertura de código (gcov/lcov)
#   - Integración con sanitizers
#   - Reporte de resultados
#
# Uso:
#   cmake -DINT128_ENABLE_TESTING=ON ..
#   cmake --build .
#   ctest --output-on-failure
#
# =============================================================================

option(INT128_ENABLE_TESTING "Enable testing with CTest" ON)
option(INT128_ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(INT128_ENABLE_TESTING)
    enable_testing()
    message(STATUS "Testing enabled with CTest")
    
    # =============================================================================
    # Configuración de timeouts
    # =============================================================================
    
    # Timeout por defecto para tests (segundos)
    set(INT128_TEST_TIMEOUT 30 CACHE STRING "Default test timeout in seconds")
    
    # Timeout para benchmarks (más largo)
    set(INT128_BENCH_TIMEOUT 120 CACHE STRING "Default benchmark timeout in seconds")
    
    message(STATUS "Test timeout: ${INT128_TEST_TIMEOUT}s")
    message(STATUS "Benchmark timeout: ${INT128_BENCH_TIMEOUT}s")
    
    # =============================================================================
    # Cobertura de código (gcov/lcov)
    # =============================================================================
    
    if(INT128_ENABLE_COVERAGE)
        if(INT128_COMPILER_ID STREQUAL "gcc" OR INT128_COMPILER_ID STREQUAL "clang")
            message(STATUS "Code coverage enabled")
            
            # Flags de cobertura
            set(COVERAGE_FLAGS --coverage -fprofile-arcs -ftest-coverage)
            add_compile_options(${COVERAGE_FLAGS})
            add_link_options(${COVERAGE_FLAGS})
            
            # Buscar herramientas de cobertura
            find_program(LCOV_EXECUTABLE NAMES lcov)
            find_program(GENHTML_EXECUTABLE NAMES genhtml)
            
            if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
                message(STATUS "Found lcov: ${LCOV_EXECUTABLE}")
                message(STATUS "Found genhtml: ${GENHTML_EXECUTABLE}")
                
                # Target para generar reporte de cobertura
                add_custom_target(coverage
                    COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file coverage.info
                    COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
                    COMMAND ${GENHTML_EXECUTABLE} coverage.info --output-directory coverage_report
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                    COMMENT "Generating code coverage report..."
                    VERBATIM
                )
                
                message(STATUS "Target 'coverage' created")
                message(STATUS "View report: ${CMAKE_BINARY_DIR}/coverage_report/index.html")
            else()
                message(WARNING "lcov/genhtml not found - coverage target not available")
                message(STATUS "Install: pacman -S lcov (MSYS2) or apt install lcov (Linux)")
            endif()
        else()
            message(WARNING "Code coverage only supported for GCC/Clang")
        endif()
    endif()
    
    # =============================================================================
    # Helper function: Agregar test con configuración estándar
    # =============================================================================
    
    function(int128_add_test test_name test_executable)
        # Argumentos opcionales: TIMEOUT, LABELS
        set(options "")
        set(oneValueArgs TIMEOUT)
        set(multiValueArgs LABELS)
        cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
        
        # Usar timeout por defecto si no se especifica
        if(NOT ARG_TIMEOUT)
            set(ARG_TIMEOUT ${INT128_TEST_TIMEOUT})
        endif()
        
        # Crear test
        add_test(NAME ${test_name} COMMAND ${test_executable})
        
        # Configurar propiedades del test
        set_tests_properties(${test_name} PROPERTIES
            TIMEOUT ${ARG_TIMEOUT}
        )
        
        # Agregar labels si se especifican
        if(ARG_LABELS)
            set_tests_properties(${test_name} PROPERTIES
                LABELS "${ARG_LABELS}"
            )
        endif()
    endfunction()
    
    # =============================================================================
    # Helper function: Agregar benchmark con configuración estándar
    # =============================================================================
    
    function(int128_add_benchmark bench_name bench_executable)
        # Argumentos opcionales: TIMEOUT, LABELS
        set(options "")
        set(oneValueArgs TIMEOUT)
        set(multiValueArgs LABELS)
        cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
        
        # Usar timeout de benchmark si no se especifica
        if(NOT ARG_TIMEOUT)
            set(ARG_TIMEOUT ${INT128_BENCH_TIMEOUT})
        endif()
        
        # Crear test
        add_test(NAME ${bench_name} COMMAND ${bench_executable})
        
        # Configurar propiedades
        set_tests_properties(${bench_name} PROPERTIES
            TIMEOUT ${ARG_TIMEOUT}
            LABELS "benchmark"
        )
        
        # Agregar labels adicionales si se especifican
        if(ARG_LABELS)
            set_tests_properties(${bench_name} PROPERTIES
                LABELS "benchmark;${ARG_LABELS}"
            )
        endif()
    endfunction()
    
    # =============================================================================
    # Configuración de CTest
    # =============================================================================
    
    # Configurar opciones de CTest
    set(CMAKE_CTEST_COMMAND ctest)
    set(CMAKE_CTEST_ARGUMENTS --output-on-failure --verbose)
    
    # Crear target 'check' para ejecutar todos los tests
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} ${CMAKE_CTEST_ARGUMENTS}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all tests with CTest..."
        USES_TERMINAL
    )
    
    # Crear targets por categoría de tests
    add_custom_target(check-tests
        COMMAND ${CMAKE_CTEST_COMMAND} -L "^test$" ${CMAKE_CTEST_ARGUMENTS}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests only..."
        USES_TERMINAL
    )
    
    add_custom_target(check-benchmarks
        COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark" ${CMAKE_CTEST_ARGUMENTS}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running benchmarks only..."
        USES_TERMINAL
    )
    
    # =============================================================================
    # Información de configuración
    # =============================================================================
    
    message(STATUS "")
    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "  TESTING CONFIGURATION")
    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "  CTest:           ✅ ENABLED")
    message(STATUS "  Test timeout:    ${INT128_TEST_TIMEOUT}s")
    message(STATUS "  Bench timeout:   ${INT128_BENCH_TIMEOUT}s")
    
    if(INT128_ENABLE_COVERAGE)
        if(LCOV_EXECUTABLE)
            message(STATUS "  Coverage:        ✅ ENABLED (lcov)")
        else()
            message(STATUS "  Coverage:        ⚠️  REQUESTED (lcov not found)")
        endif()
    else()
        message(STATUS "  Coverage:        ❌ DISABLED")
    endif()
    
    message(STATUS "")
    message(STATUS "  Available targets:")
    message(STATUS "    - check           : Run all tests")
    message(STATUS "    - check-tests     : Run tests only (no benchmarks)")
    message(STATUS "    - check-benchmarks: Run benchmarks only")
    
    if(INT128_ENABLE_COVERAGE AND LCOV_EXECUTABLE)
        message(STATUS "    - coverage        : Generate coverage report")
    endif()
    
    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "")
    
else()
    message(STATUS "Testing disabled (use -DINT128_ENABLE_TESTING=ON to enable)")
endif()

# =============================================================================
# EOF
# =============================================================================
