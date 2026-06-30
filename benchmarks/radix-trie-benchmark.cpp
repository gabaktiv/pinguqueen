//
// Radix trie benchmarks.
//

/*
Radix trie benchmarks measure insertion, lookup and deletion only.

The generated keys avoid file I/O so the measured time belongs to the trie
operations. The benchmark cases cover small tries, adaptive node growth,
shared-prefix paths and deletion from prepared tries.
*/

#include <benchmark/benchmark.h>
#include <map>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "radix-trie/file-info.hpp"
#include "radix-trie/radix-trie.hpp"

namespace {

    struct TrieInput {
        std::vector<std::string> keys;
        std::vector<std::unique_ptr<pinguqueen::intern::FileInfo>> metadata;
    };

    struct PreparedTrie {
        std::unique_ptr<pinguqueen::intern::Node> root = nullptr;
        std::vector<std::unique_ptr<pinguqueen::intern::FileInfo>> metadata;

        PreparedTrie() = default;
        ~PreparedTrie() = default;

        PreparedTrie(PreparedTrie const&) = delete;
        PreparedTrie& operator=(PreparedTrie const&) = delete;
        PreparedTrie(PreparedTrie&& other) noexcept = default;
        PreparedTrie& operator=(PreparedTrie&& other) noexcept = default;
    };

    [[nodiscard]] std::unique_ptr<pinguqueen::intern::FileInfo> makeFileInfo(
        std::string name,
        pinguqueen::u32 size
    ) {
        auto info = std::make_unique<pinguqueen::intern::FileInfo>();
        info->file_name = std::move(name);
        info->file_size_bytes = size;
        return info;
    }

    [[nodiscard]] std::string makeIndexedPath(
        std::size_t index
    ) {
        std::ostringstream key;
        key << "workspace/project/module_" << (index % 64)
            << "/file_" << index << ".cpp";
        return key.str();
    }

    [[nodiscard]] std::string makeSharedPrefixKey(
        std::size_t index
    ) {
        std::ostringstream key;
        key << "workspace/project/src/generated/deep/shared/prefix/file_"
            << index << ".hpp";
        return key.str();
    }

    [[nodiscard]] std::string makeTwoByteAdaptiveKey(
        unsigned char suffix
    ) {
        std::string key = "p";
        key.push_back(static_cast<char>(suffix));
        return key;
    }

    [[nodiscard]] TrieInput makeIndexedPathInput(
        std::size_t key_count
    ) {
        TrieInput input;
        input.keys.reserve(key_count);
        input.metadata.reserve(key_count);

        for (std::size_t index = 0; index < key_count; ++index) {
            input.keys.push_back(makeIndexedPath(index));
            input.metadata.push_back(makeFileInfo(
                input.keys.back(),
                static_cast<pinguqueen::u32>(index + 1)
            ));
        }

        return input;
    }

    [[nodiscard]] TrieInput makeSharedPrefixInput(
        std::size_t key_count
    ) {
        TrieInput input;
        input.keys.reserve(key_count);
        input.metadata.reserve(key_count);

        for (std::size_t index = 0; index < key_count; ++index) {
            input.keys.push_back(makeSharedPrefixKey(index));
            input.metadata.push_back(makeFileInfo(
                input.keys.back(),
                static_cast<pinguqueen::u32>(index + 1)
            ));
        }

        return input;
    }

    [[nodiscard]] TrieInput makeAdaptiveNodeInput(
        unsigned char key_count
    ) {
        TrieInput input;
        input.keys.reserve(key_count);
        input.metadata.reserve(key_count);

        for (unsigned char suffix = 1; suffix <= key_count; ++suffix) {
            input.keys.push_back(makeTwoByteAdaptiveKey(suffix));
            input.metadata.push_back(makeFileInfo(
                input.keys.back(),
                static_cast<pinguqueen::u32>(suffix)
            ));
        }

        return input;
    }

    [[nodiscard]] PreparedTrie buildTrie(
        TrieInput const& input
    ) {
        PreparedTrie trie;
        trie.metadata.reserve(input.keys.size());

        for (std::size_t index = 0; index < input.keys.size(); ++index) {
            trie.metadata.push_back(makeFileInfo(
                input.keys[index],
                static_cast<pinguqueen::u32>(index + 1)
            ));
            pinguqueen::intern::RadixTrie::insert_node(
                trie.root,
                input.keys[index],
                trie.metadata.back().get(),
                0
            );
        }

        return trie;
    }

    [[nodiscard]] pinguqueen::intern::LeafNode const* asLeaf(
        pinguqueen::intern::Node const* node
    ) {
        if (node == nullptr || !node->is_leaf()) {
            return nullptr;
        }

        return static_cast<pinguqueen::intern::LeafNode const*>(node);
    }

    [[nodiscard]] pinguqueen::intern::LeafNode const* findLeaf(
        pinguqueen::intern::Node* root,
        std::string_view key
    ) {
        pinguqueen::intern::Node* current = root;
        pinguqueen::u32 depth = 0;

        while (current != nullptr) {
            if (pinguqueen::intern::LeafNode const* leaf = asLeaf(current)) {
                return leaf->_full_key == key ? leaf : nullptr;
            }

            depth += current->_prefix_skip_length;
            if (depth >= key.size()) {
                return nullptr;
            }

            current = current->find_child(static_cast<pinguqueen::u8>(key[depth]));
            ++depth;
        }

        return nullptr;
    }

