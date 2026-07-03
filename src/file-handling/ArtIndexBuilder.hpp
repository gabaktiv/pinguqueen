//
// Created by gabriel on 7/2/26.
//

#pragma once
#include <filesystem>
#include "../radix-trie/radix-trie.hpp"
#include "../core/file-info.hpp"

namespace pinguqueen::file {


    class ArtIndexHandler {
        std::filesystem::path _root;
        intern::RadixTrie _art;

        void init();

     public:
         ArtIndexHandler() { init(); };
         ~ArtIndexHandler() = default;
         ArtIndexHandler(const ArtIndexHandler&) = delete;
         ArtIndexHandler& operator=(const ArtIndexHandler&) = delete;
         ArtIndexHandler(ArtIndexHandler&&) = default;
         ArtIndexHandler& operator=(ArtIndexHandler&&) = delete;

        std::vector<std::string>







    };
}