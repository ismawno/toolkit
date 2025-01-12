#pragma once

#include "tkit/container/static_array.hpp"
#include "tkit/core/non_copyable.hpp"

namespace TKit
{
/**
 * @brief A modifiable view with a static capacity that does not own the elements it references.
 *
 * It otherwise behaves like a `StaticArray`. It cannot be copied.
 *
 * @tparam T The type of the elements in the array.
 * @tparam Capacity The capacity of the array.
 */
template <typename T, usize Capacity = Limits<usize>::max()> // Consider adding allocator traits
    requires(Capacity > 0)
class WeakArray
{
    TKIT_NON_COPYABLE(WeakArray);

  public:
    using element_type = T;
    using value_type = NoCVRef<T>;

    constexpr WeakArray() noexcept : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr explicit(false) WeakArray(T *p_Data) noexcept : m_Data(p_Data), m_Size(0)
    {
    }
    constexpr WeakArray(T *p_Data, const usize p_Size) noexcept : m_Data(p_Data), m_Size(p_Size)
    {
    }
    constexpr explicit(false) WeakArray(const std::array<T, Capacity> &p_Array) noexcept
        : m_Data(p_Array.data()), m_Size(0)
    {
    }

    constexpr WeakArray(WeakArray &&p_Other) noexcept : m_Data(p_Other.m_Data), m_Size(p_Other.m_Size)
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
    }
    constexpr WeakArray &operator=(WeakArray &&p_Other) noexcept
    {
        if (this != &p_Other)
        {
            m_Data = p_Other.m_Data;
            m_Size = p_Other.m_Size;

            p_Other.m_Data = nullptr;
            p_Other.m_Size = 0;
        }
        return *this;
    }

