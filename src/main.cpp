#include <iostream>
#include "global.hpp"
#include "file-handling/ArtIndexBuilder.hpp"
#include "radix-trie/radix-trie.hpp"





int main(int argc, char *argv[]) {

    std::ios_base::sync_with_stdio( false );

    pinguqueen::file::ArtIndexBuilder cool{};
    auto& art = cool.debug_art();

    std::vector<std::string> keys = art.get_all_paths_with_prefix("");

    for (auto const& key : keys) {
        std::cout << key.substr(0, key.find('\0')) << std::endl;
    }

    //readline c libary

    //pinguqueen::datastructs::RadixTrie art2 = std::move(cool.debug_art());

    return 0;
}