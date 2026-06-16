#include "node.hpp"
#include <format>
#include <emmintrin.h>
namespace pinguqueen {

    Node4::~Node4()
    {
        for (u16 i = 0; i < _child_count; ++i) {
            delete _children[i];
        }
    }

    Node16::~Node16()
    {
        for (u16 i = 0; i < _child_count; ++i) {
            delete _children[i];
        }
    }

    Node48::~Node48()
    {
        for (auto* child : _children) {
            delete child;
        }
    }

    Node256::~Node256()
    {
        for (auto* child : _children) {
            delete child;
        }
    }



    Node* Node4::find_child(u8 key_byte) noexcept
    {
        for (u16 i = 0; i < _child_count; ++i) {
            if (_keys[i] == key_byte) return _children[i];
        }
        return nullptr;
    }

    Node* Node16::find_child(u8 key_byte) noexcept
    {
        //!! KOMPLETT CHATGPT GENERIERT, KEIN PLAN WAS DA LOS IST?!?!?! ABER ENTSPRICHT DEM PAPER ANSCHEINEND

        __m128i key_vector = _mm_set1_epi8(static_cast<char>(key_byte));
        __m128i node_keys = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_keys));
        __m128i cmp = _mm_cmpeq_epi8(key_vector, node_keys);

        u32 mask = static_cast<u32>(_mm_movemask_epi8(cmp));

        // Einfache Säuberung: Nur die Bits betrachten, die innerhalb der echten Kinder liegen
        u32 bitfield = mask & ((1U << _child_count) - 1);

        if (bitfield != 0) {
            u32 index = static_cast<u32>(__builtin_ctz(bitfield));
            return _children[index];
        }
        return nullptr;
    }



    Node* Node48::find_child(u8 key_byte) noexcept
    {
        u8 index = _keys[key_byte];
        if (index != NOTHING) { // 48 ist der "Null"-Marker
            return _children[index];
        }
        return nullptr;
    }

    Node* Node256::find_child(u8 key_byte) noexcept
    {
        return _children[key_byte];
    }



    //!DANGEROUS, NO GROW IF FULL
    void Node4::insert_pure(u8 key, Node* child) noexcept
    {
        assert(_child_count < 4);
        u8 insert_pos = 0;
        while (insert_pos < _child_count && _keys[insert_pos] < key) {
            insert_pos++;
        }
        for (u8 i = _child_count; i > insert_pos; --i) {
            _keys[i] = _keys[i - 1];
            _children[i] = _children[i - 1];
        }
        _keys[insert_pos] = key;
        _children[insert_pos] = child;
        _child_count++;
    }

    void Node4::remove_pure(u8 key, Node* child) noexcept
    {
        
    }

    //!DANGEROUS, NO GROW IF FULL
    void Node16::insert_pure(u8 key, Node* child) noexcept
    {
        assert(_child_count < 16);
        u8 insert_pos = 0;
        while (insert_pos < _child_count && _keys[insert_pos] < key) {
            insert_pos++;
        }
        for (u8 i = _child_count; i > insert_pos; --i) {
            _keys[i] = _keys[i - 1];
            _children[i] = _children[i - 1];
        }
        _keys[insert_pos] = key;
        _children[insert_pos] = child;
        _child_count++;
    }

    //!DANGEROUS, NO GROW IF FULL
    void Node48::insert_pure(u8 key, Node* child) noexcept
    {
        assert(_child_count < 48);
        u8 free_idx = 0;
        for (u8 i = 0; i < 48; ++i) {
            if (_children[i] == nullptr) {
                free_idx = i;
                break;
            }
        }
        _children[free_idx] = child;
        _keys[key] = free_idx;
        _child_count++;
    }

    //!DANGEROUS, NO GROW IF FULL
    void Node256::insert_pure(u8 key, Node* child) noexcept
    {
        assert(_children[key] == nullptr);
        _children[key] = child;
        _child_count++;
    }

}