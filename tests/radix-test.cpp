#include <gtest/gtest.h>

#include "../src/core/file-info.hpp"
#include "adaptive-radix-trie/node.hpp"
#include "adaptive-radix-trie/adaptive-radix-trie.hpp"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace pinguqueen;
using namespace pinguqueen::datastructs;

struct AdaptiveRadixTrieFixture : testing::Test {
    AdaptiveRadixTrie trie;

    core::FileInfo* insert(std::string_view key, u32 size = 1)
    {
        auto info = std::make_unique<core::FileInfo>(std::string(key), size);
        core::FileInfo* raw = info.get();
        trie.insert(std::string(key), std::move(info));
        return raw;
    }

    void erase(std::string key)
    {
        trie.remove(key);
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
    std::string search_key(key);
    search_key += '\0';

    Node* current = root;
    u32 depth = 0;

    while (current != nullptr) {
        if (const LeafNode* leaf = as_leaf(current)) {
            return leaf->_full_key == search_key ? leaf : nullptr;
        }

        depth += current->_prefix_skip_length;
        if (depth >= search_key.size()) {
            return nullptr;
        }

        current = current->find_child(static_cast<u8>(search_key[depth]));
        ++depth;
    }

    return nullptr;
}

void insert_suffix_range(AdaptiveRadixTrieFixture& fixture, unsigned char first, unsigned char last)
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
            std::string full_key(leaf->_full_key);
            if (!full_key.empty() && full_key.back() == '\0') {
                full_key.pop_back();
            }
            keys.push_back(std::move(full_key));
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

TEST_F(AdaptiveRadixTrieFixture, InsertIntoEmptyRootCreatesLeaf)
{
    core::FileInfo* info = insert("workspace/main.cpp", 42);

    const LeafNode* leaf = as_leaf(trie.root_node());

    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(leaf->_full_key, std::string("workspace/main.cpp") + '\0');
    EXPECT_EQ(leaf->_metadata.get(), info);
}

TEST_F(AdaptiveRadixTrieFixture, SharedPrefixSplitCreatesNode4WithBothLeavesReachable)
{
    core::FileInfo* car = insert("car", 1);
    core::FileInfo* cat = insert("cat", 2);

    ASSERT_NE(trie.root_node(), nullptr);
    ASSERT_EQ(trie.root_node()->_type, NodeType::Node4);
    EXPECT_EQ(trie.root_node()->_prefix_skip_length, 2U);
    EXPECT_EQ(trie.root_node()->_child_count, 2U);

    const LeafNode* car_leaf = find_leaf(trie.root_node(), "car");
    const LeafNode* cat_leaf = find_leaf(trie.root_node(), "cat");

    ASSERT_NE(car_leaf, nullptr);
    ASSERT_NE(cat_leaf, nullptr);
    EXPECT_EQ(car_leaf->_metadata.get(), car);
    EXPECT_EQ(cat_leaf->_metadata.get(), cat);
    EXPECT_EQ(find_leaf(trie.root_node(), "cab"), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, Node4KeepsChildrenSortedByEdgeByte)
{
    insert("pd");
    insert("pb");
    insert("pc");
    insert("pa");

    ASSERT_NE(trie.root_node(), nullptr);
    ASSERT_EQ(trie.root_node()->_type, NodeType::Node4);

    const auto* node = static_cast<const Node4*>(trie.root_node());
    ASSERT_EQ(node->_child_count, 4U);
    EXPECT_EQ(node->_keys[0], static_cast<u8>('a'));
    EXPECT_EQ(node->_keys[1], static_cast<u8>('b'));
    EXPECT_EQ(node->_keys[2], static_cast<u8>('c'));
    EXPECT_EQ(node->_keys[3], static_cast<u8>('d'));
}

TEST_F(AdaptiveRadixTrieFixture, GrowsFromNode4ToNode16)
{
    insert_suffix_range(*this, 1, 5);

    ASSERT_NE(trie.root_node(), nullptr);
    EXPECT_EQ(trie.root_node()->_type, NodeType::Node16);
    EXPECT_EQ(trie.root_node()->_child_count, 5U);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(1)), nullptr);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(5)), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, GrowsFromNode16ToNode48)
{
    insert_suffix_range(*this, 1, 17);

    ASSERT_NE(trie.root_node(), nullptr);
    EXPECT_EQ(trie.root_node()->_type, NodeType::Node48);
    EXPECT_EQ(trie.root_node()->_child_count, 17U);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(1)), nullptr);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(17)), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, GrowsFromNode48ToNode256)
{
    insert_suffix_range(*this, 1, 49);

    ASSERT_NE(trie.root_node(), nullptr);
    EXPECT_EQ(trie.root_node()->_type, NodeType::Node256);
    EXPECT_EQ(trie.root_node()->_child_count, 49U);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(1)), nullptr);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(49)), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, DeleteExistingLeafRemovesOnlyThatKey)
{
    insert("pa");
    insert("pb");
    insert("pc");

    erase("pb");

    EXPECT_NE(find_leaf(trie.root_node(), "pa"), nullptr);
    EXPECT_EQ(find_leaf(trie.root_node(), "pb"), nullptr);
    EXPECT_NE(find_leaf(trie.root_node(), "pc"), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, DeletingAllKeysLeavesEmptyRoot)
{
    insert("pa");
    insert("pb");
    insert("pc");

    erase("pa");
    erase("pb");
    erase("pc");

    EXPECT_EQ(trie.root_node(), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, DeletingMissingKeyDoesNotChangeExistingKeys)
{
    insert("alpha");
    insert("alpine");

    erase("altitude");

    EXPECT_NE(find_leaf(trie.root_node(), "alpha"), nullptr);
    EXPECT_NE(find_leaf(trie.root_node(), "alpine"), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, ShrinksFromNode48ToNode16)
{
    insert_suffix_range(*this, 1, 17);

    erase(key_with_suffix(17));
    erase(key_with_suffix(16));

    ASSERT_NE(trie.root_node(), nullptr);
    EXPECT_EQ(trie.root_node()->_type, NodeType::Node16);
    EXPECT_EQ(trie.root_node()->_child_count, 15U);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(1)), nullptr);
    EXPECT_EQ(find_leaf(trie.root_node(), key_with_suffix(17)), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, ShrinksFromNode256ToNode48)
{
    insert_suffix_range(*this, 1, 49);

    erase(key_with_suffix(49));
    erase(key_with_suffix(48));

    ASSERT_NE(trie.root_node(), nullptr);
    EXPECT_EQ(trie.root_node()->_type, NodeType::Node48);
    EXPECT_EQ(trie.root_node()->_child_count, 47U);
    EXPECT_NE(find_leaf(trie.root_node(), key_with_suffix(1)), nullptr);
    EXPECT_EQ(find_leaf(trie.root_node(), key_with_suffix(49)), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, CollectsAllInsertedKeysAfterDeepSharedPrefixInserts)
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
    EXPECT_EQ(collect_keys(trie.root_node()), keys);
}

TEST_F(AdaptiveRadixTrieFixture, DuplicateInsertKeepsKeyReachableExactlyOnce)
{
    core::FileInfo* first = insert("workspace/project/file.cpp", 1);
    core::FileInfo* second = insert("workspace/project/file.cpp", 2);

    const LeafNode* leaf = find_leaf(trie.root_node(), "workspace/project/file.cpp");

    ASSERT_NE(leaf, nullptr);
    EXPECT_TRUE(leaf->_metadata.get() == first || leaf->_metadata.get() == second);
    expect_only_keys(trie.root_node(), {"workspace/project/file.cpp"});
}

TEST_F(AdaptiveRadixTrieFixture, RepeatedDuplicateInsertsDoNotDestroyNeighborKeys)
{
    insert("workspace/project/file.cpp", 1);
    insert("workspace/project/file.hpp", 2);

    for (u32 i = 0; i < 25; ++i) {
        insert("workspace/project/file.cpp", 100 + i);
    }

    expect_only_keys(trie.root_node(), {
        "workspace/project/file.cpp",
        "workspace/project/file.hpp",
    });
    expect_present(trie.root_node(), {
        "workspace/project/file.cpp",
        "workspace/project/file.hpp",
    });
}

TEST_F(AdaptiveRadixTrieFixture, InsertingShorterKeyThatIsPrefixOfExistingKeyKeepsBoth)
{
    insert("workspace/project/src/main.cpp", 1);
    insert("workspace/project/src", 2);

    expect_only_keys(trie.root_node(), {
        "workspace/project/src",
        "workspace/project/src/main.cpp",
    });
}

TEST_F(AdaptiveRadixTrieFixture, InsertingLongerKeyBelowExistingPrefixKeepsBoth)
{
    insert("workspace/project/src", 1);
    insert("workspace/project/src/main.cpp", 2);

    expect_only_keys(trie.root_node(), {
        "workspace/project/src",
        "workspace/project/src/main.cpp",
    });
}

TEST_F(AdaptiveRadixTrieFixture, PrefixFamilySurvivesMixedInsertionOrder)
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

    expect_only_keys(trie.root_node(), keys);
}

TEST_F(AdaptiveRadixTrieFixture, DeleteKeyThatIsPrefixOfAnotherRemovesOnlyExactKey)
{
    insert("workspace");
    insert("workspace/project");
    insert("workspace/project/file.cpp");

    erase("workspace");

    expect_only_keys(trie.root_node(), {
        "workspace/project",
        "workspace/project/file.cpp",
    });
    expect_absent(trie.root_node(), {"workspace"});
}

TEST_F(AdaptiveRadixTrieFixture, DeleteLongerMissingKeyBelowExistingLeafDoesNotChangeTree)
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

    expect_only_keys(trie.root_node(), keys);
}

