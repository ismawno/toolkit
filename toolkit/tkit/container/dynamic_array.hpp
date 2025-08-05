#pragma once

#include "tkit/container/container.hpp"
#include "tkit/utils/concepts.hpp"
#include "tkit/utils/logging.hpp"

namespace TKit
{
/**
 * @brief A resizable array with a dynamic capacity buffer.
 *
 * It is meant to be used when the user does not now at compile time the maximum size the array may reach. This
 * container performs heap allocations.
 *
 * @tparam T The type of the elements in the array.
 * @tparam Traits The traits of the array, to define the types used for the iterators, size, etc.
 */
template <typename T, typename Traits = Container::ArrayTraits<T>> class DynamicArray
{
  public:
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = typename Traits::Iterator;
    using ConstIterator = typename Traits::ConstIterator;
    using Tools = Container::ArrayTools<Traits>;

    constexpr DynamicArray() noexcept = default;

    template <typename... Args>
    constexpr DynamicArray(const SizeType p_Size, const Args &...p_Args) noexcept : m_Size(p_Size)
    {
        growCapacity(p_Size);
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), p_Args...);
    }

    template <std::input_iterator It> constexpr DynamicArray(const It p_Begin, const It p_End) noexcept
    {
        m_Size = static_cast<SizeType>(std::distance(p_Begin, p_End));
        growCapacity(m_Size);
        Tools::CopyConstructFromRange(begin(), p_Begin, p_End);
    }

    constexpr DynamicArray(const DynamicArray &p_Other) noexcept : m_Size(p_Other.GetSize())
    {
        growCapacity(m_Size);
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr DynamicArray(DynamicArray &&p_Other) noexcept
        : m_Data(p_Other.m_Data), m_Size(p_Other.GetSize()), m_Capacity(p_Other.GetCapacity())

    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Capacity = 0;
    }

    constexpr DynamicArray(const std::initializer_list<ValueType> p_List) noexcept
        : m_Size(static_cast<SizeType>(p_List.size()))
    {
        growCapacity(m_Size);
        Tools::CopyConstructFromRange(begin(), p_List.begin(), p_List.end());
    }

    ~DynamicArray() noexcept
    {
        Clear();
        deallocateBuffer();
    }

    constexpr DynamicArray &operator=(const DynamicArray &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        const SizeType otherSize = p_Other.GetSize();
        if (otherSize > m_Capacity)
            growCapacity(otherSize);

        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = otherSize;
        return *this;
    }

    constexpr DynamicArray &operator=(DynamicArray &&p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        Clear();
        deallocateBuffer();
        m_Data = p_Other.m_Data;
        m_Size = p_Other.GetSize();
        m_Capacity = p_Other.GetCapacity();
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Capacity = 0;
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
    constexpr ValueType &Append(Args &&...p_Args) noexcept
    {
        const SizeType newSize = m_Size + 1;
        if (newSize > m_Capacity)
            growCapacity(newSize);
        m_Size = newSize;
        return *Memory::ConstructFromIterator(end() - 1, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Remove the last element from the array.
     *
     */
    constexpr void Pop() noexcept
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT] Container is already empty");
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
    constexpr void Insert(Iterator p_Pos, U &&p_Value) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        const SizeType newSize = m_Size + 1;
        if (newSize > m_Capacity)
        {
            const SizeType pos = static_cast<SizeType>(std::distance(begin(), p_Pos));
            growCapacity(newSize);
            p_Pos = begin() + pos;
        }

        Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
        m_Size = newSize;
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
    template <std::input_iterator It> constexpr void Insert(Iterator p_Pos, It p_Begin, It p_End) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");

        const SizeType newSize = m_Size + static_cast<SizeType>(std::distance(p_Begin, p_End));
        if (newSize > m_Capacity)
        {
            const SizeType pos = static_cast<SizeType>(std::distance(begin(), p_Pos));
            growCapacity(newSize);
            p_Pos = begin() + pos;
        }

        Tools::Insert(end(), p_Pos, p_Begin, p_End);
        m_Size = newSize;
    }

    /**
     * @brief Insert a range of elements at the specified position.
     *
     * The elements are copied into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    constexpr void Insert(Iterator p_Pos, const std::initializer_list<ValueType> p_Elements) noexcept
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
    constexpr void RemoveOrdered(const Iterator p_Pos) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT] Iterator is out of bounds");
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
    constexpr void RemoveOrdered(const Iterator p_Begin, const Iterator p_End) noexcept
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT] Range overflows array");

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
    constexpr void RemoveUnordered(const Iterator p_Pos) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT] Iterator is out of bounds");
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
    constexpr void Resize(const SizeType p_Size, const Args &...p_Args) noexcept
    {
        if (p_Size > m_Capacity)
            growCapacity(p_Size);

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, p_Args...);

        m_Size = p_Size;
    }

    constexpr const ValueType &operator[](const SizeType p_Index) const noexcept
    {
        return At(p_Index);
    }
    constexpr ValueType &operator[](const SizeType p_Index) noexcept
    {
        return At(p_Index);
    }
    constexpr const ValueType &At(const SizeType p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr ValueType &At(const SizeType p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr const ValueType &GetFront() const noexcept
    {
        return At(0);
    }
    constexpr ValueType &GetFront() noexcept
    {
        return At(0);
    }

    constexpr const ValueType &GetBack() const noexcept
    {
        return At(m_Size - 1);
    }
    constexpr ValueType &GetBack() noexcept
    {
        return At(m_Size - 1);
    }

    /**
     * @brief Clear the array and set its size to 0.
     *
     * The elements are destroyed if not trivially destructible. The memory is not deallocated.
     *
     */
    constexpr void Clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    /**
     * @brief Reserve memory for the array ahead of time.
     *
     * This does not change the size of the array. It can potentially reduce the number of allocations.
     *
     * @param p_Capacity
     */
    constexpr void Reserve(const SizeType p_Capacity) noexcept
    {
        if (p_Capacity > m_Capacity)
            modifyCapacity(p_Capacity);
    }

    constexpr void Shrink() noexcept
    {
        if (m_Size == 0)
            deallocateBuffer();
        else if (m_Size < m_Capacity)
            modifyCapacity(m_Size);
    }

    constexpr const ValueType *GetData() const noexcept
    {
        return reinterpret_cast<const ValueType *>(&m_Data[0]);
    }

    constexpr ValueType *GetData() noexcept
    {
        return reinterpret_cast<ValueType *>(&m_Data[0]);
    }

    constexpr Iterator begin() noexcept
    {
        return GetData();
    }

    constexpr Iterator end() noexcept
    {
        return begin() + m_Size;
    }

    constexpr ConstIterator begin() const noexcept
    {
        return GetData();
    }

    constexpr ConstIterator end() const noexcept
    {
        return begin() + m_Size;
    }

    constexpr SizeType GetSize() const noexcept
    {
        return m_Size;
    }

    constexpr SizeType GetCapacity() const noexcept
    {
        return m_Capacity;
    }

    constexpr bool IsEmpty() const noexcept
    {
        return m_Size == 0;
    }

  private:
    constexpr void modifyCapacity(const SizeType p_Capacity) noexcept
    {
        TKIT_ASSERT(p_Capacity > 0, "[TOOLKIT] Capacity must be greater than 0");
        TKIT_ASSERT(p_Capacity >= m_Size, "[TOOLKIT] Capacity is smaller than size");
        ValueType *newData =
            static_cast<ValueType *>(Memory::AllocateAligned(p_Capacity * sizeof(ValueType), alignof(ValueType)));
        TKIT_ASSERT(newData, "[TOOLKIT] Failed to allocate memory");

        if (m_Data)
        {
            Tools::MoveConstructFromRange(newData, begin(), end());
            if constexpr (!std::is_trivially_destructible_v<T>)
                Memory::DestructRange(begin(), end());
            Memory::DeallocateAligned(m_Data);
        }
        m_Data = newData;
        m_Capacity = p_Capacity;
    }

    constexpr void deallocateBuffer() noexcept
    {
        TKIT_ASSERT(m_Size == 0, "[TOOLKIT] Cannot deallocate buffer while it is not empty");
        if (m_Data)
        {
            Memory::DeallocateAligned(m_Data);
            m_Data = nullptr;
            m_Capacity = 0;
        }
    }

    constexpr void growCapacity(const SizeType p_Size) noexcept
    {
        modifyCapacity(Tools::GrowthFactor(p_Size));
    }

    ValueType *m_Data = nullptr;
    SizeType m_Size = 0;
    SizeType m_Capacity = 0;
};
} // namespace TKit
