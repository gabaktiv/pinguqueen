#pragma once
#include "../global.hpp"
#include "node.hpp"

//TODO: Implementation als Single-Ton-Art

namespace pinguqueen
{
    class RadixTrie
    {
        using Self = RadixTrie;
        Node* _root = nullptr;

        static void free_node(Node* node);
        static void replace(Node*& dest, Node* src) noexcept;
        [[nodiscard]] static bool is_leaf(const Node* node) noexcept;
        [[nodiscard]] static std::string_view load_key(const Node* node) noexcept;

        static void grow_4_to_16(Node*& parent_slot) noexcept;
        static void grow_16_to_48(Node*& parent_slot) noexcept;
        static void grow_48_to_256(Node*& parent_slot) noexcept;

        static void shrink_256_to_48(Node*& parent_slot) noexcept;
        static void shrink_48_to_16(Node*& parent_slot) noexcept;
        static void shrink_16_to_4(Node*& parent_slot) noexcept;

        static void add_child(Node*& parent, u8 key, Node* child) noexcept;
        [[nodiscard]] static u32 check_prefix(const Node* node, std::string_view key, u32 depth) noexcept;
        [[nodiscard]] LeafNode* find_leaf_node(std::string_view key) noexcept;

    public:
        RadixTrie() = default;
        ~RadixTrie();
        RadixTrie(const RadixTrie&) = delete;
        RadixTrie& operator=(const RadixTrie&) = delete;
        RadixTrie(RadixTrie&&) = delete;
        RadixTrie& operator=(RadixTrie&&) = delete;

        static void insert(Node*& node, std::string_view key, FileInfo* information, u32 depth);
        [[nodiscard]] FileInfo* search(std::string_view key) noexcept;
    };
}





