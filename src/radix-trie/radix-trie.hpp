#pragma once
#include "../global.hpp"
#include "node.hpp"

//TODO: Implementation als Single-Ton-Art

namespace pinguqueen {

    class RadixTrie
    {
        using Self = RadixTrie;
        Node* _root = nullptr;
        Node* _current = _root;

        static void free_node(Node* node); //static um den this Pointer der Klasse zu vermeiden
        static void replace (Node*& dest, Node* src) noexcept;
        [[nodiscard]] static std::string_view load_key(const Node* node) noexcept;
        static void add_child(Node* parent, uint8_t key, Node* child);

    public:

        RadixTrie() = default;
        ~RadixTrie() = default;

        [[nodiscard]] static bool is_leaf(const Node* node) noexcept;
        void insert(Node*& node, std::string_view key, FileInfo* information, u32 depth);
    };

/*
   template <typename T>
   void with_specific_node(Node* node, T&& callback) {
       //TODO: FEHLERHANDLING
        if (node == nullptr) return;
        switch (node->_type) {
            case NodeType::Node4:
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                callback(static_cast<Node4*>(node)); //kein dynamischer Cast, weil wir keinen virtuellen Konstruktor oder Desktruktor haben wollen
                break;
            case NodeType::Node16:
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                callback(static_cast<Node16*>(node));
                break;
            case NodeType::Node48:
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                callback(static_cast<Node48*>(node));
                break;
            case NodeType::Node256:
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                callback(static_cast<Node256*>(node));
                break;
            case NodeType::LeafNode:
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                callback(static_cast<LeafNode*>(node));
                break;

        }
    */
    }







}




