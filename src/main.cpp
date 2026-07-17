#include "global.hpp"
#include "core/visualiser.hpp"
#include "file-handling/ArtIndexBuilder.hpp"
#include "adaptive-radix-trie/adaptive-radix-trie.hpp"

int main(int argc, char *argv[]) {

    pinguqueen::file::ArtIndexBuilder builder{};
    auto& trie = builder.art();

    pinguqueen::core::Visualiser vis(trie);
    vis.run();

    return 0;
}