#include "radix-trie.hpp"

#include <vector>
// TODO: FEHLERBEHANDLUNG bei jeglichen asserts



void pinguqueen::RadixTrie::free_node(Node* root_node)
{
    if (root_node == nullptr) return;

    std::vector<Node*> deleteList;
    deleteList.push_back(root_node);

    while (!deleteList.empty())
    {
        Node* current = deleteList.back();
        deleteList.pop_back();

        switch (current->_type)
        {
            case NodeType::Node4:
            {
                auto* n4 = static_cast<Node4*>(current);
                for (u8 i = 0; i < n4->_child_count; ++i)
                {
                    if (n4->_children[i] != nullptr) deleteList.push_back(n4->_children[i]);
                }
                delete n4;
                break;
            }

            case NodeType::Node16:
            {
                auto* n16 = static_cast<Node16*>(current);
                for (u8 i = 0; i < n16->_child_count; ++i)
                {
                    if (n16->_children[i] != nullptr) deleteList.push_back(n16->_children[i]);
                }
                delete n16;
                break;
            }

            case NodeType::Node48:
            {
                auto* n48 = static_cast<Node48*>(current);
                for (auto* child : n48->_children) {
                    if (child != nullptr) {
                        deleteList.push_back(child);
                    }
                }
                delete n48;
                break;
            }

            case NodeType::Node256:
            {
                auto* n256 = static_cast<Node256*>(current);

                for (auto* child : n256->_children) {
                    if (child != nullptr) {
                        deleteList.push_back(child);
                    }
                }
                delete n256;
                break;
            }

            case NodeType::LeafNode:
            {
                auto* leaf = static_cast<LeafNode*>(current);
                delete leaf;
                break;
            }
        }

    }
}


void pinguqueen::RadixTrie::replace(Node*& dest, Node* src) noexcept
{
    assert(src != nullptr);
    dest = src;
}

bool pinguqueen::RadixTrie::is_leaf(const Node* node) noexcept
{
    assert(node != nullptr);
    return node->_isleaf;
}


std::string_view pinguqueen::RadixTrie::load_key(const Node* node) noexcept
{
    assert(node != nullptr && node->_type == NodeType::LeafNode);
    const auto* leaf = static_cast<const LeafNode*>(node);
    return leaf->_full_key;
}


void pinguqueen::RadixTrie::add_child(Node* parent, u8 key, Node* child)
{
    switch (parent->_type)
        {
        case NodeType::Node4:
        {
            auto* node4 = static_cast<Node4*>(parent);
            if (node4->_child_count == 4)
            {
                //TODO: grow function
            }

            //Der dümmste Insertion-Sort der Welt :)
            u8 insert_pos = 0;
            while (insert_pos < node4->_child_count && node4->_keys[insert_pos] < key) {
                insert_pos++;
            }

            for (u8 i = node4->_child_count; i > insert_pos; --i) {
                node4->_keys[i] = node4->_keys[i - 1];
                node4->_children[i] = node4->_children[i - 1];
            }

            node4->_keys[insert_pos] = key;
            node4->_children[insert_pos] = child;
            node4->_child_count++;
            break;
        }

        case NodeType::Node16:
        {
            auto* node16 = static_cast<Node16*>(parent);
            if (node16->_child_count == 16)
            {
                // TODO: Knoten ist voll! Hier zu Node48 wachsen lassen (grow)
            }

            // Keys sortiert einfügen(Gleiches Prinzip wie bei Node4)
            u8 insert_pos = 0;
            while (insert_pos < node16->_child_count && node16->_keys[insert_pos] < key) {
                insert_pos++;
            }

            for (u8 i = node16->_child_count; i > insert_pos; --i) {
                node16->_keys[i] = node16->_keys[i - 1];
                node16->_children[i] = node16->_children[i - 1];
            }

            node16->_keys[insert_pos] = key;
            node16->_children[insert_pos] = child;
            node16->_child_count++;
            break;
        }
        case NodeType::Node48 :
        {
            auto* node48 = static_cast<Node48*>(parent);
            u8 index = 0;


            //Wir suchen den ersten freien Index, Elemente könnten gelöscht worden sein. Bei den zuvorigen muss geachtet werden, dass die Arrays nach einem Löschprozess nach links verschoben werden
            for (u8 i = 0; i < 48; ++i) {
                if (node48->_children[i] == nullptr) {
                    index = i;
                    break;
                }
            }

            node48->_children[index] = child;
            node48->_keys[key] = index;

            node48->_child_count++;
            break;
        }

        case NodeType::Node256 :
        {
            auto* node256 = static_cast<Node256*>(parent);
            node256->_children[key] = child;
            node256->_child_count++;
        }
            break;
        case NodeType::LeafNode:
            break;
    }

}




void pinguqueen::RadixTrie::insert(Node*& node, std::string_view key, FileInfo* information, u32 depth)
{
    auto* leaf = new LeafNode();
    leaf->_type = NodeType::LeafNode;
    leaf->_full_key = std::string(key);
    leaf->_metadata = information;

    if (node==nullptr)
    {
        replace(node, leaf);
        return;
    }
    if (is_leaf(node))
    {
        auto newNode = new Node4();
        std::string_view key2 = load_key(node);
        u32 i = depth;
        while (i < key.length() && i < key2.length() && key[i] == key2[i])
        {
            i++;
        }
        newNode->_prefix_skip_length = i - depth;
        depth = depth + newNode->_prefix_skip_length;
        add_child(newNode, key[depth], leaf);
        add_child(newNode, key2[depth], node);
        replace(node, newNode);



    /*
2 replace(node, leaf)
3 return
4 if isLeaf(node) // expand node
5 newNode=makeNode4()
6 key2=loadKey(node)
7 for (i=depth; key[i]==key2[i]; i=i+1)
8 newNode.prefix[i-depth]=key[i]
9 newNode.prefixLen=i-depth
10 depth=depth+newNode.prefixLen
11 addChild(newNode, key[depth], leaf)
12 addChild(newNode, key2[depth], node)
13 replace(node, newNode)
14 return
15 p=checkPrefix(node, key, depth)
16 if p!=node.prefixLen // prefix mismatch
17 newNode=makeNode4()
18 addChild(newNode, key[depth+p], leaf)
19 addChild(newNode, node.prefix[p], node)
20 newNode.prefixLen=p
21 memcpy(newNode.prefix, node.prefix, p)
22 node.prefixLen=node.prefixLen-(p+1)
23 memmove(node.prefix,node.prefix+p+1,node.prefixLen)
24 replace(node, newNode)
25 return
26 depth=depth+node.prefixLen
27 next=findChild(node, key[depth])
28 if next // recurse
29 insert(next, key, leaf, depth+1)
30 else // add to inner node
31 if isFull(node)
32 grow(node)
33 addChild(node, key[depth], leaf)
     */
}



