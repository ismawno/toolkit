#pragma once

#include "tkit/container/fixed_array.hpp"
#include "tkit/container/array.hpp"
#include "tkit/utils/limits.hpp"
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
        : Span(std::addressof(data))
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
    constexpr usz GetBytes() const
    {
        return Extent * sizeof(T);
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
    using ValueType = T;
    using ElementType = std::remove_cvref_t<T>;
    static constexpr bool IsString = std::is_same_v<ElementType, char>;

    constexpr Span() : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr Span(T &data) : Span(std::addressof(data), 1)
    {
    }
    constexpr Span(T &&data) = delete;

    constexpr Span(T *data, const usize size) : m_Data(data), m_Size(size)
    {
    }
    constexpr Span(T *data)
        requires(IsString)
        : m_Data(data), m_Size(std::char_traits<T>::length(data))
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
    constexpr Span(const Array<ElementType, AllocState> &array)
        : m_Data(array.GetData()), m_Size(addOneIfString(array.GetSize()))
    {
    }
    template <typename AllocState>
    constexpr Span(Array<ElementType, AllocState> &array)
        : m_Data(array.GetData()), m_Size(addOneIfString(array.GetSize()))
    {
    }

    template <typename ElementType, usize Extent>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<ElementType, Extent> &other)
        : m_Data(other.GetData()), m_Size(addOneIfString(other.GetSize()))
    {
    }

    constexpr T *GetData() const
    {
        return m_Data;
    }
    constexpr usize GetSize() const
    {
        return subOneIfString(m_Size);
    }
    constexpr usz GetBytes() const
    {
        return GetSize() * sizeof(T);
    }

    constexpr T &operator[](const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, GetSize(), "[TOOLKIT][SPAN] ");
        return m_Data[index];
    }
    constexpr T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, GetSize(), "[TOOLKIT][SPAN] ");
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
        return m_Data + GetSize();
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
    static constexpr usize addOneIfString(const usize size)
    {
        if constexpr (IsString)
            return size + 1;
        else
            return size;
    }
    static constexpr usize subOneIfString(const usize size)
    {
        if constexpr (IsString)
        {
            if (size != 0)
                return size - 1;
            return 0;
        }
        else
            return size;
    }
    T *m_Data;
    usize m_Size;
};

using StringView = Span<const char>;

} // namespace TKit
