#pragma once

#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/container/span.hpp"

namespace TKit
{
template <typename T, typename AllocState> class Hive
{
  public:
    using ValueType = T;
    using Id = usize;
    using IdAllocState = AllocState::template Rebind<Id>;

    constexpr Hive() = default;

    template <std::input_iterator It>
    constexpr Hive(const It idxBegin, const Id idxEnd, const It idBegin, const It idEnd)
        : m_Indices(idxBegin, idxEnd), m_Ids(idBegin, idEnd)
    {
        TKIT_ASSERT(m_Indices.GetSize() == m_Ids.GetSize(),
                    "[TOOLKIT][HIVE] Indices and ids sizes must be equal, but are {} and {}", m_Indices.GetSize(),
                    m_Ids.GetSize());
#ifdef TKIT_ENABLE_ENSURE
        for (usize id = 0; id < m_Indices.GetSize(); ++id)
        {
            TKIT_ENSURE(m_Ids[m_Indices[id]] == id,
                        "[TOOLKIT][HIVE] To specify indices and ids, they must satisfy ids[indices[id]] == id, however "
                        "found ids[indices[id = {}] = {}] = {} != id = {}",
                        id, m_Indices[id], m_Ids[m_Indices[id]], id);
        }
#endif
    }

    constexpr Hive(const Span<const usize> indices, const Span<const usize> ids) : m_Indices(indices), m_Ids(ids)
    {
        TKIT_ASSERT(indices.GetSize() == ids.GetSize(),
                    "[TOOLKIT][HIVE] Indices and ids sizes must be equal, but are {} and {}", m_Indices.GetSize(),
                    indices.GetSize(), ids.GetSize());
#ifdef TKIT_ENABLE_ENSURE
        for (usize id = 0; id < m_Indices.GetSize(); ++id)
        {
            TKIT_ENSURE(m_Ids[m_Indices[id]] == id,
                        "[TOOLKIT][HIVE] To specify indices and ids, they must satisfy ids[indices[id]] == id, however "
                        "found ids[indices[id = {}] = {}] = {} != id = {}",
                        id, m_Indices[id], m_Ids[m_Indices[id]], id);
        }
#endif
    }

    constexpr Hive(const Hive &) = default;
    constexpr Hive(Hive &&) = default;

    constexpr Hive &operator=(const Hive &) = default;
    constexpr Hive &operator=(Hive &&) = default;

    template <typename OtherAlloc>
    constexpr Hive(const Hive<T, OtherAlloc> &other)
        : m_Dense(other.m_Dense), m_Indices(other.m_Indices), m_Ids(other.m_Ids)
    {
    }

    template <typename OtherAlloc>
    constexpr Hive(Hive<T, OtherAlloc> &&other)
        : m_Dense(std::move(other.m_Dense)), m_Indices(std::move(other.m_Indices)), m_Ids(std::move(other.m_Ids))
    {
    }

    template <typename OtherAlloc> constexpr Hive &operator=(const Hive<T, OtherAlloc> &other)
    {
        m_Dense = other.m_Dense;
        m_Indices = other.m_Indices;
        m_Ids = other.m_Ids;
        return *this;
    }

    template <typename OtherAlloc> constexpr Hive &operator=(Hive<T, OtherAlloc> &&other)
    {
        m_Dense = std::move(other.m_Dense);
        m_Indices = std::move(other.m_Indices);
        m_Ids = std::move(other.m_Ids);
        return *this;
    }

#define CHECK_ID_EXISTS(id)                                                                                            \
    TKIT_ASSERT((id) < m_Indices.GetSize(),                                                                            \
                "[TOOLKIT][HIVE] The id {} does not exist in the hive, as it exceeds the indices size ({})", id,       \
                m_Indices.GetSize());                                                                                  \
    TKIT_ASSERT(                                                                                                       \
        m_Indices[id] < m_Dense.GetSize(),                                                                             \
        "[TOOLKIT][HIVE] The id {} does not exist in the hive, as indices[id = {}] = {} exceeds data size ({})", id,   \
        id, m_Indices[id], m_Dense.GetSize())

    constexpr const T &At(const usize id) const
    {
        CHECK_ID_EXISTS(id);
        return m_Dense[m_Indices[id]];
    }
    constexpr T &At(const usize id)
    {
        CHECK_ID_EXISTS(id);
        return m_Dense[m_Indices[id]];
    }

    constexpr const T &operator[](const usize id) const
    {
        return At(id);
    }
    constexpr T &operator[](const usize id)
    {
        return At(id);
    }

    constexpr const T &AtByIndex(const usize idx) const
    {
        return m_Dense[idx];
    }
    constexpr T &AtByIndex(const usize idx)
    {
        return m_Dense[idx];
    }

    constexpr const T *Get(const usize id) const
    {
        return Contains(id) ? &At(id) : nullptr;
    }
    constexpr T *Get(const usize id)
    {
        return Contains(id) ? &At(id) : nullptr;
    }

    constexpr const T *GetByIndex(const usize idx) const
    {
        return idx < m_Dense.GetSize() ? &AtByIndex(idx) : nullptr;
    }
    constexpr T *GetByIndex(const usize idx)
    {
        return idx < m_Dense.GetSize() ? &AtByIndex(idx) : nullptr;
    }

    constexpr void Reserve(const usize capacity)
    {
        m_Dense.Reserve(capacity);
        m_Indices.Reserve(capacity);
        m_Ids.Reserve(capacity);
    }

    constexpr bool Contains(const usize id) const
    {
        return id < m_Indices.GetSize() && m_Indices[id] < m_Dense.GetSize();
    }
    constexpr bool IsValidIndex(const usize idx) const
    {
        return idx < m_Dense.GetSize();
    }

    constexpr void Clear()
    {
        m_Dense.Clear();
    }
    constexpr void ClearAll()
    {
        m_Dense.Clear();
        m_Indices.Clear();
        m_Ids.Clear();
    }

#define EMIT_LOOP(...)                                                                                                 \
    for (usize id = 0; id < m_Indices.GetSize(); ++id)                                                                 \
    {                                                                                                                  \
        const usize idx = m_Indices[id];                                                                               \
        if (IsValidIndex(idx))                                                                                         \
        {                                                                                                              \
            if constexpr (hasRet)                                                                                      \
            {                                                                                                          \
                if (!func(__VA_ARGS__))                                                                                \
                    return false;                                                                                      \
            }                                                                                                          \
            else                                                                                                       \
                func(__VA_ARGS__);                                                                                     \
        }                                                                                                              \
    }

#define ITERATE_BY_INSERTION_ORDER(type)                                                                               \
    if constexpr (std::is_invocable_v<F, usize, type>)                                                                 \
    {                                                                                                                  \
        constexpr bool hasRet = std::is_same_v<std::invoke_result_t<F, usize, type>, bool>;                            \
        EMIT_LOOP(id, AtByIndex(idx));                                                                                 \
        if constexpr (hasRet)                                                                                          \
            return true;                                                                                               \
    }                                                                                                                  \
    else if constexpr (std::is_invocable_v<F, usize>)                                                                  \
    {                                                                                                                  \
        constexpr bool hasRet = std::is_same_v<std::invoke_result_t<F, usize>, bool>;                                  \
        EMIT_LOOP(id);                                                                                                 \
        if constexpr (hasRet)                                                                                          \
            return true;                                                                                               \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        constexpr bool hasRet = std::is_same_v<std::invoke_result_t<F, type>, bool>;                                   \
        EMIT_LOOP(AtByIndex(idx));                                                                                     \
        if constexpr (hasRet)                                                                                          \
            return true;                                                                                               \
    }

    template <typename F> auto IterateByInsertionOrder(F func)
    {
        ITERATE_BY_INSERTION_ORDER(T &)
    }
    template <typename F> auto IterateByInsertionOrder(F func) const
    {
        ITERATE_BY_INSERTION_ORDER(const T &)
    }

#undef ITERATE_BY_INSERTION_ORDER
#undef EMIT_LOOP

    constexpr const Array<usize, IdAllocState> &GetIndices() const
    {
        return m_Indices;
    }
    constexpr const Array<usize, IdAllocState> &GetIds() const
    {
        return m_Ids;
    }
    constexpr Span<const usize> GetValidIds() const
    {
        return Span<const usize>{m_Ids.GetData(), m_Dense.GetSize()};
    }

    constexpr usize GetIndex(const usize id) const
    {
        return m_Indices[id];
    }
    constexpr usize GetId(const usize index) const
    {
        return m_Ids[index];
    }

    constexpr const T *GetData() const
    {
        return m_Dense.GetData();
    }
    constexpr T *GetData()
    {
        return m_Dense.GetData();
    }

    constexpr T *begin()
    {
        return m_Dense.begin();
    }
    constexpr T *end()
    {
        return m_Dense.end();
    }

    constexpr const T *begin() const
    {
        return m_Dense.begin();
    }
    constexpr const T *end() const
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

    constexpr usize GetNextId() const
    {
        const usize size = m_Dense.GetSize();
        const usize idxs = m_Indices.GetSize();

        TKIT_ASSERT(
            size <= idxs,
            "[TOOLKIT][HIVE] Data constainer size ({}) must be smaller or equal to index/id container size ({})", size,
            idxs);

        return size == idxs ? size : m_Ids[size];
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr usize Insert(Args &&...args)
    {
        const usize size = m_Dense.GetSize();
        const usize idxs = m_Indices.GetSize();
        TKIT_ASSERT(idxs == m_Ids.GetSize(), "[TOOLKIT][HIVE] Indices size ({}) and ids size ({}) mismatch", idxs,
                    m_Ids.GetSize());
        TKIT_ASSERT(
            size <= idxs,
            "[TOOLKIT][HIVE] Data constainer size ({}) must be smaller or equal to index/id container size ({})", size,
            idxs);
        m_Dense.Append(std::forward<Args>(args)...);
        if (size == idxs)
        {
            m_Indices.Append(size);
            m_Ids.Append(size);
            return size;
        }
        return m_Ids[size];
    }

    constexpr void Remove(const usize id)
    {
        CHECK_ID_EXISTS(id);
        const usize idx = m_Indices[id];
        const usize last = m_Dense.GetSize() - 1;
        if (idx == last)
        {
            m_Dense.Pop();
            return;
        }
        const usize lid = m_Ids[last];
        m_Dense.RemoveUnordered(m_Dense.begin() + idx);
        m_Indices[id] = last;
        m_Indices[lid] = idx;
        std::swap(m_Ids[idx], m_Ids[last]);
    }
    constexpr bool TryRemove(const usize id)
    {
        if (Contains(id))
        {
            Remove(id);
            return true;
        }
        return false;
    }
#undef CHECK_ID_EXISTS

  private:
    Array<T, AllocState> m_Dense{};
    Array<usize, IdAllocState> m_Indices{};
    Array<usize, IdAllocState> m_Ids{};

    template <typename U, typename OtherAlloc> friend class Hive;
};
template <typename T> using ArenaHive = Hive<T, ArenaAllocation<T>>;
template <typename T> using DynamicHive = Hive<T, DynamicAllocation<T>>;
template <typename T> using StackHive = Hive<T, StackAllocation<T>>;
template <typename T> using TierHive = Hive<T, TierAllocation<T>>;

template <typename T, usize Capacity> using StaticHive = Hive<T, StaticAllocation<T, Capacity>>;
} // namespace TKit
