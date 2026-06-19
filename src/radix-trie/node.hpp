#pragma once
#include "../global.hpp"
#include "file-info.hpp"
#include <string>

/*
 *Hier ist die Node-Struktur des Radix-Trie implementiert. Dabei ermöglicht diese Struktur die dynamische anpassung der Anzahl der Kinder, um Speicher anzupassen.
 *Die Anzahl der Kinder is hard-coded, Struktur wie std::vector wird nicht verwendet, da dieser standartmäßig mind. 24 Bytes verbraucht.
 * TODO: Ich nutze aus, dass die letzten 3 bits jeder erster speicheradresse einer allokation 0 sind und erkenne mit pointertagging dann, in dem ich den letzten bit auf 1 setze, ob mein knoten ein Leaf is
 */

namespace pinguqueen {

    enum class NodeType : u8 { Node4, Node16, Node48, Node256, LeafNode };

    class Node
    {
    public:
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
        [[nodiscard]] virtual Node* find_child(u8 key_byte) noexcept = 0;

    };

    class LeafNode : public Node
    {
    public:
        std::string _full_key;
        FileInfo* _metadata = nullptr;

        [[nodiscard]] bool is_full() const noexcept override { return true; }
        [[nodiscard]] Node* find_child(u8) noexcept override { return nullptr; }

    };

    class Node4 : public Node
    {
    public:

        static constexpr u8 GROW_CHILD_COUNT = 5;
        static constexpr u8 NO_CHILD = 0;

        u8 _keys[4]{};
        Node* _children[4]{};

        Node4() = default;
        ~Node4() override;
        Node4(const Node4&) = delete;
        Node4& operator=(const Node4&) = delete;
        Node4(Node4&&) = delete;
        Node4& operator=(Node4&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == 4; }

        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;
        void insert_pure(u8 key, Node* child) noexcept;  //!DANGEROUS, NO GROW OF NODE IF FULL
        //!DANGEROUS, CRASHES IF NO CHILD AVAILABLE. IT ONLY REMOVES CORRESPONDING POINTER / NO DESTRUKTOR CALLED
        void remove_pure(u8 key) noexcept;
    };

    class Node16 : public Node
    {
    public:
        u8 _keys[16]{};
        Node* _children[16]{};
        static constexpr u8 SHRINKING_CHILD_COUNT = 3;
        static constexpr u8 GROW_CHILD_COUNT = 17;


        Node16() = default;
        ~Node16() override;
        Node16(const Node16&) = delete;
        Node16& operator=(const Node16&) = delete;
        Node16(Node16&&) = delete;
        Node16& operator=(Node16&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == 16; }

        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;
        void insert_pure(u8 key, Node* child) noexcept;  //!DANGEROUS, NO GROW IF FULL
        //!DANGEROUS, NO SHRINKING IF TOO EMPTY AND ONLY REMOVING OF POINTER / NO DELETION
        void remove_pure(u8 key) noexcept;
    };

    class Node48 : public Node
    {
    public:
        u8 _keys[256]{};
        Node* _children[48]{};
        static constexpr u8 NOTHING = 48;
        static constexpr u8 SHRINKING_CHILD_COUNT = 15;
        static constexpr u8 GROW_CHILD_COUNT = 49;

        Node48() = default;
        ~Node48() override;
        Node48(const Node48&) = delete;
        Node48& operator=(const Node48&) = delete;
        Node48(Node48&&) = delete;
        Node48& operator=(Node48&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == 48; }
        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;

        void insert_pure(u8 key, Node* child) noexcept;  //!DANGEROUS, NO GROW IF FULL
        //!DANGEROUS, NO SHRINKING IF TOO EMPTY AND ONLY REMOVING OF POINTER / NO DELETION
        void remove_pure(u8 key) noexcept;
    };

    

    class Node256 : public Node
    {
    public:
        Node* _children[256]{};
        static constexpr u8 SHRINKING_CHILD_COUNT = 47;



        Node256() = default;
        ~Node256() override;
        Node256(const Node256&) = delete;
        Node256& operator=(const Node256&) = delete;
        Node256(Node256&&) = delete;
        Node256& operator=(Node256&&) = delete;

        [[nodiscard]] bool is_full() const noexcept override { return _child_count == 256; }

        [[nodiscard]] Node* find_child(u8 key_byte) noexcept override;

        void insert_pure(u8 key, Node* child) noexcept;  //!DANGEROUS, NO GROW OF NODE IF FULL
        void remove_pure(u8 key) noexcept;  //!DANGEROUS, NO SHRINKING OF NODE IF TOO EMPTY
    };

}