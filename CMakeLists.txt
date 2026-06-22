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

target_include_directories(trie_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Tests
enable_testing()

add_executable(trie_tests
        tests/main-test.cpp
)

target_link_libraries(trie_tests PRIVATE
        trie_core
)

add_test(NAME trie_tests COMMAND trie_tests)

add_executable(radix_trie_tests
        tests/radix-test.cpp
)

target_link_libraries(radix_trie_tests PRIVATE
        trie_core
)

set(RADIX_TRIE_TESTS
        insert_empty_creates_leaf
        split_shared_prefix_type_is_node4
        split_shared_prefix_skip_length
        split_shared_prefix_child_count
        split_shared_prefix_left_leaf_reachable
        split_shared_prefix_right_leaf_reachable
        split_shared_prefix_missing_leaf_not_found
        node4_three_children_keeps_first_leaf
        grow_node4_to_node16_type
        grow_node4_to_node16_child_count
        grow_node4_to_node16_keeps_edge_children
        grow_node16_to_node48_type
        grow_node16_to_node48_child_count
        grow_node16_to_node48_keeps_edge_children
        node48_before_grow_has_expected_prefix
        node48_before_grow_has_expected_child_count
        node48_before_grow_edge_children_reachable
        grow_node48_to_node256_type
        grow_node48_to_node256_child_count
        grow_node48_to_node256_preserves_prefix_skip
        grow_node48_to_node256_maps_first_child_directly
        grow_node48_to_node256_maps_last_child_directly
        grow_node48_to_node256_first_leaf_reachable
        grow_node48_to_node256_last_leaf_reachable
        delete_node4_removes_target_leaf
        delete_node4_keeps_sibling_leaves
        shrink_node16_to_node4_type
        shrink_node16_to_node4_child_count
        shrink_node48_to_node16_type
        shrink_node48_to_node16_child_count
        delete_node256_single_child_reduces_count
        delete_node256_single_child_removes_direct_child
        shrink_node256_to_node48_type
        shrink_node256_to_node48_child_count
)

foreach(test_name IN LISTS RADIX_TRIE_TESTS)
        add_test(NAME radix_trie.${test_name} COMMAND radix_trie_tests ${test_name})
endforeach()

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

        target_link_libraries(radix_trie_benchmarks PRIVATE
                trie_core
                benchmark::benchmark_main
        )
else()
        message(STATUS "Google Benchmark target not available; radix_trie_benchmarks target disabled")
endif()
