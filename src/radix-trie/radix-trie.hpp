#pragma once
#include "../global.hpp"
#include "node.hpp"

//Todo: Implementation als Single-Ton-Art

namespace pinguqueen {

    class RadixTrie
    {
        using Self = RadixTrie;
        Node* _root = nullptr;
        Node* _current = _root;

        static void free_node(Node* node); //static um den this Pointer der Klasse zu vermeiden
    public:

        RadixTrie() = default;
        ~RadixTrie() = default;

        void append(const char c) const;









    };









}