TEST_F(AdaptiveRadixTrieFixture, DeleteShorterMissingPrefixDoesNotChangeTree)
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

    expect_only_keys(trie.root_node(), keys);
}

TEST_F(AdaptiveRadixTrieFixture, AllByteSuffixesAreReachableAfterNode256Growth)
{
    std::vector<std::string> keys;
    keys.reserve(255);

    for (u16 suffix = 1; suffix <= 255; ++suffix) {
        std::string key = key_with_suffix(static_cast<unsigned char>(suffix));
        keys.push_back(key);
        insert(key, suffix);
    }

    ASSERT_NE(trie.root_node(), nullptr);
    EXPECT_EQ(trie.root_node()->_type, NodeType::Node256);
    EXPECT_EQ(trie.root_node()->_child_count, 255U);
    expect_only_keys(trie.root_node(), keys);
}

TEST_F(AdaptiveRadixTrieFixture, DeleteEverySecondByteSuffixPreservesTheRest)
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

    expect_only_keys(trie.root_node(), remaining);
}

TEST_F(AdaptiveRadixTrieFixture, DeleteAllByteSuffixesInReverseLeavesEmptyRoot)
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

    EXPECT_EQ(trie.root_node(), nullptr);
}

TEST_F(AdaptiveRadixTrieFixture, InterleavedInsertDeleteMatchesReferenceSet)
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
    expect_only_keys(trie.root_node(), expected_keys);
}

