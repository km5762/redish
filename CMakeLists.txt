cmake_minimum_required(VERSION 3.10)
project(redish VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG 6910c9d9165801d8827d628cb72eb7ea9dd538c5
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include_directories(include)
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

include(GoogleTest)
file(GLOB TEST_SOURCES tests/*.cpp)
foreach (TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    set(SRC_FILE "${CMAKE_SOURCE_DIR}/src/${TEST_NAME}.cpp")
    if (NOT EXISTS ${SRC_FILE})
        message(FATAL_ERROR "Test for '${TEST_NAME}' but no implementation at ${SRC_FILE}")
    endif ()
    set(INCLUDE_FILE "${CMAKE_SOURCE_DIR}/include/${TEST_NAME}.h")
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${SRC_FILE})
    target_link_libraries(${TEST_NAME} GTest::gtest_main)
    gtest_discover_tests(${TEST_NAME})
endforeach ()
