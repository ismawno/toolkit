#pragma once

#include "tkit/container/static_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/utils/non_copyable.hpp"

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
template <typename T, usize Capacity = Limits<usize>::max(), typename Traits = Container::ArrayTraits<NoCVRef<T>>>
class WeakArray
{
    static_assert(Capacity > 0, "[TOOLKIT][WEAK-ARRAY] Capacity must be greater than 0");
    TKIT_NON_COPYABLE(WeakArray)

  public:
    using ElementType = T;
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = ElementType *;
    using Tools = Container::ArrayTools<Traits>;

    constexpr WeakArray() : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr WeakArray(ElementType *p_Data) : m_Data(p_Data), m_Size(0)
    {
    }
    constexpr WeakArray(ElementType *p_Data, const SizeType p_Size) : m_Data(p_Data), m_Size(p_Size)
    {
    }

    constexpr
        WeakArray(const Array<ValueType, Capacity, Traits> &p_Array, const SizeType p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size)
    {
    }
    constexpr WeakArray(Array<ValueType, Capacity, Traits> &p_Array, const SizeType p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size)
    {
    }

    constexpr WeakArray(const StaticArray<ValueType, Capacity, Traits> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }
    constexpr WeakArray(StaticArray<ValueType, Capacity, Traits> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize())
    {
    }

    template <typename U>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr WeakArray(WeakArray<U, Capacity, Traits> &&p_Other)
        : m_Data(p_Other.GetData()), m_Size(p_Other.GetSize())
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
    }

    template <typename U>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr WeakArray &operator=(WeakArray<U, Capacity, Traits> &&p_Other)
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

    /**
     * @brief Insert a new element at the end of the array.
     *
     * The element is constructed in place using the provided arguments.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &Append(Args &&...p_Args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Container is already full");
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Remove the last element from the array.
     *
     */
    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][WEAK-ARRAY] Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<ValueType>)
            Memory::DestructFromIterator(end());
    }

    /**
     * @brief Insert a new element at the specified position.
     *
     * The element is copied or moved into the array.
     *
     * @param p_Pos The position to insert the element at.
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::is_convertible_v<NoCVRef<ValueType>, NoCVRef<U>>)
    constexpr void Insert(const Iterator p_Pos, U &&p_Value)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Container is already full");
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
        ++m_Size;
    }

    /**
     * @brief Insert a range of elements at the specified position.
     *
     * The elements are copied into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Begin The beginning of the range to insert.
     * @param p_End The end of the range to insert.
     */
    template <std::input_iterator It> constexpr void Insert(const Iterator p_Pos, It p_Begin, It p_End)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= Capacity,
                    "[TOOLKIT][WEAK-ARRAY] New size exceeds capacity");

        m_Size += Tools::Insert(end(), p_Pos, p_Begin, p_End);
    }

    /**
     * @brief Insert a range of elements at the specified position.
     *
     * The elements are copied into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    constexpr void Insert(const Iterator p_Pos, const std::initializer_list<ValueType> p_Elements)
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
    constexpr void RemoveOrdered(const Iterator p_Pos)
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
    constexpr void RemoveOrdered(const Iterator p_Begin, const Iterator p_End)
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT][WEAK-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT][WEAK-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT][WEAK-ARRAY] New size is negative");

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
    constexpr void RemoveUnordered(const Iterator p_Pos)
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
        requires std::constructible_from<ValueType, Args...>
    constexpr void Resize(const SizeType p_Size, Args &&...args)
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT][WEAK-ARRAY] Size is bigger than capacity");

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    constexpr ElementType &operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }

    constexpr ElementType &At(const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT][WEAK-ARRAY] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr ElementType &GetFront() const
    {
        return At(0);
    }
    constexpr ElementType &GetBack() const
    {
        return At(m_Size - 1);
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr ElementType *GetData() const
    {
        return m_Data;
    }

    constexpr Iterator begin() const
    {
        return m_Data;
    }
    constexpr Iterator end() const
    {
        return m_Data + m_Size;
    }

    constexpr SizeType GetSize() const
    {
        return m_Size;
    }
    constexpr SizeType capacity() const
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
    ElementType *m_Data;
    SizeType m_Size;

    template <typename U, usize OtherCapacity, typename OtherTraits> friend class WeakArray;
};

/**
 * @brief A modifiable view with a dynamic capacity that does not own the elements it references.
 *
 * It otherwise behaves like a `StaticArray`. It cannot be copied.
 *
 * @tparam T The type of the elements in the array.
 */
