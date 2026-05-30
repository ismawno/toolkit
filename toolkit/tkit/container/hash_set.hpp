#pragma once

#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/utils/hash.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/utils/limits.hpp"

namespace TKit
{
// NOTE(Isma): This is missing some features like custom comparison/hasher and heterogeneous lookup
// NOTE(Isma): Consider implementing a SwissTable
template <typename K> struct SetNode
{
    usz Hash;
    alignas(K) std::byte Data[sizeof(K)];
    HashNodeState State = HashNode_Free;

    constexpr const K *GetKey() const
    {
        return rcast<const K *>(Data);
    }
    constexpr K *GetKey()
    {
        return rcast<K *>(Data);
    }
};

template <typename K, typename AllocState> class HashSet
{
  public:
    static constexpr ArrayType Type = AllocState::Type;

    using KeyType = K;
    using Node = SetNode<K>;

    template <typename T> class IteratorImpl
    {
      public:
        using BucketArray =
            std::conditional_t<std::is_const_v<T>, const Array<Node, AllocState>, Array<Node, AllocState>>;

        IteratorImpl(BucketArray *buckets) : m_Buckets(buckets), m_Index(0)
        {
            if (m_Buckets->At(m_Index).State != HashNode_Occupied)
                ++(*this);
        }
        IteratorImpl(BucketArray *buckets, const usize idx) : m_Buckets(buckets), m_Index(idx)
        {
        }
        IteratorImpl(const IteratorImpl<std::remove_const_t<T>> &it)
            requires(std::is_const_v<T>)
            : m_Buckets(it.m_Buckets), m_Index(it.m_Index)
        {
        }

        IteratorImpl(const IteratorImpl &other) = default;
        IteratorImpl(IteratorImpl &&other) = default;

        IteratorImpl &operator=(const IteratorImpl &other) = default;
        IteratorImpl &operator=(IteratorImpl &&other) = default;

        operator bool() const
        {
            return m_Buckets;
        }

        const K &operator*() const
        {
            return *m_Buckets->At(m_Index).GetKey();
        }

        IteratorImpl &operator++()
        {
            for (u32 i = m_Index + 1; i < m_Buckets->GetSize(); ++i)
                if (m_Buckets->At(i).State == HashNode_Occupied)
                {
                    m_Index = i;
                    return *this;
                }
            m_Index = TKIT_USIZE_MAX;
            return *this;
        }

        IteratorImpl operator++(int)
        {
            const IteratorImpl cpy = *this;
            ++(*this);
            return cpy;
        }

        friend bool operator==(const IteratorImpl &lhs, const IteratorImpl &rhs)
        {
            return lhs.m_Index == rhs.m_Index && lhs.m_Buckets == rhs.m_Buckets;
        }
        friend bool operator!=(const IteratorImpl &lhs, const IteratorImpl &rhs)
        {
            return lhs.m_Index != rhs.m_Index || lhs.m_Buckets != rhs.m_Buckets;
        }

        template <typename TT> bool operator==(const IteratorImpl<TT> &rhs)
        {
            return m_Index == rhs.m_Index && m_Buckets == rhs.m_Buckets;
        }
        template <typename TT> bool operator!=(const IteratorImpl<TT> &rhs)
        {
            return m_Index != rhs.m_Index || m_Buckets != rhs.m_Buckets;
        }

      private:
        BucketArray *m_Buckets;
        usize m_Index;

        template <typename U> friend class IteratorImpl;
        friend class HashSet;
    };

    using Iterator = IteratorImpl<K>;
    using ConstIterator = IteratorImpl<const K>;

    constexpr HashSet() = default;
    constexpr HashSet(const usize buckets) : m_Buckets(NextPowerOfTwo(buckets))
    {
        setEmpty();
    }
    constexpr HashSet(AllocState &&state) : m_Buckets(std::move(state))
    {
    }
    constexpr HashSet(const usize buckets, AllocState &&state) : m_Buckets(NextPowerOfTwo(buckets), std::move(state))
    {
        setEmpty();
    }

    template <std::input_iterator It> constexpr HashSet(const It pbegin, const It pend)
    {
        for (const It it = pbegin; it != pend; ++it)
            Insert(*it);
    }

    constexpr HashSet(const std::initializer_list<K> list)
    {
        for (const K &pair : list)
            Insert(pair);
    }

    constexpr HashSet(const HashSet &other)
    {
        copyOp(other.m_Buckets);
    }
    constexpr HashSet(HashSet &&other)
    {
        if constexpr (Type == Array_Static)
            copyOp(other.m_Buckets);
        else
        {
            m_Buckets = std::move(other.m_Buckets);
            m_Size = other.m_Size;
        }
    }

    template <typename OtherAlloc> constexpr HashSet(const HashSet<K, OtherAlloc> &other)
    {
        copyOp(other.m_Buckets);
    }
    template <typename OtherAlloc> constexpr HashSet(HashSet<K, OtherAlloc> &&other)
    {
        copyOp(other.m_Buckets);
    }

    ~HashSet()
    {
        Clear();
    }

    constexpr HashSet &operator=(const HashSet &other)
    {
        Clear();
        copyOp(other.m_Buckets);
    }
    constexpr HashSet &operator=(HashSet &&other)
    {
        Clear();
        copyOp(other.m_Buckets);
    }
    template <typename OtherAlloc> constexpr HashSet &operator=(const HashSet<K, OtherAlloc> &other)
    {
        Clear();
        copyOp(other.m_Buckets);
    }
    template <typename OtherAlloc> constexpr HashSet &operator=(HashSet<K, OtherAlloc> &&other)
    {
        Clear();
        copyOp(other.m_Buckets);
    }

    constexpr void Clear()
    {
        if (m_Size == 0)
            return;
        for (Node &n : m_Buckets)
            if (n.State == HashNode_Occupied)
            {
                if constexpr (!std::is_trivially_destructible_v<K>)
                    Destruct(n.GetKey());
                n.State = HashNode_Tombstone;
            }
        m_Size = 0;
    }

    constexpr const K *Insert(const K &key)
    {
        return insert(Hash(key), key);
    }

    template <typename... Args> constexpr K *TryInsert(const K &key)
    {
        const usz hash = Hash(key);
        const usize buckets = m_Buckets.GetSize();
        const usize idx = usize(hash & (buckets - 1));

        Node &node = m_Buckets->At(idx);
        if (node.State == HashNode_Occupied)
            return nullptr;

        ++m_Size;
        node.Hash = hash;
        node.State = HashNode_Occupied;
        return Construct(node.GetKey(), key);
    }

    constexpr ConstIterator Find(const K &key) const
    {
        return ConstIterator{&m_Buckets, find(key, Hash(key))};
    }
    constexpr Iterator Find(const K &key)
    {
        return Iterator{&m_Buckets, find(key, Hash(key))};
    }

    constexpr bool Contains(const K &key) const
    {
        return Find(key) != end();
    }

    constexpr void Remove(const Iterator iter)
    {
        TKIT_ASSERT(m_Size != 0, "[TOOLKIT][HASH-MAP] Cannot remove an element when the size is 0");
        Node &node = iter.m_Buckets->At(iter.m_Index);
        TKIT_ASSERT(node.State == HashNode_Occupied,
                    "[TOOLKIT][HASH-MAP] Iterator must point to an occupied slot to be removed");

        node.State = HashNode_Tombstone;
        Destruct(node.GetKey());
        --m_Size;
    }

    constexpr bool Remove(const K &key)
    {
        const usize idx = find(key, Hash(key));
        if (idx == TKIT_USIZE_MAX)
            return false;

        Node &node = m_Buckets[idx];
        node.State = HashNode_Tombstone;
        Destruct(node.GetKey());
        --m_Size;
        return true;
    }

    constexpr f32 GetLoadFactor() const
    {
        return f32(m_Size) / f32(m_Buckets.GetSize());
    }

    constexpr void Rehash(const usize buckets)
    {
        if (buckets > m_Buckets.GetSize())
            rehash(NextPowerOfTwo(buckets));
    }

    constexpr usize GetSize() const
    {
        return m_Size;
    }

    constexpr ConstIterator begin() const
    {
        return ConstIterator{&m_Buckets};
    }
    constexpr ConstIterator end() const
    {
        return ConstIterator{&m_Buckets, TKIT_USIZE_MAX};
    }

    constexpr Iterator begin()
    {
        return Iterator{&m_Buckets};
    }
    constexpr Iterator end()
    {
        return Iterator{&m_Buckets, TKIT_USIZE_MAX};
    }

  private:
    void setEmpty()
    {
        for (Node &n : m_Buckets)
            n.State = HashNode_Free;
    }
    usize find(const K &key, const usz hash) const
    {
        const usize buckets = m_Buckets.GetSize();
        const usize idx = usize(hash & (buckets - 1));

        usize found = TKIT_USIZE_MAX;
        const auto tryFind = [&](const usize i) {
            const Node &node = m_Buckets[i];
            if (node.State == HashNode_Free)
                return true;
            if (node.State == HashNode_Tombstone || node.Hash != hash || key != *node.GetKey())
                return false;

            found = i;
            return true;
        };

        for (u32 i = idx; i < buckets; ++i)
            if (tryFind(i))
                return found;
        for (u32 i = 0; i < idx; ++i)
            if (tryFind(i))
                return found;

        return found;
    }

    constexpr const K *insert(const usz hash, const K &key)
    {
        const usize buckets = maybeRehash();
        const usize idx = usize(hash & (buckets - 1));
        ++m_Size;
        TKIT_ASSERT(
            m_Size <= buckets,
            "[TOOLKIT][HASH-MAP] The size of the hash map ({}) exceeds the buckets of the underlying array ({})",
            m_Size, buckets);

        const auto tryInsert = [&](const usize i) -> const K * {
            Node &node = m_Buckets[i];
            if (node.State == HashNode_Occupied)
                return nullptr;

            node.Hash = hash;
            node.State = HashNode_Occupied;
            return Construct(node.GetKey(), key);
        };

        for (u32 i = idx; i < buckets; ++i)
            if (const K *elm = tryInsert(i))
                return elm;

        for (u32 i = 0; i < idx; ++i)
            if (const K *elm = tryInsert(i))
                return elm;

        TKIT_FATAL("[TOOLKIT][HASH-MAP] Failed to insert element (this should not be possible)");
        return nullptr;
    }

    constexpr usize rehash()
    {
        return rehash(m_Buckets.GetSize() * 2);
    }

    constexpr usize rehash(const usize nbuckets)
        requires(Type != Array_Arena && Type != Array_Stack)
    {
        HashSet old = std::move(*this);
        Clear();

        TKIT_ASSERT(IsPowerOfTwo(nbuckets), "[TOOLKIT][HASH-MAP] The bucket count must be a power of 2, but is {}",
                    nbuckets);
        m_Buckets.Resize(nbuckets);
        for (Node &n : old.m_Buckets)
        {
            const K *key = n.GetKey();
            insert(n.Hash, *key);
        }
        return nbuckets;
    }

    constexpr usize maybeRehash()
    {
        if constexpr (Type == Array_Arena || Type == Array_Stack)
            return m_Buckets.GetSize();
        else
        {
            if (!m_Buckets.IsEmpty() && GetLoadFactor() < TKIT_HASH_LOAD_FACTOR_THRESHOLD)
                return m_Buckets.GetSize();
            return rehash();
        }
    }

    template <typename OtherAlloc> constexpr void copyOp(const Array<Node, OtherAlloc> &buckets)
    {
        for (const Node &n : buckets)
            if (n.State == HashNode_Occupied)
            {
                const K *key = n.GetKey();
                insert(n.Hash, *key);
            }
    }

    Array<Node, AllocState> m_Buckets{};
    usize m_Size = 0;
};

template <typename K> using ArenaHashSet = HashSet<K, ArenaAllocation<SetNode<K>>>;
template <typename K> using DynamicHashSet = HashSet<K, DynamicAllocation<SetNode<K>>>;
template <typename K> using StackHashSet = HashSet<K, StackAllocation<SetNode<K>>>;
template <typename K> using TierHashSet = HashSet<K, TierAllocation<SetNode<K>>>;

template <typename K, usize Capacity> using StaticHashSet = HashSet<K, StaticAllocation<SetNode<K>, Capacity>>;
template <typename K> using StaticHashSet4 = StaticHashSet<K, 4>;
template <typename K> using StaticHashSet8 = StaticHashSet<K, 8>;
template <typename K> using StaticHashSet16 = StaticHashSet<K, 16>;
template <typename K> using StaticHashSet32 = StaticHashSet<K, 32>;
template <typename K> using StaticHashSet64 = StaticHashSet<K, 64>;
template <typename K> using StaticHashSet128 = StaticHashSet<K, 128>;
template <typename K> using StaticHashSet196 = StaticHashSet<K, 196>;
template <typename K> using StaticHashSet256 = StaticHashSet<K, 256>;
template <typename K> using StaticHashSet384 = StaticHashSet<K, 384>;
template <typename K> using StaticHashSet512 = StaticHashSet<K, 512>;
template <typename K> using StaticHashSet768 = StaticHashSet<K, 768>;
template <typename K> using StaticHashSet1024 = StaticHashSet<K, 1024>;
} // namespace TKit
