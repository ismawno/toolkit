#pragma once

#include "kit/core/alias.hpp"
#include "kit/core/logging.hpp"
#include "kit/core/concepts.hpp"

namespace KIT
{
/**
 * @brief An STL-like array interface that manages a fixed size buffer of data. This base class does not own the data,
 * and it is up to the implementation to provide means to acquire/clean up the resources. This interface can be used as
 * a wrapper around a chunk of data to provide array-like functionality without owning the chunk itself. The
 * implementation is also responsible for providing the data() and capacity() methods.
 *
 * Using same naming conventions as the STL, to improve cnosistency.
 *
 * @tparam T The type of the elements in the array.
 * @tparam Derived The derived class that implements the data() and capacity() methods.
 */
template <typename T, typename Derived> class IArray
{
  public:
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

    constexpr IArray() noexcept = default;
    explicit constexpr IArray(const usize p_Size) noexcept : m_Size(p_Size)
    {
    }

    // Constructors that manage the derived's data buffer are not safe to implement, as that data may not have been
    // initialized properly yet (base class always goes first). In the 'protected' area, some constructors are provided
    // that can be used to initialize the data from the derived's end.

    // In the case of the assignment operators, the derived class should already be initialized, so it is safe to manage
    // the derived's data directly, but for consistency reasons, some methods are also provided in the protected area
    // for the derived to explicitly implement it.

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
     * @return const T& A reference to the element.
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
     * @return const T& A reference to the element.
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
        return data();
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
        return data();
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
        return static_cast<const Derived *>(this)->data();
    }

    /**
     * @brief Get a pointer to the data buffer.
     *
     */
    T *data() noexcept
    {
        return static_cast<Derived *>(this)->data();
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
        return static_cast<const Derived *>(this)->capacity();
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

  protected:
    // TODO: improve this
    //  All constructors that use m_Size without modifying it first need the IArray constructor called in the derived's
    //  class initializer list (quite confusing because some of them require it, some dont)

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void Constructor(Args &&...p_Args) noexcept
    {
        KIT_ASSERT(m_Size <= capacity(), "Size is bigger than capacity");
        for (auto it = begin(); it != end(); ++it)
            ::new (it) T(std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It> void Constructor(It p_Begin, It p_End) noexcept
    {
        for (auto it = p_Begin; it != p_End; ++it)
        {
            KIT_ASSERT(m_Size <= capacity(), "Size is bigger than capacity");
            ::new (begin() + m_Size++) T(*it);
        }
    }

    void Constructor(const Derived &p_Other) noexcept
    {
        KIT_ASSERT(m_Size <= capacity(), "Size is bigger than capacity");
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
    }

    void Constructor(std::initializer_list<T> p_List) noexcept
    {
        KIT_ASSERT(m_Size <= capacity(), "Size is bigger than capacity");
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(*(p_List.begin() + i));
    }

    void CopyAssignment(const Derived &p_Other) noexcept
    {
        if (this == &p_Other)
            return;
        clear();
        m_Size = p_Other.size();
        KIT_ASSERT(m_Size <= capacity(), "Size is bigger than capacity");
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
    }
    void CopyAssignment(std::initializer_list<T> p_List) noexcept
    {
        clear();
        for (auto it = p_List.begin(); it != p_List.end(); ++it)
        {
            KIT_ASSERT(m_Size <= capacity(), "Size is bigger than capacity");
            ::new (begin() + m_Size++) T(*it);
        }
    }

  private:
    usize m_Size = 0;
};
}; // namespace KIT