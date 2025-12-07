#pragma once

#include "tkit/container/weak_array.hpp"
#include "tkit/container/dynamic_array.hpp"

namespace TKit
{
/**
 * @brief A view with a static extent over a contiguous sequence of objects.
 *
 * It is meant to be used as a drop-in replacement for `std::span`, especially for personal use and to be able to
 * have a bit more control.
 *
 * @tparam T The type of the elements in the span.
 * @tparam Extent The extent of the span.
 */
template <typename T, usize Extent = Limits<usize>::Max(),
          typename Traits = Container::ArrayTraits<std::remove_cvref_t<T>>> // Consider adding allocator traits
    requires(Extent > 0)
class Span
{
  public:
    using ElementType = T;
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = ElementType *;

    constexpr Span() : m_Data(nullptr)
    {
    }
    constexpr Span(ElementType *p_Data) : m_Data(p_Data)
    {
    }

    constexpr Span(const Array<ValueType, Extent, Traits> &p_Array) : m_Data(p_Array.GetData())
    {
    }
    constexpr Span(Array<ValueType, Extent, Traits> &p_Array) : m_Data(p_Array.GetData())
    {
    }

    template <typename U>
        requires(std::convertible_to<U *, T *> && std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<U, Extent, Traits> &p_Other) : m_Data(p_Other.GetData())
    {
    }

    constexpr ElementType *GetData() const
    {
        return m_Data;
    }
    constexpr SizeType GetSize() const
    {
        return Extent;
    }

    constexpr ElementType &operator[](const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT][SPAN] Index is out of bounds: {} >= {}", p_Index, Extent);
        return m_Data[p_Index];
    }
    constexpr ElementType &At(const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT][SPAN] Index is out of bounds: {} >= {}", p_Index, Extent);
        return m_Data[p_Index];
    }

    constexpr ElementType &GetFront() const
    {
        return At(0);
    }
    constexpr ElementType &GetBack() const
    {
        return At(Extent - 1);
    }

    constexpr Iterator begin() const
    {
        return m_Data;
    }
    constexpr Iterator end() const
    {
        return m_Data + Extent;
    }

    constexpr operator bool() const
    {
        return m_Data != nullptr;
    }

  private:
    ElementType *m_Data;
};

/**
 * @brief A view with a dynamic extent over a contiguous sequence of objects.
 *
 * It is meant to be used as a drop-in replacement for `std::span`, especially for personal use and to be able to
 * have a bit more control.
 *
 * @tparam T The type of the elements in the span.
 */
template <typename T, typename Traits> class Span<T, Limits<usize>::Max(), Traits>
{
  public:
    using ElementType = T;
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = ElementType *;

    constexpr Span() : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr Span(ElementType *p_Data, const SizeType p_Size) : m_Data(p_Data), m_Size(p_Size)
    {
    }

    template <SizeType Extent>
    constexpr Span(const Array<ValueType, Extent, Traits> &p_Array) : m_Data(p_Array.GetData()), m_Size(Extent)
    {
    }
    template <SizeType Extent>
    constexpr Span(Array<ValueType, Extent, Traits> &p_Array) : m_Data(p_Array.GetData()), m_Size(Extent)
    {
    }

    template <SizeType Capacity>
    constexpr Span(const StaticArray<ValueType, Capacity, Traits> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    template <SizeType Capacity>
    constexpr Span(StaticArray<ValueType, Capacity, Traits> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename U, SizeType Capacity>
        requires(std::convertible_to<U *, T *> && std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
    constexpr Span(const WeakArray<U, Capacity, Traits> &p_Array) : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    template <typename U, SizeType Capacity>
        requires(std::convertible_to<U *, T *> && std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
    constexpr Span(WeakArray<U, Capacity, Traits> &p_Array) : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    constexpr Span(const DynamicArray<ValueType, Traits> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    constexpr Span(DynamicArray<ValueType, Traits> &p_Array) : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename U, SizeType Extent>
        requires(std::convertible_to<U *, T *> && std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<U, Extent, Traits> &p_Other) : m_Data(p_Other.GetData()), m_Size(p_Other.GetSize())
    {
    }

    constexpr ElementType *GetData() const
    {
        return m_Data;
    }
    constexpr SizeType GetSize() const
    {
        return m_Size;
    }

    constexpr ElementType &operator[](const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT][SPAN] Index is out of bounds: {} >= {}", p_Index, m_Size);
        return m_Data[p_Index];
    }
    constexpr ElementType &At(const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT][SPAN] Index is out of bounds: {} >= {}", p_Index, m_Size);
        return m_Data[p_Index];
    }

    constexpr ElementType &GetFront() const
    {
        return At(0);
    }
    constexpr ElementType &GetBack() const
    {
        return At(m_Size - 1);
    }

    constexpr Iterator begin() const
    {
        return m_Data;
    }
    constexpr Iterator end() const
    {
        return m_Data + m_Size;
    }

    constexpr bool IsEmpty() const
    {
        return m_Size == 0;
    }

    constexpr operator bool() const
    {
        return m_Data != nullptr;
    }

  private:
    ElementType *m_Data;
    SizeType m_Size;
};
} // namespace TKit
