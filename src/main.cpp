#include <iostream>
#include "global.hpp"
#include "file-handling/ArtIndexBuilder.hpp"
#include "radix-trie/radix-trie.hpp"





int main(int argc, char *argv[]) {

    pinguqueen::file::ArtIndexBuilder cool{};
    auto& art = cool.debug_art();

    std::vector<std::string> keys = art.get_all_paths_with_prefix("");

    for (auto const& key : keys) {
        std::cout << key.substr(0, key.find('\0')) << '\n';
    }

    pinguqueen::datastructs::RadixTrie art2 = std::move(cool.debug_art());

    return 0;
}