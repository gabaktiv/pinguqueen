#include "support/test-suite.hpp"

#include "radix-trie/file-info.hpp"
#include "radix-trie/radix-trie.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using namespace pinguqueen;
    using namespace pinguqueen::intern;

    struct TrieFixture {
        Node* root = nullptr;
        std::vector<std::unique_ptr<FileInfo>> metadata;

        ~TrieFixture()
        {
            delete root;
        }

        FileInfo* make_file(std::string name, u32 size)
        {
            auto info = std::make_unique<FileInfo>();
            info->file_name = std::move(name);
            info->file_size_bytes = size;

            FileInfo* raw = info.get();
            metadata.push_back(std::move(info));
            return raw;
        }

        FileInfo* insert(std::string_view key, u32 size = 1)
        {
            FileInfo* info = make_file(std::string(key), size);
            RadixTrie::insert_node(root, key, info, 0);
            return info;
        }

        void erase(std::string_view key)
        {
            RadixTrie::delete_node(root, key, 0);
        }
    };

    std::string key_with_suffix(unsigned char suffix)
    {
        std::string key = "p";
        key.push_back(static_cast<char>(suffix));
        return key;
    }

    const LeafNode* as_leaf(const Node* node)
    {
        if (node == nullptr || !node->is_leaf()) {
            return nullptr;
        }
        return static_cast<const LeafNode*>(node);
    }

    const LeafNode* find_leaf(Node* root, std::string_view key)
    {
        Node* current = root;
        u32 depth = 0;

        while (current != nullptr) {
            if (const LeafNode* leaf = as_leaf(current)) {
                return leaf->_full_key == key ? leaf : nullptr;
            }

            depth += current->_prefix_skip_length;
            if (depth >= key.size()) {
                return nullptr;
            }

            current = current->find_child(static_cast<u8>(key[depth]));
            ++depth;
        }

        return nullptr;
    }

    void insert_suffix_range(TrieFixture& fixture, unsigned char first, unsigned char last)
    {
        for (unsigned char suffix = first; suffix <= last; ++suffix) {
            fixture.insert(key_with_suffix(suffix), suffix);
        }
    }

    Node* child_for_suffix(Node* root, unsigned char suffix)
    {
        if (root == nullptr) {
            return nullptr;
        }
        return root->find_child(suffix);
    }

}

