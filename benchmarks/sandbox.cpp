#include "../src/radix-trie/radix-trie.hpp"
#include "../src/core/file-info.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

struct PreparedTrie {
    pinguqueen::intern::RadixTrie trie;

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

std::unique_ptr<pinguqueen::core::FileInfo> makeFileInfo(
    std::string name,
    std::uint32_t size
) {
    auto info = std::make_unique<pinguqueen::core::FileInfo>();
    info->_file_name = std::move(name);
    info->_file_size_bytes = size;
    return info;
}

struct TrieInput {
    std::vector<std::string> keys;
    std::vector<std::unique_ptr<pinguqueen::core::FileInfo>> metadata;
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
        PreparedTrie pt;

        for (std::size_t i = 0; i < input.keys.size(); ++i) {
            pt.trie.insert(
                input.keys[i],
                makeFileInfo(input.keys[i], static_cast<std::uint32_t>(i + 1))
            );
        }

        return pt;
    }

void run_delete_like_benchmark(std::size_t key_count, std::size_t iterations) {
    TrieInput const input = makeSharedPrefixInput(key_count);

    for (std::size_t iter = 0; iter < iterations; ++iter) {

        PreparedTrie pt = buildTrie(input);

        for (std::size_t i = 0; i < input.keys.size(); ++i) {
            pt.trie.remove(input.keys[i]);
        }

        /*
            WICHTIGER TEST:

            Wenn das Programm OHNE diese Zeile crasht,
            aber MIT dieser Zeile sauber durchlaeuft,
            dann zeigt pt.trie.root_node() nach dem letzten remove()
            wahrscheinlich noch auf freigegebenen Speicher.

            Dann liegt der Fehler sehr wahrscheinlich in delete_node().
        */

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
