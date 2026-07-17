#include "adaptive-radix-trie.hpp"
#include <algorithm>
#include <cassert>
#include <utility>

namespace pinguqueen::datastructs
{
    void AdaptiveRadixTrie::replace(std::unique_ptr<Node>& dest, std::unique_ptr<Node> src) noexcept
    {
        assert(src != nullptr);
        dest = std::move(src);

    }

    bool AdaptiveRadixTrie::is_leaf(const Node* node) noexcept
    {
        assert(node != nullptr);
        return node->is_leaf();
    }

    std::string_view AdaptiveRadixTrie::load_representative_key(const Node* node) noexcept
    {
        assert(node != nullptr);

        const Node* curr = node;
        while (!curr->is_leaf())
        {
            if (curr->_type == NodeType::Node4) {
                curr = static_cast<const Node4*>(curr)->_children[0].get();
            }
            else if (curr->_type == NodeType::Node16) {
                curr = static_cast<const Node16*>(curr)->_children[0].get();
            }
            else if (curr->_type == NodeType::Node48) {
                const auto* n48 = static_cast<const Node48*>(curr);
                curr = nullptr;
                for (const auto& child : n48->_children) {
                    if (child != nullptr) {
                        curr = child.get();
                        break;
                    }
                }
            }
            else if (curr->_type == NodeType::Node256) {
                const auto* n256 = static_cast<const Node256*>(curr);
                curr = nullptr;
                for (const auto& child : n256->_children) {
                    if (child != nullptr) {
                        curr = child.get();
                        break;
                    }
                }
            }

            assert(curr != nullptr);
        }

        assert(curr->_type == NodeType::LeafNode);
        return static_cast<const LeafNode*>(curr)->_full_key;
    }

    void AdaptiveRadixTrie::grow_4_to_16(std::unique_ptr<Node>& parent_slot) noexcept {
        assert(parent_slot->_type == NodeType::Node4);

        auto* old_node = static_cast<Node4*>(parent_slot.get());
        auto new_node = std::make_unique<Node16>();


        new_node->_prefix_skip_length = old_node->_prefix_skip_length;
        new_node->_child_count = old_node->_child_count;
        new_node->_type = NodeType::Node16;

        for (u16 i = 0; i < old_node->_child_count; ++i) {
            new_node->_keys[i] = old_node->_keys[i];
            new_node->_children[i] = std::move(old_node->_children[i]);
        }

        old_node->_child_count = 0;

        parent_slot = std::move(new_node);
    }

    void AdaptiveRadixTrie::grow_16_to_48(std::unique_ptr<Node>& parent_slot) noexcept
    {
        assert(parent_slot->_type == NodeType::Node16);

        auto* old_node = static_cast<Node16*>(parent_slot.get());
        auto new_node = std::make_unique<Node48>();

        new_node->_prefix_skip_length = old_node->_prefix_skip_length;
        new_node->_type = NodeType::Node48;
        new_node->_child_count = parent_slot->_child_count;

        std::fill(std::begin(new_node->_keys), std::end(new_node->_keys), static_cast<u8>(Node48::NON_EXISTING_EDGE));

        for (u8 i = 0; i < Node16::GROW_CHILD_COUNT; ++i){
            u8 key_byte = old_node->_keys[i];
            new_node->_children[i] = std::move(old_node->_children[i]);
            new_node->_keys[key_byte] = i;
        }

        old_node->_child_count = 0;

        parent_slot = std::move(new_node);
    }

    void AdaptiveRadixTrie::grow_48_to_256(std::unique_ptr<Node>& parent_slot) noexcept
    {
        assert(parent_slot->_type == NodeType::Node48);

        auto* old_node = static_cast<Node48*>(parent_slot.get());
        auto new_node = std::make_unique<Node256>();
        new_node->_type = NodeType::Node256;
        new_node->_child_count = old_node->_child_count;
        new_node->_prefix_skip_length = old_node->_prefix_skip_length;

        for (u16 key_byte = 0; key_byte < Node256::FULL; ++key_byte){
            u8 index = old_node->_keys[key_byte];
            if (index != Node48::NON_EXISTING_EDGE){
                new_node->_children[key_byte] = std::move(old_node->_children[index]);
            }
        }

        old_node->_child_count = 0;

        parent_slot = std::move(new_node);
    }

