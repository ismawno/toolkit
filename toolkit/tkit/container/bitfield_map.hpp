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
    using KeyAllocState = AllocState::template Rebind<Key>;

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
        m_Keys.Reserve(capacity);
        m_Values.Reserve(capacity);
    }

    constexpr BitFieldMap(const BitFieldMap &) = default;
    constexpr BitFieldMap &operator=(const BitFieldMap &) = default;

    constexpr BitFieldMap(BitFieldMap &&other)
        : m_Sparse(other.m_Sparse), m_Keys(std::move(other.m_Keys)), m_Values(std::move(other.m_Values))
    {
        releaseFrom(other);
    }
    constexpr BitFieldMap &operator=(BitFieldMap &&other)
    {
        m_Sparse = other.m_Sparse;
        m_Keys = std::move(other.m_Keys);
        m_Values = std::move(other.m_Values);
        releaseFrom(other);
        return *this;
    }

    template <typename OtherAlloc>
    constexpr BitFieldMap(const BitFieldMap<T, FieldDesc, OtherAlloc, Index> &other)
        : m_Sparse(other.m_Sparse), m_Keys(other.m_Keys), m_Values(other.m_Values)
    {
    }
    template <typename OtherAlloc>
    constexpr BitFieldMap(BitFieldMap<T, FieldDesc, OtherAlloc, Index> &&other)
        : m_Sparse(other.m_Sparse), m_Keys(std::move(other.m_Keys)), m_Values(std::move(other.m_Values))
    {
        releaseFrom(other);
    }

    template <typename OtherAlloc>
    constexpr BitFieldMap &operator=(const BitFieldMap<T, FieldDesc, OtherAlloc, Index> &other)
    {
        m_Sparse = other.m_Sparse;
        m_Keys = other.m_Keys;
        m_Values = other.m_Values;
        return *this;
    }
    template <typename OtherAlloc>
    constexpr BitFieldMap &operator=(BitFieldMap<T, FieldDesc, OtherAlloc, Index> &&other)
    {
        m_Sparse = other.m_Sparse;
        m_Keys = std::move(other.m_Keys);
        m_Values = std::move(other.m_Values);
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
    TKIT_ASSERT((idx) < GetSize(),                                                                                     \
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
        return m_Values[m_Sparse[k]];
    }
    constexpr T &At(Key k)
    {
        k = canon(k);
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

    const TKit::Array<Key, KeyAllocState> &GetKeys() const
    {
        return m_Keys;
    }
    Key GetKey(const usize idx) const
    {
        return m_Keys[idx];
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
    constexpr T &Insert(Key k, Args &&...args)
    {
        k = canon(k);
        CHECK_KEY_WITHIN_BOUNDS(k);
        TKIT_ASSERT(!Contains(k),
                    "[TOOLKIT][BITFIELD] Insert() can only be used when entry to be inserted does not exist, but key "
                    "{:#0{}b} is contained. Use TryInsert() in case the entry may be contained already",
                    k, BitCount + 2);

        TKIT_ASSERT(m_Values.GetSize() + 1 < Null,
                    "[TOOLKIT][BITFIELD] Maximum amount of elements representable with provided index type ({}) "
                    "reached. Use a bigger index type",
                    Null);
        m_Sparse[k] = m_Values.GetSize();
        m_Keys.Append(k);
        return m_Values.Append(std::forward<Args>(args)...);
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
            TKIT_ASSERT(m_Values.GetSize() + 1 < Null,
                        "[TOOLKIT][BITFIELD] Maximum amount of elements representable with provided index type ({}) "
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

    constexpr void Remove(Key k)
    {
        k = canon(k);
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

#define EMIT_LOOP(...)                                                                                                 \
    for (usize i = 0; i < GetSize(); ++i)                                                                              \
        if ((m_Keys[i] & mask) == partialKey)                                                                          \
        {                                                                                                              \
            if constexpr (hasRet)                                                                                      \
            {                                                                                                          \
                if (!func(__VA_ARGS__))                                                                                \
                    return false;                                                                                      \
            }                                                                                                          \
            else                                                                                                       \
                func(__VA_ARGS__);                                                                                     \
        }

#define QUERY_IMPL(type)                                                                                               \
    if constexpr (std::is_invocable_v<F, type>)                                                                        \
    {                                                                                                                  \
        constexpr bool hasRet = std::is_same_v<std::invoke_result_t<F, type>, bool>;                                   \
        EMIT_LOOP(m_Values[i]);                                                                                        \
        if constexpr (hasRet)                                                                                          \
            return true;                                                                                               \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        constexpr bool hasRet = std::is_same_v<std::invoke_result_t<F, Key, type>, bool>;                              \
        EMIT_LOOP(m_Keys[i], m_Values[i]);                                                                             \
        if constexpr (hasRet)                                                                                          \
            return true;                                                                                               \
    }

    template <typename F> auto Query(const Key partialKey, const Key mask, F func) const
    {
        QUERY_IMPL(const T &)
    }
    template <typename F> auto Query(const Key partialKey, const Key mask, F func)
    {
        QUERY_IMPL(T &);
    }

#undef EMIT_LOOP
#undef QUERY_IMPL

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
    TKit::Array<Key, KeyAllocState> m_Keys{};
    TKit::Array<T, AllocState> m_Values{};

    template <typename U, typename OtherFieldDesc, typename OtherAllocState, UnsignedInteger OtherIndex>
    friend class BitFieldMap;
};
} // namespace TKit
