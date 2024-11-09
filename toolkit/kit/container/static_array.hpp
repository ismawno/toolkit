#pragma once

#include "kit/core/alias.hpp"
#include "kit/core/logging.hpp"
#include "kit/core/concepts.hpp"
#include <array>

namespace KIT
{

// An STL-like array with a fixed size. It can be used as a replacement for std::array in
// the sense that it offers a bit more control and functionality, but it is not meant to be a drop-in replacement, as
// the extra functionality comes with some overhead.

/**
 * @brief An STL-like array with a fixed size buffer.
 *
 * It is meant to be used as a drop-in replacement for std::array when you need a bit more control and functionality,
 * although it comes with some overhead.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The capacity of the array.
 */
template <typename T, usize N>
    requires(N > 0)
class StaticArray
{
  public:
    // I figured that if I want to have a more STL-like interface, I should use the same naming conventions, although I
    // am not really sure when to "stop"

    using value_type = T;
    using size_type = usize;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    StaticArray() noexcept = default;

    template <typename... Args> StaticArray(const usize p_Size, Args &&...p_Args) noexcept : m_Size(p_Size)
    {
        KIT_ASSERT(p_Size <= N, "Size is bigger than capacity");
        for (auto it = begin(); it != end(); ++it)
            ::new (it) T(std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It> StaticArray(It p_Begin, It p_End) noexcept
    {
        for (auto it = p_Begin; it != p_End; ++it)
        {
            KIT_ASSERT(m_Size <= N, "Size is bigger than capacity");
            ::new (begin() + m_Size++) T(*it);
        }
    }

    // This constructor WONT include the case M == N (ie, copy constructor)
    template <usize M> explicit(false) StaticArray(const StaticArray<T, M> &p_Other) noexcept : m_Size(p_Other.size())
    {
        if constexpr (M > N)
        {
            KIT_ASSERT(p_Other.size() <= N, "Size is bigger than capacity");
        }
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
    }

    StaticArray(const StaticArray<T, N> &p_Other) noexcept : m_Size(p_Other.size())
    {
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
    }

    StaticArray(std::initializer_list<T> p_List) noexcept : m_Size(p_List.size())
    {
        KIT_ASSERT(p_List.size() <= N, "Size is bigger than capacity");
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(*(p_List.begin() + i));
    }

    ~StaticArray() noexcept
    {
        clear();
    }

    // Same goes for assignment. It wont include M == N, and use the default assignment operator
    template <usize M> StaticArray &operator=(const StaticArray<T, M> &p_Other) noexcept
    {
        if constexpr (M == N)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > N)
        {
            KIT_ASSERT(p_Other.size() <= N, "Size is bigger than capacity");
        }
        clear();
        m_Size = p_Other.size();
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
        return *this;
    }

    StaticArray &operator=(const StaticArray<T, N> &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;
        clear();
        m_Size = p_Other.size();
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
        return *this;
    }

    /**
     * @brief Insert a new element at the end of the array. The element is copied or moved into the array.
     *
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::convertible_to<U, T>)
    void push_back(U &&p_Value) noexcept
    {
        KIT_ASSERT(!full(), "Container is already full");
        ::new (begin() + m_Size++) T(std::forward<U>(p_Value));
    }

    /**
     * @brief Remove the last element from the array. The element is destroyed.
     *
     */
    void pop_back() noexcept
    {
        KIT_ASSERT(!empty(), "Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
            end()->~T();
    }

    /**
     * @brief Insert a new element at the specified position. The element is copied or moved into the array.
     *
     * @param p_Pos The position to insert the element at.
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::is_same_v<NoCVRef<T>, NoCVRef<U>>)
    void insert(const const_iterator p_Pos, U &&p_Value) noexcept
    {
        KIT_ASSERT(!full(), "Container is already full");
        KIT_ASSERT(p_Pos >= cbegin() && p_Pos <= cend(), "Iterator is out of bounds");
        if (empty() || p_Pos == end())
        {
            push_back(std::forward<U>(p_Value));
            return;
        }

        const difference_type offset = std::distance(cbegin(), p_Pos);
        const iterator pos = begin() + offset;

        // Current end() pointer is uninitialized, so it must be handled manually
        ::new (end()) T(std::move(*(end() - 1)));

        if (const iterator shiftedEnd = end() - 1; pos < shiftedEnd)
            std::copy_backward(std::make_move_iterator(pos), std::make_move_iterator(shiftedEnd), end());
        if constexpr (!std::is_trivially_destructible_v<T>)
            pos->~T();
        ::new (pos) T(std::forward<U>(p_Value));
        ++m_Size;
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Begin The beginning of the range to insert.
     * @param p_End The end of the range to insert.
     */
    template <std::input_iterator It> void insert(const const_iterator p_Pos, It p_Begin, It p_End) noexcept
    {
        KIT_ASSERT(p_Pos >= cbegin() && p_Pos <= cend(), "Iterator is out of bounds");
        if (empty() || p_Pos == end())
        {
            for (auto it = p_Begin; it != p_End; ++it)
                push_back(*it);
            return;
        }

        // This method is a bit verbose, but it has many edge cases to handle:
        // If the amount of elements to insert is small (ie, less than the trailing end), then the out of bound elements
        // must be copy-moved to the end of the array.

        // If the amount of elements to insert is bigger than the trailing end, then the trailing end must be copy-moved
        // to the end of the array, and the out of bound inserted elements must be copied as well to the remaining, left
        // portion of the out of bounds array.

        // The remaining elements inside the original array must then be shifted, in case the amount of elements to
        // insert was small

        const difference_type offset = std::distance(cbegin(), p_Pos);
        const iterator pos = begin() + offset;
        const usize count = std::distance(p_Begin, p_End);
        const usize outOfBounds = count < m_Size - offset ? count : m_Size - offset;
        KIT_ASSERT(m_Size + count <= capacity(), "New size exceeds capacity");

        // Current end() + outOfBounds pointers are uninitialized, so they must be handled manually
        for (usize i = 0; i < count; ++i)
        {
            const usize idx1 = count - i - 1;
            if (i < outOfBounds)
            {
                const usize idx2 = outOfBounds - i - 1;
                ::new (end() + idx1) T(std::move(*(pos + idx2)));
            }
            else
                ::new (end() + idx1) T(*(--p_End));
        }

        if (const iterator shiftedEnd = end() - count; pos < shiftedEnd)
            std::copy_backward(std::make_move_iterator(pos), std::make_move_iterator(shiftedEnd), end());
        for (usize i = 0; i < outOfBounds; ++i)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
                (pos + i)->~T();
            ::new (pos + i) T(*(p_Begin++));
        }
        m_Size += count;
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    void insert(const const_iterator p_Pos, std::initializer_list<T> p_Elements) noexcept
    {
        insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Erase the element at the specified position. The element is destroyed.
     *
     * @param p_Pos The position to erase the element at.
     */
    void erase(const const_iterator p_Pos) noexcept
    {
        if (empty())
            return;
        KIT_ASSERT(p_Pos >= cbegin() && p_Pos < cend(), "Iterator is out of bounds");
        const difference_type offset = std::distance(cbegin(), p_Pos);
        const iterator pos = begin() + offset;

        // Copy the elements after the erased one
        std::copy(std::make_move_iterator(pos + 1), std::make_move_iterator(end()), pos);

        // And destroy the last element
        if constexpr (!std::is_trivially_destructible_v<T>)
            (end() - 1)->~T();
        --m_Size;
    }

    /**
     * @brief Erase a range of elements. The elements are destroyed.
     *
     * @param p_Begin The beginning of the range to erase.
     * @param p_End The end of the range to erase.
     */
    void erase(const const_iterator p_Begin, const const_iterator p_End) noexcept
    {
        if (empty())
            return;
        KIT_ASSERT(p_Begin >= cbegin() && p_Begin <= cend(), "Begin iterator is out of bounds");
        KIT_ASSERT(p_End >= cbegin() && p_End <= cend(), "End iterator is out of bounds");
        KIT_ASSERT(p_Begin < p_End, "Begin iterator must come before end iterator");

        const difference_type offset = std::distance(cbegin(), p_Begin);
        const iterator it1 = begin() + offset;
        const usize count = std::distance(p_Begin, p_End);
        KIT_ASSERT(m_Size >= count, "New size is negative");

        // Copy the elements after the erased ones
        std::copy(std::make_move_iterator(it1 + count), std::make_move_iterator(end()), it1);

        // And destroy the last elements
        if constexpr (!std::is_trivially_destructible_v<T>)
            for (usize i = 0; i < count; ++i)
                (end() - i - 1)->~T();
        m_Size -= count;
    }

    /**
     * @brief Emplace a new element at the end of the array. The element is constructed in place.
     *
     * @param p_Args The arguments to pass to the constructor of T.
     * @return T& A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    T &emplace_back(Args &&...p_Args) noexcept
    {
        KIT_ASSERT(!full(), "Container is already full");
        ::new (end()) T(std::forward<Args>(p_Args)...);
        return *(begin() + m_Size++);
    }

    /**
     * @brief Get the first element in the array.
     *
     */
    const T &front() const noexcept
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *begin();
    }

    /**
     * @brief Get the first element in the array.
     *
     */
    T &front() noexcept
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *begin();
    }

    /**
     * @brief Resize the array. If the new size is smaller than the current size, the elements are destroyed. If the new
     * size is bigger than the current size, the elements are constructed in place.
     *
     * @param p_Size The new size of the array.
     * @param args The arguments to pass to the constructor of T (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void resize(const usize p_Size, Args &&...args) noexcept
    {
        KIT_ASSERT(p_Size <= capacity(), "Size is bigger than capacity");

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
            {
                auto itend = rbegin() + (m_Size - p_Size);
                for (auto it = rbegin(); it != itend; ++it)
                    it->~T();
            }

        if (p_Size > m_Size)
            for (usize i = m_Size; i < p_Size; ++i)
                ::new (begin() + i) T(std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    /**
     * @brief Get the last element in the array.
     *
     */
    const T &back() const noexcept
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *(begin() + m_Size - 1);
    }

