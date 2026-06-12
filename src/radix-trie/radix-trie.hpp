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
        static Node* grow_4_to_16(Node4* old_node) noexcept;
        static Node* grow_16_to_48(Node16* old_node) noexcept;
        static Node* grow_48_to_256(Node48* old_node) noexcept;

        static void add_child(Node*& parent, u8 key, Node* child) noexcept;
        static u32 check_prefix(const Node* node, std::string_view key, std::string_view valid_key, u32 depth) noexcept;

    public:
        RadixTrie() = default;
        ~RadixTrie();

        static void insert(Node*& node, std::string_view key, FileInfo* information, u32 depth);
    };
}





