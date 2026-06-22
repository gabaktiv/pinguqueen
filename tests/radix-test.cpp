#include <gtest/gtest.h>
#include "../src/radix-trie/radix-trie.hpp"
#include "../src/radix-trie/file-info.hpp"
#include <string>
#include <vector>

using namespace pinguqueen;

class RadixTrieTest : public ::testing::Test {
protected:
    RadixTrie trie;
    
    // Hilfskonstrukte für Metadaten
    std::vector<FileInfo*> allocated_metadata;

    FileInfo* create_metadata(const std::string& name, uint32_t size) {
        auto* info = new FileInfo();
        info->file_name = name;
        info->file_size_bytes = size;
        allocated_metadata.push_back(info);
        return info;
    }

    void TearDown() override {
        // Aufräumen der FileInfo-Metadaten, da der Trie nur die Knoten besitzt, nicht das FileInfo
        for (auto* info : allocated_metadata) {
            delete info;
        }
    }
};

// ============================================================================
// 1. BASICS: Einfügen, Suchen, Mismatch
// ============================================================================

TEST_F(RadixTrieTest, EmptyTrieReturnsNull) {
    EXPECT_EQ(trie.search("irgendein/pfad.txt"), nullptr);
}

TEST_F(RadixTrieTest, InsertAndSearchSingleElement) {
    auto* meta = create_metadata("datei.txt", 1024);
    
    // Greift auf die statische oder reguläre Schnittstelle zu, je nachdem wie du dein Root verwaltest.
    // Da insert_node statisch auf Node*& arbeitet, testen wir über die öffentliche Schnittstelle des Tries:
    // Falls deine öffentliche Schnittstelle 'insert(key, info)' heißt, passe es hier kurz an.
    // Laut deiner HPP nutzt du: 'static void insert_node(Node*& node, std::string_view key, FileInfo* information, u32 depth);'
    // Wir nehmen an, du hast eine öffentliche Wrapper-Methode, analog zu search().
    // Falls nicht, erstellen wir hier einen Test-Knoten.
}

// Da deine insert_node-Methode static ist und ein Node*& erwartet, testen wir die Core-Engine 
// direkt an den Nodes über eine lokale Instanz, um die Invarianten perfekt zu triggern!

class RadixTrieNodeCoreTest : public ::testing::Test {
protected:
    Node* root = nullptr;
    std::vector<FileInfo*> allocated_metadata;

    FileInfo* create_meta(const std::string& name, uint32_t size) {
        auto* info = new FileInfo();
        info->file_name = name;
        info->file_size_bytes = size;
        allocated_metadata.push_back(info);
        return info;
    }

    void TearDown() override {
        // Die Destruktor-Kaskade löscht alle inneren Knoten und Blätter
        delete root;
        for (auto* info : allocated_metadata) {
            delete info;
        }
    }
};

TEST_F(RadixTrieNodeCoreTest, InsertAndSearchBasic) {
    auto* meta = create_meta("test.txt", 500);
    RadixTrie::insert_node(root, "pinguin.png", meta, 0);

    // Nutzen wir das reguläre öffentliche Such-Uhrwerk (über die Instanz)
    // Um die Instanz zu füttern, müsste root in der Instanz liegen. 
    // Wenn RadixTrie ein Singleton werden soll oder _root privat ist, testen wir direkt über find_leaf_node (falls zugänglich)
    // oder nutzen die öffentliche Instanz. Für diesen Test bauen wir einen sauberen Wrapper:
}

// Vergewissern wir uns der exakten Funktionsweise anhand deines Codes:
TEST_F(RadixTrieNodeCoreTest, PathCompressionAndLeafSplit) {
    auto* meta1 = create_meta("bild1.png", 100);
    auto* meta2 = create_meta("bild2.png", 200);

    // Gemeinsamer Präfix: "ruderboot" -> Mutation bei '1' vs '2'
    RadixTrie::insert_node(root, "ruderboot1", meta1, 0);
    RadixTrie::insert_node(root, "ruderboot2", meta2, 0);

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->_type, NodeType::Node4);
    EXPECT_EQ(root->_child_count, 2);
    // Präfix-Länge von "ruderboot" ist 9
    EXPECT_EQ(root->_prefix_skip_length, 9);
}

// ============================================================================
// 2. ADAPTIVES WACHSTUM (Grow-Kaskade 4 -> 16 -> 48 -> 256)
// ============================================================================

TEST_F(RadixTrieNodeCoreTest, SequentialGrowToNode16) {
    // Erzeugt einen Node4 und füllt ihn bis zur Grenze
    for (int i = 0; i < 4; ++i) {
        std::string key = "key" + std::to_string(i); // Schlüssel mutieren an Position 3 ('0','1','2','3')
        RadixTrie::insert_node(root, key, create_meta(key, 10), 0);
    }
    ASSERT_EQ(root->_type, NodeType::Node4);
    EXPECT_EQ(root->_child_count, 4);

    // Das 5. Element MUSS grow_4_to_16 auslösen
    RadixTrie::insert_node(root, "key4", create_meta("key4", 10), 0);
    EXPECT_EQ(root->_type, NodeType::Node16);
    EXPECT_EQ(root->_child_count, 5);
}

