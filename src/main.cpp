#include "core/visualiser.hpp"
#include "file-handling/ArtIndexBuilder.hpp"

int main( [[maybe_unused]]int argc, [[maybe_unused]]char *argv[] ) {

    pinguqueen::file::ArtIndexBuilder builder{};
    pinguqueen::datastructs::AdaptiveRadixTrie& trie = builder.create_art();

    pinguqueen::core::Visualiser vis(trie);
    vis.run();

    return 0;
}