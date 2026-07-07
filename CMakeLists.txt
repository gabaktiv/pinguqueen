cmake_minimum_required(VERSION 3.16)

project(pinguqueen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PINGUQUEEN_ENABLE_SANITIZERS "Build debug targets with address/undefined sanitizers" OFF)

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
if (PINGUQUEEN_ENABLE_SANITIZERS)
        add_compile_options(
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        )

        add_link_options(
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        )
endif()

# Core-Library
add_library(trie_core
        src/radix-trie/radix-trie.cpp
        src/radix-trie/node.cpp
)
add_library(file_core
        src/file-handling/ArtIndexBuilder.cpp
        src/core/file-info.cpp
)
target_include_directories(trie_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# FTXUI
include(FetchContent)

FetchContent_Declare(
        ftxui
        GIT_REPOSITORY  https://github.com/ArthurSonzogni/FTXUI.git
        GIT_TAG         v5.0.0
)

FetchContent_MakeAvailable(ftxui)

add_executable(pinguqueen
        src/main.cpp
        src/core/visualiser.cpp
)

target_link_libraries(pinguqueen PRIVATE
        file_core
        trie_core
        ftxui::screen
        ftxui::dom
        ftxui::component
)

# Tests
enable_testing()

find_package(GTest CONFIG QUIET)

if (NOT GTest_FOUND)
        include(FetchContent)

        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG v1.15.2
        )

        FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(radix_trie_tests
        tests/radix-test.cpp
)

add_executable(sandbox
        benchmarks/sandbox.cpp
)
target_link_libraries(sandbox PRIVATE
        trie_core
)


add_executable(art_index_builder_tests
        tests/art-index-builder-test.cpp
)
target_link_libraries(art_index_builder_tests PRIVATE
        file_core
        trie_core
        GTest::gtest_main
)

gtest_discover_tests(art_index_builder_tests)

target_link_libraries(radix_trie_tests PRIVATE
        trie_core
        GTest::gtest_main
)

gtest_discover_tests(radix_trie_tests)

# Benchmarks
find_package(benchmark CONFIG QUIET)

if (NOT benchmark_FOUND)
        include(FetchContent)

        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
                google_benchmark
                GIT_REPOSITORY https://github.com/google/benchmark.git
                GIT_TAG v1.9.1
        )

        FetchContent_MakeAvailable(google_benchmark)
endif()

if (TARGET benchmark::benchmark_main)
        add_executable(radix_trie_benchmarks
                benchmarks/radix-trie-benchmark.cpp
        )

        add_executable(art_index_builder_benchmarks
                benchmarks/art-index-builder-benchmark.cpp
        )

        target_link_libraries(radix_trie_benchmarks PRIVATE
                trie_core
                benchmark::benchmark_main
        )

        target_link_libraries(art_index_builder_benchmarks PRIVATE
                file_core
                trie_core
                benchmark::benchmark_main
        )
else()
        message(STATUS "Google Benchmark target not available; radix_trie_benchmarks target disabled")
endif()

install(TARGETS pinguqueen RUNTIME DESTINATION bin)
