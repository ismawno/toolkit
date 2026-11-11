#pragma once

#include "tkit/container/array.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/utils/bit.hpp"
#include "tkit/math/math.hpp"

namespace TKit
{
template <usize N> struct IdentityFieldDesc
{
    static constexpr usize BitCount = N;
};

template <typename T, typename FieldDesc, typename AllocState, UnsignedInteger Index = u16> class BitFieldMap
{
  public:
    static constexpr usize BitCount = FieldDesc::BitCount;

    using Key = u<Math::Max(NextPowerOfTwo(BitCount), usize(8))>;
    using Pair = KeyValuePair<Key, T>;

    static constexpr ArrayType Type = AllocState::Type;
    static constexpr Index Null = TKit::Limits<Index>::Max();

    constexpr BitFieldMap()
    {
        for (Index &i : m_Sparse)
            i = Null;
    }
    constexpr BitFieldMap(const usize capacity)
        requires(Type != Array_Static)
        : BitFieldMap()
    {
        m_Dense.Reserve(capacity);
    }

    constexpr BitFieldMap(const BitFieldMap &) = default;
    constexpr BitFieldMap &operator=(const BitFieldMap &) = default;

    constexpr BitFieldMap(BitFieldMap &&other) : m_Sparse(other.m_Sparse), m_Dense(std::move(other.m_Dense))
    {
        releaseFrom(other);
    }
    constexpr BitFieldMap &operator=(BitFieldMap &&other)
    {
        m_Sparse = other.m_Sparse;
        m_Dense = std::move(other.m_Dense);
        releaseFrom(other);
        return *this;
    }

    template <typename OtherAlloc>
    constexpr BitFieldMap(const BitFieldMap<T, FieldDesc, OtherAlloc, Index> &other)
        : m_Sparse(other.m_Sparse), m_Dense(other.m_Dense)
    {
    }
    template <typename OtherAlloc>
    constexpr BitFieldMap(BitFieldMap<T, FieldDesc, OtherAlloc, Index> &&other)
        : m_Sparse(other.m_Sparse), m_Dense(std::move(other.m_Dense))
    {
        releaseFrom(other);
    }

    template <typename OtherAlloc>
    constexpr BitFieldMap &operator=(const BitFieldMap<T, FieldDesc, OtherAlloc, Index> &other)
    {
        m_Sparse = other.m_Sparse;
        m_Dense = other.m_Dense;
        return *this;
    }
    template <typename OtherAlloc>
    constexpr BitFieldMap &operator=(BitFieldMap<T, FieldDesc, OtherAlloc, Index> &&other)
    {
        m_Sparse = other.m_Sparse;
        m_Dense = std::move(other.m_Dense);
        releaseFrom(other);
        return *this;
    }

    constexpr bool Contains(Key k) const
    {
        k = canon(k);
        return k < m_Sparse.GetSize() && m_Sparse[k] != Null;
    }

#define CHECK_KEY_WITHIN_BOUNDS(k)                                                                                     \
    TKIT_ASSERT(                                                                                                       \
        (k) < m_Sparse.GetSize(),                                                                                      \
        "[TOOLKIT][BITFIELD] The key {:#0{}b} exceeds sparse array size ({}) derived from the bit count of {} "        \
        "specified at compile time",                                                                                   \
        k, BitCount + 2, m_Sparse.GetSize(), BitCount)
#define CHECK_INDEX_WITHIN_BOUNDS(idx)                                                                                 \
    TKIT_ASSERT((idx) < m_Dense.GetSize(),                                                                             \
                "[TOOLKIT][BITFIELD] The key {:#0{}b} is contained in the map, but it is not stored in the dense "     \
                "array. This datastructure has been corrupted",                                                        \
                k, BitCount + 2);

#define CHECK_KEY_EXISTS(k)                                                                                            \
    CHECK_KEY_WITHIN_BOUNDS(k);                                                                                        \
    TKIT_ASSERT(m_Sparse[k] != Null, "[TOOLKIT][BITFIELD] The key {:#0{}b} is not contained in the map", k,            \
                BitCount + 2);                                                                                         \
    CHECK_INDEX_WITHIN_BOUNDS(m_Sparse[k])

    constexpr const T &At(Key k) const
    {
        k = canon(k);
        CHECK_KEY_EXISTS(k);
        return m_Dense[m_Sparse[k]].Value;
    }
    constexpr T &At(Key k)
    {
        k = canon(k);
        CHECK_KEY_EXISTS(k);
        return m_Dense[m_Sparse[k]].Value;
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
        for (const Pair &p : m_Dense)
            m_Sparse[p.Key] = Null;
        m_Dense.Clear();
    }

    constexpr const Pair *GetData() const
    {
        return m_Dense.GetData();
    }
    constexpr Pair *GetData()
    {
        return m_Dense.GetData();
    }

    constexpr const Pair *begin() const
    {
        return m_Dense.begin();
    }
    constexpr const Pair *end() const
    {
        return m_Dense.end();
    }

    constexpr Pair *begin()
    {
        return m_Dense.begin();
    }
    constexpr Pair *end()
    {
        return m_Dense.end();
    }

    constexpr usize GetSize() const
    {
        return m_Dense.GetSize();
    }
    constexpr usize GetCapacity() const
    {
        return m_Dense.GetCapacity();
    }
    constexpr usz GetBytes() const
    {
        return m_Dense.GetBytes();
    }

    constexpr bool IsEmpty() const
    {
        return m_Dense.IsEmpty();
    }
    constexpr bool IsFull() const
    {
        return m_Dense.IsFull();
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &Insert(Key k, Args &&...args)
    {
        k = canon(k);
        CHECK_KEY_WITHIN_BOUNDS(k);
        TKIT_ASSERT(!Contains(k),
                    "[TOOLKIT][BITFIELD] Insert() can only be used when entry to be inserted does not exist, but key "
                    "{:#0{}b} is contained. Use TryInsert() in case the entry may be contained already",
                    k, BitCount + 2);

        TKIT_ASSERT(m_Dense.GetSize() + 1 < Null,
                    "[TOOLKIT][BITFIELD] Maximum amount of elements representable with provided index type ({}) "
                    "reached. Use a bigger index type",
                    Null);
        m_Sparse[k] = m_Dense.GetSize();
        return m_Dense.Append(k, std::forward<Args>(args)...).Value;
    }
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &TryInsert(bool *didInsert, Key k, Args &&...args)
    {
        k = canon(k);
        CHECK_KEY_WITHIN_BOUNDS(k);
        Index &idx = m_Sparse[k];
        if (idx == Null)
        {
            TKIT_ASSERT(m_Dense.GetSize() + 1 < Null,
                        "[TOOLKIT][BITFIELD] Maximum amount of elements representable with provided index type ({}) "
                        "reached. Use a bigger index type",
                        Null);
            if (didInsert)
                *didInsert = true;
            idx = m_Dense.GetSize();
            return m_Dense.Append(k, std::forward<Args>(args)...).Value;
        }
        if (didInsert)
            *didInsert = false;
        return m_Dense[idx].Value;
    }
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &TryInsert(const Key k, Args &&...args)
    {
        return TryInsert(nullptr, k, std::forward<Args>(args)...);
    }

    constexpr void Remove(Key k)
    {
        k = canon(k);
        CHECK_KEY_EXISTS(k);
        Index &idx = m_Sparse[k];
        const usize backIdx = m_Dense.GetSize() - 1;

        if (idx == backIdx)
            m_Dense.Pop();
        else
        {
            m_Sparse[m_Dense[backIdx].Key] = idx;
            m_Dense.RemoveUnordered(m_Dense.begin() + idx);
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
        if (!other.m_Dense.IsEmpty())
            return;
        for (const Pair &p : m_Dense)
            other.m_Sparse[p.Key] = Null;
    }
    constexpr Key canon(const Key k) const
    {
        if constexpr (requires {
                          { FieldDesc::Canonicalize(k) } -> std::convertible_to<Key>;
                      })
        {
            TKIT_ASSERT(FieldDesc::Canonicalize(FieldDesc::Canonicalize(k)) == FieldDesc::Canonicalize(k),
                        "[TOOLKIT][BITFIELD] Canonicalize function is not idempotent for key {:#0{}b}, as "
                        "Canonicalize(Canonicalize(k)) = {:#0{}b} != {:#0{}b} = Canonicalize(k)",
                        k, BitCount + 2, FieldDesc::Canonicalize(FieldDesc::Canonicalize(k)), BitCount + 2,
                        FieldDesc::Canonicalize(k), BitCount + 2);

            return FieldDesc::Canonicalize(k);
        }
        else
            return k;
    }
    TKit::FixedArray<Index, 1U << BitCount> m_Sparse{};
    TKit::Array<Pair, AllocState> m_Dense{};

    template <typename U, typename OtherFieldDesc, typename OtherAllocState, UnsignedInteger OtherIndex>
    friend class BitFieldMap;
};
} // namespace TKit
