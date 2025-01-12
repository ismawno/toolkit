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
    using size_type = usize;
    using pointer = T *;
    using reference = T &;
    using iterator = pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using Traits = std::allocator_traits<Memory::DefaultAllocator<value_type>>;

    constexpr Span() noexcept : m_Data(nullptr)
    {
    }
    constexpr explicit(false) Span(pointer p_Data) noexcept : m_Data(p_Data)
    {
    }
    constexpr explicit(false) Span(const std::array<T, Extent> &p_Array) noexcept : m_Data(p_Array.data())
    {
    }

    constexpr Span(const Span &p_Other) noexcept = default;
    constexpr Span(Span &&p_Other) noexcept = default;

    constexpr Span &operator=(const Span &p_Other) noexcept = default;
    constexpr Span &operator=(Span &&p_Other) noexcept = default;

    constexpr pointer data() const noexcept
    {
        return m_Data;
    }
    constexpr size_type size() const noexcept
    {
        return Extent;
    }

    constexpr reference operator[](const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }
    constexpr reference at(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < Extent, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }

    constexpr reference front() const noexcept
    {
        return *begin();
    }
    constexpr reference back() const noexcept
    {
        return *(begin() + Extent - 1);
    }

    constexpr iterator begin() const noexcept
    {
        return m_Data;
    }
    constexpr iterator end() const noexcept
    {
        return m_Data + Extent;
    }

    constexpr reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(end());
    }
    constexpr reverse_iterator rend() const noexcept
    {
        return reverse_iterator(begin());
    }

    explicit(false) constexpr operator std::span<T, Extent>() noexcept
    {
        return std::span<T, Extent>(data(), static_cast<size_t>(size()));
    }
    explicit(false) constexpr operator std::span<const T, Extent>() const noexcept
    {
        return std::span<const T, Extent>(data(), static_cast<size_t>(size()));
    }

  private:
    pointer m_Data;
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
    using size_type = usize;
    using pointer = T *;
    using reference = T &;
    using iterator = pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using Traits = std::allocator_traits<Memory::DefaultAllocator<value_type>>;

    constexpr Span() noexcept : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr Span(pointer p_Data, const size_type p_Size) noexcept : m_Data(p_Data), m_Size(p_Size)
    {
    }

    template <size_type Capacity>
    explicit(false) Span(const StaticArray<T, Capacity> &p_Array) noexcept
        : m_Data(p_Array.data()), m_Size(p_Array.size())
    {
    }
    template <size_type Capacity>
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

    constexpr pointer data() const noexcept
    {
        return m_Data;
    }
    constexpr size_type size() const noexcept
    {
        return m_Size;
    }

    constexpr reference operator[](const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }
    constexpr reference at(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return m_Data[p_Index];
    }

    constexpr reference front() const noexcept
    {
        return *begin();
    }
    constexpr reference back() const noexcept
    {
        return *(begin() + m_Size - 1);
    }

    constexpr iterator begin() const noexcept
    {
        return m_Data;
    }
    constexpr iterator end() const noexcept
    {
        return m_Data + m_Size;
    }

    constexpr reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(end());
    }
    constexpr reverse_iterator rend() const noexcept
    {
        return reverse_iterator(begin());
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
    pointer m_Data;
    size_type m_Size;
};
} // namespace TKit