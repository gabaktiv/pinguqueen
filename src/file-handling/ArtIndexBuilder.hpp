//
// Created by gabriel on 7/2/26.
//
#pragma once
#include <filesystem>
#include "../adaptive-radix-trie/adaptive-radix-trie.hpp"
#include "../core/file-info.hpp"

namespace pinguqueen::file {


    class ArtIndexBuilder {
        std::filesystem::path _root;
        datastructs::AdaptiveRadixTrie _art;

        void init();

     public:
         ArtIndexBuilder() { init(); };
         ~ArtIndexBuilder() = default;
         ArtIndexBuilder(const ArtIndexBuilder&) = delete;
         ArtIndexBuilder& operator=(const ArtIndexBuilder&) = delete;
         ArtIndexBuilder(ArtIndexBuilder&&) = default;
         ArtIndexBuilder& operator=(ArtIndexBuilder&&) = delete;

         [[nodiscard]] datastructs::AdaptiveRadixTrie& art() { return _art; }




    };
}
