#pragma once
#include "../global.hpp"

/*
 *Hier ist die Node-Struktur des Radix-Trie implementiert. Dabei ermöglicht diese Struktur die dynamische anpassung der Anzahl der Kinder, um Speicher anzupassen.
 *Die Anzahl der Kinder ist hard-coded, Struktur wie std::vector wird nicht verwendet, da dieser standartmäßig mind. 24 Bytes verbraucht.
 */

namespace pinguqueen {

    enum class NodeType : u8 { Node4, Node16, Node48, Node128, Node256 };

    struct Node {
        u8 child_count = 0;
        NodeType _type = NodeType::Node4;
        bool _isLeaf = 0;
    };

    struct Node4 : public Node{
        u8 keys[4]{};
        Node *_children[4]{};
    };

    struct Node16 : public Node{
        u8 keys[16]{};
        Node *_children[16]{};
    };

    //265 key-Werte, da es effizienter ist die ASCII-Zeichen zu hashmappen, als durchzuiterieren.
    struct Node48 : public Node{
        u8 keys[256]{};
        Node *_children[48]{};
    };

    struct Node128 : public Node{
        u8 keys[265]{};
        Node *_children[128]{};
    };

    struct Node265 : public Node{
        u8 keys[265]{};
        Node *_children[265]{};
    };

    };