TEST_F(AdaptiveRadixTrieFixture, DeepSharedPrefixRepeatedSplitsRemainReachable)
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

    expect_only_keys(trie.root_node(), keys);
}

TEST(AdaptiveRadixTriePrefixSearch, EmptyTrieReturnsNoPaths)
{
    AdaptiveRadixTrie trie;

    EXPECT_TRUE(trie.get_all_paths_with_prefix("workspace").empty());
}

// ============================================================================
// Substring-Suche
// ============================================================================

namespace {

std::vector<std::string> strip_null(std::vector<std::string> paths) {
    for (auto& p : paths) {
        auto pos = p.find('\0');
        if (pos != std::string::npos) p.resize(pos);
    }
    return paths;
}

} // namespace

TEST_F(AdaptiveRadixTrieFixture, SubstringEmptyTrieReturnsEmpty)
{
    EXPECT_TRUE(trie.get_all_paths_with_substring("main").empty());
}

TEST_F(AdaptiveRadixTrieFixture, SubstringNoMatchReturnsEmpty)
{
    insert("aaaa/bbbb/cccc");
    EXPECT_TRUE(trie.get_all_paths_with_substring("xyz").empty());
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesPrefix)
{
    insert("src/main.cpp");
    auto results = strip_null(trie.get_all_paths_with_substring("src"));
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0], "src/main.cpp");
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesSuffix)
{
    insert("src/main.cpp");
    auto results = strip_null(trie.get_all_paths_with_substring(".cpp"));
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0], "src/main.cpp");
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesMiddle)
{
    insert("src/main.cpp");
    auto results = strip_null(trie.get_all_paths_with_substring("main"));
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0], "src/main.cpp");
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesSingleCharacter)
{
    insert("src/main.cpp");
    auto results = strip_null(trie.get_all_paths_with_substring("i"));
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0], "src/main.cpp");
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesMultiplePaths)
{
    insert("workspace/project/src/main.cpp");
    insert("workspace/project/src/main.hpp");
    insert("workspace/project/tests/main_test.cpp");
    insert("workspace/readme.md");

    auto results = strip_null(trie.get_all_paths_with_substring("main"));
    ASSERT_EQ(results.size(), 3U);
    // Reihenfolge = Trie-Traversierungs-Reihenfolge. Ist stabil, solange collect_all_leaves so bleibt.
    EXPECT_EQ(results[0], "workspace/project/src/main.cpp");
    EXPECT_EQ(results[1], "workspace/project/src/main.hpp");
    EXPECT_EQ(results[2], "workspace/project/tests/main_test.cpp");
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesDirectoryName)
{
    insert("var/log/syslog");
    insert("var/log/auth.log");
    insert("etc/logrotate.conf");
    insert("usr/bin/logger");
    insert("home/user/scratch.txt");

    auto results = strip_null(trie.get_all_paths_with_substring("log"));
    ASSERT_EQ(results.size(), 4U);
    // "scratch.txt" enthält kein "log" -> nicht im Ergebnis
    for (const auto& r : results) {
        EXPECT_NE(r.find("log"), std::string::npos);
    }
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesMultipleOccurrencesInOnePath)
{
    insert("foo/bar/foo.baz");
    auto results = strip_null(trie.get_all_paths_with_substring("foo"));
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0], "foo/bar/foo.baz");
}

