cmake_minimum_required(VERSION 3.22)

project(
    libbag
    VERSION 0.0.1
    LANGUAGES C CXX
)

include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(catch2)

set(CMAKE_CXX_STANDARD 20)
enable_testing()

set(SOURCE_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code)
set(SOURCE_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library)
set(SOURCE_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

add_subdirectory(${SOURCE_LIBRARY_DIR})
add_subdirectory(${SOURCE_CODE_DIR})
add_subdirectory(${SOURCE_TEST_DIR})