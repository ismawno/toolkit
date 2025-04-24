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
template <typename T, usize Extent = Limits<usize>::max(),
          typename Traits = Container::ArrayTraits<NoCVRef<T>>> // Consider adding allocator traits
    requires(Extent > 0)
class Span
{
  public:
    using ElementType = T;
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = ElementType *;

    constexpr Span() noexcept : m_Data(nullptr)
    {
    }
    constexpr explicit(false) Span(ElementType *p_Data) noexcept : m_Data(p_Data)
    {
    }

    constexpr explicit(false) Span(const Array<ValueType, Extent, Traits> &p_Array) noexcept : m_Data(p_Array.GetData())
    {
    }
    constexpr explicit(false) Span(Array<ValueType, Extent, Traits> &p_Array) noexcept : m_Data(p_Array.GetData())
    {
    }

    template <typename U>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr explicit(false) Span(const Span<U, Extent, Traits> &p_Other) noexcept : m_Data(p_Other.GetData())
    {
    }

    constexpr ElementType *GetData() const noexcept
    {
        return m_Data;
    }
    constexpr SizeType GetSize() const noexcept
    {
        return Extent;
    }

    constexpr ElementType &operator[](const SizeType p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }
    constexpr ElementType &At(const SizeType p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }

    constexpr ElementType &GetFront() const noexcept
    {
        return At(0);
    }
    constexpr ElementType &GetBack() const noexcept
    {
        return At(Extent - 1);
    }

    constexpr Iterator begin() const noexcept
    {
        return m_Data;
    }
    constexpr Iterator end() const noexcept
    {
        return m_Data + Extent;
    }

    constexpr operator bool() const noexcept
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
template <typename T, typename Traits> class Span<T, Limits<usize>::max(), Traits>
{
  public:
    using ElementType = T;
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = ElementType *;

    constexpr Span() noexcept : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr Span(ElementType *p_Data, const SizeType p_Size) noexcept : m_Data(p_Data), m_Size(p_Size)
    {
    }

    template <SizeType Extent>
    constexpr explicit(false) Span(const Array<ValueType, Extent, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(Extent)
    {
    }
    template <SizeType Extent>
    constexpr explicit(false) Span(Array<ValueType, Extent, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(Extent)
    {
    }

    template <SizeType Capacity>
    constexpr explicit(false) Span(const StaticArray<ValueType, Capacity, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    template <SizeType Capacity>
    constexpr explicit(false) Span(StaticArray<ValueType, Capacity, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename U, SizeType Capacity>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr explicit(false) Span(const WeakArray<U, Capacity, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    template <typename U, SizeType Capacity>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr explicit(false) Span(WeakArray<U, Capacity, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    constexpr explicit(false) Span(const DynamicArray<ValueType, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    constexpr explicit(false) Span(DynamicArray<ValueType, Traits> &p_Array) noexcept
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename U, SizeType Extent>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr explicit(false) Span(const Span<U, Extent, Traits> &p_Other) noexcept
        : m_Data(p_Other.GetData()), m_Size(p_Other.GetSize())
    {
    }

    constexpr ElementType *GetData() const noexcept
    {
        return m_Data;
    }
    constexpr SizeType GetSize() const noexcept
    {
        return m_Size;
    }

    constexpr ElementType &operator[](const SizeType p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }
    constexpr ElementType &At(const SizeType p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }

    constexpr ElementType &GetFront() const noexcept
    {
        return At(0);
    }
    constexpr ElementType &GetBack() const noexcept
    {
        return At(Extent - 1);
    }

    constexpr Iterator begin() const noexcept
    {
        return m_Data;
    }
    constexpr Iterator end() const noexcept
    {
        return m_Data + m_Size;
    }

    constexpr bool IsEmpty() const noexcept
    {
        return m_Size == 0;
    }

    constexpr operator bool() const noexcept
    {
        return m_Data != nullptr;
    }

  private:
    ElementType *m_Data;
    SizeType m_Size;
};
} // namespace TKit