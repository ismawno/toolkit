#pragma once

#include "tkit/container/weak_array.hpp"

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
template <typename T, usize Extent = Limits<usize>::max()> // Consider adding allocator traits
    requires(Extent > 0)
class Span
{
  public:
    using element_type = T;
    using value_type = NoCVRef<T>;

    constexpr Span() noexcept : m_Data(nullptr)
    {
    }
    constexpr explicit(false) Span(T *p_Data) noexcept : m_Data(p_Data)
    {
    }
    constexpr explicit(false) Span(const std::array<T, Extent> &p_Array) noexcept : m_Data(p_Array.data())
    {
    }

    constexpr Span(const Span &p_Other) noexcept = default;
    constexpr Span(Span &&p_Other) noexcept = default;

    constexpr Span &operator=(const Span &p_Other) noexcept = default;
    constexpr Span &operator=(Span &&p_Other) noexcept = default;

    constexpr T *data() const noexcept
    {
        return m_Data;
    }
    constexpr usize size() const noexcept
    {
        return Extent;
    }

    constexpr T &operator[](const usize p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }
    constexpr T &at(const usize p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }

    constexpr T *begin() const noexcept
    {
        return m_Data;
    }
    constexpr T *end() const noexcept
    {
        return m_Data + Extent;
    }

    explicit(false) constexpr operator std::span<T, Capacity>() noexcept
    {
        return std::span<T, Capacity>(data(), static_cast<size_t>(size()));
    }
    explicit(false) constexpr operator std::span<const T, Capacity>() const noexcept
    {
        return std::span<const T, Capacity>(data(), static_cast<size_t>(size()));
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
template <typename T> class Span<T, Limits<usize>::max()>
{
  public:
    using element_type = T;
    using value_type = NoCVRef<T>;

    constexpr Span() noexcept : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr Span(T *p_Data, const usize p_Size) noexcept : m_Data(p_Data), m_Size(p_Size)
    {
    }

    template <usize Capacity>
    explicit(false) Span(const StaticArray<T, Capacity> &p_Array) noexcept
        : m_Data(p_Array.data()), m_Size(p_Array.size())
    {
    }
    template <usize Capacity>
    explicit(false) Span(const WeakArray<T, Capacity> &p_Array) noexcept
        : m_Data(p_Array.data()), m_Size(p_Array.size())
    {
    }
    explicit(false) Span(const DynamicArray<T> &p_Array) noexcept : m_Data(p_Array.data()), m_Size(p_Array.size())
    {
    }

    constexpr Span(const Span &p_Other) noexcept = default;
    constexpr Span(Span &&p_Other) noexcept = default;

    constexpr Span &operator=(const Span &p_Other) noexcept = default;
    constexpr Span &operator=(Span &&p_Other) noexcept = default;

    constexpr T *data() const noexcept
    {
        return m_Data;
    }
    constexpr usize size() const noexcept
    {
        return m_Size;
    }

    constexpr T &operator[](const usize p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }
    constexpr T &at(const usize p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }

    constexpr T &front() const noexcept
    {
        return *begin();
    }
    constexpr T &back() const noexcept
    {
        return *(begin() + m_Size - 1);
    }

    constexpr T *begin() const noexcept
    {
        return m_Data;
    }
    constexpr T *end() const noexcept
    {
        return m_Data + m_Size;
    }

    constexpr bool empty() const noexcept
    {
        return m_Size == 0;
    }

    explicit(false) constexpr operator std::span<T>() noexcept
    {
        return std::span<T>(data(), static_cast<size_t>(size()));
    }
    explicit(false) constexpr operator std::span<const T>() const noexcept
    {
        return std::span<const T>(data(), static_cast<size_t>(size()));
    }

  private:
    T *m_Data;
    usize m_Size;
};
} // namespace TKit