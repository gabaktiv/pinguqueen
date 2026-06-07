#pragma once
#include "../global.hpp"

/*
 *Hier ist die Node-Struktur des Radix-Trie implementiert. Dabei ermöglicht diese Struktur die dynamische anpassung der Anzahl der Kinder, um Speicher anzupassen.
 *Die Anzahl der Kinder ist hard-coded, Struktur wie std::vector wird nicht verwendet, da dieser standartmäßig mind. 24 Bytes verbraucht.
 *Ich nutze aus, dass die letzten 3 bits jeder erster speicheradresse einer allokation 0 sind und erkenne mit tag leaf dann, in dem ich den letzten bit doch auf 1 setze, ob mein knoten ein Leaf ist
 */

namespace pinguqueen {

    struct FileInfo;

    enum class NodeType : u8 { Node4, Node16, Node48, Node256 };

    struct Node {
        u8 _child_count = 0;
        NodeType _type = NodeType::Node4;
        bool _isLeaf = 0;
    };

    struct Node4 : public Node{
        u8 _keys[4]{};
        Node* _children[4]{};
    };

    struct Node16 : public Node{
        u8 _keys[16]{};
        Node* _children[16]{};
    };

    //265 key-Werte, da es effizienter ist die ASCII-Zeichen zu hashmappen, als durchzuiterieren.
    struct Node48 : public Node{
        u8 _keys[256]{};
        Node* _children[48]{};
    };

    struct Node265 : public Node{
        Node* _children[265]{};
    };


    [[nodiscard]] static inline bool is_leaf(const Node* node_ptr) noexcept;
    [[nodiscard]] static inline Node* tag_as_leaf(const FileInfo* file_info) noexcept;
    [[nodiscard]] static inline FileInfo* get_leaf_value(const Node* node_ptr) noexcept;

#include "node.inl"

    };












