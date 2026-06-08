
#pragma once

//Alles hier steht im Namespace von pinguqueen, da der include dieser datei in der node.hpp im abteil des Namespaces eingefügt wurde


    static inline bool is_leaf(const Node* node_ptr) noexcept {
        return (reinterpret_cast<uintptr_t>(node_ptr) & 1) == 1;
    }

    static inline Node* tag_as_leaf(const FileInfo* file_info) noexcept {
        auto address = reinterpret_cast<uintptr_t>(file_info);
        address |= 1;
        return reinterpret_cast<Node*>(address);
    }

    static inline FileInfo* get_leaf_value(const Node* node_ptr) noexcept {
        auto address = reinterpret_cast<uintptr_t>(node_ptr);
        address &= ~static_cast<uintptr_t>(1);
        return reinterpret_cast<FileInfo*>(address);
    }

