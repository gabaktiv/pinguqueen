#include <gtest/gtest.h>

#include "radix-trie/file-info.hpp"
#include "radix-trie/node.hpp"
#include "radix-trie/radix-trie.hpp"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace pinguqueen;
using namespace pinguqueen::intern;

struct RadixTrieFixture : testing::Test {
    std::unique_ptr<Node> root = nullptr;
    std::vector<std::unique_ptr<FileInfo>> metadata;

    FileInfo* make_file(std::string name, u32 size = 1)
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

void insert_suffix_range(RadixTrieFixture& fixture, unsigned char first, unsigned char last)
{
    for (unsigned char suffix = first; suffix <= last; ++suffix) {
        fixture.insert(key_with_suffix(suffix), suffix);
    }
}

std::vector<std::string> collect_keys(Node* root);

void expect_only_keys(Node* root, std::vector<std::string> expected)
{
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(collect_keys(root), expected);
}

void expect_present(Node* root, const std::vector<std::string>& keys)
{
    for (const std::string& key : keys) {
        EXPECT_NE(find_leaf(root, key), nullptr) << key;
    }
}

void expect_absent(Node* root, const std::vector<std::string>& keys)
{
    for (const std::string& key : keys) {
        EXPECT_EQ(find_leaf(root, key), nullptr) << key;
    }
}

std::vector<std::string> collect_keys(Node* root)
{
    std::vector<std::string> keys;
    std::vector<Node*> stack;

    if (root != nullptr) {
        stack.push_back(root);
    }

    while (!stack.empty()) {
        Node* current = stack.back();
        stack.pop_back();

        if (const LeafNode* leaf = as_leaf(current)) {
            keys.push_back(leaf->_full_key);
            continue;
        }

        if (current->_type == NodeType::Node4) {
            auto* node = static_cast<Node4*>(current);
            for (u16 i = 0; i < node->_child_count; ++i) {
                stack.push_back(node->_children[i].get());
            }
        }
        else if (current->_type == NodeType::Node16) {
            auto* node = static_cast<Node16*>(current);
            for (u16 i = 0; i < node->_child_count; ++i) {
                stack.push_back(node->_children[i].get());
            }
        }
        else if (current->_type == NodeType::Node48) {
            auto* node = static_cast<Node48*>(current);
            for (const auto& child : node->_children) {
                if (child != nullptr) {
                    stack.push_back(child.get());
                }
            }
        }
        else if (current->_type == NodeType::Node256) {
            auto* node = static_cast<Node256*>(current);
            for (const auto& child : node->_children) {
                if (child != nullptr) {
                    stack.push_back(child.get());
                }
            }
        }
    }

    std::sort(keys.begin(), keys.end());
    return keys;
}

} // namespace

TEST_F(RadixTrieFixture, InsertIntoEmptyRootCreatesLeaf)
{
    FileInfo* info = insert("workspace/main.cpp", 42);

    const LeafNode* leaf = as_leaf(root.get());

    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(leaf->_full_key, "workspace/main.cpp");
    EXPECT_EQ(leaf->_metadata, info);
}

TEST_F(RadixTrieFixture, SharedPrefixSplitCreatesNode4WithBothLeavesReachable)
{
    FileInfo* car = insert("car", 1);
    FileInfo* cat = insert("cat", 2);

    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->_type, NodeType::Node4);
    EXPECT_EQ(root->_prefix_skip_length, 2U);
    EXPECT_EQ(root->_child_count, 2U);

    const LeafNode* car_leaf = find_leaf(root.get(), "car");
    const LeafNode* cat_leaf = find_leaf(root.get(), "cat");

    ASSERT_NE(car_leaf, nullptr);
    ASSERT_NE(cat_leaf, nullptr);
    EXPECT_EQ(car_leaf->_metadata, car);
    EXPECT_EQ(cat_leaf->_metadata, cat);
    EXPECT_EQ(find_leaf(root.get(), "cab"), nullptr);
}

TEST_F(RadixTrieFixture, Node4KeepsChildrenSortedByEdgeByte)
{
    insert("pd");
    insert("pb");
    insert("pc");
    insert("pa");

    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->_type, NodeType::Node4);

    const auto* node = static_cast<const Node4*>(root.get());
    ASSERT_EQ(node->_child_count, 4U);
    EXPECT_EQ(node->_keys[0], static_cast<u8>('a'));
    EXPECT_EQ(node->_keys[1], static_cast<u8>('b'));
    EXPECT_EQ(node->_keys[2], static_cast<u8>('c'));
    EXPECT_EQ(node->_keys[3], static_cast<u8>('d'));
}

TEST_F(RadixTrieFixture, GrowsFromNode4ToNode16)
{
    insert_suffix_range(*this, 1, 5);

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->_type, NodeType::Node16);
    EXPECT_EQ(root->_child_count, 5U);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(1)), nullptr);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(5)), nullptr);
}

TEST_F(RadixTrieFixture, GrowsFromNode16ToNode48)
{
    insert_suffix_range(*this, 1, 17);

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->_type, NodeType::Node48);
    EXPECT_EQ(root->_child_count, 17U);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(1)), nullptr);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(17)), nullptr);
}

