#pragma once

#include "tkit/container/container.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
/**
 * @brief A resizable array with a dynamic capacity buffer.
 *
 * It is meant to be used when the user does not now at compile time the maximum size the array may reach. This
 * container performs heap allocations.
 *
 * @tparam T The type of the elements in the array.
 */
template <typename T> class DynamicArray
{
  public:
    using ValueType = T;
    using Tools = Container::ArrayTools<T>;

    constexpr DynamicArray() = default;

    template <typename... Args>
    constexpr explicit DynamicArray(const usize p_Size, const Args &...p_Args) : m_Size(p_Size)
    {
        if (m_Size > 0)
            growCapacity(m_Size);
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), p_Args...);
    }

    template <std::input_iterator It>
    constexpr DynamicArray(const It p_Begin, const It p_End)
        requires(std::is_copy_constructible_v<T>)
    {
        m_Size = static_cast<usize>(std::distance(p_Begin, p_End));
        if (m_Size > 0)
            growCapacity(m_Size);
        Tools::CopyConstructFromRange(begin(), p_Begin, p_End);
    }

    constexpr DynamicArray(const std::initializer_list<T> p_List)
        requires(std::is_copy_constructible_v<T>)
        : m_Size(static_cast<usize>(p_List.size()))
    {
        if (m_Size > 0)
            growCapacity(m_Size);
        Tools::CopyConstructFromRange(begin(), p_List.begin(), p_List.end());
    }

    constexpr DynamicArray(const DynamicArray &p_Other)
        requires(std::is_copy_constructible_v<T>)
        : m_Size(p_Other.GetSize())
    {
        if (m_Size > 0)
            growCapacity(m_Size);
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr DynamicArray(DynamicArray &&p_Other)
        requires(std::is_move_constructible_v<T>)
        : m_Data(p_Other.m_Data), m_Size(p_Other.GetSize()), m_Capacity(p_Other.GetCapacity())

    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Capacity = 0;
    }

    ~DynamicArray()
    {
        Clear();
        deallocateBuffer();
    }

    constexpr DynamicArray &operator=(const DynamicArray &p_Other)
        requires(std::is_copy_assignable_v<T>)
    {
        if (this == &p_Other)
            return *this;

        const usize otherSize = p_Other.GetSize();
        if (otherSize > m_Capacity)
            growCapacity(otherSize);

        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = otherSize;
        return *this;
    }

    constexpr DynamicArray &operator=(DynamicArray &&p_Other)
        requires(std::is_move_assignable_v<T>)
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

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &Append(Args &&...p_Args)
    {
        const usize newSize = m_Size + 1;
        if (newSize > m_Capacity)
            growCapacity(newSize);
        m_Size = newSize;
        return *Memory::ConstructFromIterator(end() - 1, std::forward<Args>(p_Args)...);
    }

    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][DYN-ARRAY] Cannot Pop(). Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
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
        requires(std::convertible_to<std::remove_cvref_t<T>, std::remove_cvref_t<U>>)
    constexpr void Insert(T *p_Pos, U &&p_Value)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][DYN-ARRAY] Iterator is out of bounds");
        const usize newSize = m_Size + 1;
        if (newSize > m_Capacity)
        {
            const usize pos = static_cast<usize>(std::distance(begin(), p_Pos));
            growCapacity(newSize);
            p_Pos = begin() + pos;
        }

        Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
        m_Size = newSize;
    }

    template <std::input_iterator It> constexpr void Insert(T *p_Pos, It p_Begin, It p_End)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][DYN-ARRAY] Iterator is out of bounds");

        const usize newSize = m_Size + static_cast<usize>(std::distance(p_Begin, p_End));
        if (newSize > m_Capacity)
        {
            const usize pos = static_cast<usize>(std::distance(begin(), p_Pos));
            growCapacity(newSize);
            p_Pos = begin() + pos;
        }

        Tools::Insert(end(), p_Pos, p_Begin, p_End);
        m_Size = newSize;
    }

    constexpr void Insert(T *p_Pos, const std::initializer_list<T> p_Elements)
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
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][DYN-ARRAY] Iterator is out of bounds");
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
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT][DYN-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT][DYN-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT][DYN-ARRAY] Range overflows array");

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
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][DYN-ARRAY] Iterator is out of bounds");
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
        requires std::constructible_from<T, Args...>
    constexpr void Resize(const usize p_Size, const Args &...p_Args)
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

    constexpr const T &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr T &operator[](const usize p_Index)
    {
        return At(p_Index);
    }
    constexpr const T &At(const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][DYN-ARRAY] ");
        return *(begin() + p_Index);
    }
    constexpr T &At(const usize p_Index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][DYN-ARRAY] ");
        return *(begin() + p_Index);
    }

    constexpr const T &GetFront() const
    {
        return At(0);
    }
    constexpr T &GetFront()
    {
        return At(0);
    }

    constexpr const T &GetBack() const
    {
        return At(m_Size - 1);
    }
    constexpr T &GetBack()
    {
        return At(m_Size - 1);
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr void Reserve(const usize p_Capacity)
    {
        if (p_Capacity > m_Capacity)
            modifyCapacity(p_Capacity);
    }

    constexpr void Shrink()
    {
        if (m_Size == 0)
            deallocateBuffer();
        else if (m_Size < m_Capacity)
            modifyCapacity(m_Size);
    }

    constexpr const T *GetData() const
    {
        return reinterpret_cast<const T *>(&m_Data[0]);
    }

    constexpr T *GetData()
    {
        return reinterpret_cast<T *>(&m_Data[0]);
    }

    constexpr T *begin()
    {
        return GetData();
    }

    constexpr T *end()
    {
        return begin() + m_Size;
    }

    constexpr const T *begin() const
    {
        return GetData();
    }

    constexpr const T *end() const
    {
        return begin() + m_Size;
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
        return m_Size == m_Capacity;
    }

  private:
    constexpr void modifyCapacity(const usize p_Capacity)
    {
        TKIT_ASSERT(p_Capacity > 0, "[TOOLKIT][DYN-ARRAY] Capacity must be greater than 0");
        TKIT_ASSERT(p_Capacity >= m_Size, "[TOOLKIT][DYN-ARRAY] Capacity ({}) is smaller than size ({})", p_Capacity,
                    m_Size);
        T *newData = static_cast<T *>(Memory::AllocateAligned(p_Capacity * sizeof(T), alignof(T)));
        TKIT_ASSERT(newData, "[TOOLKIT][DYN-ARRAY] Failed to allocate {} bytes of memory aligned to {} bytes",
                    p_Capacity * sizeof(T), alignof(T));

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

    constexpr void deallocateBuffer()
    {
        TKIT_ASSERT(m_Size == 0, "[TOOLKIT][DYN-ARRAY] Cannot deallocate buffer while it is not empty. Size is {}",
                    m_Size);
        if (m_Data)
        {
            Memory::DeallocateAligned(m_Data);
            m_Data = nullptr;
            m_Capacity = 0;
        }
    }

    constexpr void growCapacity(const usize p_Size)
    {
        modifyCapacity(Tools::GrowthFactor(p_Size));
    }

    T *m_Data = nullptr;
    usize m_Size = 0;
    usize m_Capacity = 0;
};
} // namespace TKit
