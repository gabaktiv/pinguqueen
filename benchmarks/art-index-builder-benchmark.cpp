//
// ArtIndexBuilder benchmarks.
//

/*
 * ArtIndexBuilder benchmarks measure the filesystem-scan time and
 * post-scan lookup performance across directory sizes and shapes.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "../src/file-handling/ArtIndexBuilder.hpp"
#include "../src/core/file-info.hpp"

namespace {

namespace fs = std::filesystem;

struct TempDir {
    fs::path path;

    TempDir()
    {
        char tmpl[] = "/tmp/art-bench-XXXXXX";
        char* dir = mkdtemp(tmpl);
        if (dir == nullptr) {
            std::abort();
        }
        path = fs::weakly_canonical(dir);
    }

    ~TempDir()
    {
        fs::remove_all(path);
    }

    TempDir(TempDir const&) = delete;
    TempDir& operator=(TempDir const&) = delete;
};

void create_file(fs::path const& base, std::string const& relative, std::size_t size)
{
    fs::path full = base / relative;
    fs::create_directories(full.parent_path());
    std::ofstream out(full, std::ios::binary);
    std::string content(size, 'x');
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

std::vector<std::string> make_flat_paths(std::size_t count)
{
    std::vector<std::string> paths;
    paths.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::ostringstream ss;
        ss << "file_" << i << ".bin";
        paths.push_back(std::move(ss).str());
    }
    return paths;
}

std::vector<std::string> make_nested_paths(std::size_t count)
{
    std::vector<std::string> paths;
    paths.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::ostringstream ss;
        ss << "module_" << (i % 64) << "/sub_" << ((i * 17) % 97)
           << "/file_" << i << ".cpp";
        paths.push_back(std::move(ss).str());
    }
    return paths;
}

std::vector<std::string> make_deeply_nested_paths(std::size_t count)
{
    std::vector<std::string> paths;
    paths.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::ostringstream ss;
        ss << "workspace/project/src/generated/deep/path/tree/data_"
           << i << ".dat";
        paths.push_back(std::move(ss).str());
    }
    return paths;
}

void populate_directory(fs::path const& dir, std::vector<std::string> const& paths)
{
    for (auto const& rel : paths) {
        create_file(dir, rel, 128);
    }
}

void run_construction_benchmark(
    benchmark::State& state,
    std::vector<std::string> const& paths
) {
    TempDir tmp;

    populate_directory(tmp.path, paths);

    fs::path orig = fs::current_path();
    fs::current_path(tmp.path);

    for (auto _ : state) {
        pinguqueen::file::ArtIndexBuilder builder;
        benchmark::DoNotOptimize(builder);
        benchmark::ClobberMemory();
    }

    fs::current_path(orig);

    state.SetItemsProcessed(
        static_cast<std::int64_t>(state.iterations())
        * static_cast<std::int64_t>(paths.size())
    );
}

void run_lookup_benchmark(
    benchmark::State& state,
    std::vector<std::string> const& paths
) {
    TempDir tmp;

    populate_directory(tmp.path, paths);

    fs::path orig = fs::current_path();
    fs::current_path(tmp.path);

    pinguqueen::file::ArtIndexBuilder builder;
    pinguqueen::datastructs::AdaptiveRadixTrie& trie = builder.create_art();

    for (auto _ : state) {
        for (auto const& key : paths) {
            pinguqueen::core::FileInfo* info = trie.search(key);
            benchmark::DoNotOptimize(info);
        }
        benchmark::ClobberMemory();
    }

    fs::current_path(orig);

    state.SetItemsProcessed(
        static_cast<std::int64_t>(state.iterations())
        * static_cast<std::int64_t>(paths.size())
    );
}

void BM_ConstructFlat10(benchmark::State& state)
{
    run_construction_benchmark(state, make_flat_paths(10));
}

void BM_ConstructFlat100(benchmark::State& state)
{
    run_construction_benchmark(state, make_flat_paths(100));
}

void BM_ConstructFlat1000(benchmark::State& state)
{
    run_construction_benchmark(state, make_flat_paths(1000));
}

void BM_ConstructNested10(benchmark::State& state)
{
    run_construction_benchmark(state, make_nested_paths(10));
}

void BM_ConstructNested100(benchmark::State& state)
{
    run_construction_benchmark(state, make_nested_paths(100));
}

void BM_ConstructNested1000(benchmark::State& state)
{
    run_construction_benchmark(state, make_nested_paths(1000));
}

void BM_ConstructDeeplyNested10(benchmark::State& state)
{
    run_construction_benchmark(state, make_deeply_nested_paths(10));
}

void BM_ConstructDeeplyNested100(benchmark::State& state)
{
    run_construction_benchmark(state, make_deeply_nested_paths(100));
}

void BM_ConstructDeeplyNested1000(benchmark::State& state)
{
    run_construction_benchmark(state, make_deeply_nested_paths(1000));
}

void BM_LookupNested100(benchmark::State& state)
{
    run_lookup_benchmark(state, make_nested_paths(100));
}

void BM_LookupNested1000(benchmark::State& state)
{
    run_lookup_benchmark(state, make_nested_paths(1000));
}

void BM_LookupNested10000(benchmark::State& state)
{
    run_lookup_benchmark(state, make_nested_paths(10000));
}

} // namespace

// ============================================================================
// Registrierungen
// ============================================================================

BENCHMARK(BM_ConstructFlat10);
BENCHMARK(BM_ConstructFlat100);
BENCHMARK(BM_ConstructFlat1000);

BENCHMARK(BM_ConstructNested10);
BENCHMARK(BM_ConstructNested100);
BENCHMARK(BM_ConstructNested1000);

BENCHMARK(BM_ConstructDeeplyNested10);
BENCHMARK(BM_ConstructDeeplyNested100);
BENCHMARK(BM_ConstructDeeplyNested1000);

BENCHMARK(BM_LookupNested100);
BENCHMARK(BM_LookupNested1000);
BENCHMARK(BM_LookupNested10000);

BENCHMARK_MAIN();
