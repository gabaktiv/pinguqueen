#pragma once
#include "../global.hpp"
#include "node.hpp"


/*
 * - Header-Datei des Adaptiven-Radix-Trie anhand des in der Readme verlinkten Paper.
 * - Viele Funktionen sind static, da sie nur die Nodes verwalten
*/

namespace pinguqueen::datastructs
{
    class RadixTrie
    {
        static constexpr char TERMINAL = '\0';

        using Self = RadixTrie;
        std::unique_ptr<Node> _root = nullptr;

        static void replace(std::unique_ptr<Node>& dest, std::unique_ptr<Node> src) noexcept;
        [[nodiscard]] static bool is_leaf(const Node* node) noexcept;
        [[nodiscard]] static std::string_view load_representative_key(const Node* node) noexcept;

        static void grow_4_to_16(std::unique_ptr<Node>& parent_slot) noexcept;
        static void grow_16_to_48(std::unique_ptr<Node>& parent_slot) noexcept;
        static void grow_48_to_256(std::unique_ptr<Node>& parent_slot) noexcept;
        static void shrink_256_to_48(std::unique_ptr<Node>& parent_slot) noexcept;
        static void shrink_48_to_16(std::unique_ptr<Node>& parent_slot) noexcept;
        static void shrink_16_to_4(std::unique_ptr<Node>& parent_slot) noexcept;

        static void add_child(std::unique_ptr<Node>& parent, u8 key, std::unique_ptr<Node> child) noexcept;
        static void remove_child(std::unique_ptr<Node>& parent, u8 key) noexcept;
        [[nodiscard]] static u32 check_prefix(const Node* node, std::string_view key, u32 depth) noexcept;
        [[nodiscard]] LeafNode* find_leaf_node(std::string_view key) noexcept;
        [[nodiscard]] static std::vector<std::string> collect_all_leaves(Node* node);
        static void insert_node( std::unique_ptr<Node>& node, std::string_view key, std::unique_ptr<core::FileInfo> information, u32 depth);
        static void delete_node (std::unique_ptr<Node>& node, std::string_view key, u32 depth) noexcept;


    public:
        RadixTrie() = default;
        ~RadixTrie() = default;
        RadixTrie(const RadixTrie&) = delete;
        RadixTrie& operator=(const RadixTrie&) = delete;
        RadixTrie(RadixTrie&&) = default;
        RadixTrie& operator=(RadixTrie&&) = delete;

        void insert( std::string key, std::unique_ptr<core::FileInfo> value) noexcept;
        void remove(std::string key) noexcept;
        [[nodiscard]] Node* root_node() noexcept { return _root.get(); }
        [[nodiscard]] core::FileInfo* search(std::string key) noexcept;

        //nicht dem Paper entsprechend. Diese Funktion gibt alle Suchelemente zurück mit einem bestimmten Präfix
        [[nodiscard]] std::vector<std::string> get_all_paths_with_prefix(const std::string& prefix);

        //Alle Pfade, die den Teilstring enthalten (collect_all_leaves + find-Filter)
        [[nodiscard]] std::vector<std::string> get_all_paths_with_substring(const std::string& substring);

    };
}





