#include "radix-trie.hpp"
#include <algorithm>
#include <cassert>

namespace pinguqueen::intern
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

    void RadixTrie::grow_4_to_16(Node*& parent_slot) noexcept {
        assert(parent_slot->_type == NodeType::Node4);

        auto* old_node = static_cast<Node4*>(parent_slot);
        auto* new_node = new Node16();


        new_node->_prefix_skip_length = old_node->_prefix_skip_length;
        new_node->_child_count = old_node->_child_count;
        new_node->_type = NodeType::Node16;

        for (u16 i = 0; i < old_node->_child_count; ++i) {
            new_node->_keys[i] = old_node->_keys[i];
            new_node->_children[i] = old_node->_children[i];

            old_node->_children[i] = nullptr;
        }

        old_node->_child_count = 0;
        delete old_node;

        parent_slot = new_node;
    }

    void RadixTrie::grow_16_to_48(Node*& parent_slot) noexcept
    {
        assert(parent_slot->_type == NodeType::Node16);

        auto* old_node = static_cast<Node16*>(parent_slot);
        auto* new_node = new Node48();

        new_node->_prefix_skip_length = old_node->_prefix_skip_length;
        new_node->_type = NodeType::Node48;
        new_node->_child_count = parent_slot->_child_count;

        std::fill(std::begin(new_node->_keys), std::end(new_node->_keys), static_cast<u8>(Node48::NOTHING));

        for (u8 i = 0; i < Node16::GROW_CHILD_COUNT; ++i){
            u8 key_byte = old_node->_keys[i];
            new_node->_children[i] = old_node->_children[i];
            new_node->_keys[key_byte] = i;
            old_node->_children[i] = nullptr;
        }

        old_node->_child_count = 0;
        delete old_node;

        parent_slot = new_node;
    }

    void RadixTrie::grow_48_to_256(Node*& parent_slot) noexcept
    {
        assert(parent_slot->_type == NodeType::Node48);

        auto* old_node = static_cast<Node48*>(parent_slot);
        auto* new_node = new Node256();
        new_node->_type = NodeType::Node256;
        new_node->_child_count = old_node->_child_count;
        new_node->_prefix_skip_length = old_node->_prefix_skip_length;

        for (u16 key_byte = 0; key_byte < 256; ++key_byte){
            u8 index = old_node->_keys[key_byte];
            if (index != 48){
                new_node->_children[key_byte] = old_node->_children[index];
            }
        }

        std::fill(std::begin(old_node->_children), std::end(old_node->_children), nullptr);
        old_node->_child_count = 0;
        delete old_node;

        parent_slot = new_node;
    }

    void RadixTrie::shrink_256_to_48(Node*& parent_slot) noexcept
    {
        assert(parent_slot != nullptr);
        assert(parent_slot->_type == NodeType::Node256);

        auto* old_node = static_cast<Node256*>(parent_slot);
        auto* new_node = new Node48();

        new_node->_prefix_skip_length = old_node->_prefix_skip_length;
        new_node->_child_count = old_node->_child_count;
        new_node->_type = NodeType::Node48;

        std::fill(std::begin(new_node->_keys), std::end(new_node->_keys), Node48::NOTHING);

        u8 next_free_slot = 0;
        for (u16 key_byte = 0; key_byte < Node256::FULL; ++key_byte) {
            if (old_node->_children[key_byte] != nullptr) {
                new_node->_children[next_free_slot] = old_node->_children[key_byte];
                new_node->_keys[key_byte] = next_free_slot;
                ++next_free_slot;
                old_node->_children[key_byte] = nullptr;
            }
        }
        delete old_node;
        parent_slot = new_node;
    }

    void RadixTrie::shrink_48_to_16(Node*& parent_slot) noexcept
{
    assert(parent_slot != nullptr);
    assert(parent_slot->_type == NodeType::Node48);

    auto* old_node = static_cast<Node48*>(parent_slot);
    auto* new_node = new Node16();

    // Metadaten übertragen
    new_node->_prefix_skip_length = old_node->_prefix_skip_length;
    new_node->_child_count = old_node->_child_count;
    new_node->_type = NodeType::Node16;

    u8 next_free_slot = 0;
    for (u16 key_byte = 0; key_byte < 256; ++key_byte) {
        u8 index = old_node->_keys[key_byte];
        if (index != Node48::NOTHING) {
            new_node->_keys[next_free_slot] = static_cast<u8>(key_byte);
            new_node->_children[next_free_slot] = old_node->_children[index];
            ++next_free_slot;
            old_node->_children[index] = nullptr;
        }
    }

    delete old_node;
    parent_slot = new_node;
}

