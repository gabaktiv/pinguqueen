cmake_minimum_required(VERSION 3.16)

project(pinguqueen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-Härtungen
add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
)

# Debug-Härtungen
add_compile_options(
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

add_link_options(
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# Core-Library
add_library(trie_core
        src/radix-trie/radix-trie.cpp
)

target_include_directories(trie_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Tests
enable_testing()

find_package(GTest REQUIRED)

add_executable(trie_tests
        test/main-test.cpp
)

target_link_libraries(trie_tests PRIVATE
        trie_core
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(trie_tests)