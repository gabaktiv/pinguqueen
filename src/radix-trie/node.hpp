#pragma once
#include "../global.hpp"
#include "file-info.hpp"
/*
 *Hier ist die Node-Struktur des Radix-Trie implementiert. Dabei ermöglicht diese Struktur die dynamische anpassung der Anzahl der Kinder, um Speicher anzupassen.
 *Die Anzahl der Kinder ist hard-coded, Struktur wie std::vector wird nicht verwendet, da dieser standartmäßig mind. 24 Bytes verbraucht.
 * TODO: Ich nutze aus, dass die letzten 3 bits jeder erster speicheradresse einer allokation 0 sind und erkenne mit pointertagging dann, in dem ich den letzten bit auf 1 setze, ob mein knoten ein Leaf ist
 */

namespace pinguqueen {

    enum class NodeType : u8 { Node4, Node16, Node48, Node256, LeafNode };

    struct Node {
        u8 _child_count = 0;
        NodeType _type = NodeType::Node4;
        bool _isleaf : 1 = false;

        // OPTIMISTISCHE PFADKOMPRIMIERUNG:
        //  Anzahl der zu blind übersprringenden Zeichen des Suchstrings
        u32 _prefix_skip_length = 0;
    };

    struct LeafNode : public Node {
        std::string _full_key; // Der komplette Pfad zur Verifizierung
        FileInfo* _metadata = nullptr;
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

    struct Node256 : public Node{
        Node* _children[256]{};
    };

    };












