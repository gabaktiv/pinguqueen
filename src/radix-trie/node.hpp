#pragma once
#include "../global.hpp"
#include "../core/file-info.hpp"
#include <memory>
#include <string>
#include <cassert>
/*
 * - Hier ist die Node-Struktur des Adaptive-Radix-Trie implementiert.
 * Dabei ermöglicht diese Struktur die dynamische anpassung der Anzahl der Kinder, um Speicher anzupassen.
 * - Die Anzahl der Kinder is hard-coded, Struktur wie std::vector wird nicht verwendet, da dieser standartmäßig mind. 24 Bytes overhead verbraucht.
 * - TODO: Ich nutze aus, dass die letzten 3 bits jeder erster speicheradresse einer allokation 0 sind und erkenne mit pointertagging dann, in dem ich den letzten bit auf 1 setze, ob mein knoten ein Leaf is
 * - Die Node Implementierungs-Entscheidungen sind aus dem in der Readme verlinkten Paper zurückzuführen,
 * wobei nur der optimistische Ansatz implementiert ist mit der _prefix_skip_length variabel.
 * - Die Funktionen insert_pure und remove_pure sind nicht virtuell, da zu einem, der Leaf Node diese Funktionen nicht braucht, und sowieso im Kontext des Nutzen abgefragt werden sollte,
 * was für eine Art Knoten diese Funktion ausführt, da diese zuvor vielleicht schrumpfen oder erweitert werden müssen
 *  - Ich habe mich für structs statt Klassen entschieden, da die finale Verwaltung in der RadxiTrie Klasse verwendet wird.
*/

namespace pinguqueen::datastructs {

    enum class NodeType : u8 { Node4, Node16, Node48, Node256, LeafNode };

    struct Node
    {
        //  Anzahl der zu blind überspringenden Zeichen des Suchstrings
        u32 _prefix_skip_length = 0;

        u16 _child_count = 0;
        NodeType _type = NodeType::Node4;
        bool _isleaf : 1 = false;


        Node() = default;
        virtual ~Node() = default;
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&) = delete;
        Node& operator=(Node&&) = delete;

        [[nodiscard]] bool is_leaf() const noexcept { return _isleaf; }
        [[nodiscard]] virtual bool is_full() const noexcept = 0;
        [[nodiscard]] virtual bool is_too_empty() const noexcept = 0;

