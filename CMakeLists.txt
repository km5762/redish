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

# Include project headers
include_directories(include)

# Find all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Find all test files
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

# Create a single test executable
add_executable(all_tests ${TEST_SOURCES})

# Link GoogleTest
target_link_libraries(all_tests GTest::gtest_main)

# Discover tests
include(GoogleTest)
gtest_discover_tests(all_tests)