    void AdaptiveRadixTrie::shrink_256_to_48(std::unique_ptr<Node>& parent_slot) noexcept
    {
        assert(parent_slot != nullptr);
        assert(parent_slot->_type == NodeType::Node256);

        auto* old_node = static_cast<Node256*>(parent_slot.get());
        auto new_node = std::make_unique<Node48>();

        new_node->_prefix_skip_length = old_node->_prefix_skip_length;
        new_node->_child_count = old_node->_child_count;
        new_node->_type = NodeType::Node48;

        std::fill(std::begin(new_node->_keys), std::end(new_node->_keys), Node48::NON_EXISTING_EDGE);

        u8 next_free_slot = 0;
        for (u16 key_byte = 0; key_byte < Node256::FULL; ++key_byte) {
            if (old_node->_children[key_byte] != nullptr) {
                new_node->_children[next_free_slot] = std::move(old_node->_children[key_byte]);
                new_node->_keys[key_byte] = next_free_slot;
                ++next_free_slot;
            }
        }
        parent_slot = std::move(new_node);
    }

    void AdaptiveRadixTrie::shrink_48_to_16(std::unique_ptr<Node>& parent_slot) noexcept
{
    assert(parent_slot != nullptr);
    assert(parent_slot->_type == NodeType::Node48);

    auto* old_node = static_cast<Node48*>(parent_slot.get());
    auto new_node = std::make_unique<Node16>();

    // Metadaten übertragen
    new_node->_prefix_skip_length = old_node->_prefix_skip_length;
    new_node->_child_count = old_node->_child_count;
    new_node->_type = NodeType::Node16;

    u8 next_free_slot = 0;
    for (u16 key_byte = 0; key_byte < 256; ++key_byte) {
        u8 index = old_node->_keys[key_byte];
        if (index != Node48::NON_EXISTING_EDGE) {
            new_node->_keys[next_free_slot] = static_cast<u8>(key_byte);
            new_node->_children[next_free_slot] = std::move(old_node->_children[index]);
            ++next_free_slot;
        }
    }

    parent_slot = std::move(new_node);
}

void AdaptiveRadixTrie::shrink_16_to_4(std::unique_ptr<Node>& parent_slot) noexcept
{
    assert(parent_slot != nullptr);
    assert(parent_slot->_type == NodeType::Node16);

    auto* old_node = static_cast<Node16*>(parent_slot.get());
    auto new_node = std::make_unique<Node4>();

    new_node->_prefix_skip_length = old_node->_prefix_skip_length;
    new_node->_child_count = old_node->_child_count;
    new_node->_type = NodeType::Node4;

   
    for (u16 i = 0; i < old_node->_child_count; ++i) {
        new_node->_keys[i] = old_node->_keys[i];
        new_node->_children[i] = std::move(old_node->_children[i]);
    }

    parent_slot = std::move(new_node);
    }


