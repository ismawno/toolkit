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
template <typename K, typename V> struct KeyValuePair
{
    template <typename... Args>
    KeyValuePair(const K &key, Args &&...args) : Key(key), Value(std::forward<Args>(args)...)
    {
    }
    K Key;
    V Value;
};

// NOTE(Isma): This is missing some features like custom comparison/hasher and heterogeneous lookup
// NOTE(Isma): Consider implementing a SwissTable
template <typename K, typename V> struct MapNode
{
    using Entry = KeyValuePair<const K, V>;

    usz Hash;
    alignas(Entry) std::byte Data[sizeof(Entry)];
    HashNodeState State = HashNode_Free;

    constexpr const Entry *GetEntry() const
    {
        return rcast<const Entry *>(Data);
    }
    constexpr Entry *GetEntry()
    {
        return rcast<Entry *>(Data);
    }
};

template <typename K, typename V, typename AllocState> class HashMap
{
  public:
    static constexpr ArrayType Type = AllocState::Type;
    using KeyType = K;
    using ValueType = V;

    using Node = MapNode<K, V>;
    using Pair = KeyValuePair<K, V>;
    using Entry = typename Node::Entry;

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

        auto &operator*() const
        {
            return *m_Buckets->At(m_Index).GetEntry();
        }
        auto *operator->() const
        {
            return m_Buckets->At(m_Index).GetEntry();
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
        friend class HashMap;
    };

    using Iterator = IteratorImpl<Entry>;
    using ConstIterator = IteratorImpl<const Entry>;

    constexpr HashMap() = default;
    constexpr HashMap(const usize buckets) : m_Buckets(NextPowerOfTwo(buckets))
    {
        setEmpty();
    }
    constexpr HashMap(AllocState &&state) : m_Buckets(std::move(state))
    {
    }
    constexpr HashMap(const usize capacity, AllocState &&state) : m_Buckets(NextPowerOfTwo(capacity), std::move(state))
    {
        setEmpty();
    }

    template <std::input_iterator It> constexpr HashMap(const It pbegin, const It pend)
    {
        for (const It it = pbegin; it != pend; ++it)
            Insert(*it);
    }

    constexpr HashMap(const std::initializer_list<Pair> list)
    {
        for (const Pair &pair : list)
            Insert(pair);
    }

    constexpr HashMap(const HashMap &other)
    {
        copyOp(other.m_Buckets);
    }
    constexpr HashMap(HashMap &&other)
    {
        if constexpr (Type == Array_Static)
            moveOp(other.m_Buckets);
        else
        {
            m_Buckets = std::move(other.m_Buckets);
            m_Size = other.m_Size;
        }
    }

    template <typename OtherAlloc> constexpr HashMap(const HashMap<K, V, OtherAlloc> &other)
    {
        copyOp(other.m_Buckets);
    }
    template <typename OtherAlloc> constexpr HashMap(HashMap<K, V, OtherAlloc> &&other)
    {
        moveOp(other.m_Buckets);
    }

    ~HashMap()
    {
        Clear();
    }

    constexpr HashMap &operator=(const HashMap &other)
    {
        Clear();
        copyOp(other.m_Buckets);
    }
    constexpr HashMap &operator=(HashMap &&other)
    {
        Clear();
        moveOp(other.m_Buckets);
    }
    template <typename OtherAlloc> constexpr HashMap &operator=(const HashMap<K, V, OtherAlloc> &other)
    {
        Clear();
        copyOp(other.m_Buckets);
    }
    template <typename OtherAlloc> constexpr HashMap &operator=(HashMap<K, V, OtherAlloc> &&other)
    {
        Clear();
        moveOp(other.m_Buckets);
    }

    constexpr void Clear()
    {
        if (m_Size == 0)
            return;
        for (Node &n : m_Buckets)
            if (n.State == HashNode_Occupied)
            {
                if constexpr (!std::is_trivially_destructible_v<K> || !std::is_trivially_destructible_v<V>)
                    Destruct(n.GetEntry());
                n.State = HashNode_Tombstone;
            }
        m_Size = 0;
    }

    template <typename... Args>
        requires std::constructible_from<V, Args...>
    constexpr V &Insert(const K &key, Args &&...args)
    {
        return *insert(Hash(key), key, std::forward<Args>(args)...);
    }
    constexpr V &Insert(const Pair &pair)
    {
        return *Insert(pair.Key, pair.Value);
    }

    template <typename... Args>
        requires std::constructible_from<V, Args...>
    constexpr V &TryInsert(const K &key, Args &&...args)
    {
        const usz hash = Hash(key);
        const usize idx = find<true>(key, hash);

        Node &node = m_Buckets[idx];
        if (node.State == HashNode_Occupied)
            return node.GetEntry()->Value;

        ++m_Size;
        node.Hash = hash;
        node.State = HashNode_Occupied;
        return Construct(node.GetEntry(), key, std::forward<Args>(args)...)->Value;
    }
    constexpr V &TryInsert(const Pair &pair)
    {
        return TryInsert(pair.Key, pair.Value);
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

    constexpr const V &At(const K &key) const
    {
        const auto it = Find(key);
        TKIT_ASSERT(it != end(), "[TOOLKIT][HASH-MAP] The key was not found");
        return it->Value;
    }
    constexpr V &At(const K &key)
    {
        const auto it = Find(key);
        TKIT_ASSERT(it != end(), "[TOOLKIT][HASH-MAP] The key was not found");
        return it->Value;
    }

    constexpr const V &operator[](const K &key) const
    {
        return At(key);
    }
    constexpr V &operator[](const K &key)
    {
        return TryInsert(key);
    }

    constexpr void Remove(const Iterator iter)
    {
        TKIT_ASSERT(m_Size != 0, "[TOOLKIT][HASH-MAP] Cannot remove an element when the size is 0");
        Node &node = iter.m_Buckets->At(iter.m_Index);
        TKIT_ASSERT(node.State == HashNode_Occupied,
                    "[TOOLKIT][HASH-MAP] Iterator must point to an occupied slot to be removed");

        node.State = HashNode_Tombstone;
        Destruct(node.GetEntry());
        --m_Size;
    }

    constexpr bool Remove(const K &key)
    {
        const usize idx = find(key, Hash(key));
        if (idx == TKIT_USIZE_MAX)
            return false;

        Node &node = m_Buckets[idx];
        node.State = HashNode_Tombstone;
        Destruct(node.GetEntry());
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
    template <bool GetFreeSpot = false> usize find(const K &key, const usz hash) const
    {
        const usize buckets = m_Buckets.GetSize();
        const usize idx = usize(hash & (buckets - 1));

        usize found = TKIT_USIZE_MAX;
        const auto tryFind = [&](const usize i) {
            const Node &node = m_Buckets[i];
            if (node.State == HashNode_Free)
            {
                if constexpr (GetFreeSpot)
                    found = i;
                return true;
            }
            if (node.State == HashNode_Tombstone || node.Hash != hash || key != node.GetEntry()->Key)
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

    template <typename... Args>
        requires std::constructible_from<V, Args...>
    constexpr V *insert(const usz hash, const K &key, Args &&...args)
    {
        const usize buckets = maybeRehash();
        const usize idx = usize(hash & (buckets - 1));
        ++m_Size;
        TKIT_ASSERT(m_Size <= buckets,
                    "[TOOLKIT][HASH-MAP] The size of the hash map ({}) exceeds the bucket count ({})", m_Size, buckets);

        const auto tryInsert = [&](const usize i) -> V * {
            Node &node = m_Buckets[i];
            if (node.State == HashNode_Occupied)
                return nullptr;

            node.Hash = hash;
            node.State = HashNode_Occupied;
            return &Construct(node.GetEntry(), key, std::forward<Args>(args)...)->Value;
        };

        for (u32 i = idx; i < buckets; ++i)
            if (V *elm = tryInsert(i))
                return elm;

        for (u32 i = 0; i < idx; ++i)
            if (V *elm = tryInsert(i))
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
        HashMap old = std::move(*this);
        Clear();

        TKIT_ASSERT(IsPowerOfTwo(nbuckets), "[TOOLKIT][HASH-MAP] The bucket count must be a power of 2, but is {}",
                    nbuckets);
        m_Buckets.Resize(nbuckets);
        for (Node &n : old.m_Buckets)
        {
            const Entry *e = n.GetEntry();
            insert(n.Hash, e->Key, std::move(e->Value));
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
                const Entry *entry = n.GetEntry();
                insert(n.Hash, entry->Key, entry->Value);
            }
    }
    template <typename OtherAlloc> constexpr void moveOp(Array<Node, OtherAlloc> &buckets)
    {
        for (Node &n : buckets)
            if (n.State == HashNode_Occupied)
            {
                Entry *entry = n.GetEntry();
                insert(n.Hash, entry->Key, std::move(entry->Value));
            }
    }

    Array<Node, AllocState> m_Buckets{};
    usize m_Size = 0;
};

template <typename K, typename V> using ArenaHashMap = HashMap<K, V, ArenaAllocation<MapNode<K, V>>>;
template <typename K, typename V> using DynamicHashMap = HashMap<K, V, DynamicAllocation<MapNode<K, V>>>;
template <typename K, typename V> using StackHashMap = HashMap<K, V, StackAllocation<MapNode<K, V>>>;
template <typename K, typename V> using TierHashMap = HashMap<K, V, TierAllocation<MapNode<K, V>>>;

template <typename K, typename V, usize Capacity>
using StaticHashMap = HashMap<K, V, StaticAllocation<MapNode<K, V>, Capacity>>;
template <typename K, typename V> using StaticHashMap4 = StaticHashMap<K, V, 4>;
template <typename K, typename V> using StaticHashMap8 = StaticHashMap<K, V, 8>;
template <typename K, typename V> using StaticHashMap16 = StaticHashMap<K, V, 16>;
template <typename K, typename V> using StaticHashMap32 = StaticHashMap<K, V, 32>;
template <typename K, typename V> using StaticHashMap64 = StaticHashMap<K, V, 64>;
template <typename K, typename V> using StaticHashMap128 = StaticHashMap<K, V, 128>;
template <typename K, typename V> using StaticHashMap196 = StaticHashMap<K, V, 196>;
template <typename K, typename V> using StaticHashMap256 = StaticHashMap<K, V, 256>;
template <typename K, typename V> using StaticHashMap384 = StaticHashMap<K, V, 384>;
template <typename K, typename V> using StaticHashMap512 = StaticHashMap<K, V, 512>;
template <typename K, typename V> using StaticHashMap768 = StaticHashMap<K, V, 768>;
template <typename K, typename V> using StaticHashMap1024 = StaticHashMap<K, V, 1024>;
} // namespace TKit