    void runInsertBenchmark(
        benchmark::State& state,
        TrieInput const& input
    ) {
        for (auto _: state) {
            std::unique_ptr<pinguqueen::intern::Node> root = nullptr;

            for (std::size_t index = 0; index < input.keys.size(); ++index) {
                pinguqueen::intern::RadixTrie::insert_node(
                    root,
                    input.keys[index],
                    input.metadata[index].get(),
                    0
                );
            }

            benchmark::DoNotOptimize(root.get());
            benchmark::ClobberMemory();

        }

        state.SetItemsProcessed(
            static_cast<std::int64_t>(state.iterations())
            * static_cast<std::int64_t>(input.keys.size())
        );
    }

    void runLookupBenchmark(
        benchmark::State& state,
        TrieInput const& input
    ) {
        PreparedTrie trie = buildTrie(input);

        for (auto _: state) {
            for (std::string const& key : input.keys) {
                pinguqueen::intern::LeafNode const* leaf = findLeaf(trie.root.get(), key);
                benchmark::DoNotOptimize(leaf);
            }

            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(
            static_cast<std::int64_t>(state.iterations())
            * static_cast<std::int64_t>(input.keys.size())
        );
    }

    void runDirectChildLookupBenchmark(
        benchmark::State& state,
        unsigned char key_count
    ) {
        TrieInput const input = makeAdaptiveNodeInput(key_count);
        PreparedTrie trie = buildTrie(input);

        for (auto _: state) {
            for (unsigned char suffix = 1; suffix <= key_count; ++suffix) {
                pinguqueen::intern::Node* child = trie.root->find_child(suffix);
                benchmark::DoNotOptimize(child);
            }

            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(
            static_cast<std::int64_t>(state.iterations())
            * static_cast<std::int64_t>(key_count)
        );
    }

    void runDeleteBenchmark(
        benchmark::State& state,
        TrieInput const& input
    ) {
        for (auto _: state) {
            state.PauseTiming();
            PreparedTrie trie = buildTrie(input);
            state.ResumeTiming();

            for (std::string const& key : input.keys) {
                pinguqueen::intern::RadixTrie::delete_node(trie.root, key, 0);
            }

            benchmark::DoNotOptimize(trie.root.get());
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(
            static_cast<std::int64_t>(state.iterations())
            * static_cast<std::int64_t>(input.keys.size())
        );
    }

    void BM_InsertIndexedPaths(
        benchmark::State& state
    ) {
        TrieInput const input = makeIndexedPathInput(
            static_cast<std::size_t>(state.range(0))
        );

        runInsertBenchmark(state, input);
    }

    void BM_InsertSharedPrefixPaths(
        benchmark::State& state
    ) {
        TrieInput const input = makeSharedPrefixInput(
            static_cast<std::size_t>(state.range(0))
        );

        runInsertBenchmark(state, input);
    }

    void BM_InsertAdaptiveNodeGrowth(
        benchmark::State& state
    ) {
        TrieInput const input = makeAdaptiveNodeInput(
            static_cast<unsigned char>(state.range(0))
        );

        runInsertBenchmark(state, input);
    }

    void BM_LookupIndexedPaths(
        benchmark::State& state
    ) {
        TrieInput const input = makeIndexedPathInput(
            static_cast<std::size_t>(state.range(0))
        );

        runLookupBenchmark(state, input);
    }

    void BM_LookupSharedPrefixPaths(
        benchmark::State& state
    ) {
        TrieInput const input = makeSharedPrefixInput(
            static_cast<std::size_t>(state.range(0))
        );

        runLookupBenchmark(state, input);
    }

    void BM_DirectChildLookupAfterAdaptiveGrowth(
        benchmark::State& state
    ) {
        runDirectChildLookupBenchmark(
            state,
            static_cast<unsigned char>(state.range(0))
        );
    }

    void BM_DeleteIndexedPaths(
        benchmark::State& state
    ) {
        TrieInput const input = makeIndexedPathInput(
            static_cast<std::size_t>(state.range(0))
        );

        runDeleteBenchmark(state, input);
    }

    void BM_DeleteSharedPrefixPaths(
        benchmark::State& state
    ) {
        TrieInput const input = makeSharedPrefixInput(
            static_cast<std::size_t>(state.range(0))
        );

        runDeleteBenchmark(state, input);
    }

    void BM_Lookup_StdMap(benchmark::State& state) {
        auto const input = makeSharedPrefixInput(static_cast<std::size_t>(state.range(0)));

        std::map<std::string, pinguqueen::intern::FileInfo*> standard_map;
        for (std::size_t index = 0; index < input.keys.size(); ++index) {
            standard_map[input.keys[index]] = input.metadata[index].get();
        }

        for (auto _: state) {
            for (std::string const& key : input.keys) {
                auto it = standard_map.find(key);
                benchmark::DoNotOptimize(it);
            }
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * input.keys.size());
    }

} // namespace

// ============================================================================
// REGISTRIERUNGEN GANZ UNTEN:
// ============================================================================

BENCHMARK(BM_InsertIndexedPaths)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);
BENCHMARK(BM_InsertSharedPrefixPaths)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);
BENCHMARK(BM_InsertAdaptiveNodeGrowth)->Arg(5)->Arg(17)->Arg(49);
BENCHMARK(BM_LookupIndexedPaths)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);
BENCHMARK(BM_LookupSharedPrefixPaths)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);
BENCHMARK(BM_DirectChildLookupAfterAdaptiveGrowth)->Arg(5)->Arg(17)->Arg(49);
BENCHMARK(BM_DeleteIndexedPaths)->Arg(10)->Arg(100)->Arg(1000);
BENCHMARK(BM_DeleteSharedPrefixPaths)->Arg(10)->Arg(100)->Arg(1000);

BENCHMARK(BM_Lookup_StdMap)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
