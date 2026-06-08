#include "radix-trie.hpp"

void pinguqueen::RadixTrie::free_node(Node* node)
{
    if (node == nullptr) return;

    switch (node->_type) {
        case NodeType::Node4 :
            delete static_cast<Node4*>(node);
            break;
        case NodeType::Node16 :
            delete static_cast<Node16*>(node);
            break;
        case NodeType::Node48 :
            delete static_cast<Node48*>(node);
            break;
        case NodeType::Node256 :
            delete static_cast<Node265*>(node);
            break;
    }
}

void pinguqueen::RadixTrie::append(const char c) const {

    

}