TEST_F(RadixTrieFixture, GrowsFromNode48ToNode256)
{
    insert_suffix_range(*this, 1, 49);

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->_type, NodeType::Node256);
    EXPECT_EQ(root->_child_count, 49U);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(1)), nullptr);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(49)), nullptr);
}

TEST_F(RadixTrieFixture, DeleteExistingLeafRemovesOnlyThatKey)
{
    insert("pa");
    insert("pb");
    insert("pc");

    erase("pb");

    EXPECT_NE(find_leaf(root.get(), "pa"), nullptr);
    EXPECT_EQ(find_leaf(root.get(), "pb"), nullptr);
    EXPECT_NE(find_leaf(root.get(), "pc"), nullptr);
}

TEST_F(RadixTrieFixture, DeletingAllKeysLeavesEmptyRoot)
{
    insert("pa");
    insert("pb");
    insert("pc");

    erase("pa");
    erase("pb");
    erase("pc");

    EXPECT_EQ(root, nullptr);
}

TEST_F(RadixTrieFixture, DeletingMissingKeyDoesNotChangeExistingKeys)
{
    insert("alpha");
    insert("alpine");

    erase("altitude");

    EXPECT_NE(find_leaf(root.get(), "alpha"), nullptr);
    EXPECT_NE(find_leaf(root.get(), "alpine"), nullptr);
}

TEST_F(RadixTrieFixture, ShrinksFromNode48ToNode16)
{
    insert_suffix_range(*this, 1, 17);

    erase(key_with_suffix(17));
    erase(key_with_suffix(16));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->_type, NodeType::Node16);
    EXPECT_EQ(root->_child_count, 15U);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(1)), nullptr);
    EXPECT_EQ(find_leaf(root.get(), key_with_suffix(17)), nullptr);
}

TEST_F(RadixTrieFixture, ShrinksFromNode256ToNode48)
{
    insert_suffix_range(*this, 1, 49);

    erase(key_with_suffix(49));
    erase(key_with_suffix(48));

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->_type, NodeType::Node48);
    EXPECT_EQ(root->_child_count, 47U);
    EXPECT_NE(find_leaf(root.get(), key_with_suffix(1)), nullptr);
    EXPECT_EQ(find_leaf(root.get(), key_with_suffix(49)), nullptr);
}

TEST_F(RadixTrieFixture, CollectsAllInsertedKeysAfterDeepSharedPrefixInserts)
{
    std::vector<std::string> keys = {
        "workspace/project/src/main.cpp",
        "workspace/project/src/main.hpp",
        "workspace/project/tests/main_test.cpp",
        "workspace/readme.md",
    };

    for (const std::string& key : keys) {
        insert(key);
    }

    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(collect_keys(root.get()), keys);
}

TEST_F(RadixTrieFixture, DuplicateInsertKeepsKeyReachableExactlyOnce)
{
    FileInfo* first = insert("workspace/project/file.cpp", 1);
    FileInfo* second = insert("workspace/project/file.cpp", 2);

    const LeafNode* leaf = find_leaf(root.get(), "workspace/project/file.cpp");

    ASSERT_NE(leaf, nullptr);
    EXPECT_TRUE(leaf->_metadata == first || leaf->_metadata == second);
    expect_only_keys(root.get(), {"workspace/project/file.cpp"});
}

TEST_F(RadixTrieFixture, RepeatedDuplicateInsertsDoNotDestroyNeighborKeys)
{
    insert("workspace/project/file.cpp", 1);
    insert("workspace/project/file.hpp", 2);

    for (u32 i = 0; i < 25; ++i) {
        insert("workspace/project/file.cpp", 100 + i);
    }

    expect_only_keys(root.get(), {
        "workspace/project/file.cpp",
        "workspace/project/file.hpp",
    });
    expect_present(root.get(), {
        "workspace/project/file.cpp",
        "workspace/project/file.hpp",
    });
}

TEST_F(RadixTrieFixture, InsertingShorterKeyThatIsPrefixOfExistingKeyKeepsBoth)
{
    insert("workspace/project/src/main.cpp", 1);
    insert("workspace/project/src", 2);

    expect_only_keys(root.get(), {
        "workspace/project/src",
        "workspace/project/src/main.cpp",
    });
}

TEST_F(RadixTrieFixture, InsertingLongerKeyBelowExistingPrefixKeepsBoth)
{
    insert("workspace/project/src", 1);
    insert("workspace/project/src/main.cpp", 2);

    expect_only_keys(root.get(), {
        "workspace/project/src",
        "workspace/project/src/main.cpp",
    });
}

TEST_F(RadixTrieFixture, PrefixFamilySurvivesMixedInsertionOrder)
{
    std::vector<std::string> keys = {
        "a/b/c/d",
        "a",
        "a/b/c",
        "a/b",
        "a/b/c/d/e",
        "a/bb",
        "a/b/cd",
    };

    for (const std::string& key : keys) {
        insert(key);
    }

    expect_only_keys(root.get(), keys);
}

