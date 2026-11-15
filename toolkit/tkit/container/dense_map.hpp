#pragma once

#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/math/math.hpp"

namespace TKit
{
template <usize KeyCount> using DenseMapKey = u<Math::Max(usize(NextPowerOfTwo(std::countr_zero(KeyCount))), usize(8))>;
template <usize KeyCount> using DenseMapIndex = DenseMapKey<KeyCount + 1>;

template <typename T, usize KeyCount, typename AllocState> class DenseMap
{
  public:
    using Key = DenseMapKey<KeyCount>;
    using Index = DenseMapIndex<KeyCount>;
    using KeyAllocState = AllocState::template Rebind<Key>;

    static constexpr ArrayType Type = AllocState::Type;
    static constexpr Index Null = TKit::Limits<Index>::Max();

    constexpr DenseMap()
    {
        for (Index &i : m_Sparse)
            i = Null;
    }
    constexpr DenseMap(const usize capacity)
        requires(Type != Array_Static)
        : DenseMap()
    {
        m_Keys.Reserve(capacity);
        m_Values.Reserve(capacity);
    }

    constexpr DenseMap(const DenseMap &) = default;
    constexpr DenseMap &operator=(const DenseMap &) = default;

    constexpr DenseMap(DenseMap &&other)
        : m_Sparse(other.m_Sparse), m_Keys(std::move(other.m_Keys)), m_Values(std::move(other.m_Values))
    {
        releaseFrom(other);
    }
    constexpr DenseMap &operator=(DenseMap &&other)
    {
        m_Sparse = other.m_Sparse;
        m_Keys = std::move(other.m_Keys);
        m_Values = std::move(other.m_Values);
        releaseFrom(other);
        return *this;
    }

    template <typename OtherAlloc>
    constexpr DenseMap(const DenseMap<T, KeyCount, OtherAlloc> &other)
        : m_Sparse(other.m_Sparse), m_Keys(other.m_Keys), m_Values(other.m_Values)
    {
    }
    template <typename OtherAlloc>
    constexpr DenseMap(DenseMap<T, KeyCount, OtherAlloc> &&other)
        : m_Sparse(other.m_Sparse), m_Keys(std::move(other.m_Keys)), m_Values(std::move(other.m_Values))
    {
        releaseFrom(other);
    }

    template <typename OtherAlloc> constexpr DenseMap &operator=(const DenseMap<T, KeyCount, OtherAlloc> &other)
    {
        m_Sparse = other.m_Sparse;
        m_Keys = other.m_Keys;
        m_Values = other.m_Values;
        return *this;
    }
    template <typename OtherAlloc> constexpr DenseMap &operator=(DenseMap<T, KeyCount, OtherAlloc> &&other)
    {
        m_Sparse = other.m_Sparse;
        m_Keys = std::move(other.m_Keys);
        m_Values = std::move(other.m_Values);
        releaseFrom(other);
        return *this;
    }

    constexpr bool Contains(const Key k) const
    {
        return k < m_Sparse.GetSize() && m_Sparse[k] != Null;
    }

#define CHECK_KEY_WITHIN_BOUNDS(k)                                                                                     \
    TKIT_ASSERT((k) < m_Sparse.GetSize(),                                                                              \
                "[TOOLKIT][DENSE-MAP] The key {} exceeds sparse array size ({}) derived from the key count of {} "     \
                "specified at compile time",                                                                           \
                k, m_Sparse.GetSize(), KeyCount)
#define CHECK_INDEX_WITHIN_BOUNDS(idx)                                                                                 \
    TKIT_ASSERT((idx) < GetSize(),                                                                                     \
                "[TOOLKIT][DENSE-MAP] The key {} is contained in the map, but it is not stored in the dense "          \
                "array. This datastructure has been corrupted",                                                        \
                k);

#define CHECK_KEY_EXISTS(k)                                                                                            \
    CHECK_KEY_WITHIN_BOUNDS(k);                                                                                        \
    TKIT_ASSERT(m_Sparse[k] != Null, "[TOOLKIT][DENSE-MAP] The key {} is not contained in the map", k);                \
    CHECK_INDEX_WITHIN_BOUNDS(m_Sparse[k])

    constexpr const T &At(const Key k) const
    {
        CHECK_KEY_EXISTS(k);
        return m_Values[m_Sparse[k]];
    }
    constexpr T &At(const Key k)
    {
        CHECK_KEY_EXISTS(k);
        return m_Values[m_Sparse[k]];
    }
    constexpr const T &operator[](const Key k) const
    {
        return At(k);
    }
    constexpr T &operator[](const Key k)
    {
        return At(k);
    }

    constexpr void Clear()
    {
        for (Key &k : m_Keys)
            m_Sparse[k] = Null;
        m_Keys.Clear();
        m_Values.Clear();
    }

    constexpr const T *GetData() const
    {
        return m_Values.GetData();
    }
    constexpr T *GetData()
    {
        return m_Values.GetData();
    }

    constexpr const T *begin() const
    {
        return m_Values.begin();
    }
    constexpr const T *end() const
    {
        return m_Values.end();
    }

    constexpr T *begin()
    {
        return m_Values.begin();
    }
    constexpr T *end()
    {
        return m_Values.end();
    }

    constexpr const TKit::FixedArray<Index, KeyCount> &GetSparse() const
    {
        return m_Sparse;
    }
    constexpr const TKit::Array<Key, KeyAllocState> &GetKeys() const
    {
        return m_Keys;
    }
    constexpr const TKit::Array<T, AllocState> &GetValues() const
    {
        return m_Values;
    }

    constexpr Key GetKey(const usize idx) const
    {
        return m_Keys[idx];
    }

    constexpr const T &GetValue(const usize idx) const
    {
        return m_Values[idx];
    }
    constexpr T &GetValue(const usize idx)
    {
        return m_Values[idx];
    }

    constexpr usize GetSize() const
    {
        return m_Values.GetSize();
    }
    constexpr usize GetCapacity() const
    {
        return m_Values.GetCapacity();
    }
    constexpr usz GetBytes() const
    {
        return m_Values.GetBytes();
    }

    constexpr bool IsEmpty() const
    {
        return m_Values.IsEmpty();
    }
    constexpr bool IsFull() const
    {
        return m_Values.IsFull();
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &Insert(const Key k, Args &&...args)
    {
        CHECK_KEY_WITHIN_BOUNDS(k);
        TKIT_ASSERT(!Contains(k),
                    "[TOOLKIT][DENSE-MAP] Insert() can only be used when entry to be inserted does not exist, but key "
                    "{} is contained. Use TryInsert() in case the entry may be contained already",
                    k);

        TKIT_ASSERT(m_Values.GetSize() + 1 < Null,
                    "[TOOLKIT][DENSE-MAP] Maximum amount of elements representable with provided index type ({}) "
                    "reached. Use a bigger index type",
                    Null);
        m_Sparse[k] = m_Values.GetSize();
        m_Keys.Append(k);
        return m_Values.Append(std::forward<Args>(args)...);
    }
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &TryInsert(bool *didInsert, const Key k, Args &&...args)
    {
        CHECK_KEY_WITHIN_BOUNDS(k);
        Index &idx = m_Sparse[k];
        if (idx == Null)
        {
            TKIT_ASSERT(m_Values.GetSize() + 1 < Null,
                        "[TOOLKIT][DENSE-MAP] Maximum amount of elements representable with provided index type ({}) "
                        "reached. Use a bigger index type",
                        Null);
            if (didInsert)
                *didInsert = true;
            idx = m_Values.GetSize();
            m_Keys.Append(k);
            return m_Values.Append(std::forward<Args>(args)...);
        }
        if (didInsert)
            *didInsert = false;
        return m_Values[idx];
    }
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &TryInsert(const Key k, Args &&...args)
    {
        return TryInsert(nullptr, k, std::forward<Args>(args)...);
    }

    constexpr void Remove(const Key k)
    {
        CHECK_KEY_EXISTS(k);
        Index &idx = m_Sparse[k];
        const usize backIdx = m_Values.GetSize() - 1;

        if (idx == backIdx)
        {
            m_Keys.Pop();
            m_Values.Pop();
        }
        else
        {
            m_Sparse[m_Keys[backIdx]] = idx;
            m_Values.RemoveUnordered(m_Values.begin() + idx);
            m_Keys.RemoveUnordered(m_Keys.begin() + idx);
        }
        idx = Null;
    }
    constexpr bool TryRemove(const Key k)
    {
        if (Contains(k))
        {
            Remove(k);
            return true;
        }
        return false;
    }

#undef CHECK_KEY_WITHIN_BOUNDS
#undef CHECK_INDEX_WITHIN_BOUNDS
#undef CHECK_KEY_EXISTS

  private:
    // The source's dense array is empty only if its storage was actually stolen. Static storage element-moves and keeps
    // its size (and keys), so the source stays consistent and its sparse array must be left alone
    template <typename Other> constexpr void releaseFrom(Other &other)
    {
        if (!other.m_Keys.IsEmpty())
            return;
        for (const Key k : m_Keys)
            other.m_Sparse[k] = Null;
    }
    TKit::FixedArray<Index, KeyCount> m_Sparse{};
    TKit::Array<Key, KeyAllocState> m_Keys{};
    TKit::Array<T, AllocState> m_Values{};

    template <typename U, usize OtherKeyCount, typename OtherAllocState> friend class DenseMap;
    template <typename U, typename OtherFieldDesc, typename OtherAllocState> friend class BitFieldMap;
};

template <typename T, usize KeyCount> using ArenaDenseMap = DenseMap<T, KeyCount, ArenaAllocation<T>>;
template <typename T, usize KeyCount> using StackDenseMap = DenseMap<T, KeyCount, StackAllocation<T>>;
template <typename T, usize KeyCount> using TierDenseMap = DenseMap<T, KeyCount, TierAllocation<T>>;
template <typename T, usize KeyCount> using DynamicDenseMap = DenseMap<T, KeyCount, DynamicAllocation<T>>;
template <typename T, usize KeyCount, usize N = KeyCount>
using StaticDenseMap = DenseMap<T, KeyCount, StaticAllocation<T, N>>;

} // namespace TKit