void RadixTrie::shrink_16_to_4(Node*& parent_slot) noexcept
{
    assert(parent_slot != nullptr);
    assert(parent_slot->_type == NodeType::Node16);

    auto* old_node = static_cast<Node16*>(parent_slot);
    auto* new_node = new Node4();

    new_node->_prefix_skip_length = old_node->_prefix_skip_length;
    new_node->_child_count = old_node->_child_count;
    new_node->_type = NodeType::Node4;

   
    for (u16 i = 0; i < old_node->_child_count; ++i) {
        new_node->_keys[i] = old_node->_keys[i];
        new_node->_children[i] = old_node->_children[i];
        old_node->_children[i] = nullptr;
    }

    delete old_node;
    parent_slot = new_node;
    }


    void RadixTrie::add_child(Node*& parent, u8 key, Node* child) noexcept
    {
        if (parent->is_full()) {
            switch (parent->_type)
            {
                case NodeType::Node4:
                    grow_4_to_16(parent);
                    break;
                case NodeType::Node16:
                    grow_16_to_48(parent);
                    break;
                case NodeType::Node48:
                    grow_48_to_256(parent);
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

    void RadixTrie::remove_child(Node*& parent, u8 key) noexcept
{
    switch (parent->_type)
    {
        case NodeType::Node4:
            static_cast<Node4*>(parent)->remove_pure(key);
            break;
        case NodeType::Node16:
            static_cast<Node16*>(parent)->remove_pure(key);
            break;
        case NodeType::Node48:
            static_cast<Node48*>(parent)->remove_pure(key);
            break;
        case NodeType::Node256:
            static_cast<Node256*>(parent)->remove_pure(key);
            break;
        default:
            break;
    }

    switch (parent->_type)
    {
        case NodeType::Node256:
            if (parent->_child_count <= Node256::SHRINKING_CHILD_COUNT) {
                shrink_256_to_48(parent);
            }
            break;

        case NodeType::Node48:
            if (parent->_child_count <= Node48::SHRINKING_CHILD_COUNT) {
                shrink_48_to_16(parent);
            }
            break;

        case NodeType::Node16:
            if (parent->_child_count <= Node16::SHRINKING_CHILD_COUNT) {
                shrink_16_to_4(parent);
            }
            break;

        case NodeType::Node4:
            if (parent->_child_count == 1) {
                auto* n4 = static_cast<Node4*>(parent);
                Node* last_child = n4->_children[0];

                last_child->_prefix_skip_length += n4->_prefix_skip_length + 1;

                n4->_children[0] = nullptr;
                delete n4;

                parent = last_child;
            }
            break;

        default:
            break;
    }
}

    u32 RadixTrie::check_prefix(const Node* node, std::string_view key, u32 depth) noexcept
    {
        const Node* curr = node;
        while (!curr->is_leaf())
        {
            if (curr->_type == NodeType::Node4) {
                curr = static_cast<const Node4*>(curr)->_children[0];
            }
            else if (curr->_type == NodeType::Node16) {
                curr = static_cast<const Node16*>(curr)->_children[0];
            }
            else if (curr->_type == NodeType::Node48) {
                const auto* n48 = static_cast<const Node48*>(curr);
                for (auto* child : n48->_children) {
                    if (child) { curr = child; break; }
                }
            }
            else if (curr->_type == NodeType::Node256) {
                const auto* n256 = static_cast<const Node256*>(curr);
                for (auto* child : n256->_children) {
                    if (child) { curr = child; break; }
                }
            }
        }

        std::string_view valid_key = static_cast<const LeafNode*>(curr)->_full_key;

        u32 p = 0;
        while (p < node->_prefix_skip_length &&
               (depth + p) < key.length() &&
               key[depth + p] == valid_key[depth + p])
        {
            p++;
        }

        return p;
    }


    LeafNode* RadixTrie::find_leaf_node(std::string_view key) noexcept
    {
        Node* curr = _root;
        u32 depth = 0;

        while (curr != nullptr)
        {
            // Wenn wir ein Blatt erreichen, validieren wir den vollständigen Key
            if (curr->is_leaf())
            {
                auto* leaf = static_cast<LeafNode*>(curr);
                if (leaf->_full_key == key) {
                    return leaf;
                }
                return nullptr; // Mismatch am Ende des Pfads
            }

            if (curr->_prefix_skip_length > 0){
                u32 p = check_prefix(curr, key, depth);
                if (p != curr->_prefix_skip_length) {
                    return nullptr;
                }
                depth += curr->_prefix_skip_length;
            }

            if (depth >= key.length()){
                return nullptr;
            }

            curr = curr->find_child(static_cast<u8>(key[depth]));
            depth++;
        }

        return nullptr;
    }



    void RadixTrie::insert_node(Node*& node, std::string_view key, FileInfo* information, u32 depth)
    {
        auto* leaf = new LeafNode();
        leaf->_type = NodeType::LeafNode;
        leaf->_isleaf = true;
        leaf->_full_key = std::string(key);
        leaf->_metadata = information;

        if (node == nullptr){
            replace(node, leaf);
            return;
        }

        if (node->is_leaf()){
            Node* newNode = new Node4();
            newNode->_type = NodeType::Node4;

            std::string_view key2 = load_key(node);
            u32 i = depth;
            while (i < key.length() && i < key2.length() && key[i] == key2[i]){
                i++;
            }

            newNode->_prefix_skip_length = i - depth;
            depth = depth + newNode->_prefix_skip_length;
            
            if (depth < key.length()) {
                add_child(newNode, static_cast<u8>(key[depth]), leaf);
            } else {
                delete leaf;
            }
            if (depth < key2.length()) {
                add_child(newNode, static_cast<u8>(key2[depth]), node);
            }
            replace(node, newNode);
            return;
        }

        u32 p = check_prefix(node, key, depth);
        if (p != node->_prefix_skip_length) {
            Node* newNode = new Node4;

            std::string_view valid_key = load_key(node);

            if (depth+p < key.length()) {
                add_child(newNode, static_cast<u8>(key[depth+p]), leaf);
            } else {
                delete leaf;
            }
            if (depth+p < valid_key.length()) {
                add_child(newNode, static_cast<u8>(valid_key[depth + p]), node);
            }
                newNode->_prefix_skip_length = p;
            node->_prefix_skip_length -= (p + 1);
            replace(node, newNode);
            return;

        }

        depth = depth + node->_prefix_skip_length;
        Node* next = node->find_child(static_cast<u8>(key[depth]));
        if (next != nullptr) {
            insert_node(next, key, information, depth + 1);
            delete leaf;
        }
        else {
            if (depth < key.length()) {
                add_child(node, static_cast<u8>(key[depth]), leaf);
            } else {
                delete leaf;
            }
        }

    }


    void RadixTrie::delete_node(Node*& node, std::string_view key, u32 depth) noexcept
    {
        if (node == nullptr) return;

        if (node->is_leaf()) {
            auto* leaf = static_cast<LeafNode*>(node);
            if (leaf->_full_key == key) {
                delete leaf;
                node = nullptr;
            }
            return;
        }

        u32 p = check_prefix(node, key, depth);
        if (p != node->_prefix_skip_length) return;

        depth += node->_prefix_skip_length;
        u8 key_byte = static_cast<u8>(key[depth]);

        Node* next = node->find_child(key_byte);

        if (next != nullptr) {
            delete_node(next, key, depth + 1);
            if (next == nullptr) {
                remove_child(node, key_byte);
            }
        }
    }


    FileInfo* RadixTrie::search(std::string_view key) noexcept
    {

        LeafNode* leaf = find_leaf_node(key);
        if (leaf != nullptr) {
            return leaf->_metadata;
        }
        return nullptr;
    }

    void RadixTrie::collect_all_leaves(Node* start_node, std::vector<std::string>& results) {
        if (start_node == nullptr) return;

        std::vector<Node*> node_stack;
        node_stack.reserve(256);
        node_stack.push_back(start_node);

        while (!node_stack.empty()) {
            Node* current = node_stack.back();
            node_stack.pop_back();

            if (current->is_leaf()) {
                auto* leaf = static_cast<LeafNode*>(current);
                results.push_back(leaf->_full_key);
                continue;
            }

            // Hardware-nahes Abwandern
            if (current->_type == NodeType::Node4) {
                auto* n4 = static_cast<Node4*>(current);
                for (int i = static_cast<int>(n4->_child_count) - 1; i >= 0; --i) {
                    node_stack.push_back(n4->_children[i]);
                }
            }
            else if (current->_type == NodeType::Node16) {
                auto* n16 = static_cast<Node16*>(current);
                for (int i = static_cast<int>(n16->_child_count) - 1; i >= 0; --i) {
                    node_stack.push_back(n16->_children[i]);
                }
            }
            else if (current->_type == NodeType::Node48) {
                auto* n48 = static_cast<Node48*>(current);
                // 🏆 KORREKTUR: Node48 hat maximal 48 Kinder-Slots.
                // Wir müssen die physischen Slots rückwärts prüfen!
                for (int i = 47; i >= 0; --i) {
                    if (n48->_children[i] != nullptr) {
                        node_stack.push_back(n48->_children[i]);
                    }
                }
            }
            else if (current->_type == NodeType::Node256) {
                auto* n256 = static_cast<Node256*>(current);
                for (int i = 255; i >= 0; --i) {
                    if (n256->_children[i] != nullptr) {
                        node_stack.push_back(n256->_children[i]);
                    }
                }
            }
        }
    }

    std::vector<std::string> RadixTrie::get_all_paths_with_prefix(const std::string& prefix) {
        std::vector<std::string> results;
        if (_root == nullptr) return results;

        Node* current = _root;
        u32 depth = 0;

        //Navigation to that Node where the prefix matches.
        while (current != nullptr) {
            if (current->is_leaf()) {
                auto* leaf = static_cast<LeafNode*>(current);

                if (leaf->_full_key.rfind(prefix, 0) == 0) {
                    results.push_back(leaf->_full_key);
                }
                return results;
            }

            if (depth >= prefix.size()) {
                break;
            }

            depth += current->_prefix_skip_length;

            if (depth >= prefix.size()) {
                break;
            }

            current = current->find_child(static_cast<u8>(prefix[depth]));
            ++depth;
        }
        collect_all_leaves(current, results);
        return results;
    }

}