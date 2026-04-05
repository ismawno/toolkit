#pragma once

#include "tkit/container/array.hpp"
#include "tkit/container/span.hpp"

namespace TKit
{
template <typename T, typename AllocState> class Hive
{
  public:
    using ValueType = T;
    using Id = usize;

    template <typename... Args>
    constexpr explicit Hive(Args &&...args)
        : m_Data(std::forward<Args>(args)...), m_Indices(std::forward<Args>(args)...),
          m_Ids(std::forward<Args>(args)...)
    {
        TKIT_ASSERT(
            m_Data.IsEmpty() && m_Indices.IsEmpty() && m_Ids.IsEmpty(),
            "[TOOLKIT][HIVE] Only allocator state constructors that leave the container empty are allowed to be used");
    }

    constexpr Hive(const Hive &) = default;
    constexpr Hive(Hive &&) = default;

    constexpr Hive &operator=(const Hive &) = default;
    constexpr Hive &operator=(Hive &&) = default;

    template <typename OtherAlloc>
    constexpr Hive(const Hive<T, OtherAlloc> &other)
        : m_Data(other.m_Data), m_Indices(other.m_Indices), m_Ids(other.m_Ids)
    {
    }

    template <typename OtherAlloc>
    constexpr Hive(Hive<T, OtherAlloc> &&other)
        : m_Data(std::move(other.m_Data)), m_Indices(std::move(other.m_Indices)), m_Ids(std::move(other.m_Ids))
    {
    }

    template <typename OtherAlloc> constexpr Hive &operator=(const Hive<T, OtherAlloc> &other)
    {
        m_Data = other.m_Data;
        m_Indices = other.m_Indices;
        m_Ids = other.m_Ids;
        return *this;
    }

    template <typename OtherAlloc> constexpr Hive &operator=(Hive<T, OtherAlloc> &&other)
    {
        m_Data = std::move(other.m_Data);
        m_Indices = std::move(other.m_Indices);
        m_Ids = std::move(other.m_Ids);
        return *this;
    }

    constexpr const T &operator[](const usize id) const
    {
        return At(id);
    }
    constexpr T &operator[](const usize id)
    {
        return At(id);
    }
    constexpr const T &At(const usize id) const
    {
        TKIT_ASSERT(id < m_Indices.GetSize(),
                    "[TOOLKIT][HIVE] The id {} does not exist in the hive, as it exceeds the indices size ({})", id,
                    m_Indices.GetSize());
        TKIT_ASSERT(
            m_Indices[id] < m_Data.GetSize(),
            "[TOOLKIT][HIVE] The id {} does not exist in the hive, as indices[id = {}] = {} exceeds data size ({})", id,
            id, m_Indices[id], m_Data.GetSize());
        return m_Data[m_Indices[id]];
    }
    constexpr T &At(const usize id)
    {
        TKIT_ASSERT(id < m_Indices.GetSize(),
                    "[TOOLKIT][HIVE] The id {} does not exist in the hive, as it exceeds the indices size ({})", id,
                    m_Indices.GetSize());
        TKIT_ASSERT(
            m_Indices[id] < m_Data.GetSize(),
            "[TOOLKIT][HIVE] The id {} does not exist in the hive, as indices[id = {}] = {} exceeds data size ({})", id,
            id, m_Indices[id], m_Data.GetSize());
        return m_Data[m_Indices[id]];
    }

    constexpr const T *Get(const usize id) const
    {
        return Contains(id) ? &At(id) : nullptr;
    }
    constexpr T *Get(const usize id)
    {
        return Contains(id) ? &At(id) : nullptr;
    }

    constexpr void Reserve(const usize capacity)
    {
        m_Data.Reserve(capacity);
        m_Indices.Reserve(capacity);
        m_Ids.Reserve(capacity);
    }

    constexpr bool Contains(const usize id) const
    {
        return id < m_Indices.GetSize() && m_Indices[id] < m_Data.GetSize();
    }

    constexpr const Array<usize, AllocState> &GetIndices() const
    {
        return m_Indices;
    }
    constexpr const Array<usize, AllocState> &GetIds() const
    {
        return m_Ids;
    }
    constexpr Span<const usize> GetValidIds() const
    {
        return Span<const usize>{m_Ids.GetData(), m_Data.GetSize()};
    }

    constexpr usize GetIndex(const usize id)
    {
        return m_Indices[id];
    }
    constexpr usize GetId(const usize index)
    {
        return m_Ids[index];
    }

    constexpr const T *GetData() const
    {
        return m_Data.GetData();
    }
    constexpr T *GetData()
    {
        return m_Data.GetData();
    }

    constexpr T *begin()
    {
        return m_Data.begin();
    }
    constexpr T *end()
    {
        return m_Data.end();
    }

    constexpr const T *begin() const
    {
        return m_Data.begin();
    }
    constexpr const T *end() const
    {
        return m_Data.end();
    }

    constexpr usize GetSize() const
    {
        return m_Data.GetSize();
    }
    constexpr usize GetCapacity() const
    {
        return m_Data.GetCapacity();
    }
    constexpr usz GetBytes() const
    {
        return m_Data.GetSize() * sizeof(T);
    }

    constexpr bool IsEmpty() const
    {
        return m_Data.IsEmpty();
    }
    constexpr bool IsFull() const
    {
        return m_Data.IsFull();
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr usize Insert(Args &&...args)
    {
        const usize size = m_Data.GetSize();
        const usize idxs = m_Indices.GetSize();
        TKIT_ASSERT(idxs == m_Ids.GetSize(), "[TOOLKIT][HIVE] Indices size ({}) and ids size ({}) mismatch", idxs,
                    m_Ids.GetSize());
        TKIT_ASSERT(
            size <= idxs,
            "[TOOLKIT][HIVE] Data constainer size ({}) must be smaller or equal to index/id container size ({})", size,
            idxs);
        m_Data.Append(std::forward<Args>(args)...);
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
        TKIT_ASSERT(id < m_Indices.GetSize(),
                    "[TOOLKIT][HIVE] The id {} does not exist in the hive, as it exceeds the indices size ({})", id,
                    m_Indices.GetSize());
        TKIT_ASSERT(
            m_Indices[id] < m_Data.GetSize(),
            "[TOOLKIT][HIVE] The id {} does not exist in the hive, as indices[id = {}] = {} exceeds data size ({})", id,
            id, m_Indices[id], m_Data.GetSize());
        const usize idx = m_Indices[id];
        const usize last = m_Data.GetSize() - 1;
        if (idx == last)
        {
            m_Data.Pop();
            return;
        }
        const usize lid = m_Ids[last];
        m_Data.RemoveUnordered(m_Data.begin() + idx);
        m_Indices[id] = last;
        m_Indices[lid] = idx;
        std::swap(m_Ids[idx], m_Ids[last]);
    }

  private:
    Array<T, AllocState> m_Data{};
    Array<usize, AllocState> m_Indices{};
    Array<usize, AllocState> m_Ids{};

    template <typename U, typename OtherAlloc> friend class Hive;
};
} // namespace TKit
