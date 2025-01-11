#pragma once

#include "tkit/container/container.hpp"
#include "tkit/core/concepts.hpp"
#include "tkit/core/logging.hpp"
#include <span>
#include <array>

namespace TKit
{

// An STL-like array with a fixed size. It can be used as a replacement for std::array in
// the sense that it offers a bit more control and functionality, but it is not meant to be a drop-in replacement, as
// the extra functionality comes with some overhead.

/**
 * @brief An STL-like array with a fixed size buffer.
 *
 * It is meant to be used as a drop-in replacement for `std::array` when you need a bit more control and functionality,
 * although it comes with some overhead.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The capacity of the array.
 */
template <typename T, usize N, typename Traits = std::allocator_traits<Memory::DefaultAllocator<T>>>
    requires(N > 0)
class StaticArray
{
  public:
    using value_type = T;
    using size_type = typename Traits::size_type;
    using difference_type = typename Traits::difference_type;
    using pointer = typename Traits::pointer;
    using const_pointer = typename Traits::const_pointer;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    StaticArray() noexcept = default;

    template <typename... Args> StaticArray(const size_type p_Size, Args &&...p_Args) noexcept : m_Size(p_Size)
    {
        TKIT_ASSERT(p_Size <= N, "[TOOLKIT] Size is bigger than capacity");
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), std::forward<Args>(p_Args)...);
    }

    template <std::input_iterator It> StaticArray(const It p_Begin, const It p_End) noexcept
    {
        m_Size = static_cast<size_type>(std::distance(p_Begin, p_End));
        TKIT_ASSERT(m_Size <= N, "[TOOLKIT] Size is bigger than capacity");
        Memory::ConstructRangeCopy(begin(), p_Begin, p_End);
    }

    // This constructor WONT include the case M == N (ie, copy constructor)
    template <size_type M>
    explicit(false) StaticArray(const StaticArray<T, M> &p_Other) noexcept : m_Size(p_Other.size())
    {
        if constexpr (M > N)
        {
            TKIT_ASSERT(p_Other.size() <= N, "[TOOLKIT] Size is bigger than capacity");
        }
        Memory::ConstructRangeCopy(begin(), p_Other.begin(), p_Other.end());
    }

    // This constructor WONT include the case M == N (ie, move constructor)
    template <size_type M> explicit(false) StaticArray(StaticArray<T, M> &&p_Other) noexcept : m_Size(p_Other.size())
    {
        if constexpr (M > N)
        {
            TKIT_ASSERT(p_Other.size() <= N, "[TOOLKIT] Size is bigger than capacity");
        }
        Memory::ConstructRangeMove(begin(), p_Other.begin(), p_Other.end());
    }

    StaticArray(const StaticArray<T, N> &p_Other) noexcept : m_Size(p_Other.size())
    {
        Memory::ConstructRangeCopy(begin(), p_Other.begin(), p_Other.end());
    }

    StaticArray(StaticArray<T, N> &&p_Other) noexcept : m_Size(p_Other.size())
    {
        Memory::ConstructRangeMove(begin(), p_Other.begin(), p_Other.end());
    }

    StaticArray(const std::initializer_list<T> p_List) noexcept : m_Size(p_List.size())
    {
        TKIT_ASSERT(p_List.size() <= N, "[TOOLKIT] Size is bigger than capacity");
        Memory::ConstructRangeCopy(begin(), p_List.begin(), p_List.end());
    }

    ~StaticArray() noexcept
    {
        clear();
    }

    // Same goes for assignment. It wont include `M == N`, and use the default assignment operator
    template <size_type M> StaticArray &operator=(const StaticArray<T, M> &p_Other) noexcept
    {
        if constexpr (M == N)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > N)
        {
            TKIT_ASSERT(p_Other.size() <= N, "[TOOLKIT] Size is bigger than capacity");
        }
        clear();
        m_Size = p_Other.size();

        Memory::ConstructRangeCopy(begin(), p_Other.begin(), p_Other.end());
        return *this;
    }

    // Same goes for assignment. It wont include `M == N`, and use the default assignment operator
    template <size_type M> StaticArray &operator=(StaticArray<T, M> &&p_Other) noexcept
    {
        if constexpr (M == N)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > N)
        {
            TKIT_ASSERT(p_Other.size() <= N, "[TOOLKIT] Size is bigger than capacity");
        }
        clear();
        m_Size = p_Other.size();
        Memory::ConstructRangeMove(begin(), p_Other.begin(), p_Other.end());
        return *this;
    }

    StaticArray &operator=(const StaticArray<T, N> &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;
        clear();
        m_Size = p_Other.size();
        Memory::ConstructRangeCopy(begin(), p_Other.begin(), p_Other.end());
        return *this;
    }

    StaticArray &operator=(StaticArray<T, N> &&p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;
        clear();
        m_Size = p_Other.size();
        Memory::ConstructRangeMove(begin(), p_Other.begin(), p_Other.end());
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
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        Memory::Construct(begin() + m_Size++, std::forward<U>(p_Value));
    }

    /**
     * @brief Remove the last element from the array. The element is destroyed.
     *
     */
    void pop_back() noexcept
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
    void insert(const iterator p_Pos, U &&p_Value) noexcept
    {
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        Container<Traits>::Insert(end(), p_Pos, std::forward<U>(p_Value));
        ++m_Size;
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Begin The beginning of the range to insert.
     * @param p_End The end of the range to insert.
     */
    template <std::input_iterator It> void insert(const iterator p_Pos, It p_Begin, It p_End) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= N, "[TOOLKIT] New size exceeds capacity");

        m_Size += Container<Traits>::Insert(end(), p_Pos, p_Begin, p_End);
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    void insert(const iterator p_Pos, const std::initializer_list<T> p_Elements) noexcept
    {
        insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Erase the element at the specified position. The element is destroyed.
     *
     * @param p_Pos The position to erase the element at.
     */
    void erase(const iterator p_Pos) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT] Iterator is out of bounds");
        Container<Traits>::Erase(end(), p_Pos);
        --m_Size;
    }

    /**
     * @brief Erase a range of elements. The elements are destroyed.
     *
     * @param p_Begin The beginning of the range to erase.
     * @param p_End The end of the range to erase.
     */
    void erase(const iterator p_Begin, const iterator p_End) noexcept
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT] New size is negative");

        m_Size -= Container<Traits>::Erase(end(), p_Begin, p_End);
    }

    /**
     * @brief Emplace a new element at the end of the array. The element is constructed in place.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    T &emplace_back(Args &&...p_Args) noexcept
    {
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        return *Memory::Construct(begin() + m_Size++, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Get the first element in the array.
     *
     */
    const T &front() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *begin();
    }

    /**
     * @brief Get the first element in the array.
     *
     */
    T &front() noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *begin();
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
    void resize(const size_type p_Size, Args &&...args) noexcept
    {
        TKIT_ASSERT(p_Size <= N, "[TOOLKIT] Size is bigger than capacity");

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    /**
     * @brief Get the last element in the array.
     *
     */
    const T &back() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *(begin() + m_Size - 1);
    }

    /**
     * @brief Get the last element in the array.
     *
     */
    T &back() noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *(begin() + m_Size - 1);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return A reference to the element.
     */
    const T &operator[](const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return A reference to the element.
     */
    T &operator[](const size_type p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return A reference to the element.
     */
    const T &at(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Access an element in the array.
     *
     * @param p_Index The index of the element to access.
     * @return A reference to the element.
     */
    T &at(const size_type p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    /**
     * @brief Clear the array. All elements are destroyed.
     *
     */
    void clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    /**
     * @brief Get a pointer to the data buffer.
     *
     */
    const T *data() const noexcept
    {
        return reinterpret_cast<const_pointer>(&m_Data[0]);
    }

    /**
     * @brief Get a pointer to the data buffer.
     *
     */
    T *data() noexcept
    {
        return reinterpret_cast<pointer>(&m_Data[0]);
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
        return begin() + m_Size;
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
        return begin() + m_Size;
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
        return begin() + m_Size;
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
     * @brief Get the size of the array.
     *
     */
    size_type size() const noexcept
    {
        return m_Size;
    }

    /**
     * @brief Get the capacity of the underlying buffer.
     *
     */
    constexpr size_type capacity() const noexcept
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

    explicit(false) operator std::span<T, N>() noexcept
    {
        return std::span<T, N>(data(), size());
    }
    explicit(false) operator std::span<const T, N>() const noexcept
    {
        return std::span<const T, N>(data(), size());
    }

    explicit(false) operator std::span<T>() noexcept
    {
        return std::span<T>(data(), size());
    }
    explicit(false) operator std::span<const T>() const noexcept
    {
        return std::span<const T>(data(), size());
    }

  private:
    struct alignas(T) Element
    {
        std::byte Data[sizeof(T)];
    };

    static_assert(sizeof(Element) == sizeof(T), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(T), "Element alignment is not equal to T alignment");

    std::array<Element, N> m_Data{};
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