TEST_F(RadixTrieFixture, DeleteKeyThatIsPrefixOfAnotherRemovesOnlyExactKey)
{
    insert("workspace");
    insert("workspace/project");
    insert("workspace/project/file.cpp");

    erase("workspace");

    expect_only_keys(root.get(), {
        "workspace/project",
        "workspace/project/file.cpp",
    });
    expect_absent(root.get(), {"workspace"});
}

TEST_F(RadixTrieFixture, DeleteLongerMissingKeyBelowExistingLeafDoesNotChangeTree)
{
    std::vector<std::string> keys = {
        "alpha",
        "alpine",
        "altitude",
    };

    for (const std::string& key : keys) {
        insert(key);
    }

    erase("alpha/missing/child");
    erase("alpine/missing/child");

    expect_only_keys(root.get(), keys);
}

TEST_F(RadixTrieFixture, DeleteShorterMissingPrefixDoesNotChangeTree)
{
    std::vector<std::string> keys = {
        "workspace/project/src/main.cpp",
        "workspace/project/tests/main-test.cpp",
        "workspace/readme.md",
    };

    for (const std::string& key : keys) {
        insert(key);
    }

    erase("workspace");
    erase("workspace/project");
    erase("workspace/project/src");

    expect_only_keys(root.get(), keys);
}

TEST_F(RadixTrieFixture, AllByteSuffixesAreReachableAfterNode256Growth)
{
    std::vector<std::string> keys;
    keys.reserve(255);

    for (u16 suffix = 1; suffix <= 255; ++suffix) {
        std::string key = key_with_suffix(static_cast<unsigned char>(suffix));
        keys.push_back(key);
        insert(key, suffix);
    }

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->_type, NodeType::Node256);
    EXPECT_EQ(root->_child_count, 255U);
    expect_only_keys(root.get(), keys);
}

TEST_F(RadixTrieFixture, DeleteEverySecondByteSuffixPreservesTheRest)
{
    std::vector<std::string> remaining;
    remaining.reserve(128);

    for (u16 suffix = 1; suffix <= 255; ++suffix) {
        insert(key_with_suffix(static_cast<unsigned char>(suffix)), suffix);
    }

    for (u16 suffix = 1; suffix <= 255; suffix += 2) {
        erase(key_with_suffix(static_cast<unsigned char>(suffix)));
    }

    for (u16 suffix = 2; suffix <= 254; suffix += 2) {
        remaining.push_back(key_with_suffix(static_cast<unsigned char>(suffix)));
    }

    expect_only_keys(root.get(), remaining);
}

TEST_F(RadixTrieFixture, DeleteAllByteSuffixesInReverseLeavesEmptyRoot)
{
    for (u16 suffix = 1; suffix <= 255; ++suffix) {
        insert(key_with_suffix(static_cast<unsigned char>(suffix)), suffix);
    }

    for (u16 suffix = 255; suffix >= 1; --suffix) {
        erase(key_with_suffix(static_cast<unsigned char>(suffix)));
        if (suffix == 1) {
            break;
        }
    }

    EXPECT_EQ(root, nullptr);
}

TEST_F(RadixTrieFixture, InterleavedInsertDeleteMatchesReferenceSet)
{
    std::set<std::string> expected;

    for (u32 i = 0; i < 300; ++i) {
        std::string key = "workspace/module_" + std::to_string(i % 37)
            + "/sub_" + std::to_string((i * 17) % 53)
            + "/file_" + std::to_string(i) + ".cpp";
        insert(key, i);
        expected.insert(key);

        if (i % 5 == 0) {
            std::string missing = "workspace/missing_" + std::to_string(i);
            erase(missing);
        }

        if (i >= 40 && i % 7 == 0) {
            std::string remove_key = "workspace/module_" + std::to_string((i - 40) % 37)
                + "/sub_" + std::to_string(((i - 40) * 17) % 53)
                + "/file_" + std::to_string(i - 40) + ".cpp";
            erase(remove_key);
            expected.erase(remove_key);
        }
    }

    std::vector<std::string> expected_keys(expected.begin(), expected.end());
    expect_only_keys(root.get(), expected_keys);
}

TEST_F(RadixTrieFixture, DeepSharedPrefixRepeatedSplitsRemainReachable)
{
    std::vector<std::string> keys;
    keys.reserve(150);

    for (u32 i = 0; i < 150; ++i) {
        std::string key = "workspace/project/src/generated/common/prefix/";
        key += static_cast<char>('a' + static_cast<int>(i % 26));
        key += "/branch/";
        key += std::to_string((i * 31) % 97);
        key += "/file_";
        key += std::to_string(i);
        key += ".hpp";
        keys.push_back(key);
    }

    for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
        insert(*it);
    }

    expect_only_keys(root.get(), keys);
}

TEST(RadixTriePrefixSearch, EmptyTrieReturnsNoPaths)
{
    RadixTrie trie;

    EXPECT_TRUE(trie.get_all_paths_with_prefix("workspace").empty());
}
