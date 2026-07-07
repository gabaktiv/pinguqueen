//
// Created by gabriel on 7/2/26.
//
#pragma once
#include <filesystem>
#include "../radix-trie/radix-trie.hpp"
#include "../core/file-info.hpp"

namespace pinguqueen::file {


    class ArtIndexBuilder {
        std::filesystem::path _root;
        datastructs::RadixTrie _art;

        void init();

     public:
         ArtIndexBuilder() { init(); };
         ~ArtIndexBuilder() = default;
         ArtIndexBuilder(const ArtIndexBuilder&) = delete;
         ArtIndexBuilder& operator=(const ArtIndexBuilder&) = delete;
         ArtIndexBuilder(ArtIndexBuilder&&) = default;
         ArtIndexBuilder& operator=(ArtIndexBuilder&&) = delete;

         [[nodiscard]] datastructs::RadixTrie& art() { return _art; }




    };
}
