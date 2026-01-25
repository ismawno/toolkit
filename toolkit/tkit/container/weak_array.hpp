#pragma once

#include "tkit/container/fixed_array.hpp"
#include "tkit/container/array.hpp"
#include "tkit/utils/non_copyable.hpp"
#include "tkit/utils/limits.hpp"

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
template <typename T, usize Capacity = TKIT_USIZE_MAX> class WeakArray
{
    static_assert(Capacity > 0, "[TOOLKIT][WEAK-ARRAY] Capacity must be greater than 0");
    TKIT_NON_COPYABLE(WeakArray)

  public:
    using ValueType = T;
    using ElementType = std::remove_cvref_t<T>;
    using Tools = Container::ArrayTools<ElementType>;

    constexpr WeakArray() : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr WeakArray(T *data) : m_Data(data), m_Size(0)
    {
    }
    constexpr WeakArray(T *data, const usize size) : m_Data(data), m_Size(size)
    {
    }

    constexpr WeakArray(const FixedArray<ElementType, Capacity> &array, const usize size = 0)
        : m_Data(array.GetData()), m_Size(size)
    {
    }
    constexpr WeakArray(FixedArray<ElementType, Capacity> &array, const usize size = 0)
        : m_Data(array.GetData()), m_Size(size)
    {
    }

    template <typename AllocState>
    constexpr WeakArray(const Array<ElementType, AllocState> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }
    template <typename AllocState>
    constexpr WeakArray(Array<ElementType, AllocState> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray(WeakArray<ElementType, Capacity> &&other) : m_Data(other.GetData()), m_Size(other.GetSize())
    {
        other.m_Data = nullptr;
        other.m_Size = 0;
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray &operator=(WeakArray<ElementType, Capacity> &&other)
    {
        m_Data = other.GetData();
        m_Size = other.GetSize();

        other.m_Data = nullptr;
        other.m_Size = 0;
        return *this;
    }

    constexpr WeakArray(WeakArray &&other) : m_Data(other.m_Data), m_Size(other.GetSize())
    {
        other.m_Data = nullptr;
        other.m_Size = 0;
    }
    constexpr WeakArray &operator=(WeakArray &&other)
    {
        if (this != &other)
        {
            m_Data = other.m_Data;
            m_Size = other.GetSize();

            other.m_Data = nullptr;
            other.m_Size = 0;
        }
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr ElementType &Append(Args &&...args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Append(). Container is already at capacity ({})",
                    Capacity);
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(args)...);
    }

    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][WEAK-ARRAY] Cannot Pop(). Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<ElementType>)
            Memory::DestructFromIterator(end());
    }

    template <typename ElementType>
        requires(std::convertible_to<std::remove_cvref_t<ElementType>, std::remove_cvref_t<ElementType>>)
    constexpr void Insert(T *pos, ElementType &&value)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Insert(). Container is at capacity ({})", Capacity);
        TKIT_ASSERT(pos >= begin() && pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::Insert(end(), pos, std::forward<ElementType>(value));
        ++m_Size;
    }

    template <std::input_iterator It> constexpr void Insert(T *pos, const It pbegin, const It pend)
    {
        TKIT_ASSERT(pos >= begin() && pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(pbegin, pend) + m_Size <= Capacity,
                    "[TOOLKIT][WEAK-ARRAY] New size exceeds capacity");

        m_Size += Tools::Insert(end(), pos, pbegin, pend);
    }

    constexpr void Insert(T *pos, const std::initializer_list<ElementType> elements)
    {
        Insert(pos, elements.begin(), elements.end());
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * All elements "to the right" of `pos` are shifted, and the order is preserved.
     *
     * @param pos The position to remove the element at.
     */
    constexpr void RemoveOrdered(T *pos)
    {
        TKIT_ASSERT(pos >= begin() && pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveOrdered(end(), pos);
        --m_Size;
    }

    /**
     * @brief Remove a range of elements.
     *
     * All elements "to the right" of the range are shifted, and the order is preserved.
     *
     * @param begin The beginning of the range to erase.
     * @param end The end of the range to erase.
     */
    constexpr void RemoveOrdered(T *pbegin, T *pend)
    {
        TKIT_ASSERT(pbegin >= begin() && pbegin <= end(), "[TOOLKIT][WEAK-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(pend >= begin() && pend <= end(), "[TOOLKIT][WEAK-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(pbegin, pend), "[TOOLKIT][WEAK-ARRAY] Range overflows array");

        m_Size -= Tools::RemoveOrdered(end(), pbegin, pend);
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * The last element is moved into the position of the removed element, which is then popped. The order is not
     * preserved.
     *
     * @param pos The position to remove the element at.
     */
    constexpr void RemoveUnordered(T *pos)
    {
        TKIT_ASSERT(pos >= begin() && pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveUnordered(end(), pos);
        --m_Size;
    }

    /**
     * @brief Resize the array.
     *
     * If the new size is smaller than the current size, the elements are destroyed. If the new size is bigger than the
     * current size, the elements are constructed in place.
     *
     * @param size The new size of the array.
     * @param args The arguments to pass to the constructor of `T` (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr void Resize(const usize size, Args &&...args)
    {
        TKIT_ASSERT(size <= Capacity, "[TOOLKIT][WEAK-ARRAY] Size ({}) is bigger than capacity ({})", size, Capacity);

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (size < m_Size)
                Memory::DestructRange(begin() + size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + size, std::forward<Args>(args)...);

        m_Size = size;
    }

    constexpr T &operator[](const usize index) const
    {
        return At(index);
    }

    constexpr T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_Size, "[TOOLKIT][WEAK-ARRAY] ");
        return *(begin() + index);
    }

    constexpr T &GetFront() const
    {
        return At(0);
    }
    constexpr T &GetBack() const
    {
        return At(m_Size - 1);
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr T *GetData() const
    {
        return m_Data;
    }

    constexpr T *begin() const
    {
        return m_Data;
    }
    constexpr T *end() const
    {
        return m_Data + m_Size;
    }

    constexpr usize GetSize() const
    {
        return m_Size;
    }
    constexpr usize capacity() const
    {
        return Capacity;
    }
    constexpr bool IsEmpty() const
    {
        return m_Size == 0;
    }
    constexpr bool IsFull() const
    {
        return m_Size == capacity();
    }

    constexpr operator bool() const
    {
        return m_Data != nullptr;
    }

  private:
    T *m_Data;
    usize m_Size;

    template <typename ElementType, usize OtherCapacity> friend class WeakArray;
};

/**
 * @brief A modifiable view with a dynamic capacity that does not own the elements it references.
 *
 * It otherwise behaves like a `StaticArray`. It cannot be copied.
 *
 * @tparam T The type of the elements in the array.
 */
template <typename T> class WeakArray<T, TKIT_USIZE_MAX>
{
    TKIT_NON_COPYABLE(WeakArray)

  public:
    using ElementType = std::remove_cvref_t<T>;
    using Tools = Container::ArrayTools<ElementType>;

    constexpr WeakArray() : m_Data(nullptr), m_Size(0), m_Capacity(0)
    {
    }
    constexpr WeakArray(T *data, const usize capacity, const usize size = 0)
        : m_Data(data), m_Size(size), m_Capacity(capacity)
    {
    }

    template <usize Capacity>
    constexpr WeakArray(const FixedArray<ElementType, Capacity> &array, const usize size = 0)
        : m_Data(array.GetData()), m_Size(size), m_Capacity(Capacity)
    {
    }
    template <usize Capacity>
    constexpr WeakArray(FixedArray<ElementType, Capacity> &array, const usize size = 0)
        : m_Data(array.GetData()), m_Size(size), m_Capacity(Capacity)
    {
    }

    template <typename AllocState>
    constexpr WeakArray(const Array<ElementType, AllocState> &array)
        : m_Data(array.GetData()), m_Size(array.GetSize()), m_Capacity(array.GetCapacity())
    {
    }
    template <typename AllocState>
    constexpr WeakArray(Array<ElementType, AllocState> &array)
        : m_Data(array.GetData()), m_Size(array.GetSize()), m_Capacity(array.GetCapacity())
    {
    }

    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray(WeakArray<ElementType, Capacity> &&other)
        : m_Data(other.GetData()), m_Size(other.GetSize()), m_Capacity(other.GetCapacity())
    {
        other.m_Data = nullptr;
        other.m_Size = 0;
        if constexpr (Capacity == TKIT_USIZE_MAX)
            other.m_Capacity = 0;
    }

    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray &operator=(WeakArray<ElementType, Capacity> &&other)
    {
        m_Data = other.GetData();
        m_Size = other.GetSize();
        m_Capacity = other.GetCapacity();

        other.m_Data = nullptr;
        other.m_Size = 0;
        if constexpr (Capacity == TKIT_USIZE_MAX)
            other.m_Capacity = 0;
        return *this;
    }

    constexpr WeakArray(WeakArray &&other)
        : m_Data(other.m_Data), m_Size(other.GetSize()), m_Capacity(other.GetCapacity())
    {
        other.m_Data = nullptr;
        other.m_Size = 0;
        other.m_Capacity = 0;
    }
    constexpr WeakArray &operator=(WeakArray &&other)
    {
        if (this != &other)
        {
            m_Data = other.m_Data;
            m_Size = other.GetSize();
            m_Capacity = other.GetCapacity();

            other.m_Data = nullptr;
            other.m_Size = 0;
            other.m_Capacity = 0;
        }
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr ElementType &Append(Args &&...args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Append(). Container is at capacity ({})", m_Capacity);
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(args)...);
    }

    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][WEAK-ARRAY] Cannot Pop(). Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<ElementType>)
            Memory::DestructFromIterator(end());
    }

    template <typename ElementType>
        requires(std::convertible_to<std::remove_cvref_t<ElementType>, std::remove_cvref_t<ElementType>>)
    constexpr void Insert(T *pos, ElementType &&value)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Insert(). Container is at capacity ({})", m_Capacity);
        TKIT_ASSERT(pos >= begin() && pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::Insert(end(), pos, std::forward<ElementType>(value));
        ++m_Size;
    }

    template <std::input_iterator It> constexpr void Insert(T *pos, It pbegin, It pend)
    {
        TKIT_ASSERT(pos >= begin() && pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(pbegin, pend) + m_Size <= m_Capacity,
                    "[TOOLKIT][WEAK-ARRAY] New size exceeds capacity");

        m_Size += Tools::Insert(end(), pos, pbegin, pend);
    }

    constexpr void Insert(T *pos, const std::initializer_list<ElementType> elements)
    {
        Insert(pos, elements.begin(), elements.end());
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * All elements "to the right" of `pos` are shifted, and the order is preserved.
     *
     * @param pos The position to remove the element at.
     */
    constexpr void RemoveOrdered(T *pos)
    {
        TKIT_ASSERT(pos >= begin() && pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveOrdered(end(), pos);
        --m_Size;
    }

    /**
     * @brief Remove a range of elements.
     *
     * All elements "to the right" of the range are shifted, and the order is preserved.
     *
     * @param begin The beginning of the range to erase.
     * @param end The end of the range to erase.
     */
    constexpr void RemoveOrdered(T *pbegin, T *pend)
    {
        TKIT_ASSERT(pbegin >= begin() && pbegin <= end(), "[TOOLKIT][WEAK-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(pend >= begin() && pend <= end(), "[TOOLKIT][WEAK-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(pbegin, pend), "[TOOLKIT][WEAK-ARRAY] Range overflows array");

        m_Size -= Tools::RemoveOrdered(end(), pbegin, pend);
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * The last element is moved into the position of the removed element, which is then popped. The order is not
     * preserved.
     *
     * @param pos The position to remove the element at.
     */
    constexpr void RemoveUnordered(T *pos)
    {
        TKIT_ASSERT(pos >= begin() && pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveUnordered(end(), pos);
        --m_Size;
    }

    /**
     * @brief Resize the array.
     *
     * If the new size is smaller than the current size, the elements are destroyed. If the new size is bigger than the
     * current size, the elements are constructed in place.
     *
     * @param size The new size of the array.
     * @param args The arguments to pass to the constructor of `T` (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr void Resize(const usize size, Args &&...args)
    {
        TKIT_ASSERT(size <= m_Capacity, "[TOOLKIT][WEAK-ARRAY] Size ({}) is bigger than capacity ({})", size,
                    m_Capacity);

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (size < m_Size)
                Memory::DestructRange(begin() + size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + size, std::forward<Args>(args)...);

        m_Size = size;
    }

    constexpr T &operator[](const usize index) const
    {
        return At(index);
    }
    constexpr T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_Size, "[TOOLKIT][WEAK-ARRAY] ");
        return *(begin() + index);
    }

    constexpr T &GetFront() const
    {
        return At(0);
    }

    constexpr T &GetBack() const
    {
        return At(m_Size - 1);
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr T *GetData() const
    {
        return m_Data;
    }

    constexpr T *begin() const
    {
        return m_Data;
    }
    constexpr T *end() const
    {
        return m_Data + m_Size;
    }

    constexpr usize GetSize() const
    {
        return m_Size;
    }
    constexpr usize GetCapacity() const
    {
        return m_Capacity;
    }
    constexpr bool IsEmpty() const
    {
        return m_Size == 0;
    }
    constexpr bool IsFull() const
    {
        return m_Size == GetCapacity();
    }

    constexpr operator bool() const
    {
        return m_Data != nullptr;
    }

  private:
    T *m_Data;
    usize m_Size;
    usize m_Capacity;

    template <typename ElementType, usize Capacity> friend class WeakArray;
};
} // namespace TKit
