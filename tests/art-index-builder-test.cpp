#include <gtest/gtest.h>

#include "../src/file-handling/ArtIndexBuilder.hpp"
#include "../src/core/file-info.hpp"
#include "../src/radix-trie/radix-trie.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct ArtIndexBuilderFixture : testing::Test {
    fs::path _orig_dir;
    fs::path _tmp_dir;

    void SetUp() override
    {
        _orig_dir = fs::current_path();
        char tmpl[] = "/tmp/art-test-XXXXXX";
        char* dir = mkdtemp(tmpl);
        ASSERT_NE(dir, nullptr) << "mkdtemp failed";
        _tmp_dir = fs::weakly_canonical(dir);
    }

    void TearDown() override
    {
        fs::current_path(_orig_dir);
        fs::remove_all(_tmp_dir);
    }

    void chdir_to_tmp()
    {
        fs::current_path(_tmp_dir);
    }

    void create_file(fs::path const& relative, std::string const& content = "")
    {
        fs::path full = _tmp_dir / relative;
        fs::create_directories(full.parent_path());
        std::ofstream out(full);
        out << content;
    }

    std::vector<std::string> collect_trie_keys(pinguqueen::datastructs::RadixTrie& trie)
    {
        pinguqueen::datastructs::Node* root = trie.root_node();
        if (root == nullptr) {
            return {};
        }

        std::vector<std::string> keys;
        std::vector<pinguqueen::datastructs::Node*> stack;
        stack.push_back(root);

        while (!stack.empty()) {
            auto* current = stack.back();
            stack.pop_back();

            if (current->is_leaf()) {
                auto* leaf = static_cast<pinguqueen::datastructs::LeafNode*>(current);
                std::string full_key(leaf->_full_key);
                if (!full_key.empty() && full_key.back() == '\0') {
                    full_key.pop_back();
                }
                keys.push_back(std::move(full_key));
                continue;
            }

            if (current->_type == pinguqueen::datastructs::NodeType::Node4) {
                auto* node = static_cast<pinguqueen::datastructs::Node4*>(current);
                for (pinguqueen::u16 i = 0; i < node->_child_count; ++i) {
                    stack.push_back(node->_children[i].get());
                }
            }
            else if (current->_type == pinguqueen::datastructs::NodeType::Node16) {
                auto* node = static_cast<pinguqueen::datastructs::Node16*>(current);
                for (pinguqueen::u16 i = 0; i < node->_child_count; ++i) {
                    stack.push_back(node->_children[i].get());
                }
            }
            else if (current->_type == pinguqueen::datastructs::NodeType::Node48) {
                auto* node = static_cast<pinguqueen::datastructs::Node48*>(current);
                for (auto const& child : node->_children) {
                    if (child != nullptr) {
                        stack.push_back(child.get());
                    }
                }
            }
            else if (current->_type == pinguqueen::datastructs::NodeType::Node256) {
                auto* node = static_cast<pinguqueen::datastructs::Node256*>(current);
                for (auto const& child : node->_children) {
                    if (child != nullptr) {
                        stack.push_back(child.get());
                    }
                }
            }
        }

        std::sort(keys.begin(), keys.end());
        return keys;
    }
};

TEST_F(ArtIndexBuilderFixture, EmptyDirectoryYieldsEmptyTrie)
{
    chdir_to_tmp();

    pinguqueen::file::ArtIndexBuilder builder;

    auto keys = collect_trie_keys(builder.debug_art());
    EXPECT_TRUE(keys.empty());
}

TEST_F(ArtIndexBuilderFixture, SingleFileIsInserted)
{
    chdir_to_tmp();
    create_file("hello.txt", "hello world");

    pinguqueen::file::ArtIndexBuilder builder;

    auto keys = collect_trie_keys(builder.debug_art());
    std::vector<std::string> expected = {"hello.txt"};
    EXPECT_EQ(keys, expected);
}

TEST_F(ArtIndexBuilderFixture, FilesInSubdirectoriesAreInserted)
{
    chdir_to_tmp();
    create_file("src/main.cpp", "int main() {}");
    create_file("src/util.hpp", "#pragma once");
    create_file("readme.md", "# Project");
    create_file("docs/api.md", "# API");

    pinguqueen::file::ArtIndexBuilder builder;

    auto keys = collect_trie_keys(builder.debug_art());
    std::vector<std::string> expected = {
        "docs/api.md",
        "readme.md",
        "src/main.cpp",
        "src/util.hpp",
    };
    EXPECT_EQ(keys, expected);
}

TEST_F(ArtIndexBuilderFixture, FileSizeIsStoredCorrectly)
{
    chdir_to_tmp();
    std::string content(123, 'A');
    create_file("data.bin", content);

    pinguqueen::file::ArtIndexBuilder builder;

    auto& trie = builder.debug_art();
    pinguqueen::core::FileInfo* info = trie.search("data.bin");
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->_file_name, "data.bin");
    EXPECT_EQ(info->_file_size_bytes, 123);
}

TEST_F(ArtIndexBuilderFixture, OnlyRegularFilesAreInserted)
{
    chdir_to_tmp();
    create_file("file.txt", "content");
    fs::create_directory(_tmp_dir / "empty_dir");
    fs::create_symlink("file.txt", _tmp_dir / "link.txt");

    pinguqueen::file::ArtIndexBuilder builder;

    auto keys = collect_trie_keys(builder.debug_art());
    std::vector<std::string> expected = {"file.txt"};
    EXPECT_EQ(keys, expected);
}

} // namespace
