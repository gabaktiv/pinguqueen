#include "node.hpp"
#include <algorithm>
#include <format>
#include <emmintrin.h>
#include <utility>
namespace pinguqueen::intern {

    Node48::Node48()
    {
        std::fill(_keys, _keys + 256, NOTHING);
    }

    Node* Node4::find_child(u8 key_byte) noexcept
    {
        for (u16 i = 0; i < _child_count; ++i) {
            if (_keys[i] == key_byte) return _children[i].get();
        }
        return nullptr;
    }

    std::unique_ptr<Node>* Node4::find_child_slot(u8 key_byte) noexcept
    {
        for (u16 i = 0; i < _child_count; ++i) {
            if (_keys[i] == key_byte) return &_children[i];
        }
        return nullptr;
    }

    Node* Node16::find_child(u8 key_byte) noexcept
    {

        __m128i key_vector = _mm_set1_epi8(static_cast<char>(key_byte));
        __m128i node_keys = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_keys));
        __m128i cmp = _mm_cmpeq_epi8(key_vector, node_keys);
        u32 mask = static_cast<u32>(_mm_movemask_epi8(cmp));

        if (mask != 0) {

            u32 index = static_cast<u32>(__builtin_ctz(mask));

            if (index < _child_count) {
                return _children[index].get();
            }
        }
        return nullptr;
    }

    std::unique_ptr<Node>* Node16::find_child_slot(u8 key_byte) noexcept
    {
        __m128i key_vector = _mm_set1_epi8(static_cast<char>(key_byte));
        __m128i node_keys = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_keys));
        __m128i cmp = _mm_cmpeq_epi8(key_vector, node_keys);
        u32 mask = static_cast<u32>(_mm_movemask_epi8(cmp));

        if (mask != 0) {
            u32 index = static_cast<u32>(__builtin_ctz(mask));

            if (index < _child_count) {
                return &_children[index];
            }
        }
        return nullptr;
    }



    Node* Node48::find_child(u8 key_byte) noexcept
    {
        u8 index = _keys[key_byte];
        if (index != NOTHING) { // 48 ist der "Null"-Marker
            return _children[index].get();
        }
        return nullptr;
    }

    std::unique_ptr<Node>* Node48::find_child_slot(u8 key_byte) noexcept
    {
        u8 index = _keys[key_byte];
        if (index != NOTHING && _children[index] != nullptr) {
            return &_children[index];
        }
        return nullptr;
    }

    Node* Node256::find_child(u8 key_byte) noexcept
    {
        return _children[key_byte].get();
    }

    std::unique_ptr<Node>* Node256::find_child_slot(u8 key_byte) noexcept
    {
        if (_children[key_byte] == nullptr) {
            return nullptr;
        }
        return &_children[key_byte];
    }

    //!DANGEROUS, NO GROW IF FULL
    //Insertion Sort
    void Node4::insert_pure(u8 key, std::unique_ptr<Node> child) noexcept
    {
        assert(_child_count < 4);
        u8 insert_pos = 0;
        while (insert_pos < _child_count && _keys[insert_pos] < key) {
            ++insert_pos;
        }
        for (u16 i = _child_count; i > insert_pos; --i) {
            _keys[i] = _keys[i - 1];
            _children[i] = std::move(_children[i - 1]);
        }
        _keys[insert_pos] = key;
        _children[insert_pos] = std::move(child);
        ++_child_count;
    }

    //!DANGEROUS, CRASHES IF NO CHILD AVAILABLE. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node4::remove_pure(u8 key) noexcept
    {
        assert(_child_count > NO_CHILD);

        u8 remove_pos = 0;
        while (remove_pos < _child_count && _keys[remove_pos] < key) {
            ++remove_pos;
        }
        if (remove_pos < _child_count && _keys[remove_pos] == key) {
            assert(_children[remove_pos] == nullptr);

            for (u16 i = remove_pos; i < _child_count - 1; ++i) {
                _keys[i] = _keys[i + 1];
                _children[i] = std::move(_children[i + 1]);
            }
            _keys[_child_count - 1] = 0;
            _children[_child_count - 1].reset();
            --_child_count;
        }
    }

    //!DANGEROUS, NO GROW IF FULL
    void Node16::insert_pure(u8 key, std::unique_ptr<Node> child) noexcept
    {
        assert(_child_count < 16);
        u8 insert_pos = 0;
        while (insert_pos < _child_count && _keys[insert_pos] < key) {
            ++insert_pos;
        }
        for (u16 i = _child_count; i > insert_pos; --i) {
            _keys[i] = _keys[i - 1];
            _children[i] = std::move(_children[i - 1]);
        }
        _keys[insert_pos] = key;
        _children[insert_pos] = std::move(child);
        ++_child_count;
    }


    //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node16::remove_pure(u8 key) noexcept
    {
        assert(_child_count > SHRINKING_CHILD_COUNT);

        u8 remove_pos = 0;
        while (remove_pos < _child_count && _keys[remove_pos] < key) {
            ++remove_pos;
        }
        if (remove_pos < _child_count && _keys[remove_pos] == key) {
            assert(_children[remove_pos] == nullptr);

            for (u16 i = remove_pos; i < _child_count - 1; ++i) {
                _keys[i] = _keys[i + 1];
                _children[i] = std::move(_children[i + 1]);
            }
            _keys[_child_count - 1] = 0;
            _children[_child_count - 1].reset();
            --_child_count;
        }
    }



    //!DANGEROUS, NO GROW IF FULL
    void Node48::insert_pure(u8 key, std::unique_ptr<Node> child) noexcept
    {
        assert(_child_count < GROW_CHILD_COUNT);
        u8 free_idx = 0;
        for (u8 i = 0; i < 48; ++i) {
            if (_children[i] == nullptr) {
                free_idx = i;
                break;
            }
        }
        _children[free_idx] = std::move(child);
        _keys[key] = free_idx;
        ++_child_count;
    }

    //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node48::remove_pure(u8 key) noexcept
    {
        assert(_child_count > SHRINKING_CHILD_COUNT);

        u8 child_idx = _keys[key];
        if (child_idx != Node48::NOTHING) {
            assert(_children[child_idx] == nullptr);
            _children[child_idx].reset();
            _keys[key] = Node48::NOTHING;
            --_child_count;
        }

    }


    //!DANGEROUS, NO GROW IF FULL
    void Node256::insert_pure(u8 key, std::unique_ptr<Node> child) noexcept
    {
        assert(_children[key] == nullptr);
        _children[key] = std::move(child);
        ++_child_count;
    }

    //!DANGEROUS, NO SHRINKING IF TOO EMPTY. EXPECTS THE CHILD SLOT TO BE EMPTY ALREADY.
    void Node256::remove_pure(u8 key) noexcept
    {
        assert(_child_count > SHRINKING_CHILD_COUNT);
        assert(_children[key] == nullptr);
        _children[key].reset();
        --_child_count;
    }
    
}