template <typename T, typename Traits> class WeakArray<T, Limits<usize>::max(), Traits>
{
    TKIT_NON_COPYABLE(WeakArray)

  public:
    using ElementType = T;
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = ElementType *;
    using Tools = Container::ArrayTools<Traits>;

    constexpr WeakArray() : m_Data(nullptr), m_Size(0), m_Capacity(0)
    {
    }
    constexpr WeakArray(ElementType *p_Data, const SizeType p_Capacity, const SizeType p_Size = 0)
        : m_Data(p_Data), m_Size(p_Size), m_Capacity(p_Capacity)
    {
    }

    template <SizeType Capacity>
    constexpr
        WeakArray(const Array<ValueType, Capacity, Traits> &p_Array, const SizeType p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size), m_Capacity(Capacity)
    {
    }
    template <SizeType Capacity>
    constexpr WeakArray(Array<ValueType, Capacity, Traits> &p_Array, const SizeType p_Size = 0)
        : m_Data(p_Array.GetData()), m_Size(p_Size), m_Capacity(Capacity)
    {
    }

    template <SizeType Capacity>
    constexpr WeakArray(const StaticArray<ValueType, Capacity, Traits> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(Capacity)
    {
    }
    template <SizeType Capacity>
    constexpr WeakArray(StaticArray<ValueType, Capacity, Traits> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(Capacity)
    {
    }

    constexpr WeakArray(const DynamicArray<ValueType> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(p_Array.GetCapacity())
    {
    }
    constexpr WeakArray(DynamicArray<ValueType> &p_Array)
        : m_Data(p_Array.GetData()), m_Size(p_Array.GetSize()), m_Capacity(p_Array.GetCapacity())
    {
    }

    template <typename U, SizeType Capacity>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr WeakArray(WeakArray<U, Capacity, Traits> &&p_Other)
        : m_Data(p_Other.GetData()), m_Size(p_Other.GetSize()), m_Capacity(p_Other.GetCapacity())
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        if constexpr (Capacity == Limits<usize>::max())
            p_Other.m_Capacity = 0;
    }

    template <typename U, SizeType Capacity>
        requires(std::convertible_to<U *, T *> && std::same_as<NoCVRef<U>, NoCVRef<T>>)
    constexpr WeakArray &operator=(WeakArray<U, Capacity, Traits> &&p_Other)
    {
        m_Data = p_Other.GetData();
        m_Size = p_Other.GetSize();
        m_Capacity = p_Other.GetCapacity();

        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        if constexpr (Capacity == Limits<usize>::max())
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

    /**
     * @brief Insert a new element at the end of the array.
     *
     * The element is constructed in place using the provided arguments.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &Append(Args &&...p_Args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Container is already full");
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Remove the last element from the array.
     *
     */
    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][WEAK-ARRAY] Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<ValueType>)
            Memory::DestructFromIterator(end());
    }

    /**
     * @brief Insert a new element at the specified position.
     *
     * The element is copied or moved into the array.
     *
     * @param p_Pos The position to insert the element at.
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::is_convertible_v<NoCVRef<ValueType>, NoCVRef<U>>)
    constexpr void Insert(const Iterator p_Pos, U &&p_Value)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][WEAK-ARRAY] Container is already full");
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
        ++m_Size;
    }

    /**
     * @brief Insert a range of elements at the specified position.
     *
     * The elements are copied into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Begin The beginning of the range to insert.
     * @param p_End The end of the range to insert.
     */
    template <std::input_iterator It> constexpr void Insert(const Iterator p_Pos, It p_Begin, It p_End)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][WEAK-ARRAY] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= m_Capacity,
                    "[TOOLKIT][WEAK-ARRAY] New size exceeds capacity");

        m_Size += Tools::Insert(end(), p_Pos, p_Begin, p_End);
    }

    /**
     * @brief Insert a range of elements at the specified position.
     *
     * The elements are copied into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    constexpr void Insert(const Iterator p_Pos, const std::initializer_list<ValueType> p_Elements)
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
    constexpr void RemoveOrdered(const Iterator p_Pos)
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
    constexpr void RemoveOrdered(const Iterator p_Begin, const Iterator p_End)
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT][WEAK-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT][WEAK-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT][WEAK-ARRAY] New size is negative");

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
    constexpr void RemoveUnordered(const Iterator p_Pos)
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
        requires std::constructible_from<ValueType, Args...>
    constexpr void Resize(const SizeType p_Size, Args &&...args)
    {
        TKIT_ASSERT(p_Size <= m_Capacity, "[TOOLKIT][WEAK-ARRAY] Size is bigger than capacity");

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, std::forward<Args>(args)...);

        m_Size = p_Size;
    }

    constexpr ElementType &operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }
    constexpr ElementType &At(const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT][WEAK-ARRAY] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr ElementType &GetFront() const
    {
        return At(0);
    }

    constexpr ElementType &GetBack() const
    {
        return At(m_Size - 1);
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr ElementType *GetData() const
    {
        return m_Data;
    }

    constexpr Iterator begin() const
    {
        return m_Data;
    }
    constexpr Iterator end() const
    {
        return m_Data + m_Size;
    }

    constexpr SizeType GetSize() const
    {
        return m_Size;
    }
    constexpr SizeType GetCapacity() const
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
    ElementType *m_Data;
    SizeType m_Size;
    SizeType m_Capacity;

    template <typename U, usize Capacity, typename OtherTraits> friend class WeakArray;
};
} // namespace TKit
