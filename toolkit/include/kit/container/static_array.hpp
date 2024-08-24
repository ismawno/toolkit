#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include "kit/core/logging.hpp"
#include <array>

KIT_NAMESPACE_BEGIN

template <typename T, typename U>
concept ShallowIsSame = std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

// An STL-like array with a fixed size. It can be used as a replacement for std::array in
// the sense that it offers a bit more control and functionality, but it is not meant to be a drop-in replacement, as
// the extra functionality comes with some overhead.
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

    StaticArray() KIT_NOEXCEPT = default;

    template <typename... Args> StaticArray(const usize p_Size, Args &&...p_Args) KIT_NOEXCEPT : m_Size(p_Size)
    {
        KIT_ASSERT(p_Size <= N, "Size is bigger than capacity");
        for (auto it = begin(); it != end(); ++it)
            ::new (it) T(std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It> StaticArray(It p_Begin, It p_End) KIT_NOEXCEPT
    {
        for (auto it = p_Begin; it != p_End; ++it)
        {
            KIT_ASSERT(m_Size <= N, "Size is bigger than capacity");
            ::new (begin() + m_Size++) T(*it);
        }
    }

    // This constructor WONT include the case M == N (ie, copy constructor)
    template <usize M>
    explicit(false) StaticArray(const StaticArray<T, M> &p_Other) KIT_NOEXCEPT : m_Size(p_Other.size())
    {
        if constexpr (M > N)
        {
            KIT_ASSERT(p_Other.size() <= N, "Size is bigger than capacity");
        }
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
    }

    StaticArray(const StaticArray<T, N> &p_Other) KIT_NOEXCEPT : m_Size(p_Other.size())
    {
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
    }

    StaticArray(std::initializer_list<T> p_List) KIT_NOEXCEPT : m_Size(p_List.size())
    {
        KIT_ASSERT(p_List.size() <= N, "Size is bigger than capacity");
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(*(p_List.begin() + i));
    }

    ~StaticArray() KIT_NOEXCEPT
    {
        clear();
    }

    // Same goes for assignment. It wont include M == N, and use the default assignment operator
    template <usize M> StaticArray &operator=(const StaticArray<T, M> &p_Other) KIT_NOEXCEPT
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

    StaticArray &operator=(const StaticArray<T, N> &p_Other) KIT_NOEXCEPT
    {
        if (this == &p_Other)
            return *this;
        clear();
        m_Size = p_Other.size();
        for (usize i = 0; i < m_Size; ++i)
            ::new (begin() + i) T(p_Other[i]);
        return *this;
    }

    // Add additional template to allow perfect forwarding
    template <typename U>
        requires(ShallowIsSame<T, U>)
    void push_back(U &&p_Value) KIT_NOEXCEPT
    {
        KIT_ASSERT(!full(), "Container is already full");
        ::new (begin() + m_Size++) T(std::forward<U>(p_Value));
    }

    void pop_back() KIT_NOEXCEPT
    {
        KIT_ASSERT(!empty(), "Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
            end()->~T();
    }

    // Add additional template to allow perfect forwarding
    template <typename U>
        requires(ShallowIsSame<T, U>)
    void insert(const const_iterator p_Pos, U &&p_Value) KIT_NOEXCEPT
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

    template <std::input_iterator It> void insert(const const_iterator p_Pos, It p_Begin, It p_End) KIT_NOEXCEPT
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
        KIT_ASSERT(m_Size + count <= N, "New size exceeds capacity");

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

    void insert(const const_iterator p_Pos, std::initializer_list<T> p_Elements) KIT_NOEXCEPT
    {
        insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    void erase(const const_iterator p_Pos) KIT_NOEXCEPT
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

    void erase(const const_iterator p_Begin, const const_iterator p_End) KIT_NOEXCEPT
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

    template <typename... Args> T &emplace_back(Args &&...p_Args) KIT_NOEXCEPT
    {
        KIT_ASSERT(!full(), "Container is already full");
        ::new (end()) T(std::forward<Args>(p_Args)...);
        return *(begin() + m_Size++);
    }

    const T &front() const KIT_NOEXCEPT
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *begin();
    }
    T &front() KIT_NOEXCEPT
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *begin();
    }

    template <typename... Args> void resize(const usize p_Size, Args &&...args) KIT_NOEXCEPT
    {
        KIT_ASSERT(p_Size <= N, "Size is bigger than capacity");

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

    const T &back() const KIT_NOEXCEPT
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *(begin() + m_Size - 1);
    }
    T &back() KIT_NOEXCEPT
    {
        KIT_ASSERT(!empty(), "Container is empty");
        return *(begin() + m_Size - 1);
    }

    const T &operator[](const usize p_Index) const KIT_NOEXCEPT
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }
    T &operator[](const usize p_Index) KIT_NOEXCEPT
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }

    const T &at(const usize p_Index) const KIT_NOEXCEPT
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }
    T &at(const usize p_Index) KIT_NOEXCEPT
    {
        KIT_ASSERT(p_Index < m_Size, "Index is out of bounds");
        return *(begin() + p_Index);
    }

    T *data() KIT_NOEXCEPT
    {
        return begin();
    }
    const T *data() const KIT_NOEXCEPT
    {
        return begin();
    }

    void clear() KIT_NOEXCEPT
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            for (auto it = begin(); it != end(); ++it)
                it->~T();
        m_Size = 0;
    }

    // Keep stl convention?
    constexpr usize capacity() const KIT_NOEXCEPT
    {
        return N;
    }
    usize size() const KIT_NOEXCEPT
    {
        return m_Size;
    }
    bool empty() const KIT_NOEXCEPT
    {
        return m_Size == 0;
    }
    bool full() const KIT_NOEXCEPT
    {
        return m_Size == N;
    }

    iterator begin() KIT_NOEXCEPT
    {
        return reinterpret_cast<pointer>(&m_Data[0]);
    }
    iterator end() KIT_NOEXCEPT
    {
        return reinterpret_cast<pointer>(&m_Data[0]) + m_Size;
    }

    const_iterator begin() const KIT_NOEXCEPT
    {
        return reinterpret_cast<const_pointer>(&m_Data[0]);
    }
    const_iterator end() const KIT_NOEXCEPT
    {
        return reinterpret_cast<const_pointer>(&m_Data[0]) + m_Size;
    }

    const_iterator cbegin() const KIT_NOEXCEPT
    {
        return reinterpret_cast<const_pointer>(&m_Data[0]);
    }
    const_iterator cend() const KIT_NOEXCEPT
    {
        return reinterpret_cast<const_pointer>(&m_Data[0]) + m_Size;
    }

    reverse_iterator rbegin() KIT_NOEXCEPT
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend() KIT_NOEXCEPT
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const KIT_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const KIT_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const KIT_NOEXCEPT
    {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const KIT_NOEXCEPT
    {
        return const_reverse_iterator(cbegin());
    }

  private:
    struct alignas(T) Element
    {
        std::byte m_Data[sizeof(T)];
    };

    static_assert(sizeof(Element) == sizeof(T), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(T), "Element alignment is not equal to T alignment");

    std::array<Element, N> m_Data{};
    usize m_Size = 0;
};

KIT_NAMESPACE_END