#include "node.hpp"

#include <algorithm>
#include <format>
#include <emmintrin.h>
#include <limits>
#include <utility>
namespace pinguqueen::datastructs {

    Node48::Node48()
    {
        std::fill(_keys, _keys + 256, NON_EXISTING_EDGE);
    }

    Node* Node4::find_child(u8 keyByte) noexcept
    {
        auto* slot = Node4::find_child_slot(keyByte);
        return slot == nullptr ? nullptr : slot->get();
    }

    // Linear search through the sorted key array to find the matching child slot.
    std::unique_ptr<Node>* Node4::find_child_slot(u8 keyByte) noexcept
    {
        for (u16 i = 0; i < _child_count; ++i) {
            if (_keys[i] == keyByte) return &_children[i];
        }
        return nullptr;
    }

    Node* Node16::find_child(u8 keyByte) noexcept
    {
        auto* slot = Node16::find_child_slot(keyByte);
        return slot == nullptr ? nullptr : slot->get();
    }
//TODO: Implement binary search alternative
    // SSE2 parallel comparison of all 16 keys against the search byte for constant-time lookup.
    // 1. Broadcast the search byte into all 16 lanes of a 128-bit register
    // 2. Load the 16 sorted keys from the node into another register
    // 3. Compare all 16 keys in parallel, producing a bitmask of matches
    // 4. Extract the position of the first match (CTZ = count trailing zeros)
    std::unique_ptr<Node>* Node16::find_child_slot(u8 keyByte) noexcept
    {
        __m128i key_vector = _mm_set1_epi8(static_cast<char>(keyByte));       // broadcast search byte
        __m128i node_keys = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_keys)); // load 16 keys
        __m128i cmp = _mm_cmpeq_epi8(key_vector, node_keys);                 // compare all 16
        u32 mask = static_cast<u32>(_mm_movemask_epi8(cmp));                 // bitmask of matches

        if (mask != 0) {
            u32 index = static_cast<u32>(__builtin_ctz(mask));               // position of first match

            if (index < _child_count) {                                       // only valid if within child count
                return &_children[index];
            }
        }
        return nullptr;
    }



    Node* Node48::find_child(u8 keyByte) noexcept
    {
        auto* slot = Node48::find_child_slot(keyByte);
        return slot == nullptr ? nullptr : slot->get();
    }

    // Direct hash lookup: key byte indexes into the key array to find the child index.
    std::unique_ptr<Node>* Node48::find_child_slot(u8 keyByte) noexcept
    {
        u8 index = _keys[keyByte];
        if (index != NON_EXISTING_EDGE && _children[index] != nullptr) {
            return &_children[index];
        }
        return nullptr;
    }

    Node* Node256::find_child(u8 keyByte) noexcept
    {
        auto* slot = Node256::find_child_slot(keyByte);
        return slot == nullptr ? nullptr : slot->get();
    }

    // Direct array access: key byte is the child index.
    std::unique_ptr<Node>* Node256::find_child_slot(u8 keyByte) noexcept
    {
        if (_children[keyByte] == nullptr) {
            return nullptr;
        }
        return &_children[keyByte];
    }

    //!DANGEROUS, NO GROW IF FULL
    //Insertion Sort
    void Node4::insert_unchecked(u8 keyByte, std::unique_ptr<Node> childNode) noexcept
    {
        assert(_child_count < 4);
        u8 insertPos = 0;
        while (insertPos < _child_count && _keys[insertPos] < keyByte) {
            ++insertPos;
        }
        for (u16 i = _child_count; i > insertPos; --i) {
            _keys[i] = _keys[i - 1];
            _children[i] = std::move(_children[i - 1]);
        }
        _keys[insertPos] = keyByte;
        _children[insertPos] = std::move(childNode);
        ++_child_count;
    }

    //!DANGEROUS, CRASHES IF NO CHILD AVAILABLE. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node4::remove_unchecked(u8 keyByte) noexcept
    {
        assert(_child_count > NO_CHILD);

        u8 removePos = 0;
        while (removePos < _child_count && _keys[removePos] < keyByte) {
            ++removePos;
        }
        if (removePos < _child_count && _keys[removePos] == keyByte) {
            assert(_children[removePos] == nullptr);

            for (u16 i = removePos; i < _child_count - 1; ++i) {
                _keys[i] = _keys[i + 1];
                _children[i] = std::move(_children[i + 1]);
            }
            _keys[_child_count - 1] = 0;
            _children[_child_count - 1].reset();
            --_child_count;
        }
    }

    //!DANGEROUS, NO GROW IF FULL
    void Node16::insert_unchecked(u8 keyByte, std::unique_ptr<Node> childNode) noexcept
    {
        assert(_child_count < 16);
        u8 insertPos = 0;
        while (insertPos < _child_count && _keys[insertPos] < keyByte) {
            ++insertPos;
        }
        for (u16 i = _child_count; i > insertPos; --i) {
            _keys[i] = _keys[i - 1];
            _children[i] = std::move(_children[i - 1]);
        }
        _keys[insertPos] = keyByte;
        _children[insertPos] = std::move(childNode);
        ++_child_count;
    }


    //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node16::remove_unchecked(u8 keyByte) noexcept
    {
        assert(_child_count > SHRINKING_CHILD_COUNT);

        u8 removePos = 0;
        while (removePos < _child_count && _keys[removePos] < keyByte) {
            ++removePos;
        }
        if (removePos < _child_count && _keys[removePos] == keyByte) {
            assert(_children[removePos] == nullptr);

            for (u16 i = removePos; i < _child_count - 1; ++i) {
                _keys[i] = _keys[i + 1];
                _children[i] = std::move(_children[i + 1]);
            }
            _keys[_child_count - 1] = 0;
            _children[_child_count - 1].reset();
            --_child_count;
        }
    }



    //!DANGEROUS, NO GROW IF FULL
    void Node48::insert_unchecked(u8 keyByte, std::unique_ptr<Node> childNode) noexcept
    {
        assert(_child_count < GROW_CHILD_COUNT);
        u8 freeIdx = 0;
        for (u8 i = 0; i < 48; ++i) {
            if (_children[i] == nullptr) {
                freeIdx = i;
                break;
            }
        }
        _children[freeIdx] = std::move(childNode);
        _keys[keyByte] = freeIdx;
        ++_child_count;
    }

    //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node48::remove_unchecked(u8 keyByte) noexcept
    {
        assert(_child_count > SHRINKING_CHILD_COUNT);

        u8 childIdx = _keys[keyByte];
        if (childIdx != Node48::NON_EXISTING_EDGE) {
            assert(_children[childIdx] == nullptr);
            _children[childIdx].reset();
            _keys[keyByte] = Node48::NON_EXISTING_EDGE;
            --_child_count;
        }

    }


    //!DANGEROUS, NO GROW IF FULL
    void Node256::insert_unchecked(u8 keyByte, std::unique_ptr<Node> childNode) noexcept
    {
        assert(_children[keyByte] == nullptr);
        _children[keyByte] = std::move(childNode);
        ++_child_count;
    }

    //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node256::remove_unchecked(u8 keyByte) noexcept
    {
        assert(_child_count > SHRINKING_CHILD_COUNT);
        assert(_children[keyByte] == nullptr);
        _children[keyByte].reset();
        --_child_count;
    }
    
}
