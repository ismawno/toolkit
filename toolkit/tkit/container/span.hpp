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
template <typename T, usize Extent = TKIT_USIZE_MAX> // Consider adding allocator traits
    requires(Extent > 0)
class Span
{
  public:
    using ValueType = T;
    using ElementType = std::remove_cvref_t<T>;

    constexpr Span() : m_Data(nullptr)
    {
    }
    constexpr Span(T &p_Data)
        requires(Extent == 1)
        : Span(&p_Data)
    {
    }
    constexpr Span(T &&p_Data) = delete;
    constexpr Span(T *p_Data) : m_Data(p_Data)
    {
    }

    constexpr Span(const FixedArray<ElementType, Extent> &p_Array) : m_Data(p_Array.GetData())
    {
    }
    constexpr Span(FixedArray<ElementType, Extent> &p_Array) : m_Data(p_Array.GetData())
    {
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<ElementType, Extent> &p_Other) : m_Data(p_Other.GetData())
    {
    }

    constexpr T *GetData() const
    {
        return m_Data;
    }
    constexpr usize GetSize() const
    {
        return Extent;
    }

    constexpr T &operator[](const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, Extent, "[TOOLKIT][SPAN] ");
        return m_Data[p_Index];
    }
    constexpr T &At(const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, Extent, "[TOOLKIT][SPAN] ");
        return m_Data[p_Index];
    }

    constexpr T &GetFront() const
    {
        return At(0);
    }
    constexpr T &GetBack() const
    {
        return At(Extent - 1);
    }

    constexpr T *begin() const
    {
        return m_Data;
    }
    constexpr T *end() const
    {
        return m_Data + Extent;
    }

    constexpr operator bool() const
    {
        return m_Data != nullptr;
    }

  private:
    T *m_Data;
};

/**
 * @brief A view with a dynamic extent over a contiguous sequence of objects.
 *
 * It is meant to be used as a drop-in replacement for `std::span`, especially for personal use and to be able to
 * have a bit more control.
 *
 * @tparam T The type of the elements in the span.
 */
template <typename T> class Span<T, TKIT_USIZE_MAX>
{
  public:
    using ElementType = std::remove_cvref_t<T>;

    constexpr Span() : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr Span(T &p_Data) : Span(&p_Data, 1)
    {
    }
    constexpr Span(T &&p_Data) = delete;

    constexpr Span(T *p_Data, const usize p_Size) : m_Data(p_Data), m_Size(p_Size)
    {
    }

    template <usize Extent>
    constexpr Span(const FixedArray<ElementType, Extent> &p_Array) : m_Data(p_Array.GetData()), m_Size(Extent)
    {
    }
    template <usize Extent>
    constexpr Span(FixedArray<ElementType, Extent> &p_Array) : m_Data(p_Array.GetData()), m_Size(Extent)
    {
    }

    template <usize Capacity>
    constexpr Span(const StaticArray<ElementType, Capacity> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    template <usize Capacity>
    constexpr Span(StaticArray<ElementType, Capacity> &p_Array) : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const WeakArray<ElementType, Capacity> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(WeakArray<ElementType, Capacity> &p_Array) : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    constexpr Span(const DynamicArray<ElementType> &p_Array) : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    constexpr Span(DynamicArray<ElementType> &p_Array) : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename ElementType, usize Extent>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<ElementType, Extent> &p_Other) : m_Data(p_Other.GetData()), m_Size(p_Other.GetSize())
    {
    }

    constexpr T *GetData() const
    {
        return m_Data;
    }
    constexpr usize GetSize() const
    {
        return m_Size;
    }

    constexpr T &operator[](const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][SPAN] ");
        return m_Data[p_Index];
    }
    constexpr T &At(const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][SPAN] ");
        return m_Data[p_Index];
    }

    constexpr T &GetFront() const
    {
        return At(0);
    }
    constexpr T &GetBack() const
    {
        return At(m_Size - 1);
    }

    constexpr T *begin() const
    {
        return m_Data;
    }
    constexpr T *end() const
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
    T *m_Data;
    usize m_Size;
};
} // namespace TKit
