#include "adaptive-radix-trie.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

namespace pinguqueen::datastructs
{
    void AdaptiveRadixTrie::replace(std::unique_ptr<Node>& destinationSlot, std::unique_ptr<Node> sourceNode) noexcept
    {
        assert(sourceNode != nullptr);
        destinationSlot = std::move(sourceNode);
    }

    bool AdaptiveRadixTrie::is_leaf(const Node* node) noexcept
    {
        assert(node != nullptr);
        return node->is_leaf();
    }

    // Traverses from the given node down to the leftmost leaf and returns its full key.
    // This is used to obtain a representative key for path compression comparisons.
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
                const auto* node48 = static_cast<const Node48*>(curr);
                curr = nullptr;
                for (const auto& child : node48->_children) {
                    if (child != nullptr) {
                        curr = child.get();
                        break;
                    }
                }
            }
            else if (curr->_type == NodeType::Node256) {
                const auto* node256 = static_cast<const Node256*>(curr);
                curr = nullptr;
                for (const auto& child : node256->_children) {
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

    // Grows a Node4 into a Node16 by copying keys and children into the larger array.
    void AdaptiveRadixTrie::grow_4_to_16(std::unique_ptr<Node>& parentSlot) noexcept {
        assert(parentSlot->_type == NodeType::Node4);

        auto* oldNode = static_cast<Node4*>(parentSlot.get());
        auto newNode = std::make_unique<Node16>();


        newNode->_prefix_skip_length = oldNode->_prefix_skip_length;
        newNode->_child_count = oldNode->_child_count;
        newNode->_type = NodeType::Node16;

        for (u16 i = 0; i < oldNode->_child_count; ++i) {
            newNode->_keys[i] = oldNode->_keys[i];
            newNode->_children[i] = std::move(oldNode->_children[i]);
        }

        oldNode->_child_count = 0;

        parentSlot = std::move(newNode);
    }

    // Grows a Node16 into a Node48 by remapping the sorted key array into the hash-indexed layout.
    void AdaptiveRadixTrie::grow_16_to_48(std::unique_ptr<Node>& parentSlot) noexcept
    {
        assert(parentSlot->_type == NodeType::Node16);

        auto* oldNode = static_cast<Node16*>(parentSlot.get());
        auto newNode = std::make_unique<Node48>();

        newNode->_prefix_skip_length = oldNode->_prefix_skip_length;
        newNode->_type = NodeType::Node48;
        newNode->_child_count = parentSlot->_child_count;

        std::fill(std::begin(newNode->_keys), std::end(newNode->_keys), static_cast<u8>(Node48::NON_EXISTING_EDGE));

        for (u8 i = 0; i < Node16::GROW_CHILD_COUNT; ++i){
            u8 keyByte = oldNode->_keys[i];
            newNode->_children[i] = std::move(oldNode->_children[i]);
            newNode->_keys[keyByte] = i;
        }

        oldNode->_child_count = 0;

        parentSlot = std::move(newNode);
    }

    // Grows a Node48 into a Node256 by using the key bytes directly as child indices.
    void AdaptiveRadixTrie::grow_48_to_256(std::unique_ptr<Node>& parentSlot) noexcept
    {
        assert(parentSlot->_type == NodeType::Node48);

        auto* oldNode = static_cast<Node48*>(parentSlot.get());
        auto newNode = std::make_unique<Node256>();
        newNode->_type = NodeType::Node256;
        newNode->_child_count = oldNode->_child_count;
        newNode->_prefix_skip_length = oldNode->_prefix_skip_length;

        for (u16 keyByte = 0; keyByte < Node256::MAX_CHILD_COUNT; ++keyByte){
            u8 index = oldNode->_keys[keyByte];
            if (index != Node48::NON_EXISTING_EDGE){
                newNode->_children[keyByte] = std::move(oldNode->_children[index]);
            }
        }

        oldNode->_child_count = 0;

        parentSlot = std::move(newNode);
    }

    // Shrinks a Node256 into a Node48 by building the reverse key-to-index mapping.
    void AdaptiveRadixTrie::shrink_256_to_48(std::unique_ptr<Node>& parentSlot) noexcept
    {
        assert(parentSlot != nullptr);
        assert(parentSlot->_type == NodeType::Node256);

        auto* oldNode = static_cast<Node256*>(parentSlot.get());
        auto newNode = std::make_unique<Node48>();

        newNode->_prefix_skip_length = oldNode->_prefix_skip_length;
        newNode->_child_count = oldNode->_child_count;
        newNode->_type = NodeType::Node48;

        std::fill(std::begin(newNode->_keys), std::end(newNode->_keys), Node48::NON_EXISTING_EDGE);

        u8 nextFreeSlot = 0;
        for (u16 keyByte = 0; keyByte < Node256::MAX_CHILD_COUNT; ++keyByte) {
            if (oldNode->_children[keyByte] != nullptr) {
                newNode->_children[nextFreeSlot] = std::move(oldNode->_children[keyByte]);
                newNode->_keys[keyByte] = nextFreeSlot;
                ++nextFreeSlot;
            }
        }
        parentSlot = std::move(newNode);
    }

    // Shrinks a Node48 into a Node16 by iterating over all 256 key slots and collecting active children.
    void AdaptiveRadixTrie::shrink_48_to_16(std::unique_ptr<Node>& parentSlot) noexcept
{
    assert(parentSlot != nullptr);
    assert(parentSlot->_type == NodeType::Node48);

    auto* oldNode = static_cast<Node48*>(parentSlot.get());
    auto newNode = std::make_unique<Node16>();

    // Transfer metadata
    newNode->_prefix_skip_length = oldNode->_prefix_skip_length;
    newNode->_child_count = oldNode->_child_count;
    newNode->_type = NodeType::Node16;

    u8 nextFreeSlot = 0;
    for (u16 keyByte = 0; keyByte < 256; ++keyByte) {
        u8 index = oldNode->_keys[keyByte];
        if (index != Node48::NON_EXISTING_EDGE) {
            newNode->_keys[nextFreeSlot] = static_cast<u8>(keyByte);
            newNode->_children[nextFreeSlot] = std::move(oldNode->_children[index]);
            ++nextFreeSlot;
        }
    }

    parentSlot = std::move(newNode);
}

    // Shrinks a Node16 into a Node4 by copying the sorted keys and children directly.
void AdaptiveRadixTrie::shrink_16_to_4(std::unique_ptr<Node>& parentSlot) noexcept
{
    assert(parentSlot != nullptr);
    assert(parentSlot->_type == NodeType::Node16);

    auto* oldNode = static_cast<Node16*>(parentSlot.get());
    auto newNode = std::make_unique<Node4>();

    newNode->_prefix_skip_length = oldNode->_prefix_skip_length;
    newNode->_child_count = oldNode->_child_count;
    newNode->_type = NodeType::Node4;

   
    for (u16 i = 0; i < oldNode->_child_count; ++i) {
        newNode->_keys[i] = oldNode->_keys[i];
        newNode->_children[i] = std::move(oldNode->_children[i]);
    }

    parentSlot = std::move(newNode);
    }


    // Adds a child to the given parent node. If the parent is full, it is first
    // grown to the next node type (Node4 -> Node16 -> Node48 -> Node256).
    void AdaptiveRadixTrie::add_child(std::unique_ptr<Node>& parent, u8 keyByte, std::unique_ptr<Node> childNode) noexcept
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
                static_cast<Node4*>(parent.get())->insert_unchecked(keyByte, std::move(childNode));
                break;
            }
            case NodeType::Node16:
            {
                static_cast<Node16*>(parent.get())->insert_unchecked(keyByte, std::move(childNode));
                break;
            }
            case NodeType::Node48:
            {
                static_cast<Node48*>(parent.get())->insert_unchecked(keyByte, std::move(childNode));
                break;
            }
            case NodeType::Node256:
            {
                static_cast<Node256*>(parent.get())->insert_unchecked(keyByte, std::move(childNode));
                break;
            }
            default:
                break;
        }
    }

    // Removes a child from the given parent node. If the parent has too few children,
    // it is shrunk to the previous node type (Node256 -> Node48 -> Node16 -> Node4).
    void AdaptiveRadixTrie::remove_child(std::unique_ptr<Node>& parent, u8 keyByte) noexcept
{
    switch (parent->_type)
    {
        case NodeType::Node4:
            static_cast<Node4*>(parent.get())->remove_unchecked(keyByte);
            break;
        case NodeType::Node16:
            static_cast<Node16*>(parent.get())->remove_unchecked(keyByte);
            break;
        case NodeType::Node48:
            static_cast<Node48*>(parent.get())->remove_unchecked(keyByte);
            break;
        case NodeType::Node256:
            static_cast<Node256*>(parent.get())->remove_unchecked(keyByte);
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
                auto* node4 = static_cast<Node4*>(parent.get());
                std::unique_ptr<Node> lastChild = std::move(node4->_children[0]);

                lastChild->_prefix_skip_length += node4->_prefix_skip_length + 1;

                parent = std::move(lastChild);
            }
            break;

        default:
            break;
    }
}

    // Loads the representative key of a node and compares it character by character
    // with the search key. Returns the number of matching characters.
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
                const auto* node48 = static_cast<const Node48*>(curr);
                for (const auto& child : node48->_children) {
                    if (child) { curr = child.get(); break; }
                }
            }
            else if (curr->_type == NodeType::Node256) {
                const auto* node256 = static_cast<const Node256*>(curr);
                for (const auto& child : node256->_children) {
                    if (child) { curr = child.get(); break; }
                }
            }
        }

        std::string_view validKey = static_cast<const LeafNode*>(curr)->_full_key;

        u32 matchedLength = 0;
        while (matchedLength < node->_prefix_skip_length &&
               (depth + matchedLength) < key.length() &&
               key[depth + matchedLength] == validKey[depth + matchedLength])
        {
            matchedLength++;
        }

        return matchedLength;
    }


    // Traverses the trie from root to leaf, following the path that matches the given key.
    // Handles path compression by verifying skipped segments via check_prefix.
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
                return nullptr; // Key not found at end of path
            }

            if (curr->_prefix_skip_length > 0){
                u32 matchedLength = check_prefix(curr, key, depth);
                if (matchedLength != curr->_prefix_skip_length) {
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

    // Iteratively collects all leaf keys in the subtree rooted at startNode.
    // Uses an explicit stack instead of recursion to avoid stack overflow on deep tries.
    std::vector<std::string> AdaptiveRadixTrie::collect_all_leaves(Node* startNode) {
        std::vector<std::string> results;
        if (startNode == nullptr) return results;

        std::vector<Node*> nodeStack;
        nodeStack.reserve(256);
        nodeStack.push_back(startNode);

        while (!nodeStack.empty()) {
            Node* current = nodeStack.back();
            nodeStack.pop_back();

            if (current->is_leaf()) {
                auto* leaf = static_cast<LeafNode*>(current);
                results.push_back(leaf->_full_key);
                continue;
            }

            if (current->_type == NodeType::Node4) {
                auto* node4 = static_cast<Node4*>(current);
                for (int i = static_cast<int>(node4->_child_count) - 1; i >= 0; --i) {
                    nodeStack.push_back(node4->_children[i].get());
                }
            }
            else if (current->_type == NodeType::Node16) {
                auto* node16 = static_cast<Node16*>(current);
                for (int i = static_cast<int>(node16->_child_count) - 1; i >= 0; --i) {
                    nodeStack.push_back(node16->_children[i].get());
                }
            }
            else if (current->_type == NodeType::Node48) {
                auto* node48 = static_cast<Node48*>(current);
                for (int keyByte = 255; keyByte >= 0; --keyByte) {
                    u8 index = node48->_keys[keyByte];
                    if (index != Node48::NON_EXISTING_EDGE) {
                        nodeStack.push_back(node48->_children[index].get());
                    }
                }
            }
            else if (current->_type == NodeType::Node256) {
                auto* node256 = static_cast<Node256*>(current);
                for (int i = 255; i >= 0; --i) {
                    if (node256->_children[i] != nullptr) {
                        nodeStack.push_back(node256->_children[i].get());
                    }
                }
            }
        }

        return results;
    }

    // Recursive insert following the optimistic approach from the paper.
    // Handles three cases:
    //   1. Empty path -> create new leaf
    //   2. Lazy Expansion -> split a leaf by computing the common prefix
    //   3. Path Compression -> split the compressed segment where it diverges
    void AdaptiveRadixTrie::insert_node(std::unique_ptr<Node>& node, std::string_view key, std::unique_ptr<core::FileInfo> information, u32 depth)
    {
        auto leaf = std::make_unique<LeafNode>();
        leaf->_type = NodeType::LeafNode;
        leaf->_isleaf = true;
        leaf->_full_key = std::string(key);
        leaf->_metadata = std::move(information);

        if (node == nullptr){
            replace(node, std::move(leaf));
            return;
        }
        // Lazy Expansion: If we hit a leaf, split it by computing the common prefix
        // between the existing key and the new key, then create a Node4 with two children.
        if (node->is_leaf()){
            auto* existingLeaf = static_cast<LeafNode*>(node.get());
            if (existingLeaf->_full_key == key) {
                existingLeaf->_metadata = std::move(leaf->_metadata);
                return;
            }

            std::unique_ptr<Node> newNode = std::make_unique<Node4>();
            newNode->_type = NodeType::Node4;

            std::string_view existingKey = existingLeaf->_full_key;
            u32 commonPrefixLen = depth;
            while (commonPrefixLen < key.length() && commonPrefixLen < existingKey.length() && key[commonPrefixLen] == existingKey[commonPrefixLen]){
                commonPrefixLen++;
            }

            newNode->_prefix_skip_length = commonPrefixLen - depth;
            depth = depth + newNode->_prefix_skip_length;
            
            if (depth < key.length()) {
                add_child(newNode, static_cast<u8>(key[depth]), std::move(leaf));
            }
            if (depth < existingKey.length()) {
                add_child(newNode, static_cast<u8>(existingKey[depth]), std::move(node));
            }
            replace(node, std::move(newNode));
            return;
        }
        // Path Compression: The compressed segment diverges from the search key.
        // Split the segment at the divergence point and create a new Node4 with
        // the new leaf and the old subtree as children.
        u32 matchedLength = check_prefix(node.get(), key, depth);
        if (matchedLength != node->_prefix_skip_length) {
            std::unique_ptr<Node> newNode = std::make_unique<Node4>();

            std::string_view validKey = load_representative_key(node.get());

            if (depth+matchedLength < key.length()) {
                add_child(newNode, static_cast<u8>(key[depth+matchedLength]), std::move(leaf));
            }
            if (depth+matchedLength < validKey.length()) {
                add_child(newNode, static_cast<u8>(validKey[depth + matchedLength]), std::move(node));
            }
            newNode->_prefix_skip_length = matchedLength;
            if (depth + matchedLength < validKey.length()) {
                Node* oldChild = newNode->find_child(static_cast<u8>(validKey[depth + matchedLength]));
                assert(oldChild != nullptr);
                oldChild->_prefix_skip_length -= (matchedLength + 1);
            }
            replace(node, std::move(newNode));
            return;

        }

        depth = depth + node->_prefix_skip_length;
        std::unique_ptr<Node>* nextSlot = node->find_child_slot(static_cast<u8>(key[depth]));
        if (nextSlot != nullptr) {
            insert_node(*nextSlot, key, std::move(leaf->_metadata), depth + 1);
        }
        else {
            if (depth < key.length()) {
                add_child(node, static_cast<u8>(key[depth]), std::move(leaf));
            }
        }

    }


    // Recursive delete that removes the leaf matching the key and cleans up
    // empty nodes on the way back up via remove_child.
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

        u32 matchedLength = check_prefix(node.get(), key, depth);
        if (matchedLength != node->_prefix_skip_length) return;

        depth += node->_prefix_skip_length;
        u8 keyByte = static_cast<u8>(key[depth]);

        std::unique_ptr<Node>* nextSlot = node->find_child_slot(keyByte);

        if (nextSlot != nullptr) {
            delete_node(*nextSlot, key, depth + 1);
            // Post-order cleanup: if the recursive delete emptied this child,
            // remove it from the parent and potentially shrink the parent node.
            if (*nextSlot == nullptr) {
                remove_child(node, keyByte);
            }
        }
    }

    /*
     * The terminal symbol ('\0') is appended to every key to avoid loss of information.
     * Example: Trie contains "main.cpp"; inserting "main.c" would split the path into
     * "main.c" -> "pp", where the "pp" leaf stores main.cpp's info but main.c's info is lost.
     * With the terminal symbol it becomes:
     *   "main.cpp\0" and "main.c\0" which split cleanly:
     *     "main.c" -> "\0"   (info of main.c)
     *              -> "pp\0" (info of main.cpp)
    */
    // Appends the terminal symbol to the key and delegates to insert_node.
    void AdaptiveRadixTrie::insert( std::string key, std::unique_ptr<core::FileInfo> value) noexcept
    {
        key += TERMINAL;
        std::string_view view = key;
        insert_node(_root, view, std::move(value), 0);
    }

    // Appends the terminal symbol to the key and delegates to delete_node.
    void AdaptiveRadixTrie::remove(std::string key) noexcept
    {
        key += TERMINAL;
        std::string_view view = key;
        delete_node(_root, view, 0);
    }

    // Appends the terminal symbol, finds the matching leaf, and returns its metadata.
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


    // Traverses the trie following the prefix path, handling path compression along the way.
    // Once the entire prefix is matched, collects all leaves under the current node.
    std::vector<std::string> AdaptiveRadixTrie::get_all_paths_with_prefix(const std::string& prefix)
    {
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

            // Check if the compressed path matches the search prefix
            if (current->_prefix_skip_length > 0) {
                u32 matchedLength = check_prefix(current, prefix, depth);
                u32 remaining = prefix.size() - depth;
                u32 expected = std::min(current->_prefix_skip_length, remaining);

                if (matchedLength != expected) {
                    return {};
                }

                if (remaining <= current->_prefix_skip_length) {
                    return collect_all_leaves(current);
                }
            }

            // Advance depth and descend into the matching child
            depth += current->_prefix_skip_length;

            if (depth >= prefix.size()) {
                break;
            }

            current = current->find_child(static_cast<u8>(prefix[depth]));
            ++depth;
        }

        // Fallback: collect all leaves if the prefix matched up to this point
        return collect_all_leaves(current);
    }

    // Collects all leaves and filters them by checking if the substring is contained in each path.
    std::vector<std::string> AdaptiveRadixTrie::get_all_paths_with_substring(const std::string& substring)
    {
        if (_root == nullptr) return {};

        std::vector<std::string> results = collect_all_leaves(_root.get());

        // Erase-Remove idiom: remove_if shifts non-matching paths to the front,
        // then erase trims the leftover elements from the vector.
        results.erase(
            std::remove_if(results.begin(), results.end(),
                [&](const std::string& path) {
                    return path.find(substring) == std::string::npos;
                }),
            results.end());

        return results;
    }

}
