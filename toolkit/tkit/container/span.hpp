#pragma once

#include "tkit/container/weak_array.hpp"
#include "tkit/container/array.hpp"

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
    constexpr Span(T &data)
        requires(Extent == 1)
        : Span(&data)
    {
    }
    constexpr Span(T &&data) = delete;
    constexpr Span(T *data) : m_Data(data)
    {
    }

    constexpr Span(const FixedArray<ElementType, Extent> &array) : m_Data(array.GetData())
    {
    }
    constexpr Span(FixedArray<ElementType, Extent> &array) : m_Data(array.GetData())
    {
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<ElementType, Extent> &other) : m_Data(other.GetData())
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

    constexpr T &operator[](const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, Extent, "[TOOLKIT][SPAN] ");
        return m_Data[index];
    }
    constexpr T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, Extent, "[TOOLKIT][SPAN] ");
        return m_Data[index];
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
    constexpr Span(T &data) : Span(&data, 1)
    {
    }
    constexpr Span(T &&data) = delete;

    constexpr Span(T *data, const usize size) : m_Data(data), m_Size(size)
    {
    }

    template <usize Extent>
    constexpr Span(const FixedArray<ElementType, Extent> &array) : m_Data(array.GetData()), m_Size(Extent)
    {
    }
    template <usize Extent>
    constexpr Span(FixedArray<ElementType, Extent> &array) : m_Data(array.GetData()), m_Size(Extent)
    {
    }

    template <typename AllocState>
    constexpr Span(const Array<ElementType, AllocState> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }
    template <typename AllocState>
    constexpr Span(Array<ElementType, AllocState> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }

    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const WeakArray<ElementType, Capacity> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }
    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(WeakArray<ElementType, Capacity> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }

    template <typename ElementType, usize Extent>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<ElementType, Extent> &other) : m_Data(other.GetData()), m_Size(other.GetSize())
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

    constexpr T &operator[](const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_Size, "[TOOLKIT][SPAN] ");
        return m_Data[index];
    }
    constexpr T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_Size, "[TOOLKIT][SPAN] ");
        return m_Data[index];
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
