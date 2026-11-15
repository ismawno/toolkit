#pragma once

#include "tkit/container/dense_map.hpp"

namespace TKit
{
template <usize N> struct IdentityFieldDesc
{
    static constexpr usize BitCount = N;
};

template <usize BitCount> using BitFieldKey = u<Math::Max(NextPowerOfTwo(BitCount), usize(8))>;
template <usize BitCount> using BitFieldIndex = BitFieldKey<BitCount + 1>;

template <typename Desc>
concept HasCanonicalize = requires(BitFieldKey<Desc::BitCount> k) {
    { Desc::Canonicalize(k) } -> std::convertible_to<BitFieldKey<Desc::BitCount>>;
};

template <typename Desc>
concept HasKeyValidityCheck = requires(BitFieldKey<Desc::BitCount> k) {
    { Desc::IsValid(k) } -> std::same_as<bool>;
};

template <typename FieldDesc> constexpr BitFieldIndex<FieldDesc::BitCount> ComputeKeySpace()
{
    // we use Index here because type Key can hold 0...(KeySpace - 1)
    constexpr usize bcount = FieldDesc::BitCount;
    using Index = BitFieldKey<bcount + 1>;
    constexpr Index baseSpace = Index(1) << bcount;

    constexpr auto doesCount = [](const Index k) {
        if constexpr (HasCanonicalize<FieldDesc> && HasKeyValidityCheck<FieldDesc>)
            return FieldDesc::Canonicalize(k) == k && FieldDesc::IsValid(k);
        else if constexpr (HasCanonicalize<FieldDesc>)
            return FieldDesc::Canonicalize(k) == k;
        else if constexpr (HasKeyValidityCheck<FieldDesc>)
            return FieldDesc::IsValid(k);
        else
            return true;
    };

    Index count = 0;
    for (Index k = 0; k < baseSpace; ++k)
        if (doesCount(k))
            ++count;
    return count;
}

template <typename T, typename FieldDesc, typename AllocState> class BitFieldMap
{
  public:
    static constexpr usize BitCount = FieldDesc::BitCount;

    using Key = BitFieldKey<BitCount>;
    using Index = BitFieldIndex<BitCount>;
    using KeyAllocState = AllocState::template Rebind<Key>;

    static constexpr ArrayType Type = AllocState::Type;
    static constexpr Index Null = TKit::Limits<Index>::Max();

    constexpr BitFieldMap() : m_Map()
    {
    }
    constexpr BitFieldMap(const usize capacity)
        requires(Type != Array_Static)
        : m_Map(capacity)
    {
    }

    constexpr BitFieldMap(const BitFieldMap &) = default;
    constexpr BitFieldMap &operator=(const BitFieldMap &) = default;

    constexpr BitFieldMap(BitFieldMap &&) = default;
    constexpr BitFieldMap &operator=(BitFieldMap &&) = default;

    template <typename OtherAlloc>
    constexpr BitFieldMap(const BitFieldMap<T, FieldDesc, OtherAlloc> &other) : m_Map(other.m_Map)
    {
    }
    template <typename OtherAlloc>
    constexpr BitFieldMap(BitFieldMap<T, FieldDesc, OtherAlloc> &&other) : m_Map(std::move(other.m_Map))
    {
    }

    template <typename OtherAlloc> constexpr BitFieldMap &operator=(const BitFieldMap<T, FieldDesc, OtherAlloc> &other)
    {
        m_Map = other.m_Map;
        return *this;
    }
    template <typename OtherAlloc> constexpr BitFieldMap &operator=(BitFieldMap<T, FieldDesc, OtherAlloc> &&other)
    {
        m_Map = std::move(other.m_Map);
        return *this;
    }

    constexpr bool Contains(Key k) const
    {
        k = canon(k);
        return m_Map.Contains(k);
    }

#define CHECK_KEY_WITHIN_BOUNDS(k)                                                                                     \
    TKIT_ASSERT(                                                                                                       \
        (k) < m_Map.m_Sparse.GetSize(),                                                                                \
        "[TOOLKIT][BIT-FIELD-MAP] The key {:#0{}b} exceeds sparse array size ({}) derived from the bit count of {} "   \
        "specified at compile time",                                                                                   \
        k, BitCount + 2, m_Map.m_Sparse.GetSize(), BitCount)
#define CHECK_INDEX_WITHIN_BOUNDS(idx)                                                                                 \
    TKIT_ASSERT(                                                                                                       \
        (idx) < GetSize(),                                                                                             \
        "[TOOLKIT][BIT-FIELD-MAP] The key {:#0{}b} is contained in the map, but it is not stored in the dense "        \
        "array. This datastructure has been corrupted",                                                                \
        k, BitCount + 2);

#define CHECK_KEY_EXISTS(k)                                                                                            \
    CHECK_KEY_WITHIN_BOUNDS(k);                                                                                        \
    TKIT_ASSERT(m_Map.GetSparse()[k] != Null, "[TOOLKIT][BIT-FIELD-MAP] The key {:#0{}b} is not contained in the map", \
                k, BitCount + 2);                                                                                      \
    CHECK_INDEX_WITHIN_BOUNDS(m_Map.GetSparse()[k])

    constexpr const T &At(Key k) const
    {
        k = canon(k);
        CHECK_KEY_EXISTS(k);
        return m_Map[k];
    }
    constexpr T &At(Key k)
    {
        k = canon(k);
        CHECK_KEY_EXISTS(k);
        return m_Map[k];
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
        m_Map.Clear();
    }

    constexpr const T *GetData() const
    {
        return m_Map.GetData();
    }
    constexpr T *GetData()
    {
        return m_Map.GetData();
    }

    constexpr const T *begin() const
    {
        return m_Map.begin();
    }
    constexpr const T *end() const
    {
        return m_Map.end();
    }

    constexpr T *begin()
    {
        return m_Map.begin();
    }
    constexpr T *end()
    {
        return m_Map.end();
    }

    constexpr const TKit::Array<Key, KeyAllocState> &GetKeys() const
    {
        return m_Map.GetKeys();
    }
    constexpr const TKit::Array<T, AllocState> &GetValues() const
    {
        return m_Map.GetValues();
    }
    constexpr Key GetKey(const usize idx) const
    {
        return m_Map.GetKey(idx);
    }

    constexpr const T &GetValue(const usize idx) const
    {
        return m_Map.GetValue(idx);
    }
    constexpr T &GetValue(const usize idx)
    {
        return m_Map.GetValue(idx);
    }

    constexpr usize GetSize() const
    {
        return m_Map.GetSize();
    }
    constexpr usize GetCapacity() const
    {
        return m_Map.GetCapacity();
    }
    constexpr usz GetBytes() const
    {
        return m_Map.GetBytes();
    }

    constexpr bool IsEmpty() const
    {
        return m_Map.IsEmpty();
    }
    constexpr bool IsFull() const
    {
        return m_Map.IsFull();
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &Insert(Key k, Args &&...args)
    {
        k = canon(k);
        CHECK_KEY_WITHIN_BOUNDS(k);
        TKIT_ASSERT(
            !Contains(k),
            "[TOOLKIT][BIT-FIELD-MAP] Insert() can only be used when entry to be inserted does not exist, but key "
            "{:#0{}b} is contained. Use TryInsert() in case the entry may be contained already",
            k, BitCount + 2);

        return m_Map.Insert(k, std::forward<Args>(args)...);
    }
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &TryInsert(bool *didInsert, Key k, Args &&...args)
    {
        k = canon(k);
        CHECK_KEY_WITHIN_BOUNDS(k);
        return m_Map.TryInsert(didInsert, k, std::forward<Args>(args)...);
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
        m_Map.Remove(k);
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
        if ((m_Map.m_Keys[i] & mask) == partialKey)                                                                    \
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
    TKIT_ASSERT((partialKey & mask) == partialKey,                                                                     \
                "[TOOLKIT][BIT-FIELD-MAP] The mask {:#0{}b} does not fully cover the "                                 \
                "breadth of bits that the partial key {:#0{}b} includes",                                              \
                mask, BitCount + 2, partialKey, BitCount + 2);                                                         \
    if constexpr (std::is_invocable_v<F, type>)                                                                        \
    {                                                                                                                  \
        constexpr bool hasRet = std::is_same_v<std::invoke_result_t<F, type>, bool>;                                   \
        EMIT_LOOP(m_Map.m_Values[i]);                                                                                  \
        if constexpr (hasRet)                                                                                          \
            return true;                                                                                               \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        constexpr bool hasRet = std::is_same_v<std::invoke_result_t<F, Key, type>, bool>;                              \
        EMIT_LOOP(m_Map.m_Keys[i], m_Map.m_Values[i]);                                                                 \
        if constexpr (hasRet)                                                                                          \
            return true;                                                                                               \
    }

    template <typename F> constexpr auto Query(const Key partialKey, const Key mask, F func) const
    {
        QUERY_IMPL(const T &)
    }
    template <typename F> constexpr auto Query(const Key partialKey, const Key mask, F func)
    {
        QUERY_IMPL(T &);
    }

#undef EMIT_LOOP
#undef QUERY_IMPL

#undef CHECK_KEY_WITHIN_BOUNDS
#undef CHECK_INDEX_WITHIN_BOUNDS
#undef CHECK_KEY_EXISTS

  private:
    static constexpr Key canon(Key k)
    {
        if constexpr (HasCanonicalize<FieldDesc>)
        {
            TKIT_ASSERT(FieldDesc::Canonicalize(FieldDesc::Canonicalize(k)) == FieldDesc::Canonicalize(k),
                        "[TOOLKIT][BIT-FIELD-MAP] Canonicalize function is not idempotent for key {:#0{}b}, as "
                        "Canonicalize(Canonicalize(k)) = {:#0{}b} != {:#0{}b} = Canonicalize(k)",
                        k, BitCount + 2, FieldDesc::Canonicalize(FieldDesc::Canonicalize(k)), BitCount + 2,
                        FieldDesc::Canonicalize(k), BitCount + 2);

            k = FieldDesc::Canonicalize(k);
        }
#ifdef TKIT_ENABLE_ENSURE
        if constexpr (HasKeyValidityCheck<FieldDesc>)
        {
            TKIT_ENSURE(FieldDesc::IsValid(k),
                        "[TOOLKIT][BIT-FIELD-MAP] The key {:#0{}b} has been flagged as invalid by the user defined "
                        "function IsValid()",
                        k, BitCount + 2);
        }
#endif
        return k;
    }
    TKit::DenseMap<T, usize(1) << BitCount, AllocState> m_Map;

    template <typename U, typename OtherFieldDesc, typename OtherAllocState> friend class BitFieldMap;
};

template <typename T, typename FieldDesc> using ArenaBitFieldMap = BitFieldMap<T, FieldDesc, ArenaAllocation<T>>;
template <typename T, typename FieldDesc> using StackBitFieldMap = BitFieldMap<T, FieldDesc, StackAllocation<T>>;
template <typename T, typename FieldDesc> using TierBitFieldMap = BitFieldMap<T, FieldDesc, TierAllocation<T>>;
template <typename T, typename FieldDesc> using DynamicBitFieldMap = BitFieldMap<T, FieldDesc, DynamicAllocation<T>>;
template <typename T, typename FieldDesc, usize N = ComputeKeySpace<FieldDesc>()>
using StaticBitFieldMap = BitFieldMap<T, FieldDesc, StaticAllocation<T, N>>;

} // namespace TKit