    void AdaptiveRadixTrie::add_child(std::unique_ptr<Node>& parent, u8 key, std::unique_ptr<Node> child) noexcept
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
                static_cast<Node4*>(parent.get())->insert_pure(key, std::move(child));
                break;
            }
            case NodeType::Node16:
            {
                static_cast<Node16*>(parent.get())->insert_pure(key, std::move(child));
                break;
            }
            case NodeType::Node48:
            {
                static_cast<Node48*>(parent.get())->insert_pure(key, std::move(child));
                break;
            }
            case NodeType::Node256:
            {
                static_cast<Node256*>(parent.get())->insert_pure(key, std::move(child));
                break;
            }
            default:
                break;
        }
    }

    void AdaptiveRadixTrie::remove_child(std::unique_ptr<Node>& parent, u8 key) noexcept
{
    switch (parent->_type)
    {
        case NodeType::Node4:
            static_cast<Node4*>(parent.get())->remove_pure(key);
            break;
        case NodeType::Node16:
            static_cast<Node16*>(parent.get())->remove_pure(key);
            break;
        case NodeType::Node48:
            static_cast<Node48*>(parent.get())->remove_pure(key);
            break;
        case NodeType::Node256:
            static_cast<Node256*>(parent.get())->remove_pure(key);
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
                auto* n4 = static_cast<Node4*>(parent.get());
                std::unique_ptr<Node> last_child = std::move(n4->_children[0]);

                last_child->_prefix_skip_length += n4->_prefix_skip_length + 1;

                parent = std::move(last_child);
            }
            break;

        default:
            break;
    }
}

    u32 AdaptiveRadixTrie::check_prefix(const Node* node, std::string_view key, u32 depth) noexcept
    {
        const Node* curr = node;
        while (!curr->is_leaf())
        {
            if (curr->_type == NodeType::Node4) {
                curr = static_cast<const Node4*>(curr)->_children[0].get();
            }
            else if (curr->_type == NodeType::Node16) {
                curr = static_cast<const Node16*>(curr)->_children[0].get();
            }
            else if (curr->_type == NodeType::Node48) {
                const auto* n48 = static_cast<const Node48*>(curr);
                for (const auto& child : n48->_children) {
                    if (child) { curr = child.get(); break; }
                }
            }
            else if (curr->_type == NodeType::Node256) {
                const auto* n256 = static_cast<const Node256*>(curr);
                for (const auto& child : n256->_children) {
                    if (child) { curr = child.get(); break; }
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


    LeafNode* AdaptiveRadixTrie::find_leaf_node(std::string_view key) noexcept
    {
        Node* curr = _root.get();
        u32 depth = 0;

        while (curr != nullptr)
        {

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

    std::vector<std::string> AdaptiveRadixTrie::collect_all_leaves(Node* start_node) {
        std::vector<std::string> results;
        if (start_node == nullptr) return results;

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

            if (current->_type == NodeType::Node4) {
                auto* n4 = static_cast<Node4*>(current);
                for (int i = static_cast<int>(n4->_child_count) - 1; i >= 0; --i) {
                    node_stack.push_back(n4->_children[i].get());
                }
            }
            else if (current->_type == NodeType::Node16) {
                auto* n16 = static_cast<Node16*>(current);
                for (int i = static_cast<int>(n16->_child_count) - 1; i >= 0; --i) {
                    node_stack.push_back(n16->_children[i].get());
                }
            }
            else if (current->_type == NodeType::Node48) {
                auto* n48 = static_cast<Node48*>(current);
                for (int key_byte = 255; key_byte >= 0; --key_byte) {
                    u8 index = n48->_keys[key_byte];
                    if (index != Node48::NON_EXISTING_EDGE) {
                        node_stack.push_back(n48->_children[index].get());
                    }
                }
            }
            else if (current->_type == NodeType::Node256) {
                auto* n256 = static_cast<Node256*>(current);
                for (int i = 255; i >= 0; --i) {
                    if (n256->_children[i] != nullptr) {
                        node_stack.push_back(n256->_children[i].get());
                    }
                }
            }
        }

        return results;
    }

/*
 *Insert Function is very similar to the Code of the Paper with more out-of-bounds checking. It works recursiv and follows
 *the optimisitic approach
 */
    void AdaptiveRadixTrie::insert_node (std::unique_ptr<Node>& node, std::string_view key, std::unique_ptr<core::FileInfo> information, u32 depth)
    {
        auto leaf = std::make_unique<LeafNode>();
        leaf->_type = NodeType::LeafNode;
        leaf->_isleaf = true;
        leaf->_full_key = std::string(key);
        leaf->_metadata = std::move(information);

        if (node == nullptr){
            //replace(node, std::move(leaf));
            //assert(src != nullptr);
            node = std::move(leaf);
            return;
        }
        //Lazy Expansion
        if (node->is_leaf()){
            auto* existing_leaf = static_cast<LeafNode*>(node.get());
            if (existing_leaf->_full_key == key) {
                existing_leaf->_metadata = std::move(leaf->_metadata);
                return;
            }

            std::unique_ptr<Node> newNode = std::make_unique<Node4>();
            newNode->_type = NodeType::Node4;

            std::string_view key2 = existing_leaf->_full_key;
            u32 i = depth;
            while (i < key.length() && i < key2.length() && key[i] == key2[i]){
                i++;
            }

            newNode->_prefix_skip_length = i - depth;
            depth = depth + newNode->_prefix_skip_length;
            
            if (depth < key.length()) {
                add_child(newNode, static_cast<u8>(key[depth]), std::move(leaf));
            }
            if (depth < key2.length()) {
                add_child(newNode, static_cast<u8>(key2[depth]), std::move(node));
            }
            replace(node, std::move(newNode));
            return;
        }
        //Path Compression
        u32 p = check_prefix(node.get(), key, depth);
        if (p != node->_prefix_skip_length) {
            std::unique_ptr<Node> newNode = std::make_unique<Node4>();

            std::string_view valid_key = load_representative_key(node.get());

            if (depth+p < key.length()) {
                add_child(newNode, static_cast<u8>(key[depth+p]), std::move(leaf));
            }
            if (depth+p < valid_key.length()) {
                add_child(newNode, static_cast<u8>(valid_key[depth + p]), std::move(node));
            }
            newNode->_prefix_skip_length = p;
            if (depth + p < valid_key.length()) {
                Node* old_child = newNode->find_child(static_cast<u8>(valid_key[depth + p]));
                assert(old_child != nullptr);
                old_child->_prefix_skip_length -= (p + 1);
            }
            replace(node, std::move(newNode));
            return;

        }

        depth = depth + node->_prefix_skip_length;
        std::unique_ptr<Node>* next_slot = node->find_child_slot(static_cast<u8>(key[depth]));
        if (next_slot != nullptr) {
            insert_node(*next_slot, key, std::move(leaf->_metadata), depth + 1);
        }
        else {
            if (depth < key.length()) {
                add_child(node, static_cast<u8>(key[depth]), std::move(leaf));
            }
        }

    }


    void AdaptiveRadixTrie::delete_node(std::unique_ptr<Node>& node, std::string_view key, u32 depth) noexcept
    {
        if (node == nullptr) return;

        if (node->is_leaf()) {
            auto* leaf = static_cast<LeafNode*>(node.get());
            if (leaf->_full_key == key) {
                node.reset();
            }
            return;
        }

        u32 p = check_prefix(node.get(), key, depth);
        if (p != node->_prefix_skip_length) return;

        depth += node->_prefix_skip_length;
        u8 key_byte = static_cast<u8>(key[depth]);

        std::unique_ptr<Node>* next_slot = node->find_child_slot(key_byte);

        if (next_slot != nullptr) {
            delete_node(*next_slot, key, depth + 1);
            if (*next_slot == nullptr) {
                remove_child(node, key_byte);
            }
        }
    }

    /*
     *  Terminal-Symbol is being used to avoid loss of information. Example: Trie: main.cpp; Adding: main.c . Without the Terminal-Symbol
     *  the Trie gets splitted into: main.c -> pp . Because information cannot be stored in innernodes, the "pp" leaf will store the Information of
     *  main.cpp but not main.c . The Information of main.c will be Lost. With the Terminal Symboll it gets changedn into:
     *  Trie: main.cpp\0; Adding: main.c\0 .
     *  Split: main.c -> \0    |<- Information of main.c
     *                -> pp\0  |<- Information of main.cpp
    */
    void AdaptiveRadixTrie::insert( std::string key, std::unique_ptr<core::FileInfo> value) noexcept
    {
        key += TERMINAL;
        std::string_view view = key;
        insert_node(_root, view, std::move(value), 0);
    }

    void AdaptiveRadixTrie::remove(std::string key) noexcept
    {
        key += TERMINAL;
        std::string_view view = key;
        delete_node(_root, view, 0);
    }

    core::FileInfo* AdaptiveRadixTrie::search(std::string key) noexcept
    {
        key += TERMINAL;
        std::string_view view = key;
        LeafNode* leaf = find_leaf_node(view);
        if (leaf != nullptr) {
            return leaf->_metadata.get();
        }
        return nullptr;
    }

    std::vector<std::string> AdaptiveRadixTrie::get_all_paths_with_prefix(const std::string& prefix) {
        if (_root == nullptr) return {};

        Node* current = _root.get();
        u32 depth = 0;

        while (current != nullptr) {
            if (current->is_leaf()) {
                auto* leaf = static_cast<LeafNode*>(current);

                if (leaf->_full_key.rfind(prefix, 0) == 0) {
                    return {leaf->_full_key};
                }
                return {};
            }

            if (depth >= prefix.size()) {
                break;
            }

            if (current->_prefix_skip_length > 0) {
                u32 matched = check_prefix(current, prefix, depth);
                u32 remaining = prefix.size() - depth;
                u32 expected = std::min(current->_prefix_skip_length, remaining);

                if (matched != expected) {
                    return {};
                }

                if (remaining <= current->_prefix_skip_length) {
                    return collect_all_leaves(current);
                }
            }

            depth += current->_prefix_skip_length;

            if (depth >= prefix.size()) {
                break;
            }

            current = current->find_child(static_cast<u8>(prefix[depth]));
            ++depth;
        }

        return collect_all_leaves(current);
    }

    std::vector<std::string> AdaptiveRadixTrie::get_all_paths_with_substring(const std::string& substring) {
        if (_root == nullptr) return {};

        std::vector<std::string> results = collect_all_leaves(_root.get());

        results.erase(
            std::remove_if(results.begin(), results.end(),
                [&](const std::string& path) {
                    return path.find(substring) == std::string::npos;
                }),
            results.end());

        return results;
    }

}