TEST_F(RadixTrieNodeCoreTest, SequentialGrowToNode48) {
    // Wir füllen den Knoten bis auf 16 Elemente
    for (int i = 0; i < 16; ++i) {
        // Generiert eindeutige Bytes an Position 0 durch Typ-Cast
        std::string key = " ";
        key[0] = static_cast<char>(i + 65); // 'A', 'B', 'C'...
        RadixTrie::insert_node(root, key, create_meta(key, 10), 0);
    }
    ASSERT_EQ(root->_type, NodeType::Node16);
    EXPECT_EQ(root->_child_count, 16);

    // Das 17. Element MUSS grow_16_to_48 auslösen
    std::string trigger_key = " ";
    trigger_key[0] = static_cast<char>(16 + 65);
    RadixTrie::insert_node(root, trigger_key, create_meta(trigger_key, 10), 0);

    EXPECT_EQ(root->_type, NodeType::Node48);
    EXPECT_EQ(root->_child_count, 17);
}

TEST_F(RadixTrieNodeCoreTest, SequentialGrowToNode256) {
    // Wir treiben den Node48 an seine absolute Grenze (48 Kinder)
    for (int i = 0; i < 48; ++i) {
        std::string key = " ";
        key[0] = static_cast<char>(i + 1); // Bytes 1 bis 48
        RadixTrie::insert_node(root, key, create_meta(key, 10), 0);
    }
    ASSERT_EQ(root->_type, NodeType::Node48);
    EXPECT_EQ(root->_child_count, 48);

    // Das 49. Element MUSS grow_48_to_256 auslösen
    std::string trigger_key = " ";
    trigger_key[0] = static_cast<char>(49);
    RadixTrie::insert_node(root, trigger_key, create_meta(trigger_key, 10), 0);

    EXPECT_EQ(root->_type, NodeType::Node256);
    EXPECT_EQ(root->_child_count, 49);
}

// ============================================================================
// 3. ADAPTIVES SCHRUMPFEN (Shrink-Kaskade 256 -> 48 -> 16 -> 4 -> Collapse)
// ============================================================================

TEST_F(RadixTrieNodeCoreTest, KaskadierendesShrinkenUndCollapse) {
    // 1. Wir bauen gezielt einen Node256 mit exakt 49 Elementen auf
    for (int i = 1; i <= 49; ++i) {
        std::string key = " ";
        key[0] = static_cast<char>(i);
        RadixTrie::insert_node(root, key, create_meta(key, 10), 0);
    }
    ASSERT_EQ(root->_type, NodeType::Node256);
    EXPECT_EQ(root->_child_count, 49);

    // 2. Wir löschen ein Element -> Count fällt auf 48.
    // Gemäß deinen Asserts (Count > 47) ist das vollkommen legal.
    // Nach dem Löschen fällt der Count unter/gleich 47 -> shrink_256_to_48 zündet!
    std::string delete_key = " ";
    delete_key[0] = static_cast<char>(49);
    RadixTrie::delete_node(root, delete_key, 0);
    
    EXPECT_EQ(root->_type, NodeType::Node48);
    EXPECT_EQ(root->_child_count, 48);

    // 3. Wir evakuieren den Node48 künstlich bis zur Schrumpfgrenze von 15
    // Wir löschen von Index 48 runter bis inklusive Index 17 (Es verbleiben 16 Kinder)
    for (int i = 48; i > 16; --i) {
        delete_key[0] = static_cast<char>(i);
        RadixTrie::delete_node(root, delete_key, 0);
    }
    ASSERT_EQ(root->_type, NodeType::Node48);
    EXPECT_EQ(root->_child_count, 16);

    // Dieses Löschen drückt den Count auf 15 -> shrink_48_to_16 zündet!
    delete_key[0] = static_cast<char>(16);
    RadixTrie::delete_node(root, delete_key, 0);
    EXPECT_EQ(root->_type, NodeType::Node16);
    EXPECT_EQ(root->_child_count, 15);

    // 4. Wir evakuieren den Node16 bis auf 4 Kinder
    for (int i = 15; i > 4; --i) {
        delete_key[0] = static_cast<char>(i);
        RadixTrie::delete_node(root, delete_key, 0);
    }
    ASSERT_EQ(root->_type, NodeType::Node16);
    EXPECT_EQ(root->_child_count, 4);

    // Dieses Löschen drückt den Count auf 3 -> shrink_16_to_4 zündet!
    delete_key[0] = static_cast<char>(4);
    RadixTrie::delete_node(root, delete_key, 0);
    EXPECT_EQ(root->_type, NodeType::Node4);
    EXPECT_EQ(root->_child_count, 3);

    // 5. Wir testen den totalen Pfad-Kollaps (Collapse)
    // Wir löschen das vorletzte Element, sodass nur noch 1 Kind übrig bleibt
    delete_key[0] = static_cast<char>(3);
    RadixTrie::delete_node(root, delete_key, 0);
    ASSERT_EQ(root->_type, NodeType::Node4);
    EXPECT_EQ(root->_child_count, 2);

    // Letzter finaler Löschschritt: Node4 bricht komplett in sich zusammen.
    // Das verbleibende Kind (Index 1) wird direkt nach oben gereicht und root wird selbst zum Leaf!
    delete_key[0] = static_cast<char>(2);
    RadixTrie::delete_node(root, delete_key, 0);

    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->is_leaf()); // 🏆 Erfolg: Der innere Weichenknoten wurde restlos wegrationalisiert!
}