    /**
     * @brief Get the last element in the array.
     *
     */
    T &back() noexcept
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *(begin() + m_Size - 1);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return A reference to the element.
     */
    const T &operator[](const usize p_Index) const noexcept
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return T& A reference to the element.
     */
    T &operator[](const usize p_Index) noexcept
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return A reference to the element.
     */
    const T &at(const usize p_Index) const noexcept
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return T& A reference to the element.
     */
    T &at(const usize p_Index) noexcept
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Clear the array. All elements are destroyed.
     *
     */
    void clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            for (auto it = begin(); it != end(); ++it)
                it->~T();
        m_Size = 0;
    }

    /**
     * @brief Get an iterator to the beginning of the array.
     *
     */
    iterator begin() noexcept
    {
        return reinterpret_cast<pointer>(&m_Data[0]);
    }

    /**
     * @brief Get an iterator to the end of the array.
     *
     */
    iterator end() noexcept
    {
        return data() + m_Size;
    }

    /**
     * @brief Get a const iterator to the beginning of the array.
     *
     */
    const_iterator begin() const noexcept
    {
        return reinterpret_cast<const_pointer>(&m_Data[0]);
    }

    /**
     * @brief Get a const iterator to the end of the array.
     *
     */
    const_iterator end() const noexcept
    {
        return data() + m_Size;
    }

