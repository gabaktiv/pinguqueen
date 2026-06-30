#include "../src/radix-trie/radix-trie.hpp"
#include "../src/radix-trie/file-info.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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

std::string makeSharedPrefixKey(std::size_t index) {
    std::ostringstream key;

    return key.str();
}

std::unique_ptr<pinguqueen::intern::FileInfo> makeFileInfo(
    std::string name,
    std::uint32_t size
) {
    auto info = std::make_unique<pinguqueen::intern::FileInfo>();
    info->file_name = std::move(name);
    info->file_size_bytes = size;
    return info;
}

struct TrieInput {
    std::vector<std::string> keys;
    std::vector<std::unique_ptr<pinguqueen::intern::FileInfo>> metadata;
};

TrieInput makeSharedPrefixInput(std::size_t key_count) {
    TrieInput input;
    input.keys.reserve(key_count);
    input.metadata.reserve(key_count);

    for (std::size_t i = 0; i < key_count; ++i) {
        input.keys.push_back(makeSharedPrefixKey(i));
        input.metadata.push_back(makeFileInfo(
            input.keys.back(),
            static_cast<std::uint32_t>(i + 1)
        ));
    }

    return input;
}

PreparedTrie buildTrie(TrieInput const& input) {
    PreparedTrie trie;
    trie.metadata.reserve(input.keys.size());

    for (std::size_t i = 0; i < input.keys.size(); ++i) {
        trie.metadata.push_back(makeFileInfo(
            input.keys[i],
            static_cast<std::uint32_t>(i + 1)
        ));

        pinguqueen::intern::RadixTrie::insert_node(
            trie.root,
            input.keys[i],
            trie.metadata.back().get(),
            0
        );
    }

    return trie;
}

void run_delete_like_benchmark(std::size_t key_count, std::size_t iterations) {
    TrieInput const input = makeSharedPrefixInput(key_count);

    for (std::size_t iter = 0; iter < iterations; ++iter) {

        PreparedTrie trie = buildTrie(input);

        for (std::size_t i = 0; i < input.keys.size(); ++i) {


            pinguqueen::intern::RadixTrie::delete_node(
                trie.root,
                input.keys[i],
                0
            );
        }



        /*
            WICHTIGER TEST:

            Wenn das Programm OHNE diese Zeile crasht,
            aber MIT dieser Zeile sauber durchlaeuft,
            dann zeigt trie.root nach dem letzten delete_node()
            wahrscheinlich noch auf freigegebenen Speicher.

            Dann liegt der Fehler sehr wahrscheinlich in delete_node().
        */

        //trie.root = nullptr;

        std::cout << "Iteration beendet, Destructor kommt jetzt..." << std::endl;
    }
}

int main() {
    try {
        while (true) {
            run_delete_like_benchmark(1000, 1);
        }
        std::cout << "\nGruen: Test komplett durchgelaufen.\n";
    }
    catch (std::exception const& e) {
        std::cerr << "Exception gefangen: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unbekannte Exception gefangen." << std::endl;
        return 1;
    }

    return 0;
}
