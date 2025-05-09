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

file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")
list(REMOVE_ITEM PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_library(redish_lib ${PROJECT_SOURCES})

target_include_directories(redish_lib PUBLIC include)

add_executable(redish src/main.cpp)
target_link_libraries(redish PRIVATE redish_lib)

file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE include)
target_link_libraries(tests PRIVATE redish_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