    /**
     * @brief Get a const iterator to the beginning of the array.
     *
     */
    const_iterator cbegin() const noexcept
    {
        return data();
    }

    /**
     * @brief Get a const iterator to the end of the array.
     *
     */
    const_iterator cend() const noexcept
    {
        return data() + m_Size;
    }

    /**
     * @brief Get a reverse iterator to the beginning of the array.
     *
     */
    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    /**
     * @brief Get a reverse iterator to the end of the array.
     *
     */
    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    /**
     * @brief Get a const reverse iterator to the beginning of the array.
     *
     */
    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    /**
     * @brief Get a const reverse iterator to the end of the array.
     *
     */
    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    /**
     * @brief Get a const reverse iterator to the beginning of the array.
     *
     */
    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    /**
     * @brief Get a const reverse iterator to the end of the array.
     *
     */
    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    /**
     * @brief Get a pointer to the data buffer.
     *
     */
    const T *data() const noexcept
    {
        return begin();
    }

    /**
     * @brief Get a pointer to the data buffer.
     *
     */
    T *data() noexcept
    {
        return begin();
    }

    /**
     * @brief Get the size of the array.
     *
     */
    usize size() const noexcept
    {
        return m_Size;
    }

    /**
     * @brief Get the capacity of the underlying buffer.
     *
     */
    constexpr usize capacity() const noexcept
    {
        return N;
    }

    /**
     * @brief Check if the array is empty.
     *
     */
    bool empty() const noexcept
    {
        return m_Size == 0;
    }

    /**
     * @brief Check if the array is full.
     *
     */
    bool full() const noexcept
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

    std::array<Element, N> m_Data;
    usize m_Size = 0;
};
} // namespace KIT