TEST_F(AdaptiveRadixTrieFixture, SubstringMatchesPathWithSpaces)
{
    insert("my documents/important file.txt");
    auto results = strip_null(trie.get_all_paths_with_substring("doc"));
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0], "my documents/important file.txt");
}

TEST_F(AdaptiveRadixTrieFixture, SubstringAcrossLargeTrie)
{
    for (u32 i = 0; i < 1000; ++i) {
        insert("module_" + std::to_string(i % 50) + "/file_" + std::to_string(i) + ".txt");
    }

    auto results = strip_null(trie.get_all_paths_with_substring("file_42"));
    // Nur exakte "file_42" matches (nicht file_420, file_421 etc.)
    for (const auto& r : results) {
        EXPECT_NE(r.find("file_42"), std::string::npos);
    }
    EXPECT_GT(results.size(), 0U);

    auto no_results = trie.get_all_paths_with_substring("nichtvorhanden");
    EXPECT_TRUE(no_results.empty());
}

TEST_F(AdaptiveRadixTrieFixture, SubstringDotCpp)
{
    insert("src/main.cpp");
    insert("src/helper.cpp");
    insert("src/main.hpp");
    insert("docs/guide.md");

    auto results = strip_null(trie.get_all_paths_with_substring(".cpp"));
    ASSERT_EQ(results.size(), 2U);
}
