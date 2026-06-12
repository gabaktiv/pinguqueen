#include "radix-trie.hpp"
#include <algorithm>
#include <cassert>

namespace pinguqueen
{
    RadixTrie::~RadixTrie()
    {
        free_node(_root);
    }

    void RadixTrie::free_node(Node* root_node)
    {
        delete root_node;
    }

    void RadixTrie::replace(Node*& dest, Node* src) noexcept
    {
        assert(src != nullptr);
        dest = src;
    }

    bool RadixTrie::is_leaf(const Node* node) noexcept
    {
        assert(node != nullptr);
        return node->is_leaf();
    }

    std::string_view RadixTrie::load_key(const Node* node) noexcept
    {
        assert(node != nullptr && node->_type == NodeType::LeafNode);
        const auto* leaf = static_cast<const LeafNode*>(node);
        return leaf->_full_key;
    }

    Node* RadixTrie::grow_4_to_16(Node4* old_node) noexcept
    {
        auto* new_node = new Node16();
        new_node->_type = NodeType::Node16;
        new_node->_child_count = old_node->_child_count;

        for (u8 i = 0; i < 4; ++i){
            new_node->_keys[i] = old_node->_keys[i];
            new_node->_children[i] = old_node->_children[i];
        }

        old_node->_child_count = 0; // Verhindert, dass Kinder beim Löschen sterben
        delete old_node;
        return new_node;
    }

    Node* RadixTrie::grow_16_to_48(Node16* old_node) noexcept
    {
        auto* new_node = new Node48();
        new_node->_type = NodeType::Node48;
        new_node->_child_count = old_node->_child_count;

        std::fill(std::begin(new_node->_keys), std::end(new_node->_keys), static_cast<u8>(48));

        for (u8 i = 0; i < 16; ++i){
            u8 key_byte = old_node->_keys[i];
            new_node->_children[i] = old_node->_children[i];
            new_node->_keys[key_byte] = i;
        }

        old_node->_child_count = 0;
        delete old_node;
        return new_node;
    }

    Node* RadixTrie::grow_48_to_256(Node48* old_node) noexcept
    {
        auto* new_node = new Node256();
        new_node->_type = NodeType::Node256;
        new_node->_child_count = old_node->_child_count;

        for (uint32_t key_byte = 0; key_byte < 256; ++key_byte){
            u8 index = old_node->_keys[key_byte];
            if (index != 48){
                new_node->_children[key_byte] = old_node->_children[index];
            }
        }

        std::fill(std::begin(old_node->_children), std::end(old_node->_children), nullptr);
        delete old_node;
        return new_node;
    }

    void RadixTrie::add_child(Node*& parent, u8 key, Node* child) noexcept
    {
        if (parent->is_full()) {
            switch (parent->_type)
            {
                case NodeType::Node4:
                    parent = grow_4_to_16(static_cast<Node4*>(parent));
                    break;
                case NodeType::Node16:
                    parent = grow_16_to_48(static_cast<Node16*>(parent));
                    break;
                case NodeType::Node48:
                    parent = grow_48_to_256(static_cast<Node48*>(parent));
                    break;
                default:
                    break;
            }
        }
            switch (parent->_type)
            {
                case NodeType::Node4:
                {
                    static_cast<Node4*>(parent)->insert_pure(key, child);
                    break;
                }
                case NodeType::Node16:
                {
                    static_cast<Node16*>(parent)->insert_pure(key, child);
                    break;
                }
                case NodeType::Node48:
                {
                    static_cast<Node48*>(parent)->insert_pure(key, child);
                    break;
                }
                case NodeType::Node256:
                {
                    static_cast<Node256*>(parent)->insert_pure(key, child);
                    break;
                }
                default:
                    break;
            }


    }

    void RadixTrie::insert(Node*& node, std::string_view key, FileInfo* information, u32 depth)
    {
        auto* leaf = new LeafNode();
        leaf->_type = NodeType::LeafNode;
        leaf->_isleaf = true;
        leaf->_full_key = std::string(key);
        leaf->_metadata = information;

        if (node == nullptr)
        {
            replace(node, leaf);
            return;
        }

        if (node->is_leaf())
        {
            Node* newNode = new Node4();
            newNode->_type = NodeType::Node4;

            std::string_view key2 = load_key(node);
            u32 i = depth;
            while (i < key.length() && i < key2.length() && key[i] == key2[i])
            {
                i++;
            }

            newNode->_prefix_skip_length = i - depth;
            depth = depth + newNode->_prefix_skip_length;

            add_child(newNode, key[depth], leaf);
            add_child(newNode, key2[depth], node);
            replace(node, newNode);
            return;
        }

        // Nutzt ab hier die virtuelle Suche, um ohne riesige Switch-Cases tiefer zu gehen!
        Node* next = node->find_child(static_cast<u8>(key[depth]));
        if (next != nullptr)
        {
            insert(next, key, information, depth + 1);
        }
        else
        {
            // Ab hier geht dein TODO-Kommentar weiter, wenn der Knoten voll ist...
            add_child(node, static_cast<u8>(key[depth]), leaf);
        }
    }
}