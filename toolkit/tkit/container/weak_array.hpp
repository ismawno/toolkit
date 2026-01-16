#pragma once

#include "tkit/container/static_array.hpp"
#include "tkit/container/dynamic_array.hpp"
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
    constexpr WeakArray(T *p_Data) : m_Data(p_Data), m_Size(0)
    {
    }
    constexpr WeakArray(T *p_Data, const usize p_Size) : m_Data(p_Data), m_Size(p_Size)
    {
    }

    constexpr WeakArray(const FixedArray<ElementType, Capacity> &p_Array, const usize p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size)
    {
    }
    constexpr WeakArray(FixedArray<ElementType, Capacity> &p_Array, const usize p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size)
    {
    }

    constexpr WeakArray(const StaticArray<ElementType, Capacity> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    constexpr WeakArray(StaticArray<ElementType, Capacity> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray(WeakArray<ElementType, Capacity> &&p_Other)
        : m_Data(p_Other.GetData()), m_Size(p_Other.GetSize())
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray &operator=(WeakArray<ElementType, Capacity> &&p_Other)
    {
        m_Data = p_Other.GetData();
        m_Size = p_Other.GetSize();

        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        return *this;
    }

    constexpr WeakArray(WeakArray &&p_Other) : m_Data(p_Other.m_Data), m_Size(p_Other.GetSize())
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
    }
    constexpr WeakArray &operator=(WeakArray &&p_Other)
    {
        if (this != &p_Other)
        {
            m_Data = p_Other.m_Data;
            m_Size = p_Other.GetSize();

            p_Other.m_Data = nullptr;
            p_Other.m_Size = 0;
        }
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr ElementType &Append(Args &&...p_Args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Append(). Container is already at capacity ({})",
                    Capacity);
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(p_Args)...);
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
    constexpr void Insert(T *p_Pos, ElementType &&p_Value)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Insert(). Container is at capacity ({})", Capacity);
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::Insert(end(), p_Pos, std::forward<ElementType>(p_Value));
        ++m_Size;
    }

    template <std::input_iterator It> constexpr void Insert(T *p_Pos, It p_Begin, It p_End)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= Capacity,
                    "[TOOLKIT][WEAK-ARRAY] New size exceeds capacity");

        m_Size += Tools::Insert(end(), p_Pos, p_Begin, p_End);
    }

    constexpr void Insert(T *p_Pos, const std::initializer_list<ElementType> p_Elements)
    {
        Insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * All elements "to the right" of `p_Pos` are shifted, and the order is preserved.
     *
     * @param p_Pos The position to remove the element at.
     */
    constexpr void RemoveOrdered(T *p_Pos)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveOrdered(end(), p_Pos);
        --m_Size;
    }

    /**
     * @brief Remove a range of elements.
     *
     * All elements "to the right" of the range are shifted, and the order is preserved.
     *
     * @param p_Begin The beginning of the range to erase.
     * @param p_End The end of the range to erase.
     */
    constexpr void RemoveOrdered(T *p_Begin, T *p_End)
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT][WEAK-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT][WEAK-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT][WEAK-ARRAY] Range overflows array");

        m_Size -= Tools::RemoveOrdered(end(), p_Begin, p_End);
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * The last element is moved into the position of the removed element, which is then popped. The order is not
     * preserved.
     *
     * @param p_Pos The position to remove the element at.
     */
    constexpr void RemoveUnordered(T *p_Pos)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveUnordered(end(), p_Pos);
        --m_Size;
    }

    /**
     * @brief Resize the array.
     *
     * If the new size is smaller than the current size, the elements are destroyed. If the new size is bigger than the
     * current size, the elements are constructed in place.
     *
     * @param p_Size The new size of the array.
     * @param args The arguments to pass to the constructor of `T` (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr void Resize(const usize p_Size, Args &&...args)
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT][WEAK-ARRAY] Size ({}) is bigger than capacity ({})", p_Size,
                    Capacity);

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    constexpr T &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }

    constexpr T &At(const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][WEAK-ARRAY] ");
        return *(begin() + p_Index);
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
    constexpr WeakArray(T *p_Data, const usize p_Capacity, const usize p_Size = 0)
        : m_Data(p_Data), m_Size(p_Size), m_Capacity(p_Capacity)
    {
    }

    template <usize Capacity>
    constexpr WeakArray(const FixedArray<ElementType, Capacity> &p_Array, const usize p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size), m_Capacity(Capacity)
    {
    }
    template <usize Capacity>
    constexpr WeakArray(FixedArray<ElementType, Capacity> &p_Array, const usize p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size), m_Capacity(Capacity)
    {
    }

    template <usize Capacity>
    constexpr WeakArray(const StaticArray<ElementType, Capacity> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(Capacity)
    {
    }
    template <usize Capacity>
    constexpr WeakArray(StaticArray<ElementType, Capacity> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(Capacity)
    {
    }

    constexpr WeakArray(const DynamicArray<ElementType> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(p_Array.GetCapacity())
    {
    }
    constexpr WeakArray(DynamicArray<ElementType> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(p_Array.GetCapacity())
    {
    }

    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray(WeakArray<ElementType, Capacity> &&p_Other)
        : m_Data(p_Other.GetData()), m_Size(p_Other.GetSize()), m_Capacity(p_Other.GetCapacity())
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        if constexpr (Capacity == TKIT_USIZE_MAX)
            p_Other.m_Capacity = 0;
    }

    template <typename ElementType, usize Capacity>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr WeakArray &operator=(WeakArray<ElementType, Capacity> &&p_Other)
    {
        m_Data = p_Other.GetData();
        m_Size = p_Other.GetSize();
        m_Capacity = p_Other.GetCapacity();

        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        if constexpr (Capacity == TKIT_USIZE_MAX)
            p_Other.m_Capacity = 0;
        return *this;
    }

    constexpr WeakArray(WeakArray &&p_Other)
        : m_Data(p_Other.m_Data), m_Size(p_Other.GetSize()), m_Capacity(p_Other.GetCapacity())
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Capacity = 0;
    }
    constexpr WeakArray &operator=(WeakArray &&p_Other)
    {
        if (this != &p_Other)
        {
            m_Data = p_Other.m_Data;
            m_Size = p_Other.GetSize();
            m_Capacity = p_Other.GetCapacity();

            p_Other.m_Data = nullptr;
            p_Other.m_Size = 0;
            p_Other.m_Capacity = 0;
        }
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr ElementType &Append(Args &&...p_Args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Append(). Container is at capacity ({})", m_Capacity);
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(p_Args)...);
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
    constexpr void Insert(T *p_Pos, ElementType &&p_Value)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Cannot Insert(). Container is at capacity ({})", m_Capacity);
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::Insert(end(), p_Pos, std::forward<ElementType>(p_Value));
        ++m_Size;
    }

    template <std::input_iterator It> constexpr void Insert(T *p_Pos, It p_Begin, It p_End)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= m_Capacity,
                    "[TOOLKIT][WEAK-ARRAY] New size exceeds capacity");

        m_Size += Tools::Insert(end(), p_Pos, p_Begin, p_End);
    }

    constexpr void Insert(T *p_Pos, const std::initializer_list<ElementType> p_Elements)
    {
        Insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * All elements "to the right" of `p_Pos` are shifted, and the order is preserved.
     *
     * @param p_Pos The position to remove the element at.
     */
    constexpr void RemoveOrdered(T *p_Pos)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveOrdered(end(), p_Pos);
        --m_Size;
    }

    /**
     * @brief Remove a range of elements.
     *
     * All elements "to the right" of the range are shifted, and the order is preserved.
     *
     * @param p_Begin The beginning of the range to erase.
     * @param p_End The end of the range to erase.
     */
    constexpr void RemoveOrdered(T *p_Begin, T *p_End)
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT][WEAK-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT][WEAK-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT][WEAK-ARRAY] Range overflows array");

        m_Size -= Tools::RemoveOrdered(end(), p_Begin, p_End);
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * The last element is moved into the position of the removed element, which is then popped. The order is not
     * preserved.
     *
     * @param p_Pos The position to remove the element at.
     */
    constexpr void RemoveUnordered(T *p_Pos)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::RemoveUnordered(end(), p_Pos);
        --m_Size;
    }

    /**
     * @brief Resize the array.
     *
     * If the new size is smaller than the current size, the elements are destroyed. If the new size is bigger than the
     * current size, the elements are constructed in place.
     *
     * @param p_Size The new size of the array.
     * @param args The arguments to pass to the constructor of `T` (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args>
        requires std::constructible_from<ElementType, Args...>
    constexpr void Resize(const usize p_Size, Args &&...args)
    {
        TKIT_ASSERT(p_Size <= m_Capacity, "[TOOLKIT][WEAK-ARRAY] Size ({}) is bigger than capacity ({})", p_Size,
                    m_Capacity);

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    constexpr T &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr T &At(const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][WEAK-ARRAY] ");
        return *(begin() + p_Index);
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