        [[nodiscard]] virtual Node* find_child(u8 key_byte) noexcept = 0;
        [[nodiscard]] virtual std::unique_ptr<Node>* find_child_slot(u8 key_byte) noexcept = 0;

    };

    // Should only provide Information about the data.

    struct LeafNode :  Node
    {
        std::string _full_key;
        std::unique_ptr<core::FileInfo> _metadata;

        //Blätter sollten keine Informationen über Anzahl der Kinder geben
        [[nodiscard]] bool is_full() const noexcept override { assert(false); return false; }
        [[nodiscard]] bool is_too_empty() const noexcept override{ assert(false); return false; }

        [[nodiscard]] Node* find_child(u8) noexcept override { return nullptr; }
        [[nodiscard]] std::unique_ptr<Node>* find_child_slot(u8) noexcept override { return nullptr; }

    };

    // - Node4 and Node16 are very similar. The connection to the children is implemented with 2 arrays.
    // - _keys[i] represent the edge and _children[i] leads to the next node.
    // - Example _keys[1] -> 'e'; _children[1] -> node
    // - It's important in the deletion process, that these arrays are filled from left to right with no spaces inbetween.
    // - Also inserting an edge will sort them with an insertion sort algorithm
    // (Handling of deletion, inserting and searching is implemented in the Adaptive-Radix-Trie Class, not here or the node.cpp)

    struct Node4 :  Node
    {

        static constexpr u8 GROW_CHILD_COUNT = 4;
        static constexpr u8 NO_CHILD = 0;

        u8 _keys[4]{};
        std::unique_ptr<Node> _children[4]{};

        Node4() = default;
        ~Node4() override = default;
        Node4(const Node4&) = delete;
        Node4& operator=(const Node4&) = delete;
        Node4(Node4&&) = delete;
        Node4& operator=(Node4&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == GROW_CHILD_COUNT; }
        [[nodiscard]] bool is_too_empty() const noexcept override{ return _child_count == NO_CHILD; }

        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;
        [[nodiscard]] std::unique_ptr<Node>* find_child_slot(u8 key_byte) noexcept override;

        //!DANGEROUS, NO GROW IF FULL
        void insert_pure(u8 key, std::unique_ptr<Node> child) noexcept;
        //!DANGEROUS, CRASHES IF NO CHILD AVAILABLE. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
        void remove_pure(u8 key) noexcept;
    };



    struct Node16 : Node
    {
        u8 _keys[16]{};
        std::unique_ptr<Node> _children[16]{};
        static constexpr u8 SHRINKING_CHILD_COUNT = 3;
        static constexpr u8 GROW_CHILD_COUNT = 16;


        Node16() = default;
        ~Node16() override = default;
        Node16(const Node16&) = delete;
        Node16& operator=(const Node16&) = delete;
        Node16(Node16&&) = delete;
        Node16& operator=(Node16&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == GROW_CHILD_COUNT; }
        [[nodiscard]] bool is_too_empty() const noexcept override{ return _child_count == SHRINKING_CHILD_COUNT; }

        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;
        [[nodiscard]] std::unique_ptr<Node>* find_child_slot(u8 key_byte) noexcept override;

        //!DANGEROUS, NO GROW IF FULL
        void insert_pure(u8 key, std::unique_ptr<Node> child) noexcept;
        //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
        void remove_pure(u8 key) noexcept;
    };

    // - char gets hashmapped in _keys array -> content of one index equals the index in _children array to the next child.
    // - Example: _keys[e] -> 5; _children[5] -> node4. Annoying to implement shrink and grow functions, but space-efficient.
    // - The const "NON_EXISTING_EDGE" variable is the default variable for non exisiting edges in the _keys array.
    struct Node48 : Node
    {
        static constexpr u8 NON_EXISTING_EDGE = 48;
        static constexpr u8 SHRINKING_CHILD_COUNT = 15;
        static constexpr u8 GROW_CHILD_COUNT = 48;

        u8 _keys[256]{};
        std::unique_ptr<Node> _children[48]{};

        Node48();
        ~Node48() override = default;
        Node48(const Node48&) = delete;
        Node48& operator=(const Node48&) = delete;
        Node48(Node48&&) = delete;
        Node48& operator=(Node48&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == 48; }
        [[nodiscard]] bool is_too_empty() const noexcept override{ return _child_count == SHRINKING_CHILD_COUNT; }

        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;
        [[nodiscard]] std::unique_ptr<Node>* find_child_slot(u8 key_byte) noexcept override;

        //!DANGEROUS, NO GROW IF FULL
        void insert_pure(u8 key, std::unique_ptr<Node> child) noexcept;
        //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
        void remove_pure(u8 key) noexcept;
    };

    //Only one children Array, Edges and Nodes get Hashmapped. Example:  _children['e'] -> node4
    struct Node256 : Node
    {
        std::unique_ptr<Node> _children[256]{};
        static constexpr u16 FULL = 256;
        static constexpr u8 SHRINKING_CHILD_COUNT = 47;



        Node256() = default;
        ~Node256() override = default;
        Node256(const Node256&) = delete;
        Node256& operator=(const Node256&) = delete;
        Node256(Node256&&) = delete;
        Node256& operator=(Node256&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == FULL; }
        [[nodiscard]] bool is_too_empty() const noexcept override{ return _child_count == SHRINKING_CHILD_COUNT; }

        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;
        [[nodiscard]] std::unique_ptr<Node>* find_child_slot(u8 key_byte) noexcept override;

        void insert_pure(u8 key, std::unique_ptr<Node> child) noexcept;
        //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
        void remove_pure(u8 key) noexcept;
    };

}
