#pragma once

#include "tkit/container/container.hpp"
#include "tkit/container/array.hpp"
#include "tkit/utils/concepts.hpp"
#include "tkit/utils/logging.hpp"

namespace TKit
{

/**
 * @brief An STL-like array with a fixed size buffer.
 *
 * It is meant to be used as a drop-in replacement for `Array` when you need a bit more control and functionality,
 * although it comes with some overhead.
 *
 * @tparam T The type of the elements in the array.
 * @tparam Capacity The capacity of the array.
 */
template <typename T, usize Capacity, typename Traits = std::allocator_traits<Memory::DefaultAllocator<T>>>
    requires(Capacity > 0)
class StaticArray
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

    constexpr StaticArray() noexcept = default;

    template <typename... Args>
    constexpr StaticArray(const size_type p_Size, Args &&...p_Args) noexcept : m_Size(p_Size)
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It> constexpr StaticArray(const It p_Begin, const It p_End) noexcept
    {
        m_Size = static_cast<size_type>(std::distance(p_Begin, p_End));
        TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        Memory::ConstructRangeCopy(begin(), p_Begin, p_End);
    }

    // This constructor WONT include the case M == Capacity (ie, copy constructor)
    template <size_type M>
    explicit(false) constexpr StaticArray(const StaticArray<T, M> &p_Other) noexcept : m_Size(p_Other.size())
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.size() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        Memory::ConstructRangeCopy(begin(), p_Other.begin(), p_Other.end());
    }

    // This constructor WONT include the case M == Capacity (ie, move constructor)
    template <size_type M>
    explicit(false) constexpr StaticArray(StaticArray<T, M> &&p_Other) noexcept : m_Size(p_Other.size())
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.size() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        Memory::ConstructRangeMove(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(const StaticArray<T, Capacity> &p_Other) noexcept : m_Size(p_Other.size())
    {
        Memory::ConstructRangeCopy(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(StaticArray<T, Capacity> &&p_Other) noexcept : m_Size(p_Other.size())
    {
        Memory::ConstructRangeMove(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(const std::initializer_list<T> p_List) noexcept
        : m_Size(static_cast<size_type>(p_List.size()))
    {
        TKIT_ASSERT(p_List.size() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        Memory::ConstructRangeCopy(begin(), p_List.begin(), p_List.end());
    }

    ~StaticArray() noexcept
    {
        clear();
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <size_type M> constexpr StaticArray &operator=(const StaticArray<T, M> &p_Other) noexcept
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.size() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        const usize otherSize = p_Other.size();
        const usize minSize = m_Size < otherSize ? m_Size : otherSize;
        for (usize i = 0; i < minSize; ++i)
            at(i) = p_Other[i];

        if (otherSize > m_Size)
            Memory::ConstructRangeCopy(end(), p_Other.begin() + m_Size, p_Other.end());
        else if (otherSize < m_Size)
            Memory::DestructRange(begin() + otherSize, end());

        m_Size = otherSize;
        return *this;
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <size_type M> constexpr StaticArray &operator=(StaticArray<T, M> &&p_Other) noexcept
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.size() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        const usize otherSize = p_Other.size();
        const usize minSize = m_Size < otherSize ? m_Size : otherSize;
        for (usize i = 0; i < minSize; ++i)
            at(i) = std::move(p_Other[i]);

        if (otherSize > m_Size)
            Memory::ConstructRangeMove(end(), p_Other.begin() + m_Size, p_Other.end());
        else if (otherSize < m_Size)
            Memory::DestructRange(begin() + otherSize, end());

        m_Size = otherSize;
        return *this;
    }

    constexpr StaticArray &operator=(const StaticArray<T, Capacity> &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;
        const usize otherSize = p_Other.size();
        const usize minSize = m_Size < otherSize ? m_Size : otherSize;
        for (usize i = 0; i < minSize; ++i)
            at(i) = p_Other[i];

        if (otherSize > m_Size)
            Memory::ConstructRangeCopy(end(), p_Other.begin() + m_Size, p_Other.end());
        else if (otherSize < m_Size)
            Memory::DestructRange(begin() + otherSize, end());

        m_Size = otherSize;
        return *this;
    }

    constexpr StaticArray &operator=(StaticArray<T, Capacity> &&p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;
        const usize otherSize = p_Other.size();
        const usize minSize = m_Size < otherSize ? m_Size : otherSize;
        for (usize i = 0; i < minSize; ++i)
            at(i) = std::move(p_Other[i]);

        if (otherSize > m_Size)
            Memory::ConstructRangeMove(end(), p_Other.begin() + m_Size, p_Other.end());
        else if (otherSize < m_Size)
            Memory::DestructRange(begin() + otherSize, end());

        m_Size = otherSize;
        return *this;
    }

    /**
     * @brief Insert a new element at the end of the array. The element is copied or moved into the array.
     *
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::convertible_to<U, T>)
    constexpr void push_back(U &&p_Value) noexcept
    {
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        Memory::Construct(begin() + m_Size++, std::forward<U>(p_Value));
    }

    /**
     * @brief Remove the last element from the array. The element is destroyed.
     *
     */
    constexpr void pop_back() noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::Destruct(end());
    }

    /**
     * @brief Insert a new element at the specified position. The element is copied or moved into the array.
     *
     * @param p_Pos The position to insert the element at.
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::is_convertible_v<NoCVRef<T>, NoCVRef<U>>)
    constexpr void insert(const iterator p_Pos, U &&p_Value) noexcept
    {
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        Detail::Container<Traits>::Insert(end(), p_Pos, std::forward<U>(p_Value));
        ++m_Size;
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Begin The beginning of the range to insert.
     * @param p_End The end of the range to insert.
     */
    template <std::input_iterator It> constexpr void insert(const iterator p_Pos, It p_Begin, It p_End) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= Capacity, "[TOOLKIT] New size exceeds capacity");

        m_Size += Detail::Container<Traits>::Insert(end(), p_Pos, p_Begin, p_End);
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    constexpr void insert(const iterator p_Pos, const std::initializer_list<T> p_Elements) noexcept
    {
        insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Erase the element at the specified position. The element is destroyed.
     *
     * @param p_Pos The position to erase the element at.
     */
    constexpr void erase(const iterator p_Pos) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT] Iterator is out of bounds");
        Detail::Container<Traits>::Erase(end(), p_Pos);
        --m_Size;
    }

    /**
     * @brief Erase a range of elements. The elements are destroyed.
     *
     * @param p_Begin The beginning of the range to erase.
     * @param p_End The end of the range to erase.
     */
    constexpr void erase(const iterator p_Begin, const iterator p_End) noexcept
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT] New size is negative");

        m_Size -= Detail::Container<Traits>::Erase(end(), p_Begin, p_End);
    }

    /**
     * @brief Emplace a new element at the end of the array. The element is constructed in place.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr reference emplace_back(Args &&...p_Args) noexcept
    {
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        return *Memory::Construct(begin() + m_Size++, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Resize the array. If the new size is smaller than the current size, the elements are destroyed. If the new
     * size is bigger than the current size, the elements are constructed in place.
     *
     * @param p_Size The new size of the array.
     * @param args The arguments to pass to the constructor of `T` (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr void resize(const size_type p_Size, Args &&...args) noexcept
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT] Size is bigger than capacity");

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    constexpr const_reference front() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *begin();
    }

    constexpr reference front() noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *begin();
    }

    constexpr const_reference back() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *(begin() + m_Size - 1);
    }

    constexpr reference back() noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *(begin() + m_Size - 1);
    }
    constexpr const_reference operator[](const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr reference operator[](const size_type p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr const_reference at(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr reference at(const size_type p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr void clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr const_pointer data() const noexcept
    {
        return reinterpret_cast<const_pointer>(&m_Data[0]);
    }

    constexpr pointer data() noexcept
    {
        return reinterpret_cast<pointer>(&m_Data[0]);
    }

    constexpr iterator begin() noexcept
    {
        return data();
    }

    constexpr iterator end() noexcept
    {
        return begin() + m_Size;
    }

    constexpr const_iterator begin() const noexcept
    {
        return data();
    }

    constexpr const_iterator end() const noexcept
    {
        return begin() + m_Size;
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return data();
    }

    constexpr const_iterator cend() const noexcept
    {
        return begin() + m_Size;
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

    constexpr size_type size() const noexcept
    {
        return m_Size;
    }

    constexpr size_type capacity() const noexcept
    {
        return Capacity;
    }

    constexpr bool empty() const noexcept
    {
        return m_Size == 0;
    }

    constexpr bool full() const noexcept
    {
        return m_Size == capacity();
    }

  private:
    struct alignas(T) Element
    {
        std::byte Data[sizeof(T)];
    };

    static_assert(sizeof(Element) == sizeof(T), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(T), "Element alignment is not equal to T alignment");

    Array<Element, Capacity> m_Data{};
    size_type m_Size = 0;
};

template <typename T> using StaticArray4 = StaticArray<T, 4>;
template <typename T> using StaticArray8 = StaticArray<T, 8>;
template <typename T> using StaticArray16 = StaticArray<T, 16>;
template <typename T> using StaticArray32 = StaticArray<T, 32>;
template <typename T> using StaticArray64 = StaticArray<T, 64>;
template <typename T> using StaticArray128 = StaticArray<T, 128>;
template <typename T> using StaticArray196 = StaticArray<T, 196>;
template <typename T> using StaticArray256 = StaticArray<T, 256>;
template <typename T> using StaticArray384 = StaticArray<T, 384>;
template <typename T> using StaticArray512 = StaticArray<T, 512>;
template <typename T> using StaticArray768 = StaticArray<T, 768>;
template <typename T> using StaticArray1024 = StaticArray<T, 1024>;
} // namespace TKit