    /**
     * @brief Insert a new element at the end of the array. The element is copied or moved into the array.
     *
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::convertible_to<U, T>)
    constexpr void push_back(U &&p_Value) const noexcept
    {
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        Memory::Construct(begin() + m_Size++, std::forward<U>(p_Value));
    }

    /**
     * @brief Remove the last element from the array. The element is destroyed.
     *
     */
    constexpr void pop_back() const noexcept
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
    constexpr void insert(const iterator p_Pos, U &&p_Value) const noexcept
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
    template <std::input_iterator It> constexpr void insert(const iterator p_Pos, It p_Begin, It p_End) const noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= Capacity, "[TOOLKIT] New size exceeds capacity");

        m_Size += Container<Traits>::Insert(end(), p_Pos, p_Begin, p_End);
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    constexpr void insert(const iterator p_Pos, const std::initializer_list<T> p_Elements) const noexcept
    {
        insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Erase the element at the specified position. The element is destroyed.
     *
     * @param p_Pos The position to erase the element at.
     */
    constexpr void erase(const iterator p_Pos) const noexcept
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
    constexpr void erase(const iterator p_Begin, const iterator p_End) const noexcept
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
    constexpr T &emplace_back(Args &&...p_Args) const noexcept
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
    constexpr void resize(const size_type p_Size, Args &&...args) const noexcept
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

    constexpr T &front() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *begin();
    }
    constexpr T &back() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *(begin() + m_Size - 1);
    }

    constexpr T &operator[](const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr T &at(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr void clear() const noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr T *data() const noexcept
    {
        return m_Data;
    }

    constexpr T *begin() const noexcept
    {
        return m_Data;
    }

    constexpr T *end() const noexcept
    {
        return m_Data + m_Size;
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

    explicit(false) constexpr operator std::span<T>() const noexcept
    {
        return std::span<T>(data(), static_cast<size_t>(size()));
    }

  private:
    T *m_Data;
    usize m_Size;
};

/**
 * @brief A modifiable view with a dynamic capacity that does not own the elements it references.
 *
 * It otherwise behaves like a `StaticArray`. It cannot be copied.
 *
 * @tparam T The type of the elements in the array.
 */
template <typename T> class WeakArray<T, Limits<usize>::max()>
{
  public:
    using element_type = T;
    using value_type = NoCVRef<T>;

    constexpr WeakArray() noexcept : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr WeakArray(T *p_Data, const usize p_Capacity) noexcept : m_Data(p_Data), m_Size(0), m_Capacity(p_Capacity)
    {
    }
    constexpr WeakArray(T *p_Data, const usize p_Size, const usize p_Capacity) noexcept
        : m_Data(p_Data), m_Size(p_Size), m_Capacity(p_Capacity)
    {
    }

    template <usize N>
    explicit(false) WeakArray(const StaticArray<T, N> &p_Array) noexcept
        : m_Data(p_Array.data()), m_Size(p_Array.size())
    {
    }
    explicit(false) WeakArray(const DynamicArray<T> &p_Array) noexcept : m_Data(p_Array.data()), m_Size(p_Array.size())
    {
    }

    constexpr WeakArray(WeakArray &&p_Other) noexcept : m_Data(p_Other.m_Data), m_Size(p_Other.m_Size)
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
    }
    constexpr WeakArray &operator=(WeakArray &&p_Other) noexcept
    {
        if (this != &p_Other)
        {
            m_Data = p_Other.m_Data;
            m_Size = p_Other.m_Size;

            p_Other.m_Data = nullptr;
            p_Other.m_Size = 0;
        }
        return *this;
    }

    /**
     * @brief Insert a new element at the end of the array. The element is copied or moved into the array.
     *
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::convertible_to<U, T>)
    constexpr void push_back(U &&p_Value) const noexcept
    {
        TKIT_ASSERT(!full(), "[TOOLKIT] Container is already full");
        Memory::Construct(begin() + m_Size++, std::forward<U>(p_Value));
    }

    /**
     * @brief Remove the last element from the array. The element is destroyed.
     *
     */
    constexpr void pop_back() const noexcept
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
    constexpr void insert(const iterator p_Pos, U &&p_Value) const noexcept
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
    template <std::input_iterator It> constexpr void insert(const iterator p_Pos, It p_Begin, It p_End) const noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= m_Capacity, "[TOOLKIT] New size exceeds capacity");

        m_Size += Container<Traits>::Insert(end(), p_Pos, p_Begin, p_End);
    }

    /**
     * @brief Insert a range of elements at the specified position. The elements are copied or moved into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    constexpr void insert(const iterator p_Pos, const std::initializer_list<T> p_Elements) const noexcept
    {
        insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Erase the element at the specified position. The element is destroyed.
     *
     * @param p_Pos The position to erase the element at.
     */
    constexpr void erase(const iterator p_Pos) const noexcept
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
    constexpr void erase(const iterator p_Begin, const iterator p_End) const noexcept
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
    constexpr T &emplace_back(Args &&...p_Args) const noexcept
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
    constexpr void resize(const size_type p_Size, Args &&...args) const noexcept
    {
        TKIT_ASSERT(p_Size <= m_Capacity, "[TOOLKIT] Size is bigger than capacity");

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    constexpr T &front() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *begin();
    }
    constexpr T &back() const noexcept
    {
        TKIT_ASSERT(!empty(), "[TOOLKIT] Container is empty");
        return *(begin() + m_Size - 1);
    }

    constexpr T &operator[](const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr T &at(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr void clear() const noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr T *data() const noexcept
    {
        return m_Data;
    }

    constexpr T *begin() const noexcept
    {
        return m_Data;
    }

    constexpr T *end() const noexcept
    {
        return m_Data + m_Size;
    }

    constexpr size_type size() const noexcept
    {
        return m_Size;
    }
    constexpr size_type capacity() const noexcept
    {
        return m_Capacity;
    }
    constexpr bool empty() const noexcept
    {
        return m_Size == 0;
    }
    constexpr bool full() const noexcept
    {
        return m_Size == capacity();
    }

    explicit(false) constexpr operator std::span<T>() const noexcept
    {
        return std::span<T>(data(), static_cast<size_t>(size()));
    }

  private:
    T *m_Data;
    usize m_Size;
    usize m_Capacity;
};
} // namespace TKit