int main(int argc, char** argv)
{
    const std::string_view test_filter = argc > 1 ? std::string_view(argv[1]) : std::string_view();
    pinguqueen::tests::TestSuite suite("RadixTrie", test_filter);

    suite.run("insert_empty_creates_leaf", [&] {
        TrieFixture fixture;

        FileInfo* metadata = fixture.insert("alpha.txt", 42);

        const LeafNode* leaf = as_leaf(fixture.root);
        PQ_EXPECT(leaf != nullptr);
        PQ_EXPECT_EQ(leaf->_full_key, std::string("alpha.txt"));
        PQ_EXPECT_EQ(leaf->_metadata, metadata);
    });

    suite.run("split_shared_prefix_type_is_node4", [&] {
        TrieFixture fixture;

        fixture.insert("folder/a.txt", 10);
        fixture.insert("folder/b.txt", 20);

        PQ_EXPECT(fixture.root != nullptr);
        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node4);
    });

    suite.run("split_shared_prefix_skip_length", [&] {
        TrieFixture fixture;

        fixture.insert("folder/a.txt", 10);
        fixture.insert("folder/b.txt", 20);

        PQ_EXPECT(fixture.root != nullptr);
        PQ_EXPECT_EQ(fixture.root->_prefix_skip_length, 7U);
    });

    suite.run("split_shared_prefix_child_count", [&] {
        TrieFixture fixture;

        fixture.insert("folder/a.txt", 10);
        fixture.insert("folder/b.txt", 20);

        PQ_EXPECT(fixture.root != nullptr);
        PQ_EXPECT_EQ(fixture.root->_child_count, 2U);
    });

    suite.run("split_shared_prefix_left_leaf_reachable", [&] {
        TrieFixture fixture;

        FileInfo* left = fixture.insert("folder/a.txt", 10);
        fixture.insert("folder/b.txt", 20);
        const LeafNode* leaf = find_leaf(fixture.root, "folder/a.txt");

        PQ_EXPECT(leaf != nullptr);
        if (leaf != nullptr) {
            PQ_EXPECT_EQ(leaf->_metadata, left);
        }
    });

    suite.run("split_shared_prefix_right_leaf_reachable", [&] {
        TrieFixture fixture;

        fixture.insert("folder/a.txt", 10);
        FileInfo* right = fixture.insert("folder/b.txt", 20);
        const LeafNode* leaf = find_leaf(fixture.root, "folder/b.txt");

        PQ_EXPECT(leaf != nullptr);
        if (leaf != nullptr) {
            PQ_EXPECT_EQ(leaf->_metadata, right);
        }
    });

    suite.run("split_shared_prefix_missing_leaf_not_found", [&] {
        TrieFixture fixture;

        fixture.insert("folder/a.txt", 10);
        fixture.insert("folder/b.txt", 20);

        PQ_EXPECT(find_leaf(fixture.root, "folder/c.txt") == nullptr);
    });

    suite.run("node4_three_children_keeps_first_leaf", [&] {
        TrieFixture fixture;

        FileInfo* a = fixture.insert("pkg/a.hpp", 11);
        fixture.insert("pkg/b.hpp", 12);
        fixture.insert("pkg/c.hpp", 13);
        const LeafNode* leaf = find_leaf(fixture.root, "pkg/a.hpp");

        PQ_EXPECT(leaf != nullptr);
        if (leaf != nullptr) {
            PQ_EXPECT_EQ(leaf->_metadata, a);
        }
    });

    suite.run("grow_node4_to_node16_type", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 5);

        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node16);
    });

    suite.run("grow_node4_to_node16_child_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 5);

        PQ_EXPECT_EQ(fixture.root->_child_count, 5U);
    });

    suite.run("grow_node4_to_node16_keeps_edge_children", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 5);

        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(1)) != nullptr);
        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(5)) != nullptr);
    });

    suite.run("grow_node16_to_node48_type", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 17);

        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node48);
    });

    suite.run("grow_node16_to_node48_child_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 17);

        PQ_EXPECT_EQ(fixture.root->_child_count, 17U);
    });

    suite.run("grow_node16_to_node48_keeps_edge_children", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 17);

        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(1)) != nullptr);
        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(17)) != nullptr);
    });

    suite.run("node48_before_grow_has_expected_prefix", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 48);

        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node48);
        PQ_EXPECT_EQ(fixture.root->_prefix_skip_length, 1U);
    });

    suite.run("node48_before_grow_has_expected_child_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 48);

        PQ_EXPECT_EQ(fixture.root->_child_count, 48U);
    });

    suite.run("node48_before_grow_edge_children_reachable", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 48);

        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(1)) != nullptr);
        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(48)) != nullptr);
    });

    suite.run("grow_node48_to_node256_type", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);

        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node256);
    });

    suite.run("grow_node48_to_node256_child_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);

        PQ_EXPECT_EQ(fixture.root->_child_count, 49U);
    });

    suite.run("grow_node48_to_node256_preserves_prefix_skip", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);

        PQ_EXPECT_EQ(fixture.root->_prefix_skip_length, 1U);
    });

    suite.run("grow_node48_to_node256_maps_first_child_directly", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);

        PQ_EXPECT(child_for_suffix(fixture.root, 1) != nullptr);
    });

    suite.run("grow_node48_to_node256_maps_last_child_directly", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);

        PQ_EXPECT(child_for_suffix(fixture.root, 49) != nullptr);
    });

    suite.run("grow_node48_to_node256_first_leaf_reachable", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);

        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(1)) != nullptr);
    });

    suite.run("grow_node48_to_node256_last_leaf_reachable", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);

        PQ_EXPECT(find_leaf(fixture.root, key_with_suffix(49)) != nullptr);
    });

    suite.run("delete_node4_removes_target_leaf", [&] {
        TrieFixture fixture;

        fixture.insert("set/a", 1);
        fixture.insert("set/b", 2);
        fixture.insert("set/c", 3);

        fixture.erase("set/b");

        PQ_EXPECT(find_leaf(fixture.root, "set/b") == nullptr);
    });

    suite.run("delete_node4_keeps_sibling_leaves", [&] {
        TrieFixture fixture;

        fixture.insert("set/a", 1);
        fixture.insert("set/b", 2);
        fixture.insert("set/c", 3);

        fixture.erase("set/b");

        PQ_EXPECT(find_leaf(fixture.root, "set/a") != nullptr);
        PQ_EXPECT(find_leaf(fixture.root, "set/c") != nullptr);
    });

    suite.run("shrink_node16_to_node4_type", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 5);
        fixture.erase(key_with_suffix(5));
        fixture.erase(key_with_suffix(4));

        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node4);
    });

    suite.run("shrink_node16_to_node4_child_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 5);
        fixture.erase(key_with_suffix(5));
        fixture.erase(key_with_suffix(4));

        PQ_EXPECT_EQ(fixture.root->_child_count, 3U);
    });

    suite.run("shrink_node48_to_node16_type", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 17);
        fixture.erase(key_with_suffix(17));
        fixture.erase(key_with_suffix(16));

        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node16);
    });

    suite.run("shrink_node48_to_node16_child_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 17);
        fixture.erase(key_with_suffix(17));
        fixture.erase(key_with_suffix(16));

        PQ_EXPECT_EQ(fixture.root->_child_count, 15U);
    });

    suite.run("delete_node256_single_child_reduces_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);
        fixture.erase(key_with_suffix(49));

        PQ_EXPECT_EQ(fixture.root->_child_count, 48U);
    });

    suite.run("delete_node256_single_child_removes_direct_child", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);
        fixture.erase(key_with_suffix(49));

        PQ_EXPECT(child_for_suffix(fixture.root, 49) == nullptr);
    });

    suite.run("shrink_node256_to_node48_type", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);
        fixture.erase(key_with_suffix(49));
        fixture.erase(key_with_suffix(48));

        PQ_EXPECT_EQ(fixture.root->_type, NodeType::Node48);
    });

    suite.run("shrink_node256_to_node48_child_count", [&] {
        TrieFixture fixture;

        insert_suffix_range(fixture, 1, 49);
        fixture.erase(key_with_suffix(49));
        fixture.erase(key_with_suffix(48));

        PQ_EXPECT_EQ(fixture.root->_child_count, 47U);
    });

    return suite.finish();
}
