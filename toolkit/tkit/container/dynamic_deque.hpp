#pragma once

#include "tkit/container/container.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
/**
 * @brief A circular container with a dynamic capacity buffer designed for quick insertions at its front and back.
 *
 * It is meant to be used as a double ended queue.
 *
 * @tparam T The type of the elements in the deque.
 * @tparam Traits The traits of the deque, to define the types used for the iterators, size, etc.
 */
template <typename T, typename Traits = Container::ArrayTraits<T>> class DynamicDeque
{
  public:
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = typename Traits::Iterator;
    using ConstIterator = typename Traits::ConstIterator;
    using Tools = Container::ArrayTools<Traits>;

    constexpr DynamicDeque() = default;

    template <typename... Args>
    constexpr explicit DynamicDeque(const SizeType p_Size, const Args &...p_Args) : m_Size(p_Size)
    {
        if (m_Size > 0)
            growCapacity(m_Size);
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(GetData(), GetData() + p_Size, p_Args...);
    }

    template <std::input_iterator It>
    constexpr DynamicDeque(const It p_Begin, const It p_End)
        requires(std::is_copy_constructible_v<T>)
    {
        m_Size = static_cast<SizeType>(std::distance(p_Begin, p_End));
        if (m_Size > 0)
            growCapacity(m_Size);
        Tools::CopyConstructFromRange(GetData(), p_Begin, p_End);
    }

    constexpr DynamicDeque(const std::initializer_list<ValueType> p_List)
        requires(std::is_copy_constructible_v<T>)
        : m_Size(static_cast<SizeType>(p_List.size()))
    {
        if (m_Size > 0)
            growCapacity(m_Size);
        Tools::CopyConstructFromRange(GetData(), p_List.begin(), p_List.end());
    }

    constexpr DynamicDeque(const DynamicDeque &p_Other)
    {
        const u32 otherSize = p_Other.GetSize();
        if (otherSize > 0)
            growCapacity(otherSize);
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < otherSize; i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    constexpr DynamicDeque(DynamicDeque &&p_Other)
        : m_Data(p_Other.m_Data), m_Size(p_Other.m_Size), m_Capacity(p_Other.m_Capacity), m_Front(p_Other.m_Front),
          m_Back(p_Other.m_Back)
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Capacity = 0;
        p_Other.m_Front = 0;
        p_Other.m_Back = 0;
    }

    ~DynamicDeque()
    {
        Clear();
        deallocateBuffer();
    }

    constexpr DynamicDeque &operator=(const DynamicDeque &p_Other)
    {
        if (this == &p_Other)
            return *this;

        Clear();
        const u32 otherSize = p_Other.GetSize();
        if (otherSize > m_Capacity)
            growCapacity(otherSize);

        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < otherSize; i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }

    constexpr DynamicDeque &operator=(DynamicDeque &&p_Other)
    {
        if (this == &p_Other)
            return *this;

        m_Data = p_Other.m_Data;
        m_Size = p_Other.m_Size;
        m_Capacity = p_Other.m_Capacity;
        m_Front = p_Other.m_Front;
        m_Back = p_Other.m_Back;
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_Capacity = 0;
        p_Other.m_Front = 0;
        p_Other.m_Back = 0;
        return *this;
    }

    /**
     * @brief Insert a new element at the beginning of the queue.
     *
     * The element is constructed in place using the provided arguments.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &PushFront(Args &&...p_Args)
    {
        const SizeType newSize = m_Size + 1;
        if (newSize > m_Capacity)
            growCapacity(newSize);
        ValueType &val = *Memory::ConstructFromIterator(GetData() + m_Front, std::forward<Args>(p_Args)...);
        m_Front = PrevIndex(m_Front);
        m_Size = newSize;
        return val;
    }

    /**
     * @brief Insert a new element at the end of the queue.
     *
     * The element is constructed in place using the provided arguments.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &PushBack(Args &&...p_Args)
    {
        const SizeType newSize = m_Size + 1;
        if (newSize > m_Capacity)
            growCapacity(newSize);
        ValueType &val = *Memory::ConstructFromIterator(GetData() + m_Back, std::forward<Args>(p_Args)...);
        m_Back = NextIndex(m_Back);
        m_Size = newSize;
        return val;
    }

    /**
     * @brief Erase the element at the beginning of the queue.
     *
     * The destructor will only be called if not trivially destructible.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    constexpr void PopFront()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][DYN-DEQUE] Container is already empty");
        m_Front = NextIndex(m_Front);
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(GetData() + m_Front);
        --m_Size;
    }
    /**
     * @brief Erase the element at the end of the queue.
     *
     * The destructor will only be called if not trivially destructible.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    constexpr void PopBack()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][DYN-DEQUE] Container is already empty");
        m_Back = PrevIndex(m_Back);
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(GetData() + m_Back);
        --m_Size;
    }

    /**
     * @brief Clear the deque and set its size to 0.
     *
     * The elements are destroyed if not trivially destructible. The memory is not deallocated.
     *
     */
    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            while (!IsEmpty())
                PopBack();
        else
        {
            m_Size = 0;
            m_Front = m_Capacity - 1;
            m_Back = 0;
        }
    }

    /**
     * @brief Reserve memory for the array ahead of time.
     *
     * This does not change the size of the array. It can potentially reduce the number of allocations.
     *
     * @param p_Capacity
     */
    constexpr void Reserve(const SizeType p_Capacity)
    {
        if (p_Capacity > m_Capacity)
            modifyCapacity(p_Capacity);
    }

    constexpr const ValueType *GetData() const
    {
        return m_Data;
    }
    constexpr ValueType *GetData()
    {
        return m_Data;
    }

    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr const ValueType &At(const SizeType p_Index) const
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][DYN-DEQUE] Cannot index into an empty queue");
        return *(GetData() + p_Index);
    }
    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr ValueType &At(const SizeType p_Index)
    {
        return *(GetData() + p_Index);
    }

    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr const ValueType &operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }

    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr ValueType &operator[](const SizeType p_Index)
    {
        return At(p_Index);
    }

    /**
     * @brief Get the index of the front element.
     *
     * Useful as an iteration starting point.
     *
     */
    constexpr SizeType GetFrontIndex() const
    {
        return NextIndex(m_Front);
    }
    /**
     * @brief Get the index of the back element.
     *
     * Useful as a reverse iteration starting point.
     *
     */
    constexpr SizeType GetBackIndex() const
    {
        return PrevIndex(m_Back);
    }
    /**
     * @brief Get the bound located at the front of the queue.
     *
     * Useful as a reverse iteration stop condition.
     *
     */
    constexpr SizeType GetFrontEnd() const
    {
        return m_Front;
    }
    /**
     * @brief Get the bound located at the back of the queue.
     *
     * Useful as an iteration stop condition.
     *
     */
    constexpr SizeType GetBackEnd() const
    {
        return m_Back;
    }

    /**
     * @brief Get the next buffer index, cycling if necessary.
     *
     * Useful when iterating.
     *
     * @return The next index.
     */
    constexpr SizeType NextIndex(const SizeType p_Index) const
    {
        if (p_Index == m_Capacity - 1)
            return 0;
        return p_Index + 1;
    }

    /**
     * @brief Get the previous buffer index, cycling if necessary.
     *
     * Useful when iterating.
     *
     * @return The previous index.
     */
    constexpr SizeType PrevIndex(const SizeType p_Index) const
    {
        if (p_Index == 0)
            return m_Capacity - 1;
        return p_Index - 1;
    }

    constexpr const ValueType &GetFront() const
    {
        return At(GetFrontIndex());
    }
    constexpr const ValueType &GetBack() const
    {
        return At(GetBackIndex());
    }
    constexpr ValueType &GetFront()
    {
        return At(GetFrontIndex());
    }
    constexpr ValueType &GetBack()
    {
        return At(GetBackIndex());
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
        return m_Size == m_Capacity;
    }

  private:
    constexpr void modifyCapacity(const SizeType p_Capacity)
    {
        TKIT_ASSERT(p_Capacity > 0, "[TOOLKIT][DYN-DEQUE] Capacity must be greater than 0");
        TKIT_ASSERT(p_Capacity >= m_Size, "[TOOLKIT][DYN-DEQUE] Capacity is smaller than size");
        ValueType *newData =
            static_cast<ValueType *>(Memory::AllocateAligned(p_Capacity * sizeof(ValueType), alignof(ValueType)));
        TKIT_ASSERT(newData, "[TOOLKIT][DYN-DEQUE] Failed to allocate memory");

        if (m_Data)
        {
            for (u32 i = GetFrontIndex(), j = 0; j < m_Size; i = NextIndex(i), ++j)
            {
                Memory::ConstructFromIterator(newData + j, std::move(At(i)));
                if constexpr (!std::is_trivially_destructible_v<T>)
                    Memory::DestructFromIterator(GetData() + i);
            }
            Memory::DeallocateAligned(m_Data);
        }
        m_Data = newData;
        m_Capacity = p_Capacity;
        m_Front = p_Capacity - 1;
        m_Back = m_Size;
    }

    constexpr void deallocateBuffer()
    {
        TKIT_ASSERT(m_Size == 0, "[TOOLKIT][DYN-DEQUE] Cannot deallocate buffer while it is not empty");
        if (m_Data)
        {
            Memory::DeallocateAligned(m_Data);
            m_Data = nullptr;
            m_Capacity = 0;
        }
    }

    constexpr void growCapacity(const SizeType p_Size)
    {
        modifyCapacity(Tools::GrowthFactor(p_Size));
    }

    ValueType *m_Data = nullptr;
    SizeType m_Size = 0;
    SizeType m_Capacity = 0;
    SizeType m_Front = 0;
    SizeType m_Back = 0;
};
} // namespace TKit
