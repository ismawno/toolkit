#pragma once

#include "tkit/container/alias.hpp"
#include "tkit/core/logging.hpp"

namespace TKit
{
/**
 * @brief A plain C array wrapper.
 *
 * Can be used as a drop-in replacement for `std::array`. It is here to provide a bit more control and use of the
 * `DefaultAllocator` traits.
 *
 * @tparam T The type of the elements.
 * @tparam Size The number of elements in the array.
 * @tparam Traits The allocator traits to use. By default, it uses the `DefaultAllocator` traits.
 */
template <typename T, usize Size, typename Traits = std::allocator_traits<Memory::DefaultAllocator<T>>> class Array
{
  public:
    using value_type = T;
    using size_type = typename Traits::size_type;
    using difference_type = typename Traits::difference_type;
    using pointer = typename Traits::pointer;
    using const_pointer = typename Traits::const_pointer;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr Array() noexcept = default;

    constexpr Array(const std::initializer_list<T> p_Elements) noexcept
    {
        TKIT_ASSERT(p_Elements.size() <= Size, "[TOOLKIT] Size is bigger than capacity");
        Memory::ConstructRangeCopy(begin(), p_Elements.begin(), p_Elements.end());
    }

    constexpr Array(const Array &p_Other) noexcept = default;
    constexpr Array(Array &&p_Other) noexcept = default;

    constexpr Array &operator=(const Array &p_Other) noexcept = default;
    constexpr Array &operator=(Array &&p_Other) noexcept = default;

    constexpr const_reference operator[](const size_type p_Index) const noexcept
    {
        return at(p_Index);
    }
    constexpr reference operator[](const size_type p_Index) noexcept
    {
        return at(p_Index);
    }

    constexpr const_reference at(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr reference at(const size_type p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr size_type size() const noexcept
    {
        return Size;
    }

    constexpr pointer data() noexcept
    {
        return m_Data;
    }
    constexpr const_pointer data() const noexcept
    {
        return m_Data;
    }

    constexpr const_reference front() const noexcept
    {
        return *begin();
    }
    constexpr const_reference back() const noexcept
    {
        return *(end() - 1);
    }

    constexpr reference front() noexcept
    {
        return *begin();
    }
    constexpr reference back() noexcept
    {
        return *(end() - 1);
    }

    constexpr iterator begin() noexcept
    {
        return m_Data;
    }
    constexpr iterator end() noexcept
    {
        return m_Data + Size;
    }

    constexpr const_iterator begin() const noexcept
    {
        return m_Data;
    }
    constexpr const_iterator end() const noexcept
    {
        return m_Data + Size;
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return m_Data;
    }
    constexpr const_iterator cend() const noexcept
    {
        return m_Data + Size;
    }

    constexpr reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }
    constexpr reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }
    constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

  private:
    T m_Data[Size];
};

template <typename T> using Array4 = Array<T, 4>;
template <typename T> using Array8 = Array<T, 8>;
template <typename T> using Array16 = Array<T, 16>;
template <typename T> using Array32 = Array<T, 32>;
template <typename T> using Array64 = Array<T, 64>;
template <typename T> using Array128 = Array<T, 128>;
template <typename T> using Array196 = Array<T, 196>;
template <typename T> using Array256 = Array<T, 256>;
template <typename T> using Array384 = Array<T, 384>;
template <typename T> using Array512 = Array<T, 512>;
template <typename T> using Array768 = Array<T, 768>;
template <typename T> using Array1024 = Array<T, 1024>;

} // namespace TKit