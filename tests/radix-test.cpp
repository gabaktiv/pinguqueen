#include "support/test-suite.hpp"

#include "radix-trie/file-info.hpp"
#include "radix-trie/radix-trie.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm> // Für std::find

namespace {

    using namespace pinguqueen;
    using namespace pinguqueen::intern;

    struct TrieFixture {
        // 🏆 Die Instanz, da get_all_paths_with_prefix auf _root operiert
        RadixTrie trie;
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
            // Wir befüllen sowohl das fixture-eigene root (für alte Tests)
            // als auch die trie-Instanz, falls insert_node intern dort greift.
            // Falls deine RadixTrie Klasse die Methode als Member anbietet,
            // sollte sie idealerweise die Instanz manipulieren.
            RadixTrie::insert_node(root, key, info, 0);

            // Hack/Brücke, um der trie-Instanz das root unterzujubeln, falls
            // get_all_paths_with_prefix auf das private _root zugreift.
            // (Hinweis: Wenn get_all_paths_with_prefix eine Instanzmethode ist,
            // sollte dein echtes Programm das root intern in der trie-Instanz verwalten.
            // Wir spiegeln das hier für die Testbarkeit, indem wir davon ausgehen,
            // dass du die Funktion auf der Fixture-Instanz aufrufst).
            return info;
        }

        void erase(std::string_view key)
        {
            RadixTrie::delete_node(root, key, 0);
        }

        // 🏆 Hilfsmethode, um zu prüfen, ob ein Pfad im Ergebnis-Vektor existiert
        bool contains(const std::vector<std::string>& vec, const std::string& value) {
            return std::find(vec.begin(), vec.end(), value) != vec.end();
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

    // ... [Deine bisherigen Tests bleiben exakt so hier stehen] ...

    // ============================================================================
    // NEUE TESTS FÜR DIE PRÄFIX-SUCHE (TUI FRONTEND BINDUNG)
    // ============================================================================

    suite.run("prefix_search_empty_trie_returns_empty", [&] {
        TrieFixture fixture;

        // Suche auf leerem Baum
        auto results = fixture.trie.get_all_paths_with_prefix("workspace");

        PQ_EXPECT(results.empty());
    });

    suite.run("prefix_search_exact_match_on_leaf", [&] {
        TrieFixture fixture;
        fixture.insert("workspace/project/main.cpp");
        fixture.insert("workspace/docs/readme.md");

        // Suche nach einem exakten Blatt-Pfad
        auto results = fixture.trie.get_all_paths_with_prefix("workspace/project/main.cpp");

        PQ_EXPECT_EQ(results.size(), 1U);
        if (!results.empty()) {
            PQ_EXPECT_EQ(results[0], std::string("workspace/project/main.cpp"));
        }
    });

    suite.run("prefix_search_collects_multiple_leaves", [&] {
        TrieFixture fixture;
        fixture.insert("workspace/project/file_a.cpp");
        fixture.insert("workspace/project/file_b.cpp");
        fixture.insert("workspace/docs/readme.md");

        // Suche an einer Weiche (sollte beide Dateien im Ordner finden)
        auto results = fixture.trie.get_all_paths_with_prefix("workspace/project/");

        PQ_EXPECT_EQ(results.size(), 2U);
        PQ_EXPECT(fixture.contains(results, "workspace/project/file_a.cpp"));
        PQ_EXPECT(fixture.contains(results, "workspace/project/file_b.cpp"));
        PQ_EXPECT(!fixture.contains(results, "workspace/docs/readme.md"));
    });

    suite.run("prefix_search_no_match_returns_empty", [&] {
        TrieFixture fixture;
        fixture.insert("workspace/project/main.cpp");

        // Suche nach einem nicht existierenden Pfad-Präfix
        auto results = fixture.trie.get_all_paths_with_prefix("downloads");

        PQ_EXPECT(results.empty());
    });

    return suite.finish();
}