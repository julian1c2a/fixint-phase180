# Tests subdirectory for Phase 1.75
# Priority 1: Build validation + constructors

# Discover all test files
file(GLOB TEST_SOURCES "*.cpp")

foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_20)
    
    # Add as CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
