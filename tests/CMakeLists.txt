# Tests subdirectory for Phase 1.75
# Priority 1: Build validation + constructors

# Discover all test files.
#
# T7.4 (auditoria 23 ago 2026): sin CONFIGURE_DEPENDS, CMake evalua el glob una
# sola vez, al configurar. Anyadir un test nuevo no regeneraba el build y el
# fichero pasaba desapercibido hasta que alguien borraba la cache a mano.
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_20)
    
    # Set output directory for test executables
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build_tests)
    
    
    # Add as CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
